// Copyright (C) 2006  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_MATRIx_UTILITIES_
#define DLIB_MATRIx_UTILITIES_

#include "matrix.h"
#include <cmath>
#include <complex>
#include <limits>
#include <vector>
#include <algorithm>

namespace dlib
{



// ----------------------------------------------------------------------------------------

    template <typename M, typename target_type>
    struct op_cast 
    {
        op_cast( const M& m_) : m(m_){}
        const M& m;

        const static long cost = M::cost+2;
        const static long NR = M::NR;
        const static long NC = M::NC;
        typedef target_type type;
        typedef const target_type const_ret_type;
        typedef typename M::mem_manager_type mem_manager_type;
        typedef typename M::layout_type layout_type;
        const_ret_type apply ( long r, long c) const { return static_cast<target_type>(m(r,c)); }

        long nr () const { return m.nr(); }
        long nc () const { return m.nc(); }
    };

    template <
        typename target_type,
        typename EXP
        >
    const matrix_op<op_cast<EXP, target_type> > matrix_cast (
        const matrix_exp<EXP>& m
    )
    {
        typedef op_cast<EXP, target_type> op;
        return matrix_op<op>(op(m.ref()));
    }



}

#endif // DLIB_MATRIx_UTILITIES_

