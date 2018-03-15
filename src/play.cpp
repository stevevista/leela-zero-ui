


#include "gtp_choice.h"
#include "gtp_game.hpp"
#include <fstream>
#include <cassert>
#include <cstring>
#include <sstream>
#include <csignal>
#include <iostream>

using namespace std;

#ifndef NO_GUI_SUPPORT
#include "gui/board_ui.h"
#endif
#include "tools.h"

static constexpr int default_board_size = 19;


static bool opt_comupter_is_black = true;
static bool opt_hint = true;
static bool opt_uionly = false;
static bool opt_noui = false;

constexpr int wait_time_secs = 40;


void autogtpui();
int gtp(const string& cmdline, const string& selfpath);
int advisor(const string& cmdline, const string& selfpath);
int playMatch(int rounds, const string& selfpath, const std::vector<string>& players);


int main(int argc, char **argv) {

    GTP::setup_default_parameters();
    
    string selfpath = argv[0];
    
    auto pos  = selfpath.rfind(
        #ifdef _WIN32
        '\\'
        #else
        '/'
        #endif
        );

    selfpath = selfpath.substr(0, pos); 

    std::vector<string> players;
    int rounds = 1;

    for (int i=1; i<argc; i++) {
        string opt = argv[i];

        if (opt == "--help" || opt == "-h") {
            cout << endl;
            cout << "--gtp | -g" << endl;
            cout << "--human, computer is WHITE" << endl;
            cout << "--hint" << endl;
            cout << "--ui-only" << endl;
            cout << endl;

            cout << "--player <gtp engine command line or weights file>" << endl;
            cout << "  1) multiple --player arguments for match" << endl;
            cout << "  2) if not specified, use built-in LeelaZero engine" << endl;
            cout << endl;
            cout << "--weights <weights file> | -w <weights file>" << endl;
            cout << "  if not specified, auto search in local directory" << endl;
            cout << endl;
            cout << "example:" << endl;
            cout << "./leelazui --player ./AQ -w ./best_v.txt, AQ(B) vs built-in engine with weights best_v1" << endl;
            cout << "./leelazui -w ./best_v.txt --player ./AQ, AQ(W) vs built-in engine with weights best_v1" << endl;
            cout << "./leelazui -w ./best_v.txt --player ./best_v.txt ... -p 1600 --noponder, bulit-in leelaz vs leelaz" << endl;
            cout << "./leelazui, play with LeelaZero engine (auto search best weights)" << endl;
            cout << "./leelazui --player ./AQ, play with AQ engine" << endl;
            cout << "./autogtp | ./leelazui --ui-only, use as autogtp ui" << endl;
            return 0;

        }

        if (opt == "--human") {
            opt_comupter_is_black = false;
        }
        else if (opt == "--play") {
            opt_hint = false;
        }
        else if (opt == "--ui-only") {
            opt_uionly = true;
        }
        else if (opt == "--noui") {
            opt_noui = true;
        }
        else if (opt == "--rounds") {
            rounds = stoi(argv[++i]);
        }
    }

    if (!opt_uionly)
        parseLeelaZeroArgs(argc, argv, players);

    if (players.size() && players[0].empty()) {
        fprintf(stderr, "RNG seed: %llu\n", cfg_rng_seed);
    }

    if (cfg_gtp_mode) {
        if (players.empty()) {
            fprintf(stderr, "A network weights file is required to use the program.\n");
            throw std::runtime_error("A network weights file is required to use the program");
        }
        gtp(players[0], selfpath);
    }
    else if (players.size() > 1) {
        playMatch(rounds, selfpath, players);
    }
    else if (players.size() == 1) {
        advisor(players[0], selfpath);
    }
    else {
#ifndef NO_GUI_SUPPORT
        autogtpui();
#endif
    }

    return 0;
}

#ifndef NO_GUI_SUPPORT
static shared_ptr<go_window> create_ui() {
    if (opt_noui)
        return shared_ptr<go_window>();

    try {
        return make_shared<go_window>();
    } catch(...) {
        cerr << "Failed to initialize Window system, UI disabled" << endl;
        return shared_ptr<go_window>();
    }
}

#endif

