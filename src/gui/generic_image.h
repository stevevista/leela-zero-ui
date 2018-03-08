// Copyright (C) 2014  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_GeNERIC_IMAGE_Hh_
#define DLIB_GeNERIC_IMAGE_Hh_

#include "dassert.h"

namespace dlib
{

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    struct image_traits;
    /*!
        WHAT THIS OBJECT REPRESENTS
            This is a traits class for generic image objects.  You can use it to find out
            the pixel type contained within an image via an expression of the form:
                image_traits<image_type>::pixel_type
    !*/

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
//                   UTILITIES TO MAKE ACCESSING IMAGE PIXELS SIMPLER
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

    template <
        typename image_type
        >
    class image_view
    {
        /*!
            REQUIREMENTS ON image_type
                image_type must be an image object as defined at the top of this file.  

            WHAT THIS OBJECT REPRESENTS
                This object takes an image object and wraps it with an interface that makes
                it look like a dlib::array2d.  That is, it makes it look similar to a
                regular 2-dimensional C style array, making code which operates on the
                pixels simple to read.

                Note that an image_view instance is valid until the image given to its
                constructor is modified through an interface other than the image_view
                instance.  This is because, for example, someone might cause the underlying
                image object to reallocate its memory, thus invalidating the pointer to its
                pixel data stored in the image_view.    

                As an side, the reason why this object stores a pointer to the image
                object's data and uses that pointer instead of calling image_data() each
                time a pixel is accessed is to allow for image objects to implement
                complex, and possibly slow, image_data() functions.  For example, an image
                object might perform some kind of synchronization between a GPU and the
                host memory during a call to image_data().  Therefore, we call image_data()
                only in image_view's constructor to avoid the performance penalty of
                calling it for each pixel access.
        !*/

    public:
        typedef typename image_traits<image_type>::pixel_type pixel_type;

        image_view(
            image_type& img
        ) : 
            _data((char*)image_data(img)), 
            _width_step(width_step(img)),
            _nr(num_rows(img)),
            _nc(num_columns(img)),
            _img(&img) 
        {}

        long nr() const { return _nr; }
        /*!
            ensures
                - returns the number of rows in this image.
        !*/

        long nc() const { return _nc; }
        /*!
            ensures
                - returns the number of columns in this image.
        !*/

        unsigned long size() const { return static_cast<unsigned long>(nr()*nc()); }
        /*!
            ensures
                - returns the number of pixels in this image.
        !*/

#ifndef ENABLE_ASSERTS
        pixel_type* operator[] (long row) { return (pixel_type*)(_data+_width_step*row); }
        /*!
            requires
                - 0 <= row < nr()
            ensures
                - returns a pointer to the first pixel in the row-th row.  Therefore, the
                  pixel at row and column position r,c can be accessed via (*this)[r][c].
        !*/

        const pixel_type* operator[] (long row) const { return (const pixel_type*)(_data+_width_step*row); }
        /*!
            requires
                - 0 <= row < nr()
            ensures
                - returns a const pointer to the first pixel in the row-th row.  Therefore,
                  the pixel at row and column position r,c can be accessed via
                  (*this)[r][c].
        !*/
#else
        // If asserts are enabled then we need to return a proxy class so we can make sure
        // the column accesses don't go out of bounds.
        struct pix_row
        {
            pix_row(pixel_type* data_, long nc_) : data(data_),_nc(nc_) {}
            const pixel_type& operator[] (long col) const
            {
                DLIB_ASSERT(0 <= col && col < _nc, 
                    "\t The given column index is out of range."
                    << "\n\t col: " << col 
                    << "\n\t _nc: " << _nc);
                return data[col];
            }
            pixel_type& operator[] (long col)
            {
                DLIB_ASSERT(0 <= col && col < _nc, 
                    "\t The given column index is out of range."
                    << "\n\t col: " << col 
                    << "\n\t _nc: " << _nc);
                return data[col];
            }
        private:
            pixel_type* const data;
            const long _nc;
        };
        pix_row operator[] (long row) 
        { 
            DLIB_ASSERT(0 <= row && row < _nr, 
                "\t The given row index is out of range."
                << "\n\t row: " << row 
                << "\n\t _nr: " << _nr);
            return pix_row((pixel_type*)(_data+_width_step*row), _nc); 
        }
        const pix_row operator[] (long row) const 
        { 
            DLIB_ASSERT(0 <= row && row < _nr, 
                "\t The given row index is out of range."
                << "\n\t row: " << row 
                << "\n\t _nr: " << _nr);
            return pix_row((pixel_type*)(_data+_width_step*row), _nc); 
        }
#endif

