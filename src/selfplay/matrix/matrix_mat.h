// Copyright (C) 2006  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_MATRIx_MAT_Hh_
#define DLIB_MATRIx_MAT_Hh_

#include <vector>
#include "matrix_op.h"


namespace dlib
{

// ----------------------------------------------------------------------------------------
    
    template <
        typename EXP
        >
    const matrix_exp<EXP>& mat (
        const matrix_exp<EXP>& m
    )
    {
        return m;
    }

// ----------------------------------------------------------------------------------------

    template <typename T>
    struct op_array_to_mat : does_not_alias 
    {
        op_array_to_mat( const T& vect_) : vect(vect_){}

        const T& vect;

        const static long cost = 1;
        const static long NR = 0;
        const static long NC = 1;
        typedef typename T::type type;
        typedef const typename T::type& const_ret_type;
        typedef typename T::mem_manager_type mem_manager_type;
        typedef row_major_layout layout_type;

        const_ret_type apply (long r, long  ) const { return vect[r]; }

        long nr () const { return vect.size(); }
        long nc () const { return 1; }
    }; 

// ----------------------------------------------------------------------------------------

    namespace impl
    {
        template <typename U>
        struct not_bool { typedef U type; };
        template <>
        struct not_bool<const bool&> { typedef bool type; };
    }

    template <typename T>
    struct op_std_vect_to_mat : does_not_alias 
    {
        op_std_vect_to_mat( const T& vect_) : vect(vect_){}

        const T& vect;

        const static long cost = 1;
        const static long NR = 0;
        const static long NC = 1;
        typedef typename T::value_type type;
        // Since std::vector returns a proxy for bool types we need to make sure we don't
        // return an element by reference if it is a bool type.
        typedef typename impl::not_bool<const typename T::value_type&>::type const_ret_type;

        typedef default_memory_manager mem_manager_type;
        typedef row_major_layout layout_type;

        const_ret_type apply (long r, long ) const { return vect[r]; }

        long nr () const { return vect.size(); }
        long nc () const { return 1; }
    }; 

// ----------------------------------------------------------------------------------------

    template <
        typename value_type,
        typename alloc
        >
    const matrix_op<op_std_vect_to_mat<std::vector<value_type,alloc> > > mat (
        const std::vector<value_type,alloc>& vector
    )
    {
        typedef op_std_vect_to_mat<std::vector<value_type,alloc> > op;
        return matrix_op<op>(op(vector));
    }

// ----------------------------------------------------------------------------------------

    template <typename T>
    struct op_pointer_to_mat;   

    template <typename T>
    struct op_pointer_to_col_vect   
    {
        op_pointer_to_col_vect(
            const T* ptr_,
            const long size_
        ) : ptr(ptr_), size(size_){}

        const T* ptr;
        const long size;

        const static long cost = 1;
        const static long NR = 0;
        const static long NC = 1;
        typedef T type;
        typedef const T& const_ret_type;
        typedef default_memory_manager mem_manager_type;
        typedef row_major_layout layout_type;

        const_ret_type apply (long r, long ) const { return ptr[r]; }

        long nr () const { return size; }
        long nc () const { return 1; }

        template <typename U> bool aliases               ( const matrix_exp<U>& ) const { return false; }
        template <typename U> bool destructively_aliases ( const matrix_exp<U>& ) const { return false; }

        template <long num_rows, long num_cols, typename mem_manager, typename layout>
        bool aliases (
            const matrix_exp<matrix<T,num_rows,num_cols, mem_manager,layout> >& item
        ) const 
        { 
            if (item.size() == 0)
                return false;
            else
                return (ptr == &item(0,0)); 
        }

        inline bool aliases (
            const matrix_exp<matrix_op<op_pointer_to_mat<T> > >& item
        ) const;

        bool aliases (
            const matrix_exp<matrix_op<op_pointer_to_col_vect<T> > >& item
        ) const
        {
            return item.ref().op.ptr == ptr;
        }
    }; 

// ----------------------------------------------------------------------------------------

    template <
        typename T
        >
    const matrix_op<op_pointer_to_col_vect<T> > mat (
        const T* ptr,
        long nr
    )
    {
        DLIB_ASSERT(nr >= 0 , 
                    "\tconst matrix_exp mat(ptr, nr)"
                    << "\n\t nr must be >= 0"
                    << "\n\t nr: " << nr
        );
        typedef op_pointer_to_col_vect<T> op;
        return matrix_op<op>(op(ptr, nr));
    }

// ----------------------------------------------------------------------------------------

    template <typename T>
    struct op_pointer_to_mat  
    {
        op_pointer_to_mat(
            const T* ptr_,
            const long nr_,
            const long nc_ 
        ) : ptr(ptr_), rows(nr_), cols(nc_), stride(nc_){}

        op_pointer_to_mat(
            const T* ptr_,
            const long nr_,
            const long nc_,
            const long stride_
        ) : ptr(ptr_), rows(nr_), cols(nc_), stride(stride_){}

        const T* ptr;
        const long rows;
        const long cols;
        const long stride;

