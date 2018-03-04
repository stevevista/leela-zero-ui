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
        long num_rows = 0,
        long num_cols = 0
        >
    class matrix; 

    template <typename T, typename helper = void>
    struct is_matrix
    {
        /*!
            - if (T is some kind of matrix expression from the matrix/matrix_exp_abstract.h component) then
                - is_matrix<T>::value == true
            - else
                - is_matrix<T>::value == false
        !*/

        // Don't set the helper to anything.  Just let it be void.
        ASSERT_ARE_SAME_TYPE(helper,void);
        static const bool value = false;
    };

// ----------------------------------------------------------------------------------------

    // We want to return the compile time constant if our NR and NC dimensions
    // aren't zero but if they are then we want to call ref_.nx() and return
    // the correct values. 
    template < typename exp_type, long NR >
    struct get_nr_helper
    {
        static inline long get(const exp_type&) { return NR; }
    };

    template < typename exp_type >
    struct get_nr_helper<exp_type,0>
    {
        static inline long get(const exp_type& m) { return m.nr(); }
    };

    template < typename exp_type, long NC >
    struct get_nc_helper
    {
        static inline long get(const exp_type&) { return NC; }
    };

    template < typename exp_type >
    struct get_nc_helper<exp_type,0>
    {
        static inline long get(const exp_type& m) { return m.nc(); }
    };

    template <typename EXP>
    struct matrix_traits
    {
        typedef typename EXP::type type;
        typedef typename EXP::const_ret_type const_ret_type;
        const static long NR = EXP::NR;
        const static long NC = EXP::NC;
        const static long cost = EXP::cost;
    };

// ----------------------------------------------------------------------------------------

    template <typename EXP> class matrix_exp;
    template <typename EXP>
    class matrix_exp_iterator : public std::iterator<std::forward_iterator_tag, typename matrix_traits<EXP>::type>
    {
        friend class matrix_exp<EXP>;
        matrix_exp_iterator(const EXP& m_, long r_, long c_)
        {
            r = r_;
            c = c_;
            nc = m_.nc();
            m = &m_;
        }

    public:

        matrix_exp_iterator() : r(0), c(0), nc(0), m(0) {}

        typedef typename matrix_traits<EXP>::type type;
        typedef type value_type;
        typedef typename matrix_traits<EXP>::const_ret_type const_ret_type;


        bool operator == ( const matrix_exp_iterator& itr) const
        { return r == itr.r && c == itr.c; }

        bool operator != ( const matrix_exp_iterator& itr) const
        { return !(*this == itr); }

        matrix_exp_iterator& operator++()
        {
            ++c;
            if (c==nc)
            {
                c = 0;
                ++r;
            }
            return *this;
        }

        matrix_exp_iterator operator++(int)
        {
            matrix_exp_iterator temp(*this);
            ++(*this);
            return temp;
        }

        const_ret_type operator* () const { return (*m)(r,c); }

    private:
        long r, c;
        long nc;
        const EXP* m;
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
        const static long NC = matrix_traits<EXP>::NC;
        const static long cost = matrix_traits<EXP>::cost;

        typedef matrix<type,NR,NC> matrix_type;
        typedef EXP exp_type;
        typedef matrix_exp_iterator<EXP> iterator;
        typedef matrix_exp_iterator<EXP> const_iterator;

        inline const_ret_type operator() (
            long r,
            long c
        ) const 
        { 
            DLIB_ASSERT(r < nr() && c < nc() && r >= 0 && c >= 0, 
                "\tconst type matrix_exp::operator(r,c)"
                << "\n\tYou must give a valid row and column"
                << "\n\tr:    " << r 
                << "\n\tc:    " << c
                << "\n\tnr(): " << nr()
                << "\n\tnc(): " << nc() 
                << "\n\tthis: " << this
                );
            return ref()(r,c); 
        }

        const_ret_type operator() (
            long i
        ) const 
        {
            COMPILE_TIME_ASSERT(NC == 1 || NC == 0 || NR == 1 || NR == 0);
            DLIB_ASSERT(nc() == 1 || nr() == 1, 
                "\tconst type matrix_exp::operator(i)"
                << "\n\tYou can only use this operator on column or row vectors"
                << "\n\ti:    " << i
                << "\n\tnr(): " << nr()
                << "\n\tnc(): " << nc()
                << "\n\tthis: " << this
                );
            DLIB_ASSERT( ((nc() == 1 && i < nr()) || (nr() == 1 && i < nc())) && i >= 0, 
                "\tconst type matrix_exp::operator(i)"
                << "\n\tYou must give a valid row/column number"
                << "\n\ti:    " << i
                << "\n\tnr(): " << nr()
                << "\n\tnc(): " << nc()
                << "\n\tthis: " << this
                );
            if (nc() == 1)
                return ref()(i,0);
            else
                return ref()(0,i);
        }

        long size (
        ) const { return nr()*nc(); }

        long nr (
        ) const { return get_nr_helper<exp_type,NR>::get(ref()); }

        long nc (
        ) const { return get_nc_helper<exp_type,NC>::get(ref()); }

        inline const exp_type& ref (
        ) const { return *static_cast<const exp_type*>(this); }

        inline operator const type (
        ) const 
        {
            COMPILE_TIME_ASSERT(NC == 1 || NC == 0);
            COMPILE_TIME_ASSERT(NR == 1 || NR == 0);
            DLIB_ASSERT(nr() == 1 && nc() == 1, 
                "\tmatrix_exp::operator const type() const"
                << "\n\tYou can only use this operator on a 1x1 matrix"
                << "\n\tnr(): " << nr()
                << "\n\tnc(): " << nc()
                << "\n\tthis: " << this
                );

            // Put the expression contained in this matrix_exp into
            // a temporary 1x1 matrix so that the expression will encounter
            // all the overloads of matrix_assign() and have the chance to
            // go through any applicable optimizations.
            matrix<type,1,1> temp(ref());
            return temp(0);
        }

        const_iterator begin() const { return matrix_exp_iterator<EXP>(ref(),0,0); }
        const_iterator end()   const { return matrix_exp_iterator<EXP>(ref(),nr(),0); }

    protected:
        matrix_exp() {}
        matrix_exp(const matrix_exp& ) {}

    private:

        matrix_exp& operator= (const matrix_exp&);
    };

