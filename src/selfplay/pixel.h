// Copyright (C) 2006  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_PIXEl_ 
#define DLIB_PIXEl_

#include <iostream>
#include <cmath>
#include "algs.h"
#include "uintn.h"
#include <limits>
#include <complex>
#include "enable_if.h"

namespace dlib
{

// ----------------------------------------------------------------------------------------

    /*!
        This file contains definitions of pixel objects and related classes and
        functionality.
    !*/

// ----------------------------------------------------------------------------------------

    template <typename T>
    struct pixel_traits;
    /*!
        WHAT THIS OBJECT REPRESENTS
            As the name implies, this is a traits class for pixel types.
            It defines the properties of a pixel.

        This traits class will define the following public static members:
            - bool grayscale
            - bool rgb
            - bool rgb_alpha
            - bool hsi
            - bool lab

            - bool has_alpha

            - long num 

            - basic_pixel_type
            - basic_pixel_type min()
            - basic_pixel_type max()
            - is_unsigned

        The above public constants are subject to the following constraints:
            - only one of grayscale, rgb, rgb_alpha, hsi or lab is true
            - if (rgb == true) then
                - The type T will be a struct with 3 public members of type 
                  unsigned char named "red" "green" and "blue".  
                - This type of pixel represents the RGB color space.
                - num == 3
                - has_alpha == false
                - basic_pixel_type == unsigned char
                - min() == 0 
                - max() == 255
                - is_unsigned == true
            - if (rgb_alpha == true) then
                - The type T will be a struct with 4 public members of type 
                  unsigned char named "red" "green" "blue" and "alpha".  
                - This type of pixel represents the RGB color space with
                  an alpha channel where an alpha of 0 represents a pixel
                  that is totally transparent and 255 represents a pixel 
                  with maximum opacity.
                - num == 4
                - has_alpha == true 
                - basic_pixel_type == unsigned char
                - min() == 0 
                - max() == 255
                - is_unsigned == true
            - else if (hsi == true) then
                - The type T will be a struct with 3 public members of type
                  unsigned char named "h" "s" and "i".  
                - This type of pixel represents the HSI color space.
                - num == 3
                - has_alpha == false 
                - basic_pixel_type == unsigned char
                - min() == 0 
                - max() == 255
                - is_unsigned == true
             - else if (lab == true) then
                - The type T will be a struct with 3 public members of type
                  unsigned char named "l" "a" and "b".
                - This type of pixel represents the Lab color space.
                - num == 3
                - has_alpha == false
                - basic_pixel_type == unsigned char
                - min() == 0
                - max() == 255
                - is_unsigned == true 
            - else
                - grayscale == true
                - This type of pixel represents a grayscale color space.  T 
                  will be some kind of basic scalar type such as unsigned int.
                - num == 1
                - has_alpha == false 
                - basic_pixel_type == T 
                - min() == the minimum obtainable value of objects of type T 
                - max() == the maximum obtainable value of objects of type T 
                - is_unsigned is true if min() == 0 and false otherwise
    !*/

// ----------------------------------------------------------------------------------------

    struct rgb_pixel
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This is a simple struct that represents an RGB colored graphical pixel.
        !*/

        rgb_pixel (
        ) {}

        rgb_pixel (
            unsigned char red_,
            unsigned char green_,
            unsigned char blue_
        ) : red(red_), green(green_), blue(blue_) {}

        unsigned char red;
        unsigned char green;
        unsigned char blue;
    };

// ----------------------------------------------------------------------------------------

    struct bgr_pixel
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This is a simple struct that represents an BGR colored graphical pixel.
                (the reason it exists in addition to the rgb_pixel is so you can lay
                it down on top of a memory region that organizes its color data in the
                BGR format and still be able to read it)
        !*/

        bgr_pixel (
        ) {}

        bgr_pixel (
            unsigned char blue_,
            unsigned char green_,
            unsigned char red_
        ) : blue(blue_), green(green_), red(red_) {}

        unsigned char blue;
        unsigned char green;
        unsigned char red;
    };

