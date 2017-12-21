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

#include <stdbool.h>

#define MATHC_MAJOR_VERSION 1
#define MATHC_MINOR_VERSION 2
#define MATHC_PATCH_VERSION 0
#define M_PIF 3.1415926536f
#define M_PIF_2 1.5707963268f
#define MAT_SIZE 16

struct vec {
	float x;
	float y;
	float z;
	float w;
};

struct mat {
	/* Row x Column */
	float m11;
	float m21;
	float m31;
	float m41;
	float m12;
	float m22;
	float m32;
	float m42;
	float m13;
	float m23;
	float m33;
	float m43;
	float m14;
	float m24;
	float m34;
	float m44;
};

/* Utils */
bool nearly_equal(float a, float b, float epsilon);
float to_radians(float degrees);
float to_degrees(float radians);

/* Vector 2D */
void to_pvector2(float x, float y, struct vec *result);
void pvector2_zero(struct vec *result);
bool pvector2_is_zero(struct vec *a);
bool pvector2_is_near_zero(struct vec *a, float epsilon);
bool pvector2_is_equal(struct vec *a, struct vec *b, float epsilon);
void pvector2_add(struct vec *a, struct vec *b, struct vec *result);
void pvector2_subtract(struct vec *a, struct vec *b, struct vec *result);
void pvector2_scale(struct vec *a, float scale, struct vec *result);
void pvector2_multiply(struct vec *a, struct vec *b, struct vec *result);
void pvector2_divide(struct vec *a, struct vec *b, struct vec *result);
void pvector2_negative(struct vec *a, struct vec *result);
void pvector2_inverse(struct vec *a, struct vec *result);
void pvector2_abs(struct vec *a, struct vec *result);
void pvector2_floor(struct vec *a, struct vec *result);
void pvector2_ceil(struct vec *a, struct vec *result);
void pvector2_round(struct vec *a, struct vec *result);
void pvector2_max(struct vec *a, struct vec *b, struct vec *result);
void pvector2_min(struct vec *a, struct vec *b, struct vec *result);
float pvector2_dot(struct vec *a, struct vec *b);
float pvector2_angle(struct vec *a);
float pvector2_length_squared(struct vec *a);
float pvector2_length(struct vec *a);
void pvector2_normalize(struct vec *a, struct vec *result);
void pvector2_slide(struct vec *a, struct vec *b, struct vec *result);
void pvector2_reflect(struct vec *a, struct vec *b, struct vec *result);
void pvector2_tangent(struct vec *a, struct vec *result);
void pvector2_rotate(struct vec *a, float angle, struct vec *result);
float pvector2_distance_to(struct vec *a, struct vec *b);
float pvector2_distance_squared_to(struct vec *a, struct vec *b);
void pvector2_linear_interpolation(struct vec *a, struct vec *b, float p, struct vec *result);
void pvector2_bezier3(struct vec *a, struct vec *b, struct vec *c, float p, struct vec *result);
void pvector2_bezier4(struct vec *a, struct vec *b, struct vec *c, struct vec *d, float p, struct vec *result);

struct vec to_vector2(float x, float y);
struct vec vector2_zero(void);
bool vector2_is_zero(struct vec a);
bool vector2_is_near_zero(struct vec a, float epsilon);
bool vector2_is_equal(struct vec a, struct vec b, float epsilon);
struct vec vector2_add(struct vec a, struct vec b);
struct vec vector2_subtract(struct vec a, struct vec b);
struct vec vector2_scale(struct vec a, float scale);
struct vec vector2_multiply(struct vec a, struct vec b);
struct vec vector2_divide(struct vec a, struct vec b);
struct vec vector2_negative(struct vec a);
struct vec vector2_inverse(struct vec a);
struct vec vector2_abs(struct vec a);
struct vec vector2_floor(struct vec a);
struct vec vector2_ceil(struct vec a);
struct vec vector2_round(struct vec a);
struct vec vector2_max(struct vec a, struct vec b);
struct vec vector2_min(struct vec a, struct vec b);
float vector2_dot(struct vec a, struct vec b);
float vector2_angle(struct vec a);
float vector2_length_squared(struct vec a);
float vector2_length(struct vec a);
struct vec vector2_normalize(struct vec a);
struct vec vector2_slide(struct vec a, struct vec normal);
struct vec vector2_reflect(struct vec a, struct vec normal);
struct vec vector2_tangent(struct vec a);
struct vec vector2_rotate(struct vec a, float angle);
float vector2_distance_to(struct vec a, struct vec b);
float vector2_distance_squared_to(struct vec a, struct vec b);
struct vec vector2_linear_interpolation(struct vec a, struct vec b, float p);
struct vec vector2_bezier3(struct vec a, struct vec b, struct vec c, float p);
struct vec vector2_bezier4(struct vec a, struct vec b, struct vec c, struct vec d, float p);

