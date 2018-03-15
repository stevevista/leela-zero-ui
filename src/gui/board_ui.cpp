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
: drawable(w) {

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
            assign_pixel(c[rc][col-c.left()], img_wood[row][col]);
        }
    }
}

go_window::go_window()
{
    
    load_res();
    scale_res();

    auto_mutex M(wm);
    set_size(rect_.width(), rect_.height());

    show();
}

void go_window::load_res() {

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

void go_window::scale_res() {

    auto_mutex M(wm);

    long wood_size = radius*2 * board_.board_size() + edge_size*2;
    rect_ = rectangle(wood_size, wood_size);

    if (img_wood_src.size()) {
        img_wood.set_size(wood_size, wood_size);
        resize_image(img_wood_src, img_wood);
    }

    if (img_black_src.size()) {
        img_black.set_size(radius*2, radius*2);
        resize_image(img_black_src, img_black);
    }

    if (img_white_src.size()) {
        img_white.set_size(radius*2, radius*2);
        resize_image(img_white_src, img_white);
    }
}


void go_window::paint(const canvas& c) {

    c.fill(255,178,102);

    draw_image(c, point(0, 0), img_wood);
    draw_grid(c);
    draw_stones(c);
}

void go_window::draw_grid(const canvas& c) {

    long len = radius*2* (board_.board_size() - 1);
    long start = radius+edge_size;

    long y = start;
    for (int i=0; i< board_.board_size(); i++, y+= radius*2) {

        draw_line (c, point(start, y), point(start + len, y), rgb_alpha_pixel(0, 0, 0, 100));
    }

    long x = start;
    for (int i=0; i< board_.board_size(); i++, x+= radius*2) {
        
        draw_line (c, point(x, start), point(x, start + len), rgb_alpha_pixel(0, 0, 0, 100));
    }

    // deeper boarder
    draw_line (c, point(start-1, start-1), point(start + len + 1, start-1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start-1, start + len + 1), point(start + len + 1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start-1, start-1), point(start-1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start + len + 1, start-1), point(start + len + 1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));

    //  star
    if (board_.board_size() == 19) {
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

void go_window::draw_stones(const canvas& c) {

    int pos = 0;
    for (int y=0; y< board_.board_size(); y++) {
        for (int x=0; x< board_.board_size(); x++, pos++) {
            if (board_[pos] != 0) {
                int mark = (last_xy == pos) ? 1 : 0;
                if (board_[pos] == 1)
                    draw_stone(c, x, y, 1, mark);
                else if (board_[pos] == -1) {
                    draw_stone(c, x, y, 0, mark);
                }
            }
            if (mark_xy == pos && board_[pos] == 0) {
                draw_shadow_stone(c, x, y, black_move);
            }

            if (indicate_xy == pos) {
                auto center = loc(x, y);
                draw_solid_circle(c, point(center[0], center[1]), radius*0.7, rgb_alpha_pixel(255, 0, 0, 128));
            }
        }
    }
}

void go_window::on_window_resized() {

    unsigned long width, height;
    get_size(width,height);

    auto length = std::min(width, height);
    auto new_radius = (length - edge_size*2) / (board_.board_size()*2);

    if (new_radius != radius && new_radius > 10) {
        radius = new_radius;
        scale_res();
    }
}

void go_window::draw_stone(const canvas& c, long x, long y, int black, int mark) {
    auto center = loc(x, y);
    auto left = center[0] - radius;
    auto top = center[1] - radius;
    //
    if (img_black.size() && img_white.size()) {
        draw_image(c, point(left, top), black ? img_black : img_white);
    }
    else {
        draw_solid_circle(c, point(center[0], center[1]), radius-1, black ? rgb_alpha_pixel(0, 0, 0, 255) : rgb_alpha_pixel(255, 255, 255, 255));
        draw_circle(c, point(center[0], center[1]), radius, rgb_alpha_pixel(102, 102, 102, 255));
    }

    if (mark)
        draw_mark(c, x, y, black ? rgb_alpha_pixel(255, 255, 255, 255) : rgb_alpha_pixel(0, 0, 0, 255));
}

void go_window::draw_mark(const canvas& c, long x, long y, rgb_alpha_pixel pixel) {
    auto center = loc(x, y);
    draw_circle(c, point(center[0], center[1]), radius*0.6, pixel);
}

void go_window::draw_shadow_stone(const canvas& c, long x, long y, bool black) {
    auto center = loc(x, y);
    draw_solid_circle(c, point(center[0], center[1]), radius*0.7, black ? rgb_alpha_pixel(0, 0, 0, 128) : rgb_alpha_pixel(255, 255, 255, 128));
}

bool go_window::update(bool black_to_move, int move, bool refresh) {

    if (board_.update_board(black_to_move, move)) {
        last_xy = move;
        black_move = !black_to_move;
        if (refresh) 
            invalidate();
        return true;
    }
    return false;
}

void go_window::reset(int bdsize, bool refresh) {
    last_xy = -1;
    board_.reset(bdsize);
    black_move = true;
    indicate_xy = -1;
    if (refresh)
        invalidate();
}

void go_window::invalidate() {
    invalidate_rectangle(rect_);
}

//  ----------------------------------------------------------------------------

void go_window::on_mouse_move (
            unsigned long state,
            long l,
            long t
        ) {
    if (!play_mode)
        return;

    const int bdsize = board_.board_size();
    int new_mark = -1;
    int x = (l-edge_size)/(radius*2);
    int y = (t-edge_size)/(radius*2);
    if (l>0 && t > 0 && x >= 0 && x< bdsize && y>=0 && y < bdsize)
        new_mark = (bdsize-1-y)*bdsize + x;

    if (mark_xy != new_mark) {
        mark_xy = new_mark;
        invalidate_rectangle(rect_);
    }

    
}

void go_window::on_mouse_up (
            unsigned long btn,
            unsigned long state,
            long l,
            long t
        ) 
{
    if (btn != LEFT || !play_mode)
        return;

    const int bdsize = board_.board_size();
    int new_mark = -1;
    int x = (l-edge_size)/(radius*2);
    int y = (t-edge_size)/(radius*2);
    if (l>0 && t > 0 && x >= 0 && x< bdsize && y>=0 && y < bdsize)
        new_mark = (bdsize-1-y)*bdsize + x;

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

void go_window::indicate(int pos) { 
    indicate_xy = pos; 
    invalidate_rectangle(rect_);
}