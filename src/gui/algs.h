// Copyright (C) 2003  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.

#ifdef DLIB_ALL_SOURCE_END
#include "dlib_basic_cpp_build_tutorial.txt"
#endif

#ifndef DLIB_ALGs_
#define DLIB_ALGs_

// this file contains miscellaneous stuff                      

// Give people who forget the -std=c++11 option a reminder
#if (defined(__GNUC__) && ((__GNUC__ >= 4 && __GNUC_MINOR__ >= 8) || (__GNUC__ > 4))) || \
    (defined(__clang__) && ((__clang_major__ >= 3 && __clang_minor__ >= 4) || (__clang_major__ >= 3)))
    #if __cplusplus < 201103
        #error "Dlib requires C++11 support.  Give your compiler the -std=c++11 option to enable it."
    #endif
#endif

#if defined __NVCC__
    // Disable the "statement is unreachable" message since it will go off on code that is
    // actually reachable but just happens to not be reachable sometimes during certain
    // template instantiations.
    #pragma diag_suppress code_is_unreachable
#endif


#ifdef _MSC_VER

#if  _MSC_VER < 1900
#error "dlib versions newer than v19.1 use C++11 and therefore require Visual Studio 2015 or newer."
#endif

// Disable the following warnings for Visual Studio

// this is to disable the "'this' : used in base member initializer list"
// warning you get from some of the GUI objects since all the objects
// require that their parent class be passed into their constructor. 
// In this case though it is totally safe so it is ok to disable this warning.
#pragma warning(disable : 4355)

// This is a warning you get sometimes when Visual Studio performs a Koenig Lookup. 
// This is a bug in visual studio.  It is a totally legitimate thing to 
// expect from a compiler. 
#pragma warning(disable : 4675)

// This is a warning you get from visual studio 2005 about things in the standard C++
// library being "deprecated."  I checked the C++ standard and it doesn't say jack 
// about any of them (I checked the searchable PDF).   So this warning is total Bunk.
#pragma warning(disable : 4996)

// This is a warning you get from visual studio 2003:
//    warning C4345: behavior change: an object of POD type constructed with an initializer 
//    of the form () will be default-initialized.
// I love it when this compiler gives warnings about bugs in previous versions of itself. 
#pragma warning(disable : 4345)


// Disable warnings about conversion from size_t to unsigned long and long.
#pragma warning(disable : 4267)

// Disable warnings about conversion from double to float  
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)

// Disable "warning C4180: qualifier applied to function type has no meaning; ignored".
// This warning happens often in generic code that works with functions and isn't useful.
#pragma warning(disable : 4180)

// Disable "warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)"
#pragma warning(disable : 4290)


// DNN module uses template-based network declaration that leads to very long
// type names. Visual Studio will produce Warning C4503 in such cases. https://msdn.microsoft.com/en-us/library/074af4b6.aspx says
// that correct binaries are still produced even when this warning happens, but linker errors from visual studio, if they occurr could be confusing.
#pragma warning( disable: 4503 )


#endif

#ifdef __BORLANDC__
// Disable the following warnings for the Borland Compilers
//
// These warnings just say that the compiler is refusing to inline functions with
// loops or try blocks in them.  
//
#pragma option -w-8027
#pragma option -w-8026 
#endif

#include <string>       // for the exceptions

#ifdef WIN32

// This file contains all the magical #defines you have to setup  before you
// include the windows header files.  

#ifndef NOMINMAX
#define NOMINMAX // prevent windows from messing with std::min and std::max
#endif


// now just for good measure undefine min and max if they are defined
#ifdef min
#undef min
#endif

#ifdef max 
#undef max 
#endif


// This is something stupid you have to do to make visual studio include the right
// stuff.  I don't really know what the deal is with this.
#if _WIN32_WINNT < 0x0500
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#endif // WIN32

#include <memory> 
#include <algorithm>    // for std::swap
#include <new>          // for std::bad_alloc
#include <cstdlib>
#include <limits> // for std::numeric_limits for is_finite()
#include "dassert.h"
#include "error.h"

