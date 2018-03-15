// Copyright (C) 2003  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_POINT_TrANSFORMS_H_
#define DLIB_POINT_TrANSFORMS_H_

#include "point_transforms_abstract.h"
#include "../algs.h"
#include "vector.h"
#include "../matrix.h"
#include "../optimization/optimization.h"
#include "rectangle.h"
#include "drectangle.h"
#include <vector>

namespace dlib
{

// ----------------------------------------------------------------------------------------

    class point_rotator
    {
    public:
        point_rotator (
        )
        {
            sin_angle = 0;
            cos_angle = 1;
        }

        point_rotator (
            const double& angle
        )
        {
            sin_angle = std::sin(angle);
            cos_angle = std::cos(angle);
        }

        template <typename T>
        const dlib::vector<T,2> operator() (
            const dlib::vector<T,2>& p
        ) const
        {
            double x = cos_angle*p.x() - sin_angle*p.y();
            double y = sin_angle*p.x() + cos_angle*p.y();

            return dlib::vector<double,2>(x,y);
        }

        const matrix<double,2,2> get_m(
        ) const 
        { 
            matrix<double,2,2> temp;
            temp = cos_angle, -sin_angle,
                   sin_angle, cos_angle;
            return temp; 
        }

        inline friend void serialize (const point_rotator& item, std::ostream& out)
        {
            serialize(item.sin_angle, out);
            serialize(item.cos_angle, out);
        }

        inline friend void deserialize (point_rotator& item, std::istream& in)
        {
            deserialize(item.sin_angle, in);
            deserialize(item.cos_angle, in);
        }

    private:
        double sin_angle;
        double cos_angle;
    };

// ----------------------------------------------------------------------------------------

    class point_transform
    {
    public:

        point_transform (
        )
        {
            sin_angle = 0;
            cos_angle = 1;
            translate.x() = 0;
            translate.y() = 0;
        }

        point_transform (
            const double& angle,
            const dlib::vector<double,2>& translate_
        )
        {
            sin_angle = std::sin(angle);
            cos_angle = std::cos(angle);
            translate = translate_;
        }

        template <typename T>
        const dlib::vector<T,2> operator() (
            const dlib::vector<T,2>& p
        ) const
        {
            double x = cos_angle*p.x() - sin_angle*p.y();
            double y = sin_angle*p.x() + cos_angle*p.y();

            return dlib::vector<double,2>(x,y) + translate;
        }

        const matrix<double,2,2> get_m(
        ) const 
        { 
            matrix<double,2,2> temp;
            temp = cos_angle, -sin_angle,
                   sin_angle, cos_angle;
            return temp; 
        }

        const dlib::vector<double,2> get_b(
        ) const { return translate; }

        inline friend void serialize (const point_transform& item, std::ostream& out)
        {
            serialize(item.sin_angle, out);
            serialize(item.cos_angle, out);
            serialize(item.translate, out);
        }

        inline friend void deserialize (point_transform& item, std::istream& in)
        {
            deserialize(item.sin_angle, in);
            deserialize(item.cos_angle, in);
            deserialize(item.translate, in);
        }

    private:
        double sin_angle;
        double cos_angle;
        dlib::vector<double,2> translate;
    };

// ----------------------------------------------------------------------------------------

    class point_transform_affine
    {
    public:

        point_transform_affine (
        )
        {
            m = identity_matrix<double>(2);
            b.x() = 0;
            b.y() = 0;
        }

        point_transform_affine (
            const matrix<double,2,2>& m_,
            const dlib::vector<double,2>& b_
        ) :m(m_), b(b_)
        {
        }

        const dlib::vector<double,2> operator() (
            const dlib::vector<double,2>& p
        ) const
        {
            return m*p + b;
        }

        const matrix<double,2,2>& get_m(
        ) const { return m; }

        const dlib::vector<double,2>& get_b(
        ) const { return b; }

        inline friend void serialize (const point_transform_affine& item, std::ostream& out)
        {
            serialize(item.m, out);
            serialize(item.b, out);
        }

