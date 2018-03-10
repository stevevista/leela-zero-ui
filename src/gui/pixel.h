// Copyright (C) 2006  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_PIXEl_ 
#define DLIB_PIXEl_

#include <iostream>
#include <cmath>
#include "algs.h"
#include <limits>
#include <complex>


namespace dlib
{

// ----------------------------------------------------------------------------------------

    template <typename T>
    struct pixel_traits;

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

    template <
        typename P1,
        typename P2  
        >
    inline void assign_pixel (
        P1& dest,
        const P2& src
    );


// ----------------------------------------------------------------------------------------

    template <>
    struct pixel_traits<rgb_pixel>
    {
        constexpr static bool rgb  = true;
        constexpr static bool rgb_alpha  = false;
        enum { num = 3};
    };

// ----------------------------------------------------------------------------------------

    template <>
    struct pixel_traits<rgb_alpha_pixel>
    {
        constexpr static bool rgb  = false;
        constexpr static bool rgb_alpha  = true;
        constexpr static long num = 4;
    };

// ----------------------------------------------------------------------------------------

    // The following is a bunch of conversion stuff for the assign_pixel function.

    namespace assign_pixel_helpers
    {
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

