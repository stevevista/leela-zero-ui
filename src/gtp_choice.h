#pragma once

#include "lz/GTP.h"


class GtpChoice {
public:
    function<void(const string& line)> onInput;
    function<void(const string& line)> onOutput;
    function<void()> onReset;

    static constexpr int pass_move = GtpState::pass_move;
    static constexpr int resign_move = GtpState::resign_move;
    static constexpr int invalid_move = GtpState::invalid_move;

public:
    void execute() {
        switch_ = 0;
        gtp_blt.onInput = onInput;
        gtp_blt.onOutput = onOutput;
        gtp_blt.onReset = onReset;
        gtp_blt.execute();
    }

    void execute(const string& cmdline, const string& path="", const int wait_secs=0) {
        switch_ = 1;
        gtp_proc.onInput = onInput;
        gtp_proc.onOutput = onOutput;
        gtp_proc.onReset = onReset;
        gtp_proc.onStderr = [](const string& line) { std::cerr << line << std::endl;  };
        gtp_proc.execute(cmdline, path, wait_secs);
    }

    int boardsize() const {
        return switch_ == 0 ? gtp_blt.boardsize() : gtp_proc.boardsize();
    }

    int join() {
        return switch_ == 0 ? gtp_blt.join() : gtp_proc.join();
    }

    bool alive() {
        return switch_ == 0 ? gtp_blt.alive() : gtp_proc.alive();
    }

    bool isReady() {
        return switch_ == 0 ? gtp_blt.isReady() : gtp_proc.isReady();
    }

    bool support(const string& cmd) {
        return switch_ == 0 ? gtp_blt.support(cmd) : gtp_proc.support(cmd);
    }

    string version() const;

    void send_command(const string& cmd, function<void(bool, const string&)> handler=nullptr) {
        if (switch_ == 0)
            gtp_blt.send_command(cmd, handler);
        else
            gtp_proc.send_command(cmd, handler);
    }

    bool next_move_is_black() const {
        return switch_ == 0 ? gtp_blt.next_move_is_black() : gtp_proc.next_move_is_black();
    }

    bool playing_pending() {
        return switch_ == 0 ? gtp_blt.playing_pending() : gtp_proc.playing_pending();
    }

    string move_to_text(int move) const {
        return switch_ == 0 ? gtp_blt.move_to_text(move) : gtp_proc.move_to_text(move);
    }

    int text_to_move(const string& vertex) const {
        return switch_ == 0 ? gtp_blt.text_to_move(vertex) : gtp_proc.text_to_move(vertex);
    }

    vector<GtpState::move_t> get_move_sequence() const {
        return switch_ == 0 ? gtp_blt.get_move_sequence() : gtp_proc.get_move_sequence();
    }

    void stop_think() {
        if (switch_ == 0) gtp_blt.stop_think();
    }

private:
    int switch_{0};
    GTP gtp_blt;
    GtpProcess gtp_proc;
};