// ----------------------------------------------------------------------------------------



namespace dlib
{

  template <bool B, class T = void>
  struct enable_if_c {
    typedef T type;
  };

  template <class T>
  struct enable_if_c<false, T> {};

  template <bool B, class T = void>
  struct disable_if_c {
    typedef T type;
  };

  template <class T>
  struct disable_if_c<true, T> {};



// ----------------------------------------------------------------------------------------

    /*!A swap !*/
    // make swap available in the dlib namespace
    using std::swap;

// ----------------------------------------------------------------------------------------

    /*!
        Here is where I define my return codes.  It is 
        important that they all be < 0.
    !*/

    enum general_return_codes
    {
        TIMEOUT     = -1,
        WOULDBLOCK  = -2,
        OTHER_ERROR = -3,
        SHUTDOWN    = -4,
        PORTINUSE   = -5
    };

// ----------------------------------------------------------------------------------------

    inline unsigned long square_root (
        unsigned long value
    )
    /*!
        requires
            - value <= 2^32 - 1
        ensures
            - returns the square root of value.  if the square root is not an
              integer then it will be rounded up to the nearest integer.
    !*/
    {
        unsigned long x;

        // set the initial guess for what the root is depending on 
        // how big value is
        if (value < 3)
            return value;
        else if (value < 4096) // 12
            x = 45;
        else if (value < 65536) // 16
            x = 179;
        else if (value < 1048576) // 20
            x = 717;
        else if (value < 16777216) // 24
            x = 2867;
        else if (value < 268435456) // 28
            x = 11469;
        else   // 32
            x = 45875;



        // find the root
        x = (x + value/x)>>1;
        x = (x + value/x)>>1;
        x = (x + value/x)>>1;
        x = (x + value/x)>>1;



        if (x*x < value)
            return x+1;
        else
            return x;
    }

// ----------------------------------------------------------------------------------------

    template <
        typename T
        >    
    void median (
        T& one,
        T& two,
        T& three
    );
    /*!
        requires
            - T implements operator< 
            - T is swappable by a global swap()
        ensures
            - #one is the median 
            - #one, #two, and #three is some permutation of one, two, and three.  
    !*/
    
    
    template <
        typename T
        >
    void median (
        T& one,
        T& two,
        T& three
    )    
    {    
        using std::swap;
        using dlib::swap;

        if ( one < two )
        {
            // one < two
            if ( two < three )
            {
                // one < two < three : two
                swap(one,two);
                
            }
            else
            {
                // one < two >= three
                if ( one < three)
                {
                    // three
                    swap(three,one);
                }
            }
            
        }
        else
        {
            // one >= two
            if ( three < one )
            {
                // three <= one >= two
                if ( three < two )
                {
                    // two
                    swap(two,one);
                }
                else
                {
                    // three
                    swap(three,one);
                }
            }
        }        
    }

// ----------------------------------------------------------------------------------------

    namespace relational_operators
    {
        template <
            typename A,
            typename B
            >
        constexpr bool operator> (
            const A& a,
            const B& b
        ) { return b < a; }

    // ---------------------------------

        template <
            typename A,
            typename B
            >
        constexpr bool operator!= (
            const A& a,
            const B& b
        ) { return !(a == b); }

    // ---------------------------------

        template <
            typename A,
            typename B
            >
        constexpr bool operator<= (
            const A& a,
            const B& b
        ) { return !(b < a); }

    // ---------------------------------

        template <
            typename A,
            typename B
            >
        constexpr bool operator>= (
            const A& a,
            const B& b
        ) { return !(a < b); }

    }

// ----------------------------------------------------------------------------------------

