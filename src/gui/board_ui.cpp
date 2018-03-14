#include "board_ui.h"
#include <fstream>
#include "pixel.h"
#include "rectangle.h"
#include "matrix.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_INLINE
#include "../stb/stb_image.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace dlib
{
    // ----------------------------------------------------------------------------------------

    template <
        typename P,
        int type = static_switch<
            pixel_traits<P>::rgb,
            pixel_traits<P>::rgb_alpha
            >::value
        >
    struct pixel_to_vector_helper;


    template <typename P>
    struct pixel_to_vector_helper<P,1>
    {
        template <typename M>
        static void assign (
            M& m,
            const P& pixel
        )
        {
            m(0) = static_cast<typename M::type>(pixel.red);
            m(1) = static_cast<typename M::type>(pixel.green);
            m(2) = static_cast<typename M::type>(pixel.blue);
        }
    };

    template <typename P>
    struct pixel_to_vector_helper<P,2>
    {
        template <typename M>
        static void assign (
            M& m,
            const P& pixel
        )
        {
            m(0) = static_cast<typename M::type>(pixel.red);
            m(1) = static_cast<typename M::type>(pixel.green);
            m(2) = static_cast<typename M::type>(pixel.blue);
            m(3) = static_cast<typename M::type>(pixel.alpha);
        }
    };

    template <
        typename T,
        typename P
        >
    inline const matrix<T,pixel_traits<P>::num> pixel_to_vector (
        const P& pixel
    )
    {
        COMPILE_TIME_ASSERT(pixel_traits<P>::num > 0);
        matrix<T,pixel_traits<P>::num> m;
        pixel_to_vector_helper<P>::assign(m,pixel);
        return m;
    }
    
// ----------------------------------------------------------------------------------------

    template <
        typename P,
        int type = static_switch<
            pixel_traits<P>::rgb,
            pixel_traits<P>::rgb_alpha
            >::value
        >
    struct vector_to_pixel_helper;

    template <typename P>
    struct vector_to_pixel_helper<P,1>
    {
        template <typename M>
        static void assign (
            P& pixel,
            const M& m
        )
        {
            pixel.red = static_cast<unsigned char>(m(0));
            pixel.green = static_cast<unsigned char>(m(1));
            pixel.blue = static_cast<unsigned char>(m(2));
        }
    };

    template <typename P>
    struct vector_to_pixel_helper<P,2>
    {
        template <typename M>
        static void assign (
            P& pixel,
            const M& m
        )
        {
            pixel.red = static_cast<unsigned char>(m(0));
            pixel.green = static_cast<unsigned char>(m(1));
            pixel.blue = static_cast<unsigned char>(m(2));
            pixel.alpha = static_cast<unsigned char>(m(3));
        }
    };

    template <
        typename P,
        typename EXP
        >
    inline void vector_to_pixel (
        P& pixel,
        const matrix_exp<EXP>& vector 
    )
    {
        COMPILE_TIME_ASSERT(pixel_traits<P>::num == matrix_exp<EXP>::NR);
        vector_to_pixel_helper<P>::assign(pixel,vector);
    }


    template <typename T, typename mm>
    struct image_traits<array2d<T,mm> >
    {
        typedef T pixel_type;
    };
    template <typename T, typename mm>
    struct image_traits<const array2d<T,mm> >
    {
        typedef T pixel_type;
    };

    template <typename T, typename mm>
    inline long num_rows( const array2d<T,mm>& img) { return img.nr(); }
    template <typename T, typename mm>
    inline long num_columns( const array2d<T,mm>& img) { return img.nc(); }

    template <typename T, typename mm>
    inline void set_image_size(
        array2d<T,mm>& img,
        long rows,
        long cols 
    ) { img.set_size(rows,cols); }

    template <typename T, typename mm>
    inline void* image_data(
        array2d<T,mm>& img
    )
    {
        if (img.size() != 0)
            return &img[0][0];
        else
            return 0;
    }

    template <typename T, typename mm>
    inline const void* image_data(
        const array2d<T,mm>& img
    )
    {
        if (img.size() != 0)
            return &img[0][0];
        else
            return 0;
    }

    template <typename T, typename mm>
    inline long width_step(
        const array2d<T,mm>& img
    ) 
    { 
        return img.width_step(); 
    }

}

