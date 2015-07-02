#ifndef VEC_HPP_INCLUDED
#define VEC_HPP_INCLUDED

#include <cmath>
#include <algorithm>

template<int N, typename T>
struct vec
{
    T v[N];

    vec<N, T>& operator=(T val)
    {
        for(int i=0; i<N; i++)
        {
            v[i] = val;
        }

        return *this;
    }

    vec<N, T> operator+(const vec<N, T>& other) const
    {
        vec<N, T> r;

        for(int i=0; i<N; i++)
        {
            r.v[i] = v[i] + other.v[i];
        }

        return r;
    }

    vec<N, T> operator+(T other) const
    {
        vec<N, T> r;

        for(int i=0; i<N; i++)
        {
            r.v[i] = v[i] + other;
        }

        return r;
    }

    vec<N, T> operator-(const vec<N, T>& other) const
    {
        vec<N, T> r;

        for(int i=0; i<N; i++)
        {
            r.v[i] = v[i] - other.v[i];
        }

        return r;
    }


    vec<N, T> operator-(T other) const
    {
        vec<N, T> r;

        for(int i=0; i<N; i++)
        {
            r.v[i] = v[i] - other;
        }

        return r;
    }


    vec<N, T> operator*(const vec<N, T>& other) const
    {
        vec<N, T> r;

        for(int i=0; i<N; i++)
        {
            r.v[i] = v[i] * other.v[i];
        }

        return r;
    }

    vec<N, T> operator*(float other) const
    {
        vec<N, T> r;

        for(int i=0; i<N; i++)
        {
            r.v[i] = v[i] * other;
        }

        return r;
    }

    vec<N, T> operator/(const vec<N, T>& other) const
    {
        vec<N, T> r;

        for(int i=0; i<N; i++)
        {
            r.v[i] = v[i] / other.v[i];
        }

        return r;
    }

    vec<N, T> operator/(float other) const
    {
        vec<N, T> r;

        for(int i=0; i<N; i++)
        {
            r.v[i] = v[i] / other;
        }

        return r;
    }

    float squared_length() const
    {
        float sqsum = 0;

        for(int i=0; i<N; i++)
        {
            sqsum += v[i]*v[i];
        }

        return sqsum;
    }


    float length() const
    {
        float l = squared_length();

        float val = sqrtf(l);

        return val;
    }


    vec<N, T> norm() const
    {
        float len = length();

        if(len < 0.00001f)
            return {0.f};

        return (*this) / len;
    }

    ///only makes sense for a vec3f
    ///swap to matrices once I have any idea what is life
    ///glm or myself (;_;)
    vec<N, T> rot(const vec<3, float>& pos, const vec<3, float>& rotation) const
    {
        vec<3, float> c;
        vec<3, float> s;

        for(int i=0; i<3; i++)
        {
            c.v[i] = cos(rotation.v[i]);
            s.v[i] = sin(rotation.v[i]);
        }

        vec<3, float> rel = *this - pos;

        vec<3, float> ret;

        ret.v[0] = c.v[1] * (s.v[2] * rel.v[1] + c.v[2]*rel.v[0]) - s.v[1]*rel.v[2];
        ret.v[1] = s.v[0] * (c.v[1] * rel.v[2] + s.v[1]*(s.v[2]*rel.v[1] + c.v[2]*rel.v[0])) + c.v[0]*(c.v[2]*rel.v[1] - s.v[2]*rel.v[0]);
        ret.v[2] = c.v[0] * (c.v[1] * rel.v[2] + s.v[1]*(s.v[2]*rel.v[1] + c.v[2]*rel.v[0])) - s.v[0]*(c.v[2]*rel.v[1] - s.v[2]*rel.v[0]);

        return ret;
    }

    vec<N, T> back_rot(const vec<3, float>& position, const vec<3, float>& rotation) const
    {
        vec<3, float> new_pos = this->rot(position, (vec<3, float>){-rotation.v[0], 0, 0});
        new_pos = new_pos.rot(position, (vec<3, float>){0, -rotation.v[1], 0});
        new_pos = new_pos.rot(position, (vec<3, float>){0, 0, -rotation.v[2]});

        return new_pos;
    }

