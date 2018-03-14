#pragma once

#include "gtp_agent.h"
#include <iostream>
#include <algorithm>

template<class TGTP>
class GameAdvisor : public TGTP {
public:
    GameAdvisor() {

        TGTP::onInput = [this](const string& line) {
            events_.push({"input", line});
        };
        TGTP::onOutput = [this](const string& line) {
            events_.push({"output", line});
        };
        TGTP::onReset = [this]() {
            events_.clear();
            pending_reset_ = false;
            commit_pending_ = false;
            events_.push({"reset"});
            to_moves_.clear();
            next_side_ = true;
            execute_next_move();
        };

        TGTP::onStderr = [this](const string& line) {

            buffer_ += line;
            auto lr = buffer_.rfind('\n');
            if (lr != string::npos) {
                auto str = buffer_.substr(0, lr);
                buffer_ = buffer_.substr(lr+1);

                std::istringstream ss(str);
                string subline;
                while (std::getline(ss, subline)) {

                    auto p = subline.find_first_not_of(' ');  
                    if (p >= 1) 
	                    subline.erase(subline.begin(), subline.begin()+p);

                    // something like this
                    // R4 ->       2 (V: 48.33%) (N:  8.69%) PV: R4 D16
                    if (subline.size() > 30 &&
                        ( (subline[0] >='A' && subline[0] <='T' && subline[1] >='1' && subline[1] <='9') || 
                           subline.find("pass") == 0 || 
                           subline.find("resign") == 0)
                        ) 
                    {

                        // R4
                        int move = GtpState::invalid_move;
                        if (subline[0]=='p') GtpState::pass_move;
                        else if (subline[0]=='r') GtpState::resign_move;
                        else {
                            int column;
                            if (subline[0] < 'I') {
                                column = subline[0] - 'A';
                            } else {
                                column = (subline[0] - 'A')-1;
                            }
                            int row = stoi(subline.substr(1, 3));
                            row--;
                            if (row < 19)
                                move = row*19 + column;
                        }

                        if (move != GtpState::invalid_move) {
                            // ->       2 (V: 48.33%) (N:  8.69%) PV: R4 D16
                            p = subline.find("-> ", 3);
                            if (p != string::npos) {
                                p = subline.find_first_not_of(' ', p+3);
                                // 2 (V: 48.33%) (N:  8.69%) PV: R4 D16
                                if (p != string::npos && std::isdigit(subline[p])) {
                                    // (V: 48.33%) (N:  8.69%) PV: R4 D16
                                    p = subline.find("(V: ", p+1);
                                    if (p != string::npos) {
                                        // 48.33%) (N:  8.69%) PV: R4 D16
                                        auto fv = std::stof(subline.substr(p+4));
                                        fv /= 100.0f;
                                        dists_.push_back({move, fv});
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            std::cerr << line << std::flush;
        };
    }

    bool next_move_is_black() const {
        return commit_pending_ ? !next_side_ : next_side_;
    }

    void set_init_cmds(const vector<string>& cmds) {
        init_cmds = cmds;
    }

    void hint_both(bool v) {
        hint_both_ = v;
    }

    function<void()> onResetGame;
    function<void(bool,int, const vector<pair<int,float>>&)> onThinkMove;
    function<void()> onThinkPass;
    function<void()> onThinkResign;
    function<void()> onThinkBegin;
    function<void()> onThinkEnd;
    function<void(const string&)> onGtpIn;
    function<void(const string&)> onGtpOut;
    function<void(bool,int)> onMoveCommit;

    void reset(bool my_side) {
        my_side_is_black_ = my_side;
        TGTP::send_command("clear_board");
        pending_reset_ = true;
        to_moves_.clear();

        for (auto& cmd : init_cmds)
            TGTP::send_command(cmd);
    }

    void hint() {
        my_side_is_black_ = next_move_is_black();
        if (to_moves_.empty()) {
            execute_next_move();
        }
    }

    void place(bool black_move, int pos) {
        play_move(black_move, pos);
    }

    void pop_events() {
        vector<string> ev;
        while (events_.try_pop(ev)) {
            if (ev[0] == "reset") {
                if (onResetGame)
                    onResetGame();
            }
            if (ev[0] == "predict") {
                bool black_move = ev[1] == "b";
                int move = stoi(ev[2]);
                if (onThinkMove)
                    onThinkMove(black_move, move, dists_);
            }
            else if (ev[0] == "pass") {
                if (onThinkPass)
                    onThinkPass();
            }
            else if (ev[0] == "resign") {
                if (onThinkResign)
                    onThinkResign();
            }
            else if (ev[0] == "think") {
                if (onThinkBegin)
                    onThinkBegin();
            }
            else if (ev[0] == "think_end") {
                if (onThinkEnd)
                    onThinkEnd();
            }
            else if (ev[0] == "input") {
                if (onGtpIn)
                    onGtpIn(ev[1]);
            }
            else if (ev[0] == "output") {
                if (onGtpOut)
                    onGtpOut(ev[1]);
            }
            else if (ev[0] == "update_board") {
                bool black_move = ev[1] == "b";
                int move = stoi(ev[2]);
                if (onMoveCommit)
                    onMoveCommit(black_move, move);
            }
        }
    }


protected:
    void play_move(bool black_move, const int pos) {

        events_.push({"update_board", black_move ? "b" : "w", to_string(pos)});

        auto is_empty = to_moves_.empty();
        to_moves_.push({black_move, pos});

        if (is_empty) {
            execute_next_move();
        }
    }

    void execute_next_move() {

        if (commit_pending_) {
            
            GtpState::move_t m;
            if (to_moves_.try_peek(m)) {

                commit_pending_ = false;

                if (commit_player_ == m.is_black && commit_pos_ == m.pos) {
                    // play as sugessed
                    // remove this move
                    to_moves_.try_pop();
                }
                else if (commit_player_ != m.is_black) {

                    // I passed, the cmd need to be executed
                    if (commit_pos_ != GtpState::pass_move) {
                        // i played PASS but engine suggest NOT
                        // undo & play pass
                        to_moves_.push_front({commit_player_, GtpState::pass_move});
                        to_moves_.push_front({commit_player_, -3});
                    }
                }
                else {
                    // NOT play as sugessed, undo it
                    to_moves_.push_front({commit_player_, -3});
                }
            }
        }

        GtpState::move_t m;
        if (!to_moves_.try_pop(m)) {
            think();
            return;
        }

        if (m.pos == -3) {
            TGTP::send_command("undo", [this](bool success, const string&) {
                if (!success) {
                    next_side_ = !next_side_;
                }
                execute_next_move();
            });
        } else {

            string cmd = m.is_black ? "play b " : "play w ";
            cmd += TGTP::move_to_text(m.pos);

            TGTP::send_command(cmd, [this](bool success, const string&) {
                if (!success) {
                    next_side_ = !next_side_;
                }
                execute_next_move();
            });
        }

        next_side_ = !next_side_;
    }


    void do_think(bool black_move) {
        
        events_.push({"think", black_move ? "b" : "w"});

        dists_.clear();

        TGTP::send_command(black_move ? "genmove b" : "genmove w", [black_move, this](bool success, const string& rsp) {

            if (!success) {
                next_side_ = !next_side_;
                execute_next_move();
                return;
            }

            int move = TGTP::text_to_move(rsp);
            if (move == TGTP::invalid_move)
                throw std::runtime_error("unexpected mvoe " + to_string(move));

            if (!pending_reset_) {

                string player = black_move ? "b" : "w";
                events_.push({"think_end"});
                if (move == TGTP::pass_move)
                    events_.push({"pass", player});
                else if (move == TGTP::resign_move)
                    events_.push({"resign", player});
                else {
                    events_.push({"predict", player, to_string(move)});
                }

                commit_pending_ = true;
                commit_player_ = black_move;
                commit_pos_ = move;
   
                execute_next_move();
            }
        });

        next_side_ = !next_side_;
    }

    void think() {
        if (hint_both_ || next_side_ == my_side_is_black_) {
            if (!commit_pending_)
                do_think(next_side_);
        }
    }

private:
    bool next_side_{true};
    bool my_side_is_black_{true};
    bool hint_both_{false};

    safe_queue<vector<string>> events_;
    std::atomic<bool> pending_reset_{false};
    std::atomic<bool> commit_pending_{false};
    bool commit_player_;
    int commit_pos_;
    vector<string> init_cmds;

    safe_dqueue<GtpState::move_t> to_moves_;

    string buffer_;
    vector<pair<int,float>> dists_;
};