    template <
        typename T
        >
    void exchange (
        T& a,
        T& b
    )
    /*!
        This function does the exact same thing that global swap does and it does it by
        just calling swap.  But a lot of compilers have problems doing a Koenig Lookup
        and the fact that this has a different name (global swap has the same name as
        the member functions called swap) makes them compile right.

        So this is a workaround but not too ugly of one.  But hopefully I get get
        rid of this in a few years.  So this function is already deprecated. 

        This also means you should NOT use this function in your own code unless
        you have to support an old buggy compiler that benefits from this hack.
    !*/
    {
        using std::swap;
        using dlib::swap;
        swap(a,b);
    }

// ----------------------------------------------------------------------------------------

    /*!A is_pointer_type

        This is a template where is_pointer_type<T>::value == true when T is a pointer 
        type and false otherwise.
    !*/

    template <
        typename T
        >
    class is_pointer_type
    {
    public:
        enum { value = false };
    private:
        is_pointer_type();
    };

    template <
        typename T
        >
    class is_pointer_type<T*>
    {
    public:
        enum { value = true };
    private:
        is_pointer_type();
    };

// ----------------------------------------------------------------------------------------

    /*!A is_const_type

        This is a template where is_const_type<T>::value == true when T is a const 
        type and false otherwise.
    !*/

    template <typename T>
    struct is_const_type
    {
        static const bool value = false;
    };
    template <typename T>
    struct is_const_type<const T>
    {
        static const bool value = true;
    };
    template <typename T>
    struct is_const_type<const T&>
    {
        static const bool value = true;
    };

// ----------------------------------------------------------------------------------------

    /*!A is_reference_type 

        This is a template where is_reference_type<T>::value == true when T is a reference 
        type and false otherwise.
    !*/

    template <typename T>
    struct is_reference_type
    {
        static const bool value = false;
    };

    template <typename T> struct is_reference_type<const T&> { static const bool value = true; };
    template <typename T> struct is_reference_type<T&> { static const bool value = true; };

// ----------------------------------------------------------------------------------------

    /*!A is_same_type 

        This is a template where is_same_type<T,U>::value == true when T and U are the
        same type and false otherwise.   
    !*/

    template <
        typename T,
        typename U
        >
    class is_same_type
    {
    public:
        enum {value = false};
    private:
        is_same_type();
    };

    template <typename T>
    class is_same_type<T,T>
    {
    public:
        enum {value = true};
    private:
        is_same_type();
    };

// ----------------------------------------------------------------------------------------

    /*!A is_float_type

        This is a template that can be used to determine if a type is one of the built
        int floating point types (i.e. float, double, or long double).
    !*/

    template < typename T > struct is_float_type  { const static bool value = false; };
    template <> struct is_float_type<float>       { const static bool value = true; };
    template <> struct is_float_type<double>      { const static bool value = true; };
    template <> struct is_float_type<long double> { const static bool value = true; };

// ----------------------------------------------------------------------------------------

    /*!A is_convertible

        This is a template that can be used to determine if one type is convertible 
        into another type.

        For example:
            is_convertible<int,float>::value == true    // because ints are convertible to floats
            is_convertible<int*,float>::value == false  // because int pointers are NOT convertible to floats
    !*/

    template <typename from, typename to>
    struct is_convertible
    {
        struct yes_type { char a; };
        struct no_type { yes_type a[2]; };
        static const from& from_helper();
        static yes_type test(to);
        static no_type test(...);
        const static bool value = sizeof(test(from_helper())) == sizeof(yes_type);
    };

// ----------------------------------------------------------------------------------------

    struct general_ {};
    struct special_ : general_ {};
    template<typename> struct int_ { typedef int type; };

// ----------------------------------------------------------------------------------------


    /*!A is_same_object 

        This is a templated function which checks if both of its arguments are actually
        references to the same object.  It returns true if they are and false otherwise.

    !*/

    // handle the case where T and U are unrelated types.
    template < typename T, typename U >
    typename disable_if_c<is_convertible<T*, U*>::value || is_convertible<U*,T*>::value, bool>::type
    is_same_object (
        const T& a, 
        const U& b
    ) 
    { 
        return ((void*)&a == (void*)&b); 
    }

