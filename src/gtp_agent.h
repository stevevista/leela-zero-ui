#pragma once

#include "tiny-process-library/process.hpp"
#include "safe_queue.hpp"
#include <functional>
#include <atomic>
#include <sstream>
#include <iostream>

using namespace std;
using namespace TinyProcessLib;

class GtpState {
public:
    function<void(const string& line)> onInput;
    function<void(const string& line)> onOutput;
    function<void(const string& line)> onStderr;
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

    string move_to_text(int move) const;
    int text_to_move(const string& vertex) const;

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

    void execute(const string& cmdline, const string& path="", const int wait_secs=0);
    bool restore(int secs=10);
    
    bool alive();
    bool isReady();
    bool support(const string& cmd);
    string version() const;

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


