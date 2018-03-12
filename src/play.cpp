


#include "lz/GTP.h"
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
//#include "autogtp.h"

static constexpr int default_board_size = 19;


static bool opt_play_mode = false;
static bool opt_comupter_is_black = true;
static bool opt_advisor_sim = false;
static bool opt_match = false;

constexpr int wait_time_secs = 40;


void autogtpui();
int gtp(const string& cmdline, const string& selfpath);
int advisor(const string& cmdline, const string& selfpath);
int selfplay(const string& selfpath, const vector<string>& players);

#ifndef NO_GUI_SUPPORT
void update_board_by_seqs(go_window& ui, const vector<GtpState::move_t>& seqs) {
    ui.reset(0, false);
    for (auto& m : seqs) {
        ui.update(m.is_black, m.pos, false);
    }
    ui.invalidate();
}

#endif


int main(int argc, char **argv) {

#ifndef NO_GUI_SUPPORT
    if (argc < 2) {
        autogtpui();
        return 0;
    }
#endif

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
    /*
    Order o;
    o.add("network", "./leelaz-model-4937679-16000.txt");
    o.add("leelazVer", "0.11.1");
    o.add("options", "-p 1 --noponder ");
    executeProductionJob(
        o,
        selfpath
    );
    return 0;
    */

    vector<string> players;
    string append_str;

    for (int i=1; i<argc; i++) {
        string opt = argv[i];

        if (opt == "...") {
            for (int j=i+1; j<argc; j++) {
                append_str += " ";
                append_str += argv[j];
            }
            continue;
        }

        if (opt == "--play") {
            opt_play_mode = true;
        }
        else if (opt == "--human") {
            opt_comupter_is_black = false;
        }
        else if (opt == "--advisor-sim") {
            opt_play_mode = true;
            opt_advisor_sim = true;
        }
        else if (opt == "--match") {
            opt_match = true;
        }
        else if (opt == "--player") {
            string player = argv[++i];
            if (player.find(" ") == string::npos && player.find(".txt") != string::npos)
                player = "./leelaz -g -w " + player;
            players.push_back(player);
        }
        else if (opt == "--threads" || opt == "-t") {
                int num_threads = std::stoi(argv[++i]);
                if (num_threads > cfg_num_threads) {
                    fprintf(stderr, "Clamping threads to maximum = %d\n", cfg_num_threads);
                } else if (num_threads != cfg_num_threads) {
                    fprintf(stderr, "Using %d thread(s).\n", num_threads);
                    cfg_num_threads = num_threads;
                }
        } 
        else if (opt == "--playouts" || opt == "-p") {
                cfg_max_playouts = std::stoi(argv[++i]);
        }
            else if (opt == "--noponder") {
                cfg_allow_pondering = false;
            }
            else if (opt == "--visits" || opt == "-v") {
                cfg_max_visits = std::stoi(argv[++i]);
            }
            else if (opt == "--lagbuffer" || opt == "-b") {
                int lagbuffer = std::stoi(argv[++i]);
                if (lagbuffer != cfg_lagbuffer_cs) {
                    fprintf(stderr, "Using per-move time margin of %.2fs.\n", lagbuffer/100.0f);
                    cfg_lagbuffer_cs = lagbuffer;
                }
            }
            else if (opt == "--resignpct" || opt == "-r") {
                cfg_resignpct = std::stoi(argv[++i]);
            }
            else if (opt == "--seed" || opt == "-s") {
                cfg_rng_seed = std::stoull(argv[++i]);
                if (cfg_num_threads > 1) {
                    fprintf(stderr, "Seed specified but multiple threads enabled.\n");
                    fprintf(stderr, "Games will likely not be reproducible.\n");
                }
            }
            else if (opt == "--dumbpass" || opt == "-d") {
                cfg_dumbpass = true;
            }
            else if (opt == "--weights" || opt == "-w") {
                cfg_weightsfile = argv[++i];
                if (players.empty())
                    players.push_back("0");
            }
            else if (opt == "--logfile" || opt == "-l") {
                cfg_logfile = argv[++i];
                fprintf(stderr, "Logging to %s.\n", cfg_logfile.c_str());
                cfg_logfile_handle = fopen(cfg_logfile.c_str(), "a");
            }
            else if (opt == "--quiet" || opt == "-q") {
                cfg_quiet = true;
            }
            #ifdef USE_OPENCL
            else if (opt == "--gpu") {
                cfg_gpus = {std::stoi(argv[++i])};
            }
            #endif
            else if (opt == "--puct") {
                cfg_puct = std::stof(argv[++i]);
            }
            else if (opt == "--softmax_temp") {
                cfg_softmax_temp = std::stof(argv[++i]);
            }
        else if (opt == "--fpu_reduction") {
            cfg_fpu_reduction = std::stof(argv[++i]);
        }
        else if (opt == "--timemanage") {
            std::string tm = argv[++i];
            if (tm == "auto") {
                cfg_timemanage = TimeManagement::AUTO;
            } else if (tm == "on") {
                cfg_timemanage = TimeManagement::ON;
            } else if (tm == "off") {
                cfg_timemanage = TimeManagement::OFF;
            } else {
                fprintf(stderr, "Invalid timemanage value.\n");
                throw std::runtime_error("Invalid timemanage value.");
            }
        }
    }


    if (cfg_timemanage == TimeManagement::AUTO) {
        cfg_timemanage = TimeManagement::ON;
    }

    if (cfg_max_playouts < std::numeric_limits<decltype(cfg_max_playouts)>::max() && cfg_allow_pondering) {
        fprintf(stderr, "Nonsensical options: Playouts are restricted but "
                            "thinking on the opponent's time is still allowed. "
                            "Ponder disabled.\n");
        cfg_allow_pondering = false;
    }
    
    if (cfg_weightsfile.empty() && players.empty()) {
        fprintf(stderr, "A network weights file is required to use the program.\n");
        throw std::runtime_error("A network weights file is required to use the program");
    }

    fprintf(stderr, "RNG seed: %llu\n", cfg_rng_seed);

    if (append_str.size())
        for (auto& line : players) {
            if (line != "0")
                line += append_str;
        }

    

    if (players.size() > 1 || opt_match)
        selfplay(selfpath, players);
    else if (opt_play_mode)
        advisor(players.empty() ? "" : players[0], selfpath);
    else
        gtp(players.empty() ? "" : players[0], selfpath);

    return 0;
}

