# MATHC

[![Build Status](https://travis-ci.org/ferreiradaselva/mathc.svg?branch=master)](https://travis-ci.org/ferreiradaselva/mathc)

MATHC is a simple math library for 2D and 3D game programming. It contains implementations for:

- 2D vectors
- 3D vectors
- Quaternions
- Matrices
- Easing functions

It support C99 standard or later.

## Release

The CHANGELOG and RELEASE NOTES can be found in [CHANGELOG.md](CHANGELOG.md#020---2017-11-11).

## Reference

The reference is the file `REFERENCE.md`.

There are examples on my other repository using this math library:

[CGDFW examples](https://github.com/ferreiradaselva/cgdfw/tree/master/examples)

## Contributing

Check the file `CONTRIBUTING.md` for contribution rules and contributions of interest.

## Float

Every structure and function uses `float`, because it is the most used type on 2D and 3D programming with OpenGL.

**The type `float` loses precision with large numbers, why not use `double`?** Because every `double` value would be converted to `float` before sending to OpenGL, anyway. Which means your physics would run with high precision, but the rendering would still be affected by the `float` imprecision. Instead, *the good practice* to solve the problem with large numbers is to truncate the world position back to `[0.0f, 0.0f, 0.0f]` when the world distance to the center is too large. If the world is too big that even when truncating there are stil large numbers, the correct approach is to divide the world in chunks.

## Passing Arguments as Value or Pointer

For every function **that takes a structure**, there are two versions. One that you pass structures as value and other that you pass as pointer. The functions that pass the value by pointer have a prefix `p` before the type name (pvector2, pvector3, pquaternion and pmatrix) and the result is the `*result` argument or a returned `float`.

You can decide which one you want to use.

Examples:

```c
/* Pass by value and return a value */
struct mat projection = matrix_ortho(-100.0f, 100.0f, 100.0f, -100.0f);
struct mat view = matrix_look_at(to_vector3(0.0f, 0.0f, 1.0f),
	to_vector3(0.0f, 0.0f, 0.0f));
struct mat pv = matrix_multiply_matrix(projection, view);

/* Pass by pointer */
struct vec pos = {0};
struct vec target = {0};
struct mat projection = {0};
struct mat view = {0};
struct mat multiplied_matrix = {0};

to_pvector3(0.0f, 0.0f, 1.0f, &pos);
to_pvector3(0.0f, 0.0f, 0.0f, &target);
to_pvector3(0.0f, 1.0f, 0.0f, &up);
pmatrix_ortho(-100.0f, 100.0f, 100.0f, -100.0f, &projection);
pmatrix_look_at(&pos, &target, &view);
pmatrix_multiply_matrix(&projection, &view, &multiplied_matrix);
```

## Vectors

All vectors (2D, 3D and quaternions) use the same structure type `struct vec`. The `z` component is still useful for 2D vectors, as it is used by OpenGL for depth testing. This means the only extra component on 2D and 3D vectors is the `w` component, which is used by quaternions.

Examples:

```c
/* Rotate a 2D vector 90º */
struct vec direction = to_vector2(0.0f, -1.0f);
direction = vector2_rotate(90.0f * M_PIF / 180.0f);

/* Get the angle (radians) of a 2D vector */
float angle = vector2_angle(direction);

/* Create a 3D vector */
struct vec position = to_vector3(0.0f, 0.0f, 0.0f);

/* Create a quaternion */
struct vec quaternion = to_quaternion(0.0f, 0.0f, 0.0f, 1.0f);
/* Spherical interpolation between two quaternions */
struct vec interpolated = quaternion_spherical_linear_interpolation(a, b, 0.5f);
```

You don't need to create your OpenGL buffer (VBO) to take 4 elements. When using `glMapBufferRange()`/`glMapBuffer()` and `glUnmapBuffer()`, you can pass only the used elements to the VBO, that has element count of your choice.

## Matrices

All matrices are 4×4. There are functions for setting up projection matrices, view matrices and model matrices.

Usually, model matrices are used to modify vertices on client-side or GPU-side. If you want to modify on client-side, you can use the functions `matrix_multiply_f4()` or `pmatrix_multiply_f4()` to modify an array with 4 `float` elements. Example:

```c
float v[4] = {0.0f, 10.0f, 0.0f, 1.0f}; /* Compoments X, Y, Z and W */
struct mat rotation = matrix_rotation_z(to_radians(45.0f));
matrix_multiply_f4(rotation, v);
```

If you want to modify on GPU-side, you can use the functions `matrix_to_array()` or `pmatrix_to_array()` to push the matrix to an array with 16 `float` elements. Example:

```c
float v[MAT_SIZE]; /* MAT_SIZE is a macro in mathc.h with value 16 */
struct mat projection = matrix_ortho(-100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 1.0f);
struct mat view = matrix_look_at(to_vector3(0.0f, 0.0f, 1.0f),
	to_vector3(0.0f, 0.0f, 0.0f));
struct mat pv = matrix_multiply_matrix(projection, view);
matrix_to_array(pv, v);
```

## Easing Functions

The easing functions are an implementation of the functions presented in [easings.net](http://easings.net/). They are mainly useful for animations.

Easing functions take a value that range from `0.0f` to `1.0f` and usually will return a value inside that same range. However, in some of the easing functions, the returned value extrapolate that range.

## LICENSE

The source code of this project is licensed under the terms of the ZLIB license:

Copyright (C) 2016 Felipe Ferreira da Silva

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
