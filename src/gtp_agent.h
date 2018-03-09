#pragma once

#include "tiny-process-library/process.hpp"
#include "safe_queue.hpp"
#include <functional>
#include <atomic>
#include <sstream>

using namespace std;
using namespace TinyProcessLib;

class GtpState {
public:
    function<void(const string& line)> onInput;
    function<void(const string& line)> onOutput;
    function<void()> onReset;

    static constexpr int pass_move = -1;
    static constexpr int resign_move = -2;
    static constexpr int invalid_move = -100;

    struct move_t {
        bool is_black;
        int pos;
    };

    vector<move_t> get_move_sequence() const;
 
public:
    int boardsize() const { return board_size_; }
    bool next_move_is_black() const;

    string move_to_text(int move) const;
    int text_to_move(const string& vertex) const;

    string pending_command();
    bool playing_pending();

    template<typename TGTP>
    static string send_command_sync(TGTP& gtp, const string& cmd, bool& success, int timeout_secs=-1) {

        string ret;
        std::atomic<bool> returned{false};

        gtp.send_command(cmd, [&success, &ret, &returned](bool ok, const string& out) {

            success = ok;
            ret = out;
            returned = true;
        });

        if (!returned)
            this_thread::sleep_for(chrono::microseconds(10)); 

        if (!returned)
            this_thread::sleep_for(chrono::microseconds(100)); 

        int ecplised = 0;
        while (!returned) {
            
            if (!gtp.alive()) {
                success = false;
                return "? not active";
            }
            this_thread::sleep_for(chrono::microseconds(1)); 

            if (!returned && timeout_secs > 0 && ecplised++ >= timeout_secs) {
                success = false;
                return "? timeout";
            }
        }

        return success ? ret : ("? ") + ret;
    }

    template<typename TGTP>
    static string send_command_sync(TGTP& gtp, const string& cmd, int timeout_secs=-1) {
        bool success;
        return send_command_sync(gtp, cmd, success, timeout_secs);
    }

    template<typename TGTP>
    static int wait_quit(TGTP& gtp) {
        gtp.send_command("quit");
        return gtp.join();
    }

protected:
    void clean_command_queue();
    void clean_board();
    void clean_up();

protected:

    int board_size_{19};

    vector<move_t> history_moves_;
    vector<int> handicaps_;

    struct command_t {
        string cmd;
        function<void(bool, const string&)> handler;
    };
    safe_queue<command_t> command_queue_;
};


class GtpProcess : public GtpState {
public:
    function<void(const string& line)> onUnexpectOutput;
    function<void(const string& line)> onStderr;

    void execute(const string& cmdline, const string& path="", const int wait_secs=0);
    bool restore(int secs=10);
    
    bool alive();
    bool isReady();
    bool support(const string& cmd);

    void send_command(const string& cmd, function<void(bool, const string&)> handler=nullptr);

    int join() {
        if (!process_) return -1;
        return process_->get_exit_status();
    }

private:
    void kill();
    void onGtpResult(int id, bool success, const string& cmd, const string& rsp);
    
private:
    string command_line_;
    string path_;

    shared_ptr<Process> process_;
    
    vector<string> support_commands_;
    std::atomic<bool> ready_{false};
    std::atomic<bool> ready_query_made_{false}; 

    string protocol_version_;
    string name_;
    string version_;
    

    string recvbuffer_;
    mutable std::mutex mtx_;   
};


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
            while (!events_.empty()) events_.try_pop();
            pending_reset_ = false;
            commit_pending_ = false;
            my_side_is_black_ = true;
            events_.push({"reset"});
        };
    }

    function<void()> onResetGame;
    function<void(bool,int)> onThinkMove;
    function<void()> onThinkPass;
    function<void()> onThinkResign;
    function<void()> onThinkBegin;
    function<void()> onThinkEnd;
    function<void(const string&)> onGtpIn;
    function<void(const string&)> onGtpOut;
    function<void(bool,int)> onMoveCommit;

    void reset(bool my_side) {
        TGTP::send_command("clear_board");
        pending_reset_ = true;

        if (my_side)
            hint();
    }

    void hint() {
        if (commit_pending_ || TGTP::playing_pending())
            return;

        my_side_is_black_ = TGTP::next_move_is_black();
        think(my_side_is_black_);
    }

    void place(bool black_move, int pos) {
        events_.push({"update_board", black_move ? "b" : "w", to_string(pos)});

        if (commit_pending_) {

            commit_pending_ = false;

            if (black_move == commit_player_ &&
                pos == commit_pos_) {
                return;
            }

            TGTP::send_command("undo"); // not my turn or not play as sugessed, undo genmove
        }

        put_stone(black_move, pos);
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
                    onThinkMove(black_move, move);
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
    bool my_side_is_black_;

    void think(bool black_move) {
        events_.push({"think", black_move ? "b" : "w"});

        TGTP::send_command(black_move ? "genmove b" : "genmove w", [black_move, this](bool success, const string& rsp) {

            if (!success)
                return;

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
            }
        });
    }

    void put_stone(bool black_move, const int pos) {

        string cmd = black_move ? "play b " : "play w ";
        cmd += TGTP::move_to_text(pos);

        TGTP::send_command(cmd, [black_move, cmd, this](bool success, const string&) {

            if (!success) return; // throw std::runtime_error("unexpected play result " + to_string(ret));

            if (!pending_reset_) {

                if (commit_pending_) {
                    commit_pending_ = false;

                    // duplicated !!!
                    // someone play the stone before think return 
                    TGTP::send_command("undo");  // undo myself
                    TGTP::send_command("undo");  // undo genmove
                    // replay myself
                    TGTP::send_command(cmd);
                    return;
                }

                if (my_side_is_black_ == !black_move)
                    think(my_side_is_black_);
            }
        });
    }

private:

    safe_queue<vector<string>> events_;
    std::atomic<bool> pending_reset_{false};
    std::atomic<bool> commit_pending_{false};
    bool commit_player_;
    int commit_pos_;
};