// ----------------------------------------------------------------------------------------

    class interpolate_bilinear
    {
    };

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    struct is_rgb_image { const static bool value = pixel_traits<typename image_traits<image_type>::pixel_type>::rgb; };

    // This is an optimized version of resize_image for the case where bilinear
    // interpolation is used.
    template <
        typename image_type1,
        typename image_type2
        >
    typename disable_if_c<(is_rgb_image<image_type1>::value&&is_rgb_image<image_type2>::value)>::type 
    resize_image (
        const image_type1& in_img_,
        image_type2& out_img_,
        interpolate_bilinear
    )
    {
        // make sure requires clause is not broken
        DLIB_ASSERT( is_same_object(in_img_, out_img_) == false ,
            "\t void resize_image()"
            << "\n\t Invalid inputs were given to this function."
            << "\n\t is_same_object(in_img_, out_img_):  " << is_same_object(in_img_, out_img_)
            );

        const_image_view<image_type1> in_img(in_img_);
        image_view<image_type2> out_img(out_img_);

        if (out_img.size() == 0 || in_img.size() == 0)
            return;


        typedef typename image_traits<image_type1>::pixel_type T;
        const double x_scale = (in_img.nc()-1)/(double)std::max<long>((out_img.nc()-1),1);
        const double y_scale = (in_img.nr()-1)/(double)std::max<long>((out_img.nr()-1),1);
        double y = -y_scale;
        for (long r = 0; r < out_img.nr(); ++r)
        {
            y += y_scale;
            const long top    = static_cast<long>(std::floor(y));
            const long bottom = std::min(top+1, in_img.nr()-1);
            const double tb_frac = y - top;
            double x = -x_scale;
            {
                for (long c = 0; c < out_img.nc(); ++c)
                {
                    x += x_scale;
                    const long left   = static_cast<long>(std::floor(x));
                    const long right  = std::min(left+1, in_img.nc()-1);
                    const double lr_frac = x - left;

                    const T tl = in_img[top][left];
                    const T tr = in_img[top][right];
                    const T bl = in_img[bottom][left];
                    const T br = in_img[bottom][right];

                    T temp;
                    assign_pixel(temp, 0);
                    vector_to_pixel(temp, 
                        (1-tb_frac)*((1-lr_frac)*pixel_to_vector<double>(tl) + lr_frac*pixel_to_vector<double>(tr)) + 
                            tb_frac*((1-lr_frac)*pixel_to_vector<double>(bl) + lr_frac*pixel_to_vector<double>(br)));
                    assign_pixel(out_img[r][c], temp);
                }
            }
        }
    }


// ----------------------------------------------------------------------------------------

    template <
        typename image_type1,
        typename image_type2
        >
    void resize_image (
        const image_type1& in_img,
        image_type2& out_img
    )
    {
        // make sure requires clause is not broken
        DLIB_ASSERT( is_same_object(in_img, out_img) == false ,
            "\t void resize_image()"
            << "\n\t Invalid inputs were given to this function."
            << "\n\t is_same_object(in_img, out_img):  " << is_same_object(in_img, out_img)
            );

        resize_image(in_img, out_img, interpolate_bilinear());
    }