    ///only valid for a 2-vec
    ///need to rejiggle the templates to work this out
    vec<2, T> rot(float rot_angle)
    {
        float len = length();

        float cur_angle = angle();

        float new_angle = cur_angle + rot_angle;

        float nx = len * cos(new_angle);
        float ny = len * sin(new_angle);

        return {nx, ny};
    }

    float angle()
    {
        return atan2(v[1], v[0]);
    }

    operator float() const
    {
        static_assert(N == 1, "Implicit float can conversion only be used on vec<1,T> types");

        return v[0];
    }
};

template<int N, typename T>
bool operator<(const vec<N, T>& v1, const vec<N, T>& v2)
{
    for(int i=0; i<N; i++)
    {
        if(v1.v[i] < v2.v[i])
            return true;
        if(v1.v[i] > v2.v[i])
            return false;
    }

    return false;
}


typedef vec<3, float> vec3f;
typedef vec<2, float> vec2f;

typedef vec<3, int> vec3i;
typedef vec<2, int> vec2i;



template<int N, typename T>
inline
vec<N, T> clamp(vec<N, T> v1, T p1, T p2)
{
    for(int i=0; i<N; i++)
    {
        v1.v[i] = v1.v[i] < p1 ? p1 : v1.v[i];
        v1.v[i] = v1.v[i] > p2 ? p2 : v1.v[i];
    }

    return v1;
}

template<typename T>
inline
T clamp(T v1, T p1, T p2)
{
    v1 = v1 < p1 ? p1 : v1;
    v1 = v1 > p2 ? p2 : v1;

    return v1;
}


inline vec3f rot(const vec3f& p1, const vec3f& pos, const vec3f& rot)
{
    return p1.rot(pos, rot);
}

inline vec3f back_rot(const vec3f& p1, const vec3f& pos, const vec3f& rot)
{
    return p1.back_rot(pos, rot);
}

inline vec3f cross(const vec3f& v1, const vec3f& v2)
{
    vec3f ret;

    ret.v[0] = v1.v[1] * v2.v[2] - v1.v[2] * v2.v[1];
    ret.v[1] = v1.v[2] * v2.v[0] - v1.v[0] * v2.v[2];
    ret.v[2] = v1.v[0] * v2.v[1] - v1.v[1] * v2.v[0];

    return ret;
}

template<int N, typename T>
inline float dot(const vec<N, T>& v1, const vec<N, T>& v2)
{
    float ret = 0;

    for(int i=0; i<N; i++)
    {
        ret += v1.v[i] * v2.v[i];
    }

    return ret;
}

template<int N, typename T>
inline vec<N, T> operator-(const vec<N, T>& v1)
{
    vec<N, T> ret;

    for(int i=0; i<N; i++)
    {
        ret.v[i] = -v1.v[i];
    }

    return ret;
}

inline vec3f operator*(float v, const vec3f& v1)
{
    return v1 * v;
}

inline vec3f operator+(float v, const vec3f& v1)
{
    return v1 + v;
}

/*inline vec3f operator-(float v, const vec3f& v1)
{
    return v1 - v;
}*/

inline vec3f operator/(float v, const vec3f& v1)
{
    return v1 / v;
}

inline float r2d(float v)
{
    return (v / (M_PI*2.f)) * 360.f;
}

template<int N, typename T>
inline vec<N, T> fabs(const vec<N, T>& v)
{
    vec<N, T> v1;

    for(int i=0; i<N; i++)
    {
        v1.v[i] = fabs(v.v[i]);
    }

    return v1;
}

template<int N, typename T>
inline vec<N, T> floor(const vec<N, T>& v)
{
    vec<N, T> v1;

    for(int i=0; i<N; i++)
    {
        v1.v[i] = floorf(v.v[i]);
    }

    return v1;
}

template<int N, typename T>
inline vec<N, T> ceil(const vec<N, T>& v)
{
    vec<N, T> v1;

    for(int i=0; i<N; i++)
    {
        v1.v[i] = ceilf(v.v[i]);
    }

    return v1;
}

template<int N, typename T>
inline vec<N, T> min(const vec<N, T>& v1, const vec<N, T>& v2)
{
    vec<N, T> ret;

    for(int i=0; i<N; i++)
    {
        ret.v[i] = std::min(v1.v[i], v2.v[i]);
    }

    return ret;
}

