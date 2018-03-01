#include "ui.h"
#include "image_loader/png_loader.h"
#include <fstream>
#include "pixel.h"
#include "matrix/rectangle.h"
#include "matrix/matrix_utilities.h"


namespace dlib
{
    // ----------------------------------------------------------------------------------------

    template <
        typename P,
        int type = static_switch<
            pixel_traits<P>::grayscale,
            pixel_traits<P>::rgb,
            pixel_traits<P>::hsi,
            pixel_traits<P>::rgb_alpha,
            pixel_traits<P>::lab
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
            m(0) = static_cast<typename M::type>(pixel);
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
        }
    };

    template <typename P>
    struct pixel_to_vector_helper<P,3>
    {
        template <typename M>
        static void assign (
            M& m,
            const P& pixel
        )
        {
            m(0) = static_cast<typename M::type>(pixel.h);
            m(1) = static_cast<typename M::type>(pixel.s);
            m(2) = static_cast<typename M::type>(pixel.i);
        }
    };

    template <typename P>
    struct pixel_to_vector_helper<P,4>
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

    template <typename P>
    struct pixel_to_vector_helper<P,5>
    {
        template <typename M>
        static void assign (
                M& m,
                const P& pixel
        )
        {
            m(0) = static_cast<typename M::type>(pixel.l);
            m(1) = static_cast<typename M::type>(pixel.a);
            m(2) = static_cast<typename M::type>(pixel.b);
        }
    };


    template <
        typename T,
        typename P
        >
    inline const matrix<T,pixel_traits<P>::num,1> pixel_to_vector (
        const P& pixel
    )
    {
        COMPILE_TIME_ASSERT(pixel_traits<P>::num > 0);
        matrix<T,pixel_traits<P>::num,1> m;
        pixel_to_vector_helper<P>::assign(m,pixel);
        return m;
    }
    
