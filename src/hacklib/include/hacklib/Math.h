#ifndef HACKLIB_MATH_H
#define HACKLIB_MATH_H

#define GLM_FORCE_NO_CTOR_INIT
#include "glm/glm.hpp"

#ifdef _WIN32
#include "d3dx9.h"
#endif


namespace hl {


class Vec3 : public glm::vec3
{
public:
    /*
    Crashes the msvc2015 compiler:
    https://connect.microsoft.com/VisualStudio/feedback/details/2749762
    Workaround: write all constructors...
    */
    //using glm::vec3::vec3;

#ifdef _WIN32
    Vec3(const D3DXVECTOR3& v) : glm::vec3(v.x, v.y, v.z) { }

    operator D3DXVECTOR3() const
    {
        return *reinterpret_cast<const D3DXVECTOR3*>(this);
    }
#endif

    template <typename T, glm::precision P>
    using tmat4x4 = glm::tmat4x4<T, P>;

    template <typename T, glm::precision P>
    using tvec3 = glm::tvec3<T, P>;

    typedef float T;
    static const glm::precision P = glm::highp;

    // -- Implicit basic constructors --

    GLM_FUNC_DECL Vec3() : glm::vec3() { }
    GLM_FUNC_DECL Vec3(tvec3<T, P> const & v) : glm::vec3(v) { }
    template <glm::precision Q>
    GLM_FUNC_DECL Vec3(tvec3<T, Q> const & v) : glm::vec3(v) { }

    // -- Explicit basic constructors --

    GLM_FUNC_DECL explicit Vec3(T const & scalar) : glm::vec3(scalar) { }
    GLM_FUNC_DECL Vec3(T const & a, T const & b, T const & c) : glm::vec3(a, b, c) { }

    // -- Conversion scalar constructors --

    /// Explicit converions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
    template <typename A, typename B, typename C>
    GLM_FUNC_DECL Vec3(A const & a, B const & b, C const & c) : glm::vec3(a, b, c) { }
    template <typename A, typename B, typename C>
    GLM_FUNC_DECL Vec3(glm::tvec1<A, P> const & a, glm::tvec1<B, P> const & b, glm::tvec1<C, P> const & c) : glm::vec3(a, b, c) { }

    // -- Conversion vector constructors --

    /// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
    template <typename A, typename B, glm::precision Q>
    GLM_FUNC_DECL explicit Vec3(glm::tvec2<A, Q> const & a, B const & b) : glm::vec3(a, b) { }
    /// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
    template <typename A, typename B, glm::precision Q>
    GLM_FUNC_DECL explicit Vec3(glm::tvec2<A, Q> const & a, glm::tvec1<B, Q> const & b) : glm::vec3(a, b) { }
    /// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
    template <typename A, typename B, glm::precision Q>
    GLM_FUNC_DECL explicit Vec3(A const & a, glm::tvec2<B, Q> const & b) : glm::vec3(a, b) { }
    /// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
    template <typename A, typename B, glm::precision Q>
    GLM_FUNC_DECL explicit Vec3(glm::tvec1<A, Q> const & a, glm::tvec2<B, Q> const & b) : glm::vec3(a, b) { }
    /// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
    template <typename U, glm::precision Q>
    GLM_FUNC_DECL explicit Vec3(glm::tvec4<U, Q> const & v) : glm::vec3(v) { }

    /// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
    template <typename U, glm::precision Q>
    GLM_FUNC_DECL GLM_EXPLICIT Vec3(glm::tvec3<U, Q> const & v) : glm::vec3(v) { }
};

class Mat4x4 : public glm::mat4x4
{
public:
    /*
    Crashes the msvc2015 compiler:
    https://connect.microsoft.com/VisualStudio/feedback/details/2749762
    Workaround: write all constructors...
    */
    //using glm::mat4x4::mat4x4;

#ifdef _WIN32
    Mat4x4(const D3DXMATRIX& m) : glm::mat4x4(m._11, m._12, m._13, m._14, m._21, m._22, m._23, m._24, m._31, m._32, m._33, m._34, m._41, m._42, m._43, m._44) { }

