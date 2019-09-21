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

#include "mathc.h"

#ifdef __GCWZERO__
#undef MMIN
#define MMIN fmin
#undef MMAX
#define MMAX fmax
#endif

/* Utils */
bool nearly_equal(mfloat_t a, mfloat_t b, mfloat_t epsilon)
{
	bool result = false;
	if (a == b) {
		result = true;
	} else {
		result = MABS(a - b) < epsilon;
	}
	return result;
}

mfloat_t to_radians(mfloat_t degrees)
{
	return degrees * MPI / MFLOAT_C(180.0);
}

mfloat_t to_degrees(mfloat_t radians)
{
	return radians * MFLOAT_C(180.0) / MPI;
}

/* Vector 2D */
bool vec2_is_zero(mfloat_t *a)
{
	return nearly_equal(a[0], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[1], MFLOAT_C(0.0), MFLT_EPSILON);
}

bool vec2_is_near_zero(mfloat_t *a, mfloat_t epsilon)
{
	return nearly_equal(a[0], MFLOAT_C(0.0), epsilon) && nearly_equal(a[1], MFLOAT_C(0.0), epsilon);
}

bool vec2_is_equal(mfloat_t *a, mfloat_t *b)
{
	return nearly_equal(a[0], b[0], MFLT_EPSILON) && nearly_equal(a[1], b[1], MFLT_EPSILON);
}

bool vec2_is_nearly_equal(mfloat_t *a, mfloat_t *b, mfloat_t epsilon)
{
	return nearly_equal(a[0], b[0], epsilon) && nearly_equal(a[1], b[1], epsilon);
}

mfloat_t *vec2(mfloat_t *result, mfloat_t x, mfloat_t y)
{
	result[0] = x;
	result[1] = y;
	return result;
}

mfloat_t *vec2_assign(mfloat_t *result, mfloat_t *a)
{
	result[0] = a[0];
	result[1] = a[1];
	return result;
}

mfloat_t *vec2_assign_vec2i(mfloat_t *result, mint_t *a)
{
	result[0] = (mfloat_t)a[0];
	result[1] = (mfloat_t)a[1];
	return result;
}

mfloat_t *vec2_zero(mfloat_t *result)
{
	result[0] = MFLOAT_C(0.0);
	result[1] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *vec2_one(mfloat_t *result)
{
	result[0] = MFLOAT_C(1.0);
	result[1] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *vec2_add(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] + b[0];
	result[1] = a[1] + b[1];
	return result;
}

mfloat_t *vec2_subtract(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] - b[0];
	result[1] = a[1] - b[1];
	return result;
}

mfloat_t *vec2_scale(mfloat_t *result, mfloat_t *a, mfloat_t scalar)
{
	result[0] = a[0] * scalar;
	result[1] = a[1] * scalar;
	return result;
}

mfloat_t *vec2_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] * b[0];
	result[1] = a[1] * b[1];
	return result;
}

mfloat_t *vec2_multiply_mat2(mfloat_t *result, mfloat_t *a, mfloat_t *m)
{
	mfloat_t x = a[0];
	mfloat_t y = a[1];
	result[0] = m[0] * x + m[2] * y;
	result[1] = m[1] * x + m[3] * y;
	return result;
}

mfloat_t *vec2_divide(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] / b[0];
	result[1] = a[1] / b[1];
	return result;
}

mfloat_t *vec2_snap(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MFLOOR(a[0] / b[0]) * b[0];
	result[1] = MFLOOR(a[1] / b[1]) * b[1];
	return result;
}

mfloat_t *vec2_negative(mfloat_t *result, mfloat_t *a)
{
	result[0] = -a[0];
	result[1] = -a[1];
	return result;
}

mfloat_t *vec2_inverse(mfloat_t *result, mfloat_t *a)
{
	if (!nearly_equal(a[0], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[0] = MFLOAT_C(1.0) / a[0];
	} else {
		result[0] = MFLOAT_C(0.0);
	}
	if (!nearly_equal(a[1], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[1] = MFLOAT_C(1.0) / a[1];
	} else {
		result[1] = MFLOAT_C(0.0);
	}
	return result;
}

mfloat_t *vec2_abs(mfloat_t *result, mfloat_t *a)
{
	result[0] = MABS(a[0]);
	result[1] = MABS(a[1]);
	return result;
}

mfloat_t *vec2_floor(mfloat_t *result, mfloat_t *a)
{
	result[0] = MFLOOR(a[0]);
	result[1] = MFLOOR(a[1]);
	return result;
}

mfloat_t *vec2_ceil(mfloat_t *result, mfloat_t *a)
{
	result[0] = MCEIL(a[0]);
	result[1] = MCEIL(a[1]);
	return result;
}

mfloat_t *vec2_round(mfloat_t *result, mfloat_t *a)
{
	result[0] = MROUND(a[0]);
	result[1] = MROUND(a[1]);
	return result;
}

mfloat_t *vec2_max(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MMAX(a[0], b[0]);
	result[1] = MMAX(a[1], b[1]);
	return result;
}

mfloat_t *vec2_min(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MMIN(a[0], b[0]);
	result[1] = MMIN(a[1], b[1]);
	return result;
}

mfloat_t *vec2_clamp(mfloat_t *result, mfloat_t *a, mfloat_t *lower, mfloat_t *higher)
{
	result[0] = a[0];
	result[1] = a[1];
	if (result[0] < lower[0]) {
		result[0] = lower[0];
	}
	if (result[0] > higher[0]) {
		result[0] = higher[0];
	}
	if (result[1] < lower[1]) {
		result[1] = lower[1];
	}
	if (result[1] > higher[1]) {
		result[1] = higher[1];
	}
	return result;
}

mfloat_t *vec2_normalize(mfloat_t *result, mfloat_t *a)
{
	mfloat_t length = MSQRT(a[0] * a[0] + a[1] * a[1]);
	if (!nearly_equal(length, MFLOAT_C(0.0), MFLT_EPSILON)) {
		length = MFLOAT_C(1.0) / length;
		result[0] = a[0] * length;
		result[1] = a[1] * length;
	} else {
		result[0] = MFLOAT_C(0.0);
		result[1] = MFLOAT_C(0.0);
	}
	return result;
}

mfloat_t *vec2_project(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t s = vec2_dot(a, b) / vec2_dot(b, b);
	result[0] = b[0] * s;
	result[1] = b[1] * s;
	return result;
}

mfloat_t *vec2_slide(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t d = vec2_dot(a, b);
	result[0] = a[0] - b[0] * d;
	result[1] = a[1] - b[1] * d;
	return result;
}

mfloat_t *vec2_reflect(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t d = MFLOAT_C(2.0) * vec2_dot(a, b);
	result[0] = a[0] - b[0] * d;
	result[1] = a[1] - b[1] * d;
	return result;
}

mfloat_t *vec2_tangent(mfloat_t *result, mfloat_t *a)
{
	result[0] = a[1];
	result[1] = -a[0];
	return result;
}

mfloat_t *vec2_rotate(mfloat_t *result, mfloat_t *a, mfloat_t angle)
{
	mfloat_t cs = MCOS(angle);
	mfloat_t sn = MSIN(angle);
	mfloat_t x = a[0];
	mfloat_t y = a[1];
	result[0] = x * cs - y * sn;
	result[1] = x * sn + y * cs;
	return result;
}

mfloat_t *vec2_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p)
{
	result[0] = a[0] + (b[0] - a[0]) * p;
	result[1] = a[1] + (b[1] - a[1]) * p;
	return result;
}

mfloat_t *vec2_bezier3(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t *c, mfloat_t p)
{
	mfloat_t tmp_a[VEC2_SIZE];
	mfloat_t tmp_b[VEC2_SIZE];
	vec2_lerp(tmp_a, a, b, p);
	vec2_lerp(tmp_b, b, c, p);
	vec2_lerp(result, tmp_a, tmp_b, p);
	return result;
}

mfloat_t *vec2_bezier4(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t *c, mfloat_t *d, mfloat_t p)
{
	mfloat_t tmp_a[VEC2_SIZE];
	mfloat_t tmp_b[VEC2_SIZE];
	mfloat_t tmp_c[VEC2_SIZE];
	mfloat_t tmp_d[VEC2_SIZE];
	mfloat_t tmp_e[VEC2_SIZE];
	vec2_lerp(tmp_a, a, b, p);
	vec2_lerp(tmp_b, b, c, p);
	vec2_lerp(tmp_c, c, d, p);
	vec2_lerp(tmp_d, tmp_a, tmp_b, p);
	vec2_lerp(tmp_e, tmp_b, tmp_c, p);
	vec2_lerp(result, tmp_d, tmp_e, p);
	return result;
}

mfloat_t vec2_dot(mfloat_t *a, mfloat_t *b)
{
	return a[0] * b[0] + a[1] * b[1];
}

mfloat_t vec2_angle(mfloat_t *a)
{
	return MATAN2(a[1], a[0]);
}

mfloat_t vec2_length(mfloat_t *a)
{
	return MSQRT(a[0] * a[0] + a[1] * a[1]);
}

mfloat_t vec2_length_squared(mfloat_t *a)
{
	return a[0] * a[0] + a[1] * a[1];
}

mfloat_t vec2_distance(mfloat_t *a, mfloat_t *b)
{
	return MSQRT((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]));
}

mfloat_t vec2_distance_squared(mfloat_t *a, mfloat_t *b)
{
	return (a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]);
}

/* Vector 2D Integer */
bool vec2i_is_zero(mint_t *a)
{
	return a[0] == 0 && a[1] == 0;
}

bool vec2i_is_equal(mint_t *a, mint_t *b)
{
	return a[0] == b[0] && a[1] == b[1];
}

mint_t *vec2i(mint_t *result, mint_t x, mint_t y)
{
	result[0] = x;
	result[1] = y;
	return result;
}

mint_t *vec2i_assign(mint_t *result, mint_t *a)
{
	result[0] = a[0];
	result[1] = a[1];
	return result;
}

mint_t *vec2i_assign_vec2(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)a[0];
	result[1] = (mint_t)a[1];
	return result;
}

mint_t *vec2i_zero(mint_t *result)
{
	result[0] = 0;
	result[1] = 0;
	return result;
}

mint_t *vec2i_one(mint_t *result)
{
	result[0] = 1;
	result[1] = 1;
	return result;
}

mint_t *vec2i_add(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] + b[0];
	result[1] = a[1] + b[1];
	return result;
}

mint_t *vec2i_subtract(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] - b[0];
	result[1] = a[1] - b[1];
	return result;
}

mint_t *vec2i_scale(mint_t *result, mint_t *a, mfloat_t scalar)
{
	result[0] = (mint_t)MROUND(a[0] * scalar);
	result[1] = (mint_t)MROUND(a[1] * scalar);
	return result;
}

mint_t *vec2i_multiply(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] * b[0];
	result[1] = a[1] * b[1];
	return result;
}

mint_t *vec2i_multiply_mat2(mint_t *result, mint_t *a, mfloat_t *m)
{
	mint_t x = a[0];
	mint_t y = a[1];
	result[0] = (mint_t)MROUND(m[0] * x + m[2] * y);
	result[1] = (mint_t)MROUND(m[1] * x + m[3] * y);
	return result;
}

mint_t *vec2i_divide(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] / b[0];
	result[1] = a[1] / b[1];
	return result;
}

mint_t *vec2i_snap(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = (a[0] / b[0]) * b[0];
	result[0] = (a[0] / b[0]) * b[0];
	return result;
}

mint_t *vec2i_negative(mint_t *result, mint_t *a)
{
	result[0] = -a[0];
	result[1] = -a[1];
	return result;
}

mint_t *vec2i_inverse(mint_t *result, mint_t *a)
{
	if (a[0] != 0) {
		result[0] = (mint_t)MROUND(MFLOAT_C(1.0) / a[0]);
	} else {
		result[0] = 0;
	}
	if (a[1] != 0) {
		result[1] = (mint_t)MROUND(MFLOAT_C(1.0) / a[1]);
	} else {
		result[1] = 0;
	}
	return result;
}

mint_t *vec2i_abs(mint_t *result, mint_t *a)
{
	result[0] = a[0];
	if (result[0] < 0) {
		result[0] = -result[0];
	}
	result[1] = a[1];
	if (result[1] < 0) {
		result[1] = -result[1];
	}
	return result;
}

mint_t *vec2i_floor(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MFLOOR(a[0]);
	result[1] = (mint_t)MFLOOR(a[1]);
	return result;
}

mint_t *vec2i_ceil(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MCEIL(a[0]);
	result[1] = (mint_t)MCEIL(a[1]);
	return result;
}

mint_t *vec2i_round(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MROUND(a[0]);
	result[1] = (mint_t)MROUND(a[1]);
	return result;
}

mint_t *vec2i_max(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = MMAXI(a[0], b[0]);
	result[1] = MMAXI(a[1], b[1]);
	return result;
}

mint_t *vec2i_min(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = MMINI(a[0], b[0]);
	result[1] = MMINI(a[1], b[1]);
	return result;
}

mint_t *vec2i_clamp(mint_t *result, mint_t *a, mint_t *lower, mint_t *higher)
{
	result[0] = a[0];
	result[1] = a[1];
	if (result[0] < lower[0]) {
		result[0] = lower[0];
	}
	if (result[0] > higher[0]) {
		result[0] = higher[0];
	}
	if (result[1] < lower[1]) {
		result[1] = lower[1];
	}
	if (result[1] > higher[1]) {
		result[1] = higher[1];
	}
	return result;
}