// ----------------------------------------------------------------------------------------

    struct rgb_alpha_pixel
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This is a simple struct that represents an RGB colored graphical pixel
                with an alpha channel.
        !*/

        rgb_alpha_pixel (
        ) {}

        rgb_alpha_pixel (
            unsigned char red_,
            unsigned char green_,
            unsigned char blue_,
            unsigned char alpha_
        ) : red(red_), green(green_), blue(blue_), alpha(alpha_) {}

        unsigned char red;
        unsigned char green;
        unsigned char blue;
        unsigned char alpha;
    };

// ----------------------------------------------------------------------------------------

    struct hsi_pixel
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This is a simple struct that represents an HSI colored graphical pixel.
        !*/

        hsi_pixel (
        ) {}

        hsi_pixel (
            unsigned char h_,
            unsigned char s_,
            unsigned char i_
        ) : h(h_), s(s_), i(i_) {}

        unsigned char h;
        unsigned char s;
        unsigned char i;
    };
    // ----------------------------------------------------------------------------------------

    struct lab_pixel
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This is a simple struct that represents an Lab colored graphical pixel.
        !*/

        lab_pixel (
        ) {}

        lab_pixel (
                unsigned char l_,
                unsigned char a_,
                unsigned char b_
        ) : l(l_), a(a_), b(b_) {}

        unsigned char l;
        unsigned char a;
        unsigned char b;
    };

// ----------------------------------------------------------------------------------------

    template <
        typename P1,
        typename P2  
        >
    inline void assign_pixel (
        P1& dest,
        const P2& src
    );
    /*!
        requires
            - pixel_traits<P1> must be defined
            - pixel_traits<P2> must be defined
        ensures
            - if (P1 and P2 are the same type of pixel) then
                - simply copies the value of src into dest.  In other words,
                  dest will be identical to src after this function returns.
            - else if (P1 and P2 are not the same type of pixel) then
                - assigns pixel src to pixel dest and does any necessary color space
                  conversions.   
                - When converting from a grayscale color space with more than 255 values the
                  pixel intensity is saturated at pixel_traits<P1>::max() or pixel_traits<P1>::min()
                  as appropriate.
                - if (the dest pixel has an alpha channel and the src pixel doesn't) then
                    - #dest.alpha == 255 
                - else if (the src pixel has an alpha channel but the dest pixel doesn't) then
                    - #dest == the original dest value blended with the src value according
                      to the alpha channel in src.  
                      (i.e.  #dest == src*(alpha/255) + dest*(1-alpha/255))
    !*/


// ----------------------------------------------------------------------------------------

    template <>
    struct pixel_traits<rgb_pixel>
    {
        constexpr static bool rgb  = true;
        constexpr static bool rgb_alpha  = false;
        constexpr static bool grayscale = false;
        enum { num = 3};
        typedef unsigned char basic_pixel_type;
        static basic_pixel_type min() { return 0;}
        static basic_pixel_type max() { return 255;}
        constexpr static bool is_unsigned = true;
        constexpr static bool has_alpha = false;
    };

// ----------------------------------------------------------------------------------------

    template <>
    struct pixel_traits<bgr_pixel>
    {
        constexpr static bool rgb  = true;
        constexpr static bool rgb_alpha  = false;
        constexpr static bool grayscale = false;
        constexpr static long num = 3;
        typedef unsigned char basic_pixel_type;
        static basic_pixel_type min() { return 0;}
        static basic_pixel_type max() { return 255;}
        constexpr static bool is_unsigned = true;
        constexpr static bool has_alpha = false;
    };

// ----------------------------------------------------------------------------------------

    template <>
    struct pixel_traits<rgb_alpha_pixel>
    {
        constexpr static bool rgb  = false;
        constexpr static bool rgb_alpha  = true;
        constexpr static bool grayscale = false;
        constexpr static long num = 4;
        typedef unsigned char basic_pixel_type;
        static basic_pixel_type min() { return 0;}
        static basic_pixel_type max() { return 255;}
        constexpr static bool is_unsigned = true;
        constexpr static bool has_alpha = true;
    };

