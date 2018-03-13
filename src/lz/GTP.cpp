/*
    This file is part of Leela Zero.
    Copyright (C) 2017 Gian-Carlo Pascutto

    Leela Zero is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leela Zero is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Leela Zero.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include "GTP.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <sstream>
#include <cstdarg>

#include "FastBoard.h"
#include "FullBoard.h"
#include "GameState.h"
#include "Network.h"
#include "SMP.h"
#include "UCTSearch.h"
#include "Utils.h"
#include "Zobrist.h"
#include "NNCache.h"

using namespace Utils;

// Configuration flags
bool cfg_gtp_mode;
bool cfg_allow_pondering;
int cfg_num_threads;
int cfg_max_threads;
int cfg_max_playouts;
int cfg_max_visits;
TimeManagement::enabled_t cfg_timemanage;
int cfg_lagbuffer_cs;
int cfg_resignpct;
std::uint64_t cfg_rng_seed;
bool cfg_dumbpass;
#ifdef USE_OPENCL
std::vector<int> cfg_gpus;
bool cfg_sgemm_exhaustive;
bool cfg_tune_only;
#endif
float cfg_puct;
float cfg_softmax_temp;
float cfg_fpu_reduction;
std::string cfg_weightsfile;
std::string cfg_logfile;
FILE* cfg_logfile_handle;
bool cfg_quiet;


void GTP::setup_default_parameters() {

    cfg_gtp_mode = false;
    cfg_allow_pondering = true;
    cfg_max_threads = std::max(1, std::min(SMP::get_num_cpus(), MAX_CPUS));
#ifdef USE_OPENCL
    // If we will be GPU limited, using many threads won't help much.
    cfg_num_threads = std::min(2, cfg_max_threads);
#else
    cfg_num_threads = cfg_max_threads;
#endif
    cfg_max_playouts = std::numeric_limits<decltype(cfg_max_playouts)>::max();
    cfg_max_visits = std::numeric_limits<decltype(cfg_max_visits)>::max();
    cfg_timemanage = TimeManagement::AUTO;
    cfg_lagbuffer_cs = 100;
#ifdef USE_OPENCL
    cfg_gpus = { };
    cfg_sgemm_exhaustive = false;
    cfg_tune_only = false;
#endif
    cfg_puct = 0.8f;
    cfg_softmax_temp = 1.0f;
    cfg_fpu_reduction = 0.25f;
    // see UCTSearch::should_resign
    cfg_resignpct = -1;
    cfg_dumbpass = false;
    cfg_logfile_handle = nullptr;
    cfg_quiet = false;

    // C++11 doesn't guarantee *anything* about how random this is,
    // and in MinGW it isn't random at all. But we can mix it in, which
    // helps when it *is* high quality (Linux, MSVC).
    std::random_device rd;
    std::ranlux48 gen(rd());
    std::uint64_t seed1 = (gen() << 16) ^ gen();
    // If the above fails, this is one of our best, portable, bets.
    std::uint64_t seed2 = std::chrono::high_resolution_clock::
        now().time_since_epoch().count();
    cfg_rng_seed = seed1 ^ seed2;
}

std::string GTP::get_life_list(const GameState & game, bool live) {
    std::vector<std::string> stringlist;
    std::string result;
    const auto& board = game.board;

    if (live) {
        for (int i = 0; i < board.get_boardsize(); i++) {
            for (int j = 0; j < board.get_boardsize(); j++) {
                int vertex = board.get_vertex(i, j);

                if (board.get_square(vertex) != FastBoard::EMPTY) {
                    stringlist.push_back(board.get_string(vertex));
                }
            }
        }
    }

    // remove multiple mentions of the same string
    // unique reorders and returns new iterator, erase actually deletes
    std::sort(stringlist.begin(), stringlist.end());
    stringlist.erase(std::unique(stringlist.begin(), stringlist.end()),
                     stringlist.end());

    for (size_t i = 0; i < stringlist.size(); i++) {
        result += (i == 0 ? "" : "\n") + stringlist[i];
    }

    return result;
}



bool GTP::input_pending_ = false;

void GTP::send_command(const std::string& cmd, function<void(bool, const string&)> handler) {
    command_queue_.push({cmd, handler});
}


// Setup global objects after command line has been parsed
void init_global_objects() {

    static bool inited = false;
    if (inited)
        return;

    inited = true;

    thread_pool.initialize(cfg_num_threads);

    // Use deterministic random numbers for hashing
    auto rng = std::make_unique<Random>(5489);
    Zobrist::init_zobrist(*rng);

    // Initialize the main thread RNG.
    // Doing this here avoids mixing in the thread_id, which
    // improves reproducibility across platforms.
    Random::get_Rng().seedrandom(cfg_rng_seed);

    NNCache::get_NNCache().set_size_from_playouts(cfg_max_playouts);

    // Initialize network
    Network::initialize();
}

static const vector<string> s_commands = {
        "protocol_version",
        "name",
        "version",
        "quit",
        "known_command",
        "list_commands",
        "boardsize",
        "clear_board",
        "komi",
        "play",
        "genmove",
        "undo",
        "final_score",
        "final_status_list",
        "time_settings",
        "time_left",
        "fixed_handicap",
        "place_free_handicap",
        "set_free_handicap"
    };

bool GTP::support(const string& cmd) {
    
    return find(s_commands.begin(), s_commands.end(), cmd) != s_commands.end();  
}

void GTP::run() {

    clean_up();

    function<void(bool, const string&)> handler;

    init_global_objects();

    game = std::make_unique<GameState>();

    /* set board limits */
    auto komi = 7.5f;
    game->init_game(BOARD_SIZE, komi);

    search = std::make_unique<UCTSearch>(*game);

    ready_ = true;

    auto gtp_vprint = [&](bool error, const char *fmt, va_list ap) {
		
        char buffer[4096];
        int n = vsprintf(buffer, fmt, ap);
        auto rsp = string(buffer, n);

        if (onOutput) {
            onOutput((error ? "? " : "= ") + rsp + "\n\n");
        }

        if (handler) {
            handler(!error, rsp);
        }
	};

	auto gtp_print = [&](const char *fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		gtp_vprint(false, fmt, ap);
		va_end(ap);
	};

	auto gtp_fail = [&](const char *fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		gtp_vprint(true, fmt, ap);
		va_end(ap);
    };


    bool pondering = false;
    string command;

    if (onReset)
        onReset();

    for (;;) {

        
        std::thread read_th([&] {
            string input;
			while(input == "" || input == "#") {

                command_t cmd;
				command_queue_.wait_and_pop(cmd);
                auto xinput = cmd.cmd;
                handler = cmd.handler;

                input = "";
                /* eat empty lines, simple preprocessing, lower case */
                for (unsigned int tmp = 0; tmp < xinput.size(); tmp++) {
                    if (xinput[tmp] == 9) {
                        input += " ";
                    } else if ((xinput[tmp] > 0 && xinput[tmp] <= 9)
                        || (xinput[tmp] >= 11 && xinput[tmp] <= 31)
                        || xinput[tmp] == 127) {
                    continue;
                    } else {
                        input += std::tolower(xinput[tmp]);
                    }

                    // eat multi whitespace
                    if (input.size() > 1) {
                        if (std::isspace(input[input.size() - 2]) &&
                            std::isspace(input[input.size() - 1])) {
                            input.resize(input.size() - 1);
                        }
                    }
                }
            }

            if (onInput)
                onInput(input);

            stop_ponder();
            pondering = false;

            if (std::isdigit(input[0])) {
				std::istringstream strm(input);
				char spacer;
                int id;
				strm >> id;
				strm >> std::noskipws >> spacer;
				std::getline(strm, command);
			} else {
				command = input;
			}
        });

        input_pending_ = false;

        if (pondering && cfg_allow_pondering && !game->has_resigned()) {
            search->ponder();
        }

        read_th.join();

        /* process commands */
        if (command == "protocol_version") {
            gtp_print("%d", GTP_VERSION);
        } else if (command == "name") {
            gtp_print(PROGRAM_NAME);
        } else if (command == "version") {
            gtp_print(PROGRAM_VERSION);
        } else if (command == "quit") {
            gtp_print("");
            return;
        }  else if (command.find("known_command") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;

            cmdstream >> tmp;     /* remove known_command */
            cmdstream >> tmp;

            bool found = find(s_commands.begin(), s_commands.end(), tmp) != s_commands.end();
            gtp_print(found ? "true" : "false");
 
        } else if (command.find("list_commands") == 0) {
            std::string outtmp(s_commands[0]);
            for (int i = 1; s_commands[i].size() > 0; i++) {
                outtmp = outtmp + "\n" + s_commands[i];
            }
            gtp_print(outtmp.c_str());

        } else if (command.find("boardsize") == 0) {
            std::istringstream cmdstream(command);
            std::string stmp;
            int tmp;

            cmdstream >> stmp;  // eat boardsize
            cmdstream >> tmp;

            if (!cmdstream.fail()) {
                if (tmp != BOARD_SIZE) {
                    gtp_fail("unacceptable size");
                } else {
                    board_size_ = tmp;
                    float old_komi = game->get_komi();
                    game->init_game(tmp, old_komi);
                    gtp_print("");
                }
            } else {
                gtp_fail("syntax not understood");
            }
        } else if (command.find("clear_board") == 0) {
            game->reset_game();
            search = std::make_unique<UCTSearch>(*game);
            clean_board();
            if (onReset)
                onReset();
            gtp_print("");
        } else if (command.find("komi") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;
            float komi = 7.5f;
            float old_komi = game->get_komi();

            cmdstream >> tmp;  // eat komi
            cmdstream >> komi;

            if (!cmdstream.fail()) {
                if (komi != old_komi) {
                    game->set_komi(komi);
                }
                gtp_print("");
            } else {
                gtp_fail("syntax not understood");
            }

        } else if (command.find("play") == 0) {
            if (command.find("resign") != std::string::npos) {
                bool is_black = game->get_to_move() == FastBoard::BLACK;
                game->play_move(FastBoard::RESIGN);
                history_moves_.push_back({is_black, -2});
                gtp_print("");
            } else if (command.find("pass") != std::string::npos) {
                bool is_black = game->get_to_move() == FastBoard::BLACK;
                game->play_move(FastBoard::PASS);
                history_moves_.push_back({is_black, -1});
                gtp_print("");
            } else {
                std::istringstream cmdstream(command);
                std::string tmp;
                std::string color, vertex;

                cmdstream >> tmp;   //eat play
                cmdstream >> color;
                cmdstream >> vertex;

                if (!cmdstream.fail()) {
                    if (!game->play_textmove(color, vertex)) {
                        gtp_fail("illegal move");
                    } else {
                        bool is_black = color[0] == 'b';
                        auto xy = game->board.get_xy(game->get_last_move());
                        auto pos = (xy.second)*board_size_ + xy.first;
                        history_moves_.push_back({is_black, pos});
                        gtp_print("");
                    }
                } else {
                    gtp_fail("syntax not understood");
                }
            }
        } else if (command.find("genmove") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;

            cmdstream >> tmp;  // eat genmove
            cmdstream >> tmp;

            if (!cmdstream.fail()) {
                int who;
                if (tmp == "w" || tmp == "white") {
                    who = FastBoard::WHITE;
                } else if (tmp == "b" || tmp == "black") {
                    who = FastBoard::BLACK;
                } else {
                    gtp_fail("syntax error");
                    continue;
                }
                // start thinking
                {
                    game->set_to_move(who);
                    int move = search->think(who);
                    game->play_move(move);

                    bool is_black = who == FastBoard::BLACK;
                    int pos;
                    if (move == FastBoard::RESIGN) pos = -2;
                    else if (move == FastBoard::PASS) pos = -1;
                    else {
                        auto xy = game->board.get_xy(game->get_last_move());
                        pos = (xy.second)*board_size_ + xy.first;
                    }
                    history_moves_.push_back({is_black, pos});

                    std::string vertex = game->move_to_text(move);
                    gtp_print("%s", vertex.c_str());
                }

                pondering = true;

            } else {
                gtp_fail("syntax not understood");
            }

        } else if (command.find("undo") == 0) {
            if (game->undo_move()) {
                if (history_moves_.size())
                    history_moves_.erase(history_moves_.end()-1);
                gtp_print("");
            } else {
                gtp_fail("cannot undo");
            }
        } else if (command.find("final_score") == 0) {
            float ftmp = game->final_score();
            /* white wins */
            if (ftmp < -0.1) {
                gtp_print("W+%3.1f", (float)fabs(ftmp));
            } else if (ftmp > 0.1) {
                gtp_print("B+%3.1f", ftmp);
            } else {
                gtp_print("0");
            }

        } else if (command.find("final_status_list") == 0) {
            if (command.find("alive") != std::string::npos) {
                std::string livelist = get_life_list(*game, true);
                gtp_print(livelist.c_str());
            } else if (command.find("dead") != std::string::npos) {
                std::string deadlist = get_life_list(*game, false);
                gtp_print(deadlist.c_str());
            } else {
                gtp_print("");
            }

        } else if (command.find("time_settings") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;
            int maintime, byotime, byostones;

            cmdstream >> tmp >> maintime >> byotime >> byostones;

            if (!cmdstream.fail()) {
                // convert to centiseconds and set
                game->set_timecontrol(maintime * 100, byotime * 100, byostones, 0);

                gtp_print("");
            } else {
                gtp_fail("syntax not understood");
            }

        } else if (command.find("time_left") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp, color;
            int time, stones;

            cmdstream >> tmp >> color >> time >> stones;

            if (!cmdstream.fail()) {
                int icolor;

                if (color == "w" || color == "white") {
                    icolor = FastBoard::WHITE;
                } else if (color == "b" || color == "black") {
                    icolor = FastBoard::BLACK;
                } else {
                    gtp_fail("Color in time adjust not understood.\n");
                    continue;
                }

                game->adjust_time(icolor, time * 100, stones);

                gtp_print("");
                pondering = true;

            } else {
                gtp_fail("syntax not understood");
            }

        } else if (command.find("fixed_handicap") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;
            int stones;

            cmdstream >> tmp;   // eat fixed_handicap
            cmdstream >> stones;

            if (!cmdstream.fail() && game->set_fixed_handicap(stones)) {
                auto stonestring = game->board.get_stone_list();
                gtp_print("%s", stonestring.c_str());

                handicaps_.clear();
                for (int j = 0; j < board_size_; j++) {
                    for (int i = 0; i < board_size_; i++) {
                        if (game->board.get_square(i,j) == FastBoard::BLACK) {
                            handicaps_.push_back(j*board_size_ + i);
                        }
                    }
                }

            } else {
                gtp_fail("Not a valid number of handicap stones");
            }

        } else if (command.find("place_free_handicap") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;
            int stones;

            cmdstream >> tmp;   // eat place_free_handicap
            cmdstream >> stones;

            if (!cmdstream.fail()) {
                game->place_free_handicap(stones);
                auto stonestring = game->board.get_stone_list();
                gtp_print("%s", stonestring.c_str());

                handicaps_.clear();
                for (int j = 0; j < board_size_; j++) {
                    for (int i = 0; i < board_size_; i++) {
                        if (game->board.get_square(i,j) == FastBoard::BLACK) {
                            handicaps_.push_back(j*board_size_ + i);
                        }
                    }
                }

            } else {
                gtp_fail("Not a valid number of handicap stones");
            }

        } else if (command.find("set_free_handicap") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;

            cmdstream >> tmp;   // eat set_free_handicap

            do {
                std::string vertex;

                cmdstream >> vertex;

                if (!cmdstream.fail()) {
                    if (!game->play_textmove("black", vertex)) {
                        gtp_fail("illegal move");
                    } else {
                        game->set_handicap(game->get_handicap() + 1);
                    }
                }
            } while (!cmdstream.fail());

            std::string stonestring = game->board.get_stone_list();
            gtp_print("%s", stonestring.c_str());

            handicaps_.clear();
            for (int j = 0; j < board_size_; j++) {
                for (int i = 0; i < board_size_; i++) {
                    if (game->board.get_square(i,j) == FastBoard::BLACK) {
                        handicaps_.push_back(j*board_size_ + i);
                    }
                }
            }

        } else {
            gtp_fail("unknown command");
        }
    }
}

string GTP::version() const {
    return PROGRAM_VERSION;
}
