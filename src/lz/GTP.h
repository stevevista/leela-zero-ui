/*
    This file is part of Leela Zero.
    Copyright (C) 2017 Gian-Carlo Pascutto

    Leela Zero is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leela Zero is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Leela Zero.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GTP_H_INCLUDED
#define GTP_H_INCLUDED

#include "config.h"

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>

#include "GameState.h"
#include "UCTSearch.h"
#include "../safe_queue.hpp"
#include "../gtp_agent.h"

using namespace std;

extern bool cfg_gtp_mode;
extern bool cfg_allow_pondering;
extern int cfg_num_threads;
extern int cfg_max_threads;
extern int cfg_max_playouts;
extern int cfg_max_visits;
extern TimeManagement::enabled_t cfg_timemanage;
extern int cfg_lagbuffer_cs;
extern int cfg_resignpct;
extern std::uint64_t cfg_rng_seed;
extern bool cfg_dumbpass;
#ifdef USE_OPENCL
extern std::vector<int> cfg_gpus;
extern bool cfg_sgemm_exhaustive;
extern bool cfg_tune_only;
#endif
extern float cfg_puct;
extern float cfg_softmax_temp;
extern float cfg_fpu_reduction;
extern std::string cfg_logfile;
extern std::string cfg_weightsfile;
extern FILE* cfg_logfile_handle;
extern bool cfg_quiet;


class GTP : public GtpState {

    static bool input_pending_;

    unique_ptr<GameState> game;
    unique_ptr<UCTSearch> search;
    std::thread th_;
    std::atomic<bool> ready_{false};
    
public:
    static void setup_default_parameters();

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


#endif