/* Vector 3D */
void to_pvector3(float x, float y, float z, struct vec *result);
void pvector3_zero(struct vec *result);
bool pvector3_is_zero(struct vec *a);
bool pvector3_is_near_zero(struct vec *a, float epsilon);
bool pvector3_is_equal(struct vec *a, struct vec *b, float epsilon);
void pvector3_add(struct vec *a, struct vec *b, struct vec *result);
void pvector3_subtract(struct vec *a, struct vec *b, struct vec *result);
void pvector3_scale(struct vec *a, float scale, struct vec *result);
void pvector3_multiply(struct vec *a, struct vec *b, struct vec *result);
void pvector3_divide(struct vec *a, struct vec *b, struct vec *result);
void pvector3_negative(struct vec *a, struct vec *result);
void pvector3_inverse(struct vec *a, struct vec *result);
void pvector3_abs(struct vec *a, struct vec *result);
void pvector3_floor(struct vec *a, struct vec *result);
void pvector3_ceil(struct vec *a, struct vec *result);
void pvector3_round(struct vec *a, struct vec *result);
void pvector3_max(struct vec *a, struct vec *b, struct vec *result);
void pvector3_min(struct vec *a, struct vec *b, struct vec *result);
float pvector3_dot(struct vec *a, struct vec *b);
void pvector3_cross(struct vec *a, struct vec *b, struct vec *result);
float pvector3_length_squared(struct vec *a);
float pvector3_length(struct vec *a);
void pvector3_normalize(struct vec *a, struct vec *result);
void pvector3_slide(struct vec *a, struct vec *b, struct vec *result);
void pvector3_reflect(struct vec *a, struct vec *normal, struct vec *result);
float pvector3_distance_to(struct vec *a, struct vec *b);
float pvector3_distance_squared_to(struct vec *a, struct vec *b);
void pvector3_linear_interpolation(struct vec *a, struct vec *b, float p, struct vec *result);
void pvector3_bezier3(struct vec *a, struct vec *b, struct vec *c, float p, struct vec *result);
void pvector3_bezier4(struct vec *a, struct vec *b, struct vec *c, struct vec *d, float p, struct vec *result);

struct vec to_vector3(float x, float y, float z);
struct vec vector3_zero(void);
struct vec vector3_add(struct vec a, struct vec b);
bool vector3_is_zero(struct vec a);
bool vector3_is_near_zero(struct vec a, float epsilon);
bool vector3_is_equal(struct vec a, struct vec b, float epsilon);
struct vec vector3_subtract(struct vec a, struct vec b);
struct vec vector3_scale(struct vec a, float scale);
struct vec vector3_multiply(struct vec a, struct vec b);
struct vec vector3_divide(struct vec a, struct vec b);
struct vec vector3_negative(struct vec a);
struct vec vector3_inverse(struct vec a);
struct vec vector3_abs(struct vec a);
struct vec vector3_floor(struct vec a);
struct vec vector3_ceil(struct vec a);
struct vec vector3_round(struct vec a);
struct vec vector3_max(struct vec a, struct vec b);
struct vec vector3_min(struct vec a, struct vec b);
float vector3_dot(struct vec a, struct vec b);
struct vec vector3_cross(struct vec a, struct vec b);
float vector3_length_squared(struct vec a);
float vector3_length(struct vec a);
struct vec vector3_normalize(struct vec a);
struct vec vector3_slide(struct vec a, struct vec b);
struct vec vector3_reflect(struct vec a, struct vec normal);
float vector3_distance_to(struct vec a, struct vec b);
float vector3_distance_squared_to(struct vec a, struct vec b);
struct vec vector3_linear_interpolation(struct vec a, struct vec b, float p);
struct vec vector3_bezier3(struct vec a, struct vec b, struct vec c, float p);
struct vec vector3_bezier4(struct vec a, struct vec b, struct vec c, struct vec d, float p);

