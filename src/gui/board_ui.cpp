#include "board_ui.h"
#include <fstream>
#include "../dlib/pixel.h"
#include "../dlib/geometry.h"
#include "../dlib/matrix.h"
#include "../dlib/image_transforms.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_INLINE
#include "../stb/stb_image.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


// ----------------------------------------------------------------------------------------

template<typename T>
void load_image(const std::string& path, T& t_) {
	int x, y, c;
	auto result = stbi_load(path.c_str(),&x,&y,&c,4);
    if (!result)
        return;

	//std::cout << x << std::endl;
	//std::cout << y << std::endl;
	//std::cout << c << std::endl;
    image_view<T> t(t_);
    t.set_size( y, x );
	
	unsigned char* ptr = result;
	for ( int n = 0; n < y; n++ )
    {
       for ( int m = 0; m < x;m++ )
       {
            rgb_alpha_pixel p;
            p.red = *ptr++;
            p.green = *ptr++;
            p.blue = *ptr++;
            p.alpha = *ptr++;
            assign_pixel( t[n][m], p );
       }
    }	
	
	stbi_image_free(result);
}


board_display::board_display(drawable_window& w)
: drawable(w, MOUSE_MOVE|MOUSE_CLICK) {

    boardsize_ = board_.board_size();
	load_res();
    rescale();
	enable_events();
}


point board_display::stone_pt(long x, long y) const {

    long left = rect.left() + edge_size + x*stone_size();
    long top = rect.top() + edge_size + (boardsize_-1-y)*stone_size();
    return point( left+radius_, top+radius_ );
}

rectangle board_display::stone_rect(long x, long y) const {

    long left = rect.left() + edge_size + x*stone_size();
    long top = rect.top() + edge_size + (boardsize_-1-y)*stone_size();
    return rectangle(left, top, left+stone_size()-1, top+stone_size()-1);
}

rectangle board_display::stone_rect(long pos) const {

    if (pos < 0)
        return rectangle();
    return stone_rect(pos%boardsize_, pos/boardsize_);
}

int board_display::pt_to_stone_pos(long left ,long top) const {

    int x = (left-edge_size)/stone_size();
    int y = (top-edge_size)/stone_size();
    if (x < 0 || y < 0)
        return -1;
    if (x >= boardsize_ || y >= boardsize_)
        return -1;

    return (boardsize_-1-y)*boardsize_ + x;
}



void board_display::rescale() {

    long wood_size = stone_size() * boardsize_ + edge_size*2;

    rect = resize_rect(rect, wood_size, wood_size);

    if (img_wood_src.size()) {
        img_wood.set_size(wood_size, wood_size);
        resize_image(img_wood_src, img_wood);
    }

    if (img_black_src.size()) {
        img_black.set_size(stone_size(), stone_size());
        resize_image(img_black_src, img_black);
    }

    if (img_white_src.size()) {
        img_white.set_size(stone_size(), stone_size());
        resize_image(img_white_src, img_white);
    }
}


void board_display::draw(const canvas& c) const {

    rectangle area = rect.intersect(c);
    if (area.is_empty())
        return;

    // draw background
    for (long row = area.top(); row <= area.bottom(); ++row)
    {
        const long rc = row-c.top();
        for (long col = area.left(); col <= area.right(); ++col)
        {
			if (img_wood.size())
				assign_pixel(c[rc][col-c.left()], img_wood[row][col]);
			else 
				assign_pixel(c[rc][col-c.left()], rgb_pixel(204,152, 51));
        }
    }
	
    draw_grid(c);
	draw_stones(c);
}

void board_display::draw_grid(const canvas& c) const {

    long len = stone_size() * (boardsize_ - 1);
    long start = radius_+edge_size;

    long y = start;
    for (int i=0; i< boardsize_; i++, y+= stone_size()) {

        draw_line (c, point(start, y), point(start + len, y), rgb_alpha_pixel(0, 0, 0, 100));
    }

    long x = start;
    for (int i=0; i< boardsize_; i++, x+= stone_size()) {
        
        draw_line (c, point(x, start), point(x, start + len), rgb_alpha_pixel(0, 0, 0, 100));
    }

    // deeper boarder
    draw_line (c, point(start-1, start-1), point(start + len + 1, start-1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start-1, start + len + 1), point(start + len + 1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start-1, start-1), point(start-1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start + len + 1, start-1), point(start + len + 1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));

    //  star
    if (boardsize_ == 19) {
        draw_solid_circle(c, point(3,3), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, point(3,9), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, point(3,15), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, point(9,3), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, point(9,9), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, point(9,15), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, point(15,3), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, point(15,9), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, point(15,15), 3, rgb_alpha_pixel(0, 0, 0, 100));
    }
}

void board_display::draw_stones(const canvas& c) const {
	
    int pos = 0;
    for (int y=0; y< boardsize_; y++) {
        for (int x=0; x< boardsize_; x++, pos++) {
            if (board_[pos] != 0) {
                draw_stone(c, x, y, board_[pos], last_xy == pos);
            }
        }
    }

	if (mark_xy >= 0 && board_[mark_xy] == 0) {
        draw_shadow_stone(c, mark_xy%boardsize_, mark_xy/boardsize_, black_move);
    }

    if (indicate_xy >= 0) {
        draw_solid_circle(c, stone_pt(indicate_xy%boardsize_, indicate_xy/boardsize_), radius_*0.7, rgb_alpha_pixel(255, 0, 0, 128));
    }
}