        const static long cost = 1;
        const static long NR = 0;
        const static long NC = 0;
        typedef T type;
        typedef const T& const_ret_type;
        typedef default_memory_manager mem_manager_type;
        typedef row_major_layout layout_type;

        const_ret_type apply (long r, long c) const { return ptr[r*stride + c]; }

        long nr () const { return rows; }
        long nc () const { return cols; }

        template <typename U> bool aliases               ( const matrix_exp<U>& ) const { return false; }
        template <typename U> bool destructively_aliases ( const matrix_exp<U>& ) const { return false; }

        template <long num_rows, long num_cols, typename mem_manager, typename layout>
        bool aliases (
            const matrix_exp<matrix<T,num_rows,num_cols, mem_manager,layout> >& item
        ) const 
        { 
            if (item.size() == 0)
                return false;
            else
                return (ptr == &item(0,0)); 
        }

        bool aliases (
            const matrix_exp<matrix_op<op_pointer_to_mat<T> > >& item
        ) const
        {
            return item.ref().op.ptr == ptr;
        }

        bool aliases (
            const matrix_exp<matrix_op<op_pointer_to_col_vect<T> > >& item
        ) const
        {
            return item.ref().op.ptr == ptr;
        }
    }; 

    template <typename T>
    bool op_pointer_to_col_vect<T>::
    aliases (
        const matrix_exp<matrix_op<op_pointer_to_mat<T> > >& item
    ) const
    {
        return item.ref().op.ptr == ptr;
    }

    template <typename T, long NR, long NC, typename MM, typename L>
    bool matrix<T,NR,NC,MM,L>::aliases (
        const matrix_exp<matrix_op<op_pointer_to_mat<T> > >& item
    ) const
    {
        if (size() != 0)
            return item.ref().op.ptr == &data(0,0);
        else
            return false;
    }

    template <typename T, long NR, long NC, typename MM, typename L>
    bool matrix<T,NR,NC,MM,L>::aliases (
        const matrix_exp<matrix_op<op_pointer_to_col_vect<T> > >& item
    ) const
    {
        if (size() != 0)
            return item.ref().op.ptr == &data(0,0);
        else
            return false;
    }

// ----------------------------------------------------------------------------------------

    template <
        typename T
        >
    const matrix_op<op_pointer_to_mat<T> > mat (
        const T* ptr,
        long nr,
        long nc
    )
    {
        DLIB_ASSERT(nr >= 0 && nc >= 0 , 
                    "\tconst matrix_exp mat(ptr, nr, nc)"
                    << "\n\t nr and nc must be >= 0"
                    << "\n\t nr: " << nr
                    << "\n\t nc: " << nc
        );
        typedef op_pointer_to_mat<T> op;
        return matrix_op<op>(op(ptr,nr,nc));
    }

    template <
        typename T
        >
    const matrix_op<op_pointer_to_mat<T> > mat (
        const T* ptr,
        long nr,
        long nc,
        long stride
    )
    {
        DLIB_ASSERT(nr >= 0 && nc >= 0 && stride > 0 , 
                    "\tconst matrix_exp mat(ptr, nr, nc, stride)"
                    << "\n\t nr and nc must be >= 0 while stride > 0"
                    << "\n\t nr: " << nr
                    << "\n\t nc: " << nc
                    << "\n\t stride: " << stride
        );
        typedef op_pointer_to_mat<T> op;
        return matrix_op<op>(op(ptr,nr,nc,stride));
    }

// ----------------------------------------------------------------------------------------

}


namespace Eigen
{
    template<typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
    class Matrix;
}

namespace dlib
{
    template <typename T, int _Rows, int _Cols>
    struct op_eigen_Matrix_to_mat : does_not_alias 
    {
        op_eigen_Matrix_to_mat( const T& array_) : m(array_){}

        const T& m;

        const static long cost = 1;
        const static long NR = (_Rows > 0) ? _Rows : 0;
        const static long NC = (_Cols > 0) ? _Cols : 0;
        typedef typename T::Scalar type;
        typedef typename T::Scalar const_ret_type;
        typedef default_memory_manager mem_manager_type;
        typedef row_major_layout layout_type;

        const_ret_type apply (long r, long c ) const { return m(r,c); }

        long nr () const { return m.rows(); }
        long nc () const { return m.cols(); }
    }; 

// ----------------------------------------------------------------------------------------

    template <
        typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols
        >
    const matrix_op<op_eigen_Matrix_to_mat< ::Eigen::Matrix<_Scalar,_Rows,_Cols,_Options,_MaxRows,_MaxCols>,_Rows,_Cols > > mat (
        const ::Eigen::Matrix<_Scalar,_Rows,_Cols,_Options,_MaxRows,_MaxCols>& m
    )
    {
        typedef op_eigen_Matrix_to_mat< ::Eigen::Matrix<_Scalar,_Rows,_Cols,_Options,_MaxRows,_MaxCols>,_Rows,_Cols > op;
        return matrix_op<op>(op(m));
    }


}

#endif // DLIB_MATRIx_MAT_Hh_