template<int N, typename T>
inline vec<N, T> max(const vec<N, T>& v1, const vec<N, T>& v2)
{
    vec<N, T> ret;

    for(int i=0; i<N; i++)
    {
        ret.v[i] = std::max(v1.v[i], v2.v[i]);
    }

    return ret;
}

template<typename U>
inline vec<4, float> rgba_to_vec(const U& rgba)
{
    vec<4, float> ret;

    ret.v[0] = rgba.r;
    ret.v[1] = rgba.g;
    ret.v[2] = rgba.b;
    ret.v[3] = rgba.a;

    return ret;
}

template<typename U>
inline vec<3, float> xyz_to_vec(const U& xyz)
{
    vec<3, float> ret;

    ret.v[0] = xyz.x;
    ret.v[1] = xyz.y;
    ret.v[2] = xyz.z;

    return ret;
}

template<int N, typename T>
inline vec<N, T> d2r(const vec<N, T>& v1)
{
    vec<N, T> ret;

    for(int i=0; i<N; i++)
    {
        ret = (v1.v[i] / 360.f) * M_PI * 2;
    }

    return ret;
}

template<int N, typename T>
inline vec<N, T> mix(const vec<N, T>& v1, const vec<N, T>& v2, float a)
{
    vec<N, T> ret;

    for(int i=0; i<N; i++)
    {
        ret.v[i] = v1.v[i] + (v2.v[i] - v1.v[i]) * a;
    }

    return ret;
}

/*template<int N, typename T>
inline vec<N, T> slerp(const vec<N, T>& v1, const vec<N, T>& v2, float a)
{
    vec<N, T> ret;

    ///im sure you can convert the cos of a number to the sign, rather than doing this
    float angle = acos(dot(v1.norm(), v2.norm()));

    if(angle < 0.0001f && angle >= -0.0001f)
        return mix(v1, v2, a);

    float a1 = sin((1 - a) * angle) / sin(angle);
    float a2 = sin(a * angle) / sin(angle);

    ret = a1 * v1 + a2 * v2;

    return ret;
}*/

template<int N, typename T>
inline vec<N, T> slerp(const vec<N, T>& v1, const vec<N, T>& v2, float a)
{
    vec<N, T> ret;

    ///im sure you can convert the cos of a number to the sign, rather than doing this
    float angle = acos(dot(v1, v2) / (v1.length() * v2.length()));

    if(angle < 0.0001f && angle >= -0.0001f)
        return mix(v1, v2, a);

    float a1 = sin((1 - a) * angle) / sin(angle);
    float a2 = sin(a * angle) / sin(angle);

    ret = a1 * v1 + a2 * v2;

    return ret;
}


template<int N, typename T>
struct mat
{
    T v[N][N];

    mat<N, T> from_vec(vec3f v1, vec3f v2, vec3f v3) const
    {
        mat m;

        for(int i=0; i<3; i++)
            m.v[0][i] = v1.v[i];

        for(int i=0; i<3; i++)
            m.v[1][i] = v2.v[i];

        for(int i=0; i<3; i++)
            m.v[2][i] = v3.v[i];

        return m;
    }

    void load(vec3f v1, vec3f v2, vec3f v3)
    {
        for(int i=0; i<3; i++)
            v[0][i] = v1.v[i];

        for(int i=0; i<3; i++)
            v[1][i] = v2.v[i];

        for(int i=0; i<3; i++)
            v[2][i] = v3.v[i];
    }

    float det() const
    {
        float a11, a12, a13, a21, a22, a23, a31, a32, a33;

        a11 = v[0][0];
        a12 = v[0][1];
        a13 = v[0][2];

        a21 = v[1][0];
        a22 = v[1][1];
        a23 = v[1][2];

        a31 = v[2][0];
        a32 = v[2][1];
        a33 = v[2][2];

        ///get determinant
        float d = a11*a22*a33 + a21*a32*a13 + a31*a12*a23 - a11*a32*a23 - a31*a22*a13 - a21*a12*a33;

        return d;
    }