        void set_size(long rows, long cols) 
        /*!
            requires
                - rows >= 0 && cols >= 0
            ensures
                - Tells the underlying image to resize itself to have the given number of
                  rows and columns.
                - #nr() == rows
                - #nc() == cols
        !*/
        { 
            DLIB_ASSERT((cols >= 0 && rows >= 0),
                        "\t image_view::set_size(long rows, long cols)"
                        << "\n\t The images can't have negative rows or columns."
                        << "\n\t cols: " << cols 
                        << "\n\t rows: " << rows 
            );
            set_image_size(*_img, rows, cols); *this = *_img; 
        }

        void clear() { set_size(0,0); }
        /*!
            ensures
                - sets the image to have 0 pixels in it.
        !*/

    private:

        char* _data;
        long _width_step;
        long _nr;
        long _nc;
        image_type* _img;
    };

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    class const_image_view
    {
        /*!
            REQUIREMENTS ON image_type
                image_type must be an image object as defined at the top of this file.  

            WHAT THIS OBJECT REPRESENTS
                This object is just like the image_view except that it provides a "const"
                view into an image.  That is, it has the same interface as image_view
                except that you can't modify the image through a const_image_view.
        !*/

    public:
        typedef typename image_traits<image_type>::pixel_type pixel_type;

        const_image_view(
            const image_type& img
        ) : 
            _data((char*)image_data(img)), 
            _width_step(width_step(img)),
            _nr(num_rows(img)),
            _nc(num_columns(img))
        {}

        long nr() const { return _nr; }
        long nc() const { return _nc; }
        unsigned long size() const { return static_cast<unsigned long>(nr()*nc()); }
#ifndef ENABLE_ASSERTS
        const pixel_type* operator[] (long row) const { return (const pixel_type*)(_data+_width_step*row); }
#else
        // If asserts are enabled then we need to return a proxy class so we can make sure
        // the column accesses don't go out of bounds.
        struct pix_row
        {
            pix_row(pixel_type* data_, long nc_) : data(data_),_nc(nc_) {}
            const pixel_type& operator[] (long col) const
            {
                DLIB_ASSERT(0 <= col && col < _nc, 
                    "\t The given column index is out of range."
                    << "\n\t col: " << col 
                    << "\n\t _nc: " << _nc);
                return data[col];
            }
        private:
            pixel_type* const data;
            const long _nc;
        };
        const pix_row operator[] (long row) const 
        { 
            DLIB_ASSERT(0 <= row && row < _nr, 
                "\t The given row index is out of range."
                << "\n\t row: " << row 
                << "\n\t _nr: " << _nr);
            return pix_row((pixel_type*)(_data+_width_step*row), _nc); 
        }
#endif

    private:
        const char* _data;
        long _width_step;
        long _nr;
        long _nc;
    };

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    image_view<image_type> make_image_view ( image_type& img) 
    { return image_view<image_type>(img); }
    /*!
        requires
            - image_type == an image object that implements the interface defined at the
              top of this file.
        ensures
            - constructs an image_view from an image object
    !*/

    template <typename image_type>
    const_image_view<image_type> make_image_view (const image_type& img) 
    { return const_image_view<image_type>(img); }
    /*!
        requires
            - image_type == an image object that implements the interface defined at the
              top of this file.
        ensures
            - constructs a const_image_view from an image object
    !*/

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    inline unsigned long image_size(
        const image_type& img
    ) { return num_columns(img)*num_rows(img); }
    /*!
        requires
            - image_type == an image object that implements the interface defined at the
              top of this file.
        ensures
            - returns the number of pixels in the given image.
    !*/

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    inline long num_rows(
        const image_type& img
    ) { return img.nr(); }
    /*!
        ensures
            - By default, try to use the member function .nr() to determine the number
              of rows in an image.  However, as stated at the top of this file, image
              objects should provide their own overload of num_rows() if needed.
    !*/

    template <typename image_type>
    inline long num_columns(
        const image_type& img
    ) { return img.nc(); }
    /*!
        ensures
            - By default, try to use the member function .nc() to determine the number
              of columns in an image.  However, as stated at the top of this file, image
              objects should provide their own overload of num_rows() if needed.
    !*/

// ----------------------------------------------------------------------------------------

}

#endif // DLIB_GeNERIC_IMAGE_Hh_