// ----------------------------------------------------------------------------------------

    template <
        typename P,
        int type = static_switch<
            pixel_traits<P>::grayscale,
            pixel_traits<P>::rgb,
            pixel_traits<P>::hsi,
            pixel_traits<P>::rgb_alpha,
            pixel_traits<P>::lab
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
            pixel = static_cast<unsigned char>(m(0));
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
        }
    };

    template <typename P>
    struct vector_to_pixel_helper<P,3>
    {
        template <typename M>
        static void assign (
            P& pixel,
            const M& m
        )
        {
            pixel.h = static_cast<unsigned char>(m(0));
            pixel.s = static_cast<unsigned char>(m(1));
            pixel.i = static_cast<unsigned char>(m(2));
        }
    };

    template <typename P>
    struct vector_to_pixel_helper<P,4>
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

    template <typename P>
    struct vector_to_pixel_helper<P,5>
    {
        template <typename M>
        static void assign (
                P& pixel,
                const M& m
        )
        {
            pixel.l = static_cast<unsigned char>(m(0));
            pixel.a = static_cast<unsigned char>(m(1));
            pixel.b = static_cast<unsigned char>(m(2));
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
        COMPILE_TIME_ASSERT(matrix_exp<EXP>::NC == 1);
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
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

    class black_background
    {
    public:
        template <typename pixel_type>
        void operator() ( pixel_type& p) const { assign_pixel(p, 0); }
    };

    class white_background
    {
    public:
        template <typename pixel_type>
        void operator() ( pixel_type& p) const { assign_pixel(p, 255); }
    };

    class no_background
    {
    public:
        template <typename pixel_type>
        void operator() ( pixel_type& ) const { }
    };

// ----------------------------------------------------------------------------------------

    template <
        typename image_type1,
        typename image_type2,
        typename interpolation_type,
        typename point_mapping_type,
        typename background_type
        >
    void transform_image (
        const image_type1& in_img,
        image_type2& out_img,
        const interpolation_type& interp,
        const point_mapping_type& map_point,
        const background_type& set_background,
        const rectangle& area
    )
    {
        // make sure requires clause is not broken
        DLIB_ASSERT( get_rect(out_img).contains(area) == true &&
                     is_same_object(in_img, out_img) == false ,
            "\t void transform_image()"
            << "\n\t Invalid inputs were given to this function."
            << "\n\t get_rect(out_img).contains(area): " << get_rect(out_img).contains(area)
            << "\n\t get_rect(out_img): " << get_rect(out_img)
            << "\n\t area:              " << area
            << "\n\t is_same_object(in_img, out_img):  " << is_same_object(in_img, out_img)
            );

        const_image_view<image_type1> imgv(in_img);
        image_view<image_type2> out_imgv(out_img);

        for (long r = area.top(); r <= area.bottom(); ++r)
        {
            for (long c = area.left(); c <= area.right(); ++c)
            {
                if (!interp(imgv, map_point(dlib::vector<double,2>(c,r)), out_imgv[r][c]))
                    set_background(out_imgv[r][c]);
            }
        }
    }

// ----------------------------------------------------------------------------------------

    template <
        typename image_type1,
        typename image_type2,
        typename interpolation_type,
        typename point_mapping_type,
        typename background_type
        >
    void transform_image (
        const image_type1& in_img,
        image_type2& out_img,
        const interpolation_type& interp,
        const point_mapping_type& map_point,
        const background_type& set_background
    )
    {
        // make sure requires clause is not broken
        DLIB_ASSERT( is_same_object(in_img, out_img) == false ,
            "\t void transform_image()"
            << "\n\t Invalid inputs were given to this function."
            << "\n\t is_same_object(in_img, out_img):  " << is_same_object(in_img, out_img)
            );

        transform_image(in_img, out_img, interp, map_point, set_background, get_rect(out_img));
    }

// ----------------------------------------------------------------------------------------

    template <
        typename image_type1,
        typename image_type2,
        typename interpolation_type,
        typename point_mapping_type
        >
    void transform_image (
        const image_type1& in_img,
        image_type2& out_img,
        const interpolation_type& interp,
        const point_mapping_type& map_point
    )
    {
        // make sure requires clause is not broken
        DLIB_ASSERT( is_same_object(in_img, out_img) == false ,
            "\t void transform_image()"
            << "\n\t Invalid inputs were given to this function."
            << "\n\t is_same_object(in_img, out_img):  " << is_same_object(in_img, out_img)
            );


        transform_image(in_img, out_img, interp, map_point, black_background(), get_rect(out_img));
    }

// ----------------------------------------------------------------------------------------

    namespace impl
    {
        class helper_resize_image 
        {
        public:
            helper_resize_image(
                double x_scale_,
                double y_scale_
            ):
                x_scale(x_scale_),
                y_scale(y_scale_)
            {}

            dlib::vector<double,2> operator() (
                const dlib::vector<double,2>& p
            ) const
            {
                return dlib::vector<double,2>(p.x()*x_scale, p.y()*y_scale);
            }

        private:
            const double x_scale;
            const double y_scale;
        };
    }

    template <
        typename image_type1,
        typename image_type2,
        typename interpolation_type
        >
    void resize_image (
        const image_type1& in_img,
        image_type2& out_img,
        const interpolation_type& interp
    )
    {
        // make sure requires clause is not broken
        DLIB_ASSERT( is_same_object(in_img, out_img) == false ,
            "\t void resize_image()"
            << "\n\t Invalid inputs were given to this function."
            << "\n\t is_same_object(in_img, out_img):  " << is_same_object(in_img, out_img)
            );

        const double x_scale = (num_columns(in_img)-1)/(double)std::max<long>((num_columns(out_img)-1),1);
        const double y_scale = (num_rows(in_img)-1)/(double)std::max<long>((num_rows(out_img)-1),1);
        transform_image(in_img, out_img, interp, 
                        ::impl::helper_resize_image(x_scale,y_scale));
    }

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    struct is_rgb_image { const static bool value = pixel_traits<typename image_traits<image_type>::pixel_type>::rgb; };
    template <typename image_type>
    struct is_grayscale_image { const static bool value = pixel_traits<typename image_traits<image_type>::pixel_type>::grayscale; };

    // This is an optimized version of resize_image for the case where bilinear
    // interpolation is used.
    template <
        typename image_type1,
        typename image_type2
        >
    typename disable_if_c<(is_rgb_image<image_type1>::value&&is_rgb_image<image_type2>::value) || 
                          (is_grayscale_image<image_type1>::value&&is_grayscale_image<image_type2>::value)>::type 
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
        typedef typename image_traits<image_type2>::pixel_type U;
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
            if (pixel_traits<U>::grayscale)
            {
                for (long c = 0; c < out_img.nc(); ++c)
                {
                    x += x_scale;
                    const long left   = static_cast<long>(std::floor(x));
                    const long right  = std::min(left+1, in_img.nc()-1);
                    const double lr_frac = x - left;

                    double tl = 0, tr = 0, bl = 0, br = 0;

                    assign_pixel(tl, in_img[top][left]);
                    assign_pixel(tr, in_img[top][right]);
                    assign_pixel(bl, in_img[bottom][left]);
                    assign_pixel(br, in_img[bottom][right]);

                    double temp = (1-tb_frac)*((1-lr_frac)*tl + lr_frac*tr) + 
                        tb_frac*((1-lr_frac)*bl + lr_frac*br);

                    assign_pixel(out_img[r][c], temp);
                }
            }
            else
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

    template <
        typename image_type
        >
    void resize_image (
        double size_scale,
        image_type& img 
    )
    {
        // make sure requires clause is not broken
        DLIB_ASSERT( size_scale > 0 ,
            "\t void resize_image()"
            << "\n\t Invalid inputs were given to this function."
            << "\n\t size_scale:  " << size_scale
            );

        image_type temp;
        set_image_size(temp, std::round(size_scale*num_rows(img)), std::round(size_scale*num_columns(img)));
        resize_image(img, temp);
        swap(img, temp);
    }


// ----------------------------------------------------------------------------------------

    template <typename pixel_type>
    void draw_line (
        const canvas& c,
        const point& p1,
        const point& p2,
        const pixel_type& pixel, 
        const rectangle& area = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        rectangle valid_area(c.intersect(area));
        long x1 = p1.x();
        long y1 = p1.y();
        long x2 = p2.x();
        long y2 = p2.y();
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
    inline void draw_line (
        const canvas& c,
        const point& p1,
        const point& p2
    ){ draw_line(c,p1,p2,0); }


// ----------------------------------------------------------------------------------------

    template <typename pixel_type>
    void draw_circle (
        const canvas& c,
        const point& center_point,
        double radius,
        const pixel_type& pixel,
        const rectangle& area = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        using std::sqrt;
        rectangle valid_area(c.intersect(area));
        const long x = center_point.x();
        const long y = center_point.y();
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
    inline void draw_circle (
        const canvas& c,
        const point& center_point,
        double radius
    ){ draw_circle(c, center_point, radius, 0); }

// ----------------------------------------------------------------------------------------

    template <typename pixel_type>
    void draw_solid_circle (
        const canvas& c,
        const point& center_point,
        double radius,
        const pixel_type& pixel,
        const rectangle& area = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        using std::sqrt;
        rectangle valid_area(c.intersect(area));
        const long x = center_point.x();
        const long y = center_point.y();
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
                    draw_line(c, point(i,top),point(i,bottom),pixel,area);
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
                    draw_line(c, point(i,top),point(i,bottom),pixel,area);
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
    inline void draw_solid_circle (
        const canvas& c,
        const point& center_point,
        double radius
    ) { draw_solid_circle(c, center_point, radius, 0); }

// ----------------------------------------------------------------------------------------

    template <
        typename image_type 
        >
    void draw_image (
        const canvas& c,
        const point& p,
        const image_type& img,
        const rectangle& area_ = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        const long x = p.x();
        const long y = p.y();
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

// ----------------------------------------------------------------------------------------

    template <
        typename image_type 
        >
    void draw_image (
        const canvas& c,
        const rectangle& rect,
        const image_type& img,
        const rectangle& area_ = rectangle(std::numeric_limits<long>::min(), std::numeric_limits<long>::min(),
                                          std::numeric_limits<long>::max(), std::numeric_limits<long>::max())
    )
    {
        const rectangle area = c.intersect(rect).intersect(area_);
        if (area.is_empty() || num_columns(img) * num_rows(img) == 0)
            return;

        const matrix<long,1> x = matrix_cast<long>(round(linspace(0, num_columns(img)-1, rect.width())));
        const matrix<long,1> y = matrix_cast<long>(round(linspace(0, num_rows(img)-1, rect.height())));

        for (long row = area.top(); row <= area.bottom(); ++row)
        {
            const long r = y(row-rect.top());
            long cc = area.left() - rect.left();
            for (long col = area.left(); col <= area.right(); ++col)
            {
                assign_pixel(c[row-c.top()][col-c.left()], img[r][x(cc++)]);
            }
        }
    }



go_window::go_window(int bdsize)
: board_size(bdsize) {
    
    load_res();
    scale_res();

    auto_mutex M(wm);
    const auto r = dlib::get_rect(img_wood);
    set_size(r.width(), r.height());
    show();
}

void go_window::load_res() {

    load_png(img_wood_src, "../wood_1024.png");
    load_png(img_black_src, "../black_64.png");
    load_png(img_white_src, "../white_64.png");
}

void go_window::scale_res() {

    auto_mutex M(wm);

    long wood_size = radius*2 * board_size + edge_size*2;
    img_wood.set_size(wood_size, wood_size);
    resize_image(img_wood_src, img_wood);

    img_black.set_size(radius*2, radius*2);
    resize_image(img_black_src, img_black);

    img_white.set_size(radius*2, radius*2);
    resize_image(img_white_src, img_white);
}


void go_window::paint(const canvas& c) {

    c.fill(212,208,200);

    draw_image(c, point(0, 0), img_wood);
    draw_grid(c);
    draw_stones(c);
}

void go_window::draw_grid(const canvas& c) {

    long len = radius*2* (board_size - 1);
    long start = radius+edge_size;

    long y = start;
    for (int i=0; i< board_size; i++, y+= radius*2) {

        draw_line (c, point(start, y), point(start + len, y), rgb_alpha_pixel(0, 0, 0, 100));
    }

    long x = start;
    for (int i=0; i< board_size; i++, x+= radius*2) {
        
        draw_line (c, point(x, start), point(x, start + len), rgb_alpha_pixel(0, 0, 0, 100));
    }

    // deeper boarder
    draw_line (c, point(start-1, start-1), point(start + len + 1, start-1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start-1, start + len + 1), point(start + len + 1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start-1, start-1), point(start-1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));
    draw_line (c, point(start + len + 1, start-1), point(start + len + 1, start + len + 1), rgb_alpha_pixel(0, 0, 0, 100));

    //  star
    if (board_size == 19) {
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
    for (int y=0; y< board_size; y++) {
        for (int x=0; x< board_size; x++, pos++) {
            if (board[pos] != 0) {
                int mark = (last_xy == pos) ? 1 : 0;
                if (board[pos] == 1)
                    draw_stone(c, x, y, 1, mark);
                else if (board[pos] == -1) {
                    draw_stone(c, x, y, 0, mark);
                }
            }
        }
    }
}

void go_window::on_window_resized() {

    unsigned long width, height;
    get_size(width,height);

    auto length = std::min(width, height);
    auto new_radius = (length - edge_size*2) / (board_size*2);

    if (new_radius != radius && new_radius > 10) {
        radius = new_radius;
        scale_res();
    }
}

void go_window::draw_stone(const canvas& c, long x, long y, int black, int mark) {
    auto center = loc(x, y);
    auto left = center.x() - radius;
    auto top = center.y() - radius;
    draw_image(c, point(left, top), black ? img_black : img_white);

    if (mark)
        draw_circle(c, center, radius*0.6, black ? rgb_alpha_pixel(255, 255, 255, 255) : rgb_alpha_pixel(0, 0, 0, 255));
}


void go_window::update(int move, const GoBoard& _board) {

    last_xy = move;
    for (int i=0; i< board_size*board_size; i++)
        board[i] = _board[i];
    invalidate_rectangle(get_rect(img_wood));
}

//  ----------------------------------------------------------------------------

