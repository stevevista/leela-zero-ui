// Copyright (C) 2010  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_MATRIx_OP_H_
#define DLIB_MATRIx_OP_H_

#include "matrix_exp.h"


namespace dlib
{

// ----------------------------------------------------------------------------------------

    template <typename OP >
    class matrix_op;

    template < typename OP >
    struct matrix_traits<matrix_op<OP> >
    {
        typedef typename OP::type type;
        typedef typename OP::const_ret_type const_ret_type;
        typedef typename OP::mem_manager_type mem_manager_type;
        typedef typename OP::layout_type layout_type;
        const static long NR = OP::NR;
        const static long NC = OP::NC;
        const static long cost = OP::cost;
    };

    template <
        typename OP
        >
    class matrix_op : public matrix_exp<matrix_op<OP> >
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                The matrix_op is simply a tool for reducing the amount of boilerplate
                you need to write when creating matrix expressions.  
        !*/

    public:
        typedef typename matrix_traits<matrix_op>::type type;
        typedef typename matrix_traits<matrix_op>::const_ret_type const_ret_type;
        typedef typename matrix_traits<matrix_op>::mem_manager_type mem_manager_type;
        typedef typename matrix_traits<matrix_op>::layout_type layout_type;
        const static long NR = matrix_traits<matrix_op>::NR;
        const static long NC = matrix_traits<matrix_op>::NC;
        const static long cost = matrix_traits<matrix_op>::cost;

    private:
        // This constructor exists simply for the purpose of causing a compile time error if
        // someone tries to create an instance of this object with the wrong kind of object.
        template <typename T1>
        matrix_op (T1); 
    public:

        matrix_op (
            const OP& op_
        ) :
            op(op_)
        {}

        const_ret_type operator() (
            long r, 
            long c
        ) const { return op.apply(r,c); }

        const_ret_type operator() ( long i ) const 
        { return matrix_exp<matrix_op>::operator()(i); }

        long nr (
        ) const { return op.nr(); }

        long nc (
        ) const { return op.nc(); }


        const OP op;
    }; 

}

#endif // DLIB_MATRIx_OP_H_