    // handle the case where T and U are related types because their pointers can be
    // implicitly converted into one or the other.  E.g. a derived class and its base class. 
    // Or where both T and U are just the same type.  This way we make sure that if there is a
    // valid way to convert between these two pointer types then we will take that route rather
    // than the void* approach used otherwise.
    template < typename T, typename U >
    typename enable_if_c<is_convertible<T*, U*>::value || is_convertible<U*,T*>::value, bool>::type
    is_same_object (
        const T& a, 
        const U& b
    ) 
    { 
        return (&a == &b); 
    }

// ----------------------------------------------------------------------------------------

    /*!A is_unsigned_type 

        This is a template where is_unsigned_type<T>::value == true when T is an unsigned
        scalar type and false when T is a signed scalar type.
    !*/
    template <
        typename T
        >
    struct is_unsigned_type
    {
        static const bool value = static_cast<T>((static_cast<T>(0)-static_cast<T>(1))) > 0;
    };
    template <> struct is_unsigned_type<long double> { static const bool value = false; };
    template <> struct is_unsigned_type<double>      { static const bool value = false; };
    template <> struct is_unsigned_type<float>       { static const bool value = false; };

// ----------------------------------------------------------------------------------------

    /*!A is_signed_type 

        This is a template where is_signed_type<T>::value == true when T is a signed
        scalar type and false when T is an unsigned scalar type.
    !*/
    template <
        typename T
        >
    struct is_signed_type
    {
        static const bool value = !is_unsigned_type<T>::value;
    };

// ----------------------------------------------------------------------------------------

    template <
        typename T
        >
    class copy_functor
    {
    public:
        void operator() (
            const T& source, 
            T& destination
        ) const
        {
            destination = source;
        }
    };

// ----------------------------------------------------------------------------------------

    /*!A static_switch

        To use this template you give it some number of boolean expressions and it
        tells you which one of them is true.   If more than one of them is true then
        it causes a compile time error.

        for example:
            static_switch<1 + 1 == 2, 4 - 1 == 4>::value == 1  // because the first expression is true
            static_switch<1 + 1 == 3, 4 == 4>::value == 2      // because the second expression is true
            static_switch<1 + 1 == 3, 4 == 5>::value == 0      // 0 here because none of them are true 
            static_switch<1 + 1 == 2, 4 == 4>::value == compiler error  // because more than one expression is true 
    !*/

    template < bool v1 = 0, bool v2 = 0, bool v3 = 0, bool v4 = 0, bool v5 = 0,
               bool v6 = 0, bool v7 = 0, bool v8 = 0, bool v9 = 0, bool v10 = 0, 
               bool v11 = 0, bool v12 = 0, bool v13 = 0, bool v14 = 0, bool v15 = 0 >
    struct static_switch; 

    template <> struct static_switch<0,0,0,0,0,0,0,0,0,0,0,0,0,0,0> { const static int value = 0; };
    template <> struct static_switch<1,0,0,0,0,0,0,0,0,0,0,0,0,0,0> { const static int value = 1; };
    template <> struct static_switch<0,1,0,0,0,0,0,0,0,0,0,0,0,0,0> { const static int value = 2; };
    template <> struct static_switch<0,0,1,0,0,0,0,0,0,0,0,0,0,0,0> { const static int value = 3; };
    template <> struct static_switch<0,0,0,1,0,0,0,0,0,0,0,0,0,0,0> { const static int value = 4; };
    template <> struct static_switch<0,0,0,0,1,0,0,0,0,0,0,0,0,0,0> { const static int value = 5; };
    template <> struct static_switch<0,0,0,0,0,1,0,0,0,0,0,0,0,0,0> { const static int value = 6; };
    template <> struct static_switch<0,0,0,0,0,0,1,0,0,0,0,0,0,0,0> { const static int value = 7; };
    template <> struct static_switch<0,0,0,0,0,0,0,1,0,0,0,0,0,0,0> { const static int value = 8; };
    template <> struct static_switch<0,0,0,0,0,0,0,0,1,0,0,0,0,0,0> { const static int value = 9; };
    template <> struct static_switch<0,0,0,0,0,0,0,0,0,1,0,0,0,0,0> { const static int value = 10; };
    template <> struct static_switch<0,0,0,0,0,0,0,0,0,0,1,0,0,0,0> { const static int value = 11; };
    template <> struct static_switch<0,0,0,0,0,0,0,0,0,0,0,1,0,0,0> { const static int value = 12; };
    template <> struct static_switch<0,0,0,0,0,0,0,0,0,0,0,0,1,0,0> { const static int value = 13; };
    template <> struct static_switch<0,0,0,0,0,0,0,0,0,0,0,0,0,1,0> { const static int value = 14; };
    template <> struct static_switch<0,0,0,0,0,0,0,0,0,0,0,0,0,0,1> { const static int value = 15; };

// ----------------------------------------------------------------------------------------
    /*!A is_built_in_scalar_type
        
        This is a template that allows you to determine if the given type is a built
        in scalar type such as an int, char, float, short, etc.

        For example, is_built_in_scalar_type<char>::value == true
        For example, is_built_in_scalar_type<std::string>::value == false 
    !*/