    operator D3DXMATRIX() const
    {
        return *reinterpret_cast<const D3DXMATRIX*>(this);
    }
#endif

    template <typename T, glm::precision P>
    using tmat4x4 = glm::tmat4x4<T, P>;

    template <typename T, glm::precision P>
    using tvec4 = glm::tvec4<T, P>;

    typedef float T;
    static const glm::precision P = glm::highp;

    // -- Constructors --

    GLM_FUNC_DECL Mat4x4() : glm::mat4x4() { }
    GLM_FUNC_DECL Mat4x4(tmat4x4<T, P> const & m) : glm::mat4x4(m) { }
    template <glm::precision Q>
    GLM_FUNC_DECL Mat4x4(tmat4x4<T, Q> const & m) : glm::mat4x4(m) { }

    //GLM_FUNC_DECL explicit Mat4x4(ctor);
    GLM_FUNC_DECL explicit Mat4x4(T const & x) : glm::mat4x4(x) { }
    GLM_FUNC_DECL Mat4x4(
        T const & x0, T const & y0, T const & z0, T const & w0,
        T const & x1, T const & y1, T const & z1, T const & w1,
        T const & x2, T const & y2, T const & z2, T const & w2,
        T const & x3, T const & y3, T const & z3, T const & w3) : glm::mat4x4(x0, y0, z0, w0, x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3) { }
    GLM_FUNC_DECL Mat4x4(
        col_type const & v0,
        col_type const & v1,
        col_type const & v2,
        col_type const & v3) : glm::mat4x4(v0, v1, v2, v3) { }

    // -- Conversions --

    template <
        typename X1, typename Y1, typename Z1, typename W1,
        typename X2, typename Y2, typename Z2, typename W2,
        typename X3, typename Y3, typename Z3, typename W3,
        typename X4, typename Y4, typename Z4, typename W4>
        GLM_FUNC_DECL Mat4x4(
            X1 const & x1, Y1 const & y1, Z1 const & z1, W1 const & w1,
            X2 const & x2, Y2 const & y2, Z2 const & z2, W2 const & w2,
            X3 const & x3, Y3 const & y3, Z3 const & z3, W3 const & w3,
            X4 const & x4, Y4 const & y4, Z4 const & z4, W4 const & w4) : glm::mat4x4(x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4) { }

    template <typename V1, typename V2, typename V3, typename V4>
    GLM_FUNC_DECL Mat4x4(
        tvec4<V1, P> const & v1,
        tvec4<V2, P> const & v2,
        tvec4<V3, P> const & v3,
        tvec4<V4, P> const & v4) : glm::mat4x4(v1, v2, v3, v4) { }

    // -- Matrix conversions --

    template <typename U, glm::precision Q>
    GLM_FUNC_DECL GLM_EXPLICIT Mat4x4(tmat4x4<U, Q> const & m) : glm::mat4x4(m) { }

    GLM_FUNC_DECL explicit Mat4x4(glm::tmat2x2<T, P> const & x) : glm::mat4x4(x) { }
    GLM_FUNC_DECL explicit Mat4x4(glm::tmat3x3<T, P> const & x) : glm::mat4x4(x) { }
    GLM_FUNC_DECL explicit Mat4x4(glm::tmat2x3<T, P> const & x) : glm::mat4x4(x) { }
    GLM_FUNC_DECL explicit Mat4x4(glm::tmat3x2<T, P> const & x) : glm::mat4x4(x) { }
    GLM_FUNC_DECL explicit Mat4x4(glm::tmat2x4<T, P> const & x) : glm::mat4x4(x) { }
    GLM_FUNC_DECL explicit Mat4x4(glm::tmat4x2<T, P> const & x) : glm::mat4x4(x) { }
    GLM_FUNC_DECL explicit Mat4x4(glm::tmat3x4<T, P> const & x) : glm::mat4x4(x) { }
    GLM_FUNC_DECL explicit Mat4x4(glm::tmat4x3<T, P> const & x) : glm::mat4x4(x) { }
};

}

#endif