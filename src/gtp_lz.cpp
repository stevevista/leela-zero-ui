#include "gtp_lz.h"
#include <sstream>
#include <cstdarg>
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
#include "lz/Utils.h"
#include "lz/Zobrist.h"
#include "lz/SGFTree.h"
#include "lz/Utils.h"

using namespace Utils;

bool Utils::input_pending(void) {
    return GtpLZ::input_pending();
}

static std::mutex IOmutex;

void Utils::myprintf(const char *fmt, ...) {
    if (cfg_quiet) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    
    if (!GtpLZ::vstderr(fmt, ap))
        vfprintf(stderr, fmt, ap);

    va_end(ap);

    if (cfg_logfile_handle) {
        std::lock_guard<std::mutex> lock(IOmutex);
        va_start(ap, fmt);
        vfprintf(cfg_logfile_handle, fmt, ap);
        va_end(ap);
    }
}

void UCTSearch::stop_think() {
    m_run = false;
}


std::string GtpLZ::get_life_list(const GameState & game, bool live) {
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

    // Initialize network
    Network::initialize();
}


bool GtpLZ::input_pending_ = false;

static GtpLZ* gtp_inst = nullptr;

bool GtpLZ::vstderr(const char *fmt, va_list ap) {

    if (gtp_inst && gtp_inst->onStderr) {
        char buf[4096];
        vsprintf(buf, fmt, ap);
        gtp_inst->onStderr(buf);
        return true;
    }
    return false;
}



GtpLZ::GtpLZ()
{
    gtp_inst = this;
}

GtpLZ::~GtpLZ() {
    gtp_inst = nullptr;
}


void GtpLZ::send_command(const std::string& cmd, function<void(bool, const string&)> handler) {
    command_queue_.push({cmd, handler});
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
        "set_free_handicap",
        "loadsgf",
        "printsgf",
        "kgs-genmove_cleanup",
        "kgs-time_settings",
        "kgs-game_over",
        "heatmap"
    };

bool GtpLZ::support(const string& cmd) {
    
    return find(s_commands.begin(), s_commands.end(), cmd) != s_commands.end();  
}