    template <typename T> struct is_built_in_scalar_type        { const static bool value = false; };

    template <> struct is_built_in_scalar_type<float>           { const static bool value = true; };
    template <> struct is_built_in_scalar_type<double>          { const static bool value = true; };
    template <> struct is_built_in_scalar_type<long double>     { const static bool value = true; };
    template <> struct is_built_in_scalar_type<short>           { const static bool value = true; };
    template <> struct is_built_in_scalar_type<int>             { const static bool value = true; };
    template <> struct is_built_in_scalar_type<long>            { const static bool value = true; };
    template <> struct is_built_in_scalar_type<unsigned short>  { const static bool value = true; };
    template <> struct is_built_in_scalar_type<unsigned int>    { const static bool value = true; };
    template <> struct is_built_in_scalar_type<unsigned long>   { const static bool value = true; };
    template <> struct is_built_in_scalar_type<unsigned long long>        { const static bool value = true; };
    template <> struct is_built_in_scalar_type<long long>         { const static bool value = true; };
    template <> struct is_built_in_scalar_type<char>            { const static bool value = true; };
    template <> struct is_built_in_scalar_type<signed char>     { const static bool value = true; };
    template <> struct is_built_in_scalar_type<unsigned char>   { const static bool value = true; };
    // Don't define one for wchar_t when using a version of visual studio
    // older than 8.0 (visual studio 2005) since before then they improperly set
    // wchar_t to be a typedef rather than its own type as required by the C++ 
    // standard.
#if !defined(_MSC_VER) || _NATIVE_WCHAR_T_DEFINED
    template <> struct is_built_in_scalar_type<wchar_t>         { const static bool value = true; };
#endif

// ----------------------------------------------------------------------------------------

    /*!A tabs 

        This is a template to compute the absolute value a number at compile time.

        For example,
            abs<-4>::value == 4
            abs<4>::value == 4
    !*/

        template <long x, typename enabled=void>
        struct tabs { const static long value = x; };
        template <long x>
        struct tabs<x,typename enable_if_c<(x < 0)>::type> { const static long value = -x; };

// ----------------------------------------------------------------------------------------

    /*!A tmax

        This is a template to compute the max of two values at compile time

        For example,
            abs<4,7>::value == 7
    !*/

        template <long x, long y, typename enabled=void>
        struct tmax { const static long value = x; };
        template <long x, long y>
        struct tmax<x,y,typename enable_if_c<(y > x)>::type> { const static long value = y; };

// ----------------------------------------------------------------------------------------

    /*!A tmin 

        This is a template to compute the min of two values at compile time

        For example,
            abs<4,7>::value == 4
    !*/

        template <long x, long y, typename enabled=void>
        struct tmin { const static long value = x; };
        template <long x, long y>
        struct tmin<x,y,typename enable_if_c<(y < x)>::type> { const static long value = y; };

}

#endif // DLIB_ALGs_

