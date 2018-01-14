/*
Copyright (C) 2016 Felipe Ferreira da Silva

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim
     that you wrote the original software. If you use this software in a
     product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef MATHC_H
#define MATHC_H

#include <math.h>
#include <float.h>
#ifdef MATHC_NO_STDBOOL
	#define bool int
	#define true 1
	#define false 0
#else
	#include <stdbool.h>
#endif
#ifndef MATHC_NO_STDINT
	#include <stdint.h>
#endif

#define MATHC_MAJOR_VERSION 2
#define MATHC_MINOR_VERSION 0
#define MATHC_PATCH_VERSION 0

/* Component type */
#ifndef mfloat_t
	#define mfloat_t float
#endif
#ifndef mint_t
	#ifndef MATHC_NO_STDINT
		#define mint_t int32_t
	#else
		#define mint_t int
	#endif
#endif

/* Array sizes for declarations */
#define VEC2_SIZE 2
#define VEC3_SIZE 3
#define VEC4_SIZE 4
#define QUAT_SIZE 4
#define MAT2_SIZE 4
#define MAT3_SIZE 9
#define MAT4_SIZE 16

/* Float-point precision used internally */
#ifdef MATHC_DOUBLE_PRECISION
	#define MPI 3.14159265358979323846
	#define MPI_2 1.57079632679489661923
	#define MPI_4 0.78539816339744830962
	#define MFLT_EPSILON DBL_EPSILON
	#define MABS fabs
	#define MMIN fmin
	#define MMAX fmax
	#define MSQRT sqrt
	#define MSIN sin
	#define MCOS cos
	#define MACOS acos
	#define MTAN tan
	#define MATAN2 atan2
	#define MPOW pow
	#define MFLOOR floor
	#define MCEIL ceil
	#define MROUND round
	#define MFLOAT_C(c) c
#else
	#define MPI 3.1415926536f
	#define MPI_2 1.5707963268f
	#define MPI_4 0.7853981634f
	#define MFLT_EPSILON FLT_EPSILON
	#define MABS fabsf
	#define MMIN fminf
	#define MMAX fmaxf
	#define MSQRT sqrtf
	#define MSIN sinf
	#define MCOS cosf
	#define MACOS acosf
	#define MTAN tanf
	#define MATAN2 atan2f
	#define MPOW powf
	#define MFLOOR floorf
	#define MCEIL ceilf
	#define MROUND roundf
	#define MFLOAT_C(c) c ## f
#endif

#define MVECI_ROUND MROUND
#ifdef MVECI_ROUND_FLOOR_FUNC
	#define MVECI_ROUND MFLOOR
#endif
#ifdef MVECI_ROUND_CEIL_FUNC
	#define MVECI_ROUND MCEIL
#endif

/* Enable or disable structures */
#ifdef MATHC_NO_STRUCTURES
	#define MATHC_NO_POINTER_STRUCT_FUNCTIONS
	#define MATHC_NO_STRUCT_FUNCTIONS
#else
struct vec2 {
	mfloat_t x;
	mfloat_t y;
};

struct vec3 {
	mfloat_t x;
	mfloat_t y;
	mfloat_t z;
};

struct vec4 {
	mint_t x;
	mint_t y;
	mint_t z;
	mint_t w;
};

struct vec2i {
	mint_t x;
	mint_t y;
};

struct vec3i {
	mint_t x;
	mint_t y;
	mint_t z;
};

struct vec4i {
	mint_t x;
	mint_t y;
	mint_t z;
	mint_t w;
};

struct quat {
	mfloat_t x;
	mfloat_t y;
	mfloat_t z;
	mfloat_t w;
};

/*
Matrix 2x2 representation:
0/m11 2/m12
1/m21 3/m22
*/
struct mat2 {
	mfloat_t m11;
	mfloat_t m21;
	mfloat_t m12;
	mfloat_t m22;
};

/*
Matrix 3x3 representation:
0/m11 3/m12 6/m13
1/m21 4/m22 7/m23
2/m31 5/m32 8/m33
*/
struct mat3 {
	mfloat_t m11;
	mfloat_t m21;
	mfloat_t m31;
	mfloat_t m12;
	mfloat_t m22;
	mfloat_t m32;
	mfloat_t m13;
	mfloat_t m23;
	mfloat_t m33;
};

/*
Matrix 4x4 representation:
0/m11 4/m12  8/m13 12/m14
1/m21 5/m22  9/m23 13/m24
2/m31 6/m32 10/m33 14/m34
3/m41 7/m42 11/m43 15/m44
*/
struct mat4 {
	mfloat_t m11;
	mfloat_t m21;
	mfloat_t m31;
	mfloat_t m41;
	mfloat_t m12;
	mfloat_t m22;
	mfloat_t m32;
	mfloat_t m42;
	mfloat_t m13;
	mfloat_t m23;
	mfloat_t m33;
	mfloat_t m43;
	mfloat_t m14;
	mfloat_t m24;
	mfloat_t m34;
	mfloat_t m44;
};
#endif

#if !defined(MATHC_NO_POINTER_STRUCT_FUNCTIONS) || !defined(MATHC_NO_STRUCT_FUNCTIONS)
	#ifdef _MSC_VER
		#define MATHC_INLINE __forceinline
	#else
		#define MATHC_INLINE inline __attribute__((always_inline))
	#endif
#endif

/* Utils */
bool nearly_equal(mfloat_t a, mfloat_t b, mfloat_t epsilon);
mfloat_t to_radians(mfloat_t degrees);
mfloat_t to_degrees(mfloat_t radians);