// ----------------------------------------------------------------------------------------

    template <typename pixel_type>
    void draw_line (
        const canvas& c,
        const std::array<long,2>& p1,
        const std::array<long,2>& p2,
        const pixel_type& pixel, 
        const rectangle& area = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        rectangle valid_area(c.intersect(area));
        long x1 = p1[0];
        long y1 = p1[1];
        long x2 = p2[0];
        long y2 = p2[1];
        if (x1 == x2)
        {
            // if the x coordinate is inside the canvas's area
            if (x1 <= valid_area.right() && x1 >= valid_area.left())
            {
                // make sure y1 comes before y2
                if (y1 > y2)
                    swap(y1,y2);

                y1 = std::max(y1,valid_area.top());
                y2 = std::min(y2,valid_area.bottom());
                // this is a vertical line
                for (long y = y1; y <= y2; ++y)
                {
                    assign_pixel(c[y-c.top()][x1-c.left()], pixel);
                }
            }
        }
        else if (y1 == y2)
        {
            // if the y coordinate is inside the canvas's area
            if (y1 <= valid_area.bottom() && y1 >= valid_area.top())
            {
                // make sure x1 comes before x2
                if (x1 > x2)
                    swap(x1,x2);

                x1 = std::max(x1,valid_area.left());
                x2 = std::min(x2,valid_area.right());
                // this is a horizontal line
                for (long x = x1; x <= x2; ++x)
                {
                    assign_pixel(c[y1-c.top()][x-c.left()], pixel);
                }
            }
        }
        else
        {
            rgb_alpha_pixel alpha_pixel;
            assign_pixel(alpha_pixel, pixel);
            const unsigned char max_alpha = alpha_pixel.alpha;

            const long rise = (((long)y2) - ((long)y1));
            const long run = (((long)x2) - ((long)x1));
            if (std::abs(rise) < std::abs(run))
            {
                const double slope = ((double)rise)/run;

                double first, last;

                if (x1 > x2)                
                {
                    first = std::max(x2,valid_area.left());
                    last = std::min(x1,valid_area.right());
                }
                else
                {
                    first = std::max(x1,valid_area.left());
                    last = std::min(x2,valid_area.right());
                }                             


                long y;
                long x;
                const double x1f = x1;
                const double y1f = y1;
                for (double i = first; i <= last; ++i)
                {   
                    const double dy = slope*(i-x1f) + y1f;
                    const double dx = i;

                    y = static_cast<long>(dy);
                    x = static_cast<long>(dx);


                    if (y >= valid_area.top() && y <= valid_area.bottom())
                    {
                        alpha_pixel.alpha = static_cast<unsigned char>((1.0-(dy-y))*max_alpha);
                        assign_pixel(c[y-c.top()][x-c.left()], alpha_pixel);
                    }
                    if (y+1 >= valid_area.top() && y+1 <= valid_area.bottom())
                    {
                        alpha_pixel.alpha = static_cast<unsigned char>((dy-y)*max_alpha);
                        assign_pixel(c[y+1-c.top()][x-c.left()], alpha_pixel);
                    }
                }         
            }
            else
            {
                const double slope = ((double)run)/rise;

                double first, last;

                if (y1 > y2)                
                {
                    first = std::max(y2,valid_area.top());
                    last = std::min(y1,valid_area.bottom());
                }
                else
                {
                    first = std::max(y1,valid_area.top());
                    last = std::min(y2,valid_area.bottom());
                }                             

                long x;
                long y;
                const double x1f = x1;
                const double y1f = y1;
                for (double i = first; i <= last; ++i)
                {   
                    const double dx = slope*(i-y1f) + x1f;
                    const double dy = i;

                    y = static_cast<long>(dy);
                    x = static_cast<long>(dx);

                    if (x >= valid_area.left() && x <= valid_area.right())
                    {
                        alpha_pixel.alpha = static_cast<unsigned char>((1.0-(dx-x))*max_alpha);
                        assign_pixel(c[y-c.top()][x-c.left()], alpha_pixel);
                    }
                    if (x+1 >= valid_area.left() && x+1 <= valid_area.right())
                    {
                        alpha_pixel.alpha = static_cast<unsigned char>((dx-x)*max_alpha);
                        assign_pixel(c[y-c.top()][x+1-c.left()], alpha_pixel);
                    }
                } 
            }
        }

    }