int gtp(const string& cmdline, const string& selfpath) {

    GtpChoice agent;

    function<bool()> check_gui_closed = [&]() { return false; };
    function<void()> close_window = [&]() {};
    function<void()> wait_until_closed = [&]() {};
    function<void()> toggle_ui = [&]() {}; 

#ifndef NO_GUI_SUPPORT

    bool showing = true;
    auto board_ui = create_ui();

    if (board_ui) {

        board_ui->setMoveClickHandler([&](bool black, int pos) {
            agent.send_command("play " + string(black? "b " :"w ") + agent.move_to_text(pos));
            return false; // no commit
        });

        agent.onPlayChange = [&](bool black, int pos) {

            if (pos == GtpState::move_undo) {
                board_ui->undo(black);
            }
            else
                board_ui->update(black, pos);
        };

        check_gui_closed = [&]() {
            if (board_ui->is_closed()) {
                agent.send_command("quit");
                return true;
            }
            return false;
        };

        close_window = [&]() {
            board_ui->close_window();
        };

        wait_until_closed = [&]() {
            board_ui->wait_until_closed();
        };

        toggle_ui = [&]() {
            if (showing)
                board_ui->hide();
            else 
                board_ui->show();
            showing = !showing;
        }; 
    }

#endif  


    agent.onOutput = [&](const string& line) {
        cout << line;
    };

    if (cmdline.empty())
        agent.execute();
    else
        agent.execute(cmdline, selfpath, wait_time_secs);

    // User player input
    std::thread([&]() {

		while(true) {
			std::string input_str;
			getline(std::cin, input_str);
            
            if (input_str == "ui")
                toggle_ui();
            else
                agent.send_command(input_str);
            if (input_str == "quit") {
                close_window();
                break;
            }
		}

	}).detach();

    wait_until_closed();
    check_gui_closed();
    agent.join();
    return 0;
}

int advisor(const string& cmdline, const string& selfpath) {

    GameAdvisor<GtpChoice> agent;
    //GameAdvisor agent("/home/steve/dev/app/leelaz -w /home/steve/dev/data/weights.txt -g");
    
    function<bool()> check_gui_closed = [&]() { return false; };
    function<void()> close_window = [&]() {};
    function<void()> wait_until_closed = [&]() {};
    function<void()> toggle_ui = [&]() {}; 

#ifndef NO_GUI_SUPPORT
    bool showing = true;
    auto board_ui = create_ui();

    if (board_ui) {

        board_ui->setMoveClickHandler([&](bool black, int pos) {
            if (black == !opt_comupter_is_black) {
                agent.place(black, pos);
                return true;
            }
            else
                return false; //no commit
        });

        agent.onPlayChange = [&](bool black, int pos) {

            if (black != opt_comupter_is_black)
                return;

            if (pos == GtpState::move_undo) {
                board_ui->undo(black);
            }
            else
                board_ui->update(black, pos);
        };

        check_gui_closed = [&]() {
            if (board_ui->is_closed()) {
                agent.send_command("quit");
                return true;
            }
            return false;
        };

        close_window = [&]() {
            board_ui->close_window();
        };

        wait_until_closed = [&]() {
            board_ui->wait_until_closed();
        };
        
        toggle_ui = [&]() {
            if (showing)
                board_ui->hide();
            else 
                board_ui->show();
            showing = !showing;
        }; 
    }

#endif    

    agent.onGtpIn = [](const string& line) {
        cout << line << endl;
    };

    agent.onGtpOut = [&](const string& line) {
        cout << line;
    };

    agent.set_init_cmds({"time_settings 1800 15 1"});
    if (opt_hint)
        agent.hint_both(true);


    agent.onThinkMove = [&](bool black, int move, const std::vector<genmove_stats>& dist) {

        if (black == opt_comupter_is_black) {
            agent.place(black, move);
        }
        else {
#ifndef NO_GUI_SUPPORT
            if (board_ui)
                board_ui->indicate(move);
#endif    
            cout << "suggest move: " << agent.move_to_text(move) << endl;
            for (auto p : dist) {
                cout << "P: " << p.score << endl;

            }
        }
    };

    if (cmdline.empty())
        agent.execute();
    else
        agent.execute(cmdline, selfpath, wait_time_secs);

    // User player input
    std::thread([&]() {

		while(true) {
			std::string input_str;
			getline(std::cin, input_str);

            auto pos = agent.text_to_move(input_str);
            if (pos != GtpState::invalid_move) {
                agent.place(agent.next_move_is_black(), pos);
            }
            else if (input_str == "ui")
                toggle_ui();
            else
                agent.send_command(input_str);

            if (input_str == "quit") {
                close_window();
                break;
            }
		}

	}).detach();

    // computer play as BLACK
    agent.reset(opt_comupter_is_black);

    while (true) {
        this_thread::sleep_for(chrono::microseconds(100));
        agent.pop_events();
        if (!agent.alive()) {
            close_window();
            break;
        }
        if (check_gui_closed()) 
            break;
    }

    wait_until_closed();
    agent.join();
    return 0;
}


static string move_to_text_sgf(int pos, int bdsize) {
    std::ostringstream result;

    if (pos < 0) {
        result << "tt";
    }
    else {
        int column = pos % bdsize;
        int row = pos / bdsize;

        // SGF inverts rows
        row = bdsize - row - 1;
        result << static_cast<char>('a' + column);
        result << static_cast<char>('a' + row);
    }

    return result.str();
}