mint_t *vec2i_normalize(mint_t *result, mint_t *a)
{
	mfloat_t length = MSQRT((mfloat_t)(a[0] * a[0] + a[1] * a[1]));
	if (!nearly_equal(length, MFLOAT_C(0.0), MFLT_EPSILON)) {
		length = MFLOAT_C(1.0) / length;
		result[0] = (mint_t)MROUND(a[0] * length);
		result[1] = (mint_t)MROUND(a[1] * length);
	} else {
		result[0] = 0;
		result[1] = 0;
	}
	return result;
}

mint_t *vec2i_project(mint_t *result, mint_t *a, mint_t *b)
{
	mfloat_t s = (mfloat_t)vec2i_dot(a, b) / vec2i_dot(b, b);
	result[0] = (mint_t)MROUND(b[0] * s);
	result[1] = (mint_t)MROUND(b[1] * s);
	return result;
}

mint_t *vec2i_slide(mint_t *result, mint_t *a, mint_t *b)
{
	mfloat_t d = (mfloat_t)vec2i_dot(a, b);
	result[0] = a[0] - (mint_t)MROUND(b[0] * d);
	result[1] = a[1] - (mint_t)MROUND(b[1] * d);
	return result;
}

mint_t *vec2i_reflect(mint_t *result, mint_t *a, mint_t *b)
{
	mfloat_t d = MFLOAT_C(2.0) * vec2i_dot(a, b);
	result[0] = a[0] - (mint_t)MROUND(b[0] * d);
	result[1] = a[1] - (mint_t)MROUND(b[1] * d);
	return result;
}

mint_t *vec2i_tangent(mint_t *result, mint_t *a)
{
	result[0] = a[1];
	result[1] = -a[0];
	return result;
}

mint_t *vec2i_rotate(mint_t *result, mint_t *a, mfloat_t angle)
{
	mfloat_t cs = MCOS(angle);
	mfloat_t sn = MSIN(angle);
	mint_t x = a[0];
	mint_t y = a[1];
	result[0] = (mint_t)MROUND(x * cs - y * sn);
	result[1] = (mint_t)MROUND(x * sn + y * cs);
	return result;
}

mint_t *vec2i_lerp(mint_t *result, mint_t *a, mint_t *b, mfloat_t p)
{
	result[0] = a[0] + (mint_t)MROUND((b[0] - a[0]) * p);
	result[1] = a[1] + (mint_t)MROUND((b[1] - a[1]) * p);
	return result;
}

mint_t *vec2i_bezier3(mint_t *result, mint_t *a, mint_t *b, mint_t *c, mfloat_t p)
{
	mint_t tmp_a[VEC2_SIZE];
	mint_t tmp_b[VEC2_SIZE];
	vec2i_lerp(tmp_a, a, b, p);
	vec2i_lerp(tmp_b, b, c, p);
	vec2i_lerp(result, tmp_a, tmp_b, p);
	return result;
}

mint_t *vec2i_bezier4(mint_t *result, mint_t *a, mint_t *b, mint_t *c, mint_t *d, mfloat_t p)
{
	mint_t tmp_a[VEC2_SIZE];
	mint_t tmp_b[VEC2_SIZE];
	mint_t tmp_c[VEC2_SIZE];
	mint_t tmp_d[VEC2_SIZE];
	mint_t tmp_e[VEC2_SIZE];
	vec2i_lerp(tmp_a, a, b, p);
	vec2i_lerp(tmp_b, b, c, p);
	vec2i_lerp(tmp_c, c, d, p);
	vec2i_lerp(tmp_d, tmp_a, tmp_b, p);
	vec2i_lerp(tmp_e, tmp_b, tmp_c, p);
	vec2i_lerp(result, tmp_d, tmp_e, p);
	return result;
}

mint_t vec2i_dot(mint_t *a, mint_t *b)
{
	return a[0] * b[0] + a[1] * b[1];
}

mfloat_t vec2i_angle(mint_t *a)
{
	return MATAN2((mfloat_t)a[1], (mfloat_t)a[0]);
}

mfloat_t vec2i_length(mint_t *a)
{
	return MSQRT((mfloat_t)a[0] * a[0] + a[1] * a[1]);
}

mint_t vec2i_length_squared(mint_t *a)
{
	return a[0] * a[0] + a[1] * a[1];
}

mfloat_t vec2i_distance(mint_t *a, mint_t *b)
{
	return MSQRT((mfloat_t)(a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]));
}

mint_t vec2i_distance_squared(mint_t *a, mint_t *b)
{
	return (a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]);
}

/* Vector 3D */
bool vec3_is_zero(mfloat_t *a)
{
	return nearly_equal(a[0], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[1], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[2], MFLOAT_C(0.0), MFLT_EPSILON);
}

bool vec3_is_near_zero(mfloat_t *a, mfloat_t epsilon)
{
	return nearly_equal(a[0], MFLOAT_C(0.0), epsilon) && nearly_equal(a[1], MFLOAT_C(0.0), epsilon) && nearly_equal(a[2], MFLOAT_C(0.0), epsilon);
}

bool vec3_is_equal(mfloat_t *a, mfloat_t *b)
{
	return nearly_equal(a[0], b[0], MFLT_EPSILON) && nearly_equal(a[1], b[1], MFLT_EPSILON) && nearly_equal(a[2], b[2], MFLT_EPSILON);
}

bool vec3_is_nearly_equal(mfloat_t *a, mfloat_t *b, mfloat_t epsilon)
{
	return nearly_equal(a[0], b[0], epsilon) && nearly_equal(a[1], b[1], epsilon) && nearly_equal(a[2], b[2], epsilon);
}

mfloat_t *vec3(mfloat_t *result, mfloat_t x, mfloat_t y, mfloat_t z)
{
	result[0] = x;
	result[1] = y;
	result[2] = z;
	return result;
}

mfloat_t *vec3_assign(mfloat_t *result, mfloat_t *a)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	return result;
}

mfloat_t *vec3_assign_vec3i(mfloat_t *result, mint_t *a)
{
	result[0] = (mfloat_t)a[0];
	result[1] = (mfloat_t)a[1];
	result[2] = (mfloat_t)a[2];
	return result;
}

mfloat_t *vec3_zero(mfloat_t *result)
{
	result[0] = MFLOAT_C(0.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *vec3_one(mfloat_t *result)
{
	result[0] = MFLOAT_C(1.0);
	result[1] = MFLOAT_C(1.0);
	result[2] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *vec3_add(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] + b[0];
	result[1] = a[1] + b[1];
	result[2] = a[2] + b[2];
	return result;
}

mfloat_t *vec3_subtract(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] - b[0];
	result[1] = a[1] - b[1];
	result[2] = a[2] - b[2];
	return result;
}

mfloat_t *vec3_scale(mfloat_t *result, mfloat_t *a, mfloat_t scalar)
{
	result[0] = a[0] * scalar;
	result[1] = a[1] * scalar;
	result[2] = a[2] * scalar;
	return result;
}

mfloat_t *vec3_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] * b[0];
	result[1] = a[1] * b[1];
	result[2] = a[2] * b[2];
	return result;
}

mfloat_t *vec3_multiply_mat3(mfloat_t *result, mfloat_t *a, mfloat_t *m)
{
	mfloat_t x = a[0];
	mfloat_t y = a[1];
	mfloat_t z = a[2];
	result[0] = m[0] * x + m[3] * y + m[6] * z;
	result[1] = m[1] * x + m[4] * y + m[7] * z;
	result[2] = m[2] * x + m[5] * y + m[8] * z;
	return result;
}

mfloat_t *vec3_divide(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] / b[0];
	result[1] = a[1] / b[1];
	result[2] = a[2] / b[2];
	return result;
}

mfloat_t *vec3_snap(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MFLOOR(a[0] / b[0]) * b[0];
	result[1] = MFLOOR(a[1] / b[1]) * b[1];
	result[2] = MFLOOR(a[2] / b[2]) * b[2];
	return result;
}

mfloat_t *vec3_negative(mfloat_t *result, mfloat_t *a)
{
	result[0] = -a[0];
	result[1] = -a[1];
	result[2] = -a[2];
	return result;
}