void GtpLZ::run() {

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

    for (;;) {

        
        std::thread read_th([&] {
            string input;
			while(input == "" || input == "#") {

                command_t cmd;
				command_queue_.wait_and_pop(cmd);
                auto xinput = cmd.cmd;
                handler = cmd.handler;

                bool transform_lowercase = true;

                // Required on Unixy systems
                if (xinput.find("loadsgf") != std::string::npos) {
                    transform_lowercase = false;
                }

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
                        if (transform_lowercase) {
                            input += std::tolower(xinput[tmp]);
                        } else {
                            input += xinput[tmp];
                        }
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
            for (auto i = size_t{1}; i < s_commands.size(); i++) {
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
                add_move(is_black, -2);
                gtp_print("");
            } else if (command.find("pass") != std::string::npos) {
                bool is_black = game->get_to_move() == FastBoard::BLACK;
                game->play_move(FastBoard::PASS);
                add_move(is_black, -1);
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
                        add_move(is_black, pos);
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
                    add_move(is_black, pos);

                    std::string vertex = game->move_to_text(move);
                    gtp_print("%s", vertex.c_str());
                }

                pondering = true;

            } else {
                gtp_fail("syntax not understood");
            }

        } else if (command.find("undo") == 0) {
            if (game->undo_move()) {
                undo_move();
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
                            add_handicap(j*board_size_ + i);
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
                            add_handicap(j*board_size_ + i);
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
                        add_handicap(j*board_size_ + i);
                    }
                }
            }

        } else if (command.find("loadsgf") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp, filename;
            int movenum;

            cmdstream >> tmp;   // eat loadsgf
            cmdstream >> filename;

            if (!cmdstream.fail()) {
                cmdstream >> movenum;

                if (cmdstream.fail()) {
                    movenum = 999;
                }
            } else {
                gtp_fail("Missing filename.");
                continue;
            }

            auto sgftree = std::make_unique<SGFTree>();

            try {
                sgftree->load_from_file(filename);
                *game = sgftree->follow_mainline_state(movenum - 1);
                gtp_print("");
            } catch (const std::exception&) {
                gtp_fail("cannot load file");
            }

        }  else if (command.find("printsgf") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp, filename;

            cmdstream >> tmp;   // eat printsgf
            cmdstream >> filename;

            auto sgf_text = SGFTree::state_to_string(*game, 0);

            if (cmdstream.fail()) {
                gtp_print("%s\n", sgf_text.c_str());
            } else {
                std::ofstream out(filename);
                out << sgf_text;
                out.close();
                gtp_print("");
            }
        }  else if (command.find("netbench") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;
            int iterations;

            cmdstream >> tmp;  // eat netbench
            cmdstream >> iterations;

            if (!cmdstream.fail()) {
                Network::benchmark(game.get(), iterations);
            } else {
                Network::benchmark(game.get());
            }
            gtp_print("");

        }  else if (command.find("kgs-time_settings") == 0) {
            // none, absolute, byoyomi, or canadian
            std::istringstream cmdstream(command);
            std::string tmp;
            std::string tc_type;
            int maintime, byotime, byostones, byoperiods;

            cmdstream >> tmp >> tc_type;

            if (tc_type.find("none") != std::string::npos) {
                // 30 mins
                game->set_timecontrol(30 * 60 * 100, 0, 0, 0);
            } else if (tc_type.find("absolute") != std::string::npos) {
                cmdstream >> maintime;
                game->set_timecontrol(maintime * 100, 0, 0, 0);
            } else if (tc_type.find("canadian") != std::string::npos) {
                cmdstream >> maintime >> byotime >> byostones;
                // convert to centiseconds and set
                game->set_timecontrol(maintime * 100, byotime * 100, byostones, 0);
            } else if (tc_type.find("byoyomi") != std::string::npos) {
                // KGS style Fischer clock
                cmdstream >> maintime >> byotime >> byoperiods;
                game->set_timecontrol(maintime * 100, byotime * 100, 0, byoperiods);
            } else {
                gtp_fail("syntax not understood");
                continue;
            }

            if (!cmdstream.fail()) {
                gtp_print("");
            } else {
                gtp_fail("syntax not understood");
            }

        } else if (command.find("kgs-chat") == 0) {
            // kgs-chat (game|private) Name Message
            std::istringstream cmdstream(command);
            std::string tmp;

            cmdstream >> tmp; // eat kgs-chat
            cmdstream >> tmp; // eat game|private
            cmdstream >> tmp; // eat player name
            do {
                cmdstream >> tmp; // eat message
            } while (!cmdstream.fail());

            gtp_fail("I'm a go bot, not a chat bot.");

        } else if (command.find("kgs-game_over") == 0) {
            // Do nothing. Particularly, don't ponder.
            gtp_print("");

        } else if (command.find("auto") == 0) {
            do {
                int move = search->think(game->get_to_move(), UCTSearch::NORMAL);
                game->play_move(move);
                game->display_state();

            } while (game->get_passes() < 2 && !game->has_resigned());

        } else if (command.find("go") == 0) {
            int move = search->think(game->get_to_move());
            game->play_move(move);

            std::string vertex = game->move_to_text(move);
            myprintf("%s\n", vertex.c_str());

        } else if (command.find("heatmap") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;
            int rotation;

            cmdstream >> tmp;   // eat heatmap
            cmdstream >> rotation;

            if (!cmdstream.fail()) {
                auto vec = Network::get_scored_moves(
                    game.get(), Network::Ensemble::DIRECT, rotation, true);
                Network::show_heatmap(game.get(), vec, false);
            } else {
                auto vec = Network::get_scored_moves(
                    game.get(), Network::Ensemble::DIRECT, 0, true);
                Network::show_heatmap(game.get(), vec, false);
            }
            gtp_print("");

        } else if (command.find("showboard") == 0) {
            gtp_print("");
            game->display_state();
        }  else if (command.find("kgs-genmove_cleanup") == 0) {
            std::istringstream cmdstream(command);
            std::string tmp;

            cmdstream >> tmp;  // eat kgs-genmove
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
                game->set_passes(0);
                {
                    game->set_to_move(who);
                    int move = search->think(who, UCTSearch::NOPASS);
                    game->play_move(move);

                    std::string vertex = game->move_to_text(move);
                    gtp_print("%s", vertex.c_str());
                }
                if (cfg_allow_pondering) {
                    // now start pondering
                    if (!game->has_resigned()) {
                        search->ponder();
                    }
                }
            } else {
                gtp_fail("syntax not understood");
            }

        } else {
            gtp_fail("unknown command");
        }
    }
}

string GtpLZ::version() const {
    return PROGRAM_VERSION;
}