void board_display::draw_stone(const canvas& c, long x, long y, int value, bool mark) const {

    auto center = stone_pt(x, y);
    auto left = center.x() - radius_;
    auto top = center.y() - radius_;
    //
    if (img_black.size() && img_white.size()) {
        draw_image(c, point(left, top), value == 1 ? img_black : img_white);
    }
    else {
        draw_solid_circle(c, center, radius_-1, value == 1 ? rgb_alpha_pixel(0, 0, 0, 255) : rgb_alpha_pixel(255, 255, 255, 255));
        draw_circle(c, center, radius_, rgb_alpha_pixel(102, 102, 102, 255));
    }

    if (mark)
        draw_mark(c, x, y, value == 1 ? rgb_alpha_pixel(255, 255, 255, 255) : rgb_alpha_pixel(0, 0, 0, 255));
}

void board_display::draw_mark(const canvas& c, long x, long y, rgb_alpha_pixel pixel) const {

    draw_circle(c, stone_pt(x, y), radius_*0.6, pixel);
}

void board_display::draw_shadow_stone(const canvas& c, long x, long y, bool black) const {

    draw_solid_circle(c, stone_pt(x, y), radius_*0.7, black ? rgb_alpha_pixel(0, 0, 0, 28) : rgb_alpha_pixel(255, 255, 255, 128));
}


void board_display::load_res() {

    char buf[1024] = {};
#ifdef _WIN32
	GetModuleFileName(NULL, buf, sizeof(buf)-1);
	std::string sep = "\\";
#else
	readlink("/proc/self/exe", buf, sizeof(buf)-1);
	std::string sep = "/";
#endif

    std::string path_base(buf);
	// Delete file name.
	auto pos_filename = path_base.rfind(sep);
	if(pos_filename != std::string::npos){
		path_base = path_base.substr(0, pos_filename + 1);
	}

	load_image(path_base + "res/wood.png", img_wood_src);
	load_image(path_base + "res/black.png", img_black_src);
	load_image(path_base + "res/white.png", img_white_src);
}



void board_display::
    set_size (
        unsigned long width,
        unsigned long height
    )
{
    auto_mutex M(m);
    rectangle old(rect);

	auto length = std::min(width, height);
    auto new_radius = (length - edge_size*2) / (boardsize_*2) + 1;

    if (new_radius != radius_ && new_radius > 10) {
        radius_ = new_radius;
		rescale();
		parent.invalidate_rectangle(rect+old);
    }
}

void board_display::on_mouse_move (
            unsigned long,
            long l,
            long t
        ) {

    if (!play_mode)
        return;

    int new_pos = pt_to_stone_pos(l, t);
    if (mark_xy != new_pos) {
        auto old_pos = mark_xy;
        mark_xy = new_pos;
        parent.invalidate_rectangle(stone_rect(old_pos) + stone_rect(new_pos));
    }
}

void board_display::on_mouse_up (
            unsigned long btn,
            unsigned long,
            long l,
            long t
        ) 
{
    if (btn != base_window::LEFT || !play_mode)
        return;

    int new_mark = pt_to_stone_pos(l, t);
    if (new_mark != -1 && board_[new_mark] == 0) {

        indicate_xy = -1;
        if (onMoveClick) {
            if (onMoveClick(black_move, new_mark)) {
                update(black_move, new_mark);
            }
        }
        else {
            update(black_move, new_mark);
        }
    }
}


bool board_display::update(bool black_to_move, int move) {

    if (board_.update_board(black_to_move, move)) {
        last_xy = move;
        black_move = !black_to_move;
        parent.invalidate_rectangle(rect);
        return true;
    }
    return false;
}

bool board_display::undo(bool is_black_move) {

    bool prev_color;
    if (board_.undo(prev_color, last_xy)) {
        black_move = is_black_move;
        parent.invalidate_rectangle(rect);
        return true;
    }
    return false;
}

void board_display::reset(int bdsize) {
    last_xy = -1;
    board_.reset(bdsize);
    black_move = true;
    indicate_xy = -1;
    parent.invalidate_rectangle(rect);
}


void board_display::indicate(int pos) { 
    indicate_xy = pos; 
    parent.invalidate_rectangle(stone_rect(pos));
}


go_window::go_window()
:board(*this)
{
    auto_mutex M(wm);

    board.set_pos(0, 0);
    set_size(board.width(), board.height());

    show();
}


void go_window::on_window_resized() {

	drawable_window::on_window_resized();
	
    unsigned long width, height;
    get_size(width,height);
	board.set_size(width,height);        
}

bool go_window::update(bool black_to_move, int move) {

    return board.update(black_to_move, move);
}

bool go_window::undo(bool is_black_move) {

    return board.undo(is_black_move);
}


void go_window::reset(int bdsize) {
    board.reset(bdsize);
}

//  ----------------------------------------------------------------------------