mfloat_t *vec3_inverse(mfloat_t *result, mfloat_t *a)
{
	if (!nearly_equal(a[0], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[0] = MFLOAT_C(1.0) / a[0];
	} else {
		result[0] = MFLOAT_C(0.0);
	}
	if (!nearly_equal(a[1], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[1] = MFLOAT_C(1.0) / a[1];
	} else {
		result[1] = MFLOAT_C(0.0);
	}
	if (!nearly_equal(a[2], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[2] = MFLOAT_C(1.0) / a[2];
	} else {
		result[2] = MFLOAT_C(0.0);
	}
	return result;
}

mfloat_t *vec3_abs(mfloat_t *result, mfloat_t *a)
{
	result[0] = MABS(a[0]);
	result[1] = MABS(a[1]);
	result[2] = MABS(a[2]);
	return result;
}

mfloat_t *vec3_floor(mfloat_t *result, mfloat_t *a)
{
	result[0] = MFLOOR(a[0]);
	result[1] = MFLOOR(a[1]);
	result[2] = MFLOOR(a[2]);
	return result;
}

mfloat_t *vec3_ceil(mfloat_t *result, mfloat_t *a)
{
	result[0] = MCEIL(a[0]);
	result[1] = MCEIL(a[1]);
	result[2] = MCEIL(a[2]);
	return result;
}

mfloat_t *vec3_round(mfloat_t *result, mfloat_t *a)
{
	result[0] = MROUND(a[0]);
	result[1] = MROUND(a[1]);
	result[2] = MROUND(a[2]);
	return result;
}

mfloat_t *vec3_max(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MMAX(a[0], b[0]);
	result[1] = MMAX(a[1], b[1]);
	result[2] = MMAX(a[2], b[2]);
	return result;
}

mfloat_t *vec3_min(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MMIN(a[0], b[0]);
	result[1] = MMIN(a[1], b[1]);
	result[2] = MMIN(a[2], b[2]);
	return result;
}

mfloat_t *vec3_clamp(mfloat_t *result, mfloat_t *a, mfloat_t *lower, mfloat_t *higher)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	if (result[0] < lower[0]) {
		result[0] = lower[0];
	}
	if (result[0] > higher[0]) {
		result[0] = higher[0];
	}
	if (result[1] < lower[1]) {
		result[1] = lower[1];
	}
	if (result[1] > higher[1]) {
		result[1] = higher[1];
	}
	if (result[2] < lower[2]) {
		result[2] = lower[2];
	}
	if (result[2] > higher[2]) {
		result[2] = higher[2];
	}
	return result;
}

mfloat_t *vec3_cross(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t cross[VEC3_SIZE];
	cross[0] = a[1] * b[2] - a[2] * b[1];
	cross[1] = a[2] * b[0] - a[0] * b[2];
	cross[2] = a[0] * b[1] - a[1] * b[0];
	result[0] = cross[0];
	result[1] = cross[1];
	result[2] = cross[2];
	return result;
}

mfloat_t *vec3_normalize(mfloat_t *result, mfloat_t *a)
{
	mfloat_t length = MSQRT(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	if (!nearly_equal(length, MFLOAT_C(0.0), MFLT_EPSILON)) {
		length = MFLOAT_C(1.0) / length;
		result[0] = a[0] * length;
		result[1] = a[1] * length;
		result[2] = a[2] * length;
	} else {
		result[0] = MFLOAT_C(0.0);
		result[1] = MFLOAT_C(0.0);
		result[2] = MFLOAT_C(0.0);
	}
	return result;
}

mfloat_t *vec3_project(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t s = vec3_dot(a, b) / vec3_dot(b, b);
	result[0] = b[0] * s;
	result[1] = b[1] * s;
	result[2] = b[2] * s;
	return result;
}

mfloat_t *vec3_slide(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t d = vec3_dot(a, b);
	result[0] = a[0] - b[0] * d;
	result[1] = a[1] - b[1] * d;
	result[2] = a[2] - b[2] * d;
	return result;
}

mfloat_t *vec3_reflect(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t d = MFLOAT_C(2.0) * vec3_dot(a, b);
	result[0] = a[0] - b[0] * d;
	result[1] = a[1] - b[1] * d;
	result[2] = a[2] - b[2] * d;
	return result;
}

mfloat_t *vec3_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p)
{
	result[0] = a[0] + (b[0] - a[0]) * p;
	result[1] = a[1] + (b[1] - a[1]) * p;
	result[2] = a[2] + (b[2] - a[2]) * p;
	return result;
}

mfloat_t *vec3_bezier3(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t *c, mfloat_t p)
{
	mfloat_t tmp_a[VEC3_SIZE];
	mfloat_t tmp_b[VEC3_SIZE];
	vec3_lerp(tmp_a, a, b, p);
	vec3_lerp(tmp_b, b, c, p);
	vec3_lerp(result, tmp_a, tmp_b, p);
	return result;
}

mfloat_t *vec3_bezier4(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t *c, mfloat_t *d, mfloat_t p)
{
	mfloat_t tmp_a[VEC3_SIZE];
	mfloat_t tmp_b[VEC3_SIZE];
	mfloat_t tmp_c[VEC3_SIZE];
	mfloat_t tmp_d[VEC3_SIZE];
	mfloat_t tmp_e[VEC3_SIZE];
	vec3_lerp(tmp_a, a, b, p);
	vec3_lerp(tmp_b, b, c, p);
	vec3_lerp(tmp_c, c, d, p);
	vec3_lerp(tmp_d, tmp_a, tmp_b, p);
	vec3_lerp(tmp_e, tmp_b, tmp_c, p);
	vec3_lerp(result, tmp_d, tmp_e, p);
	return result;
}

mfloat_t vec3_dot(mfloat_t *a, mfloat_t *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

mfloat_t vec3_length(mfloat_t *a)
{
	return MSQRT(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

mfloat_t vec3_length_squared(mfloat_t *a)
{
	return a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
}

mfloat_t vec3_distance(mfloat_t *a, mfloat_t *b)
{
	return MSQRT((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

mfloat_t vec3_distance_squared(mfloat_t *a, mfloat_t *b)
{
	return (a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]);
}

/* Vector 3D Integer */
bool vec3i_is_zero(mint_t *a)
{
	return a[0] == 0 && a[1] == 0;
}

bool vec3i_is_equal(mint_t *a, mint_t *b)
{
	return a[0] == b[0] && a[1] == b[1];
}

mint_t *vec3i(mint_t *result, mint_t x, mint_t y, mint_t z)
{
	result[0] = x;
	result[1] = y;
	result[2] = z;
	return result;
}

mint_t *vec3i_assign(mint_t *result, mint_t *a)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	return result;
}

mint_t *vec3i_assign_vec3(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)a[0];
	result[1] = (mint_t)a[1];
	result[2] = (mint_t)a[2];
	return result;
}

mint_t *vec3i_zero(mint_t *result)
{
	result[0] = 0;
	result[1] = 0;
	result[2] = 0;
	return result;
}

mint_t *vec3i_one(mint_t *result)
{
	result[0] = 1;
	result[1] = 1;
	result[2] = 1;
	return result;
}

mint_t *vec3i_add(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] + b[0];
	result[1] = a[1] + b[1];
	result[2] = a[2] + b[2];
	return result;
}

mint_t *vec3i_subtract(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] - b[0];
	result[1] = a[1] - b[1];
	result[2] = a[2] - b[2];
	return result;
}

mint_t *vec3i_scale(mint_t *result, mint_t *a, mfloat_t scalar)
{
	result[0] = (mint_t)MROUND(a[0] * scalar);
	result[1] = (mint_t)MROUND(a[1] * scalar);
	result[2] = (mint_t)MROUND(a[2] * scalar);
	return result;
}

mint_t *vec3i_multiply(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] * b[0];
	result[1] = a[1] * b[1];
	result[2] = a[2] * b[2];
	return result;
}

mint_t *vec3i_multiply_mat3(mint_t *result, mint_t *a, mfloat_t *m)
{
	mint_t x = a[0];
	mint_t y = a[1];
	mint_t z = a[2];
	result[0] = (mint_t)MROUND(m[0] * x + m[3] * y + m[6] * z);
	result[1] = (mint_t)MROUND(m[1] * x + m[4] * y + m[7] * z);
	result[2] = (mint_t)MROUND(m[2] * x + m[5] * y + m[8] * z);
	return result;
}

mint_t *vec3i_divide(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] / b[0];
	result[1] = a[1] / b[1];
	result[2] = a[2] / b[2];
	return result;
}

mint_t *vec3i_snap(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = (a[0] / b[0]) * b[0];
	result[1] = (a[1] / b[1]) * b[1];
	result[2] = (a[2] / b[2]) * b[2];
	return result;
}

mint_t *vec3i_negative(mint_t *result, mint_t *a)
{
	result[0] = -a[0];
	result[1] = -a[1];
	result[2] = -a[2];
	return result;
}

mint_t *vec3i_inverse(mint_t *result, mint_t *a)
{
	if (a[0] != 0) {
		result[0] = (mint_t)MROUND(MFLOAT_C(1.0) / (mfloat_t)a[0]);
	} else {
		result[0] = 0;
	}
	if (a[1] != 0) {
		result[1] = (mint_t)MROUND(MFLOAT_C(1.0) / (mfloat_t)a[1]);
	} else {
		result[1] = 0;
	}
	if (a[2] != 0) {
		result[2] = (mint_t)MROUND(MFLOAT_C(1.0) / (mfloat_t)a[2]);
	} else {
		result[2] = 0;
	}
	return result;
}

mint_t *vec3i_abs(mint_t *result, mint_t *a)
{
	result[0] = a[0];
	if (result[0] < 0) {
		result[0] = -result[0];
	}
	result[1] = a[1];
	if (result[1] < 0) {
		result[1] = -result[1];
	}
	result[2] = a[2];
	if (result[2] < 0) {
		result[2] = -result[2];
	}
	return result;
}

mint_t *vec3i_floor(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MFLOOR(a[0]);
	result[1] = (mint_t)MFLOOR(a[1]);
	result[2] = (mint_t)MFLOOR(a[2]);
	return result;
}

mint_t *vec3i_ceil(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MCEIL(a[0]);
	result[1] = (mint_t)MCEIL(a[1]);
	result[2] = (mint_t)MCEIL(a[2]);
	return result;
}

mint_t *vec3i_round(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MROUND(a[0]);
	result[1] = (mint_t)MROUND(a[1]);
	result[2] = (mint_t)MROUND(a[2]);
	return result;
}

mint_t *vec3i_max(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = MMAXI(a[0], b[0]);
	result[1] = MMAXI(a[1], b[1]);
	result[2] = MMAXI(a[2], b[2]);
	return result;
}

mint_t *vec3i_min(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = MMINI(a[0], b[0]);
	result[1] = MMINI(a[1], b[1]);
	result[2] = MMINI(a[2], b[2]);
	return result;
}

mint_t *vec3i_clamp(mint_t *result, mint_t *a, mint_t *lower, mint_t *higher)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	if (result[0] < lower[0]) {
		result[0] = lower[0];
	}
	if (result[0] > higher[0]) {
		result[0] = higher[0];
	}
	if (result[1] < lower[1]) {
		result[1] = lower[1];
	}
	if (result[1] > higher[1]) {
		result[1] = higher[1];
	}
	if (result[2] < lower[2]) {
		result[2] = lower[2];
	}
	if (result[2] > higher[2]) {
		result[2] = higher[2];
	}
	return result;
}

mint_t *vec3i_cross(mint_t *result, mint_t *a, mint_t *b)
{
	mint_t cross[VEC3_SIZE];
	cross[0] = a[1] * b[2] - a[2] * b[1];
	cross[1] = a[2] * b[0] - a[0] * b[2];
	cross[2] = a[0] * b[1] - a[1] * b[0];
	result[0] = cross[0];
	result[1] = cross[1];
	result[2] = cross[2];
	return result;
}

