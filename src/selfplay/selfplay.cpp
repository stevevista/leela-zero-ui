


#include "../gtp_agent.h"
//#include "../train_data/convert.hpp"


#include <fstream>
#include <cassert>
#include <cstring>
#include <sstream>
#include <csignal>

using namespace std;

#ifndef NO_GUI_SUPPORT
#include "ui.h"
#endif


static int opt_games = -1;
static std::string opt_b_player;
static std::string opt_w_player;

void parse_commandline( int argc, char **argv ) {
    for (int i = 1; i < argc; i++) { 
        std::string opt = std::string(argv[i]);

        if (opt == "--games") {
            opt_games = atoi(argv[++i]);
        }
        else if (opt == "--black") {
            opt_b_player = argv[++i];
        }
        else if (opt == "--white") {
            opt_w_player = argv[++i];
        }
    }
}



GtpAgent* black_ptr = nullptr;
GtpAgent* white_ptr = nullptr;
#ifndef NO_GUI_SUPPORT
go_window* window_ptr = nullptr; 
#endif

int main(int argc, char **argv) {

    parse_commandline(argc, argv);

    GtpAgent black("something");

    black.onInput = [](const string& line) {
        cout << line << endl;;
    };

    black.onOutput = [](const string& line) {
        cout << line;
    };

    black_ptr = &black;
    white_ptr = nullptr;

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
        white_ptr->quit();
    };

   sigIntHandler.sa_handler = (__sighandler_t)my_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;

   sigaction(SIGINT, &sigIntHandler, NULL);

    GoBoard board(19);

    int game_count = 0;
    
 //   GameArchive::Writer writer;
//    writer.create("selfplay.data", FastBoard::get_boardsize());

    while (game_count++ < opt_games && black_ptr->alive()) {

        bool ok;

        GtpAgent* me = black_ptr;
        GtpAgent* other = white_ptr;
        bool black_to_move = true;
        bool last_is_pass = false;
        int winner = 0;

        board.reset();
        me->send_command_sync("clear_board", ok);
        if (me != other)
            other->send_command_sync("clear_board", ok);

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

            other->send_command_sync((black_to_move ? "play b " : "play w ") + vtx, ok);
            if (!ok)
                throw runtime_error("unexpect error while play");

            
            auto move = me->text_to_move(vtx);
            std::vector<int> removed;
            if (move >= 0) {  
                board.update_board(black_to_move ? 1 : -1, move, removed);
#ifndef NO_GUI_SUPPORT
                my_window.update(move, board);
#endif
            }

            std::vector<float> probs(362, 0);
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
            } else {
                probs[move] = 1;
            }

            // generate move seq
            int move_val = move;
            //if (move_val < 0) move_val = boardsq;
            //writer.add_move(move_val, removed, probs, !probs.empty());


            black_to_move = !black_to_move;
            auto tmp = me;
            me = other;
            other = tmp;
        }

        // a game is over 
        if (winner == 0) {
            auto rsp = black_ptr->send_command_sync("final_score", ok);
            if (rsp.find("B+") == 0) {
                winner = 1;
            } else {
                winner = -1;
            }
        }

        // writer.end_game(result);
    }

#ifndef NO_GUI_SUPPORT
    my_window.close_window();
#endif
    black_ptr->quit();
    white_ptr->quit();

#ifndef NO_GUI_SUPPORT
    my_window.wait_until_closed();
#endif

    // writer.close();

    return 0;
}

