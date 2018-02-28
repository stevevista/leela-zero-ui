#pragma once

#include "tiny-process-library/process.hpp"
#include "safe_queue.hpp"
#include <functional>
#include <atomic>

using namespace std;
using namespace TinyProcessLib;

class GtpAgent {
public:
    function<void(const string& line)> onOutput;
    function<void(const string& line)> onUnexpectOutput;

    GtpAgent(const string& cmdline="", const string& path="");

    bool alive();
    bool isReady();
    bool support(const string& cmd);
    void send_command(const string& cmd, function<void(bool, const string&)> handler=nullptr);
    string send_command_sync(const string& cmd, bool& success);

    void kill();
    int join() {
        return process_->get_exit_status();
    }

    // gtp command wraper
    static constexpr int pass_move = -1;
    static constexpr int resign_move = -2;
    static constexpr int invalid_move = -100;
    static constexpr int cmd_fail = -101;
    static constexpr int cmd_unsupport = -102;

    void generate_move(bool black_move, function<void(int)> handler);
    void play(bool black_move, int pos, function<void(int)> handler=nullptr);
    void undo(function<void(int)> handler=nullptr);
    void quit();
    void execute(const string& cmdline, const string& path="");

private:
    void clean_command_queue();
    void onGtpResult(int id, bool success, const string& cmd, const string& rsp);
    
protected:
    string command_line_;
    string path_;

private:
    shared_ptr<Process> process_;
    struct command_t {
        string cmd;
        function<void(bool, const string&)> handler;
    };
    safe_queue<command_t> command_queue_;
    vector<string> support_commands_;
    std::atomic<bool> ready_{false};
    std::atomic<bool> ready_query_made_{false}; 

    string protocol_version_;
    string name_;
    string version_;
    int board_size_{19};

    string recvbuffer_;
    mutable std::mutex mtx_;   
};


class GameAdvisor : public GtpAgent {
public:
    GameAdvisor(const string& cmdline, const string& path="");

    function<void(bool,int)> onThinkMove;
    
    void reset();
    void think(bool black_move);
    void place(bool black_move, int pos);
    void pop_events();
    bool restore(int secs=10);
    bool wait_till_ready(int secs=10);

private:
    void reset_vars();

    safe_queue<string> events_;
    std::atomic<bool> pending_reset_{false};

    std::vector<int> history_;
    bool last_player_;
    std::atomic<bool> commit_pending_{false};
    mutable std::mutex hmtx_;  
};