mint_t *vec3i_normalize(mint_t *result, mint_t *a)
{
	mfloat_t length = MSQRT((mfloat_t)a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	if (!nearly_equal(length, MFLOAT_C(0.0), MFLT_EPSILON)) {
		length = MFLOAT_C(1.0) / length;
		result[0] = (mint_t)MROUND(a[0] * length);
		result[1] = (mint_t)MROUND(a[1] * length);
		result[2] = (mint_t)MROUND(a[2] * length);
	} else {
		result[0] = 0;
		result[1] = 0;
		result[2] = 0;
	}
	return result;
}

mint_t *vec3i_project(mint_t *result, mint_t *a, mint_t *b)
{
	mfloat_t s = (mfloat_t)vec3i_dot(a, b) / vec3i_dot(b, b);
	result[0] = (mint_t)MROUND(b[0] * s);
	result[1] = (mint_t)MROUND(b[1] * s);
	result[2] = (mint_t)MROUND(b[2] * s);
	return result;
}

mint_t *vec3i_slide(mint_t *result, mint_t *a, mint_t *b)
{
	mfloat_t d = (mfloat_t)vec3i_dot(a, b);
	result[0] = (mint_t)MROUND(a[0] - b[0] * d);
	result[1] = (mint_t)MROUND(a[1] - b[1] * d);
	result[2] = (mint_t)MROUND(a[2] - b[2] * d);
	return result;
}

mint_t *vec3i_reflect(mint_t *result, mint_t *a, mint_t *b)
{
	mfloat_t d = MFLOAT_C(2.0) * vec3i_dot(a, b);
	result[0] = a[0] - (mint_t)MROUND(b[0] * d);
	result[1] = a[1] - (mint_t)MROUND(b[1] * d);
	result[2] = a[2] - (mint_t)MROUND(b[2] * d);
	return result;
}

mint_t *vec3i_lerp(mint_t *result, mint_t *a, mint_t *b, mfloat_t p)
{
	result[0] = a[0] + (mint_t)MROUND((b[0] - a[0]) * p);
	result[1] = a[1] + (mint_t)MROUND((b[1] - a[1]) * p);
	result[2] = a[2] + (mint_t)MROUND((b[2] - a[2]) * p);
	return result;
}

mint_t *vec3i_bezier3(mint_t *result, mint_t *a, mint_t *b, mint_t *c, mfloat_t p)
{
	mint_t tmp_a[VEC3_SIZE];
	mint_t tmp_b[VEC3_SIZE];
	vec3i_lerp(tmp_a, a, b, p);
	vec3i_lerp(tmp_b, b, c, p);
	vec3i_lerp(result, tmp_a, tmp_b, p);
	return result;
}

mint_t *vec3i_bezier4(mint_t *result, mint_t *a, mint_t *b, mint_t *c, mint_t *d, mfloat_t p)
{
	mint_t tmp_a[VEC3_SIZE];
	mint_t tmp_b[VEC3_SIZE];
	mint_t tmp_c[VEC3_SIZE];
	mint_t tmp_d[VEC3_SIZE];
	mint_t tmp_e[VEC3_SIZE];
	vec3i_lerp(tmp_a, a, b, p);
	vec3i_lerp(tmp_b, b, c, p);
	vec3i_lerp(tmp_c, c, d, p);
	vec3i_lerp(tmp_d, tmp_a, tmp_b, p);
	vec3i_lerp(tmp_e, tmp_b, tmp_c, p);
	vec3i_lerp(result, tmp_d, tmp_e, p);
	return result;
}

mint_t vec3i_dot(mint_t *a, mint_t *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

mfloat_t vec3i_length(mint_t *a)
{
	return MSQRT((mfloat_t)a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

mint_t vec3i_length_squared(mint_t *a)
{
	return a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
}

mfloat_t vec3i_distance(mint_t *a, mint_t *b)
{
	return MSQRT((mfloat_t)(a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

mint_t vec3i_distance_squared(mint_t *a, mint_t *b)
{
	return (a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]);
}

/* Vector 4D */
bool vec4_is_zero(mfloat_t *a)
{
	return nearly_equal(a[0], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[1], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[2], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[3], MFLOAT_C(0.0), MFLT_EPSILON);
}

bool vec4_is_near_zero(mfloat_t *a, mfloat_t epsilon)
{
	return nearly_equal(a[0], MFLOAT_C(0.0), epsilon) && nearly_equal(a[1], MFLOAT_C(0.0), epsilon) && nearly_equal(a[2], MFLOAT_C(0.0), epsilon) && nearly_equal(a[3], MFLOAT_C(0.0), epsilon);
}

bool vec4_is_equal(mfloat_t *a, mfloat_t *b)
{
	return nearly_equal(a[0], b[0], MFLT_EPSILON) && nearly_equal(a[1], b[1], MFLT_EPSILON) && nearly_equal(a[2], b[2], MFLT_EPSILON) && nearly_equal(a[3], b[3], MFLT_EPSILON);
}

bool vec4_is_nearly_equal(mfloat_t *a, mfloat_t *b, mfloat_t epsilon)
{
	return nearly_equal(a[0], b[0], epsilon) && nearly_equal(a[1], b[1], epsilon) && nearly_equal(a[2], b[2], epsilon) && nearly_equal(a[3], b[3], epsilon);
}

mfloat_t *vec4(mfloat_t *result, mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w)
{
	result[0] = x;
	result[1] = y;
	result[2] = z;
	result[3] = w;
	return result;
}

mfloat_t *vec4_assign(mfloat_t *result, mfloat_t *a)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	result[3] = a[3];
	return result;
}

mfloat_t *vec4_assign_vec4i(mfloat_t *result, mint_t *a)
{
	result[0] = (mfloat_t)a[0];
	result[1] = (mfloat_t)a[1];
	result[2] = (mfloat_t)a[2];
	result[3] = (mfloat_t)a[3];
	return result;
}

mfloat_t *vec4_zero(mfloat_t *result)
{
	result[0] = MFLOAT_C(0.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *vec4_one(mfloat_t *result)
{
	result[0] = MFLOAT_C(1.0);
	result[1] = MFLOAT_C(1.0);
	result[2] = MFLOAT_C(1.0);
	result[3] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *vec4_add(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] + b[0];
	result[1] = a[1] + b[1];
	result[2] = a[2] + b[2];
	result[3] = a[3] + b[3];
	return result;
}

mfloat_t *vec4_subtract(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] - b[0];
	result[1] = a[1] - b[1];
	result[2] = a[2] - b[2];
	result[3] = a[3] - b[3];
	return result;
}

mfloat_t *vec4_scale(mfloat_t *result, mfloat_t *a, mfloat_t scalar)
{
	result[0] = a[0] * scalar;
	result[1] = a[1] * scalar;
	result[2] = a[2] * scalar;
	result[3] = a[3] * scalar;
	return result;
}

mfloat_t *vec4_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] * b[0];
	result[1] = a[1] * b[1];
	result[2] = a[2] * b[2];
	result[3] = a[3] * b[3];
	return result;
}

mfloat_t *vec4_multiply_mat4(mfloat_t *result, mfloat_t *a, mfloat_t *m)
{
	mfloat_t x = a[0];
	mfloat_t y = a[1];
	mfloat_t z = a[2];
	mfloat_t w = a[3];
	result[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
	result[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
	result[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
	result[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
	return result;
}

mfloat_t *vec4_divide(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[0] / b[0];
	result[1] = a[1] / b[1];
	result[2] = a[2] / b[2];
	result[3] = a[3] / b[3];
	return result;
}

mfloat_t *vec4_snap(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MFLOOR(a[0] / b[0]) * b[0];
	result[1] = MFLOOR(a[1] / b[1]) * b[1];
	result[2] = MFLOOR(a[2] / b[2]) * b[2];
	result[3] = MFLOOR(a[3] / b[3]) * b[3];
	return result;
}

mfloat_t *vec4_negative(mfloat_t *result, mfloat_t *a)
{
	result[0] = -a[0];
	result[1] = -a[1];
	result[2] = -a[2];
	result[3] = -a[3];
	return result;
}

mfloat_t *vec4_inverse(mfloat_t *result, mfloat_t *a)
{
	if (!nearly_equal(a[0], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[0] = MFLOAT_C(1.0) / a[0];
	} else {
		result[0] = MFLOAT_C(0.0);
	}
	if (!nearly_equal(a[1], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[1] = MFLOAT_C(1.0) / a[1];
	} else {
		result[1] = MFLOAT_C(0.0);
	}
	if (!nearly_equal(a[2], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[2] = MFLOAT_C(1.0) / a[2];
	} else {
		result[2] = MFLOAT_C(0.0);
	}
	if (!nearly_equal(a[3], MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[3] = MFLOAT_C(1.0) / a[3];
	} else {
		result[3] = MFLOAT_C(0.0);
	}
	return result;
}

mfloat_t *vec4_abs(mfloat_t *result, mfloat_t *a)
{
	result[0] = MABS(a[0]);
	result[1] = MABS(a[1]);
	result[2] = MABS(a[2]);
	result[3] = MABS(a[3]);
	return result;
}

mfloat_t *vec4_floor(mfloat_t *result, mfloat_t *a)
{
	result[0] = MFLOOR(a[0]);
	result[1] = MFLOOR(a[1]);
	result[2] = MFLOOR(a[2]);
	result[3] = MFLOOR(a[3]);
	return result;
}

mfloat_t *vec4_ceil(mfloat_t *result, mfloat_t *a)
{
	result[0] = MCEIL(a[0]);
	result[1] = MCEIL(a[1]);
	result[2] = MCEIL(a[2]);
	result[3] = MCEIL(a[3]);
	return result;
}

mfloat_t *vec4_round(mfloat_t *result, mfloat_t *a)
{
	result[0] = MROUND(a[0]);
	result[1] = MROUND(a[1]);
	result[2] = MROUND(a[2]);
	result[3] = MROUND(a[3]);
	return result;
}

mfloat_t *vec4_max(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MMAX(a[0], b[0]);
	result[1] = MMAX(a[1], b[1]);
	result[2] = MMAX(a[2], b[2]);
	result[3] = MMAX(a[3], b[3]);
	return result;
}

mfloat_t *vec4_min(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = MMIN(a[0], b[0]);
	result[1] = MMIN(a[1], b[1]);
	result[2] = MMIN(a[2], b[2]);
	result[3] = MMIN(a[3], b[3]);
	return result;
}

mfloat_t *vec4_clamp(mfloat_t *result, mfloat_t *a, mfloat_t *lower, mfloat_t *higher)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	result[3] = a[3];
	if (result[0] < lower[0]) {
		result[0] = lower[0];
	}
	if (result[0] > higher[0]) {
		result[0] = higher[0];
	}
	if (result[1] < lower[1]) {
		result[1] = lower[1];
	}
	if (result[1] > higher[1]) {
		result[1] = higher[1];
	}
	if (result[2] < lower[2]) {
		result[2] = lower[2];
	}
	if (result[2] > higher[2]) {
		result[2] = higher[2];
	}
	if (result[3] < lower[3]) {
		result[3] = lower[3];
	}
	if (result[3] > higher[3]) {
		result[3] = higher[3];
	}
	return result;
}

mfloat_t *vec4_normalize(mfloat_t *result, mfloat_t *a)
{
	mfloat_t length = MSQRT(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3]);
	if (!nearly_equal(length, MFLOAT_C(0.0), MFLT_EPSILON)) {
		length = MFLOAT_C(1.0) / length;
		result[0] = a[0] * length;
		result[1] = a[1] * length;
		result[2] = a[2] * length;
		result[3] = a[3] * length;
	} else {
		result[0] = MFLOAT_C(0.0);
		result[1] = MFLOAT_C(0.0);
		result[2] = MFLOAT_C(0.0);
		result[3] = MFLOAT_C(0.0);
	}
	return result;
}

mfloat_t *vec4_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p)
{
	result[0] = a[0] + (b[0] - a[0]) * p;
	result[1] = a[1] + (b[1] - a[1]) * p;
	result[2] = a[2] + (b[2] - a[2]) * p;
	result[3] = a[3] + (b[3] - a[3]) * p;
	return result;
}

/* Vector 4D Integer */
bool vec4i_is_zero(mint_t *a)
{
	return a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0;
}

bool vec4i_is_equal(mint_t *a, mint_t *b)
{
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

mint_t *vec4i(mint_t *result, mint_t x, mint_t y, mint_t z, mint_t w)
{
	result[0] = x;
	result[1] = y;
	result[2] = z;
	result[3] = w;
	return result;
}

mint_t *vec4i_assign(mint_t *result, mint_t *a)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	result[3] = a[3];
	return result;
}

mint_t *vec4i_assign_vec4(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MROUND(a[0]);
	result[1] = (mint_t)MROUND(a[1]);
	result[2] = (mint_t)MROUND(a[2]);
	result[3] = (mint_t)MROUND(a[3]);
	return result;
}

mint_t *vec4i_zero(mint_t *result)
{
	result[0] = 0;
	result[1] = 0;
	result[2] = 0;
	result[3] = 0;
	return result;
}

mint_t *vec4i_one(mint_t *result)
{
	result[0] = 1;
	result[1] = 1;
	result[2] = 1;
	result[3] = 1;
	return result;
}

mint_t *vec4i_add(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] + b[0];
	result[1] = a[1] + b[1];
	result[2] = a[2] + b[2];
	result[3] = a[3] + b[3];
	return result;
}

mint_t *vec4i_subtract(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] - b[0];
	result[1] = a[1] - b[1];
	result[2] = a[2] - b[2];
	result[3] = a[3] - b[3];
	return result;
}

mint_t *vec4i_scale(mint_t *result, mint_t *a, mfloat_t scalar)
{
	result[0] = (mint_t)MROUND(a[0] * scalar);
	result[1] = (mint_t)MROUND(a[1] * scalar);
	result[2] = (mint_t)MROUND(a[2] * scalar);
	result[3] = (mint_t)MROUND(a[3] * scalar);
	return result;
}

mint_t *vec4i_multiply(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] * b[0];
	result[1] = a[1] * b[1];
	result[2] = a[2] * b[2];
	result[3] = a[3] * b[3];
	return result;
}

mint_t *vec4i_multiply_mat4(mint_t *result, mint_t *a, mfloat_t *m)
{
	mint_t x = a[0];
	mint_t y = a[1];
	mint_t z = a[2];
	mint_t w = a[3];
	result[0] = (mint_t)MROUND(m[0] * x + m[4] * y + m[8] * z + m[12] * w);
	result[1] = (mint_t)MROUND(m[1] * x + m[5] * y + m[9] * z + m[13] * w);
	result[2] = (mint_t)MROUND(m[2] * x + m[6] * y + m[10] * z + m[14] * w);
	result[3] = (mint_t)MROUND(m[3] * x + m[7] * y + m[11] * z + m[15] * w);
	return result;
}

mint_t *vec4i_divide(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = a[0] / b[0];
	result[1] = a[1] / b[1];
	result[2] = a[2] / b[2];
	result[3] = a[3] / b[3];
	return result;
}

mint_t *vec4i_snap(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = (a[0] / b[0]) * b[0];
	result[1] = (a[1] / b[1]) * b[1];
	result[2] = (a[2] / b[2]) * b[2];
	result[3] = (a[3] / b[3]) * b[3];
	return result;
}

mint_t *vec4i_negative(mint_t *result, mint_t *a)
{
	result[0] = -a[0];
	result[1] = -a[1];
	result[2] = -a[2];
	result[3] = -a[3];
	return result;
}

mint_t *vec4i_inverse(mint_t *result, mint_t *a)
{
	if (a[0] != 0) {
		result[0] = (mint_t)MROUND(MFLOAT_C(1.0) / (mfloat_t)a[0]);
	} else {
		result[0] = 0;
	}
	if (a[1] != 0) {
		result[1] = (mint_t)MROUND(MFLOAT_C(1.0) / (mfloat_t)a[1]);
	} else {
		result[1] = 0;
	}
	if (a[2] != 0) {
		result[2] = (mint_t)MROUND(MFLOAT_C(1.0) / (mfloat_t)a[2]);
	} else {
		result[2] = 0;
	}
	if (a[3] != 0) {
		result[3] = (mint_t)MROUND(MFLOAT_C(1.0) / (mfloat_t)a[3]);
	} else {
		result[3] = 0;
	}
	return result;
}

mint_t *vec4i_abs(mint_t *result, mint_t *a)
{
	result[0] = a[0];
	if (result[0] < 0) {
		result[0] = -result[0];
	}
	result[1] = a[1];
	if (result[1] < 0) {
		result[1] = -result[1];
	}
	result[2] = a[2];
	if (result[2] < 0) {
		result[2] = -result[2];
	}
	result[3] = a[3];
	if (result[3] < 0) {
		result[3] = -result[3];
	}return result;
}

mint_t *vec4i_floor(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MFLOOR(a[0]);
	result[1] = (mint_t)MFLOOR(a[1]);
	result[2] = (mint_t)MFLOOR(a[2]);
	result[3] = (mint_t)MFLOOR(a[3]);
	return result;
}

mint_t *vec4i_ceil(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MCEIL(a[0]);
	result[1] = (mint_t)MCEIL(a[1]);
	result[2] = (mint_t)MCEIL(a[2]);
	result[3] = (mint_t)MCEIL(a[3]);
	return result;
}

mint_t *vec4i_round(mint_t *result, mfloat_t *a)
{
	result[0] = (mint_t)MROUND(a[0]);
	result[1] = (mint_t)MROUND(a[1]);
	result[2] = (mint_t)MROUND(a[2]);
	result[3] = (mint_t)MROUND(a[3]);
	return result;
}

mint_t *vec4i_max(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = MMAXI(a[0], b[0]);
	result[1] = MMAXI(a[1], b[1]);
	result[2] = MMAXI(a[2], b[2]);
	result[3] = MMAXI(a[3], b[3]);
	return result;
}

mint_t *vec4i_min(mint_t *result, mint_t *a, mint_t *b)
{
	result[0] = MMINI(a[0], b[0]);
	result[1] = MMINI(a[1], b[1]);
	result[2] = MMINI(a[2], b[2]);
	result[3] = MMINI(a[3], b[3]);
	return result;
}

mint_t *vec4i_clamp(mint_t *result, mint_t *a, mint_t *lower, mint_t *higher)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	result[3] = a[3];
	if (result[0] < lower[0]) {
		result[0] = lower[0];
	}
	if (result[0] > higher[0]) {
		result[0] = higher[0];
	}
	if (result[1] < lower[1]) {
		result[1] = lower[1];
	}
	if (result[1] > higher[1]) {
		result[1] = higher[1];
	}
	if (result[2] < lower[2]) {
		result[2] = lower[2];
	}
	if (result[2] > higher[2]) {
		result[2] = higher[2];
	}
	if (result[3] < lower[3]) {
		result[3] = lower[3];
	}
	if (result[3] > higher[3]) {
		result[3] = higher[3];
	}
	return result;
}

mint_t *vec4i_normalize(mint_t *result, mint_t *a)
{
	mfloat_t length = MSQRT((mfloat_t)a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3]);
	if (!nearly_equal(length, MFLOAT_C(0.0), MFLT_EPSILON)) {
		length = MFLOAT_C(1.0) / length;
		result[0] = (mint_t)MROUND(a[0] * length);
		result[1] = (mint_t)MROUND(a[1] * length);
		result[2] = (mint_t)MROUND(a[2] * length);
		result[3] = (mint_t)MROUND(a[3] * length);
	} else {
		result[0] = 0;
		result[1] = 0;
		result[2] = 0;
		result[3] = 0;
	}
	return result;
}

mint_t *vec4i_lerp(mint_t *result, mint_t *a, mint_t *b, mfloat_t p)
{
	result[0] = a[0] + (mint_t)MROUND((b[0] - a[0]) * p);
	result[1] = a[1] + (mint_t)MROUND((b[1] - a[1]) * p);
	result[2] = a[2] + (mint_t)MROUND((b[2] - a[2]) * p);
	result[3] = a[3] + (mint_t)MROUND((b[3] - a[3]) * p);
	return result;
}

/* Quaternion */
bool quat_is_zero(mfloat_t *a)
{
	return nearly_equal(a[0], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[1], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[2], MFLOAT_C(0.0), MFLT_EPSILON) && nearly_equal(a[3], MFLOAT_C(0.0), MFLT_EPSILON);
}

bool quat_is_near_zero(mfloat_t *a, mfloat_t epsilon)
{
	return nearly_equal(a[0], MFLOAT_C(0.0), epsilon) && nearly_equal(a[1], MFLOAT_C(0.0), epsilon) && nearly_equal(a[2], MFLOAT_C(0.0), epsilon) && nearly_equal(a[3], MFLOAT_C(0.0), epsilon);
}

bool quat_is_equal(mfloat_t *a, mfloat_t *b)
{
	return nearly_equal(a[0], b[0], MFLT_EPSILON) && nearly_equal(a[1], b[1], MFLT_EPSILON) && nearly_equal(a[2], b[2], MFLT_EPSILON) && nearly_equal(a[3], b[3], MFLT_EPSILON);
}

bool quat_is_nearly_equal(mfloat_t *a, mfloat_t *b, mfloat_t epsilon)
{
	return nearly_equal(a[0], b[0], epsilon) && nearly_equal(a[1], b[1], epsilon) && nearly_equal(a[2], b[2], epsilon) && nearly_equal(a[3], b[3], epsilon);
}

mfloat_t *quat(mfloat_t *result, mfloat_t x, mfloat_t y, mfloat_t z, mfloat_t w)
{
	result[0] = x;
	result[1] = y;
	result[2] = z;
	result[3] = w;
	return result;
}

mfloat_t *quat_assign(mfloat_t *result, mfloat_t *a)
{
	result[0] = a[0];
	result[1] = a[1];
	result[2] = a[2];
	result[3] = a[3];
	return result;
}

mfloat_t *quat_zero(mfloat_t *result)
{
	result[0] = MFLOAT_C(0.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *quat_null(mfloat_t *result)
{
	result[0] = MFLOAT_C(0.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *quat_scale(mfloat_t *result, mfloat_t *a, mfloat_t scalar)
{
	result[0] = a[0] * scalar;
	result[1] = a[1] * scalar;
	result[2] = a[2] * scalar;
	result[3] = a[3] * scalar;
	return result;
}

mfloat_t *quat_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	result[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
	result[1] = a[3] * b[1] + a[1] * b[3] + a[2] * b[0] - a[0] * b[2];
	result[2] = a[3] * b[2] + a[2] * b[3] + a[0] * b[1] - a[1] * b[0];
	result[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
	return result;
}

mfloat_t *quat_divide(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t x = a[0];
	mfloat_t y = a[1];
	mfloat_t z = a[2];
	mfloat_t w = a[3];
	mfloat_t length_squared = MFLOAT_C(1.0) / (b[0] * b[0] + b[1] * b[1] + b[8] * b[8] + b[3] * b[3]);
	mfloat_t normalized_x = -b[0] * length_squared;
	mfloat_t normalized_y = -b[1] * length_squared;
	mfloat_t normalized_z = -b[8] * length_squared;
	mfloat_t normalized_w = b[3] * length_squared;
	result[0] = x * normalized_w + normalized_x * w + (y * normalized_z - z * normalized_y);
	result[1] = y * normalized_w + normalized_y * w + (z * normalized_x - x * normalized_z);
	result[2] = z * normalized_w + normalized_z * w + (x * normalized_y - y * normalized_x);
	result[3] = w * normalized_w - (x * normalized_x + y * normalized_y + z * normalized_z);
	return result;
}

mfloat_t *quat_negative(mfloat_t *result, mfloat_t *a)
{
	result[0] = -a[0];
	result[1] = -a[1];
	result[2] = -a[2];
	result[3] = -a[3];
	return result;
}

mfloat_t *quat_conjugate(mfloat_t *result, mfloat_t *a)
{
	result[0] = -a[0];
	result[1] = -a[1];
	result[2] = -a[2];
	result[3] = a[3];
	return result;
}

mfloat_t *quat_inverse(mfloat_t *result, mfloat_t *a)
{
	mfloat_t length = a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3];
	if (!nearly_equal(length, MFLOAT_C(0.0), MFLT_EPSILON)) {
		length = MFLOAT_C(1.0) / length;
	} else {
		length = MFLOAT_C(0.0);
	}
	result[0] = -a[0] * length;
	result[1] = -a[1] * length;
	result[2] = -a[2] * length;
	result[3] = a[3] * length;
	return result;
}

mfloat_t *quat_normalize(mfloat_t *result, mfloat_t *a)
{
	mfloat_t length = MFLOAT_C(1.0) / MSQRT(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3]);
	if (!nearly_equal(length, MFLOAT_C(0.0), MFLT_EPSILON)) {
		result[0] = a[0] * length;
		result[1] = a[1] * length;
		result[2] = a[2] * length;
		result[3] = a[3] * length;
	} else {
		result[0] = MFLOAT_C(0.0);
		result[1] = MFLOAT_C(0.0);
		result[2] = MFLOAT_C(0.0);
		result[3] = MFLOAT_C(1.0);
	}
	return result;
}

mfloat_t *quat_power(mfloat_t *result, mfloat_t *a, mfloat_t exponent)
{
	if (MABS(a[3]) < MFLOAT_C(1.0) - MFLT_EPSILON) {
		mfloat_t alpha = MACOS(a[3]);
		mfloat_t new_alpha = alpha * exponent;
		mfloat_t s = MSIN(new_alpha) / MSIN(alpha);
		result[0] = result[0] * s;
		result[1] = result[1] * s;
		result[2] = result[2] * s;
		result[3] = MCOS(new_alpha);
	} else {
		result[0] = a[0];
		result[1] = a[1];
		result[2] = a[1];
		result[3] = a[3];
	}
	return result;
}

mfloat_t *quat_from_axis_angle(mfloat_t *result, mfloat_t *a, mfloat_t angle)
{
	mfloat_t half = angle * MFLOAT_C(0.5);
	mfloat_t s = MSIN(half);
	result[0] = a[0] * s;
	result[1] = a[1] * s;
	result[2] = a[2] * s;
	result[3] = MCOS(half);
	quat_normalize(result, result);
	return result;
}

mfloat_t *quat_from_vec3(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t cross[VEC3_SIZE];
	mfloat_t dot = vec3_dot(a, b);
	mfloat_t a_length_sq = vec3_length_squared(a);
	mfloat_t b_length_sq = vec3_length_squared(a);
	vec3_cross(cross, a, b);
	quat(result, cross[0], cross[1], cross[1], dot + MSQRT(a_length_sq * b_length_sq));
	quat_normalize(result, result);
	return result;
}

mfloat_t *quat_from_mat4(mfloat_t *result, mfloat_t *m)
{
	mfloat_t sr;
	mfloat_t half;
	mfloat_t scale = m[0] + m[5] + m[10];
	if (scale > MFLOAT_C(0.0)) {
		sr = MSQRT(scale + MFLOAT_C(1.0));
		result[3] = sr * MFLOAT_C(0.5);
		sr = MFLOAT_C(0.5) / sr;
		result[0] = (m[9] - m[6]) * sr;
		result[1] = (m[2] - m[8]) * sr;
		result[2] = (m[4] - m[1]) * sr;
	} else if ((m[0] >= m[5]) && (m[0] >= m[10])) {
		sr = MSQRT(MFLOAT_C(1.0) + m[0] - m[5] - m[10]);
		half = MFLOAT_C(0.5) / sr;
		result[0] = MFLOAT_C(0.5) * sr;
		result[1] = (m[4] + m[1]) * half;
		result[2] = (m[8] + m[2]) * half;
		result[3] = (m[9] - m[6]) * half;
	} else if (m[5] > m[10]) {
		sr = MSQRT(MFLOAT_C(1.0) + m[5] - m[0] - m[10]);
		half = MFLOAT_C(0.5) / sr;
		result[0] = (m[1] + m[4]) * half;
		result[1] = MFLOAT_C(0.5) * sr;
		result[2] = (m[6] + m[9]) * half;
		result[3] = (m[2] - m[8]) * half;
	} else {
		sr = MSQRT(MFLOAT_C(1.0) + m[10] - m[0] - m[5]);
		half = MFLOAT_C(0.5) / sr;
		result[0] = (m[2] + m[8]) * half;
		result[1] = (m[6] + m[9]) * half;
		result[2] = MFLOAT_C(0.5) * sr;
		result[3] = (m[4] - m[1]) * half;
	}
	return result;
}

mfloat_t *quat_from_yaw_pitch_roll(mfloat_t *result, mfloat_t yaw, mfloat_t pitch, mfloat_t roll)
{
	mfloat_t half_roll = roll * MFLOAT_C(0.5);
	mfloat_t half_pitch = pitch * MFLOAT_C(0.5);
	mfloat_t half_yaw = yaw * MFLOAT_C(0.5);
	mfloat_t sin_roll = MSIN(half_roll);
	mfloat_t cos_roll = MCOS(half_roll);
	mfloat_t sin_pitch = MSIN(half_pitch);
	mfloat_t cos_pitch = MCOS(half_pitch);
	mfloat_t sin_yaw = MSIN(half_yaw);
	mfloat_t cos_yaw = MCOS(half_yaw);
	result[0] = cos_yaw * sin_pitch * cos_roll + sin_yaw * cos_pitch * sin_roll;
	result[1] = sin_yaw * cos_pitch * cos_roll - cos_yaw * sin_pitch * sin_roll;
	result[2] = cos_yaw * cos_pitch * sin_roll - sin_yaw * sin_pitch * cos_roll;
	result[3] = cos_yaw * cos_pitch * cos_roll + sin_yaw * sin_pitch * sin_roll;
	return result;
}

mfloat_t *quat_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p)
{
	result[0] = a[0] + (b[0] - a[0]) * p;
	result[1] = a[1] + (b[1] - a[1]) * p;
	result[2] = a[2] + (b[2] - a[2]) * p;
	result[3] = a[3] + (b[3] - a[3]) * p;
	return result;
}

mfloat_t *quat_slerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p)
{
	mfloat_t tmp_a[QUAT_SIZE];
	mfloat_t tmp_b[QUAT_SIZE];
	mfloat_t cos_theta = quat_dot(a, b);
	mfloat_t p0;
	mfloat_t p1;
	quat(tmp_a, a[0], a[1], a[2], a[3]);
	quat(tmp_b, b[0], b[1], b[2], b[3]);
	/* Take shortest arc */
	if (cos_theta < MFLOAT_C(0.0)) {
		quat_negative(tmp_b, tmp_b);
		cos_theta = -cos_theta;
	}
	/* Check if quaternions are close */
	if (cos_theta > MFLOAT_C(0.95)) {
		/* Use linear interpolation */
		p0 = MFLOAT_C(1.0) - p;
		p1 = p;
	} else {
		mfloat_t theta = MACOS(cos_theta);
		mfloat_t sin_theta = MSIN(theta);
		p0 = MSIN((MFLOAT_C(1.0) - p) * theta) / sin_theta;
		p1 = MSIN(p * theta) / sin_theta;
	}
	result[0] = tmp_a[0] * p0 + tmp_b[0] * p1;
	result[1] = tmp_a[1] * p0 + tmp_b[1] * p1;
	result[2] = tmp_a[2] * p0 + tmp_b[2] * p1;
	result[3] = tmp_a[3] * p0 + tmp_b[3] * p1;
	return result;
}

mfloat_t quat_dot(mfloat_t *a, mfloat_t *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

mfloat_t quat_angle(mfloat_t *a, mfloat_t *b)
{
	mfloat_t s = MSQRT(quat_length_squared(a) * quat_length_squared(b));
	if (MABS(s) > MFLT_EPSILON) {
		s = MFLOAT_C(1.0) / s;
	} else {
		s = MFLOAT_C(0.0);
	}
	return MACOS(quat_dot(a, b) * s);
}

mfloat_t quat_length(mfloat_t *a)
{
	return MSQRT(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3]);
}

mfloat_t quat_length_squared(mfloat_t *a)
{
	return a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3];
}

/* Matrix 2x2 */
mfloat_t *mat2(mfloat_t *result,
	mfloat_t m11, mfloat_t m12,
	mfloat_t m21, mfloat_t m22)
{
	result[0] = m11;
	result[1] = m21;
	result[2] = m12;
	result[3] = m22;
	return result;
}

mfloat_t *mat2_zero(mfloat_t *result)
{
	result[0] = MFLOAT_C(0.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	return result;
}

mfloat_t mat2_determinant(mfloat_t *m)
{
	return m[0] * m[3] - m[2] * m[1];
}

mfloat_t *mat2_assign(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[2] = m[2];
	result[3] = m[3];
	return result;
}

mfloat_t *mat2_assign_mat3(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[2] = m[3];
	result[3] = m[4];
	return result;
}

mfloat_t *mat2_assign_mat4(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[2] = m[4];
	result[3] = m[5];
	return result;
}

mfloat_t *mat2_transpose(mfloat_t *result, mfloat_t *m)
{
	mfloat_t transposed[MAT2_SIZE];
	transposed[0] = m[0];
	transposed[1] = m[2];
	transposed[2] = m[1];
	transposed[3] = m[3];
	result[0] = transposed[0];
	result[1] = transposed[1];
	result[2] = transposed[2];
	result[3] = transposed[3];
	return result;
}

mfloat_t *mat2_cofactor(mfloat_t *result, mfloat_t *m)
{
	mfloat_t adjugate[MAT2_SIZE];
	adjugate[0] = m[3];
	adjugate[1] = -m[1];
	adjugate[2] = -m[2];
	adjugate[3] = m[0];
	result[0] = adjugate[0];
	result[1] = adjugate[1];
	result[2] = adjugate[2];
	result[3] = adjugate[3];
	return result;
}

mfloat_t *mat2_inverse(mfloat_t *result, mfloat_t *m)
{
	mfloat_t inverse[MAT2_SIZE];
	mfloat_t det = mat2_determinant(m);
	if (!nearly_equal(det, MFLOAT_C(0.0), MFLT_EPSILON)) {
		mat2_cofactor(inverse, m);
		mat2_scale(inverse, inverse, MFLOAT_C(1.0) / det);
		result[0] = inverse[0];
		result[1] = inverse[1];
		result[2] = inverse[2];
		result[3] = inverse[3];
	}
	return result;
}

mfloat_t *mat2_rotation(mfloat_t *result, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	result[0] = c;
	result[1] = s;
	result[2] = -s;
	result[3] = c;
	return result;
}

mfloat_t *mat2_scaling(mfloat_t *result, mfloat_t *v)
{
	result[0] = v[0];
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = v[1];
	return result;
}

mfloat_t *mat2_negative(mfloat_t *result, mfloat_t *m)
{
	result[0] = -m[0];
	result[1] = -m[1];
	result[2] = -m[2];
	result[3] = -m[3];
	return result;
}

mfloat_t *mat2_scale(mfloat_t *result, mfloat_t *m, mfloat_t scalar)
{
	result[0] = m[0] * scalar;
	result[1] = m[1] * scalar;
	result[2] = m[2] * scalar;
	result[3] = m[3] * scalar;
	return result;
}

mfloat_t *mat2_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t multiplied[MAT3_SIZE];
	multiplied[0] = a[0] * b[0] + a[2] * b[1];
	multiplied[1] = a[1] * b[0] + a[3] * b[1];
	multiplied[2] = a[0] * b[2] + a[2] * b[3];
	multiplied[3] = a[1] * b[2] + a[3] * b[3];
	result[0] = multiplied[0];
	result[1] = multiplied[1];
	result[2] = multiplied[2];
	result[3] = multiplied[3];
	return result;
}

mfloat_t *mat2_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p)
{
	result[0] = a[0] + (b[0] - a[0]) * p;
	result[1] = a[1] + (b[1] - a[1]) * p;
	result[2] = a[2] + (b[2] - a[2]) * p;
	result[3] = a[3] + (b[3] - a[3]) * p;
	return result;
}

/* Matrix 3x3 */
mfloat_t *mat3(mfloat_t *result,
	mfloat_t m11, mfloat_t m12, mfloat_t m13,
	mfloat_t m21, mfloat_t m22, mfloat_t m23,
	mfloat_t m31, mfloat_t m32, mfloat_t m33)
{
	result[0] = m11;
	result[1] = m21;
	result[2] = m31;
	result[3] = m12;
	result[4] = m22;
	result[5] = m32;
	result[6] = m13;
	result[7] = m23;
	result[8] = m33;
	return result;
}

mfloat_t *mat3_zero(mfloat_t *result)
{
	result[0] = MFLOAT_C(0.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(0.0);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *mat3_identity(mfloat_t *result)
{
	result[0] = MFLOAT_C(1.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(1.0);
	result[5] = MFLOAT_C(0.0);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(8.0);
	return result;
}

mfloat_t mat3_determinant(mfloat_t *m)
{
	mfloat_t det1;
	mfloat_t det2;
	mfloat_t det3;
	mfloat_t m2[MAT2_SIZE];
	m2[0] = m[4];
	m2[1] = m[5];
	m2[2] = m[7];
	m2[3] = m[8];
	det1 = mat2_determinant(m2);
	m2[0] = m[1];
	m2[1] = m[2];
	m2[2] = m[7];
	m2[3] = m[8];
	det2 = mat2_determinant(m2);
	m2[0] = m[1];
	m2[1] = m[2];
	m2[2] = m[4];
	m2[3] = m[5];
	det3 = mat2_determinant(m2);
	return m[0] * det1 - m[3] * det2 - m[6] * det3;
}

mfloat_t *mat3_assign(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[2] = m[2];
	result[3] = m[3];
	result[4] = m[4];
	result[5] = m[5];
	result[6] = m[6];
	result[7] = m[7];
	result[8] = m[8];
	return result;
}

mfloat_t *mat3_assign_mat2(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[3] = m[2];
	result[4] = m[3];
	return result;
}

mfloat_t *mat3_assign_mat4(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[2] = m[2];
	result[3] = m[4];
	result[4] = m[5];
	result[5] = m[6];
	result[6] = m[8];
	result[7] = m[9];
	result[8] = m[10];
	return result;
}

mfloat_t *mat3_transpose(mfloat_t *result, mfloat_t *m)
{
	mfloat_t transposed[MAT4_SIZE];
	transposed[0] = m[0];
	transposed[1] = m[3];
	transposed[2] = m[6];
	transposed[3] = m[1];
	transposed[4] = m[4];
	transposed[5] = m[7];
	transposed[6] = m[2];
	transposed[7] = m[5];
	transposed[8] = m[8];
	result[0] = transposed[0];
	result[1] = transposed[1];
	result[2] = transposed[2];
	result[3] = transposed[3];
	result[4] = transposed[4];
	result[5] = transposed[5];
	result[6] = transposed[6];
	result[7] = transposed[7];
	result[8] = transposed[8];
	return result;
}

mfloat_t *mat3_cofactor(mfloat_t *result, mfloat_t *m)
{
	mfloat_t cofactor[MAT3_SIZE];
	mfloat_t m2[MAT2_SIZE];
	m2[0] = m[4];
	m2[1] = m[5];
	m2[2] = m[7];
	m2[3] = m[8];
	cofactor[0] = mat2_determinant(m2);
	m2[0] = m[3];
	m2[1] = m[5];
	m2[2] = m[6];
	m2[3] = m[8];
	cofactor[1] = -mat2_determinant(m2);
	m2[0] = m[3];
	m2[1] = m[4];
	m2[2] = m[6];
	m2[3] = m[7];
	cofactor[2] = mat2_determinant(m2);
	m2[0] = m[1];
	m2[1] = m[2];
	m2[2] = m[7];
	m2[3] = m[8];
	cofactor[3] = -mat2_determinant(m2);
	m2[0] = m[0];
	m2[1] = m[2];
	m2[2] = m[6];
	m2[3] = m[8];
	cofactor[4] = mat2_determinant(m2);
	m2[0] = m[0];
	m2[1] = m[1];
	m2[2] = m[6];
	m2[3] = m[7];
	cofactor[5] = -mat2_determinant(m2);
	m2[0] = m[1];
	m2[1] = m[2];
	m2[2] = m[4];
	m2[3] = m[5];
	cofactor[6] = mat2_determinant(m2);
	m2[0] = m[0];
	m2[1] = m[2];
	m2[2] = m[3];
	m2[3] = m[5];
	cofactor[7] = -mat2_determinant(m2);
	m2[0] = m[0];
	m2[1] = m[1];
	m2[2] = m[4];
	m2[3] = m[5];
	cofactor[8] = mat2_determinant(m2);
	result[0] = cofactor[0];
	result[1] = cofactor[1];
	result[2] = cofactor[2];
	result[3] = cofactor[3];
	result[4] = cofactor[4];
	result[5] = cofactor[5];
	result[6] = cofactor[6];
	result[7] = cofactor[7];
	result[8] = cofactor[8];
	return result;
}

mfloat_t *mat3_adjugate(mfloat_t *result, mfloat_t *m)
{
	result = m;
	return result;
}

mfloat_t *mat3_inverse(mfloat_t *result, mfloat_t *m)
{
	result = m;
	return result;
}

mfloat_t *mat3_rotation_x(mfloat_t *result, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	result[0] = MFLOAT_C(1.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = c;
	result[5] = s;
	result[6] = MFLOAT_C(0.0);
	result[7] = -s;
	result[8] = -c;
	return result;
}

mfloat_t *mat3_rotation_y(mfloat_t *result, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	result[0] = c;
	result[1] = MFLOAT_C(0.0);
	result[2] = -s;
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(0.0);
	result[6] = s;
	result[7] = MFLOAT_C(0.0);
	result[8] = c;
	return result;
}

mfloat_t *mat3_rotation_z(mfloat_t *result, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	result[0] = c;
	result[1] = s;
	result[2] = MFLOAT_C(0.0);
	result[3] = -s;
	result[4] = c;
	result[5] = MFLOAT_C(0.0);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat3_rotation_axis(mfloat_t *result, mfloat_t *a, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	mfloat_t one_c = MFLOAT_C(1.0) - c;
	mfloat_t x = a[0];
	mfloat_t y = a[4];
	mfloat_t z = a[8];
	mfloat_t xx = x * x;
	mfloat_t xy = x * y;
	mfloat_t xz = x * z;
	mfloat_t yy = y * y;
	mfloat_t yz = y * z;
	mfloat_t zz = z * z;
	mfloat_t l = xx + yy + zz;
	mfloat_t sqrt_l = MSQRT(l);
	result[0] = (xx + (yy + zz) * c) / l;
	result[1] = (xy * one_c + a[2] * sqrt_l * s) / l;
	result[2] = (xz * one_c - a[1] * sqrt_l * s) / l;
	result[3] = (xy * one_c - a[2] * sqrt_l * s) / l;
	result[4] = (yy + (xx + zz) * c) / l;
	result[5] = (yz * one_c + a[0] * sqrt_l * s) / l;
	result[6] = (xz * one_c + a[1] * sqrt_l * s) / l;
	result[7] = (yz * one_c - a[0] * sqrt_l * s) / l;
	result[8] = (zz + (xx + yy) * c) / l;
	return result;
}

mfloat_t *mat3_rotation_quaternion(mfloat_t *result, mfloat_t *q)
{
	mfloat_t xx = q[0] * q[0];
	mfloat_t yy = q[1] * q[1];
	mfloat_t zz = q[2] * q[2];
	mfloat_t xy = q[0] * q[1];
	mfloat_t zw = q[2] * q[3];
	mfloat_t xz = q[8] * q[0];
	mfloat_t yw = q[1] * q[3];
	mfloat_t yz = q[1] * q[2];
	mfloat_t xw = q[0] * q[3];
	result[0] = MFLOAT_C(1.0) - MFLOAT_C(2.0) * (yy - zz);
	result[1] = MFLOAT_C(2.0) * (xy + zw);
	result[2] = MFLOAT_C(2.0) * (xz - yw);
	result[3] = MFLOAT_C(2.0) * (xy - zw);
	result[4] = MFLOAT_C(1.0) - MFLOAT_C(2.0) * (xx - zz);
	result[5] = MFLOAT_C(2.0) * (yz + xw);
	result[6] = MFLOAT_C(2.0) * (xz + yw);
	result[7] = MFLOAT_C(2.0) * (yz - xw);
	result[8] = MFLOAT_C(1.0) - MFLOAT_C(2.0) * (xx - yy);
	return result;
}

mfloat_t *mat3_scaling(mfloat_t *result, mfloat_t *v)
{
	result[0] = v[0];
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = v[1];
	result[5] = MFLOAT_C(0.0);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = v[2];
	return result;
}

mfloat_t *mat3_negative(mfloat_t *result, mfloat_t *m)
{
	result[0] = -m[0];
	result[1] = -m[1];
	result[2] = -m[2];
	result[3] = -m[3];
	result[4] = -m[4];
	result[5] = -m[5];
	result[6] = -m[6];
	result[7] = -m[7];
	result[8] = -m[8];
	return result;
}

mfloat_t *mat3_scale(mfloat_t *result, mfloat_t *m, mfloat_t scalar)
{
	result[0] = m[0] * scalar;
	result[1] = m[1] * scalar;
	result[2] = m[2] * scalar;
	result[3] = m[3] * scalar;
	result[4] = m[4] * scalar;
	result[5] = m[5] * scalar;
	result[6] = m[6] * scalar;
	result[7] = m[7] * scalar;
	result[8] = m[8] * scalar;
	return result;
}

mfloat_t *mat3_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t multiplied[MAT3_SIZE];
	multiplied[0] = a[0] * b[0] + a[3] * b[1] + a[6] * b[2];
	multiplied[1] = a[1] * b[0] + a[4] * b[1] + a[7] * b[2];
	multiplied[2] = a[2] * b[0] + a[5] * b[1] + a[8] * b[2];
	multiplied[3] = a[0] * b[3] + a[3] * b[4] + a[6] * b[5];
	multiplied[4] = a[1] * b[3] + a[4] * b[4] + a[7] * b[5];
	multiplied[5] = a[2] * b[3] + a[5] * b[4] + a[8] * b[5];
	multiplied[6] = a[0] * b[6] + a[3] * b[7] + a[6] * b[8];
	multiplied[7] = a[1] * b[6] + a[4] * b[7] + a[7] * b[8];
	multiplied[8] = a[2] * b[6] + a[5] * b[7] + a[8] * b[8];
	result[0] = multiplied[0];
	result[1] = multiplied[1];
	result[2] = multiplied[2];
	result[3] = multiplied[3];
	result[4] = multiplied[4];
	result[5] = multiplied[5];
	result[6] = multiplied[6];
	result[7] = multiplied[7];
	result[8] = multiplied[8];
	return result;
}

mfloat_t *mat3_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p)
{
	result[0] = a[0] + (b[0] - a[0]) * p;
	result[1] = a[1] + (b[1] - a[1]) * p;
	result[2] = a[2] + (b[2] - a[2]) * p;
	result[3] = a[3] + (b[3] - a[3]) * p;
	result[4] = a[4] + (b[4] - a[4]) * p;
	result[5] = a[5] + (b[5] - a[5]) * p;
	result[6] = a[6] + (b[6] - a[6]) * p;
	result[7] = a[7] + (b[7] - a[7]) * p;
	result[8] = a[8] + (b[8] - a[8]) * p;
	return result;
}

/* Matrix 4x4 */
mfloat_t *mat4(mfloat_t *result,
	mfloat_t m11, mfloat_t m12, mfloat_t m13, mfloat_t m14,
	mfloat_t m21, mfloat_t m22, mfloat_t m23, mfloat_t m24,
	mfloat_t m31, mfloat_t m32, mfloat_t m33, mfloat_t m34,
	mfloat_t m41, mfloat_t m42, mfloat_t m43, mfloat_t m44)
{
	result[0] = m11;
	result[1] = m21;
	result[2] = m31;
	result[3] = m41;
	result[4] = m12;
	result[5] = m22;
	result[6] = m32;
	result[7] = m42;
	result[8] = m13;
	result[9] = m23;
	result[10] = m33;
	result[11] = m43;
	result[12] = m14;
	result[13] = m24;
	result[14] = m34;
	result[15] = m44;
	return result;
}

mfloat_t *mat4_zero(mfloat_t *result)
{
	result[0] = MFLOAT_C(0.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(0.0);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = MFLOAT_C(0.0);
	result[11] = MFLOAT_C(0.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = MFLOAT_C(0.0);
	result[15] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *mat4_identity(mfloat_t *result)
{
	result[0] = MFLOAT_C(1.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(1.0);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = MFLOAT_C(1.0);
	result[11] = MFLOAT_C(0.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = MFLOAT_C(0.0);
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t mat4_determinant(mfloat_t *m)
{
	mfloat_t det1;
	mfloat_t det2;
	mfloat_t det3;
	mfloat_t det4;
	mfloat_t m2[MAT3_SIZE];
	m2[0] = m[5];
	m2[1] = m[6];
	m2[2] = m[7];
	m2[3] = m[9];
	m2[4] = m[10];
	m2[5] = m[11];
	m2[6] = m[13];
	m2[7] = m[14];
	m2[8] = m[15];
	det1 = mat3_determinant(m2);
	m2[0] = m[1];
	m2[1] = m[2];
	m2[2] = m[3];
	m2[3] = m[9];
	m2[4] = m[10];
	m2[5] = m[11];
	m2[6] = m[13];
	m2[7] = m[14];
	m2[8] = m[15];
	det2 = mat2_determinant(m2);
	m2[0] = m[1];
	m2[1] = m[2];
	m2[2] = m[3];
	m2[3] = m[5];
	m2[4] = m[6];
	m2[5] = m[7];
	m2[6] = m[13];
	m2[7] = m[14];
	m2[8] = m[15];
	det3 = mat2_determinant(m2);
	m2[0] = m[1];
	m2[1] = m[2];
	m2[2] = m[3];
	m2[3] = m[5];
	m2[4] = m[6];
	m2[5] = m[7];
	m2[6] = m[9];
	m2[7] = m[10];
	m2[8] = m[11];
	det4 = mat2_determinant(m2);
	return m[0] * det1 - m[4] * det2 + m[8] * det3 - m[12] * det4;
}

mfloat_t *mat4_assign(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[2] = m[2];
	result[3] = m[3];
	result[4] = m[4];
	result[5] = m[5];
	result[6] = m[6];
	result[7] = m[7];
	result[8] = m[8];
	result[9] = m[9];
	result[10] = m[10];
	result[11] = m[11];
	result[12] = m[12];
	result[13] = m[13];
	result[14] = m[14];
	result[15] = m[15];
	return result;
}

mfloat_t *mat4_assign_mat2(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[4] = m[2];
	result[5] = m[3];
	return result;
}

mfloat_t *mat4_assign_mat3(mfloat_t *result, mfloat_t *m)
{
	result[0] = m[0];
	result[1] = m[1];
	result[2] = m[2];
	result[4] = m[3];
	result[5] = m[4];
	result[6] = m[5];
	result[8] = m[6];
	result[9] = m[7];
	result[10] = m[8];
	return result;
}

mfloat_t *mat4_transpose(mfloat_t *result, mfloat_t *m)
{
	mfloat_t transposed[MAT4_SIZE];
	transposed[0] = m[0];
	transposed[1] = m[4];
	transposed[2] = m[8];
	transposed[3] = m[12];
	transposed[4] = m[1];
	transposed[5] = m[5];
	transposed[6] = m[9];
	transposed[7] = m[13];
	transposed[8] = m[2];
	transposed[9] = m[6];
	transposed[10] = m[10];
	transposed[11] = m[14];
	transposed[12] = m[3];
	transposed[13] = m[7];
	transposed[14] = m[11];
	transposed[15] = m[15];
	result[0] = transposed[0];
	result[1] = transposed[1];
	result[2] = transposed[2];
	result[3] = transposed[3];
	result[4] = transposed[4];
	result[5] = transposed[5];
	result[6] = transposed[6];
	result[7] = transposed[7];
	result[8] = transposed[8];
	result[9] = transposed[9];
	result[10] = transposed[10];
	result[11] = transposed[11];
	result[12] = transposed[12];
	result[13] = transposed[13];
	result[14] = transposed[14];
	result[15] = transposed[15];
	return result;
}

mfloat_t *mat4_adjugate(mfloat_t *result, mfloat_t *m)
{
	result = m;
	return result;
}

mfloat_t *mat4_inverse(mfloat_t *result, mfloat_t *m)
{
	mfloat_t inverse[MAT4_SIZE];
	mfloat_t det;
	inverse[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];
	inverse[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];
	inverse[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];
	inverse[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];
	inverse[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];
	inverse[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];
	inverse[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];
	inverse[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];
	inverse[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];
	inverse[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];
	inverse[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];
	inverse[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];
	inverse[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];
	inverse[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];
	inverse[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];
	inverse[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];
	det = m[0] * inverse[0] + m[1] * inverse[4] + m[2] * inverse[8] + m[3] * inverse[12];
	if (!nearly_equal(det, MFLOAT_C(0.0), MFLT_EPSILON)) {
		det = MFLOAT_C(1.0) / det;
	}
	result[0] = inverse[0] * det;
	result[1] = inverse[1] * det;
	result[2] = inverse[2] * det;
	result[3] = inverse[3] * det;
	result[4] = inverse[4] * det;
	result[5] = inverse[5] * det;
	result[6] = inverse[6] * det;
	result[7] = inverse[7] * det;
	result[8] = inverse[8] * det;
	result[9] = inverse[9] * det;
	result[10] = inverse[10] * det;
	result[11] = inverse[11] * det;
	result[12] = inverse[12] * det;
	result[13] = inverse[13] * det;
	result[14] = inverse[14] * det;
	result[15] = inverse[15] * det;
	return result;
}

mfloat_t *mat4_ortho(mfloat_t *result, mfloat_t l, mfloat_t r, mfloat_t b, mfloat_t t, mfloat_t n, mfloat_t f)
{
	result[0] = MFLOAT_C(2.0) / (r - l);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(2.0) / (t - b);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = -MFLOAT_C(2.0) / (f - n);
	result[11] = MFLOAT_C(0.0);
	result[12] = -((r + l) / (r - l));
	result[13] = -((t + b) / (t - b));
	result[14] = -((f + n) / (f - n));
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_perspective(mfloat_t *result, mfloat_t fov_y, mfloat_t aspect, mfloat_t n, mfloat_t f)
{
	mfloat_t tan_half_fov_y = MFLOAT_C(1.0) / MTAN(fov_y * MFLOAT_C(0.5));
	result[0] = MFLOAT_C(1.0) / aspect * tan_half_fov_y;
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(1.0) / tan_half_fov_y;
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = f / (n - f);
	result[11] = -MFLOAT_C(1.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = -(f * n) / (f - n);
	result[15] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *mat4_perspective_fov(mfloat_t *result, mfloat_t fov, mfloat_t w, mfloat_t h, mfloat_t n, mfloat_t f)
{
	mfloat_t h2 = MCOS(fov * MFLOAT_C(0.5)) / MSIN(fov * MFLOAT_C(0.5));
	mfloat_t w2 = h2 * h / w;
	result[0] = w2;
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = h2;
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = f / (n - f);
	result[11] = -MFLOAT_C(1.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = -(f * n) / (f - n);
	result[15] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *mat4_perspective_infinite(mfloat_t *result, mfloat_t fov_y, mfloat_t aspect, mfloat_t n)
{
	mfloat_t range = MTAN(fov_y * MFLOAT_C(0.5)) * n;
	mfloat_t left = -range * aspect;
	mfloat_t right = range * aspect;
	mfloat_t top = range;
	mfloat_t bottom = -range;
	result[0] = MFLOAT_C(2.0) * n / (right - left);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(2.0) * n / (top - bottom);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = -MFLOAT_C(1.0);
	result[11] = -MFLOAT_C(1.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = -MFLOAT_C(2.0) * n;
	result[15] = MFLOAT_C(0.0);
	return result;
}

mfloat_t *mat4_rotation_x(mfloat_t *result, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	result[0] = MFLOAT_C(1.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = c;
	result[6] = s;
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = -s;
	result[10] = c;
	result[11] = MFLOAT_C(0.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = MFLOAT_C(0.0);
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_rotation_y(mfloat_t *result, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	result[0] = c;
	result[1] = MFLOAT_C(0.0);
	result[2] = -s;
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(0.0);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = s;
	result[9] = MFLOAT_C(0.0);
	result[10] = c;
	result[11] = MFLOAT_C(0.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = MFLOAT_C(0.0);
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_rotation_z(mfloat_t *result, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	result[0] = c;
	result[1] = s;
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = -s;
	result[5] = c;
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = MFLOAT_C(1.0);
	result[11] = MFLOAT_C(0.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = MFLOAT_C(0.0);
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_rotation_axis(mfloat_t *result, mfloat_t *a, mfloat_t angle)
{
	mfloat_t c = MCOS(angle);
	mfloat_t s = MSIN(angle);
	mfloat_t one_c = MFLOAT_C(1.0) - c;
	mfloat_t x = a[0];
	mfloat_t y = a[4];
	mfloat_t z = a[8];
	mfloat_t xx = x * x;
	mfloat_t xy = x * y;
	mfloat_t xz = x * z;
	mfloat_t yy = y * y;
	mfloat_t yz = y * z;
	mfloat_t zz = z * z;
	mfloat_t l = xx + yy + zz;
	mfloat_t sqrt_l = MSQRT(l);
	result[0] = (xx + (yy + zz) * c) / l;
	result[1] = (xy * one_c + a[2] * sqrt_l * s) / l;
	result[2] = (xz * one_c - a[1] * sqrt_l * s) / l;
	result[3] = MFLOAT_C(0.0);
	result[4] = (xy * one_c - a[2] * sqrt_l * s) / l;
	result[5] = (yy + (xx + zz) * c) / l;
	result[6] = (yz * one_c + a[0] * sqrt_l * s) / l;
	result[7] = MFLOAT_C(0.0);
	result[8] = (xz * one_c + a[1] * sqrt_l * s) / l;
	result[9] = (yz * one_c - a[0] * sqrt_l * s) / l;
	result[10] = (zz + (xx + yy) * c) / l;
	result[11] = MFLOAT_C(0.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = MFLOAT_C(0.0);
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_rotation_quaternion(mfloat_t *result, mfloat_t *q)
{
	mfloat_t xx = q[0] * q[0];
	mfloat_t yy = q[1] * q[1];
	mfloat_t zz = q[2] * q[2];
	mfloat_t xy = q[0] * q[1];
	mfloat_t zw = q[2] * q[3];
	mfloat_t xz = q[8] * q[0];
	mfloat_t yw = q[1] * q[3];
	mfloat_t yz = q[1] * q[2];
	mfloat_t xw = q[0] * q[3];
	result[0] = MFLOAT_C(1.0) - MFLOAT_C(2.0) * (yy - zz);
	result[1] = MFLOAT_C(2.0) * (xy + zw);
	result[2] = MFLOAT_C(2.0) * (xz - yw);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(2.0) * (xy - zw);
	result[5] = MFLOAT_C(1.0) - MFLOAT_C(2.0) * (xx - zz);
	result[6] = MFLOAT_C(2.0) * (yz + xw);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(2.0) * (xz + yw);
	result[9] = MFLOAT_C(2.0) * (yz - xw);
	result[10] = MFLOAT_C(1.0) - MFLOAT_C(2.0) * (xx - yy);
	result[11] = MFLOAT_C(0.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = MFLOAT_C(0.0);
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_look_at(mfloat_t *result, mfloat_t *position, mfloat_t *target, mfloat_t *up)
{
	mfloat_t tmp_forward[VEC3_SIZE];
	mfloat_t tmp_side[VEC3_SIZE];
	mfloat_t tmp_up[VEC3_SIZE];
	vec3_subtract(tmp_forward, target, position);
	vec3_normalize(tmp_forward, tmp_forward);
	vec3_cross(tmp_side, tmp_forward, up);
	vec3_normalize(tmp_side, tmp_side);
	vec3_cross(tmp_up, tmp_side, tmp_forward);
	result[0] = tmp_side[0];
	result[1] = tmp_up[0];
	result[2] = -tmp_forward[0];
	result[3] = MFLOAT_C(0.0);
	result[4] = tmp_side[1];
	result[5] = tmp_up[1];
	result[6] = -tmp_forward[1];
	result[7] = MFLOAT_C(0.0);
	result[8] = tmp_side[2];
	result[9] = tmp_up[2];
	result[10] = -tmp_forward[2];
	result[11] = MFLOAT_C(0.0);
	result[12] = -vec3_dot(tmp_side, position);
	result[13] = -vec3_dot(tmp_up, position);
	result[14] = vec3_dot(tmp_forward, position);
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_translation(mfloat_t *result, mfloat_t *v)
{
	result[0] = MFLOAT_C(1.0);
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = MFLOAT_C(1.0);
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = MFLOAT_C(1.0);
	result[11] = MFLOAT_C(0.0);
	result[12] = v[0];
	result[13] = v[1];
	result[14] = v[2];
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_scaling(mfloat_t *result, mfloat_t *v)
{
	result[0] = v[0];
	result[1] = MFLOAT_C(0.0);
	result[2] = MFLOAT_C(0.0);
	result[3] = MFLOAT_C(0.0);
	result[4] = MFLOAT_C(0.0);
	result[5] = v[1];
	result[6] = MFLOAT_C(0.0);
	result[7] = MFLOAT_C(0.0);
	result[8] = MFLOAT_C(0.0);
	result[9] = MFLOAT_C(0.0);
	result[10] = v[2];
	result[11] = MFLOAT_C(0.0);
	result[12] = MFLOAT_C(0.0);
	result[13] = MFLOAT_C(0.0);
	result[14] = MFLOAT_C(0.0);
	result[15] = MFLOAT_C(1.0);
	return result;
}

mfloat_t *mat4_negative(mfloat_t *result, mfloat_t *m)
{
	result[0] = -m[0];
	result[1] = -m[1];
	result[2] = -m[2];
	result[3] = -m[3];
	result[4] = -m[4];
	result[5] = -m[5];
	result[6] = -m[6];
	result[7] = -m[7];
	result[8] = -m[8];
	result[9] = -m[9];
	result[10] = -m[10];
	result[11] = -m[11];
	result[12] = -m[12];
	result[13] = -m[13];
	result[14] = -m[14];
	result[15] = -m[15];
	return result;
}

mfloat_t *mat4_scale(mfloat_t *result, mfloat_t *m, mfloat_t scalar)
{
	result[0] = m[0] * scalar;
	result[1] = m[1] * scalar;
	result[2] = m[2] * scalar;
	result[3] = m[3] * scalar;
	result[4] = m[4] * scalar;
	result[5] = m[5] * scalar;
	result[6] = m[6] * scalar;
	result[7] = m[7] * scalar;
	result[8] = m[8] * scalar;
	result[9] = m[9] * scalar;
	result[10] = m[10] * scalar;
	result[11] = m[11] * scalar;
	result[12] = m[12] * scalar;
	result[13] = m[13] * scalar;
	result[14] = m[14] * scalar;
	result[15] = m[15] * scalar;
	return result;
}

mfloat_t *mat4_multiply(mfloat_t *result, mfloat_t *a, mfloat_t *b)
{
	mfloat_t multiplied[MAT4_SIZE];
	multiplied[0] = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
	multiplied[1] = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
	multiplied[2] = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
	multiplied[3] = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];
	multiplied[4] = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
	multiplied[5] = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
	multiplied[6] = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
	multiplied[7] = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];
	multiplied[8] = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
	multiplied[9] = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
	multiplied[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
	multiplied[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];
	multiplied[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
	multiplied[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
	multiplied[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
	multiplied[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
	result[0] = multiplied[0];
	result[1] = multiplied[1];
	result[2] = multiplied[2];
	result[3] = multiplied[3];
	result[4] = multiplied[4];
	result[5] = multiplied[5];
	result[6] = multiplied[6];
	result[7] = multiplied[7];
	result[8] = multiplied[8];
	result[9] = multiplied[9];
	result[10] = multiplied[10];
	result[11] = multiplied[11];
	result[12] = multiplied[12];
	result[13] = multiplied[13];
	result[14] = multiplied[14];
	result[15] = multiplied[15];
	return result;
}

mfloat_t *mat4_lerp(mfloat_t *result, mfloat_t *a, mfloat_t *b, mfloat_t p)
{
	result[0] = a[0] + (b[0] - a[0]) * p;
	result[1] = a[1] + (b[1] - a[1]) * p;
	result[2] = a[2] + (b[2] - a[2]) * p;
	result[3] = a[3] + (b[3] - a[3]) * p;
	result[4] = a[4] + (b[4] - a[4]) * p;
	result[5] = a[5] + (b[5] - a[5]) * p;
	result[6] = a[6] + (b[6] - a[6]) * p;
	result[7] = a[7] + (b[7] - a[7]) * p;
	result[8] = a[8] + (b[8] - a[8]) * p;
	result[9] = a[9] + (b[9] - a[9]) * p;
	result[10] = a[10] + (b[10] - a[10]) * p;
	result[11] = a[11] + (b[11] - a[11]) * p;
	result[12] = a[12] + (b[12] - a[12]) * p;
	result[13] = a[13] + (b[13] - a[13]) * p;
	result[14] = a[14] + (b[14] - a[14]) * p;
	result[15] = a[15] + (b[15] - a[15]) * p;
	return result;
}

#ifndef MATHC_NO_EASING_FUNCTIONS
/* Easing functions */
mfloat_t quadratic_ease_in(mfloat_t p)
{
	return p * p;
}

mfloat_t quadratic_ease_out(mfloat_t p)
{
	return -(p * (p - MFLOAT_C(2.0)));
}

mfloat_t quadratic_ease_in_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(2.0) * p * p;
	} else {
		f = (-MFLOAT_C(2.0) * p * p) + (MFLOAT_C(4.0) * p) - MFLOAT_C(1.0);
	}
	return f;
}

mfloat_t cubic_ease_in(mfloat_t p)
{
	return p * p * p;
}

mfloat_t cubic_ease_out(mfloat_t p)
{
	mfloat_t f = (p - MFLOAT_C(1.0));
	return f * f * f + MFLOAT_C(1.0);
}

mfloat_t cubic_ease_in_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(4.0) * p * p * p;
	} else {
		f = ((MFLOAT_C(2.0) * p) - MFLOAT_C(2.0));
		f = MFLOAT_C(0.5) * f * f * f + MFLOAT_C(1.0);
	}
	return f;
}

mfloat_t quartic_ease_in(mfloat_t p)
{
	return p * p * p * p;
}

mfloat_t quartic_ease_out(mfloat_t p)
{
	mfloat_t f = (p - MFLOAT_C(1.0));
	return f * f * f * (MFLOAT_C(1.0) - p) + MFLOAT_C(1.0);
}

mfloat_t quartic_ease_in_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(8.0) * p * p * p * p;
	} else {
		f = (p - MFLOAT_C(1.0));
		f = -MFLOAT_C(8.0) * f * f * f * f + MFLOAT_C(1.0);
	}
	return f;
}

mfloat_t quintic_ease_in(mfloat_t p)
{
	return p * p * p * p * p;
}

mfloat_t quintic_ease_out(mfloat_t p)
{
	mfloat_t f = (p - MFLOAT_C(1.0));
	return f * f * f * f * f + MFLOAT_C(1.0);
}

mfloat_t quintic_ease_in_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(16.0) * p * p * p * p * p;
	} else {
		f = ((MFLOAT_C(2.0) * p) - MFLOAT_C(2.0));
		f = MFLOAT_C(0.5) * f * f * f * f * f + MFLOAT_C(1.0);
	}
	return f;
}

mfloat_t sine_ease_in(mfloat_t p)
{
	return MSIN((p - MFLOAT_C(1.0)) * MPI_2) + MFLOAT_C(1.0);
}

mfloat_t sine_ease_out(mfloat_t p)
{
	return MSIN(p * MPI_2);
}

mfloat_t sine_ease_in_out(mfloat_t p)
{
	return MFLOAT_C(0.5) * (MFLOAT_C(1.0) - MCOS(p * MPI));
}

mfloat_t circular_ease_in(mfloat_t p)
{
	return MFLOAT_C(1.0) - MSQRT(MFLOAT_C(1.0) - (p * p));
}

mfloat_t circular_ease_out(mfloat_t p)
{
	return MSQRT((MFLOAT_C(2.0) - p) * p);
}

mfloat_t circular_ease_in_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(0.5) * (MFLOAT_C(1.0) - MSQRT(MFLOAT_C(1.0) - MFLOAT_C(4.0) * (p * p)));
	} else {
		f = MFLOAT_C(0.5) * (MSQRT(-((MFLOAT_C(2.0) * p) - MFLOAT_C(3.0)) * ((MFLOAT_C(2.0) * p) - MFLOAT_C(1.0))) + MFLOAT_C(1.0));
	}
	return f;
}

mfloat_t exponential_ease_in(mfloat_t p)
{
	mfloat_t f = p;
	if (p != MFLOAT_C(0.0)) {
		f = MPOW(MFLOAT_C(2.0), MFLOAT_C(10.0) * (p - MFLOAT_C(1.0)));
	}
	return f;
}

mfloat_t exponential_ease_out(mfloat_t p)
{
	mfloat_t f = p;
	if (p != MFLOAT_C(1.0)) {
		f = MFLOAT_C(1.0) - MPOW(MFLOAT_C(2.0), -MFLOAT_C(10.0) * p);
	}
	return f;
}

mfloat_t exponential_ease_in_out(mfloat_t p)
{
	mfloat_t f = p;
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(0.5) * MPOW(MFLOAT_C(2.0), (MFLOAT_C(20.0) * p) - MFLOAT_C(10.0));
	} else {
		f = -MFLOAT_C(0.5) * MPOW(MFLOAT_C(2.0), (-MFLOAT_C(20.0) * p) + MFLOAT_C(10.0)) + MFLOAT_C(1.0);
	}
	return f;
}

mfloat_t elastic_ease_in(mfloat_t p)
{
	return MSIN(MFLOAT_C(13.0) * MPI_2 * p) * MPOW(MFLOAT_C(2.0), MFLOAT_C(10.0) * (p - MFLOAT_C(1.0)));
}

mfloat_t elastic_ease_out(mfloat_t p)
{
	return MSIN(-MFLOAT_C(13.0) * MPI_2 * (p + MFLOAT_C(1.0))) * MPOW(MFLOAT_C(2.0), -MFLOAT_C(10.0) * p) + MFLOAT_C(1.0);
}

mfloat_t elastic_ease_in_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(0.5) * MSIN(MFLOAT_C(13.0) * MPI_2 * (MFLOAT_C(2.0) * p)) * MPOW(MFLOAT_C(2.0), MFLOAT_C(10.0) * ((MFLOAT_C(2.0) * p) - MFLOAT_C(1.0)));
	} else {
		f = MFLOAT_C(0.5) * (MSIN(-MFLOAT_C(13.0) * MPI_2 * ((MFLOAT_C(2.0) * p - MFLOAT_C(1.0)) + MFLOAT_C(1.0))) * MPOW(MFLOAT_C(2.0), -MFLOAT_C(10.0) * (MFLOAT_C(2.0) * p - MFLOAT_C(1.0))) + MFLOAT_C(2.0));
	}
	return f;
}

mfloat_t back_ease_in(mfloat_t p)
{
	return p * p * p - p * MSIN(p * MPI);
}

mfloat_t back_ease_out(mfloat_t p)
{
	mfloat_t f = (MFLOAT_C(1.0) - p);
	return MFLOAT_C(1.0) - (f * f * f - f * MSIN(f * MPI));
}

mfloat_t back_ease_in_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(2.0) * p;
		f = MFLOAT_C(0.5) * (f * f * f - f * MSIN(f * MPI));
	} else {
		f = (MFLOAT_C(1.0) - (MFLOAT_C(2.0) * p - MFLOAT_C(1.0)));
		f = MFLOAT_C(0.5) * (MFLOAT_C(1.0) - (f * f * f - f * MSIN(f * MPI))) + MFLOAT_C(0.5);
	}
	return f;
}

mfloat_t bounce_ease_in(mfloat_t p)
{
	return MFLOAT_C(1.0) - bounce_ease_out(MFLOAT_C(1.0) - p);
}

mfloat_t bounce_ease_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(4.0) / MFLOAT_C(11.0)) {
		f = (MFLOAT_C(121.0) * p * p) / MFLOAT_C(16.0);
	} else if (p < MFLOAT_C(8.0) / MFLOAT_C(11.0)) {
		f = (MFLOAT_C(363.0) / MFLOAT_C(40.0) * p * p) - (MFLOAT_C(99.0) / MFLOAT_C(10.0) * p) + MFLOAT_C(17.0) / MFLOAT_C(5.0);
	} else if (p < MFLOAT_C(9.0) / MFLOAT_C(10.0)) {
		f = (MFLOAT_C(4356.0) / MFLOAT_C(361.0) * p * p) - (MFLOAT_C(35442.0) / MFLOAT_C(1805.0) * p) + MFLOAT_C(16061.0) / MFLOAT_C(1805.0);
	} else {
		f = (MFLOAT_C(54.0) / MFLOAT_C(5.0) * p * p) - (MFLOAT_C(513.0) / MFLOAT_C(25.0) * p) + MFLOAT_C(268.0) / MFLOAT_C(25.0);
	}
	return f;
}

mfloat_t bounce_ease_in_out(mfloat_t p)
{
	mfloat_t f = MFLOAT_C(0.0);
	if (p < MFLOAT_C(0.5)) {
		f = MFLOAT_C(0.5) * bounce_ease_in(p * MFLOAT_C(2.0));
	} else {
		f = MFLOAT_C(0.5) * bounce_ease_out(p * MFLOAT_C(2.0) - MFLOAT_C(1.0)) + MFLOAT_C(0.5);
	}
	return f;
}
#endif
