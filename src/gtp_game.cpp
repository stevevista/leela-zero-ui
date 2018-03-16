#include "gtp_game.hpp"
#include "gtp_choice.h"
#include <cctype>

template<class TGTP>
GameAdvisor<TGTP>::GameAdvisor() {

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
        resetted_ = true;
        execute_next_move();
    };

    TGTP::onStderr = [this](const string& line) {

        processStderr(line);
    };
}


template<class TGTP>
void GameAdvisor<TGTP>::reset() {
    pending_reset_ = true;
    to_moves_.clear();
    TGTP::send_command("clear_board");
        
    for (auto& cmd : init_cmds)
        TGTP::send_command(cmd);
}


template<class TGTP>
void GameAdvisor<TGTP>::hint_off() {
    hint_black_ = false;
    hint_white_ = false;
}

template<class TGTP>
void GameAdvisor<TGTP>::hint_both() {
    hint_black();
    hint_white();
}

template<class TGTP>
void GameAdvisor<TGTP>::hint() {
    hint_off();
    if (next_move_is_black()) {
        hint_black();
    } else {
        hint_white();
    }
}

template<class TGTP>
void GameAdvisor<TGTP>::hint_black() {
    hint_black_ = true;
    if (resetted_ && to_moves_.empty())
        execute_next_move();
}


template<class TGTP>
void GameAdvisor<TGTP>::hint_white() {
    hint_white_ = true;
    if (resetted_ && to_moves_.empty())
        execute_next_move();
}

template<class TGTP>
void GameAdvisor<TGTP>::think() {

    if (hint_black_ == next_side_ || hint_white_ == !next_side_) {
        if (!commit_pending_)
            do_think(next_side_);
    }
}



template<class TGTP>
void GameAdvisor<TGTP>::play_move(bool black_move, const int pos) {

    auto is_empty = to_moves_.empty();
    to_moves_.push({black_move, pos});

    if (is_empty) {
        execute_next_move();
    }
}

template<class TGTP>
void GameAdvisor<TGTP>::execute_next_move() {

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


template<class TGTP>
void GameAdvisor<TGTP>::do_think(bool black_move) {
        
    events_.push({"think", black_move ? "b" : "w"});

    stats_.clear();

    TGTP::send_command(black_move ? "genmove b" : "genmove w", [black_move, this](bool success, const string& rsp) {

        if (!success) {
            next_side_ = !next_side_;
            if (!to_moves_.empty())
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


template<class TGTP>
void GameAdvisor<TGTP>::processStderr(const string& output) {

    buffer_ += output;
    auto lr = buffer_.rfind('\n');

    if (lr != string::npos) {
        auto str = buffer_.substr(0, lr);
        buffer_ = buffer_.substr(lr+1);

        std::istringstream ss(str);
        string line;
        while (std::getline(ss, line)) {
            processStderrOneLine(line);
        }
    }

    if (!eat_stderr)
        std::cerr << output << std::flush;
}

template<class TGTP>
void GameAdvisor<TGTP>::processStderrOneLine(const string& line) {

    parseLeelaDumpStatsLine(line);
}

template<class TGTP>
bool GameAdvisor<TGTP>::parseLeelaDumpStatsLine(const string& line) {

    // myprintf("%4s -> %7d (V: %5.2f%%) (N: %5.2f%%) PV: %s\n",
    // 
    // something like this
    // R4 ->       2 (V: 48.33%) (N:  8.69%) PV: R4 D16

    auto p = line.find_first_not_of(' ');  
    if (p == string::npos)
        return false; 

    int move;
    if (line[p] >='A' && line[p] <='T' && line[p+1] >='1' && line[p+1] <='9') {

        int column, row;
        if (line[p] < 'I') {
            column = line[p] - 'A';
        } else {
            column = (line[p] - 'A')-1;
        }
        row = stoi(line.substr(p+1, p+3));
        row--;
        if (row >= 19)
            return false;

        move = row*19 + column;

    } else if (line[p] == 'p' && line.find("pass") == p) {
        move = GtpState::pass_move;
    } else if (line[p] == 'r' && line.find("resign") == p) {
        move = GtpState::resign_move;
    }else {
        return false;
    }

    // ->       2 (V: 48.33%) (N:  8.69%) PV: R4 D16
    p = line.find("-> ", p+3);
    if (p == string::npos)
        return false; 

    p = line.find_first_not_of(' ', p+3);
    if (p == string::npos)
        return false; 

    // 2 (V: 48.33%) (N:  8.69%) PV: R4 D16
    if (!std::isdigit(line[p]))
        return false;

    int visits = stoi(line.substr(p, p+7));

    // (V: 48.33%) (N:  8.69%) PV: R4 D16
    p = line.find("(V: ", p+1);
    if (p == string::npos)
        return false; 
   
    // 48.33%) (N:  8.69%) PV: R4 D16
    float probs = std::stof(line.substr(p+4, p+14));
    probs /= 100.0f;

    p = line.find("(N: ", p+1);
    if (p == string::npos)
        return false; 

    // 8.69%) PV: R4 D16
    float score = std::stof(line.substr(p+4, p+14));
    score /= 100.0f;

    stats_.push_back({move, visits, probs, score});
                                    
    return true;                          
}




template class GameAdvisor<GtpProcess>;
template class GameAdvisor<GTP>;
template class GameAdvisor<GtpChoice>;