/* Quaternion */
void to_pquaternion(float x, float y, float z, float w, struct vec *result);
void pquaternion_zero(struct vec *result);
void pquaternion_null(struct vec *result);
bool pquaternion_is_zero(struct vec *a);
bool pquaternion_is_near_zero(struct vec *a, float epsilon);
bool pquaternion_is_equal(struct vec *a, struct vec *b, float epsilon);
void pquaternion_add(struct vec *a, struct vec *b, struct vec *result);
void pquaternion_subtract(struct vec *a, struct vec *b, struct vec *result);
void pquaternion_scale(struct vec *a, float scale, struct vec *result);
void pquaternion_multiply(struct vec *a, struct vec *b, struct vec *result);
void pquaternion_divide(struct vec *a, struct vec *b, struct vec *result);
void pquaternion_negative(struct vec *a, struct vec *result);
void pquaternion_conjugate(struct vec *a, struct vec *result);
void pquaternion_inverse(struct vec *a, struct vec *result);
void pquaternion_abs(struct vec *a, struct vec *result);
void pquaternion_floor(struct vec *a, struct vec *result);
void pquaternion_ceil(struct vec *a, struct vec *result);
void pquaternion_round(struct vec *a, struct vec *result);
void pquaternion_max(struct vec *a, struct vec *b, struct vec *result);
void pquaternion_min(struct vec *a, struct vec *b, struct vec *result);
float pquaternion_dot(struct vec *a, struct vec *b);
float pquaternion_angle(struct vec *a, struct vec *b);
float pquaternion_length_squared(struct vec *a);
float pquaternion_length(struct vec *a);
void pquaternion_normalize(struct vec *a, struct vec *result);
void pquaternion_power(struct vec *a, float exponent, struct vec *result);
void pquaternion_from_axis_angle(struct vec *a, float angle, struct vec *result);
void pquaternion_to_axis_angle(struct vec *a, struct vec *result);
void pquaternion_from_2_vectors(struct vec *a, struct vec *b, struct vec *result);
void pquaternion_rotation_matrix(struct mat *m, struct vec *result);
void pquaternion_yaw_pitch_roll(float yaw, float pitch, float roll, struct vec *result);
void pquaternion_linear_interpolation(struct vec *a, struct vec *b, float p, struct vec *result);
void pquaternion_spherical_linear_interpolation(struct vec *a, struct vec *b, float p, struct vec *result);

struct vec to_quaternion(float x, float y, float z, float w);
struct vec quaternion_zero(void);
struct vec quaternion_null(void);
bool quaternion_is_zero(struct vec a);
bool quaternion_is_near_zero(struct vec a, float epsilon);
bool quaternion_is_equal(struct vec a, struct vec b, float epsilon);
struct vec quaternion_add(struct vec a, struct vec b);
struct vec quaternion_subtract(struct vec a, struct vec b);
struct vec quaternion_scale(struct vec a, float scale);
struct vec quaternion_multiply(struct vec a, struct vec b);
struct vec quaternion_divide(struct vec a, struct vec b);
struct vec quaternion_negative(struct vec a);
struct vec quaternion_conjugate(struct vec a);
struct vec quaternion_inverse(struct vec a);
struct vec quaternion_abs(struct vec a);
struct vec quaternion_floor(struct vec a);
struct vec quaternion_ceil(struct vec a);
struct vec quaternion_round(struct vec a);
struct vec quaternion_max(struct vec a, struct vec b);
struct vec quaternion_min(struct vec a, struct vec b);
float quaternion_dot(struct vec a, struct vec b);
float quaternion_angle(struct vec a, struct vec b);
float quaternion_length_squared(struct vec a);
float quaternion_length(struct vec a);
struct vec quaternion_normalize(struct vec a);
struct vec quaternion_power(struct vec a, float exponent);
struct vec quaternion_from_axis_angle(struct vec a, float angle);
struct vec quaternion_to_axis_angle(struct vec a);
struct vec quaternion_from_2_vectors(struct vec a, struct vec b);
struct vec quaternion_rotation_matrix(struct mat m);
struct vec quaternion_yaw_pitch_roll(float yaw, float pitch, float roll);
struct vec quaternion_linear_interpolation(struct vec a, struct vec b, float p);
struct vec quaternion_spherical_linear_interpolation(struct vec a, struct vec b, float p);

