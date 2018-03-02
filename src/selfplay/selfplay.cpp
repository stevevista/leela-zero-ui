


#include "../gtp_agent.h"
#include "../train_data/archive.hpp"
#include <fstream>
#include <cassert>
#include <cstring>
#include <sstream>
#include <csignal>
#include <iostream>

using namespace std;

#ifndef NO_GUI_SUPPORT
#include "ui.h"
#endif


static int opt_games = -1;
static int opt_size = 19;

GtpAgent* black_ptr = nullptr;
GtpAgent* white_ptr = nullptr;
#ifndef NO_GUI_SUPPORT
go_window* window_ptr = nullptr; 
#endif

int main(int argc, char **argv) {

    string selfpath = argv[0];
    
    auto pos  = selfpath.rfind(
        #ifdef _WIN32
        '\\'
        #else
        '/'
        #endif
        );

    selfpath = selfpath.substr(0, pos); 

    string append;
    string cmdline1, cmdline2;

    for (int i=1; i<argc; i++) {
        string opt = argv[i];

        if (opt == "--pass") {
            i++;
            for (; i<argc; i++) {
                append += " ";
                append += argv[i];
            }
            break;
        }

        if (opt == "--player") {
            if (cmdline1.empty())
                cmdline1 = argv[++i];
            else if (cmdline2.empty())
                cmdline2 = argv[++i];
            else {
                ++i;
            }
        }
        else if (opt == "--games") {
            opt_games = stoi(argv[++i]);
        }
        else if (opt == "--size") {
            opt_size = stoi(argv[++i]);
        }
    }


    GtpAgent black;
    GtpAgent white;

    black.onInput = [](const string& line) {
        cout << line << endl;;
    };

    black.onOutput = [](const string& line) {
        cout << line;
    };


    if (cmdline1.empty() && cmdline2.empty()) {
        std::cerr << "no player" << std::endl;
        return -1;
    }

    cmdline1 += append;
    black_ptr = &black;
    black_ptr->execute(cmdline1, selfpath);
    if (!black_ptr->wait_till_ready()) {
        std::cerr << "cannot start player 1" << std::endl;
        return -1;
    }

    if (!cmdline2.empty()) {
        cmdline2 += append;
        white_ptr = &white;
        white_ptr->execute(cmdline2, selfpath);
        if (!white_ptr->wait_till_ready()) {
            std::cerr << "cannot start player 2" << std::endl;
            return -1;
        }
    }

#ifndef NO_GUI_SUPPORT
    go_window my_window(19);
    window_ptr = &my_window;
#endif

    struct sigaction sigIntHandler;

    auto my_handler = [](int) {
#ifndef NO_GUI_SUPPORT
        window_ptr->close_window();
#endif
        black_ptr->quit();
        if (white_ptr) white_ptr->quit();
    };

   sigIntHandler.sa_handler = (__sighandler_t)my_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;

   sigaction(SIGINT, &sigIntHandler, NULL);

    GoBoard board;
    bool ok;

    if (opt_size != 19) {
        black_ptr->send_command_sync("boardsize " + to_string(opt_size), ok);
        if (!ok) {
            std::cerr << "player 1 do not support size " << opt_size << std::endl;
            return -1;
        }

        if (white_ptr) {
            white_ptr->send_command_sync("boardsize " + to_string(opt_size), ok);
            if (!ok) {
                std::cerr << "player 2 do not support size " << opt_size << std::endl;
                return -1;
            }
        }
    }

    int game_count = 0;
    const int boardsize = black_ptr->boardsize();
    
    GameArchive::Writer writer;
    writer.create("selfplay.data", boardsize);

    while ((opt_games < 0 || game_count++ < opt_games) && black_ptr->alive()) {

        GtpAgent* me = black_ptr;
        GtpAgent* other = white_ptr;
        bool black_to_move = true;
        bool last_is_pass = false;
        int winner = 0;

        bool has_probilities = me->support("probilities");

        board.reset(boardsize);
        black_ptr->send_command_sync("clear_board");
        if (white_ptr)
            white_ptr->send_command_sync("clear_board");

        writer.start_game();

#ifndef NO_GUI_SUPPORT
        my_window.update(-1, board);
#endif

        for (int move_count = 0; move_count<361*2; move_count++) {
            
            auto vtx = me->send_command_sync(black_to_move ? "genmove b" : "genmove w", ok);
            if (!ok)
                throw runtime_error("unexpect error while genmove");

            if (vtx == "resign") {
                winner = black_to_move ? -1 : 1;
                break;
            }

            if (vtx == "pass") {
                if (last_is_pass) 
                    break;
                last_is_pass = true;
            } else
                last_is_pass = false;

        
            auto move = me->text_to_move(vtx);
            std::vector<int> removed;
            if (move >= 0) {  
                board.update_board(black_to_move ? 1 : -1, move, removed);
#ifndef NO_GUI_SUPPORT
                my_window.update(move, board);
#endif
            }

            std::vector<float> probs;
            
            if (has_probilities) {
                auto rsp = me->send_command_sync("probilities", ok);
                if (ok) {
                    // probs
                    float v;
                    std::istringstream rspstream(rsp);
                    rspstream >> v;
                    while (!rspstream.fail()) {
                        probs.emplace_back(v);
                        rspstream >> v;
                    }
                }
            }
             
            // generate move seq
            int move_val = move;
            if (move_val < 0) move_val = boardsize*boardsize;

            if (!has_probilities) {
                writer.add_move(move_val, {});
            } else {
                writer.add_move(move_val, probs, !probs.empty());
            }

            if (other) {
                other->send_command_sync((black_to_move ? "play b " : "play w ") + vtx, ok);
                if (!ok)
                    throw runtime_error("unexpect error while play");

                auto tmp = me;
                me = other;
                other = tmp;
            }

            black_to_move = !black_to_move;
        }

        std::cout << "game over" << std::endl;
 
        // a game is over 
        if (winner == 0) {
            auto rsp = black_ptr->send_command_sync("final_score", ok);
            if (rsp.find("B+") == 0) {
                winner = 1;
            } else {
                winner = -1;
            }
        }

        writer.end_game(winner);
    }

#ifndef NO_GUI_SUPPORT
    my_window.close_window();
#else
    black_ptr->quit();
    if (white_ptr) white_ptr->quit();
#endif

#ifndef NO_GUI_SUPPORT
    my_window.wait_until_closed();
#endif

    writer.close();

    return 0;
}

