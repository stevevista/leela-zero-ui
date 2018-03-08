#pragma once 
#include <array>
#include "gui_core.h"
#include "array2d.h"
#include "../train_data/archive.hpp"
#include "../gtp_agent.h"

using namespace std;
using namespace dlib;

class go_window : public base_window {
public:
    go_window(); 
    void reset(int bdsize);
    void update(bool black_move, int move);
    void update(const vector<GtpState::move_t>& seqs);

private:
    void paint (
            const canvas& c
        );

    void on_window_resized();

private:
    void draw_grid(const canvas& c);
    void draw_stones(const canvas& c);
    void draw_stone(const canvas& c, long x, long y, int black, int mark);
    void draw_mark(const canvas& c, long x, long y);
    void load_res();
    void scale_res();

    std::array<long,2> loc(long x, long y) const {
        long start = edge_size + radius;
        return { start+x*radius*2, start+(board_.board_size()-1-y)*radius*2 };
    }
    

    array2d<rgb_alpha_pixel> img_wood_src;
    array2d<rgb_alpha_pixel> img_black_src;
    array2d<rgb_alpha_pixel> img_white_src;

    array2d<rgb_alpha_pixel> img_wood;
    array2d<rgb_alpha_pixel> img_black;
    array2d<rgb_alpha_pixel> img_white;

    long radius = 16;
    long edge_size = 10;

    int last_xy;
    GoBoard board_;
};