/* Matrix */
void pmatrix_zero(struct mat *result);
void pmatrix_identity(struct mat *result);
void pmatrix_transpose(struct mat *m, struct mat *result);
void pmatrix_inverse(struct mat *m, struct mat *result);
void pmatrix_ortho(float l, float r, float b, float t, float n, float f, struct mat *result);
void pmatrix_perspective(float fov_y, float aspect, float n, float f, struct mat *result);
void pmatrix_perspective_fov(float fov, float w, float h, float n, float f, struct mat *result);
void pmatrix_perspective_infinite(float fov_y, float aspect, float n, struct mat *result);
void pmatrix_rotation_x(float angle, struct mat *result);
void pmatrix_rotation_y(float angle, struct mat *result);
void pmatrix_rotation_z(float angle, struct mat *result);
void pmatrix_rotation_axis(struct vec *a, float angle, struct mat *result);
void pmatrix_rotation_quaternion(struct vec *q, struct mat *result);
void pmatrix_look_at_up(struct vec *pos, struct vec *target, struct vec *up_axis, struct mat *result);
void pmatrix_look_at(struct vec *pos, struct vec *target, struct mat *result);
void pmatrix_scale(struct vec *v, struct mat *result);
void pmatrix_get_scale(struct mat *m, struct vec *result);
void pmatrix_translation(struct vec *v, struct mat *result);
void pmatrix_get_translation(struct mat *m, struct vec *result);
void pmatrix_negative(struct mat *m, struct mat *result);
void pmatrix_multiply(struct mat *m, float s, struct mat *result);
void pmatrix_multiply_matrix(struct mat *a, struct mat *b, struct mat *result);
void pmatrix_linear_interpolation(struct mat *a, struct mat *b, float p, struct mat *result);
void pmatrix_multiply_f4(struct mat *m, float *result);
void pmatrix_to_array(struct mat *m, float *result);

struct mat matrix_zero(void);
struct mat matrix_identity(void);
struct mat matrix_transpose(struct mat m);
struct mat matrix_inverse(struct mat m);
struct mat matrix_ortho(float l, float r, float b, float t, float n, float f);
struct mat matrix_perspective(float fov_y, float aspect, float n, float f);
struct mat matrix_perspective_fov(float fov, float w, float h, float n, float f);
struct mat matrix_perspective_infinite(float fov_y, float aspect, float n);
struct mat matrix_rotation_x(float angle);
struct mat matrix_rotation_y(float angle);
struct mat matrix_rotation_z(float angle);
struct mat matrix_rotation_axis(struct vec a, float angle);
struct mat matrix_rotation_quaternion(struct vec q);
struct mat matrix_look_at_up(struct vec pos, struct vec target, struct vec up_axis);
struct mat matrix_look_at(struct vec pos, struct vec target);
struct mat matrix_scale(struct vec v);
struct vec matrix_get_scale(struct mat m);
struct mat matrix_translation(struct vec v);
struct vec matrix_get_translation(struct mat m);
struct mat matrix_negative(struct mat m);
struct mat matrix_multiply(struct mat m, float s);
struct mat matrix_multiply_matrix(struct mat a, struct mat b);
struct mat matrix_linear_interpolation(struct mat a, struct mat b, float p);
void matrix_multiply_f4(struct mat m, float *result);
void matrix_to_array(struct mat m, float *result);

/* Intersection */
bool pvector2_in_circle(struct vec *v, struct vec *circle_position, float radius);
bool pvector2_in_triangle(struct vec *v, struct vec *a, struct vec *b, struct vec *c);

bool vector2_in_circle(struct vec v, struct vec circle_position, float radius);
bool vector2_in_triangle(struct vec v, struct vec a, struct vec b, struct vec c);

/* Easing functions */
float quadratic_ease_in(float p);
float quadratic_ease_out(float p);
float quadratic_ease_in_out(float p);
float cubic_ease_in(float p);
float cubic_ease_out(float p);
float cubic_ease_in_out(float p);
float quartic_ease_in(float p);
float quartic_ease_out(float p);
float quartic_ease_in_out(float p);
float quintic_ease_in(float p);
float quintic_ease_out(float p);
float quintic_ease_in_out(float p);
float sine_ease_in(float p);
float sine_ease_out(float p);
float sine_ease_in_out(float p);
float circular_ease_in(float p);
float circular_ease_out(float p);
float circular_ease_in_out(float p);
float exponential_ease_in(float p);
float exponential_ease_out(float p);
float exponential_ease_in_out(float p);
float elastic_ease_in(float p);
float elastic_ease_out(float p);
float elastic_ease_in_out(float p);
float back_ease_in(float p);
float back_ease_out(float p);
float back_ease_in_out(float p);
float bounce_ease_in(float p);
float bounce_ease_out(float p);
float bounce_ease_in_out(float p);

#endif