int gtp(const string& cmdline, const string& selfpath) {

    GtpChoice agent;

#ifndef NO_GUI_SUPPORT
    bool showing = true;
    go_window my_window;

    my_window.onMoveClick = [&](bool black, int pos) {
        agent.send_command("play " + string(black? "b " :"w ") + agent.move_to_text(pos));
        return false; // no commit
    };


    auto check_gui_closed = [&]() {
        if (my_window.is_closed()) {
            agent.send_command("quit");
            return true;
        }
        return false;
    };

    auto close_window = [&]() {
        my_window.close_window();
    };

    auto wait_until_closed = [&]() {
        my_window.wait_until_closed();
    };

    auto sync_ui_board = [&]() {
        update_board_by_seqs(my_window, agent.get_move_sequence());
    };
    
    auto toggle_ui = [&]() {
        if (showing)
            my_window.hide();
        else 
            my_window.show();
        showing = !showing;
    }; 

#else
    auto check_gui_closed = [&]() { return false; };
    auto close_window = [&]() {};
    auto wait_until_closed = [&]() {};
    auto sync_ui_board = [&]() {};
    auto toggle_ui = [&]() {}; 

#endif  


    agent.onOutput = [&](const string& line) {
        sync_ui_board();
        cout << line;
    };

    if (cmdline.empty() || cmdline == "0")
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
    
#ifndef NO_GUI_SUPPORT
    bool showing = true;
    go_window my_window;

    my_window.onMoveClick = [&](bool black, int pos) {
        if (opt_advisor_sim) {
            agent.place(black, pos);
            return true; //commit
        }
        else {
            agent.place(black, pos);
            return false; //no commit
        }
    };


    auto check_gui_closed = [&]() {
        if (my_window.is_closed()) {
            agent.send_command("quit");
            return true;
        }
        return false;
    };

    auto close_window = [&]() {
        my_window.close_window();
    };

    auto wait_until_closed = [&]() {
        my_window.wait_until_closed();
    };

    auto sync_ui_board = [&]() {
        update_board_by_seqs(my_window, agent.get_move_sequence());
    };
    
    auto toggle_ui = [&]() {
        if (showing)
            my_window.hide();
        else 
            my_window.show();
        showing = !showing;
    }; 

#else
    auto check_gui_closed = [&]() { return false; };
    auto close_window = [&]() {};
    auto wait_until_closed = [&]() {};
    auto sync_ui_board = [&]() {};
    auto toggle_ui = [&]() {}; 

#endif    

    agent.onGtpIn = [](const string& line) {
        cout << line << endl;
    };

    agent.onGtpOut = [&](const string& line) {
        if (!opt_advisor_sim) {
            sync_ui_board();
        }
        cout << line;
    };

    agent.set_init_cmds({"time_settings 1800 15 1"});


    agent.onThinkMove = [&](bool black, int move) {
        if (opt_advisor_sim) {
#ifndef NO_GUI_SUPPORT
            my_window.indicate(move);
#endif
        }
        else {
            agent.place(black, move);
        }

        //agent.place(black, move);
    };

    if (cmdline.empty() || cmdline == "0")
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
            else if (input_str == "hint")
                agent.hint();
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

int selfplay(const string& selfpath, const vector<string>& players) {

    GtpChoice black;
    GtpChoice white;

    black.onInput = [](const string& line) {
        cout << line << endl;;
    };

    black.onOutput = [](const string& line) {
        cout << line;
    };

    GtpChoice* white_ptr = nullptr;

    if (players.empty() || players[0] == "0")
        black.execute();
    else
        black.execute(players[0], selfpath, wait_time_secs);

    if (!black.isReady()) {
        std::cerr << "cannot start player 1" << std::endl;
        return -1;
    }

    if (players.size() > 1) {
        white_ptr = &white;
        if (players[1] == "0")
            white.execute();
        else
            white.execute(players[1], selfpath, wait_time_secs);
        if (!white.isReady()) {
            std::cerr << "cannot start player 2" << std::endl;
            std::cerr << players[1] << endl;
            return -1;
        }
    }

#ifndef NO_GUI_SUPPORT
    go_window my_window;
    my_window.enable_play_mode(false);
#endif

    bool ok;

    if (default_board_size != 19) {
        GtpState::send_command_sync(black, "boardsize " + to_string(default_board_size), ok);
        if (!ok) {
            std::cerr << "player 1 do not support size " << default_board_size << std::endl;
            return -1;
        }

        if (white_ptr) {
            GtpState::send_command_sync(*white_ptr, "boardsize " + to_string(default_board_size), ok);
            if (!ok) {
                std::cerr << "player 2 do not support size " << default_board_size << std::endl;
                return -1;
            }
        }
    }

    auto me = &black;
    auto other = white_ptr;
    bool black_to_move = true;
    bool last_is_pass = false;
    string result;
    string sgf_moves;

    
    GtpState::send_command_sync(black, "clear_board");
    if (white_ptr)
        GtpState::send_command_sync(*white_ptr, "clear_board");

#ifndef NO_GUI_SUPPORT
    my_window.reset(black.boardsize());
#endif

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
            
#ifndef NO_GUI_SUPPORT
        my_window.update(black_to_move, move);
#endif

        if (other) {
            GtpState::send_command_sync(*other, (black_to_move ? "play b " : "play w ") + vtx, ok);
            if (!ok)
                throw runtime_error("unexpect error while play");

            auto tmp = me;
            me = other;
            other = tmp;
        }

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



#ifndef NO_GUI_SUPPORT
    my_window.wait_until_closed();
#endif

    GtpState::wait_quit(black);
    if (white_ptr) GtpState::wait_quit(*white_ptr);

    return 0;
}