        inline friend void deserialize (point_transform_affine& item, std::istream& in)
        {
            deserialize(item.m, in);
            deserialize(item.b, in);
        }

    private:
        matrix<double,2,2> m;
        dlib::vector<double,2> b;
    };

// ----------------------------------------------------------------------------------------

    class rectangle_transform
    {
    public:

        rectangle_transform (
        )
        {
        }

        rectangle_transform (
            const point_transform_affine& tform_
        ) :tform(tform_)
        {
        }

        drectangle operator() (
            const drectangle& r
        ) const
        {
            dpoint tl = r.tl_corner();
            dpoint tr = r.tr_corner();
            dpoint bl = r.bl_corner();
            dpoint br = r.br_corner();
            // The new rectangle wouold ideally have this area if we could actually rotrate
            // the box.
            double new_area = length(tform(tl)-tform(tr))*length(tform(tl)-tform(bl));

            // But if we rotate the coners of the rectangle and then find the rectangle
            // that contains them we get this, which might have a much larger area than we
            // want.
            drectangle temp;
            temp += tform(tl);
            temp += tform(tr);
            temp += tform(bl);
            temp += tform(br);
            // so we adjust the area to match the target area and have the same center as
            // the above box.
            double scale = std::sqrt(new_area/temp.area());

            return centered_rect(center(temp), static_cast<long>(temp.width()*scale+0.5), static_cast<long>(temp.height()*scale+0.5));
        }

        rectangle operator() (
            const rectangle& r
        ) const
        {
            return (*this)(drectangle(r));
        }

        const point_transform_affine& get_tform(
        ) const { return tform; }

        inline friend void serialize (const rectangle_transform& item, std::ostream& out)
        {
            serialize(item.tform, out);
        }

        inline friend void deserialize (rectangle_transform& item, std::istream& in)
        {
            deserialize(item.tform, in);
        }

    private:
        point_transform_affine tform;
    };

// ----------------------------------------------------------------------------------------

    inline point_transform_affine operator* (
        const point_transform_affine& lhs,
        const point_transform_affine& rhs
    )
    {
        return point_transform_affine(lhs.get_m()*rhs.get_m(), lhs.get_m()*rhs.get_b()+lhs.get_b());
    }

// ----------------------------------------------------------------------------------------

    class point_transform_projective
    {
    public:

        point_transform_projective (
        )
        {
            m = identity_matrix<double>(3);
        }

        point_transform_projective (
            const matrix<double,3,3>& m_
        ) :m(m_)
        {
        }
        
        point_transform_projective (
            const point_transform_affine& tran
        ) 
        {
            set_subm(m, 0,0, 2,2) = tran.get_m();
            set_subm(m, 0,2, 2,1) = tran.get_b();
            m(2,0) = 0;
            m(2,1) = 0;
            m(2,2) = 1;
        }
        

        const dlib::vector<double,2> operator() (
            const dlib::vector<double,2>& p
        ) const
        {
            dlib::vector<double,3> temp(p);
            temp.z() = 1;
            temp = m*temp;
            if (temp.z() != 0)
                temp = temp/temp.z();
            return temp;
        }

        const matrix<double,3,3>& get_m(
        ) const { return m; }

        inline friend void serialize (const point_transform_projective& item, std::ostream& out)
        {
            serialize(item.m, out);
        }

        inline friend void deserialize (point_transform_projective& item, std::istream& in)
        {
            deserialize(item.m, in);
        }

    private:
        matrix<double,3,3> m;
    };

// ----------------------------------------------------------------------------------------

    inline point_transform_projective operator* (
        const point_transform_projective& lhs,
        const point_transform_projective& rhs
    )
    {
        return point_transform_projective(lhs.get_m()*rhs.get_m());
    }

// ----------------------------------------------------------------------------------------

    inline point_transform_projective inv (
        const point_transform_projective& trans
    )
    {
        return point_transform_projective(inv(trans.get_m()));
    }

// ----------------------------------------------------------------------------------------

