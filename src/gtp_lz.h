#pragma once
#include "lz/GTP.h"
#include "safe_queue.hpp"
#include "gtp_agent.h"

class GtpLZ : public GtpState {

    static bool input_pending_;

    unique_ptr<GameState> game;
    unique_ptr<UCTSearch> search;
    std::thread th_;
    std::atomic<bool> ready_{false};
    
public:
    GtpLZ();
    ~GtpLZ();

    static bool vstderr(const char *fmt, va_list ap);

    static bool input_pending() { return input_pending_; }
    static void stop_ponder() { input_pending_ = true; }

    void send_command(const string& cmd, function<void(bool, const string&)> handler=nullptr);

    bool alive() {
        return ready_ && th_.joinable();
    }
    bool isReady() { return alive(); }
    bool support(const string& cmd);
    string version() const;

    void execute() {
        ready_ = false;
        th_ = std::thread([this] {
            run();
            ready_ = false;
        });
        while (!ready_)
            this_thread::sleep_for(chrono::microseconds(100));
    }

    int join() {
        if (th_.joinable()) {
            th_.join();
            return 0;
        }
        else
            return -1;
    }

    void stop_think() {
        if (search) search->stop_think();
    }

private:
    void run();

private:
    static constexpr int GTP_VERSION = 2;

    static std::string get_life_list(const GameState & game, bool live);
};