int playMatch(int index, GtpChoice& black, GtpChoice& white, function<void()> uiReset, function<void(bool, int)> uiUpdate) {

    bool ok;

    auto me = &black;
    auto other = &white;
    bool black_to_move = true;
    bool last_is_pass = false;
    string result;
    string sgf_moves;

    GtpState::send_command_sync(black, "clear_board");
    GtpState::send_command_sync(white, "clear_board");


    uiReset();


    for (int move_count = 0; move_count<361*2; move_count++) {
            
        auto vtx = GtpState::send_command_sync(*me, black_to_move ? "genmove b" : "genmove w", ok);
        if (!ok)
            throw runtime_error("unexpect error while genmove");

        auto move = me->text_to_move(vtx);
        auto movestr = move_to_text_sgf(move, black.boardsize());
        if (black_to_move)
            sgf_moves.append(";B[" + movestr + "]");
        else    
            sgf_moves.append(";W[" + movestr + "]");

        if (move_count % 10 == 0) {
            sgf_moves.append("\n");
        }

        if (vtx == "resign") {
            result = black_to_move ? "W+Resign" : "B+Resign";
            break;
        }

        if (vtx == "pass") {
            if (last_is_pass) 
                break;
            last_is_pass = true;
        } else
            last_is_pass = false;
            

        uiUpdate(black_to_move, move);

        GtpState::send_command_sync(*other, (black_to_move ? "play b " : "play w ") + vtx, ok);
        if (!ok)
            throw runtime_error("unexpect error while play");

        auto tmp = me;
        me = other;
        other = tmp;

        black_to_move = !black_to_move;
    }
 
        // a game is over 
    if (result.empty()) {
        result = GtpState::send_command_sync(black, "final_score", ok);
    }

    std::cout << "Result: " << result << std::endl;

    string sgf_string;
    float komi = 7.5;
    int size = black.boardsize();
    time_t now;
    time(&now);
    char timestr[sizeof "2017-10-16"];
    strftime(timestr, sizeof timestr, "%F", localtime(&now));

    sgf_string.append("(;GM[1]FF[4]RU[Chinese]");
    sgf_string.append("DT[" + std::string(timestr) + "]");
    sgf_string.append("SZ[" + std::to_string(size) + "]");
    sgf_string.append("KM[7.5]");
    sgf_string.append("RE[" + result + "]");
    sgf_string.append("\n");
    sgf_string.append(sgf_moves);
    sgf_string.append(")\n");

    std::ofstream ofs("selfplay.sgf");
    ofs << sgf_string;
    ofs.close();

    if (result[0] == 'B')
        return 1;
    else
        return -1;
}

int playMatch(int rounds, const string& selfpath, const std::vector<string>& players) {

    GtpChoice black;
    GtpChoice white;

    black.onInput = [](const string& line) {
        cout << line << endl;;
    };

    black.onOutput = [](const string& line) {
        cout << line;
    };

    if (players[0].empty())
        black.execute();
    else
        black.execute(players[0], selfpath, wait_time_secs);

    if (!black.isReady()) {
        std::cerr << "cannot start player 1" << std::endl;
        return -1;
    }

    if (players[1].empty())
        white.execute();
    else
        white.execute(players[1], selfpath, wait_time_secs);
    if (!white.isReady()) {
        std::cerr << "cannot start player 2" << std::endl;
        std::cerr << players[1] << endl;
        return -1;
    }

    if (default_board_size != 19) {
        bool ok;
        GtpState::send_command_sync(black, "boardsize " + to_string(default_board_size), ok);
        if (!ok) {
            std::cerr << "player 1 do not support size " << default_board_size << std::endl;
            return -1;
        }

        GtpState::send_command_sync(white, "boardsize " + to_string(default_board_size), ok);
        if (!ok) {
            std::cerr << "player 2 do not support size " << default_board_size << std::endl;
            return -1;
        }
    }

    function<void()> uiReset = [&] {
    };

    function<void(bool, int)> uiUpdate = [&](bool, int) {
    };

#ifndef NO_GUI_SUPPORT

    auto board_ui = create_ui();

    if (board_ui) {
        board_ui->enable_play_mode(false);
        board_ui->reset(black.boardsize());
        
        uiReset = [&] {
            board_ui->reset();
        };

        uiUpdate = [&](bool black, int move) {
            board_ui->update(black, move);
        };
    }
    
#endif

    int wins = 0;
    for (int i=0; i<rounds; i++) {

        int sel = 0;
        int re;
        if (rounds > 1)
            sel = ::rand()%2;

        if (sel == 0) {
            re = playMatch(i, black, white, uiReset, uiUpdate);
        } else {
            re = playMatch(i, white, black, uiReset, uiUpdate);
            re *= -1;
        }

        if (re == 1)
            wins++;
    }

    cout << wins << "/" << rounds << endl;
    
#ifndef NO_GUI_SUPPORT
    if (board_ui)
        board_ui->wait_until_closed();
#endif

    GtpState::wait_quit(black);
    GtpState::wait_quit(white);
}