// ----------------------------------------------------------------------------------------

    // something is a matrix if it is convertible to a matrix_exp object
    template <typename T>
    struct is_matrix<T, typename enable_if<is_convertible<T, const matrix_exp<typename T::exp_type>& > >::type > 
    { static const bool value = true; }; 
    /*
        is_matrix<T>::value == 1 if T is a matrix type else 0
    */

// ------------------------------------------------------------------------------------

        template <
            typename T,
            long num_rows,
            long num_cols
            >
        class data_layout // when the sizes are all non zero and small
        {
        public:
            const static long NR = num_rows;
            const static long NC = num_cols;

            data_layout() {}

            T& operator() (
                long r, 
                long c
            ) { return *(data+r*num_cols + c); }

            const T& operator() (
                long r, 
                long c
            ) const { return *(data+r*num_cols + c); }

            T& operator() (
                long i 
            ) { return data[i]; }

            const T& operator() (
                long i
            ) const { return data[i]; }

            void swap(
                data_layout& item
            )
            {
                for (long r = 0; r < num_rows; ++r)
                {
                    for (long c = 0; c < num_cols; ++c)
                    {
                        exchange((*this)(r,c),item(r,c));
                    }
                }
            }

            long nr (
            ) const { return num_rows; }

            long nc (
            ) const { return num_cols; }

            void set_size (
                long ,
                long 
            )
            {
            }

    private:
        T data[num_rows*num_cols];

    private:  // emphasize the following members are private
        data_layout(const data_layout&);
        const data_layout& operator=(const data_layout&);
    };