    mat<N, T> invert() const
    {
        float d = det();

        float a11, a12, a13, a21, a22, a23, a31, a32, a33;

        a11 = v[0][0];
        a12 = v[0][1];
        a13 = v[0][2];

        a21 = v[1][0];
        a22 = v[1][1];
        a23 = v[1][2];

        a31 = v[2][0];
        a32 = v[2][1];
        a33 = v[2][2];


        vec3f ir1, ir2, ir3;

        ir1.v[0] = a22 * a33 - a23 * a32;
        ir1.v[1] = a13 * a32 - a12 * a33;
        ir1.v[2] = a12 * a23 - a13 * a22;

        ir2.v[0] = a23 * a31 - a21 * a33;
        ir2.v[1] = a11 * a33 - a13 * a31;
        ir2.v[2] = a13 * a21 - a11 * a23;

        ir3.v[0] = a21 * a32 - a22 * a31;
        ir3.v[1] = a12 * a31 - a11 * a32;
        ir3.v[2] = a11 * a22 - a12 * a21;

        ir1 = ir1 * d;
        ir2 = ir2 * d;
        ir3 = ir3 * d;

        return from_vec(ir1, ir2, ir3);
    }

    vec<3, T> get_v1() const
    {
        return {v[0][0], v[0][1], v[0][2]};
    }
    vec<3, T> get_v2() const
    {
        return {v[1][0], v[1][1], v[1][2]};
    }
    vec<3, T> get_v3() const
    {
        return {v[2][0], v[2][1], v[2][2]};
    }

    void from_dir(vec3f dir)
    {
        vec3f up = {0, 1, 0};

        vec3f xaxis = cross(up, dir).norm();
        vec3f yaxis = cross(dir, xaxis).norm();

        v[0][0] = xaxis.v[0];
        v[0][1] = yaxis.v[0];
        v[0][2] = dir.v[0];

        v[1][0] = xaxis.v[1];
        v[1][1] = yaxis.v[1];
        v[1][2] = dir.v[1];

        v[2][0] = xaxis.v[2];
        v[2][1] = yaxis.v[2];
        v[2][2] = dir.v[2];
    }

    void load_rotation_matrix(vec3f rotation)
    {
        vec3f c;
        vec3f s;

        for(int i=0; i<3; i++)
        {
            c.v[i] = cos(-rotation.v[i]);
            s.v[i] = sin(-rotation.v[i]);
        }

        ///rotation matrix
        vec3f r1 = {c.v[1]*c.v[2], -c.v[1]*s.v[2], s.v[1]};
        vec3f r2 = {c.v[0]*s.v[2] + c.v[2]*s.v[0]*s.v[1], c.v[0]*c.v[2] - s.v[0]*s.v[1]*s.v[2], -c.v[1]*s.v[0]};
        vec3f r3 = {s.v[0]*s.v[2] - c.v[0]*c.v[2]*s.v[1], c.v[2]*s.v[0] + c.v[0]*s.v[1]*s.v[2], c.v[1]*c.v[0]};

        load(r1, r2, r3);
    }

    vec<3, T> operator*(const vec<3, T>& other) const
    {
        vec<3, T> val;

        val.v[0] = v[0][0] * other.v[0] + v[0][1] * other.v[1] + v[0][2] * other.v[2];
        val.v[1] = v[1][0] * other.v[0] + v[1][1] * other.v[1] + v[1][2] * other.v[2];
        val.v[2] = v[2][0] * other.v[0] + v[2][1] * other.v[1] + v[2][2] * other.v[2];

        return val;
    }
};

/*template<typename T>
vec<3, T> operator*(const mat<3, T> m, const vec<3, T>& other)
{
    vec<3, T> val;

    val.v[0] = m.v[0][0] * other.v[0] + m.v[0][1] * other.v[1] + m.v[0][2] * other.v[2];
    val.v[1] = m.v[1][0] * other.v[0] + m.v[1][1] * other.v[1] + m.v[1][2] * other.v[2];
    val.v[2] = m.v[2][0] * other.v[0] + m.v[2][1] * other.v[1] + m.v[2][2] * other.v[2];

    return val;
}*/

typedef mat<3, float> mat3f;

#endif // VEC_HPP_INCLUDED
