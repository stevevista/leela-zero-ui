#include "gui/board_ui.h"
#include <iostream>
#include <thread>
#include <cctype>

using namespace std;


void autogtpui() {

    // 1 (B D17) 2 (W Q16) 3 (B Q4) ...

    go_window board_ui;
    board_ui.hide();
    board_ui.enable_play_mode(false);

    string text, last1, last2;
    while(!cin.eof()) {

        char buf[2];
        buf[1] = 0;
        auto n = cin.read(buf, 1).gcount();
        if (n == 0) {
            this_thread::sleep_for(chrono::microseconds(1));
            continue;
        }

        
        if (buf[0] == '\n' || buf[0] == ' ') {
            cout << text << buf << std::flush;
            //1 
            //(B 
            //D3)
            if (text.back() == ')') {

                int move = -100;
                bool is_black;
                int num;
                auto mtext = text.substr(0, text.size()-1);
                if (mtext == "pass") {
                    move = -1;
                } else if (mtext == "resign") {
                    move = -2;
                } else {
                    if (mtext.size() >=2 && 
                        std::isalpha(mtext[0]) && 
                        std::isdigit(mtext[1]) &&
                        std::toupper(mtext[0]) != 'I') {
                        auto A = std::toupper(mtext[0]);
                        int column, row;
                        if (A < 'I') {
                            column = A - 'A';
                        } else {
                            column = (A - 'A')-1;
                        }

                        row = stoi(mtext.substr(1));
                        row--;

                        if (column < 19 && row < 19) {
                            move = row * 19 + column;
                        }
                    }
                }

                if (move != -100 &&
                    last1.size()==2 && 
                    last1[0] == '(' &&
                    (last1[1] == 'B' || last1[1] == 'W')) {
                    is_black = last1[1] == 'B';
                } else {
                    move = -100;
                }   

                if (move != -100) {
                    num = stoi(last2);
                    if (num == 0)
                        move = -100;
                }

                if (move != -100) {
                    if (num == 1) {
                        board_ui.show();
                        board_ui.reset();
                    }
                    board_ui.update(is_black, move);
                }
            }

            last2 = last1;
            last1 = text;
            text = "";
        } else {
            text.append(buf);
        }
    }
}