// ----------------------------------------------------------------------------------------


    template <typename M, bool use_reference = true>
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
        const static long NC = (RHS::NC > LHS::NC) ? RHS::NC : LHS::NC;
        const static long cost = LHS::cost+RHS::cost+1;
    };

    template <
        typename LHS,
        typename RHS
        >
    class matrix_add_exp : public matrix_exp<matrix_add_exp<LHS,RHS> >
    {
        /*!
            REQUIREMENTS ON LHS AND RHS
                - must be matrix_exp objects. 
        !*/
    public:
        typedef typename matrix_traits<matrix_add_exp>::type type;
        typedef typename matrix_traits<matrix_add_exp>::const_ret_type const_ret_type;
        const static long NR = matrix_traits<matrix_add_exp>::NR;
        const static long NC = matrix_traits<matrix_add_exp>::NC;
        const static long cost = matrix_traits<matrix_add_exp>::cost;

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
            COMPILE_TIME_ASSERT(LHS::NR == RHS::NR || LHS::NR == 0 || RHS::NR == 0);
            COMPILE_TIME_ASSERT(LHS::NC == RHS::NC || LHS::NC == 0 || RHS::NC == 0);
            DLIB_ASSERT(lhs.nc() == rhs.nc() &&
                   lhs.nr() == rhs.nr(), 
                "\tconst matrix_exp operator+(const matrix_exp& lhs, const matrix_exp& rhs)"
                << "\n\tYou are trying to add two incompatible matrices together"
                << "\n\tlhs.nr(): " << lhs.nr()
                << "\n\tlhs.nc(): " << lhs.nc()
                << "\n\trhs.nr(): " << rhs.nr()
                << "\n\trhs.nc(): " << rhs.nc()
                << "\n\t&lhs: " << &lhs 
                << "\n\t&rhs: " << &rhs 
                );

            // You can only add matrices together if they both contain the same types of elements.
            COMPILE_TIME_ASSERT((is_same_type<typename LHS::type, typename RHS::type>::value == true));
        }

        const type operator() (
            long r, 
            long c
        ) const { return lhs(r,c) + rhs(r,c); }

        inline const type operator() ( long i ) const 
        { return matrix_exp<matrix_add_exp>::operator()(i); }

        long nr (
        ) const { return lhs.nr(); }

        long nc (
        ) const { return lhs.nc(); }

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

    template <typename M, bool use_reference >
    struct matrix_traits<matrix_mul_scal_exp<M,use_reference> >
    {
        typedef typename M::type type;
        typedef typename M::type const_ret_type;
        const static long NR = M::NR;
        const static long NC = M::NC;
        const static long cost = M::cost+1;
    };

    template <typename T, bool is_ref> struct conditional_reference { typedef T type; };
    template <typename T> struct conditional_reference<T,true>      { typedef T& type; };


    template <
        typename M,
        bool use_reference
        >
    class matrix_mul_scal_exp : public matrix_exp<matrix_mul_scal_exp<M,use_reference> >
    {
        /*!
            REQUIREMENTS ON M 
                - must be a matrix_exp object.

        !*/
    public:
        typedef typename matrix_traits<matrix_mul_scal_exp>::type type;
        typedef typename matrix_traits<matrix_mul_scal_exp>::const_ret_type const_ret_type;
        const static long NR = matrix_traits<matrix_mul_scal_exp>::NR;
        const static long NC = matrix_traits<matrix_mul_scal_exp>::NC;
        const static long cost = matrix_traits<matrix_mul_scal_exp>::cost;

        // You aren't allowed to multiply a matrix of matrices by a scalar.   
        COMPILE_TIME_ASSERT(is_matrix<type>::value == false);

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
            long r, 
            long c
        ) const { return m(r,c)*s; }

        long nr (
        ) const { return m.nr(); }

        long nc (
        ) const { return m.nc(); }

        typedef typename conditional_reference<const M,use_reference>::type M_ref_type;

        M_ref_type m;
        const type s;
    };


    template <
        typename EXP,
        typename S 
        >
    inline typename disable_if<is_matrix<S>, const matrix_mul_scal_exp<EXP> >::type operator* (
        const S& s,
        const matrix_exp<EXP>& m
    )
    {
        typedef typename EXP::type type;
        return matrix_mul_scal_exp<EXP>(m.ref(),static_cast<type>(s));
    }

    template <
        typename EXP,
        typename S,
        bool B
        >
    inline typename disable_if<is_matrix<S>, const matrix_mul_scal_exp<EXP> >::type operator* (
        const S& s,
        const matrix_mul_scal_exp<EXP,B>& m
    )
    {
        typedef typename EXP::type type;
        return matrix_mul_scal_exp<EXP>(m.m,static_cast<type>(s)*m.s);
    }


