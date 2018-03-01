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
    function<void(const string& line)> onInput;
    function<void(const string& line)> onStderr;

    GtpAgent(const string& cmdline="", const string& path="");

    void execute(const string& cmdline, const string& path="");
    bool restore(int secs=10);
    bool wait_till_ready(int secs=10);
    
    bool alive();
    bool isReady() const;
    bool support(const string& cmd);
    bool next_move_is_black() const;

    void send_command(const string& cmd, function<void(bool, const string&)> handler=nullptr);
    string send_command_sync(const string& cmd, bool& success);

    int join() {
        return process_->get_exit_status();
    }

    // gtp command wraper
    static constexpr int pass_move = -1;
    static constexpr int resign_move = -2;
    static constexpr int invalid_move = -100;
 
    void quit();
    
    int text_to_move(const string& vertex) const;

protected:
    string pending_command();
    virtual void onReset() {}

private:
    void clear_vars();
    void kill();
    void clean_command_queue();
    void onGtpResult(int id, bool success, const string& cmd, const string& rsp);
    
protected:
    string command_line_;
    string path_;

    int board_size_{19};

    struct move_t {
        bool is_black;
        int pos;
    };
    vector<move_t> history_moves_;
    vector<int> handicaps_;

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
    

    string recvbuffer_;
    mutable std::mutex mtx_;   
};


class GameAdvisor : public GtpAgent {
public:
    GameAdvisor(const string& cmdline="", const string& path="");

    function<void(bool,int)> onThinkMove;
    function<void()> onThinkPass;
    function<void()> onThinkResign;
    function<void()> onThinkBegin;
    function<void()> onThinkEnd;
    function<void(const string&)> onGtpIn;
    function<void(const string&)> onGtpOut;
    function<void()> onMoveChange;

    void reset(bool my_side);
    void hint();
    void place(bool black_move, int pos);
    void pop_events();


protected:
    bool my_side_is_black_;

    void think(bool black_move);
    void put_stone(bool black_move, int pos);

    virtual void onReset();

private:
    void reset_vars();

    safe_queue<string> events_;
    std::atomic<bool> pending_reset_{false};
    std::atomic<bool> commit_pending_{false};
    bool commit_player_;
    int commit_pos_;
};

