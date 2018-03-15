#pragma once 
#include <array>
#include "../dlib/gui_core.h"
#include "../dlib/gui_widgets.h"
#include "../dlib/array2d.h"
#include "board.h"


using namespace std;
using namespace dlib;


class board_display : public drawable {
public:
    board_display(drawable_window& w);

	void set_size (
        unsigned long width,
        unsigned long height
    );

    void reset(int bdsize=0);
    bool update(bool black_move, int move);
    bool undo(bool black_move);

    void enable_play_mode(bool v) {play_mode = v;}
    void indicate(int pos);

    function<bool(bool,int)> onMoveClick;
	
private:
    void draw(const canvas& c) const;
	
	void on_mouse_move (
            unsigned long ,
            long ,
            long 
        );

    void on_mouse_up (
            unsigned long ,
            unsigned long ,
            long ,
            long 
        );

    void load_res();
	
	
    void draw_grid(const canvas& c) const;
	void draw_stones(const canvas& c) const;
    void draw_stone(const canvas& c, long x, long y, int value, bool mark) const;
	void draw_shadow_stone(const canvas& c, long x, long y, bool black) const;
    void draw_mark(const canvas& c, long x, long y, rgb_alpha_pixel pixel) const;

    

private:
    void rescale();
    long stone_size() const { return radius_*2-1; }
    point stone_pt(long x, long y) const;
    rectangle stone_rect(long x, long y) const;
    rectangle stone_rect(long pos) const;
    int pt_to_stone_pos(long left ,long top) const;

    long radius_{16};

private:
    array2d<rgb_alpha_pixel> img_wood_src;
    array2d<rgb_alpha_pixel> img_black_src;
    array2d<rgb_alpha_pixel> img_white_src;

    array2d<rgb_alpha_pixel> img_wood;
    array2d<rgb_alpha_pixel> img_black;
    array2d<rgb_alpha_pixel> img_white;

    const long edge_size{10};

    GoBoard board_;
	
	int last_xy{-1};
    int mark_xy{-1};
    int indicate_xy{-1};
    bool black_move{true};
    bool play_mode{true};
    int boardsize_;
};

class go_window : public drawable_window {
public:
    go_window(); 

    void reset(int bdsize=0);
    bool update(bool black_move, int move);
    bool undo(bool black_move);

    void enable_play_mode(bool v) { board.enable_play_mode(v); }
    void indicate(int pos) { board.indicate(pos); }

    void setMoveClickHandler(function<bool(bool,int)> f) {
        board.onMoveClick = f;
    }

private:
    void on_window_resized();

private:
	board_display board;
};