    template <typename T>
    const dlib::vector<T,2> rotate_point (
        const dlib::vector<T,2>& center,
        const dlib::vector<T,2>& p,
        double angle
    )
    {
        point_rotator rot(angle);
        return rot(p-center)+center;
    }

// ----------------------------------------------------------------------------------------

    inline matrix<double,2,2> rotation_matrix (
         double angle
    )
    {
        const double ca = std::cos(angle);
        const double sa = std::sin(angle);

        matrix<double,2,2> m;
        m = ca, -sa,
            sa, ca;
        return m;
    }

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

    class point_transform_affine3d
    {
    public:

        point_transform_affine3d (
        )
        {
            m = identity_matrix<double>(3);
            b.x() = 0;
            b.y() = 0;
        }

        point_transform_affine3d (
            const matrix<double,3,3>& m_,
            const dlib::vector<double,3>& b_
        ) :m(m_), b(b_)
        {
        }

        const dlib::vector<double,3> operator() (
            const dlib::vector<double,3>& p
        ) const
        {
            return m*p + b;
        }

        const matrix<double,3,3>& get_m(
        ) const { return m; }

        const dlib::vector<double,3>& get_b(
        ) const { return b; }

        inline friend void serialize (const point_transform_affine3d& item, std::ostream& out)
        {
            serialize(item.m, out);
            serialize(item.b, out);
        }

        inline friend void deserialize (point_transform_affine3d& item, std::istream& in)
        {
            deserialize(item.m, in);
            deserialize(item.b, in);
        }

    private:
        matrix<double,3,3> m;
        dlib::vector<double,3> b;
    };

// ----------------------------------------------------------------------------------------

    inline point_transform_affine3d operator* (
        const point_transform_affine3d& lhs,
        const point_transform_affine& rhs
    )
    {
        matrix<double,3,3> m;
        m = 0;
        set_subm(m, get_rect(rhs.get_m())) = rhs.get_m();
        vector<double,3> b = rhs.get_b();

        return point_transform_affine3d(lhs.get_m()*m, lhs.get_m()*b+lhs.get_b());
    }

// ----------------------------------------------------------------------------------------

    inline point_transform_affine3d operator* (
        const point_transform_affine3d& lhs,
        const point_transform_affine3d& rhs
    )
    {
        return point_transform_affine3d(lhs.get_m()*rhs.get_m(), lhs.get_m()*rhs.get_b()+lhs.get_b());
    }

// ----------------------------------------------------------------------------------------

    inline point_transform_affine3d rotate_around_x (
        double angle
    )
    {
        const double ca = std::cos(angle);
        const double sa = std::sin(angle);

        matrix<double,3,3> m;
        m = 1,  0,  0,
            0, ca, -sa,
            0, sa, ca;

        vector<double,3> b;

        return point_transform_affine3d(m,b);
    }

// ----------------------------------------------------------------------------------------

    inline point_transform_affine3d rotate_around_y (
        double angle
    )
    {
        const double ca = std::cos(angle);
        const double sa = std::sin(angle);

        matrix<double,3,3> m;
        m = ca,  0, sa,
             0,  1, 0,
            -sa, 0, ca;

        vector<double,3> b;

        return point_transform_affine3d(m,b);
    }

// ----------------------------------------------------------------------------------------

    inline point_transform_affine3d rotate_around_z (
        double angle
    )
    {
        const double ca = std::cos(angle);
        const double sa = std::sin(angle);

        matrix<double,3,3> m;
        m = ca, -sa, 0,
            sa, ca,  0,
            0,   0,  1;

        vector<double,3> b;

        return point_transform_affine3d(m,b);
    }

// ----------------------------------------------------------------------------------------

    inline point_transform_affine3d translate_point (
        const vector<double,3>& delta
    )
    {
        return point_transform_affine3d(identity_matrix<double>(3),delta);
    }

