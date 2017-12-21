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

#include <math.h>
#include <float.h>
#include "mathc.h"

/* Check C standard */
#ifdef __STDC__
  #define PREDEF_STANDARD_C89
  #ifdef __STDC_VERSION__
    #if __STDC_VERSION__ >= 199901L
      #define PREDEF_STANDARD_C99
    #endif
  #endif
#endif

/* Use `extern inline` for C99 or later */
#ifdef PREDEF_STANDARD_C99
#define MATHC_EXTERN_INLINE extern inline
#else
#define MATHC_EXTERN_INLINE
#endif

/* Utils */
bool nearly_equal(float a, float b, float epsilon)
{
	bool result = false;
	float abs_a = fabsf(a);
	float abs_b = fabsf(b);
	float diff = fabsf(a - b);
	if (a == b) {
		result = true;
	} else if (a == 0.0f || b == 0.0f || diff < FLT_EPSILON) {
		result = diff < epsilon;
	} else {
		result = diff / fminf(abs_a + abs_b, FLT_MAX) < epsilon;
	}
	return result;
}

float to_radians(float degrees)
{
	return degrees * M_PIF / 180.0f;
}

float to_degrees(float radians)
{
	return radians * 180.0f / M_PIF;
}

/* Vector 2D */
void to_pvector2(float x, float y, struct vec *result)
{
	result->x = x;
	result->y = y;
	result->z = 0.0f;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec to_vector2(float x, float y)
{
	struct vec result;
	to_pvector2(x, y, &result);
	return result;
}

void pvector2_zero(struct vec *result)
{
	result->x = 0.0f;
	result->y = 0.0f;
	result->z = 0.0f;
	result->w = 0.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_zero(void)
{
	struct vec result;
	pvector2_zero(&result);
	return result;
}

bool pvector2_is_zero(struct vec *a)
{
	bool is_zero = false;
	if (fabs(a->x) < FLT_EPSILON && fabs(a->y) < FLT_EPSILON) {
		is_zero = true;
	}
	return is_zero;
}

MATHC_EXTERN_INLINE bool vector2_is_zero(struct vec a)
{
	return pvector2_is_zero(&a);
}

bool pvector2_is_near_zero(struct vec *a, float epsilon)
{
	bool is_near_zero = false;
	if (fabs(a->x) < epsilon && fabs(a->y) < epsilon) {
		is_near_zero = true;
	}
	return is_near_zero;
}

MATHC_EXTERN_INLINE bool vector2_is_near_zero(struct vec a, float epsilon)
{
	return pvector2_is_near_zero(&a, epsilon);
}

bool pvector2_is_equal(struct vec *a, struct vec *b, float epsilon)
{
	bool is_equal = false;
	if (fabs(a->x - b->x) < epsilon && fabs(a->y - b->y) < FLT_EPSILON) {
		is_equal = true;
	}
	return is_equal;
}

MATHC_EXTERN_INLINE bool vector2_is_equal(struct vec a, struct vec b, float epsilon)
{
	return pvector2_is_equal(&a, &b, epsilon);
}

void pvector2_add(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x + b->x;
	result->y = a->y + b->y;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_add(struct vec a, struct vec b)
{
	struct vec result;
	pvector2_add(&a, &b, &result);
	return result;
}

void pvector2_subtract(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_subtract(struct vec a, struct vec b)
{
	struct vec result;
	pvector2_subtract(&a, &b, &result);
	return result;
}

void pvector2_scale(struct vec *a, float scale, struct vec *result)
{
	result->x = a->x * scale;
	result->y = a->y * scale;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_scale(struct vec a, float scale)
{
	struct vec result;
	pvector2_scale(&a, scale, &result);
	return result;
}

void pvector2_multiply(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x * b->x;
	result->y = a->y * b->y;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_multiply(struct vec a, struct vec b)
{
	struct vec result;
	pvector2_multiply(&a, &b, &result);
	return result;
}

void pvector2_divide(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x / b->x;
	result->y = a->y / b->y;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_divide(struct vec a, struct vec b)
{
	struct vec result;
	pvector2_divide(&a, &b, &result);
	return result;
}

void pvector2_negative(struct vec *a, struct vec *result)
{
	result->x = -a->x;
	result->y = -a->y;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_negative(struct vec a)
{
	struct vec result;
	pvector2_negative(&a, &result);
	return result;
}

void pvector2_inverse(struct vec *a, struct vec *result)
{
	if (a->x != 0.0f) {
		result->x = 1.0f / a->x;
	} else {
		result->x = 0.0f;
	}
	if (a->y != 0.0f) {
		result->y = 1.0f / a->y;
	} else {
		result->y = 0.0f;
	}
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_inverse(struct vec a)
{
	struct vec result;
	pvector2_inverse(&a, &result);
	return result;
}

void pvector2_abs(struct vec *a, struct vec *result)
{
	result->x = fabsf(a->x);
	result->y = fabsf(a->y);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_abs(struct vec a)
{
	struct vec result;
	pvector2_abs(&a, &result);
	return result;
}

void pvector2_floor(struct vec *a, struct vec *result)
{
	result->x = floorf(a->x);
	result->y = floorf(a->y);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_floor(struct vec a)
{
	struct vec result;
	pvector2_floor(&a, &result);
	return result;
}

void pvector2_ceil(struct vec *a, struct vec *result)
{
	result->x = ceilf(a->x);
	result->y = ceilf(a->y);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_ceil(struct vec a)
{
	struct vec result;
	pvector2_ceil(&a, &result);
	return result;
}

void pvector2_round(struct vec *a, struct vec *result)
{
	result->x = roundf(a->x);
	result->y = roundf(a->y);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_round(struct vec a)
{
	struct vec result;
	pvector2_round(&a, &result);
	return result;
}

void pvector2_max(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = fmaxf(a->x, b->x);
	result->y = fmaxf(a->y, b->y);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_max(struct vec a, struct vec b)
{
	struct vec result;
	pvector2_max(&a, &b, &result);
	return result;
}

void pvector2_min(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = fminf(a->x, b->x);
	result->y = fminf(a->y, b->y);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_min(struct vec a, struct vec b)
{
	struct vec result;
	pvector2_min(&a, &b, &result);
	return result;
}

float pvector2_dot(struct vec *a, struct vec *b)
{
	return a->x * b->x + a->y * b->y;
}

MATHC_EXTERN_INLINE float vector2_dot(struct vec a, struct vec b)
{
	return pvector2_dot(&a, &b);
}

float pvector2_angle(struct vec *a)
{
	return atan2f(a->y, a->x);
}

MATHC_EXTERN_INLINE float vector2_angle(struct vec a)
{
	return pvector2_angle(&a);
}

float pvector2_length_squared(struct vec *a)
{
	return a->x * a->x + a->y * a->y;
}

MATHC_EXTERN_INLINE float vector2_length_squared(struct vec a)
{
	return pvector2_length_squared(&a);
}

float pvector2_length(struct vec *a)
{
	return sqrtf(a->x * a->x + a->y * a->y);
}

MATHC_EXTERN_INLINE float vector2_length(struct vec a)
{
	return pvector2_length(&a);
}

void pvector2_normalize(struct vec *a, struct vec *result)
{
	float length = sqrtf(a->x * a->x + a->y * a->y);
	if (length > FLT_EPSILON) {
		length = 1.0f / length;
		result->x = a->x * length;
		result->y = a->y * length;
	} else {
		result->x = 0.0f;
		result->y = 0.0f;
	}
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_normalize(struct vec a)
{
	struct vec result;
	pvector2_normalize(&a, &result);
	return result;
}

void pvector2_slide(struct vec *a, struct vec *normal, struct vec *result)
{
	float d = pvector2_dot(a, normal);
	result->x = a->x - normal->x * d;
	result->y = a->y - normal->y * d;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_slide(struct vec a, struct vec normal)
{
	struct vec result;
	pvector2_slide(&a, &normal, &result);
	return result;
}

void pvector2_reflect(struct vec *a, struct vec *normal, struct vec *result)
{
	float d = 2.0f * pvector2_dot(a, normal);
	result->x = a->x - normal->x * d;
	result->y = a->y - normal->y * d;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_reflect(struct vec a, struct vec normal)
{
	struct vec result;
	pvector2_reflect(&a, &normal, &result);
	return result;
}

void pvector2_tangent(struct vec *a, struct vec *result)
{
	result->x = a->y;
	result->y = -a->x;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_tangent(struct vec a)
{
	struct vec result;
	pvector2_tangent(&a, &result);
	return result;
}

void pvector2_rotate(struct vec *a, float angle, struct vec *result)
{
	float cs = cosf(angle);
	float sn = sinf(angle);
	float x = a->x;
	float y = a->y;
	result->x = x * cs - y * sn;
	result->y = x * sn + y * cs;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_rotate(struct vec a, float angle)
{
	struct vec result;
	pvector2_rotate(&a, angle, &result);
	return result;
}

float pvector2_distance_to(struct vec *a, struct vec *b)
{
	return sqrtf((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
}

MATHC_EXTERN_INLINE float vector2_distance_to(struct vec a, struct vec b)
{
	return pvector2_distance_to(&a, &b);
}

float pvector2_distance_squared_to(struct vec *a, struct vec *b)
{
	return (a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y);
}

MATHC_EXTERN_INLINE float vector2_distance_squared_to(struct vec a, struct vec b)
{
	return pvector2_distance_squared_to(&a, &b);
}

void pvector2_linear_interpolation(struct vec *a, struct vec *b, float p, struct vec *result)
{
	result->x = a->x + (b->x - a->x) * p;
	result->y = a->y + (b->y - a->y) * p;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector2_linear_interpolation(struct vec a, struct vec b, float p)
{
	struct vec result;
	pvector2_linear_interpolation(&a, &b, p, &result);
	return result;
}

void pvector2_bezier3(struct vec *a, struct vec *b, struct vec *c, float p, struct vec *result)
{
	struct vec tmp_a;
	struct vec tmp_b;
	pvector2_linear_interpolation(a, b, p, &tmp_a);
	pvector2_linear_interpolation(b, c, p, &tmp_b);
	pvector2_linear_interpolation(&tmp_a, &tmp_b, p, result);
}

MATHC_EXTERN_INLINE struct vec vector2_bezier3(struct vec a, struct vec b, struct vec c, float p)
{
	struct vec result;
	pvector2_bezier3(&a, &b, &c, p, &result);
	return result;
}

void pvector2_bezier4(struct vec *a, struct vec *b, struct vec *c, struct vec *d, float p, struct vec *result)
{
	struct vec tmp_a;
	struct vec tmp_b;
	struct vec tmp_c;
	struct vec tmp_d;
	struct vec tmp_e;
	pvector2_linear_interpolation(a, b, p, &tmp_a);
	pvector2_linear_interpolation(b, c, p, &tmp_b);
	pvector2_linear_interpolation(c, d, p, &tmp_c);
	pvector2_linear_interpolation(&tmp_a, &tmp_b, p, &tmp_d);
	pvector2_linear_interpolation(&tmp_b, &tmp_c, p, &tmp_e);
	pvector2_linear_interpolation(&tmp_d, &tmp_e, p, result);
}

MATHC_EXTERN_INLINE struct vec vector2_bezier4(struct vec a, struct vec b, struct vec c, struct vec d, float p)
{
	struct vec result;
	pvector2_bezier4(&a, &b, &c, &d, p, &result);
	return result;
}

/* Vector 3D */
void to_pvector3(float x, float y, float z, struct vec *result)
{
	result->x = x;
	result->y = y;
	result->z = z;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec to_vector3(float x, float y, float z)
{
	struct vec result;
	to_pvector3(x, y, z, &result);
	return result;
}

void pvector3_zero(struct vec *result)
{
	result->x = 0.0f;
	result->y = 0.0f;
	result->z = 0.0f;
	result->w = 0.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_zero(void)
{
	struct vec result;
	pvector3_zero(&result);
	return result;
}

bool pvector3_is_zero(struct vec *a)
{
	bool is_zero = false;
	if (fabs(a->x) < FLT_EPSILON && fabs(a->y) < FLT_EPSILON && fabs(a->z) < FLT_EPSILON) {
		is_zero = true;
	}
	return is_zero;
}

MATHC_EXTERN_INLINE bool vector3_is_zero(struct vec a)
{
	return pvector3_is_zero(&a);
}

bool pvector3_is_near_zero(struct vec *a, float epsilon)
{
	bool is_near_zero = false;
	if (fabs(a->x) < epsilon && fabs(a->y) < epsilon && fabs(a->z) < epsilon) {
		is_near_zero = true;
	}
	return is_near_zero;
}

MATHC_EXTERN_INLINE bool vector3_is_near_zero(struct vec a, float epsilon)
{
	return pvector3_is_near_zero(&a, epsilon);
}

bool pvector3_is_equal(struct vec *a, struct vec *b, float epsilon)
{
	bool is_equal = false;
	if (fabs(a->x - b->x) < epsilon && fabs(a->y - b->y) < FLT_EPSILON && fabs(a->z - b->z) < FLT_EPSILON) {
		is_equal = true;
	}
	return is_equal;
}

MATHC_EXTERN_INLINE bool vector3_is_equal(struct vec a, struct vec b, float epsilon)
{
	return pvector3_is_equal(&a, &b, epsilon);
}

void pvector3_add(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x + b->x;
	result->y = a->y + b->y;
	result->z = a->z + b->z;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_add(struct vec a, struct vec b)
{
	struct vec result;
	pvector3_add(&a, &b, &result);
	return result;
}

void pvector3_subtract(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->z = a->z - b->z;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_subtract(struct vec a, struct vec b)
{
	struct vec result;
	pvector3_subtract(&a, &b, &result);
	return result;
}

void pvector3_scale(struct vec *a, float scale, struct vec *result)
{
	result->x = a->x * scale;
	result->y = a->y * scale;
	result->z = a->z * scale;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_scale(struct vec a, float scale)
{
	struct vec result;
	pvector3_scale(&a, scale, &result);
	return result;
}

void pvector3_multiply(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x * b->x;
	result->y = a->y * b->y;
	result->z = a->z * b->z;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_multiply(struct vec a, struct vec b)
{
	struct vec result;
	pvector3_multiply(&a, &b, &result);
	return result;
}

void pvector3_divide(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x / b->x;
	result->y = a->y / b->y;
	result->z = a->z / b->z;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_divide(struct vec a, struct vec b)
{
	struct vec result;
	pvector3_divide(&a, &b, &result);
	return result;
}

void pvector3_negative(struct vec *a, struct vec *result)
{
	result->x = -a->x;
	result->y = -a->y;
	result->z = -a->z;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_negative(struct vec a)
{
	struct vec result;
	pvector3_negative(&a, &result);
	return result;
}

void pvector3_inverse(struct vec *a, struct vec *result)
{
	if (a->x != 0.0f) {
		result->x = 1.0f / a->x;
	} else {
		result->x = 0.0f;
	}
	if (a->y != 0.0f) {
		result->y = 1.0f / a->y;
	} else {
		result->y = 0.0f;
	}
	if (a->z != 0.0f) {
		result->z = 1.0f / a->z;
	} else {
		result->z = 0.0f;
	}
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_inverse(struct vec a)
{
	struct vec result;
	pvector3_inverse(&a, &result);
	return result;
}

void pvector3_abs(struct vec *a, struct vec *result)
{
	result->x = fabsf(a->x);
	result->y = fabsf(a->y);
	result->z = fabsf(a->z);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_abs(struct vec a)
{
	struct vec result;
	pvector3_abs(&a, &result);
	return result;
}

void pvector3_floor(struct vec *a, struct vec *result)
{
	result->x = floorf(a->x);
	result->y = floorf(a->y);
	result->z = floorf(a->z);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_floor(struct vec a)
{
	struct vec result;
	pvector3_floor(&a, &result);
	return result;
}

void pvector3_ceil(struct vec *a, struct vec *result)
{
	result->x = ceilf(a->x);
	result->y = ceilf(a->y);
	result->z = ceilf(a->z);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_ceil(struct vec a)
{
	struct vec result;
	pvector3_ceil(&a, &result);
	return result;
}

void pvector3_round(struct vec *a, struct vec *result)
{
	result->x = roundf(a->x);
	result->y = roundf(a->y);
	result->z = roundf(a->z);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_round(struct vec a)
{
	struct vec result;
	pvector3_round(&a, &result);
	return result;
}

void pvector3_max(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = fmaxf(a->x, b->x);
	result->y = fmaxf(a->y, b->y);
	result->z = fmaxf(a->z, b->z);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_max(struct vec a, struct vec b)
{
	struct vec result;
	pvector3_max(&a, &b, &result);
	return result;
}

void pvector3_min(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = fminf(a->x, b->x);
	result->y = fminf(a->y, b->y);
	result->z = fminf(a->z, b->z);
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_min(struct vec a, struct vec b)
{
	struct vec result;
	pvector3_min(&a, &b, &result);
	return result;
}

float pvector3_dot(struct vec *a, struct vec *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z;
}

MATHC_EXTERN_INLINE float vector3_dot(struct vec a, struct vec b)
{
	return pvector3_dot(&a, &b);
}

void pvector3_cross(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->y * b->z - a->z * b->y;
	result->y = a->z * b->x - a->x * b->z;
	result->z = a->x * b->y - a->y * b->x;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_cross(struct vec a, struct vec b)
{
	struct vec result;
	pvector3_cross(&a, &b, &result);
	return result;
}

float pvector3_length_squared(struct vec *a)
{
	return a->x * a->x + a->y * a->y + a->z * a->z;
}

MATHC_EXTERN_INLINE float vector3_length_squared(struct vec a)
{
	return pvector3_length_squared(&a);
}

float pvector3_length(struct vec *a)
{
	return sqrtf(a->x * a->x + a->y * a->y + a->z * a->z);
}

MATHC_EXTERN_INLINE float vector3_length(struct vec a)
{
	return pvector3_length(&a);
}

void pvector3_normalize(struct vec *a, struct vec *result)
{
	float length = sqrtf(a->x * a->x + a->y * a->y + a->z * a->z);
	if (length > FLT_EPSILON) {
		length = 1.0f / length;
		result->x = a->x * length;
		result->y = a->y * length;
		result->z = a->z * length;
	} else {
		result->x = 0.0f;
		result->y = 0.0f;
		result->z = 0.0f;
	}
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_normalize(struct vec a)
{
	struct vec result;
	pvector3_normalize(&a, &result);
	return result;
}

void pvector3_slide(struct vec *a, struct vec *normal, struct vec *result)
{
	float d = pvector3_dot(a, normal);
	result->x = a->x - normal->x * d;
	result->y = a->y - normal->y * d;
	result->y = a->z - normal->z * d;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_slide(struct vec a, struct vec normal)
{
	struct vec result;
	pvector3_slide(&a, &normal, &result);
	return result;
}

void pvector3_reflect(struct vec *a, struct vec *normal, struct vec *result)
{
	float d = 2.0f * pvector3_dot(a, normal);
	result->x = a->x - normal->x * d;
	result->y = a->y - normal->y * d;
	result->z = a->z - normal->z * d;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_reflect(struct vec a, struct vec normal)
{
	struct vec result;
	pvector3_reflect(&a, &normal, &result);
	return result;
}

float pvector3_distance_to(struct vec *a, struct vec *b)
{
	return sqrtf((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y) + (a->z - b->z) * (a->z - b->z));
}

MATHC_EXTERN_INLINE float vector3_distance_to(struct vec a, struct vec b)
{
	return pvector3_distance_to(&a, &b);
}

float pvector3_distance_squared_to(struct vec *a, struct vec *b)
{
	return (a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y) + (a->z - b->z) * (a->z - b->z);
}

MATHC_EXTERN_INLINE float vector3_distance_squared_to(struct vec a, struct vec b)
{
	return pvector3_distance_squared_to(&a, &b);
}

void pvector3_linear_interpolation(struct vec *a, struct vec *b, float p, struct vec *result)
{
	result->x = a->x + (b->x - a->x) * p;
	result->y = a->y + (b->y - a->y) * p;
	result->z = a->z + (b->z - a->z) * p;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec vector3_linear_interpolation(struct vec a, struct vec b, float p)
{
	struct vec result;
	pvector3_linear_interpolation(&a, &b, p, &result);
	return result;
}

void pvector3_bezier3(struct vec *a, struct vec *b, struct vec *c, float p, struct vec *result)
{
	struct vec tmp_a;
	struct vec tmp_b;
	pvector3_linear_interpolation(a, b, p, &tmp_a);
	pvector3_linear_interpolation(b, c, p, &tmp_b);
	pvector3_linear_interpolation(&tmp_a, &tmp_b, p, result);
}

MATHC_EXTERN_INLINE struct vec vector3_bezier3(struct vec a, struct vec b, struct vec c, float p)
{
	struct vec result;
	pvector3_bezier3(&a, &b, &c, p, &result);
	return result;
}

void pvector3_bezier4(struct vec *a, struct vec *b, struct vec *c, struct vec *d, float p, struct vec *result)
{
	struct vec tmp_a;
	struct vec tmp_b;
	struct vec tmp_c;
	struct vec tmp_d;
	struct vec tmp_e;
	pvector3_linear_interpolation(a, b, p, &tmp_a);
	pvector3_linear_interpolation(b, c, p, &tmp_b);
	pvector3_linear_interpolation(c, d, p, &tmp_c);
	pvector3_linear_interpolation(&tmp_a, &tmp_b, p, &tmp_d);
	pvector3_linear_interpolation(&tmp_b, &tmp_c, p, &tmp_e);
	pvector3_linear_interpolation(&tmp_d, &tmp_e, p, result);
}

MATHC_EXTERN_INLINE struct vec vector3_bezier4(struct vec a, struct vec b, struct vec c, struct vec d, float p)
{
	struct vec result;
	pvector3_bezier4(&a, &b, &c, &d, p, &result);
	return result;
}

/* Quaternion */
void to_pquaternion(float x, float y, float z, float w, struct vec *result)
{
	result->x = x;
	result->y = y;
	result->z = z;
	result->w = w;
}

struct vec to_quaternion(float x, float y, float z, float w)
{
	struct vec result;
	to_pquaternion(x, y, z, w, &result);
	return result;
}


void pquaternion_zero(struct vec *result)
{
	result->x = 0.0f;
	result->y = 0.0f;
	result->z = 0.0f;
	result->w = 0.0f;
}

MATHC_EXTERN_INLINE struct vec quaternion_zero(void)
{
	struct vec result;
	pquaternion_zero(&result);
	return result;
}

bool pquaternion_is_zero(struct vec *a)
{
	bool is_zero = false;
	if (fabs(a->x) < FLT_EPSILON && fabs(a->y) < FLT_EPSILON && fabs(a->z) < FLT_EPSILON && fabs(a->w) < FLT_EPSILON) {
		is_zero = true;
	}
	return is_zero;
}

MATHC_EXTERN_INLINE bool quaternion_is_zero(struct vec a)
{
	return pvector3_is_zero(&a);
}

bool pquaternion_is_near_zero(struct vec *a, float epsilon)
{
	bool is_near_zero = false;
	if (fabs(a->x) < epsilon && fabs(a->y) < epsilon && fabs(a->z) < epsilon && fabs(a->w) < epsilon) {
		is_near_zero = true;
	}
	return is_near_zero;
}

MATHC_EXTERN_INLINE bool quaternion_is_near_zero(struct vec a, float epsilon)
{
	return pquaternion_is_near_zero(&a, epsilon);
}

bool pquaternion_is_equal(struct vec *a, struct vec *b, float epsilon)
{
	bool is_equal = false;
	if (fabs(a->x - b->x) < epsilon && fabs(a->y - b->y) < FLT_EPSILON && fabs(a->z - b->z) < FLT_EPSILON) {
		is_equal = true;
	}
	return is_equal;
}

MATHC_EXTERN_INLINE bool quaternion_is_equal(struct vec a, struct vec b, float epsilon)
{
	return pquaternion_is_equal(&a, &b, epsilon);
}

void pquaternion_add(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x + b->x;
	result->y = a->y + b->y;
	result->z = a->z + b->z;
	result->w = a->w + b->w;
}

MATHC_EXTERN_INLINE struct vec quaternion_add(struct vec a, struct vec b)
{
	struct vec result;
	pquaternion_add(&a, &b, &result);
	return result;
}

void pquaternion_subtract(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->y = a->z - b->z;
	result->w = a->w - b->w;
}

MATHC_EXTERN_INLINE struct vec quaternion_subtract(struct vec a, struct vec b)
{
	struct vec result;
	pquaternion_subtract(&a, &b, &result);
	return result;
}

void pquaternion_scale(struct vec *a, float scale, struct vec *result)
{
	result->x = a->x * scale;
	result->y = a->y * scale;
	result->z = a->z * scale;
	result->w = a->w * scale;
}

MATHC_EXTERN_INLINE struct vec quaternion_scale(struct vec a, float scale)
{
	struct vec result;
	pquaternion_scale(&a, scale, &result);
	return result;
}

void pquaternion_multiply(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y;
	result->y = a->w * b->y + a->y * b->w + a->z * b->x - a->x * b->z;
	result->z = a->w * b->z + a->z * b->w + a->x * b->y - a->y * b->x;
	result->w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z;
}

MATHC_EXTERN_INLINE struct vec quaternion_multiply(struct vec a, struct vec b)
{
	struct vec result;
	pquaternion_multiply(&a, &b, &result);
	return result;
}

void pquaternion_divide(struct vec *a, struct vec *b, struct vec *result)
{
	float x = a->x;
	float y = a->y;
	float z = a->z;
	float w = a->w;
	float length_squared = 1.0f / (b->x * b->x + b->y * b->y + b->z * b->z + b->w * b->w);
	float normalized_x = -b->x * length_squared;
	float normalized_y = -b->y * length_squared;
	float normalized_z = -b->z * length_squared;
	float normalized_w = b->w * length_squared;
	result->x = x * normalized_w + normalized_x * w + (y * normalized_z - z * normalized_y);
	result->y = y * normalized_w + normalized_y * w + (z * normalized_x - x * normalized_z);
	result->z = z * normalized_w + normalized_z * w + (x * normalized_y - y * normalized_x);
	result->w = w * normalized_w - (x * normalized_x + y * normalized_y + z * normalized_z);
}

MATHC_EXTERN_INLINE struct vec quaternion_divide(struct vec a, struct vec b)
{
	struct vec result;
	pquaternion_divide(&a, &b, &result);
	return result;
}

void pquaternion_negative(struct vec *a, struct vec *result)
{
	result->x = -a->x;
	result->y = -a->y;
	result->z = -a->z;
	result->w = -a->w;
}

MATHC_EXTERN_INLINE struct vec quaternion_negative(struct vec a)
{
	struct vec result;
	pquaternion_negative(&a, &result);
	return result;
}

void pquaternion_conjugate(struct vec *a, struct vec *result)
{
	result->x = -a->x;
	result->y = -a->y;
	result->z = -a->z;
	result->w = a->w;
}

MATHC_EXTERN_INLINE struct vec quaternion_conjugate(struct vec a)
{
	struct vec result;
	pquaternion_conjugate(&a, &result);
	return result;
}

void pquaternion_inverse(struct vec *a, struct vec *result)
{
	float length = sqrtf(a->x * a->x + a->y * a->y + a->z * a->z + a->w * a->w);
	if (fabs(length) > FLT_EPSILON) {
		length = 1.0f / length;
	} else {
		length = 0.0f;
	}
	result->x = -a->x * length;
	result->y = -a->y * length;
	result->z = -a->z * length;
	result->w = a->w * length;
}

MATHC_EXTERN_INLINE struct vec quaternion_inverse(struct vec a)
{
	struct vec result;
	pquaternion_inverse(&a, &result);
	return result;
}

void pquaternion_abs(struct vec *a, struct vec *result)
{
	result->x = fabsf(a->x);
	result->y = fabsf(a->y);
	result->z = fabsf(a->z);
	result->w = fabsf(a->w);
}

MATHC_EXTERN_INLINE struct vec quaternion_abs(struct vec a)
{
	struct vec result;
	pquaternion_abs(&a, &result);
	return result;
}

void pquaternion_floor(struct vec *a, struct vec *result)
{
	result->x = floorf(a->x);
	result->y = floorf(a->y);
	result->z = floorf(a->z);
	result->w = floorf(a->w);
}

MATHC_EXTERN_INLINE struct vec quaternion_floor(struct vec a)
{
	struct vec result;
	pquaternion_floor(&a, &result);
	return result;
}

void pquaternion_ceil(struct vec *a, struct vec *result)
{
	result->x = ceilf(a->x);
	result->y = ceilf(a->y);
	result->z = ceilf(a->z);
	result->w = ceilf(a->w);
}

MATHC_EXTERN_INLINE struct vec quaternion_ceil(struct vec a)
{
	struct vec result;
	pquaternion_ceil(&a, &result);
	return result;
}

void pquaternion_round(struct vec *a, struct vec *result)
{
	result->x = roundf(a->x);
	result->y = roundf(a->y);
	result->z = roundf(a->z);
	result->w = roundf(a->w);
}

MATHC_EXTERN_INLINE struct vec quaternion_round(struct vec a)
{
	struct vec result;
	pquaternion_round(&a, &result);
	return result;
}

void pquaternion_max(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = fmaxf(a->x, b->x);
	result->y = fmaxf(a->y, b->y);
	result->z = fmaxf(a->z, b->z);
	result->w = fmaxf(a->w, b->w);
}

MATHC_EXTERN_INLINE struct vec quaternion_max(struct vec a, struct vec b)
{
	struct vec result;
	pquaternion_max(&a, &b, &result);
	return result;
}

void pquaternion_min(struct vec *a, struct vec *b, struct vec *result)
{
	result->x = fminf(a->x, b->x);
	result->y = fminf(a->y, b->y);
	result->z = fminf(a->z, b->z);
	result->w = fminf(a->w, b->w);
}

MATHC_EXTERN_INLINE struct vec quaternion_min(struct vec a, struct vec b)
{
	struct vec result;
	pquaternion_min(&a, &b, &result);
	return result;
}

float pquaternion_dot(struct vec *a, struct vec *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

MATHC_EXTERN_INLINE float quaternion_dot(struct vec a, struct vec b)
{
	return pquaternion_dot(&a, &b);
}

float pquaternion_angle(struct vec *a, struct vec *b)
{
	float s = sqrtf(pquaternion_length_squared(a) * pquaternion_length_squared(b));
	if (fabs(s) > FLT_EPSILON) {
		s = 1.0f / s;
	} else {
		s = 0.0f;
	}
	return acosf(pquaternion_dot(a, b) * s);
}

MATHC_EXTERN_INLINE float quaternion_angle(struct vec a, struct vec b)
{
	return pquaternion_angle(&a, &b);
}

float pquaternion_length_squared(struct vec *a)
{
	return a->x * a->x + a->y * a->y + a->z * a->z + a->w * a->w;
}

MATHC_EXTERN_INLINE float quaternion_length_squared(struct vec a)
{
	return pquaternion_length_squared(&a);
}

float pquaternion_length(struct vec *a)
{
	return sqrtf(a->x * a->x + a->y * a->y + a->z * a->z + a->w * a->w);
}

MATHC_EXTERN_INLINE float quaternion_length(struct vec a)
{
	return pquaternion_length(&a);
}

void pquaternion_normalize(struct vec *a, struct vec *result)
{
	float length = 1.0f / sqrtf(a->x * a->x + a->y * a->y + a->z * a->z + a->w * a->w);
	if (length > FLT_EPSILON) {
		result->x = a->x * length;
		result->y = a->y * length;
		result->z = a->z * length;
		result->w = a->w * length;
	} else {
		result->x = 0.0f;
		result->y = 0.0f;
		result->z = 0.0f;
		result->w = 1.0f;
	}
}

MATHC_EXTERN_INLINE struct vec quaternion_normalize(struct vec a)
{
	struct vec result;
	pquaternion_normalize(&a, &result);
	return result;
}

void pquaternion_power(struct vec *a, float exponent, struct vec *result)
{
	if (fabsf(a->w) < 0.9999f) {
		float alpha = acosf(a->w);
		float new_alpha = alpha * exponent;
		float s = sinf(new_alpha) / sinf(alpha);
		result->x = result->x * s;
		result->y = result->y * s;
		result->z = result->z * s;
		result->w = cosf(new_alpha);
	} else {
		result->x = a->x;
		result->y = a->y;
		result->z = a->z;
		result->w = a->w;
	}
}

MATHC_EXTERN_INLINE struct vec quaternion_power(struct vec a, float exponent)
{
	struct vec result;
	pquaternion_power(&a, exponent, &result);
	return result;
}

void pquaternion_from_axis_angle(struct vec *a, float angle, struct vec *result)
{
	float half = angle * 0.5f;
	float s = sinf(half);
	result->x = a->x * s;
	result->y = a->y * s;
	result->z = a->z * s;
	result->w = cosf(half);
	pquaternion_normalize(result, result);
}

MATHC_EXTERN_INLINE struct vec quaternion_from_axis_angle(struct vec a, float angle)
{
	struct vec result;
	pquaternion_from_axis_angle(&a, angle, &result);
	return result;
}

void pquaternion_to_axis_angle(struct vec *a, struct vec *result)
{
	float sa;
	struct vec tmp;
	pquaternion_normalize(a, &tmp);
	sa = sqrtf(1.0f - tmp.w * tmp.w);
	if (fabsf(sa) <= FLT_EPSILON) {
		sa = 1.0f;
	}
	result->x = tmp.x / sa;
	result->y = tmp.y / sa;
	result->z = tmp.z / sa;
	result->w = acosf(tmp.w) * 2.0f;
}

MATHC_EXTERN_INLINE struct vec quaternion_to_axis_angle(struct vec a)
{
	struct vec result;
	pquaternion_to_axis_angle(&a, &result);
	return result;
}

void pquaternion_rotation_matrix(struct mat *m, struct vec *result)
{
	float sr;
	float half;
	float scale = m->m11 + m->m22 + m->m33;
	if (scale > 0.0f) {
		sr = sqrtf(scale + 1.0f);
		result->w = sr * 0.5f;
		sr = 0.5f / sr;
		result->x = (m->m23 - m->m32) * sr;
		result->y = (m->m31 - m->m13) * sr;
		result->z = (m->m12 - m->m21) * sr;
	} else if ((m->m11 >= m->m22) && (m->m11 >= m->m33)) {
		sr = sqrtf(1.0f + m->m11 - m->m22 - m->m33);
		half = 0.5f / sr;
		result->x = 0.5f * sr;
		result->y = (m->m12 + m->m21) * half;
		result->z = (m->m13 + m->m31) * half;
		result->w = (m->m23 - m->m32) * half;
	} else if (m->m22 > m->m33) {
		sr = sqrtf(1.0f + m->m22 - m->m11 - m->m33);
		half = 0.5f / sr;
		result->x = (m->m21 + m->m12) * half;
		result->y = 0.5f * sr;
		result->z = (m->m32 + m->m23) * half;
		result->w = (m->m31 - m->m13) * half;
	} else {
		sr = sqrtf(1.0f + m->m33 - m->m11 - m->m22);
		half = 0.5f / sr;
		result->x = (m->m31 + m->m13) * half;
		result->y = (m->m32 + m->m23) * half;
		result->z = 0.5f * sr;
		result->w = (m->m12 - m->m21) * half;
	}
}

MATHC_EXTERN_INLINE struct vec quaternion_rotation_matrix(struct mat m)
{
	struct vec result;
	pquaternion_rotation_matrix(&m, &result);
	return result;
}

void pquaternion_yaw_pitch_roll(float yaw, float pitch, float roll, struct vec *result)
{
	float half_roll = roll * 0.5f;
	float half_pitch = pitch * 0.5f;
	float half_yaw = yaw * 0.5f;
	float sin_roll = sinf(half_roll);
	float cos_roll = cosf(half_roll);
	float sin_pitch = sinf(half_pitch);
	float cos_pitch = cosf(half_pitch);
	float sin_yaw = sinf(half_yaw);
	float cos_yaw = cosf(half_yaw);
	result->x = cos_yaw * sin_pitch * cos_roll + sin_yaw * cos_pitch * sin_roll;
	result->y = sin_yaw * cos_pitch * cos_roll - cos_yaw * sin_pitch * sin_roll;
	result->z = cos_yaw * cos_pitch * sin_roll - sin_yaw * sin_pitch * cos_roll;
	result->w = cos_yaw * cos_pitch * cos_roll + sin_yaw * sin_pitch * sin_roll;
}

MATHC_EXTERN_INLINE struct vec quaternion_yaw_pitch_roll(float yaw, float pitch, float roll)
{
	struct vec result;
	pquaternion_yaw_pitch_roll(yaw, pitch, roll, &result);
	return result;
}

void pquaternion_linear_interpolation(struct vec *a, struct vec *b, float p, struct vec *result)
{
	result->x = a->x + (b->x - a->x) * p;
	result->y = a->y + (b->y - a->y) * p;
	result->z = a->z + (b->z - a->z) * p;
	result->w = a->w + (b->w - a->w) * p;
}

MATHC_EXTERN_INLINE struct vec quaternion_linear_interpolation(struct vec a, struct vec b, float p)
{
	struct vec result;
	pquaternion_linear_interpolation(&a, &b, p, &result);
	return result;
}

void pquaternion_spherical_linear_interpolation(struct vec *a, struct vec *b, float p, struct vec *result)
{
	struct vec tmp_a = *a;
	struct vec tmp_b = *b;
	float cos_theta = pquaternion_dot(a, b);
	float k0;
	float k1;
	/* Take shortest arc */
	if (cos_theta < 0.0f) {
		pquaternion_negative(&tmp_b, &tmp_b);
		cos_theta = -cos_theta;
	}
	/* Check if quaternions are close */
	if (cos_theta > 0.9999f) {
		/* Use linear interpolation */
		k0 = 1.0f - p;
		k1 = p;
	} else {
		float theta = acosf(cos_theta);
		float sin_theta = sinf(theta);
		k0 = sinf((1.f - p) * theta) / sin_theta;
		k1 = sinf(p * theta) / sin_theta;
	}
	result->x = tmp_a.x * k0 + tmp_b.x * k1;
	result->y = tmp_a.y * k0 + tmp_b.y * k1;
	result->z = tmp_a.z * k0 + tmp_b.z * k1;
	result->w = tmp_a.w * k0 + tmp_b.w * k1;
}

MATHC_EXTERN_INLINE struct vec quaternion_spherical_linear_interpolation(struct vec a, struct vec b, float p)
{
	struct vec result;
	pquaternion_spherical_linear_interpolation(&a, &b, p, &result);
	return result;
}

/* Matrix */
void pmatrix_zero(struct mat *result)
{
	result->m11 = 0.0f;
	result->m12 = 0.0f;
	result->m13 = 0.0f;
	result->m14 = 0.0f;
	result->m21 = 0.0f;
	result->m22 = 0.0f;
	result->m23 = 0.0f;
	result->m24 = 0.0f;
	result->m31 = 0.0f;
	result->m32 = 0.0f;
	result->m33 = 0.0f;
	result->m34 = 0.0f;
	result->m41 = 0.0f;
	result->m42 = 0.0f;
	result->m43 = 0.0f;
	result->m44 = 0.0f;
}

MATHC_EXTERN_INLINE struct mat matrix_zero(void)
{
	struct mat result;
	pmatrix_zero(&result);
	return result;
}

void pmatrix_identity(struct mat *result)
{
	result->m11 = 1.0f;
	result->m12 = 0.0f;
	result->m13 = 0.0f;
	result->m14 = 0.0f;
	result->m21 = 0.0f;
	result->m22 = 1.0f;
	result->m23 = 0.0f;
	result->m24 = 0.0f;
	result->m31 = 0.0f;
	result->m32 = 0.0f;
	result->m33 = 1.0f;
	result->m34 = 0.0f;
	result->m41 = 0.0f;
	result->m42 = 0.0f;
	result->m43 = 0.0f;
	result->m44 = 1.0f;
}

MATHC_EXTERN_INLINE struct mat matrix_identity(void)
{
	struct mat result;
	pmatrix_identity(&result);
	return result;
}

void pmatrix_transpose(struct mat *m, struct mat *result)
{
	result->m11 = m->m11;
	result->m21 = m->m12;
	result->m31 = m->m13;
	result->m41 = m->m14;
	result->m12 = m->m21;
	result->m22 = m->m22;
	result->m32 = m->m23;
	result->m42 = m->m24;
	result->m13 = m->m31;
	result->m23 = m->m32;
	result->m33 = m->m33;
	result->m43 = m->m34;
	result->m14 = m->m41;
	result->m24 = m->m42;
	result->m34 = m->m43;
	result->m44 = m->m44;
}

struct mat matrix_transpose(struct mat m)
{
	struct mat result;
	pmatrix_transpose(&m, &result);
	return result;
}

void pmatrix_inverse(struct mat *m, struct mat *result)
{
	struct mat inv;
	float det;
	inv.m11 = m->m22 * m->m33 * m->m44 -
		m->m22 * m->m43 * m->m34 -
		m->m23 * m->m32 * m->m44 +
		m->m23 * m->m42 * m->m34 +
		m->m24 * m->m32 * m->m43 -
		m->m24 * m->m42 * m->m33;
	inv.m12 = -m->m12 * m->m33 * m->m44 +
		m->m12 * m->m43 * m->m34 +
		m->m13 * m->m32 * m->m44 -
		m->m13 * m->m42 * m->m34 -
		m->m14 * m->m32 * m->m43 +
		m->m14 * m->m42 * m->m33;
	inv.m13 = m->m12 * m->m23 * m->m44 -
		m->m12 * m->m43 * m->m24 -
		m->m13 * m->m22 * m->m44 +
		m->m13 * m->m42 * m->m24 +
		m->m14 * m->m22 * m->m43 -
		m->m14 * m->m42 * m->m23;
	inv.m14 = -m->m12 * m->m23 * m->m34 +
		m->m12 * m->m33 * m->m24 +
		m->m13 * m->m22 * m->m34 -
		m->m13 * m->m32 * m->m24 -
		m->m14 * m->m22 * m->m33 +
		m->m14 * m->m32 * m->m23;
	inv.m21 = -m->m21 * m->m33 * m->m44 +
		m->m21 * m->m43 * m->m34 +
		m->m23 * m->m31 * m->m44 -
		m->m23 * m->m41 * m->m34 -
		m->m24 * m->m31 * m->m43 +
		m->m24 * m->m41 * m->m33;
	inv.m22 = m->m11 * m->m33 * m->m44 -
		m->m11 * m->m43 * m->m34 -
		m->m13 * m->m31 * m->m44 +
		m->m13 * m->m41 * m->m34 +
		m->m14 * m->m31 * m->m43 -
		m->m14 * m->m41 * m->m33;
	inv.m23 = -m->m11 * m->m23 * m->m44 +
		m->m11 * m->m43 * m->m24 +
		m->m13 * m->m21 * m->m44 -
		m->m13 * m->m41 * m->m24 -
		m->m14 * m->m21 * m->m43 +
		m->m14 * m->m41 * m->m23;
	inv.m24 = m->m11 * m->m23 * m->m34 -
		m->m11 * m->m33 * m->m24 -
		m->m13 * m->m21 * m->m34 +
		m->m13 * m->m31 * m->m24 +
		m->m14 * m->m21 * m->m33 -
		m->m14 * m->m31 * m->m23;
	inv.m31 = m->m21 * m->m32 * m->m44 -
		m->m21 * m->m42 * m->m34 -
		m->m22 * m->m31 * m->m44 +
		m->m22 * m->m41 * m->m34 +
		m->m24 * m->m31 * m->m42 -
		m->m24 * m->m41 * m->m32;
	inv.m32 = -m->m11 * m->m32 * m->m44 +
		m->m11 * m->m42 * m->m34 +
		m->m12 * m->m31 * m->m44 -
		m->m12 * m->m41 * m->m34 -
		m->m14 * m->m31 * m->m42 +
		m->m14 * m->m41 * m->m32;
	inv.m33 = m->m11 * m->m22 * m->m44 -
		m->m11 * m->m42 * m->m24 -
		m->m12 * m->m21 * m->m44 +
		m->m12 * m->m41 * m->m24 +
		m->m14 * m->m21 * m->m42 -
		m->m14 * m->m41 * m->m22;
	inv.m34 = -m->m11 * m->m22 * m->m34 +
		m->m11 * m->m32 * m->m24 +
		m->m12 * m->m21 * m->m34 -
		m->m12 * m->m31 * m->m24 -
		m->m14 * m->m21 * m->m32 +
		m->m14 * m->m31 * m->m22;
	inv.m41 = -m->m21 * m->m32 * m->m43 +
		m->m21 * m->m42 * m->m33 +
		m->m22 * m->m31 * m->m43 -
		m->m22 * m->m41 * m->m33 -
		m->m23 * m->m31 * m->m42 +
		m->m23 * m->m41 * m->m32;
	inv.m42 = m->m11 * m->m32 * m->m43 -
		m->m11 * m->m42 * m->m33 -
		m->m12 * m->m31 * m->m43 +
		m->m12 * m->m41 * m->m33 +
		m->m13 * m->m31 * m->m42 -
		m->m13 * m->m41 * m->m32;
	inv.m43 = -m->m11 * m->m22 * m->m43 +
		m->m11 * m->m42 * m->m23 +
		m->m12 * m->m21 * m->m43 -
		m->m12 * m->m41 * m->m23 -
		m->m13 * m->m21 * m->m42 +
		m->m13 * m->m41 * m->m22;
	inv.m44 = m->m11 * m->m22 * m->m33 -
		m->m11 * m->m32 * m->m23 -
		m->m12 * m->m21 * m->m33 +
		m->m12 * m->m31 * m->m23 +
		m->m13 * m->m21 * m->m32 -
		m->m13 * m->m31 * m->m22;
	det = m->m11 * inv.m11 + m->m21 * inv.m12 + m->m31 * inv.m13 + m->m41 * inv.m14;
	/* Matrix can not be inverted if det == 0 */
	if (det != 0) {
		det = 1.0f / det;
	}
	result->m11 = inv.m11 * det;
	result->m21 = inv.m21 * det;
	result->m31 = inv.m31 * det;
	result->m41 = inv.m41 * det;
	result->m12 = inv.m12 * det;
	result->m22 = inv.m22 * det;
	result->m32 = inv.m32 * det;
	result->m42 = inv.m42 * det;
	result->m13 = inv.m13 * det;
	result->m23 = inv.m23 * det;
	result->m33 = inv.m33 * det;
	result->m43 = inv.m43 * det;
	result->m14 = inv.m14 * det;
	result->m24 = inv.m24 * det;
	result->m34 = inv.m34 * det;
	result->m44 = inv.m44 * det;
}

struct mat matrix_inverse(struct mat m)
{
	struct mat result;
	pmatrix_inverse(&m, &result);
	return result;
}

#include <stdio.h>

void pmatrix_ortho(float l, float r, float b, float t, float n, float f, struct mat *result)
{
	pmatrix_identity(result);
	result->m11 = 2.0f / (r - l);
	result->m22 = 2.0f / (t - b);
	result->m33 = -2.0f / (f - n);
	result->m14 = -((r + l) / (r - l));
	result->m24 = -((t + b) / (t - b));
	result->m34 = -((f + n) / (f - n));
}

MATHC_EXTERN_INLINE struct mat matrix_ortho(float l, float r, float b, float t, float n, float f)
{
	struct mat result;
	pmatrix_ortho(l, r, b, t, n, f, &result);
	return result;
}

void pmatrix_perspective(float fov_y, float aspect, float n, float f, struct mat *result)
{
	const float tan_half_fov_y = 1.0f / tanf(fov_y * 0.5f);
	pmatrix_zero(result);
	result->m11 = 1.0f / aspect * tan_half_fov_y;
	result->m22 = 1.0f / tan_half_fov_y;
	result->m33 = f / (n - f);
	result->m43 = -1.0f;
	result->m34 = -(f * n) / (f - n);
}

MATHC_EXTERN_INLINE struct mat matrix_perspective(float fov_y, float aspect, float n, float f)
{
	struct mat result;
	pmatrix_perspective(fov_y, aspect, n, f, &result);
	return result;
}

void pmatrix_perspective_fov(float fov, float w, float h, float n, float f, struct mat *result)
{
	const float h2 = cosf(fov * 0.5f) / sinf(fov * 0.5f);
	const float w2 = h2 * h / w;
	pmatrix_zero(result);
	result->m11 = w2;
	result->m22 = h2;
	result->m33 = f / (n - f);
	result->m43 = -1.0f;
	result->m34 = -(f * n) / (f - n);
}

MATHC_EXTERN_INLINE struct mat matrix_perspective_fov(float fov, float w, float h, float n, float f)
{
	struct mat result;
	pmatrix_perspective_fov(fov, w, h, n, f, &result);
	return result;
}

void pmatrix_perspective_infinite(float fov_y, float aspect, float n, struct mat *result)
{
	const float range = tanf(fov_y * 0.5f) * n;
	const float left = -range * aspect;
	const float right = range * aspect;
	const float top = range;
	const float bottom = -range;
	pmatrix_zero(result);
	result->m11 = 2.0f * n / (right - left);
	result->m22 = 2.0f * n / (top - bottom);
	result->m33 = -1.0f;
	result->m43 = -1.0f;
	result->m34 = -2.0f * n;
}

MATHC_EXTERN_INLINE struct mat matrix_perspective_infinite(float fov_y, float aspect, float n)
{
	struct mat result;
	pmatrix_perspective_infinite(fov_y, aspect, n, &result);
	return result;
}

void pmatrix_rotation_x(float angle, struct mat *result)
{
	pmatrix_identity(result);
	float c = cosf(angle);
	float s = sinf(angle);
	result->m22 = c;
	result->m23 = -s;
	result->m32 = s;
	result->m33 = c;
}

MATHC_EXTERN_INLINE struct mat matrix_rotation_x(float angle)
{
	struct mat result;
	pmatrix_rotation_x(angle, &result);
	return result;
}

void pmatrix_rotation_y(float angle, struct mat *result)
{
	pmatrix_identity(result);
	float c = cosf(angle);
	float s = sinf(angle);
	result->m11 = c;
	result->m13 = s;
	result->m31 = -s;
	result->m33 = c;
}

MATHC_EXTERN_INLINE struct mat matrix_rotation_y(float angle)
{
	struct mat result;
	pmatrix_rotation_y(angle, &result);
	return result;
}

void pmatrix_rotation_z(float angle, struct mat *result)
{
	pmatrix_identity(result);
	float c = cosf(angle);
	float s = sinf(angle);
	result->m11 = c;
	result->m12 = -s;
	result->m21 = s;
	result->m22 = c;
}

MATHC_EXTERN_INLINE struct mat matrix_rotation_z(float angle)
{
	struct mat result;
	pmatrix_rotation_z(angle, &result);
	return result;
}

void pmatrix_rotation_axis(struct vec *a, float angle, struct mat *result)
{
	pmatrix_identity(result);
	float c = cosf(angle);
	float s = sinf(angle);
	float one_c = 1.0f - c;
	float x = a->x;
	float y = a->y;
	float z = a->z;
	float xx = x * x;
	float xy = x * y;
	float xz = x * z;
	float yy = y * y;
	float yz = y * z;
	float zz = z * z;
	float l = xx + yy + zz;
	float sqrt_l = sqrtf(l);
	pmatrix_identity(result);
	result->m11 = (xx + (yy + zz) * c) / l;
	result->m12 = (xy * one_c - a->z * sqrt_l * s) / l;
	result->m13 = (xz * one_c + a->y * sqrt_l * s) / l;
	result->m21 = (xy * one_c + a->z * sqrt_l * s) / l;
	result->m22 = (yy + (xx + zz) * c) / l;
	result->m23 = (yz * one_c - a->x * sqrt_l * s) / l;
	result->m31 = (xz * one_c - a->y * sqrt_l * s) / l;
	result->m32 = (yz * one_c + a->x * sqrt_l * s) / l;
	result->m33 = (zz + (xx + yy) * c) / l;
}

MATHC_EXTERN_INLINE struct mat matrix_rotation_axis(struct vec a, float angle)
{
	struct mat result;
	pmatrix_rotation_axis(&a, angle, &result);
	return result;
}

void pmatrix_rotation_quaternion(struct vec *q, struct mat *result)
{
	pmatrix_identity(result);
	float xx = q->x * q->x;
	float yy = q->y * q->y;
	float zz = q->z * q->z;
	float xy = q->x * q->y;
	float zw = q->z * q->w;
	float xz = q->z * q->x;
	float yw = q->y * q->w;
	float yz = q->y * q->z;
	float xw = q->x * q->w;
	result->m11 = 1.0f - 2.0f * (yy - zz);
	result->m12 = 2.0f * (xy - zw);
	result->m13 = 2.0f * (xz + yw);
	result->m21 = 2.0f * (xy + zw);
	result->m22 = 1.0f - 2.0f * (xx - zz);
	result->m23 = 2.0f * (yz - xw);
	result->m31 = 2.0f * (xz - yw);
	result->m32 = 2.0f * (yz + xw);
	result->m33 = 1.0f - 2.0f * (xx - yy);
}

MATHC_EXTERN_INLINE struct mat matrix_rotation_quaternion(struct vec q)
{
	struct mat result;
	pmatrix_rotation_quaternion(&q, &result);
	return result;
}

void pmatrix_look_at_up(struct vec *position, struct vec *target, struct vec *up_axis, struct mat *result)
{
	struct vec forward_axis;
	struct vec side_axis;
	pvector3_subtract(target, position, &forward_axis);
	pvector3_normalize(&forward_axis, &forward_axis);
	pvector3_cross(&forward_axis, up_axis, &side_axis);
	pvector3_normalize(&side_axis, &side_axis);
	pvector3_cross(&side_axis, &forward_axis, up_axis);
	pmatrix_identity(result);
	result->m11 = side_axis.x;
	result->m12 = side_axis.y;
	result->m13 = side_axis.z;
	result->m21 = up_axis->x;
	result->m22 = up_axis->y;
	result->m23 = up_axis->z;
	result->m31 = -forward_axis.x;
	result->m32 = -forward_axis.y;
	result->m33 = -forward_axis.z;
	result->m14 = -pvector3_dot(&side_axis, position);
	result->m24 = -pvector3_dot(up_axis, position);
	result->m34 = pvector3_dot(&forward_axis, position);
}

MATHC_EXTERN_INLINE struct mat matrix_look_at_up(struct vec pos, struct vec target, struct vec up_axis)
{
	struct mat result;
	pmatrix_look_at_up(&pos, &target, &up_axis, &result);
	return result;
}

void pmatrix_look_at(struct vec *position, struct vec *target, struct mat *result)
{
	struct vec forward_axis;
	struct vec side_axis;
	struct vec up_axis;
	pvector3_subtract(target, position, &forward_axis);
	pvector3_normalize(&forward_axis, &forward_axis);
	if (fabsf(forward_axis.x) < FLT_EPSILON && fabsf(forward_axis.z) < FLT_EPSILON) {
		if (forward_axis.y > 0.0f) {
			to_pvector3(0.0f, 0.0f, -1.0f, &up_axis);
		} else {
			to_pvector3(0.0f, 0.0f, 1.0f, &up_axis);
		}
	} else {
		to_pvector3(0.0f, 1.0f, 0.0f, &up_axis);
	}
	pvector3_cross(&forward_axis, &up_axis, &side_axis);
	pvector3_normalize(&side_axis, &side_axis);
	pvector3_cross(&side_axis, &forward_axis, &up_axis);
	pmatrix_identity(result);
	result->m11 = side_axis.x;
	result->m12 = side_axis.y;
	result->m13 = side_axis.z;
	result->m21 = up_axis.x;
	result->m22 = up_axis.y;
	result->m23 = up_axis.z;
	result->m31 = -forward_axis.x;
	result->m32 = -forward_axis.y;
	result->m33 = -forward_axis.z;
	result->m14 = -pvector3_dot(&side_axis, position);
	result->m24 = -pvector3_dot(&up_axis, position);
	result->m34 = pvector3_dot(&forward_axis, position);
}

MATHC_EXTERN_INLINE struct mat matrix_look_at(struct vec pos, struct vec target)
{
	struct mat result;
	pmatrix_look_at(&pos, &target, &result);
	return result;
}

void pmatrix_scale(struct vec *v, struct mat *result)
{
	pmatrix_identity(result);
	result->m11 = v->x;
	result->m22 = v->y;
	result->m33 = v->z;
}

MATHC_EXTERN_INLINE struct mat matrix_scale(struct vec v)
{
	struct mat result;
	pmatrix_scale(&v, &result);
	return result;
}

void pmatrix_get_scale(struct mat *m, struct vec *result)
{
	result->x = m->m11;
	result->y = m->m22;
	result->z = m->m33;
}

MATHC_EXTERN_INLINE struct vec matrix_get_scale(struct mat m)
{
	struct vec result;
	pmatrix_get_scale(&m, &result);
	return result;
}

void pmatrix_translation(struct vec *v, struct mat *result)
{
	pmatrix_identity(result);
	result->m14 = v->x;
	result->m24 = v->y;
	result->m34 = v->z;
}

MATHC_EXTERN_INLINE struct mat matrix_translation(struct vec v)
{
	struct mat result;
	pmatrix_translation(&v, &result);
	return result;
}

void pmatrix_get_translation(struct mat *m, struct vec *result)
{
	result->x = m->m14;
	result->y = m->m24;
	result->z = m->m34;
	result->w = 1.0f;
}

MATHC_EXTERN_INLINE struct vec matrix_get_translation(struct mat m)
{
	struct vec result;
	pmatrix_get_translation(&m, &result);
	return result;
}

void pmatrix_negative(struct mat *m, struct mat *result)
{
	result->m11 = -m->m11;
	result->m12 = -m->m12;
	result->m13 = -m->m13;
	result->m14 = -m->m14;
	result->m21 = -m->m21;
	result->m22 = -m->m22;
	result->m23 = -m->m23;
	result->m24 = -m->m24;
	result->m31 = -m->m31;
	result->m32 = -m->m32;
	result->m33 = -m->m33;
	result->m34 = -m->m34;
	result->m41 = -m->m41;
	result->m42 = -m->m42;
	result->m43 = -m->m43;
	result->m44 = -m->m44;
}

MATHC_EXTERN_INLINE struct mat matrix_negative(struct mat m)
{
	struct mat result;
	pmatrix_negative(&m, &result);
	return result;
}

void pmatrix_multiply(struct mat *m, float s, struct mat *result)
{
	result->m11 = m->m11 * s;
	result->m12 = m->m12 * s;
	result->m13 = m->m13 * s;
	result->m14 = m->m14 * s;
	result->m21 = m->m21 * s;
	result->m22 = m->m22 * s;
	result->m23 = m->m23 * s;
	result->m24 = m->m24 * s;
	result->m31 = m->m31 * s;
	result->m32 = m->m32 * s;
	result->m33 = m->m33 * s;
	result->m34 = m->m34 * s;
	result->m41 = m->m41 * s;
	result->m42 = m->m42 * s;
	result->m43 = m->m43 * s;
	result->m44 = m->m44 * s;
}

MATHC_EXTERN_INLINE struct mat matrix_multiply(struct mat m, float s)
{
	struct mat result;
	pmatrix_multiply(&m, s, &result);
	return result;
}

void pmatrix_multiply_matrix(struct mat *a, struct mat *b, struct mat *result)
{
	result->m11 = a->m11 * b->m11 + a->m12 * b->m21 + a->m13 * b->m31 + a->m14 * b->m41;
	result->m12 = a->m11 * b->m12 + a->m12 * b->m22 + a->m13 * b->m32 + a->m14 * b->m42;
	result->m13 = a->m11 * b->m13 + a->m12 * b->m23 + a->m13 * b->m33 + a->m14 * b->m43;
	result->m14 = a->m11 * b->m14 + a->m12 * b->m24 + a->m13 * b->m34 + a->m14 * b->m44;
	result->m21 = a->m21 * b->m11 + a->m22 * b->m21 + a->m23 * b->m31 + a->m24 * b->m41;
	result->m22 = a->m21 * b->m12 + a->m22 * b->m22 + a->m23 * b->m32 + a->m24 * b->m42;
	result->m23 = a->m21 * b->m13 + a->m22 * b->m23 + a->m23 * b->m33 + a->m24 * b->m43;
	result->m24 = a->m21 * b->m14 + a->m22 * b->m24 + a->m23 * b->m34 + a->m24 * b->m44;
	result->m31 = a->m31 * b->m11 + a->m32 * b->m21 + a->m33 * b->m31 + a->m34 * b->m41;
	result->m32 = a->m31 * b->m12 + a->m32 * b->m22 + a->m33 * b->m32 + a->m34 * b->m42;
	result->m33 = a->m31 * b->m13 + a->m32 * b->m23 + a->m33 * b->m33 + a->m34 * b->m43;
	result->m34 = a->m31 * b->m14 + a->m32 * b->m24 + a->m33 * b->m34 + a->m34 * b->m44;
	result->m41 = a->m41 * b->m11 + a->m42 * b->m21 + a->m43 * b->m31 + a->m44 * b->m41;
	result->m42 = a->m41 * b->m12 + a->m42 * b->m22 + a->m43 * b->m32 + a->m44 * b->m42;
	result->m43 = a->m41 * b->m13 + a->m42 * b->m23 + a->m43 * b->m33 + a->m44 * b->m43;
	result->m44 = a->m41 * b->m14 + a->m42 * b->m24 + a->m43 * b->m34 + a->m44 * b->m44;
}

MATHC_EXTERN_INLINE struct mat matrix_multiply_matrix(struct mat a, struct mat b)
{
	struct mat result;
	pmatrix_multiply_matrix(&a, &b, &result);
	return result;
}

void pmatrix_linear_interpolation(struct mat *a, struct mat *b, float p, struct mat *result)
{
	result->m11 = a->m11 + (b->m11 - a->m11) * p;
	result->m12 = a->m12 + (b->m12 - a->m12) * p;
	result->m13 = a->m13 + (b->m13 - a->m13) * p;
	result->m14 = a->m14 + (b->m14 - a->m14) * p;
	result->m21 = a->m21 + (b->m21 - a->m21) * p;
	result->m22 = a->m22 + (b->m22 - a->m22) * p;
	result->m23 = a->m23 + (b->m23 - a->m23) * p;
	result->m24 = a->m24 + (b->m24 - a->m24) * p;
	result->m31 = a->m31 + (b->m31 - a->m31) * p;
	result->m32 = a->m32 + (b->m32 - a->m32) * p;
	result->m33 = a->m33 + (b->m33 - a->m33) * p;
	result->m34 = a->m34 + (b->m34 - a->m34) * p;
	result->m41 = a->m41 + (b->m41 - a->m41) * p;
	result->m42 = a->m42 + (b->m42 - a->m42) * p;
	result->m43 = a->m43 + (b->m43 - a->m43) * p;
	result->m44 = a->m44 + (b->m44 - a->m44) * p;
}

MATHC_EXTERN_INLINE struct mat matrix_linear_interpolation(struct mat a, struct mat b, float p)
{
	struct mat result;
	pmatrix_linear_interpolation(&a, &b, p, &result);
	return result;
}

void pmatrix_multiply_f4(struct mat *m, float *result)
{
	float v0 = result[0];
	float v1 = result[1];
	float v2 = result[2];
	float v3 = result[3];
	result[0] = (m->m11 * v0) + (m->m12 * v1) + (m->m13 * v2) + (m->m14 * v3);
	result[1] = (m->m21 * v0) + (m->m22 * v1) + (m->m23 * v2) + (m->m24 * v3);
	result[2] = (m->m31 * v0) + (m->m32 * v1) + (m->m33 * v2) + (m->m34 * v3);
	result[3] = (m->m41 * v0) + (m->m42 * v1) + (m->m43 * v2) + (m->m44 * v3);
}

MATHC_EXTERN_INLINE void matrix_multiply_f4(struct mat m, float *result)
{
	pmatrix_multiply_f4(&m, result);
}

void pmatrix_to_array(struct mat *m, float *result)
{
	result[0] = m->m11;
	result[1] = m->m21;
	result[2] = m->m31;
	result[3] = m->m41;
	result[4] = m->m12;
	result[5] = m->m22;
	result[6] = m->m32;
	result[7] = m->m42;
	result[8] = m->m13;
	result[9] = m->m23;
	result[10] = m->m33;
	result[11] = m->m43;
	result[12] = m->m14;
	result[13] = m->m24;
	result[14] = m->m34;
	result[15] = m->m44;
}

MATHC_EXTERN_INLINE void matrix_to_array(struct mat m, float *result)
{
	pmatrix_to_array(&m, result);
}

/* Easing functions */
float quadratic_ease_in(float p)
{
	return p * p;
}

float quadratic_ease_out(float p)
{
	return -(p * (p - 2.0f));
}

float quadratic_ease_in_out(float p)
{
	float f = 0.0f;
	if (p < 0.5f) {
		f = 2.0f * p * p;
	} else {
		f = (-2.0f * p * p) + (4.0f * p) - 1.0f;
	}
	return f;
}

float cubic_ease_in(float p)
{
	return p * p * p;
}

float cubic_ease_out(float p)
{
	float f = (p - 1.0f);
	return f * f * f + 1.0f;
}

float cubic_ease_in_out(float p)
{
	float f = 0.0f;
	if (p < 0.5f) {
		f = 4.0f * p * p * p;
	} else {
		f = ((2.0f * p) - 2.0f);
		f = 0.5f * f * f * f + 1.0f;
	}
	return f;
}

float quartic_ease_in(float p)
{
	return p * p * p * p;
}

float quartic_ease_out(float p)
{
	float f = (p - 1.0f);
	return f * f * f * (1.0f - p) + 1.0f;
}

float quartic_ease_in_out(float p)
{
	float f = 0.0f;
	if (p < 0.5f) {
		f = 8.0f * p * p * p * p;
	} else {
		f = (p - 1.0f);
		f = -8.0f * f * f * f * f + 1.0f;
	}
	return f;
}

float quintic_ease_in(float p)
{
	return p * p * p * p * p;
}

float quintic_ease_out(float p)
{
	float f = (p - 1.0f);
	return f * f * f * f * f + 1.0f;
}

float quintic_ease_in_out(float p)
{
	float f = 0.0f;
	if (p < 0.5f) {
		f = 16.0f * p * p * p * p * p;
	} else {
		f = ((2.0f * p) - 2.0f);
		f = 0.5f * f * f * f * f * f + 1.0f;
	}
	return f;
}

float sine_ease_in(float p)
{
	return sinf((p - 1.0f) * M_PIF_2) + 1.0f;
}

float sine_ease_out(float p)
{
	return sinf(p * M_PIF_2);
}

float sine_ease_in_out(float p)
{
	return 0.5f * (1.0f - cosf(p * M_PIF));
}

float circular_ease_in(float p)
{
	return 1.0f - sqrtf(1.0f - (p * p));
}

float circular_ease_out(float p)
{
	return sqrtf((2.0f - p) * p);
}

float circular_ease_in_out(float p)
{
	float f = 0.0f;
	if (p < 0.5f) {
		f = 0.5f * (1.0f - sqrtf(1.0f - 4.0f * (p * p)));
	} else {
		f = 0.5f * (sqrtf(-((2.0f * p) - 3.0f) * ((2.0f * p) - 1.0f)) + 1.0f);
	}
	return f;
}

float exponential_ease_in(float p)
{
	float f = p;
	if (p != 0.0f) {
		f = powf(2.0f, 10.0f * (p - 1.0f));
	}
	return f;
}

float exponential_ease_out(float p)
{
	float f = p;
	if (p != 1.0f) {
		f = 1 - powf(2.0f, -10.0f * p);
	}
	return f;
}

float exponential_ease_in_out(float p)
{
	float f = p;
	if (p == 0.0f || p == 1.0f) {
		f = p;
	} else if (p < 0.5f) {
		f = 0.5f * powf(2.0f, (20.0f * p) - 10.0f);
	} else {
		f = -0.5f * powf(2.0f, (-20.0f * p) + 10.0f) + 1.0f;
	}
	return f;
}

float elastic_ease_in(float p)
{
	return sinf(13.0f * M_PIF_2 * p) * powf(2.0f, 10.0f * (p - 1.0f));
}

float elastic_ease_out(float p)
{
	return sinf(-13.0f * M_PIF_2 * (p + 1.0f)) * powf(2.0f, -10.0f * p) + 1.0f;
}

float elastic_ease_in_out(float p)
{
	float f = 0.0f;
	if (p < 0.5f) {
		f = 0.5f * sinf(13.0f * M_PIF_2 * (2.0f * p)) * powf(2.0f, 10.0f * ((2.0f * p) - 1.0f));
	} else {
		f = 0.5f * (sinf(-13.0f * M_PIF_2 * ((2.0f * p - 1.0f) + 1.0f)) * powf(2.0f, -10.0f * (2.0f * p - 1.0f)) + 2.0f);
	}
	return f;
}

float back_ease_in(float p)
{
	return p * p * p - p * sinf(p * M_PIF);
}

float back_ease_out(float p)
{
	float f = (1.0f - p);
	return 1.0f - (f * f * f - f * sinf(f * M_PIF));
}

float back_ease_in_out(float p)
{
	float f = 0.0f;
	if (p < 0.5f) {
		f = 2.0f * p;
		f = 0.5f * (f * f * f - f * sinf(f * M_PIF));
	} else {
		f = (1.0f - (2.0f * p - 1.0f));
		f = 0.5f * (1.0f - (f * f * f - f * sinf(f * M_PIF))) + 0.5f;
	}
	return f;
}

float bounce_ease_in(float p)
{
	return 1.0f - bounce_ease_out(1.0f - p);
}

float bounce_ease_out(float p)
{
	float f = 0.0f;
	if (p < 4.0f / 11.0f) {
		f = (121.0f * p * p) / 16.0f;
	} else if (p < 8.0f / 11.0f) {
		f = (363.0f / 40.0f * p * p) - (99.0f / 10.0f * p) + 17.0f / 5.0f;
	} else if (p < 9.0f / 10.0f) {
		f = (4356.0f / 361.0f * p * p) - (35442.0f / 1805.0f * p) + 16061 / 1805.0f;
	} else {
		f = (54.0f / 5.0f * p * p) - (513.0f / 25.0f * p) + 268.0f / 25.0f;
	}
	return f;
}

float bounce_ease_in_out(float p)
{
	float f = 0.0f;
	if (p < 0.5f) {
		f = 0.5f * bounce_ease_in(p * 2.0f);
	} else {
		f = 0.5f * bounce_ease_out(p * 2.0f - 1.0f) + 0.5f;
	}
	return f;
}
