// Copyright (C) 2006  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_MATRIx_
#define DLIB_MATRIx_

#include "algs.h"
#include <sstream>
#include <algorithm>
#include <utility>

#ifdef _MSC_VER
// Disable the following warnings for Visual Studio

// This warning is:
//    "warning C4355: 'this' : used in base member initializer list"
// Which we get from this code but it is not an error so I'm turning this
// warning off and then turning it back on at the end of the file.
#pragma warning(disable : 4355)

#endif

namespace dlib
{

    template <
        typename T,
        long num_rows
        >
    class matrix; 

// ----------------------------------------------------------------------------------------
    template <typename EXP>
    struct matrix_traits
    {
        typedef typename EXP::type type;
        typedef typename EXP::const_ret_type const_ret_type;
        const static long NR = EXP::NR;
    };

// ----------------------------------------------------------------------------------------

    template <
        typename EXP
        >
    class matrix_exp 
    {
        /*!
            REQUIREMENTS ON EXP
                EXP should be something convertible to a matrix_exp.  That is,
                it should inherit from matrix_exp
        !*/

    public:
        typedef typename matrix_traits<EXP>::type type;
        typedef type value_type;
        typedef typename matrix_traits<EXP>::const_ret_type const_ret_type;
        const static long NR = matrix_traits<EXP>::NR;

        typedef EXP exp_type;

        const_ret_type operator() (
            long i
        ) const 
        {
            return ref()(i);
        }

        long size (
        ) const { return NR; }

        inline const exp_type& ref (
        ) const { return *static_cast<const exp_type*>(this); }

    protected:
        matrix_exp() {}
        matrix_exp(const matrix_exp& ) {}

    private:

        matrix_exp& operator= (const matrix_exp&);
    };

// ----------------------------------------------------------------------------------------


    template <typename M>
    class matrix_mul_scal_exp;

// ----------------------------------------------------------------------------------------

    template <typename LHS, typename RHS>
    class matrix_add_exp;

    template <typename LHS, typename RHS>
    struct matrix_traits<matrix_add_exp<LHS,RHS> >
    {
        typedef typename LHS::type type;
        typedef typename LHS::type const_ret_type;
        const static long NR = (RHS::NR > LHS::NR) ? RHS::NR : LHS::NR;
    };

    template <
        typename LHS,
        typename RHS
        >
    class matrix_add_exp : public matrix_exp<matrix_add_exp<LHS,RHS> >
    {
    public:
        typedef typename matrix_traits<matrix_add_exp>::type type;
        typedef typename matrix_traits<matrix_add_exp>::const_ret_type const_ret_type;
        const static long NR = matrix_traits<matrix_add_exp>::NR;

        // This constructor exists simply for the purpose of causing a compile time error if
        // someone tries to create an instance of this object with the wrong kind of objects.
        template <typename T1, typename T2>
        matrix_add_exp (T1,T2); 

        matrix_add_exp (
            const LHS& lhs_,
            const RHS& rhs_
        ) :
            lhs(lhs_),
            rhs(rhs_)
        {
            // You can only add matrices together if they both have the same number of rows and columns.
            COMPILE_TIME_ASSERT(LHS::NR == RHS::NR);

            // You can only add matrices together if they both contain the same types of elements.
            COMPILE_TIME_ASSERT((is_same_type<typename LHS::type, typename RHS::type>::value == true));
        }

        const type operator() (
            long r
        ) const { return lhs(r) + rhs(r); }

        const LHS& lhs;
        const RHS& rhs;
    };

    template <
        typename EXP1,
        typename EXP2
        >
    inline const matrix_add_exp<EXP1, EXP2> operator+ (
        const matrix_exp<EXP1>& m1,
        const matrix_exp<EXP2>& m2
    )
    {
        return matrix_add_exp<EXP1, EXP2>(m1.ref(),m2.ref());
    }

// ----------------------------------------------------------------------------------------

    template <typename M>
    struct matrix_traits<matrix_mul_scal_exp<M> >
    {
        typedef typename M::type type;
        typedef typename M::type const_ret_type;
        const static long NR = M::NR;
    };

    template <
        typename M
        >
    class matrix_mul_scal_exp : public matrix_exp<matrix_mul_scal_exp<M> >
    {
        /*!
            REQUIREMENTS ON M 
                - must be a matrix_exp object.

        !*/
    public:
        typedef typename matrix_traits<matrix_mul_scal_exp>::type type;
        typedef typename matrix_traits<matrix_mul_scal_exp>::const_ret_type const_ret_type;
        const static long NR = matrix_traits<matrix_mul_scal_exp>::NR;

        // This constructor exists simply for the purpose of causing a compile time error if
        // someone tries to create an instance of this object with the wrong kind of objects.
        template <typename T1>
        matrix_mul_scal_exp (T1, const type&); 

        matrix_mul_scal_exp (
            const M& m_,
            const type& s_
        ) :
            m(m_),
            s(s_)
        {}

        const type operator() (
            long r
        ) const { return m(r)*s; }

        const M& m;
        const type s;
    };


    template <
        typename EXP,
        typename S 
        >
    inline const matrix_mul_scal_exp<EXP> operator* (
        const S& s,
        const matrix_exp<EXP>& m
    )
    {
        typedef typename EXP::type type;
        return matrix_mul_scal_exp<EXP>(m.ref(),static_cast<type>(s));
    }



// ----------------------------------------------------------------------------------------

    template <
        typename T,
        long num_rows
        >
    struct matrix_traits<matrix<T,num_rows> >
    {
        typedef T type;
        typedef const T& const_ret_type;
        const static long NR = num_rows;

    };

    template <
        typename T,
        long num_rows
        >
    class matrix : public matrix_exp<matrix<T,num_rows> > 
    {

        COMPILE_TIME_ASSERT(num_rows >= 0); 

    public:
        typedef T type;
        typedef const T& const_ret_type;
        const static long NR = num_rows;

        matrix () 
        {
        }

        matrix(matrix&& item)
        {
            swap(item);
        }

        matrix& operator= (
            matrix&& rhs
        )
        {
            swap(rhs);
            return *this;
        }

        T& operator() (
            long i
        ) 
        {
            // You can only use this operator on column vectors.
            DLIB_ASSERT( 0 <= i && i < size(), 
                "\tconst type matrix::operator(i)"
                << "\n\tYou must give a valid row/column number"
                << "\n\ti:      " << i
                << "\n\tsize(): " << size()
                << "\n\tthis:   " << this
                );
            return data[i];
        }

        const T& operator() (
            long i
        ) const
        {
            // You can only use this operator on column vectors.
            DLIB_ASSERT( 0 <= i && i < size(), 
                "\tconst type matrix::operator(i)"
                << "\n\tYou must give a valid row/column number"
                << "\n\ti:      " << i
                << "\n\tsize(): " << size()
                << "\n\tthis:   " << this
                );
            return data[i];
        }

        long size (
        ) const { return NR; }

        void swap (
            matrix& item
        )
        {
            for (long r = 0; r < NR; ++r)
            {
                exchange(data[r],item.data[r]);
            }
        }

    private:
        T data[NR];
    };

}

#ifdef _MSC_VER
// put that warning back to its default setting
#pragma warning(default : 4355)
#endif

#endif // DLIB_MATRIx_