// ----------------------------------------------------------------------------------------

    template <typename pixel_type>
    void draw_circle (
        const canvas& c,
        const std::array<long,2>& center_point,
        double radius,
        const pixel_type& pixel,
        const rectangle& area = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        using std::sqrt;
        rectangle valid_area(c.intersect(area));
        const long x = center_point[0];
        const long y = center_point[1];
        if (radius > 1)
        {
            long first_x = static_cast<long>(x - radius + 0.5);
            long last_x = static_cast<long>(x + radius + 0.5);
            const double rs = radius*radius;

            // ensure that we only loop over the part of the x dimension that this
            // canvas contains.
            if (first_x < valid_area.left())
                first_x = valid_area.left();
            if (last_x > valid_area.right())
                last_x = valid_area.right();

            long top, bottom;

            top = static_cast<long>(sqrt(std::max(rs - (first_x-x-0.5)*(first_x-x-0.5),0.0))+0.5);
            top += y;
            long last = top;

            // draw the left half of the circle
            long middle = std::min(x-1,last_x);
            for (long i = first_x; i <= middle; ++i)
            {
                double a = i - x + 0.5;
                // find the top of the arc
                top = static_cast<long>(sqrt(std::max(rs - a*a,0.0))+0.5);
                top += y;
                long temp = top;

                while(top >= last) 
                {
                    bottom = y - top + y;
                    if (top >= valid_area.top() && top <= valid_area.bottom() )
                    {
                        assign_pixel(c[top-c.top()][i-c.left()],pixel);
                    }

                    if (bottom >= valid_area.top() && bottom <= valid_area.bottom() )
                    {
                        assign_pixel(c[bottom-c.top()][i-c.left()],pixel);
                    }
                    --top;
                }

                last = temp;
            }

            middle = std::max(x,first_x);
            top = static_cast<long>(sqrt(std::max(rs - (last_x-x+0.5)*(last_x-x+0.5),0.0))+0.5);
            top += y;
            last = top;
            // draw the right half of the circle
            for (long i = last_x; i >= middle; --i)
            {
                double a = i - x - 0.5;
                // find the top of the arc
                top = static_cast<long>(sqrt(std::max(rs - a*a,0.0))+0.5);
                top += y;
                long temp = top;

                while(top >= last) 
                {
                    bottom = y - top + y;
                    if (top >= valid_area.top() && top <= valid_area.bottom() )
                    {
                        assign_pixel(c[top-c.top()][i-c.left()],pixel);
                    }

                    if (bottom >= valid_area.top() && bottom <= valid_area.bottom() )
                    {
                        assign_pixel(c[bottom-c.top()][i-c.left()],pixel);
                    }
                    --top;
                }

                last = temp;
            }
        }
        else if (radius == 1 &&
                 x >= valid_area.left() && x <= valid_area.right() &&
                 y >= valid_area.top() && y <= valid_area.bottom() )
        {
            assign_pixel(c[y-c.top()][x-c.left()], pixel);
        }
    }

// ----------------------------------------------------------------------------------------

    template <typename pixel_type>
    void draw_solid_circle (
        const canvas& c,
        const std::array<long,2>& center_point,
        double radius,
        const pixel_type& pixel,
        const rectangle& area = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        using std::sqrt;
        rectangle valid_area(c.intersect(area));
        const long x = center_point[0];
        const long y = center_point[1];
        if (radius > 1)
        {
            long first_x = static_cast<long>(x - radius + 0.5);
            long last_x = static_cast<long>(x + radius + 0.5);
            const double rs = radius*radius;

            // ensure that we only loop over the part of the x dimension that this
            // canvas contains.
            if (first_x < valid_area.left())
                first_x = valid_area.left();
            if (last_x > valid_area.right())
                last_x = valid_area.right();

            long top, bottom;

            top = static_cast<long>(sqrt(std::max(rs - (first_x-x-0.5)*(first_x-x-0.5),0.0))+0.5);
            top += y;
            long last = top;

            // draw the left half of the circle
            long middle = std::min(x-1,last_x);
            for (long i = first_x; i <= middle; ++i)
            {
                double a = i - x + 0.5;
                // find the top of the arc
                top = static_cast<long>(sqrt(std::max(rs - a*a,0.0))+0.5);
                top += y;
                long temp = top;

                while(top >= last) 
                {
                    bottom = y - top + y;
                    draw_line(c, {i,top}, {i,bottom},pixel,area);
                    --top;
                }

                last = temp;
            }

            middle = std::max(x,first_x);
            top = static_cast<long>(sqrt(std::max(rs - (last_x-x+0.5)*(last_x-x+0.5),0.0))+0.5);
            top += y;
            last = top;
            // draw the right half of the circle
            for (long i = last_x; i >= middle; --i)
            {
                double a = i - x - 0.5;
                // find the top of the arc
                top = static_cast<long>(sqrt(std::max(rs - a*a,0.0))+0.5);
                top += y;
                long temp = top;

                while(top >= last) 
                {
                    bottom = y - top + y;
                    draw_line(c, {i,top}, {i,bottom},pixel,area);
                    --top;
                }

                last = temp;
            }
        }
        else if (radius == 1 &&
                 x >= valid_area.left() && x <= valid_area.right() &&
                 y >= valid_area.top() && y <= valid_area.bottom() )
        {
            assign_pixel(c[y-c.top()][x-c.left()], pixel);
        }
    }