// ----------------------------------------------------------------------------------------

    template <
        typename EXP1,
        typename EXP2
        >
    bool operator== (
        const matrix_exp<EXP1>& m1,
        const matrix_exp<EXP2>& m2
    )
    {
        if (m1.nr() == m2.nr() && m1.nc() == m2.nc())
        {
            for (long r = 0; r < m1.nr(); ++r)
            {
                for (long c = 0; c < m1.nc(); ++c)
                {
                    if (m1(r,c) != m2(r,c))
                        return false;
                }
            }
            return true;
        }
        return false;
    }

    template <
        typename EXP1,
        typename EXP2
        >
    inline bool operator!= (
        const matrix_exp<EXP1>& m1,
        const matrix_exp<EXP2>& m2
    ) { return !(m1 == m2); }

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

    template <
        typename T,
        long num_rows,
        long num_cols
        >
    struct matrix_traits<matrix<T,num_rows, num_cols> >
    {
        typedef T type;
        typedef const T& const_ret_type;
        const static long NR = num_rows;
        const static long NC = num_cols;
        const static long cost = 1;

    };

    template <
        typename T,
        long num_rows,
        long num_cols
        >
    class matrix : public matrix_exp<matrix<T,num_rows,num_cols> > 
    {

        COMPILE_TIME_ASSERT(num_rows >= 0 && num_cols >= 0); 

    public:
        typedef typename matrix_traits<matrix>::type type;
        typedef typename matrix_traits<matrix>::const_ret_type const_ret_type;
        const static long NR = matrix_traits<matrix>::NR;
        const static long NC = matrix_traits<matrix>::NC;
        const static long cost = matrix_traits<matrix>::cost;
        typedef T*          iterator;       
        typedef const T*    const_iterator; 

        matrix () 
        {
        }

        explicit matrix (
            long length 
        ) 
        {
            // This object you are trying to call matrix(length) on is not a column or 
            // row vector.
            COMPILE_TIME_ASSERT(NR == 1 || NC == 1);
            DLIB_ASSERT( length >= 0, 
                "\tmatrix::matrix(length)"
                << "\n\tlength must be at least 0"
                << "\n\tlength: " << length 
                << "\n\tNR:     " << NR 
                << "\n\tNC:     " << NC 
                << "\n\tthis:   " << this
                );

            if (NR == 1)
            {
                DLIB_ASSERT(NC == 0 || NC == length,
                    "\tmatrix::matrix(length)"
                    << "\n\tSince this is a statically sized matrix length must equal NC"
                    << "\n\tlength: " << length 
                    << "\n\tNR:     " << NR 
                    << "\n\tNC:     " << NC 
                    << "\n\tthis:   " << this
                    );

                data.set_size(1,length);
            }
            else
            {
                DLIB_ASSERT(NR == 0 || NR == length,
                    "\tvoid matrix::set_size(length)"
                    << "\n\tSince this is a statically sized matrix length must equal NR"
                    << "\n\tlength: " << length 
                    << "\n\tNR:     " << NR 
                    << "\n\tNC:     " << NC 
                    << "\n\tthis:   " << this
                    );

                data.set_size(length,1);
            }
        }

        matrix (
            long rows,
            long cols 
        )  
        {
            DLIB_ASSERT( (NR == 0 || NR == rows) && ( NC == 0 || NC == cols) && 
                    rows >= 0 && cols >= 0, 
                "\tvoid matrix::matrix(rows, cols)"
                << "\n\tYou have supplied conflicting matrix dimensions"
                << "\n\trows: " << rows
                << "\n\tcols: " << cols
                << "\n\tNR:   " << NR 
                << "\n\tNC:   " << NC 
                );
            data.set_size(rows,cols);
        }

#ifdef DLIB_HAS_RVALUE_REFERENCES
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
#endif // DLIB_HAS_RVALUE_REFERENCES

        template <typename U, size_t len>
        explicit matrix (
            U (&array)[len]
        ) 
        {
            COMPILE_TIME_ASSERT(NR*NC == len && len > 0);
            size_t idx = 0;
            for (long r = 0; r < NR; ++r)
            {
                for (long c = 0; c < NC; ++c)
                {
                    data(r,c) = static_cast<T>(array[idx]);
                    ++idx;
                }
            }
        }

        T& operator() (
            long r, 
            long c
        ) 
        { 
            DLIB_ASSERT(r < nr() && c < nc() &&
                   r >= 0 && c >= 0, 
                "\tT& matrix::operator(r,c)"
                << "\n\tYou must give a valid row and column"
                << "\n\tr:    " << r 
                << "\n\tc:    " << c
                << "\n\tnr(): " << nr()
                << "\n\tnc(): " << nc() 
                << "\n\tthis: " << this
                );
            return data(r,c); 
        }

        const T& operator() (
            long r, 
            long c
        ) const 
        { 
            DLIB_ASSERT(r < nr() && c < nc() &&
                   r >= 0 && c >= 0, 
                "\tconst T& matrix::operator(r,c)"
                << "\n\tYou must give a valid row and column"
                << "\n\tr:    " << r 
                << "\n\tc:    " << c
                << "\n\tnr(): " << nr()
                << "\n\tnc(): " << nc() 
                << "\n\tthis: " << this
                );
            return data(r,c);
        }

        T& operator() (
            long i
        ) 
        {
            // You can only use this operator on column vectors.
            COMPILE_TIME_ASSERT(NC == 1 || NC == 0 || NR == 1 || NR == 0);
            DLIB_ASSERT(nc() == 1 || nr() == 1, 
                "\tconst type matrix::operator(i)"
                << "\n\tYou can only use this operator on column or row vectors"
                << "\n\ti:    " << i
                << "\n\tnr(): " << nr()
                << "\n\tnc(): " << nc()
                << "\n\tthis: " << this
                );
            DLIB_ASSERT( 0 <= i && i < size(), 
                "\tconst type matrix::operator(i)"
                << "\n\tYou must give a valid row/column number"
                << "\n\ti:      " << i
                << "\n\tsize(): " << size()
                << "\n\tthis:   " << this
                );
            return data(i);
        }

        const T& operator() (
            long i
        ) const
        {
            // You can only use this operator on column vectors.
            COMPILE_TIME_ASSERT(NC == 1 || NC == 0 || NR == 1 || NR == 0);
            DLIB_ASSERT(nc() == 1 || nr() == 1, 
                "\tconst type matrix::operator(i)"
                << "\n\tYou can only use this operator on column or row vectors"
                << "\n\ti:    " << i
                << "\n\tnr(): " << nr()
                << "\n\tnc(): " << nc()
                << "\n\tthis: " << this
                );
            DLIB_ASSERT( 0 <= i && i < size(), 
                "\tconst type matrix::operator(i)"
                << "\n\tYou must give a valid row/column number"
                << "\n\ti:      " << i
                << "\n\tsize(): " << size()
                << "\n\tthis:   " << this
                );
            return data(i);
        }

        inline operator const type (
        ) const 
        {
            COMPILE_TIME_ASSERT(NC == 1 || NC == 0);
            COMPILE_TIME_ASSERT(NR == 1 || NR == 0);
            DLIB_ASSERT( nr() == 1 && nc() == 1 , 
                "\tmatrix::operator const type"
                << "\n\tYou can only attempt to implicit convert a matrix to a scalar if"
                << "\n\tthe matrix is a 1x1 matrix"
                << "\n\tnr(): " << nr() 
                << "\n\tnc(): " << nc() 
                << "\n\tthis: " << this
                );
            return data(0);
        }

        long nr (
        ) const { return data.nr(); }

        long nc (
        ) const { return data.nc(); }

        long size (
        ) const { return data.nr()*data.nc(); }

        matrix& operator *= (
            const T a
        )
        {
            *this = *this * a;
            return *this;
        }

        matrix& operator /= (
            const T a
        )
        {
            *this = *this / a;
            return *this;
        }

        matrix& operator= (
            const matrix& m
        )
        {
            if (this != &m)
            {
                set_size(m.nr(),m.nc());
                const long size = m.nr()*m.nc();
                for (long i = 0; i < size; ++i)
                    data(i) = m.data(i);
            }
            return *this;
        }

        void swap (
            matrix& item
        )
        {
            data.swap(item.data);
        }

        iterator begin() 
        {
            if (size() != 0)
                return &data(0,0);
            else
                return 0;
        }

        iterator end()
        {
            if (size() != 0)
                return &data(0,0)+size();
            else
                return 0;
        }

        const_iterator begin()  const
        {
            if (size() != 0)
                return &data(0,0);
            else
                return 0;
        }

        const_iterator end() const
        {
            if (size() != 0)
                return &data(0,0)+size();
            else
                return 0;
        }

    private:


        data_layout<T,NR,NC> data;
    };

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

    template <
        typename T,
        long NR,
        long NC
        >
    void swap(
        matrix<T,NR,NC>& a,
        matrix<T,NR,NC>& b
    ) { a.swap(b); }


}

#ifdef _MSC_VER
// put that warning back to its default setting
#pragma warning(default : 4355)
#endif

#endif // DLIB_MATRIx_