/* Vector 2D */
bool vec2_is_zero(mfloat_t *a);
bool vec2_is_near_zero(mfloat_t *a, mfloat_t epsilon);
bool vec2_is_equal(mfloat_t *a, mfloat_t *b);
bool vec2_is_nearly_equal(mfloat_t *a, mfloat_t *b, mfloat_t epsilon);
mfloat_t *vec2(mfloat_t *result, mfloat_t x, mfloat_t y);
mfloat_t *vec2_assign(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_assign_vec2i(mfloat_t *result, mint_t *a);
mfloat_t *vec2_zero(mfloat_t *result);
mfloat_t *vec2_one(mfloat_t *result);
mfloat_t *vec2_add(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_subtract(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_scale(mfloat_t *result, mfloat_t *a, mfloat_t scalar);
mfloat_t *vec2_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_multiply_mat2(mfloat_t *result, mfloat_t *a, mfloat_t *m);
mfloat_t *vec2_divide(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_snap(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_negative(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_inverse(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_abs(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_floor(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_ceil(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_round(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_max(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_min(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_clamp(mfloat_t *result, mfloat_t *a, mfloat_t *lower, mfloat_t *higher);
mfloat_t *vec2_normalize(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_project(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_slide(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_reflect(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec2_tangent(mfloat_t *result, mfloat_t *a);
mfloat_t *vec2_rotate(mfloat_t *result, mfloat_t *a, mfloat_t angle);
mfloat_t *vec2_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p);
mfloat_t *vec2_bezier3(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t *c, mfloat_t p);
mfloat_t *vec2_bezier4(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t *c, mfloat_t *d, mfloat_t p);
mfloat_t vec2_dot(mfloat_t *a, mfloat_t *b);
mfloat_t vec2_angle(mfloat_t *a);
mfloat_t vec2_length(mfloat_t *a);
mfloat_t vec2_length_squared(mfloat_t *a);
mfloat_t vec2_distance(mfloat_t *a, mfloat_t *b);
mfloat_t vec2_distance_squared(mfloat_t *a, mfloat_t *b);

/* Vector 2D Integer */
bool vec2i_is_zero(mint_t *a);
bool vec2i_is_equal(mint_t *a, mint_t *b);
mint_t *vec2i(mint_t *result, mint_t x, mint_t y);
mint_t *vec2i_assign(mint_t *result, mint_t *a);
mint_t *vec2i_assign_vec2(mint_t *result, mfloat_t *a);
mint_t *vec2i_zero(mint_t *result);
mint_t *vec2i_one(mint_t *result);
mint_t *vec2i_add(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_subtract(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_scale(mint_t *result, mint_t *a, mfloat_t scalar);
mint_t *vec2i_multiply(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_multiply_mat2(mint_t *result, mint_t *a, mfloat_t *m);
mint_t *vec2i_divide(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_snap(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_negative(mint_t *result, mint_t *a);
mint_t *vec2i_inverse(mint_t *result, mint_t *a);
mint_t *vec2i_abs(mint_t *result, mint_t *a);
mint_t *vec2i_floor(mint_t *result, mfloat_t *a);
mint_t *vec2i_ceil(mint_t *result, mfloat_t *a);
mint_t *vec2i_round(mint_t *result, mfloat_t *a);
mint_t *vec2i_max(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_min(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_clamp(mint_t *result, mint_t *a, mint_t *lower, mint_t *higher);
mint_t *vec2i_normalize(mint_t *result, mint_t *a);
mint_t *vec2i_project(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_slide(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_reflect(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec2i_tangent(mint_t *result, mint_t *a);
mint_t *vec2i_rotate(mint_t *result, mint_t *a, mfloat_t angle);
mint_t *vec2i_lerp(mint_t *result, mint_t *a, mint_t *b, mfloat_t p);
mint_t *vec2i_bezier3(mint_t *result, mint_t *a, mint_t *b, mint_t *c, mfloat_t p);
mint_t *vec2i_bezier4(mint_t *result, mint_t *a, mint_t *b, mint_t *c, mint_t *d, mfloat_t p);
mfloat_t vec2i_dot(mint_t *a, mint_t *b);
mfloat_t vec2i_angle(mint_t *a);
mfloat_t vec2i_length(mint_t *a);
mfloat_t vec2i_length_squared(mint_t *a);
mfloat_t vec2i_distance(mint_t *a, mint_t *b);
mfloat_t vec2i_distance_squared(mint_t *a, mint_t *b);

/* Vector 3D */
bool vec3_is_zero(mfloat_t *a);
bool vec3_is_near_zero(mfloat_t *a, mfloat_t epsilon);
bool vec3_is_equal(mfloat_t *a, mfloat_t *b);
bool vec3_is_nearly_equal(mfloat_t *a, mfloat_t *b, mfloat_t epsilon);
mfloat_t *vec3(mfloat_t *result, mfloat_t x, mfloat_t y, mfloat_t z);
mfloat_t *vec3_assign(mfloat_t *result, mfloat_t *a);
mfloat_t *vec3_assign_vec3i(mfloat_t *result, mint_t *a);
mfloat_t *vec3_zero(mfloat_t *result);
mfloat_t *vec3_one(mfloat_t *result);
mfloat_t *vec3_add(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_subtract(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_scale(mfloat_t *result, mfloat_t *a, mfloat_t scalar);
mfloat_t *vec3_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_multiply_mat3(mfloat_t *result, mfloat_t *a, mfloat_t *m);
mfloat_t *vec3_divide(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_snap(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_negative(mfloat_t *result, mfloat_t *a);
mfloat_t *vec3_inverse(mfloat_t *result, mfloat_t *a);
mfloat_t *vec3_abs(mfloat_t *result, mfloat_t *a);
mfloat_t *vec3_floor(mfloat_t *result, mfloat_t *a);
mfloat_t *vec3_ceil(mfloat_t *result, mfloat_t *a);
mfloat_t *vec3_round(mfloat_t *result, mfloat_t *a);
mfloat_t *vec3_max(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_min(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_clamp(mfloat_t *result, mfloat_t *a, mfloat_t *lower, mfloat_t *higher);
mfloat_t *vec3_cross(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_normalize(mfloat_t *result, mfloat_t *a);
mfloat_t *vec3_project(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_slide(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_reflect(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec3_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p);
mfloat_t *vec3_bezier3(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t *c, mfloat_t p);
mfloat_t *vec3_bezier4(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t *c, mfloat_t *d, mfloat_t p);
mfloat_t vec3_dot(mfloat_t *a, mfloat_t *b);
mfloat_t vec3_length(mfloat_t *a);
mfloat_t vec3_length_squared(mfloat_t *a);
mfloat_t vec3_distance(mfloat_t *a, mfloat_t *b);
mfloat_t vec3_distance_squared(mfloat_t *a, mfloat_t *b);

/* Vector 3D Integer */
bool vec3i_is_zero(mint_t *a);
bool vec3i_is_equal(mint_t *a, mint_t *b);
mint_t *vec3i(mint_t *result, mint_t x, mint_t y, mint_t z);
mint_t *vec3i_assign(mint_t *result, mint_t *a);
mint_t *vec3i_assign_vec3(mint_t *result, mfloat_t *a);
mint_t *vec3i_zero(mint_t *result);
mint_t *vec3i_one(mint_t *result);
mint_t *vec3i_add(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_subtract(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_scale(mint_t *result, mint_t *a, mfloat_t scalar);
mint_t *vec3i_multiply(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_multiply_mat3(mint_t *result, mint_t *a, mfloat_t *m);
mint_t *vec3i_divide(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_snap(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_negative(mint_t *result, mint_t *a);
mint_t *vec3i_inverse(mint_t *result, mint_t *a);
mint_t *vec3i_abs(mint_t *result, mint_t *a);
mint_t *vec3i_floor(mint_t *result, mfloat_t *a);
mint_t *vec3i_ceil(mint_t *result, mfloat_t *a);
mint_t *vec3i_round(mint_t *result, mfloat_t *a);
mint_t *vec3i_max(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_min(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_clamp(mint_t *result, mint_t *a, mint_t *lower, mint_t *higher);
mint_t *vec3i_cross(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_normalize(mint_t *result, mint_t *a);
mint_t *vec3i_project(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_slide(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_reflect(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec3i_lerp(mint_t *result, mint_t *a, mint_t *b, mfloat_t p);
mint_t *vec3i_bezier3(mint_t *result, mint_t *a, mint_t *b, mint_t *c, mfloat_t p);
mint_t *vec3i_bezier4(mint_t *result, mint_t *a, mint_t *b, mint_t *c, mint_t *d, mfloat_t p);
mfloat_t vec3i_dot(mint_t *a, mint_t *b);
mfloat_t vec3i_length(mint_t *a);
mfloat_t vec3i_length_squared(mint_t *a);
mfloat_t vec3i_distance(mint_t *a, mint_t *b);
mfloat_t vec3i_distance_squared(mint_t *a, mint_t *b);

/* Vector 4D */
bool vec4_is_zero(mfloat_t *a);
bool vec4_is_near_zero(mfloat_t *a, mfloat_t epsilon);
bool vec4_is_equal(mfloat_t *a, mfloat_t *b);
bool vec4_is_nearly_equal(mfloat_t *a, mfloat_t *b, mfloat_t epsilon);
mfloat_t *vec4(mfloat_t *result, mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w);
mfloat_t *vec4_assign(mfloat_t *result, mfloat_t *a);
mfloat_t *vec4_assign_vec4i(mfloat_t *result, mint_t *a);
mfloat_t *vec4_zero(mfloat_t *result);
mfloat_t *vec4_one(mfloat_t *result);
mfloat_t *vec4_add(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec4_subtract(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec4_scale(mfloat_t *result, mfloat_t *a, mfloat_t scalar);
mfloat_t *vec4_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec4_multiply_mat4(mfloat_t *result, mfloat_t *a, mfloat_t *m);
mfloat_t *vec4_divide(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec4_snap(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec4_negative(mfloat_t *result, mfloat_t *a);
mfloat_t *vec4_inverse(mfloat_t *result, mfloat_t *a);
mfloat_t *vec4_abs(mfloat_t *result, mfloat_t *a);
mfloat_t *vec4_floor(mfloat_t *result, mfloat_t *a);
mfloat_t *vec4_ceil(mfloat_t *result, mfloat_t *a);
mfloat_t *vec4_round(mfloat_t *result, mfloat_t *a);
mfloat_t *vec4_max(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec4_min(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *vec4_clamp(mfloat_t *result, mfloat_t *a, mfloat_t *lower, mfloat_t *higher);
mfloat_t *vec4_normalize(mfloat_t *result, mfloat_t *a);
mfloat_t *vec4_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p);

/* Vector 4D Integer */
bool vec4i_is_zero(mint_t *a);
bool vec4i_is_equal(mint_t *a, mint_t *b);
mint_t *vec4i(mint_t *result, mint_t x, mint_t y, mint_t z, mint_t w);
mint_t *vec4i_assign(mint_t *result, mint_t *a);
mint_t *vec4i_assign_vec4(mint_t *result, mfloat_t *a);
mint_t *vec4i_zero(mint_t *result);
mint_t *vec4i_one(mint_t *result);
mint_t *vec4i_add(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec4i_subtract(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec4i_scale(mint_t *result, mint_t *a, mfloat_t scalar);
mint_t *vec4i_multiply(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec4i_multiply_mat4(mint_t *result, mint_t *a, mfloat_t *m);
mint_t *vec4i_divide(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec4i_snap(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec4i_negative(mint_t *result, mint_t *a);
mint_t *vec4i_inverse(mint_t *result, mint_t *a);
mint_t *vec4i_abs(mint_t *result, mint_t *a);
mint_t *vec4i_floor(mint_t *result, mfloat_t *a);
mint_t *vec4i_ceil(mint_t *result, mfloat_t *a);
mint_t *vec4i_round(mint_t *result, mfloat_t *a);
mint_t *vec4i_max(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec4i_min(mint_t *result, mint_t *a, mint_t *b);
mint_t *vec4i_clamp(mint_t *result, mint_t *a, mint_t *lower, mint_t *higher);
mint_t *vec4i_normalize(mint_t *result, mint_t *a);
mint_t *vec4i_lerp(mint_t *result, mint_t *a, mint_t *b, mfloat_t p);

/* Quaternion */
bool quat_is_zero(mfloat_t *a);
bool quat_is_near_zero(mfloat_t *a, mfloat_t epsilon);
bool quat_is_equal(mfloat_t *a, mfloat_t *b);
bool quat_is_nearly_equal(mfloat_t *a, mfloat_t *b, mfloat_t epsilon);
mfloat_t *quat(mfloat_t *result, mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w);
mfloat_t *quat_assign(mfloat_t *result, mfloat_t *a);
mfloat_t *quat_zero(mfloat_t *result);
mfloat_t *quat_null(mfloat_t *result);
mfloat_t *quat_scale(mfloat_t *result, mfloat_t *a, mfloat_t scalar);
mfloat_t *quat_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *quat_divide(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *quat_negative(mfloat_t *result, mfloat_t *a);
mfloat_t *quat_conjugate(mfloat_t *result, mfloat_t *a);
mfloat_t *quat_inverse(mfloat_t *result, mfloat_t *a);
mfloat_t *quat_normalize(mfloat_t *result, mfloat_t *a);
mfloat_t *quat_power(mfloat_t *result, mfloat_t *a, mfloat_t exponent);
mfloat_t *quat_from_axis_angle(mfloat_t *result, mfloat_t *a, mfloat_t angle);
mfloat_t *quat_from_vec3(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *quat_from_mat4(mfloat_t *result, mfloat_t *m);
mfloat_t *quat_from_yaw_pitch_roll(mfloat_t *result, mfloat_t yaw, mfloat_t pitch, mfloat_t roll);
mfloat_t *quat_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p);
mfloat_t *quat_slerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p);
mfloat_t quat_dot(mfloat_t *a, mfloat_t *b);
mfloat_t quat_angle(mfloat_t *a, mfloat_t *b);
mfloat_t quat_length(mfloat_t *a);
mfloat_t quat_length_squared(mfloat_t *a);

/* Matrix 2x2 */
mfloat_t *mat2(mfloat_t *result,
	mfloat_t m11, mfloat_t m12,
	mfloat_t m21, mfloat_t m22);
mfloat_t *mat2_zero(mfloat_t *result);
mfloat_t *mat2_identity(mfloat_t *result);
mfloat_t mat2_determinant(mfloat_t *m);
mfloat_t *mat2_assign(mfloat_t *result, mfloat_t *m);
mfloat_t *mat2_assign_mat3(mfloat_t *result, mfloat_t *m);
mfloat_t *mat2_assign_mat4(mfloat_t *result, mfloat_t *m);
mfloat_t *mat2_transpose(mfloat_t *result, mfloat_t *m);
mfloat_t *mat2_cofactor(mfloat_t *result, mfloat_t *m);
mfloat_t *mat2_inverse(mfloat_t *result, mfloat_t *m);
mfloat_t *mat2_rotation(mfloat_t *result, mfloat_t angle);
mfloat_t *mat2_scaling(mfloat_t *result, mfloat_t *v);
mfloat_t *mat2_negative(mfloat_t *result, mfloat_t *m);
mfloat_t *mat2_scale(mfloat_t *result, mfloat_t *m, mfloat_t scalar);
mfloat_t *mat2_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *mat2_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p);

/* Matrix 3x3 */
mfloat_t *mat3(mfloat_t *result,
	mfloat_t m11, mfloat_t m12, mfloat_t m13,
	mfloat_t m21, mfloat_t m22, mfloat_t m23,
	mfloat_t m31, mfloat_t m32, mfloat_t m33);
mfloat_t *mat3_zero(mfloat_t *result);
mfloat_t *mat3_identity(mfloat_t *result);
mfloat_t mat3_determinant(mfloat_t *m);
mfloat_t *mat3_assign(mfloat_t *result, mfloat_t *m);
mfloat_t *mat3_assign_mat2(mfloat_t *result, mfloat_t *m);
mfloat_t *mat3_assign_mat4(mfloat_t *result, mfloat_t *m);
mfloat_t *mat3_transpose(mfloat_t *result, mfloat_t *m);
mfloat_t *mat3_cofactor(mfloat_t *result, mfloat_t *m);
mfloat_t *mat3_adjugate(mfloat_t *result, mfloat_t *m);
mfloat_t *mat3_inverse(mfloat_t *result, mfloat_t *m);
mfloat_t *mat3_rotation_x(mfloat_t *result, mfloat_t angle);
mfloat_t *mat3_rotation_y(mfloat_t *result, mfloat_t angle);
mfloat_t *mat3_rotation_z(mfloat_t *result, mfloat_t angle);
mfloat_t *mat3_rotation_axis(mfloat_t *result, mfloat_t *a, mfloat_t angle);
mfloat_t *mat3_rotation_quaternion(mfloat_t *result, mfloat_t *q);
mfloat_t *mat3_scaling(mfloat_t *result, mfloat_t *v);
mfloat_t *mat3_negative(mfloat_t *result, mfloat_t *m);
mfloat_t *mat3_scale(mfloat_t *result, mfloat_t *m, mfloat_t scalar);
mfloat_t *mat3_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *mat3_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p);

/* Matrix 4x4 */
mfloat_t *mat4(mfloat_t *result,
	mfloat_t m11, mfloat_t m12, mfloat_t m13, mfloat_t m14,
	mfloat_t m21, mfloat_t m22, mfloat_t m23, mfloat_t m24,
	mfloat_t m31, mfloat_t m32, mfloat_t m33, mfloat_t m34,
	mfloat_t m41, mfloat_t m42, mfloat_t m43, mfloat_t m44);
mfloat_t *mat4_zero(mfloat_t *result);
mfloat_t *mat4_identity(mfloat_t *result);
mfloat_t mat4_determinant(mfloat_t *m);
mfloat_t *mat4_assign(mfloat_t *result, mfloat_t *m);
mfloat_t *mat4_assign_mat2(mfloat_t *result, mfloat_t *m);
mfloat_t *mat4_assign_mat3(mfloat_t *result, mfloat_t *m);
mfloat_t *mat4_transpose(mfloat_t *result, mfloat_t *m);
mfloat_t *mat4_adjugate(mfloat_t *result, mfloat_t *m);
mfloat_t *mat4_inverse(mfloat_t *result, mfloat_t *m);
mfloat_t *mat4_ortho(mfloat_t *result, mfloat_t l, mfloat_t r, mfloat_t b, mfloat_t t, mfloat_t n, mfloat_t f);
mfloat_t *mat4_perspective(mfloat_t *result, mfloat_t fov_y, mfloat_t aspect, mfloat_t n, mfloat_t f);
mfloat_t *mat4_perspective_fov(mfloat_t *result, mfloat_t fov, mfloat_t w, mfloat_t h, mfloat_t n, mfloat_t f);
mfloat_t *mat4_perspective_infinite(mfloat_t *result, mfloat_t fov_y, mfloat_t aspect, mfloat_t n);
mfloat_t *mat4_rotation_x(mfloat_t *result, mfloat_t angle);
mfloat_t *mat4_rotation_y(mfloat_t *result, mfloat_t angle);
mfloat_t *mat4_rotation_z(mfloat_t *result, mfloat_t angle);
mfloat_t *mat4_rotation_axis(mfloat_t *result, mfloat_t *a, mfloat_t angle);
mfloat_t *mat4_rotation_quaternion(mfloat_t *result, mfloat_t *q);
mfloat_t *mat4_look_at(mfloat_t *result, mfloat_t *position, mfloat_t *target, mfloat_t *up_axis);
mfloat_t *mat4_translation(mfloat_t *result, mfloat_t *v);
mfloat_t *mat4_scaling(mfloat_t *result, mfloat_t *v);
mfloat_t *mat4_negative(mfloat_t *result, mfloat_t *m);
mfloat_t *mat4_scale(mfloat_t *result, mfloat_t *m, mfloat_t s);
mfloat_t *mat4_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b);
mfloat_t *mat4_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p);

#ifndef MATHC_NO_POINTER_STRUCT_FUNCTIONS
/* Vector 2D */
MATHC_INLINE bool psvec2_is_zero(struct vec2 *a)
{
	return vec2_is_zero((mfloat_t *)a);
}

MATHC_INLINE bool psvec2_is_near_zero(struct vec2 *a, mfloat_t epsilon)
{
	return vec2_is_near_zero((mfloat_t *)a, epsilon);
}

MATHC_INLINE bool psvec2_is_equal(struct vec2 *a, struct vec2 *b)
{
	return vec2_is_equal((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE bool psvec2_is_nearly_equal(struct vec2 *a, struct vec2 *b, mfloat_t epsilon)
{
	return vec2_is_nearly_equal((mfloat_t *)a, (mfloat_t *)b, epsilon);
}

MATHC_INLINE struct vec2 *psvec2(struct vec2 *result, mfloat_t x, mfloat_t y)
{
	vec2((mfloat_t *)result, x, y);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_assign(struct vec2 *result, struct vec2 *a)
{
	vec2_assign((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_assign_vec2i(struct vec2 *result, struct vec2i *a)
{
	vec2_assign_vec2i((mfloat_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_zero(struct vec2 *result)
{
	vec2_zero((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_one(struct vec2 *result)
{
	vec2_one((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_add(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_add((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_subtract(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_subtract((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_scale(struct vec2 *result, struct vec2 *a, mfloat_t scalar)
{
	vec2_scale((mfloat_t *)result, (mfloat_t *)a, scalar);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_multiply(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_multiply((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_multiply_mat2(struct vec2 *result, struct vec2 *a, struct mat2 *m)
{
	vec2_multiply_mat2((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_divide(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_divide((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_snap(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_snap((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_negative(struct vec2 *result, struct vec2 *a)
{
	vec2_negative((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_inverse(struct vec2 *result, struct vec2 *a)
{
	vec2_inverse((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_abs(struct vec2 *result, struct vec2 *a)
{
	vec2_abs((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_floor(struct vec2 *result, struct vec2 *a)
{
	vec2_floor((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_ceil(struct vec2 *result, struct vec2 *a)
{
	vec2_ceil((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_round(struct vec2 *result, struct vec2 *a)
{
	vec2_round((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_max(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_max((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_min(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_min((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_clamp(struct vec2 *result, struct vec2 *a, struct vec2 *lower, struct vec2 *higher)
{
	vec2_clamp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)lower, (mfloat_t *)higher);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_normalize(struct vec2 *result, struct vec2 *a)
{
	vec2_normalize((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_project(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_project((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_slide(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_slide((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_reflect(struct vec2 *result, struct vec2 *a, struct vec2 *b)
{
	vec2_reflect((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_tangent(struct vec2 *result, struct vec2 *a)
{
	vec2_tangent((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_rotate(struct vec2 *result, struct vec2 *a, mfloat_t angle)
{
	vec2_rotate((mfloat_t *)result, (mfloat_t *)a, angle);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_lerp(struct vec2 *result, struct vec2 *a, struct vec2 *b, mfloat_t p)
{
	vec2_lerp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, p);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_bezier3(struct vec2 *result, struct vec2 *a, struct vec2 *b, struct vec2 *c, mfloat_t p)
{
	vec2_bezier3((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, (mfloat_t *)c, p);
	return result;
}

MATHC_INLINE struct vec2 *psvec2_bezier4(struct vec2 *result, struct vec2 *a, struct vec2 *b, struct vec2 *c, struct vec2 *d, mfloat_t p)
{
	vec2_bezier4((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, (mfloat_t *)c, (mfloat_t *)d, p);
	return result;
}

MATHC_INLINE mfloat_t psvec2_dot(struct vec2 *a, struct vec2 *b)
{
	return vec2_dot((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE mfloat_t psvec2_angle(struct vec2 *a)
{
	return vec2_angle((mfloat_t *)a);
}

MATHC_INLINE mfloat_t psvec2_length_squared(struct vec2 *a)
{
	return vec2_length_squared((mfloat_t *)a);
}

MATHC_INLINE mfloat_t psvec2_length(struct vec2 *a)
{
	return vec2_length((mfloat_t *)a);
}

MATHC_INLINE mfloat_t psvec2_distance(struct vec2 *a, struct vec2 *b)
{
	return vec2_distance((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE mfloat_t psvec2_distance_squared(struct vec2 *a, struct vec2 *b)
{
	return vec2_distance_squared((mfloat_t *)a, (mfloat_t *)b);
}

/* Vector 2D Integer */
MATHC_INLINE bool psvec2i_is_zero(struct vec2i *a)
{
	return vec2i_is_zero((mint_t *)a);
}

MATHC_INLINE bool psvec2i_is_equal(struct vec2i *a, struct vec2i *b)
{
	return vec2i_is_equal((mint_t *)a, (mint_t *)b);
}

MATHC_INLINE struct vec2i *psvec2i(struct vec2i *result, mfloat_t x, mfloat_t y)
{
	vec2i((mint_t *)result, x, y);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_assign(struct vec2i *result, struct vec2i *a)
{
	vec2i_assign((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_assign_vec2(struct vec2i *result, struct vec2 *a)
{
	vec2i_assign_vec2((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_zero(struct vec2i *result)
{
	vec2i_zero((mint_t *)result);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_one(struct vec2i *result)
{
	vec2i_one((mint_t *)result);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_add(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_add((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_subtract(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_subtract((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_scale(struct vec2i *result, struct vec2i *a, mfloat_t scalar)
{
	vec2i_scale((mint_t *)result, (mint_t *)a, scalar);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_multiply(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_multiply((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_multiply_mat2(struct vec2i *result, struct vec2i *a, struct mat2 *m)
{
	vec2i_multiply_mat2((mint_t *)result, (mint_t *)a, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_divide(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_divide((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_snap(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_snap((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_negative(struct vec2i *result, struct vec2i *a)
{
	vec2i_negative((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_inverse(struct vec2i *result, struct vec2i *a)
{
	vec2i_inverse((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_abs(struct vec2i *result, struct vec2i *a)
{
	vec2i_abs((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_floor(struct vec2i *result, struct vec2 *a)
{
	vec2i_floor((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_ceil(struct vec2i *result, struct vec2 *a)
{
	vec2i_ceil((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_round(struct vec2i *result, struct vec2 *a)
{
	vec2i_round((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_max(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_max((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_min(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_min((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_clamp(struct vec2i *result, struct vec2i *a, struct vec2i *lower, struct vec2i *higher)
{
	vec2i_clamp((mint_t *)result, (mint_t *)a, (mint_t *)lower, (mint_t *)higher);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_normalize(struct vec2i *result, struct vec2i *a)
{
	vec2i_normalize((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_project(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_project((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_slide(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_slide((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_reflect(struct vec2i *result, struct vec2i *a, struct vec2i *b)
{
	vec2i_reflect((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_tangent(struct vec2i *result, struct vec2i *a)
{
	vec2i_tangent((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_rotate(struct vec2i *result, struct vec2i *a, mfloat_t angle)
{
	vec2i_rotate((mint_t *)result, (mint_t *)a, angle);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_lerp(struct vec2i *result, struct vec2i *a, struct vec2i *b, mfloat_t p)
{
	vec2i_lerp((mint_t *)result, (mint_t *)a, (mint_t *)b, p);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_bezier3(struct vec2i *result, struct vec2i *a, struct vec2i *b, struct vec2i *c, mfloat_t p)
{
	vec2i_bezier3((mint_t *)result, (mint_t *)a, (mint_t *)b, (mint_t *)c, p);
	return result;
}

MATHC_INLINE struct vec2i *psvec2i_bezier4(struct vec2i *result, struct vec2i *a, struct vec2i *b, struct vec2i *c, struct vec2i *d, mfloat_t p)
{
	vec2i_bezier4((mint_t *)result, (mint_t *)a, (mint_t *)b, (mint_t *)c, (mint_t *)d, p);
	return result;
}

MATHC_INLINE mfloat_t psvec2i_dot(struct vec2i *a, struct vec2i *b)
{
	return vec2i_dot((mint_t *)a, (mint_t *)b);
}

MATHC_INLINE mfloat_t psvec2i_angle(struct vec2i *a)
{
	return vec2i_angle((mint_t *)a);
}

MATHC_INLINE mfloat_t psvec2i_length_squared(struct vec2i *a)
{
	return vec2i_length_squared((mint_t *)a);
}

MATHC_INLINE mfloat_t psvec2i_length(struct vec2i *a)
{
	return vec2i_length((mint_t *)a);
}

MATHC_INLINE mfloat_t psvec2i_distance(struct vec2i *a, struct vec2i *b)
{
	return vec2i_distance((mint_t *)a, (mint_t *)b);
}

MATHC_INLINE mfloat_t psvec2i_distance_squared(struct vec2i *a, struct vec2i *b)
{
	return vec2i_distance_squared((mint_t *)a, (mint_t *)b);
}

/* Vector 3D */
MATHC_INLINE bool psvec3_is_zero(struct vec3 *a)
{
	return vec3_is_zero((mfloat_t *)a);
}

MATHC_INLINE bool psvec3_is_near_zero(struct vec3 *a, mfloat_t epsilon)
{
	return vec3_is_near_zero((mfloat_t *)a, epsilon);
}

MATHC_INLINE bool psvec3_is_equal(struct vec3 *a, struct vec3 *b)
{
	return vec3_is_equal((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE bool psvec3_is_nearly_equal(struct vec3 *a, struct vec3 *b, mfloat_t epsilon)
{
	return vec3_is_nearly_equal((mfloat_t *)a, (mfloat_t *)b, epsilon);
}

MATHC_INLINE struct vec3 *psvec3(struct vec3 *result, mfloat_t x, mfloat_t y, mfloat_t z)
{
	vec3((mfloat_t *)result, x, y, z);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_assign(struct vec3 *result, struct vec3 *a)
{
	vec3_assign((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_assign_vec3i(struct vec3 *result, struct vec3i *a)
{
	vec3_assign_vec3i((mfloat_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_zero(struct vec3 *result)
{
	vec3_zero((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_one(struct vec3 *result)
{
	vec3_one((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_add(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_add((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_subtract(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_subtract((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_scale(struct vec3 *result, struct vec3 *a, mfloat_t scalar)
{
	vec3_scale((mfloat_t *)result, (mfloat_t *)a, scalar);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_multiply(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_multiply((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_multiply_mat3(struct vec3 *result, struct vec3 *a, struct mat3 *m)
{
	vec3_multiply_mat3((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_divide(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_divide((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_snap(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_snap((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_negative(struct vec3 *result, struct vec3 *a)
{
	vec3_negative((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_inverse(struct vec3 *result, struct vec3 *a)
{
	vec3_inverse((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_abs(struct vec3 *result, struct vec3 *a)
{
	vec3_abs((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_floor(struct vec3 *result, struct vec3 *a)
{
	vec3_floor((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_ceil(struct vec3 *result, struct vec3 *a)
{
	vec3_ceil((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_round(struct vec3 *result, struct vec3 *a)
{
	vec3_round((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_max(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_max((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_min(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_min((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_clamp(struct vec3 *result, struct vec3 *a, struct vec3 *lower, struct vec3 *higher)
{
	vec3_clamp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)lower, (mfloat_t *)higher);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_cross(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_cross((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_normalize(struct vec3 *result, struct vec3 *a)
{
	vec3_normalize((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_project(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_project((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_slide(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_slide((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_reflect(struct vec3 *result, struct vec3 *a, struct vec3 *b)
{
	vec3_reflect((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_lerp(struct vec3 *result, struct vec3 *a, struct vec3 *b, mfloat_t p)
{
	vec3_lerp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, p);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_bezier3(struct vec3 *result, struct vec3 *a, struct vec3 *b, struct vec3 *c, mfloat_t p)
{
	vec3_bezier3((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, (mfloat_t *)c, p);
	return result;
}

MATHC_INLINE struct vec3 *psvec3_bezier4(struct vec3 *result, struct vec3 *a, struct vec3 *b, struct vec3 *c, struct vec3 *d, mfloat_t p)
{
	vec3_bezier4((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, (mfloat_t *)c, (mfloat_t *)d, p);
	return result;
}

MATHC_INLINE mfloat_t psvec3_dot(struct vec3 *a, struct vec3 *b)
{
	return vec3_dot((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE mfloat_t psvec3_length(struct vec3 *a)
{
	return vec3_length((mfloat_t *)a);
}

MATHC_INLINE mfloat_t psvec3_length_squared(struct vec3 *a)
{
	return vec3_length_squared((mfloat_t *)a);
}

MATHC_INLINE mfloat_t psvec3_distance(struct vec3 *a, struct vec3 *b)
{
	return vec3_distance((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE mfloat_t psvec3_distance_squared(struct vec3 *a, struct vec3 *b)
{
	return vec3_distance_squared((mfloat_t *)a, (mfloat_t *)b);
}

/* Vector 3D Integer */
MATHC_INLINE bool psvec3i_is_zero(struct vec3i *a)
{
	return vec3i_is_zero((mint_t *)a);
}

MATHC_INLINE bool psvec3i_is_equal(struct vec3i *a, struct vec3i *b)
{
	return vec3i_is_equal((mint_t *)a, (mint_t *)b);
}

MATHC_INLINE struct vec3i *psvec3i(struct vec3i *result, mfloat_t x, mfloat_t y, mfloat_t z)
{
	vec3i((mint_t *)result, x, y, z);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_assign(struct vec3i *result, struct vec3i *a)
{
	vec3i_assign((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_assign_vec3(struct vec3i *result, struct vec3 *a)
{
	vec3i_assign_vec3((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_zero(struct vec3i *result)
{
	vec3i_zero((mint_t *)result);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_one(struct vec3i *result)
{
	vec3i_one((mint_t *)result);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_add(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_add((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_subtract(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_subtract((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_scale(struct vec3i *result, struct vec3i *a, mfloat_t scalar)
{
	vec3i_scale((mint_t *)result, (mint_t *)a, scalar);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_multiply(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_multiply((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_multiply_mat3(struct vec3i *result, struct vec3i *a, struct mat3 *m)
{
	vec3i_multiply_mat3((mint_t *)result, (mint_t *)a, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_divide(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_divide((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_snap(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_snap((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_negative(struct vec3i *result, struct vec3i *a)
{
	vec3i_negative((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_inverse(struct vec3i *result, struct vec3i *a)
{
	vec3i_inverse((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_abs(struct vec3i *result, struct vec3i *a)
{
	vec3i_abs((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_floor(struct vec3i *result, struct vec3 *a)
{
	vec3i_floor((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_ceil(struct vec3i *result, struct vec3 *a)
{
	vec3i_ceil((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_round(struct vec3i *result, struct vec3 *a)
{
	vec3i_round((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_max(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_max((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_min(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_min((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_clamp(struct vec3i *result, struct vec3i *a, struct vec3i *lower, struct vec3i *higher)
{
	vec3i_clamp((mint_t *)result, (mint_t *)a, (mint_t *)lower, (mint_t *)higher);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_cross(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_cross((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_normalize(struct vec3i *result, struct vec3i *a)
{
	vec3i_normalize((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_project(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_project((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_slide(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_slide((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_reflect(struct vec3i *result, struct vec3i *a, struct vec3i *b)
{
	vec3i_reflect((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_lerp(struct vec3i *result, struct vec3i *a, struct vec3i *b, mfloat_t p)
{
	vec3i_lerp((mint_t *)result, (mint_t *)a, (mint_t *)b, p);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_bezier3(struct vec3i *result, struct vec3i *a, struct vec3i *b, struct vec3i *c, mfloat_t p)
{
	vec3i_bezier3((mint_t *)result, (mint_t *)a, (mint_t *)b, (mint_t *)c, p);
	return result;
}

MATHC_INLINE struct vec3i *psvec3i_bezier4(struct vec3i *result, struct vec3i *a, struct vec3i *b, struct vec3i *c, struct vec3i *d, mfloat_t p)
{
	vec3i_bezier4((mint_t *)result, (mint_t *)a, (mint_t *)b, (mint_t *)c, (mint_t *)d, p);
	return result;
}

MATHC_INLINE mfloat_t psvec3i_dot(struct vec3i *a, struct vec3i *b)
{
	return vec3i_dot((mint_t *)a, (mint_t *)b);
}

MATHC_INLINE mfloat_t psvec3i_length(struct vec3i *a)
{
	return vec3i_length((mint_t *)a);
}

MATHC_INLINE mfloat_t psvec3i_length_squared(struct vec3i *a)
{
	return vec3i_length_squared((mint_t *)a);
}

MATHC_INLINE mfloat_t psvec3i_distance(struct vec3i *a, struct vec3i *b)
{
	return vec3i_distance((mint_t *)a, (mint_t *)b);
}

MATHC_INLINE mfloat_t psvec3i_distance_squared(struct vec3i *a, struct vec3i *b)
{
	return vec3i_distance_squared((mint_t *)a, (mint_t *)b);
}

/* Vector 4D */
MATHC_INLINE bool psvec4_is_zero(struct vec4 *a)
{
	return vec4_is_zero((mfloat_t *)a);
}

MATHC_INLINE bool psvec4_is_near_zero(struct vec4 *a, mfloat_t epsilon)
{
	return vec4_is_near_zero((mfloat_t *)a, epsilon);
}

MATHC_INLINE bool psvec4_is_equal(struct vec4 *a, struct vec4 *b)
{
	return vec4_is_equal((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE bool psvec4_is_nearly_equal(struct vec4 *a, struct vec4 *b, mfloat_t epsilon)
{
	return vec4_is_nearly_equal((mfloat_t *)a, (mfloat_t *)b, epsilon);
}

MATHC_INLINE struct vec4 *psvec4(struct vec4 *result, mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w)
{
	vec4((mfloat_t *)result, x, y, z, w);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_assign(struct vec4 *result, struct vec4 *a)
{
	vec4_assign((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_assign_vec4i(struct vec4 *result, struct vec4i *a)
{
	vec4_assign_vec4i((mfloat_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_zero(struct vec4 *result)
{
	vec4_zero((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_one(struct vec4 *result)
{
	vec4_one((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_add(struct vec4 *result, struct vec4 *a, struct vec4 *b)
{
	vec4_add((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_subtract(struct vec4 *result, struct vec4 *a, struct vec4 *b)
{
	vec4_subtract((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_scale(struct vec4 *result, struct vec4 *a, mfloat_t scalar)
{
	vec4_scale((mfloat_t *)result, (mfloat_t *)a, scalar);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_multiply(struct vec4 *result, struct vec4 *a, struct vec4 *b)
{
	vec4_multiply((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_multiply_mat4(struct vec4 *result, struct vec4 *a, struct mat4 *m)
{
	vec4_multiply_mat4((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_divide(struct vec4 *result, struct vec4 *a, struct vec4 *b)
{
	vec4_divide((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_snap(struct vec4 *result, struct vec4 *a, struct vec4 *b)
{
	vec4_snap((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_negative(struct vec4 *result, struct vec4 *a)
{
	vec4_negative((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_inverse(struct vec4 *result, struct vec4 *a)
{
	vec4_inverse((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_abs(struct vec4 *result, struct vec4 *a)
{
	vec4_abs((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_floor(struct vec4 *result, struct vec4 *a)
{
	vec4_floor((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_ceil(struct vec4 *result, struct vec4 *a)
{
	vec4_ceil((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_round(struct vec4 *result, struct vec4 *a)
{
	vec4_round((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_max(struct vec4 *result, struct vec4 *a, struct vec4 *b)
{
	vec4_max((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_min(struct vec4 *result, struct vec4 *a, struct vec4 *b)
{
	vec4_min((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_clamp(struct vec4 *result, struct vec4 *a, struct vec4 *lower, struct vec4 *higher)
{
	vec4_clamp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)lower, (mfloat_t *)higher);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_normalize(struct vec4 *result, struct vec4 *a)
{
	vec4_normalize((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4 *psvec4_lerp(struct vec4 *result, struct vec4 *a, struct vec4 *b, mfloat_t p)
{
	vec4_lerp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, p);
	return result;
}

/* Vector 4D */
MATHC_INLINE bool psvec4i_is_zero(struct vec4i *a)
{
	return vec4i_is_zero((mint_t *)a);
}

MATHC_INLINE bool psvec4i_is_equal(struct vec4i *a, struct vec4i *b)
{
	return vec4i_is_equal((mint_t *)a, (mint_t *)b);
}

MATHC_INLINE struct vec4i *psvec4i(struct vec4i *result, mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w)
{
	vec4i((mint_t *)result, x, y, z, w);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_assign(struct vec4i *result, struct vec4i *a)
{
	vec4i_assign((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_assign_vec4(struct vec4i *result, struct vec4 *a)
{
	vec4i_assign_vec4((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_zero(struct vec4i *result)
{
	vec4i_zero((mint_t *)result);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_one(struct vec4i *result)
{
	vec4i_one((mint_t *)result);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_add(struct vec4i *result, struct vec4i *a, struct vec4i *b)
{
	vec4i_add((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_subtract(struct vec4i *result, struct vec4i *a, struct vec4i *b)
{
	vec4i_subtract((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_scale(struct vec4i *result, struct vec4i *a, mfloat_t scalar)
{
	vec4i_scale((mint_t *)result, (mint_t *)a, scalar);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_multiply(struct vec4i *result, struct vec4i *a, struct vec4i *b)
{
	vec4i_multiply((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_multiply_mat4(struct vec4i *result, struct vec4i *a, struct mat4 *m)
{
	vec4i_multiply_mat4((mint_t *)result, (mint_t *)a, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_divide(struct vec4i *result, struct vec4i *a, struct vec4i *b)
{
	vec4i_divide((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_snap(struct vec4i *result, struct vec4i *a, struct vec4i *b)
{
	vec4i_snap((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_negative(struct vec4i *result, struct vec4i *a)
{
	vec4i_negative((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_inverse(struct vec4i *result, struct vec4i *a)
{
	vec4i_inverse((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_abs(struct vec4i *result, struct vec4i *a)
{
	vec4i_abs((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_floor(struct vec4i *result, struct vec4 *a)
{
	vec4i_floor((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_ceil(struct vec4i *result, struct vec4 *a)
{
	vec4i_ceil((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_round(struct vec4i *result, struct vec4 *a)
{
	vec4i_round((mint_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_max(struct vec4i *result, struct vec4i *a, struct vec4i *b)
{
	vec4i_max((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_min(struct vec4i *result, struct vec4i *a, struct vec4i *b)
{
	vec4i_min((mint_t *)result, (mint_t *)a, (mint_t *)b);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_clamp(struct vec4i *result, struct vec4i *a, struct vec4i *lower, struct vec4i *higher)
{
	vec4i_clamp((mint_t *)result, (mint_t *)a, (mint_t *)lower, (mint_t *)higher);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_normalize(struct vec4i *result, struct vec4i *a)
{
	vec4i_normalize((mint_t *)result, (mint_t *)a);
	return result;
}

MATHC_INLINE struct vec4i *psvec4i_lerp(struct vec4i *result, struct vec4i *a, struct vec4i *b, mfloat_t p)
{
	vec4i_lerp((mint_t *)result, (mint_t *)a, (mint_t *)b, p);
	return result;
}

/* Quaternion */
MATHC_INLINE bool psquat_is_zero(struct quat *a)
{
	return quat_is_zero((mfloat_t *)a);
}

MATHC_INLINE bool psquat_is_near_zero(struct quat *a, mfloat_t epsilon)
{
	return quat_is_near_zero((mfloat_t *)a, epsilon);
}

MATHC_INLINE bool psquat_is_equal(struct quat *a, struct quat *b)
{
	return quat_is_equal((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE bool psquat_is_nearly_equal(struct quat *a, struct quat *b, mfloat_t epsilon)
{
	return quat_is_nearly_equal((mfloat_t *)a, (mfloat_t *)b, epsilon);
}

MATHC_INLINE struct quat *psquat(struct quat *result, mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w)
{
	quat((mfloat_t *)result, x, y, z, w);
	return result;
}

MATHC_INLINE struct quat *psquat_assign(struct quat *result, struct quat *a)
{
	quat_assign((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct quat *psquat_zero(struct quat *result)
{
	quat_zero((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct quat *psquat_null(struct quat *result)
{
	quat_null((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct quat *psquat_scale(struct quat *result, struct quat *a, mfloat_t scalar)
{
	quat_scale((mfloat_t *)result, (mfloat_t *)a, scalar);
	return result;
}

MATHC_INLINE struct quat *psquat_multiply(struct quat *result, struct quat *a, struct quat *b)
{
	quat_multiply((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct quat *psquat_divide(struct quat *result, struct quat *a, struct quat *b)
{
	quat_divide((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct quat *psquat_negative(struct quat *result, struct quat *a)
{
	quat_negative((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct quat *psquat_conjugate(struct quat *result, struct quat *a)
{
	quat_conjugate((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct quat *psquat_inverse(struct quat *result, struct quat *a)
{
	quat_inverse((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct quat *psquat_normalize(struct quat *result, struct quat *a)
{
	quat_normalize((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct quat *psquat_power(struct quat *result, struct quat *a, mfloat_t exponent)
{
	quat_power((mfloat_t *)result, (mfloat_t *)a, exponent);
	return result;
}

MATHC_INLINE struct quat *psquat_from_axis_angle(struct quat *result, struct vec3 *a, mfloat_t angle)
{
	quat_from_axis_angle((mfloat_t *)result, (mfloat_t *)a, angle);
	return result;
}

MATHC_INLINE struct quat *psquat_from_vec3(struct quat *result, struct vec3 *a, struct vec3 *b)
{
	quat_from_vec3((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct quat *psquat_from_mat4(struct quat *result, struct mat4 *a)
{
	quat_from_mat4((mfloat_t *)result, (mfloat_t *)a);
	return result;
}

MATHC_INLINE struct quat *psquat_from_yaw_pitch_roll(struct quat *result, mfloat_t yaw, mfloat_t pitch, mfloat_t roll)
{
	quat_from_yaw_pitch_roll((mfloat_t *)result, yaw, pitch, roll);
	return result;
}

MATHC_INLINE struct quat *psquat_lerp(struct quat *result, struct quat *a, struct quat *b, mfloat_t p)
{
	quat_lerp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, p);
	return result;
}

MATHC_INLINE struct quat *psquat_slerp(struct quat *result, struct quat *a, struct quat *b, mfloat_t p)
{
	quat_slerp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, p);
	return result;
}

MATHC_INLINE mfloat_t psquat_dot(struct quat *a, struct quat *b)
{
	return quat_dot((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE mfloat_t psquat_angle(struct quat *a, struct quat *b)
{
	return quat_angle((mfloat_t *)a, (mfloat_t *)b);
}

MATHC_INLINE mfloat_t psquat_length(struct quat *a)
{
	return quat_length((mfloat_t *)a);
}

MATHC_INLINE mfloat_t psquat_length_squared(struct quat *a)
{
	return quat_length_squared((mfloat_t *)a);
}

/* Matrix 2x2 */
MATHC_INLINE struct mat2 *psmat2(struct mat2 *result,
	mfloat_t m11, mfloat_t m12,
	mfloat_t m21, mfloat_t m22)
{
	mat2((mfloat_t *)result,
		m11, m12,
		m21, m22);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_zero(struct mat2 *result)
{
	mat2_zero((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_identity(struct mat2 *result)
{
	mat2_identity((mfloat_t *)result);
	return result;
}

MATHC_INLINE mfloat_t psmat2_determinant(struct mat2 *m)
{
	return mat2_determinant((mfloat_t *)m);
}

MATHC_INLINE struct mat2 *psmat2_assign(struct mat2 *result, struct mat2 *m)
{
	mat2_assign((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_assign_mat3(struct mat2 *result, struct mat3 *m)
{
	mat2_assign_mat3((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_assign_mat4(struct mat2 *result, struct mat4 *m)
{
	mat2_assign_mat4((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_transpose(struct mat2 *result, struct mat2 *m)
{
	mat2_transpose((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_cofactor(struct mat2 *result, struct mat2 *m)
{
	mat2_cofactor((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_inverse(struct mat2 *result, struct mat2 *m)
{
	mat2_inverse((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_rotation(struct mat2 *result, mfloat_t angle)
{
	mat2_rotation((mfloat_t *)result, angle);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_scaling(struct mat2 *result, struct mat2 *v)
{
	mat2_scaling((mfloat_t *)result, (mfloat_t *)v);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_negative(struct mat2 *result, struct mat2 *m)
{
	mat2_negative((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_scale(struct mat2 *result, struct mat2 *m, mfloat_t s)
{
	mat2_scale((mfloat_t *)result, (mfloat_t *)m, s);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_multiply(struct mat2 *result, struct mat2 *a, struct mat2 *b)
{
	mat2_multiply((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct mat2 *psmat2_lerp(struct mat2 *result, struct mat2 *a, struct mat2 *b, mfloat_t p)
{
	mat2_lerp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, p);
	return result;
}

/* Matrix 3x3 */
MATHC_INLINE struct mat3 *psmat3(struct mat3 *result,
	mfloat_t m11, mfloat_t m12, mfloat_t m13,
	mfloat_t m21, mfloat_t m22, mfloat_t m23,
	mfloat_t m31, mfloat_t m32, mfloat_t m33)
{
	mat3((mfloat_t *)result,
		m11, m12, m13,
		m21, m22, m23,
		m31, m32, m33);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_zero(struct mat3 *result)
{
	mat3_zero((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_identity(struct mat3 *result)
{
	mat3_identity((mfloat_t *)result);
	return result;
}

MATHC_INLINE mfloat_t psmat3_determinant(struct mat3 *m)
{
	return mat3_determinant((mfloat_t *)m);
}

MATHC_INLINE struct mat3 *psmat3_assign(struct mat3 *result, struct mat3 *m)
{
	mat3_assign((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_assign_mat2(struct mat3 *result, struct mat2 *m)
{
	mat3_assign_mat2((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_assign_mat4(struct mat3 *result, struct mat4 *m)
{
	mat3_assign_mat4((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_transpose(struct mat3 *result, struct mat3 *m)
{
	mat3_transpose((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_cofactor(struct mat3 *result, struct mat3 *m)
{
	mat3_cofactor((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_adjugate(struct mat3 *result, struct mat3 *m)
{
	mat3_adjugate((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_inverse(struct mat3 *result, struct mat3 *m)
{
	mat3_inverse((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_rotation_x(struct mat3 *result, mfloat_t angle)
{
	mat3_rotation_x((mfloat_t *)result, angle);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_rotation_y(struct mat3 *result, mfloat_t angle)
{
	mat3_rotation_y((mfloat_t *)result, angle);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_rotation_z(struct mat3 *result, mfloat_t angle)
{
	mat3_rotation_z((mfloat_t *)result, angle);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_rotation_axis(struct mat3 *result, struct vec3 *a, mfloat_t angle)
{
	mat3_rotation_axis((mfloat_t *)result, (mfloat_t *)a, angle);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_rotation_quaternion(struct mat3 *result, struct quat *q)
{
	mat3_rotation_quaternion((mfloat_t *)result, (mfloat_t *)q);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_scaling(struct mat3 *result, struct mat3 *v)
{
	mat3_scaling((mfloat_t *)result, (mfloat_t *)v);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_negative(struct mat3 *result, struct mat3 *m)
{
	mat3_negative((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_scale(struct mat3 *result, struct mat3 *m, mfloat_t s)
{
	mat3_scale((mfloat_t *)result, (mfloat_t *)m, s);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_multiply(struct mat3 *result, struct mat3 *a, struct mat3 *b)
{
	mat3_multiply((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct mat3 *psmat3_lerp(struct mat3 *result, struct mat3 *a, struct mat3 *b, mfloat_t p)
{
	mat3_lerp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, p);
	return result;
}

/* Matrix 4x4 */
MATHC_INLINE struct mat4 *psmat4(struct mat4 *result,
	mfloat_t m11, mfloat_t m12, mfloat_t m13, mfloat_t m14,
	mfloat_t m21, mfloat_t m22, mfloat_t m23, mfloat_t m24,
	mfloat_t m31, mfloat_t m32, mfloat_t m33, mfloat_t m34,
	mfloat_t m41, mfloat_t m42, mfloat_t m43, mfloat_t m44)
{
	mat4((mfloat_t *)result,
		m11, m12, m13, m14,
		m21, m22, m23, m24,
		m31, m32, m33, m34,
		m41, m42, m43, m44);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_zero(struct mat4 *result)
{
	mat4_zero((mfloat_t *)result);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_identity(struct mat4 *result)
{
	mat4_identity((mfloat_t *)result);
	return result;
}

MATHC_INLINE mfloat_t psmat4_determinant(struct mat4 *m)
{
	return mat4_determinant((mfloat_t *)m);
}

MATHC_INLINE struct mat4 *psmat4_assign(struct mat4 *result, struct mat4 *m)
{
	mat4_assign((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_assign_mat2(struct mat4 *result, struct mat2 *m)
{
	mat4_assign_mat2((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_assign_mat3(struct mat4 *result, struct mat3 *m)
{
	mat4_assign_mat3((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_transpose(struct mat4 *result, struct mat4 *m)
{
	mat4_transpose((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_adjugate(struct mat4 *result, struct mat4 *m)
{
	mat4_adjugate((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_inverse(struct mat4 *result, struct mat4 *m)
{
	mat4_inverse((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_ortho(struct mat4 *result, mfloat_t l, mfloat_t r, mfloat_t b, mfloat_t t, mfloat_t n, mfloat_t f)
{
	mat4_ortho((mfloat_t *)result, l, r, b, t, n, f);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_perspective(struct mat4 *result, mfloat_t fov_y, mfloat_t aspect, mfloat_t n, mfloat_t f)
{
	mat4_perspective((mfloat_t *)result, fov_y, aspect, n, f);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_perspective_fov(struct mat4 *result, mfloat_t fov, mfloat_t w, mfloat_t h, mfloat_t n, mfloat_t f)
{
	mat4_perspective_fov((mfloat_t *)result, fov, w, h, n, f);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_perspective_infinite(struct mat4 *result, mfloat_t fov_y, mfloat_t aspect, mfloat_t n)
{
	mat4_perspective_infinite((mfloat_t *)result, fov_y, aspect, n);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_rotation_x(struct mat4 *result, mfloat_t angle)
{
	mat4_rotation_x((mfloat_t *)result, angle);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_rotation_y(struct mat4 *result, mfloat_t angle)
{
	mat4_rotation_y((mfloat_t *)result, angle);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_rotation_z(struct mat4 *result, mfloat_t angle)
{
	mat4_rotation_z((mfloat_t *)result, angle);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_rotation_axis(struct mat4 *result, struct mat4 *a, mfloat_t angle)
{
	mat4_rotation_axis((mfloat_t *)result, (mfloat_t *)a, angle);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_rotation_quaternion(struct mat4 *result, struct mat4 *q)
{
	mat4_rotation_quaternion((mfloat_t *)result, (mfloat_t *)q);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_look_at(struct mat4 *result, struct vec3 *position, struct vec3 *target, struct vec3 *up_axis)
{
	mat4_look_at((mfloat_t *)result, (mfloat_t *)position, (mfloat_t *)target, (mfloat_t *)up_axis);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_translation(struct mat4 *result, struct mat4 *v)
{
	mat4_translation((mfloat_t *)result, (mfloat_t *)v);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_scaling(struct mat4 *result, struct mat4 *v)
{
	mat4_scaling((mfloat_t *)result, (mfloat_t *)v);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_negative(struct mat4 *result, struct mat4 *m)
{
	mat4_negative((mfloat_t *)result, (mfloat_t *)m);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_scale(struct mat4 *result, struct mat4 *m, mfloat_t s)
{
	mat4_scale((mfloat_t *)result, (mfloat_t *)m, s);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_multiply(struct mat4 *result, struct mat4 *a, struct mat4 *b)
{
	mat4_multiply((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b);
	return result;
}

MATHC_INLINE struct mat4 *psmat4_lerp(struct mat4 *result, struct mat4 *a, struct mat4 *b, mfloat_t p)
{
	mat4_lerp((mfloat_t *)result, (mfloat_t *)a, (mfloat_t *)b, p);
	return result;
}
#endif

#ifndef MATHC_NO_STRUCT_FUNCTIONS
/* Vector 2D */
MATHC_INLINE bool svec2_is_zero(struct vec2 a)
{
	return vec2_is_zero((mfloat_t *)&a);
}

MATHC_INLINE bool svec2_is_near_zero(struct vec2 a, mfloat_t epsilon)
{
	return vec2_is_near_zero((mfloat_t *)&a, epsilon);
}

MATHC_INLINE bool svec2_is_equal(struct vec2 a, struct vec2 b)
{
	return vec2_is_equal((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE bool svec2_is_nearly_equal(struct vec2 a, struct vec2 b, mfloat_t epsilon)
{
	return vec2_is_nearly_equal((mfloat_t *)&a, (mfloat_t *)&b, epsilon);
}

MATHC_INLINE struct vec2 svec2(mfloat_t x, mfloat_t y)
{
	struct vec2 result;
	vec2((mfloat_t *)&result, x, y);
	return result;
}

MATHC_INLINE struct vec2 svec2_assign(struct vec2 a)
{
	struct vec2 result;
	vec2_assign((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_assign_vec2i(struct vec2i a)
{
	struct vec2 result;
	vec2_assign_vec2i((mfloat_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_zero(void)
{
	struct vec2 result;
	vec2_zero((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct vec2 svec2_one(void)
{
	struct vec2 result;
	vec2_one((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct vec2 svec2_add(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_add((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_subtract(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_subtract((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_scale(struct vec2 a, mfloat_t scalar)
{
	struct vec2 result;
	vec2_scale((mfloat_t *)&result, (mfloat_t *)&a, scalar);
	return result;
}

MATHC_INLINE struct vec2 svec2_multiply(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_multiply((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_multiply_mat2(struct vec2 a, struct mat2 m)
{
	struct vec2 result;
	vec2_multiply_mat2((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct vec2 svec2_divide(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_divide((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_snap(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_snap((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_negative(struct vec2 a)
{
	struct vec2 result;
	vec2_negative((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_inverse(struct vec2 a)
{
	struct vec2 result;
	vec2_inverse((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_abs(struct vec2 a)
{
	struct vec2 result;
	vec2_abs((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_floor(struct vec2 a)
{
	struct vec2 result;
	vec2_floor((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_ceil(struct vec2 a)
{
	struct vec2 result;
	vec2_ceil((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_round(struct vec2 a)
{
	struct vec2 result;
	vec2_round((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_max(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_max((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_min(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_min((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_clamp(struct vec2 a, struct vec2 lower, struct vec2 higher)
{
	struct vec2 result;
	vec2_clamp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&lower, (mfloat_t *)&higher);
	return result;
}

MATHC_INLINE struct vec2 svec2_normalize(struct vec2 a)
{
	struct vec2 result;
	vec2_normalize((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_project(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_project((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_slide(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_slide((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_reflect(struct vec2 a, struct vec2 b)
{
	struct vec2 result;
	vec2_reflect((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec2 svec2_tangent(struct vec2 a)
{
	struct vec2 result;
	vec2_tangent((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2 svec2_rotate(struct vec2 a, mfloat_t angle)
{
	struct vec2 result;
	vec2_rotate((mfloat_t *)&result, (mfloat_t *)&a, angle);
	return result;
}

MATHC_INLINE struct vec2 svec2_lerp(struct vec2 a, struct vec2 b, mfloat_t p)
{
	struct vec2 result;
	vec2_lerp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, p);
	return result;
}

MATHC_INLINE struct vec2 svec2_bezier3(struct vec2 a, struct vec2 b, struct vec2 c, mfloat_t p)
{
	struct vec2 result;
	vec2_bezier3((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, (mfloat_t *)&c, p);
	return result;
}

MATHC_INLINE struct vec2 svec2_bezier4(struct vec2 a, struct vec2 b, struct vec2 c, struct vec2 d, mfloat_t p)
{
	struct vec2 result;
	vec2_bezier4((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, (mfloat_t *)&c, (mfloat_t *)&d, p);
	return result;
}

MATHC_INLINE mfloat_t svec2_dot(struct vec2 a, struct vec2 b)
{
	return vec2_dot((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE mfloat_t svec2_angle(struct vec2 a)
{
	return vec2_angle((mfloat_t *)&a);
}

MATHC_INLINE mfloat_t svec2_length_squared(struct vec2 a)
{
	return vec2_length_squared((mfloat_t *)&a);
}

MATHC_INLINE mfloat_t svec2_length(struct vec2 a)
{
	return vec2_length((mfloat_t *)&a);
}

MATHC_INLINE mfloat_t svec2_distance(struct vec2 a, struct vec2 b)
{
	return vec2_distance((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE mfloat_t svec2_distance_squared(struct vec2 a, struct vec2 b)
{
	return vec2_distance_squared((mfloat_t *)&a, (mfloat_t *)&b);
}

/* Vector 2D Integer */
MATHC_INLINE bool svec2i_is_zero(struct vec2i a)
{
	return vec2i_is_zero((mint_t *)&a);
}

MATHC_INLINE bool svec2i_is_equal(struct vec2i a, struct vec2i b)
{
	return vec2i_is_equal((mint_t *)&a, (mint_t *)&b);
}

MATHC_INLINE struct vec2i svec2i(mfloat_t x, mfloat_t y)
{
	struct vec2i result;
	vec2i((mint_t *)&result, x, y);
	return result;
}

MATHC_INLINE struct vec2i svec2i_assign(struct vec2i a)
{
	struct vec2i result;
	vec2i_assign((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_assign_vec2(struct vec2 a)
{
	struct vec2i result;
	vec2i_assign_vec2((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_zero(void)
{
	struct vec2i result;
	vec2i_zero((mint_t *)&result);
	return result;
}

MATHC_INLINE struct vec2i svec2i_one(void)
{
	struct vec2i result;
	vec2i_one((mint_t *)&result);
	return result;
}

MATHC_INLINE struct vec2i svec2i_add(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_add((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_subtract(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_subtract((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_scale(struct vec2i a, mfloat_t scalar)
{
	struct vec2i result;
	vec2i_scale((mint_t *)&result, (mint_t *)&a, scalar);
	return result;
}

MATHC_INLINE struct vec2i svec2i_multiply(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_multiply((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_multiply_mat2(struct vec2i a, struct mat2 m)
{
	struct vec2i result;
	vec2i_multiply_mat2((mint_t *)&result, (mint_t *)&a, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct vec2i svec2i_divide(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_divide((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_snap(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_snap((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_negative(struct vec2i a)
{
	struct vec2i result;
	vec2i_negative((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_inverse(struct vec2i a)
{
	struct vec2i result;
	vec2i_inverse((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_abs(struct vec2i a)
{
	struct vec2i result;
	vec2i_abs((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_floor(struct vec2 *a)
{
	struct vec2i result;
	vec2i_floor((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_ceil(struct vec2 *a)
{
	struct vec2i result;
	vec2i_ceil((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_round(struct vec2 *a)
{
	struct vec2i result;
	vec2i_round((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_max(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_max((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_min(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_min((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_clamp(struct vec2i a, struct vec2i lower, struct vec2i higher)
{
	struct vec2i result;
	vec2i_clamp((mint_t *)&result, (mint_t *)&a, (mint_t *)&lower, (mint_t *)&higher);
	return result;
}

MATHC_INLINE struct vec2i svec2i_normalize(struct vec2i a)
{
	struct vec2i result;
	vec2i_normalize((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_project(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_project((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_slide(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_slide((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_reflect(struct vec2i a, struct vec2i b)
{
	struct vec2i result;
	vec2i_reflect((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec2i svec2i_tangent(struct vec2i a)
{
	struct vec2i result;
	vec2i_tangent((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec2i svec2i_rotate(struct vec2i a, mfloat_t angle)
{
	struct vec2i result;
	vec2i_rotate((mint_t *)&result, (mint_t *)&a, angle);
	return result;
}

MATHC_INLINE struct vec2i svec2i_lerp(struct vec2i a, struct vec2i b, mfloat_t p)
{
	struct vec2i result;
	vec2i_lerp((mint_t *)&result, (mint_t *)&a, (mint_t *)&b, p);
	return result;
}

MATHC_INLINE struct vec2i svec2i_bezier3(struct vec2i a, struct vec2i b, struct vec2i c, mfloat_t p)
{
	struct vec2i result;
	vec2i_bezier3((mint_t *)&result, (mint_t *)&a, (mint_t *)&b, (mint_t *)&c, p);
	return result;
}

MATHC_INLINE struct vec2i svec2i_bezier4(struct vec2i a, struct vec2i b, struct vec2i c, struct vec2i d, mfloat_t p)
{
	struct vec2i result;
	vec2i_bezier4((mint_t *)&result, (mint_t *)&a, (mint_t *)&b, (mint_t *)&c, (mint_t *)&d, p);
	return result;
}

MATHC_INLINE mfloat_t svec2i_dot(struct vec2i a, struct vec2i b)
{
	return vec2i_dot((mint_t *)&a, (mint_t *)&b);
}

MATHC_INLINE mfloat_t svec2i_angle(struct vec2i a)
{
	return vec2i_angle((mint_t *)&a);
}

MATHC_INLINE mfloat_t svec2i_length_squared(struct vec2i a)
{
	return vec2i_length_squared((mint_t *)&a);
}

MATHC_INLINE mfloat_t svec2i_length(struct vec2i a)
{
	return vec2i_length((mint_t *)&a);
}

MATHC_INLINE mfloat_t svec2i_distance(struct vec2i a, struct vec2i b)
{
	return vec2i_distance((mint_t *)&a, (mint_t *)&b);
}

MATHC_INLINE mfloat_t svec2i_distance_squared(struct vec2i a, struct vec2i b)
{
	return vec2i_distance_squared((mint_t *)&a, (mint_t *)&b);
}

/* Vector 3 */
MATHC_INLINE bool svec3_is_zero(struct vec3 a)
{
	return vec3_is_zero((mfloat_t *)&a);
}

MATHC_INLINE bool svec3_is_near_zero(struct vec3 a, mfloat_t epsilon)
{
	return vec3_is_near_zero((mfloat_t *)&a, epsilon);
}

MATHC_INLINE bool svec3_is_equal(struct vec3 a, struct vec3 b)
{
	return vec3_is_equal((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE bool svec3_is_nearly_equal(struct vec3 a, struct vec3 b, mfloat_t epsilon)
{
	return vec3_is_nearly_equal((mfloat_t *)&a, (mfloat_t *)&b, epsilon);
}

MATHC_INLINE struct vec3 svec3(mfloat_t x, mfloat_t y, mfloat_t z)
{
	struct vec3 result;
	vec3((mfloat_t *)&result, x, y, z);
	return result;
}

MATHC_INLINE struct vec3 svec3_assign(struct vec3 a)
{
	struct vec3 result;
	vec3_assign((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_assign_vec3i(struct vec3i a)
{
	struct vec3 result;
	vec3_assign_vec3i((mfloat_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_zero(void)
{
	struct vec3 result;
	vec3_zero((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct vec3 svec3_one(void)
{
	struct vec3 result;
	vec3_one((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct vec3 svec3_add(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_add((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_subtract(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_subtract((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_scale(struct vec3 a, mfloat_t scalar)
{
	struct vec3 result;
	vec3_scale((mfloat_t *)&result, (mfloat_t *)&a, scalar);
	return result;
}

MATHC_INLINE struct vec3 svec3_multiply(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_multiply((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_multiply_mat3(struct vec3 a, struct mat3 m)
{
	struct vec3 result;
	vec3_multiply_mat3((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct vec3 svec3_divide(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_divide((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_snap(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_snap((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_negative(struct vec3 a)
{
	struct vec3 result;
	vec3_negative((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_inverse(struct vec3 a)
{
	struct vec3 result;
	vec3_inverse((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_abs(struct vec3 a)
{
	struct vec3 result;
	vec3_abs((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_floor(struct vec3 a)
{
	struct vec3 result;
	vec3_floor((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_ceil(struct vec3 a)
{
	struct vec3 result;
	vec3_ceil((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_round(struct vec3 a)
{
	struct vec3 result;
	vec3_round((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_max(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_max((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_min(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_min((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_clamp(struct vec3 a, struct vec3 lower, struct vec3 higher)
{
	struct vec3 result;
	vec3_clamp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&lower, (mfloat_t *)&higher);
	return result;
}

MATHC_INLINE struct vec3 svec3_cross(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_cross((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_normalize(struct vec3 a)
{
	struct vec3 result;
	vec3_normalize((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3 svec3_project(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_project((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_slide(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_slide((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_reflect(struct vec3 a, struct vec3 b)
{
	struct vec3 result;
	vec3_reflect((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec3 svec3_lerp(struct vec3 a, struct vec3 b, mfloat_t p)
{
	struct vec3 result;
	vec3_lerp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, p);
	return result;
}

MATHC_INLINE struct vec3 svec3_bezier3(struct vec3 a, struct vec3 b, struct vec3 c, mfloat_t p)
{
	struct vec3 result;
	vec3_bezier3((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, (mfloat_t *)&c, p);
	return result;
}

MATHC_INLINE struct vec3 svec3_bezier4(struct vec3 a, struct vec3 b, struct vec3 c, struct vec3 d, mfloat_t p)
{
	struct vec3 result;
	vec3_bezier4((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, (mfloat_t *)&c, (mfloat_t *)&d, p);
	return result;
}

MATHC_INLINE mfloat_t svec3_dot(struct vec3 a, struct vec3 b)
{
	return vec3_dot((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE mfloat_t svec3_length(struct vec3 a)
{
	return vec3_length((mfloat_t *)&a);
}

MATHC_INLINE mfloat_t svec3_length_squared(struct vec3 a)
{
	return vec3_length_squared((mfloat_t *)&a);
}

MATHC_INLINE mfloat_t svec3_distance(struct vec3 a, struct vec3 b)
{
	return vec3_distance((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE mfloat_t svec3_distance_squared(struct vec3 a, struct vec3 b)
{
	return vec3_distance_squared((mfloat_t *)&a, (mfloat_t *)&b);
}

/* Vector 3 Integer */
MATHC_INLINE bool svec3i_is_zero(struct vec3i a)
{
	return vec3i_is_zero((mint_t *)&a);
}

MATHC_INLINE bool svec3i_is_equal(struct vec3i a, struct vec3i b)
{
	return vec3i_is_equal((mint_t *)&a, (mint_t *)&b);
}

MATHC_INLINE struct vec3i svec3i(mint_t x, mint_t y, mint_t z)
{
	struct vec3i result;
	vec3i((mint_t *)&result, x, y, z);
	return result;
}

MATHC_INLINE struct vec3i svec3i_assign(struct vec3i a)
{
	struct vec3i result;
	vec3i_assign((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_assign_vec3(struct vec3 a)
{
	struct vec3i result;
	vec3i_assign_vec3((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_zero(void)
{
	struct vec3i result;
	vec3i_zero((mint_t *)&result);
	return result;
}

MATHC_INLINE struct vec3i svec3i_one(void)
{
	struct vec3i result;
	vec3i_one((mint_t *)&result);
	return result;
}

MATHC_INLINE struct vec3i svec3i_add(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_add((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_subtract(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_subtract((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_scale(struct vec3i a, mint_t scalar)
{
	struct vec3i result;
	vec3i_scale((mint_t *)&result, (mint_t *)&a, scalar);
	return result;
}

MATHC_INLINE struct vec3i svec3i_multiply(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_multiply((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_multiply_mat3(struct vec3i a, struct mat3 m)
{
	struct vec3i result;
	vec3i_multiply_mat3((mint_t *)&result, (mint_t *)&a, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct vec3i svec3i_divide(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_divide((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_snap(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_snap((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_negative(struct vec3i a)
{
	struct vec3i result;
	vec3i_negative((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_inverse(struct vec3i a)
{
	struct vec3i result;
	vec3i_inverse((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_abs(struct vec3i a)
{
	struct vec3i result;
	vec3i_abs((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_floor(struct vec3 a)
{
	struct vec3i result;
	vec3i_floor((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_ceil(struct vec3 a)
{
	struct vec3i result;
	vec3i_ceil((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_round(struct vec3 a)
{
	struct vec3i result;
	vec3i_round((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_max(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_max((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_min(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_min((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_clamp(struct vec3i a, struct vec3i lower, struct vec3i higher)
{
	struct vec3i result;
	vec3i_clamp((mint_t *)&result, (mint_t *)&a, (mint_t *)&lower, (mint_t *)&higher);
	return result;
}

MATHC_INLINE struct vec3i svec3i_cross(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_cross((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_normalize(struct vec3i a)
{
	struct vec3i result;
	vec3i_normalize((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec3i svec3i_project(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_project((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_slide(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_slide((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_reflect(struct vec3i a, struct vec3i b)
{
	struct vec3i result;
	vec3i_reflect((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec3i svec3i_lerp(struct vec3i a, struct vec3i b, mint_t p)
{
	struct vec3i result;
	vec3i_lerp((mint_t *)&result, (mint_t *)&a, (mint_t *)&b, p);
	return result;
}

MATHC_INLINE struct vec3i svec3i_bezier3(struct vec3i a, struct vec3i b, struct vec3i c, mint_t p)
{
	struct vec3i result;
	vec3i_bezier3((mint_t *)&result, (mint_t *)&a, (mint_t *)&b, (mint_t *)&c, p);
	return result;
}

MATHC_INLINE struct vec3i svec3i_bezier4(struct vec3i a, struct vec3i b, struct vec3i c, struct vec3i d, mint_t p)
{
	struct vec3i result;
	vec3i_bezier4((mint_t *)&result, (mint_t *)&a, (mint_t *)&b, (mint_t *)&c, (mint_t *)&d, p);
	return result;
}

MATHC_INLINE mint_t svec3i_dot(struct vec3i a, struct vec3i b)
{
	return vec3i_dot((mint_t *)&a, (mint_t *)&b);
}

MATHC_INLINE mint_t svec3i_length(struct vec3i a)
{
	return vec3i_length((mint_t *)&a);
}

MATHC_INLINE mint_t svec3i_length_squared(struct vec3i a)
{
	return vec3i_length_squared((mint_t *)&a);
}

MATHC_INLINE mint_t svec3i_distance(struct vec3i a, struct vec3i b)
{
	return vec3i_distance((mint_t *)&a, (mint_t *)&b);
}

MATHC_INLINE mint_t svec3i_distance_squared(struct vec3i a, struct vec3i b)
{
	return vec3i_distance_squared((mint_t *)&a, (mint_t *)&b);
}

/* Vector 4D */
MATHC_INLINE bool svec4_is_zero(struct vec4 a)
{
	return vec4_is_zero((mfloat_t *)&a);
}

MATHC_INLINE bool svec4_is_near_zero(struct vec4 a, mfloat_t epsilon)
{
	return vec4_is_near_zero((mfloat_t *)&a, epsilon);
}

MATHC_INLINE bool svec4_is_equal(struct vec4 a, struct vec4 b)
{
	return vec4_is_equal((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE bool svec4_is_nearly_equal(struct vec4 a, struct vec4 b, mfloat_t epsilon)
{
	return vec4_is_nearly_equal((mfloat_t *)&a, (mfloat_t *)&b, epsilon);
}

MATHC_INLINE struct vec4 svec4(mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w)
{
	struct vec4 result;
	vec4((mfloat_t *)&result, x, y, z, w);
	return result;
}

MATHC_INLINE struct vec4 svec4_assign(struct vec4 a)
{
	struct vec4 result;
	vec4_assign((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_assign_vec4i(struct vec4i a)
{
	struct vec4 result;
	vec4_assign_vec4i((mfloat_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_zero(void)
{
	struct vec4 result;
	vec4_zero((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct vec4 svec4_one(void)
{
	struct vec4 result;
	vec4_one((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct vec4 svec4_add(struct vec4 a, struct vec4 b)
{
	struct vec4 result;
	vec4_add((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec4 svec4_subtract(struct vec4 a, struct vec4 b)
{
	struct vec4 result;
	vec4_subtract((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec4 svec4_scale(struct vec4 a, mfloat_t scalar)
{
	struct vec4 result;
	vec4_scale((mfloat_t *)&result, (mfloat_t *)&a, scalar);
	return result;
}

MATHC_INLINE struct vec4 svec4_multiply(struct vec4 a, struct vec4 b)
{
	struct vec4 result;
	vec4_multiply((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec4 svec4_multiply_mat4(struct vec4 a, struct mat4 m)
{
	struct vec4 result;
	vec4_multiply_mat4((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct vec4 svec4_divide(struct vec4 a, struct vec4 b)
{
	struct vec4 result;
	vec4_divide((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec4 svec4_snap(struct vec4 a, struct vec4 b)
{
	struct vec4 result;
	vec4_snap((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec4 svec4_negative(struct vec4 a)
{
	struct vec4 result;
	vec4_negative((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_inverse(struct vec4 a)
{
	struct vec4 result;
	vec4_inverse((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_abs(struct vec4 a)
{
	struct vec4 result;
	vec4_abs((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_floor(struct vec4 a)
{
	struct vec4 result;
	vec4_floor((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_ceil(struct vec4 a)
{
	struct vec4 result;
	vec4_ceil((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_round(struct vec4 a)
{
	struct vec4 result;
	vec4_round((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_max(struct vec4 a, struct vec4 b)
{
	struct vec4 result;
	vec4_max((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec4 svec4_min(struct vec4 a, struct vec4 b)
{
	struct vec4 result;
	vec4_min((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct vec4 svec4_clamp(struct vec4 a, struct vec4 lower, struct vec4 higher)
{
	struct vec4 result;
	vec4_clamp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&lower, (mfloat_t *)&higher);
	return result;
}

MATHC_INLINE struct vec4 svec4_normalize(struct vec4 a)
{
	struct vec4 result;
	vec4_normalize((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4 svec4_lerp(struct vec4 a, struct vec4 b, mfloat_t p)
{
	struct vec4 result;
	vec4_lerp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, p);
	return result;
}

/* Vector 4D Integer */
MATHC_INLINE bool svec4i_is_zero(struct vec4i a)
{
	return vec4i_is_zero((mint_t *)&a);
}

MATHC_INLINE bool svec4i_is_equal(struct vec4i a, struct vec4i b)
{
	return vec4i_is_equal((mint_t *)&a, (mint_t *)&b);
}

MATHC_INLINE struct vec4i svec4i(mint_t x, mint_t y, mint_t z, mint_t w)
{
	struct vec4i result;
	vec4i((mint_t *)&result, x, y, z, w);
	return result;
}

MATHC_INLINE struct vec4i svec4i_assign(struct vec4i a)
{
	struct vec4i result;
	vec4i_assign((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_assign_vec4(struct vec4 a)
{
	struct vec4i result;
	vec4i_assign_vec4((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_zero(void)
{
	struct vec4i result;
	vec4i_zero((mint_t *)&result);
	return result;
}

MATHC_INLINE struct vec4i svec4i_one(void)
{
	struct vec4i result;
	vec4i_one((mint_t *)&result);
	return result;
}

MATHC_INLINE struct vec4i svec4i_add(struct vec4i a, struct vec4i b)
{
	struct vec4i result;
	vec4i_add((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec4i svec4i_subtract(struct vec4i a, struct vec4i b)
{
	struct vec4i result;
	vec4i_subtract((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec4i svec4i_scale(struct vec4i a, mint_t scalar)
{
	struct vec4i result;
	vec4i_scale((mint_t *)&result, (mint_t *)&a, scalar);
	return result;
}

MATHC_INLINE struct vec4i svec4i_multiply(struct vec4i a, struct vec4i b)
{
	struct vec4i result;
	vec4i_multiply((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec4i svec4i_multiply_mat4(struct vec4i a, struct mat4 m)
{
	struct vec4i result;
	vec4i_multiply_mat4((mint_t *)&result, (mint_t *)&a, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct vec4i svec4i_divide(struct vec4i a, struct vec4i b)
{
	struct vec4i result;
	vec4i_divide((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec4i svec4i_snap(struct vec4i a, struct vec4i b)
{
	struct vec4i result;
	vec4i_snap((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec4i svec4i_negative(struct vec4i a)
{
	struct vec4i result;
	vec4i_negative((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_inverse(struct vec4i a)
{
	struct vec4i result;
	vec4i_inverse((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_abs(struct vec4i a)
{
	struct vec4i result;
	vec4i_abs((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_floor(struct vec4 a)
{
	struct vec4i result;
	vec4i_floor((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_ceil(struct vec4 a)
{
	struct vec4i result;
	vec4i_ceil((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_round(struct vec4 a)
{
	struct vec4i result;
	vec4i_round((mint_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_max(struct vec4i a, struct vec4i b)
{
	struct vec4i result;
	vec4i_max((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec4i svec4i_min(struct vec4i a, struct vec4i b)
{
	struct vec4i result;
	vec4i_min((mint_t *)&result, (mint_t *)&a, (mint_t *)&b);
	return result;
}

MATHC_INLINE struct vec4i svec4i_clamp(struct vec4i a, struct vec4i lower, struct vec4i higher)
{
	struct vec4i result;
	vec4i_clamp((mint_t *)&result, (mint_t *)&a, (mint_t *)&lower, (mint_t *)&higher);
	return result;
}

MATHC_INLINE struct vec4i svec4i_normalize(struct vec4i a)
{
	struct vec4i result;
	vec4i_normalize((mint_t *)&result, (mint_t *)&a);
	return result;
}

MATHC_INLINE struct vec4i svec4i_lerp(struct vec4i a, struct vec4i b, mint_t p)
{
	struct vec4i result;
	vec4i_lerp((mint_t *)&result, (mint_t *)&a, (mint_t *)&b, p);
	return result;
}

/* Quaternion */
MATHC_INLINE bool squat_is_zero(struct quat a)
{
	return quat_is_zero((mfloat_t *)&a);
}

MATHC_INLINE bool squat_is_near_zero(struct quat a, mfloat_t epsilon)
{
	return quat_is_near_zero((mfloat_t *)&a, epsilon);
}

MATHC_INLINE bool squat_is_equal(struct quat a, struct quat b)
{
	return quat_is_equal((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE bool squat_is_nearly_equal(struct quat a, struct quat b, mfloat_t epsilon)
{
	return quat_is_nearly_equal((mfloat_t *)&a, (mfloat_t *)&b, epsilon);
}

MATHC_INLINE struct quat squat(mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w)
{
	struct quat result;
	quat((mfloat_t *)&result, x, y, z, w);
	return result;
}

MATHC_INLINE struct quat squat_assign(struct quat a)
{
	struct quat result;
	quat_assign((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct quat squat_zero(void)
{
	struct quat result;
	quat_zero((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct quat squat_null(void)
{
	struct quat result;
	quat_null((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct quat squat_scale(struct quat a, mfloat_t scalar)
{
	struct quat result;
	quat_scale((mfloat_t *)&result, (mfloat_t *)&a, scalar);
	return result;
}

MATHC_INLINE struct quat squat_multiply(struct quat a, struct quat b)
{
	struct quat result;
	quat_multiply((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct quat squat_divide(struct quat a, struct quat b)
{
	struct quat result;
	quat_divide((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct quat squat_negative(struct quat a)
{
	struct quat result;
	quat_negative((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct quat squat_conjugate(struct quat a)
{
	struct quat result;
	quat_conjugate((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct quat squat_inverse(struct quat a)
{
	struct quat result;
	quat_inverse((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct quat squat_normalize(struct quat a)
{
	struct quat result;
	quat_normalize((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct quat squat_power(struct quat a, mfloat_t exponent)
{
	struct quat result;
	quat_power((mfloat_t *)&result, (mfloat_t *)&a, exponent);
	return result;
}

MATHC_INLINE struct quat squat_from_axis_angle(struct vec3 a, mfloat_t angle)
{
	struct quat result;
	quat_from_axis_angle((mfloat_t *)&result, (mfloat_t *)&a, angle);
	return result;
}

MATHC_INLINE struct quat squat_from_vec3(struct vec3 a, struct vec3 b)
{
	struct quat result;
	quat_from_vec3((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct quat squat_from_mat4(struct mat4 a)
{
	struct quat result;
	quat_from_mat4((mfloat_t *)&result, (mfloat_t *)&a);
	return result;
}

MATHC_INLINE struct quat squat_from_yaw_pitch_roll(mfloat_t yaw, mfloat_t pitch, mfloat_t roll)
{
	struct quat result;
	quat_from_yaw_pitch_roll((mfloat_t *)&result, yaw, pitch, roll);
	return result;
}

MATHC_INLINE struct quat squat_lerp(struct quat a, struct quat b, mfloat_t p)
{
	struct quat result;
	quat_lerp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, p);
	return result;
}

MATHC_INLINE struct quat squat_slerp(struct quat a, struct quat b, mfloat_t p)
{
	struct quat result;
	quat_slerp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, p);
	return result;
}

MATHC_INLINE mfloat_t squat_dot(struct quat a, struct quat b)
{
	return quat_dot((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE mfloat_t squat_angle(struct quat a, struct quat b)
{
	return quat_angle((mfloat_t *)&a, (mfloat_t *)&b);
}

MATHC_INLINE mfloat_t squat_length(struct quat a)
{
	return quat_length((mfloat_t *)&a);
}

MATHC_INLINE mfloat_t squat_length_squared(struct quat a)
{
	return quat_length_squared((mfloat_t *)&a);
}

/* Matrix 2x2 */
MATHC_INLINE struct mat2 smat2(
	mfloat_t m11, mfloat_t m12,
	mfloat_t m21, mfloat_t m22)
{
	struct mat2 result;
	mat2((mfloat_t *)&result,
		m11, m12,
		m21, m22);
	return result;
}

MATHC_INLINE struct mat2 smat2_zero(void)
{
	struct mat2 result;
	mat2_zero((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct mat2 smat2_identity(void)
{
	struct mat2 result;
	mat2_identity((mfloat_t *)&result);
	return result;
}

MATHC_INLINE mfloat_t smat2_determinant(struct mat2 m)
{
	return mat2_determinant((mfloat_t *)&m);
}

MATHC_INLINE struct mat2 smat2_assign(struct mat2 m)
{
	struct mat2 result;
	mat2_assign((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat2 smat2_assign_mat3(struct mat3 m)
{
	struct mat2 result;
	mat2_assign_mat3((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat2 smat2_assign_mat4(struct mat4 m)
{
	struct mat2 result;
	mat2_assign_mat4((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat2 smat2_transpose(struct mat2 m)
{
	struct mat2 result;
	mat2_transpose((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat2 smat2_cofactor(struct mat2 m)
{
	struct mat2 result;
	mat2_cofactor((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat2 smat2_inverse(struct mat2 m)
{
	struct mat2 result;
	mat2_inverse((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat2 smat2_rotation(mfloat_t angle)
{
	struct mat2 result;
	mat2_rotation((mfloat_t *)&result, angle);
	return result;
}

MATHC_INLINE struct mat2 smat2_scaling(struct mat2 v)
{
	struct mat2 result;
	mat2_scaling((mfloat_t *)&result, (mfloat_t *)&v);
	return result;
}

MATHC_INLINE struct mat2 smat2_negative(struct mat2 m)
{
	struct mat2 result;
	mat2_negative((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat2 smat2_scale(struct mat2 m, mfloat_t s)
{
	struct mat2 result;
	mat2_scale((mfloat_t *)&result, (mfloat_t *)&m, s);
	return result;
}

MATHC_INLINE struct mat2 smat2_multiply(struct mat2 a, struct mat2 b)
{
	struct mat2 result;
	mat2_multiply((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct mat2 smat2_lerp(struct mat2 a, struct mat2 b, mfloat_t p)
{
	struct mat2 result;
	mat2_lerp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, p);
	return result;
}

/* Matrix 3x3 */
MATHC_INLINE struct mat3 smat3(
	mfloat_t m11, mfloat_t m12, mfloat_t m13,
	mfloat_t m21, mfloat_t m22, mfloat_t m23,
	mfloat_t m31, mfloat_t m32, mfloat_t m33)
{
	struct mat3 result;
	mat3((mfloat_t *)&result,
		m11, m12, m13,
		m21, m22, m23,
		m31, m32, m33);
	return result;
}

MATHC_INLINE struct mat3 smat3_zero(void)
{
	struct mat3 result;
	mat3_zero((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct mat3 smat3_identity(void)
{
	struct mat3 result;
	mat3_identity((mfloat_t *)&result);
	return result;
}

MATHC_INLINE mfloat_t smat3_determinant(struct mat3 m)
{
	return mat3_determinant((mfloat_t *)&m);
}

MATHC_INLINE struct mat3 smat3_assign(struct mat3 m)
{
	struct mat3 result;
	mat3_assign((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat3 smat3_assign_mat2(struct mat2 m)
{
	struct mat3 result;
	mat3_assign_mat2((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat3 smat3_assign_mat4(struct mat4 m)
{
	struct mat3 result;
	mat3_assign_mat4((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat3 smat3_transpose(struct mat3 m)
{
	struct mat3 result;
	mat3_transpose((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat3 smat3_cofactor(struct mat3 m)
{
	struct mat3 result;
	mat3_cofactor((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat3 smat3_adjugate(struct mat3 m)
{
	struct mat3 result;
	mat3_adjugate((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat3 smat3_inverse(struct mat3 m)
{
	struct mat3 result;
	mat3_inverse((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat3 smat3_rotation_x(mfloat_t angle)
{
	struct mat3 result;
	mat3_rotation_x((mfloat_t *)&result, angle);
	return result;
}

MATHC_INLINE struct mat3 smat3_rotation_y(mfloat_t angle)
{
	struct mat3 result;
	mat3_rotation_y((mfloat_t *)&result, angle);
	return result;
}

MATHC_INLINE struct mat3 smat3_rotation_z(mfloat_t angle)
{
	struct mat3 result;
	mat3_rotation_z((mfloat_t *)&result, angle);
	return result;
}

MATHC_INLINE struct mat3 smat3_rotation_axis(struct vec3 a, mfloat_t angle)
{
	struct mat3 result;
	mat3_rotation_axis((mfloat_t *)&result, (mfloat_t *)&a, angle);
	return result;
}

MATHC_INLINE struct mat3 smat3_rotation_quaternion(struct quat q)
{
	struct mat3 result;
	mat3_rotation_quaternion((mfloat_t *)&result, (mfloat_t *)&q);
	return result;
}

MATHC_INLINE struct mat3 smat3_scaling(struct mat3 v)
{
	struct mat3 result;
	mat3_scaling((mfloat_t *)&result, (mfloat_t *)&v);
	return result;
}

MATHC_INLINE struct mat3 smat3_negative(struct mat3 m)
{
	struct mat3 result;
	mat3_negative((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat3 smat3_scale(struct mat4 m, mfloat_t s)
{
	struct mat3 result;
	mat3_scale((mfloat_t *)&result, (mfloat_t *)&m, s);
	return result;
}

MATHC_INLINE struct mat3 smat3_multiply(struct mat3 a, struct mat3 b)
{
	struct mat3 result;
	mat3_multiply((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct mat3 smat3_lerp(struct mat3 a, struct mat3 b, mfloat_t p)
{
	struct mat3 result;
	mat3_lerp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, p);
	return result;
}

/* Matrix 4x4 */
MATHC_INLINE struct mat4 smat4(
	mfloat_t m11, mfloat_t m12, mfloat_t m13, mfloat_t m14,
	mfloat_t m21, mfloat_t m22, mfloat_t m23, mfloat_t m24,
	mfloat_t m31, mfloat_t m32, mfloat_t m33, mfloat_t m34,
	mfloat_t m41, mfloat_t m42, mfloat_t m43, mfloat_t m44)
{
	struct mat4 result;
	mat4((mfloat_t *)&result,
		m11, m12, m13, m14,
		m21, m22, m23, m24,
		m31, m32, m33, m34,
		m41, m42, m43, m44);
	return result;
}

MATHC_INLINE struct mat4 smat4_zero(void)
{
	struct mat4 result;
	mat4_zero((mfloat_t *)&result);
	return result;
}

MATHC_INLINE struct mat4 smat4_identity(void)
{
	struct mat4 result;
	mat4_identity((mfloat_t *)&result);
	return result;
}

MATHC_INLINE mfloat_t smat4_determinant(struct mat4 m)
{
	return mat4_determinant((mfloat_t *)&m);
}

MATHC_INLINE struct mat4 smat4_assign(struct mat4 m)
{
	struct mat4 result;
	mat4_assign((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat4 smat4_assign_mat2(struct mat2 m)
{
	struct mat4 result;
	mat4_assign_mat2((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat4 smat4_assign_mat3(struct mat3 m)
{
	struct mat4 result;
	mat4_assign_mat3((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat4 smat4_transpose(struct mat4 m)
{
	struct mat4 result;
	mat4_transpose((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat4 smat4_adjugate(struct mat4 m)
{
	struct mat4 result;
	mat4_adjugate((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat4 smat4_inverse(struct mat4 m)
{
	struct mat4 result;
	mat4_inverse((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat4 smat4_ortho(mfloat_t l, mfloat_t r, mfloat_t b, mfloat_t t, mfloat_t n, mfloat_t f)
{
	struct mat4 result;
	mat4_ortho((mfloat_t *)&result, l, r, b, t, n, f);
	return result;
}

MATHC_INLINE struct mat4 smat4_perspective(mfloat_t fov_y, mfloat_t aspect, mfloat_t n, mfloat_t f)
{
	struct mat4 result;
	mat4_perspective((mfloat_t *)&result, fov_y, aspect, n, f);
	return result;
}

MATHC_INLINE struct mat4 smat4_perspective_fov(mfloat_t fov, mfloat_t w, mfloat_t h, mfloat_t n, mfloat_t f)
{
	struct mat4 result;
	mat4_perspective_fov((mfloat_t *)&result, fov, w, h, n, f);
	return result;
}

MATHC_INLINE struct mat4 smat4_perspective_infinite(mfloat_t fov_y, mfloat_t aspect, mfloat_t n)
{
	struct mat4 result;
	mat4_perspective_infinite((mfloat_t *)&result, fov_y, aspect, n);
	return result;
}

MATHC_INLINE struct mat4 smat4_rotation_x(mfloat_t angle)
{
	struct mat4 result;
	mat4_rotation_x((mfloat_t *)&result, angle);
	return result;
}

MATHC_INLINE struct mat4 smat4_rotation_y(mfloat_t angle)
{
	struct mat4 result;
	mat4_rotation_y((mfloat_t *)&result, angle);
	return result;
}

MATHC_INLINE struct mat4 smat4_rotation_z(mfloat_t angle)
{
	struct mat4 result;
	mat4_rotation_z((mfloat_t *)&result, angle);
	return result;
}

MATHC_INLINE struct mat4 smat4_rotation_axis(struct mat4 a, mfloat_t angle)
{
	struct mat4 result;
	mat4_rotation_axis((mfloat_t *)&result, (mfloat_t *)&a, angle);
	return result;
}

MATHC_INLINE struct mat4 smat4_rotation_quaternion(struct mat4 q)
{
	struct mat4 result;
	mat4_rotation_quaternion((mfloat_t *)&result, (mfloat_t *)&q);
	return result;
}

MATHC_INLINE struct mat4 smat4_look_at(struct vec3 position, struct vec3 target, struct vec3 up_axis)
{
	struct mat4 result;
	mat4_look_at((mfloat_t *)&result, (mfloat_t *)&position, (mfloat_t *)&target, (mfloat_t *)&up_axis);
	return result;
}

MATHC_INLINE struct mat4 smat4_translation(struct mat4 v)
{
	struct mat4 result;
	mat4_translation((mfloat_t *)&result, (mfloat_t *)&v);
	return result;
}

MATHC_INLINE struct mat4 smat4_scaling(struct mat4 v)
{
	struct mat4 result;
	mat4_scaling((mfloat_t *)&result, (mfloat_t *)&v);
	return result;
}

MATHC_INLINE struct mat4 smat4_negative(struct mat4 m)
{
	struct mat4 result;
	mat4_negative((mfloat_t *)&result, (mfloat_t *)&m);
	return result;
}

MATHC_INLINE struct mat4 smat4_scale(struct mat4 m, mfloat_t s)
{
	struct mat4 result;
	mat4_scale((mfloat_t *)&result, (mfloat_t *)&m, s);
	return result;
}

MATHC_INLINE struct mat4 smat4_multiply(struct mat4 a, struct mat4 b)
{
	struct mat4 result;
	mat4_multiply((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b);
	return result;
}

MATHC_INLINE struct mat4 smat4_lerp(struct mat4 a, struct mat4 b, mfloat_t p)
{
	struct mat4 result;
	mat4_lerp((mfloat_t *)&result, (mfloat_t *)&a, (mfloat_t *)&b, p);
	return result;
}
#endif

#ifndef MATHC_NO_EASING_FUNCTIONS
/* Easing functions */
mfloat_t quadratic_ease_in(mfloat_t p);
mfloat_t quadratic_ease_out(mfloat_t p);
mfloat_t quadratic_ease_in_out(mfloat_t p);
mfloat_t cubic_ease_in(mfloat_t p);
mfloat_t cubic_ease_out(mfloat_t p);
mfloat_t cubic_ease_in_out(mfloat_t p);
mfloat_t quartic_ease_in(mfloat_t p);
mfloat_t quartic_ease_out(mfloat_t p);
mfloat_t quartic_ease_in_out(mfloat_t p);
mfloat_t quintic_ease_in(mfloat_t p);
mfloat_t quintic_ease_out(mfloat_t p);
mfloat_t quintic_ease_in_out(mfloat_t p);
mfloat_t sine_ease_in(mfloat_t p);
mfloat_t sine_ease_out(mfloat_t p);
mfloat_t sine_ease_in_out(mfloat_t p);
mfloat_t circular_ease_in(mfloat_t p);
mfloat_t circular_ease_out(mfloat_t p);
mfloat_t circular_ease_in_out(mfloat_t p);
mfloat_t exponential_ease_in(mfloat_t p);
mfloat_t exponential_ease_out(mfloat_t p);
mfloat_t exponential_ease_in_out(mfloat_t p);
mfloat_t elastic_ease_in(mfloat_t p);
mfloat_t elastic_ease_out(mfloat_t p);
mfloat_t elastic_ease_in_out(mfloat_t p);
mfloat_t back_ease_in(mfloat_t p);
mfloat_t back_ease_out(mfloat_t p);
mfloat_t back_ease_in_out(mfloat_t p);
mfloat_t bounce_ease_in(mfloat_t p);
mfloat_t bounce_ease_out(mfloat_t p);
mfloat_t bounce_ease_in_out(mfloat_t p);
#endif

#endif