    inline point_transform_affine3d translate_point (
        double x,
        double y,
        double z
    )
    {
        return translate_point(vector<double>(x,y,z));
    }

// ----------------------------------------------------------------------------------------

    class camera_transform
    {

    public:

        camera_transform  (
        )
        {
            *this = camera_transform(vector<double>(1,1,1), 
                                     vector<double>(0,0,0),
                                     vector<double>(0,0,1),
                                     90,
                                     1);
        }

        camera_transform (
            const vector<double>& camera_pos_,
            const vector<double>& camera_looking_at_,
            const vector<double>& camera_up_direction_,
            const double camera_field_of_view_, 
            const unsigned long num_pixels_
        )
        {
            // make sure requires clause is not broken
            DLIB_CASSERT(0 < camera_field_of_view_ && camera_field_of_view_ < 180,
                "\t camera_transform::camera_transform()"
                << "\n\t Invalid inputs were given to this function."
                << "\n\t camera_field_of_view_: " << camera_field_of_view_
                );

            camera_pos = camera_pos_;
            camera_looking_at = camera_looking_at_;
            camera_up_direction = camera_up_direction_;
            camera_field_of_view = camera_field_of_view_;
            num_pixels = num_pixels_;

            dlib::vector<double> X,Y,Z;
            Z = (camera_looking_at - camera_pos).normalize();
            Y = camera_up_direction - dot(camera_up_direction,Z)*Z; 
            Y = Y.normalize();
            X = Z.cross(Y);

            set_rowm(proj,0) = trans(X);
            // Minus because images have y axis going down but we want the 3d projection to appear using a normal coordinate system with y going up.
            set_rowm(proj,1) = -trans(Y); 
            set_rowm(proj,2) = trans(Z);

            width = num_pixels/2.0;
            dist_scale = width/std::tan(pi/180*camera_field_of_view/2);
        }

        vector<double> get_camera_pos()         const { return camera_pos; }
        vector<double> get_camera_looking_at()  const { return camera_looking_at; }
        vector<double> get_camera_up_direction()const { return camera_up_direction; }
        double get_camera_field_of_view()       const { return camera_field_of_view; }
        unsigned long get_num_pixels()                   const { return num_pixels; }

        inline dpoint operator() (
            const vector<double>& p,
            double& scale,
            double& distance
        ) const
        {
            vector<double> temp = p-camera_pos;
            temp = proj*temp;
            distance = temp.z();
            scale = dist_scale/(temp.z()>0 ? temp.z() : 1e-9);
            temp.x() = temp.x()*scale + width;
            temp.y() = temp.y()*scale + width;
            return temp;
        }

        dpoint operator() (
            const vector<double>& p
        ) const
        {
            double scale, distance;
            return (*this)(p,scale,distance);
        }

        inline friend void serialize (const camera_transform& item, std::ostream& out)
        {
            serialize(item.camera_pos, out);
            serialize(item.camera_looking_at, out);
            serialize(item.camera_up_direction, out);
            serialize(item.camera_field_of_view, out); 
            serialize(item.num_pixels, out);
            serialize(item.proj, out);
            serialize(item.dist_scale, out);
            serialize(item.width, out);
        }

        inline friend void deserialize (camera_transform& item, std::istream& in)
        {
            deserialize(item.camera_pos, in);
            deserialize(item.camera_looking_at, in);
            deserialize(item.camera_up_direction, in);
            deserialize(item.camera_field_of_view, in); 
            deserialize(item.num_pixels, in);
            deserialize(item.proj, in);
            deserialize(item.dist_scale, in);
            deserialize(item.width, in);
        }

    private:

        vector<double> camera_pos;
        vector<double> camera_looking_at;
        vector<double> camera_up_direction;
        double camera_field_of_view; 
        unsigned long num_pixels;
        matrix<double,3,3> proj;
        double dist_scale;
        double width;

    };

// ----------------------------------------------------------------------------------------

}

#endif // DLIB_POINT_TrANSFORMS_H_