// ----------------------------------------------------------------------------------------

    template <
        typename image_type 
        >
    void draw_image (
        const canvas& c,
        const std::array<long,2>& p,
        const image_type& img,
        const rectangle& area_ = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        const long x = p[0];
        const long y = p[1];
        rectangle rect(x,y,num_columns(img)+x-1,num_rows(img)+y-1);
        rectangle area = c.intersect(rect).intersect(area_);
        if (area.is_empty())
            return;

        for (long row = area.top(); row <= area.bottom(); ++row)
        {
            for (long col = area.left(); col <= area.right(); ++col)
            {
                assign_pixel(c[row-c.top()][col-c.left()], img[row-rect.top()][col-rect.left()]);
            }
        }
    }


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

    draw_image(c, {0, 0}, img_wood);
    draw_grid(c);
    draw_stones(c);
}

void go_window::draw_grid(const canvas& c) {

    long len = radius*2* (board_.board_size() - 1);
    long start = radius+edge_size;

    long y = start;
    for (int i=0; i< board_.board_size(); i++, y+= radius*2) {

        draw_line (c, {start, y}, {start + len, y}, rgb_alpha_pixel(0, 0, 0, 100));
    }

    long x = start;
    for (int i=0; i< board_.board_size(); i++, x+= radius*2) {
        
        draw_line (c, {x, start}, {x, start + len}, rgb_alpha_pixel(0, 0, 0, 100));
    }

    // deeper boarder
    draw_line (c, {start-1, start-1}, {start + len + 1, start-1}, rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, {start-1, start + len + 1}, {start + len + 1, start + len + 1}, rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, {start-1, start-1}, {start-1, start + len + 1}, rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, {start + len + 1, start-1}, {start + len + 1, start + len + 1}, rgb_alpha_pixel(0, 0, 0, 100));

    //  star
    if (board_.board_size() == 19) {
        draw_solid_circle(c, loc(3,3), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, loc(3,9), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, loc(3,15), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, loc(9,3), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, loc(9,9), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, loc(9,15), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, loc(15,3), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, loc(15,9), 3, rgb_alpha_pixel(0, 0, 0, 100));
        draw_solid_circle(c, loc(15,15), 3, rgb_alpha_pixel(0, 0, 0, 100));
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
                draw_solid_circle(c, center, radius*0.7, rgb_alpha_pixel(255, 0, 0, 128));
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
        draw_image(c, {left, top}, black ? img_black : img_white);
    }
    else {
        draw_solid_circle(c, center, radius-1, black ? rgb_alpha_pixel(0, 0, 0, 255) : rgb_alpha_pixel(255, 255, 255, 255));
        draw_circle(c, center, radius, rgb_alpha_pixel(102, 102, 102, 255));
    }

    if (mark)
        draw_mark(c, x, y, black ? rgb_alpha_pixel(255, 255, 255, 255) : rgb_alpha_pixel(0, 0, 0, 255));
}

void go_window::draw_mark(const canvas& c, long x, long y, rgb_alpha_pixel pixel) {
    auto center = loc(x, y);
    draw_circle(c, center, radius*0.6, pixel);
}

void go_window::draw_shadow_stone(const canvas& c, long x, long y, bool black) {
    auto center = loc(x, y);
    draw_solid_circle(c, center, radius*0.7, black ? rgb_alpha_pixel(0, 0, 0, 128) : rgb_alpha_pixel(255, 255, 255, 128));
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