// ----------------------------------------------------------------------------------------

    // The following is a bunch of conversion stuff for the assign_pixel function.

    namespace assign_pixel_helpers
    {

    // -----------------------------

        template <typename T>
        typename unsigned_type<T>::type make_unsigned (
            const T& val
        ) { return static_cast<typename unsigned_type<T>::type>(val); }

        inline float make_unsigned(const float& val) { return val; }
        inline double make_unsigned(const double& val) { return val; }
        inline long double make_unsigned(const long double& val) { return val; }


        template <typename T, typename P>
        typename enable_if_c<pixel_traits<T>::is_unsigned == pixel_traits<P>::is_unsigned, bool>::type less_or_equal_to_max (
            const P& p
        ) 
        /*!
            ensures
                - returns true if p is <= max value of T
        !*/
        { 
            return p <= pixel_traits<T>::max();         
        }

        template <typename T, typename P>
        typename enable_if_c<pixel_traits<T>::is_unsigned && !pixel_traits<P>::is_unsigned, bool>::type less_or_equal_to_max (
            const P& p
        ) 
        { 
            if (p <= 0)
                return true;
            else if (make_unsigned(p) <= pixel_traits<T>::max())
                return true;
            else
                return false;
        }

        template <typename T, typename P>
        typename enable_if_c<!pixel_traits<T>::is_unsigned && pixel_traits<P>::is_unsigned, bool>::type less_or_equal_to_max (
            const P& p
        ) 
        { 
            return p <= make_unsigned(pixel_traits<T>::max());
        }

    // -----------------------------

        template <typename T, typename P>
        typename enable_if_c<pixel_traits<P>::is_unsigned, bool >::type greater_or_equal_to_min (
            const P& 
        ) { return true; }
        /*!
            ensures
                - returns true if p is >= min value of T
        !*/

        template <typename T, typename P>
        typename enable_if_c<!pixel_traits<P>::is_unsigned && pixel_traits<T>::is_unsigned, bool >::type greater_or_equal_to_min (
            const P& p
        ) 
        { 
            return p >= 0;
        }

        template <typename T, typename P>
        typename enable_if_c<!pixel_traits<P>::is_unsigned && !pixel_traits<T>::is_unsigned, bool >::type greater_or_equal_to_min (
            const P& p
        ) 
        { 
            return p >= pixel_traits<T>::min();
        }

    // -----------------------------
    // -----------------------------
    // -----------------------------

        template < typename P1, typename P2 >
        typename enable_if_c<pixel_traits<P1>::rgb && pixel_traits<P2>::rgb>::type
        assign(P1& dest, const P2& src) 
        { 
            dest.red = src.red; 
            dest.green = src.green; 
            dest.blue = src.blue; 
        }

        template < typename P1, typename P2 >
        typename enable_if_c<pixel_traits<P1>::rgb_alpha && pixel_traits<P2>::rgb_alpha>::type
        assign(P1& dest, const P2& src) 
        { 
            dest.red = src.red; 
            dest.green = src.green; 
            dest.blue = src.blue; 
            dest.alpha = src.alpha; 
        }

    // -----------------------------

        struct HSL
        {
            double h;
            double s;
            double l;
        };

        struct COLOUR
        {
            double r;
            double g;
            double b;
        };

        /*
            I found this excellent bit of code for dealing with HSL spaces at 
            http://local.wasp.uwa.edu.au/~pbourke/colour/hsl/
        */
        /*
            Calculate HSL from RGB
            Hue is in degrees
            Lightness is between 0 and 1
            Saturation is between 0 and 1
        */
        inline HSL RGB2HSL(COLOUR c1)
        {
            double themin,themax,delta;
            HSL c2;
            using namespace std;

            themin = std::min(c1.r,std::min(c1.g,c1.b));
            themax = std::max(c1.r,std::max(c1.g,c1.b));
            delta = themax - themin;
            c2.l = (themin + themax) / 2;
            c2.s = 0;
            if (c2.l > 0 && c2.l < 1)
                c2.s = delta / (c2.l < 0.5 ? (2*c2.l) : (2-2*c2.l));
            c2.h = 0;
            if (delta > 0) {
                if (themax == c1.r && themax != c1.g)
                    c2.h += (c1.g - c1.b) / delta;
                if (themax == c1.g && themax != c1.b)
                    c2.h += (2 + (c1.b - c1.r) / delta);
                if (themax == c1.b && themax != c1.r)
                    c2.h += (4 + (c1.r - c1.g) / delta);
                c2.h *= 60;
            }
            return(c2);
        }

        /*
            Calculate RGB from HSL, reverse of RGB2HSL()
            Hue is in degrees
            Lightness is between 0 and 1
            Saturation is between 0 and 1
        */
        inline COLOUR HSL2RGB(HSL c1)
        {
            COLOUR c2,sat,ctmp;
            using namespace std;

            if (c1.h < 120) {
                sat.r = (120 - c1.h) / 60.0;
                sat.g = c1.h / 60.0;
                sat.b = 0;
            } else if (c1.h < 240) {
                sat.r = 0;
                sat.g = (240 - c1.h) / 60.0;
                sat.b = (c1.h - 120) / 60.0;
            } else {
                sat.r = (c1.h - 240) / 60.0;
                sat.g = 0;
                sat.b = (360 - c1.h) / 60.0;
            }
            sat.r = std::min(sat.r,1.0);
            sat.g = std::min(sat.g,1.0);
            sat.b = std::min(sat.b,1.0);

            ctmp.r = 2 * c1.s * sat.r + (1 - c1.s);
            ctmp.g = 2 * c1.s * sat.g + (1 - c1.s);
            ctmp.b = 2 * c1.s * sat.b + (1 - c1.s);

            if (c1.l < 0.5) {
                c2.r = c1.l * ctmp.r;
                c2.g = c1.l * ctmp.g;
                c2.b = c1.l * ctmp.b;
            } else {
                c2.r = (1 - c1.l) * ctmp.r + 2 * c1.l - 1;
                c2.g = (1 - c1.l) * ctmp.g + 2 * c1.l - 1;
                c2.b = (1 - c1.l) * ctmp.b + 2 * c1.l - 1;
            }

            return(c2);
        }

        // -----------------------------

        struct Lab
        {
            double l;
            double a;
            double b;
        };
        /*
            Calculate Lab from RGB
            L is between 0 and 100
            a is between -128 and 127
            b is between -128 and 127
            RGB is between 0.0 and 1.0
        */
        inline Lab RGB2Lab(COLOUR c1)
        {
            Lab c2;
            using namespace std;

            double var_R = c1.r;
            double var_G = c1.g;
            double var_B = c1.b;

            if (var_R > 0.04045) {
                var_R = pow(((var_R + 0.055) / 1.055), 2.4);
            } else {
                var_R = var_R / 12.92;
            }

            if (var_G > 0.04045) {
                var_G = pow(((var_G + 0.055) / 1.055), 2.4);
            } else {
                var_G = var_G / 12.92;
            }

            if (var_B > 0.04045) {
                var_B = pow(((var_B + 0.055) / 1.055), 2.4);
            } else {
                var_B = var_B / 12.92;
            }

            var_R = var_R * 100;
            var_G = var_G * 100;
            var_B = var_B * 100;

//Observer. = 2°, Illuminant = D65
            double X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
            double Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
            double Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;

            double var_X = X / 95.047;
            double var_Y = Y / 100.000;
            double var_Z = Z / 108.883;

            if (var_X > 0.008856) {
                var_X = pow(var_X, (1.0 / 3));
            }
            else {
                var_X = (7.787 * var_X) + (16.0 / 116);
            }

            if (var_Y > 0.008856) {
                var_Y = pow(var_Y, (1.0 / 3));
            }
            else {
                var_Y = (7.787 * var_Y) + (16.0 / 116);
            }

            if (var_Z > 0.008856) {
                var_Z = pow(var_Z, (1.0 / 3));
            }
            else {
                var_Z = (7.787 * var_Z) + (16.0 / 116);
            }

            //clamping
            c2.l = max(0.0, (116.0 * var_Y) - 16);
            c2.a = max(-128.0, min(127.0, 500.0 * (var_X - var_Y)));
            c2.b = max(-128.0, min(127.0, 200.0 * (var_Y - var_Z)));

            return c2;
        }

        /*
            Calculate RGB from Lab, reverse of RGB2LAb()
            L is between 0 and 100
            a is between -128 and 127
            b is between -128 and 127
            RGB is between 0.0 and 1.0
        */
        inline COLOUR Lab2RGB(Lab c1) {
            COLOUR c2;
            using namespace std;

            double var_Y = (c1.l + 16) / 116.0;
            double var_X = (c1.a / 500.0) + var_Y;
            double var_Z = var_Y - (c1.b / 200);

            if (pow(var_Y, 3) > 0.008856) {
                var_Y = pow(var_Y, 3);
            } else {
                var_Y = (var_Y - 16.0 / 116) / 7.787;
            }

            if (pow(var_X, 3) > 0.008856) {
                var_X = pow(var_X, 3);
            } else {
                var_X = (var_X - 16.0 / 116) / 7.787;
            }

            if (pow(var_Z, 3) > 0.008856) {
                var_Z = pow(var_Z, 3);
            } else {
                var_Z = (var_Z - 16.0 / 116) / 7.787;
            }

            double X = var_X * 95.047;
            double Y = var_Y * 100.000;
            double Z = var_Z * 108.883;

            var_X = X / 100.0;
            var_Y = Y / 100.0;
            var_Z = Z / 100.0;

            double var_R = var_X * 3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
            double var_G = var_X * -0.9689 + var_Y * 1.8758 + var_Z * 0.0415;
            double var_B = var_X * 0.0557 + var_Y * -0.2040 + var_Z * 1.0570;

            if (var_R > 0.0031308) {
                var_R = 1.055 * pow(var_R, (1 / 2.4)) - 0.055;
            } else {
                var_R = 12.92 * var_R;
            }

            if (var_G > 0.0031308) {
                var_G = 1.055 * pow(var_G, (1 / 2.4)) - 0.055;
            } else {
                var_G = 12.92 * var_G;
            }

            if (var_B > 0.0031308) {
                var_B = 1.055 * pow(var_B, (1 / 2.4)) - 0.055;
            } else {
                var_B = 12.92 * var_B;
            }

            // clamping
            c2.r = max(0.0, min(1.0, var_R));
            c2.g = max(0.0, min(1.0, var_G));
            c2.b = max(0.0, min(1.0, var_B));

            return (c2);
        }


    // -----------------------------
        // dest is a color rgb_pixel

        template < typename P1 >
        typename enable_if_c<pixel_traits<P1>::rgb>::type
        assign(P1& dest, const unsigned char& src) 
        { 
            dest.red = src; 
            dest.green = src; 
            dest.blue = src; 
        }

        template < typename P1, typename P2 >
        typename enable_if_c<pixel_traits<P1>::rgb && pixel_traits<P2>::rgb_alpha>::type
        assign(P1& dest, const P2& src) 
        { 
            if (src.alpha == 255)
            {
                dest.red = src.red;
                dest.green = src.green;
                dest.blue = src.blue;
            }
            else
            {
                // perform this assignment using fixed point arithmetic: 
                // dest = src*(alpha/255) + dest*(1 - alpha/255);
                // dest = src*(alpha/255) + dest*1 - dest*(alpha/255);
                // dest = dest*1 + src*(alpha/255) - dest*(alpha/255);
                // dest = dest*1 + (src - dest)*(alpha/255);
                // dest += (src - dest)*(alpha/255);

                unsigned int temp_r = src.red;
                unsigned int temp_g = src.green;
                unsigned int temp_b = src.blue;

                temp_r -= dest.red;
                temp_g -= dest.green;
                temp_b -= dest.blue;

                temp_r *= src.alpha;
                temp_g *= src.alpha;
                temp_b *= src.alpha;

                temp_r >>= 8;
                temp_g >>= 8;
                temp_b >>= 8;

                dest.red += static_cast<unsigned char>(temp_r&0xFF);
                dest.green += static_cast<unsigned char>(temp_g&0xFF);
                dest.blue += static_cast<unsigned char>(temp_b&0xFF);
            }
        }

    // -----------------------------
    // dest is a color rgb_alpha_pixel

        template < typename P1 >
        typename enable_if_c<pixel_traits<P1>::rgb_alpha>::type
        assign(P1& dest, const unsigned char& src) 
        { 
            dest.red = src; 
            dest.green = src; 
            dest.blue = src; 
            dest.alpha = 255;
        }

        template < typename P1, typename P2 >
        typename enable_if_c<pixel_traits<P1>::rgb_alpha && pixel_traits<P2>::rgb>::type
        assign(P1& dest, const P2& src) 
        { 
            dest.red = src.red;
            dest.green = src.green;
            dest.blue = src.blue;
            dest.alpha = 255;
        }
    }

    // -----------------------------

    template < typename P1, typename P2 >
    inline void assign_pixel (
        P1& dest,
        const P2& src
    ) { assign_pixel_helpers::assign(dest,src); }

// ----------------------------------------------------------------------------------------

}

#endif // DLIB_PIXEl_ 

