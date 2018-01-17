# MATHC Reference

## Utils

```c
#define MPI
#define MPI_2
#define MPI_4
```

Constant values of Pi, Pi divided by 2, and Pi divided by 4.

```c
#define VEC2_SIZE 2
#define VEC3_SIZE 3
#define QUAT_SIZE 4
#define MAT3_SIZE 9
#define MAT4_SIZE 16
```

Syntactic-sugar macros used for variable declaration.

```c
#define MFLT_EPSILON
```

Smaller error margin for float-point comparison.

```c
bool nearly_equal(mfloat_t a, mfloat_t b, mfloat_t epsilon);
```

Used to compare two `mfloat_t` variables with an error margin `epsilon`. The standard header `<float.h>` comes with the macro `FLT_EPSILON` that can be used as the error margin. Greater values are also acceptable in most cases, such as `FLT_EPSILON * 10.0f` and `FLT_EPSILON * 100.0f`.

Returns `true` if the values are accepted as equal and `false` otherwise.

```c
mfloat_t to_radians(mfloat_t degrees);
```

Return the angle `degrees` in radians.

```c
mfloat_t to_degrees(mfloat_t radians);
```

Return the angle `radians` in degrees.

# LICENSE

The source code of this project is licensed under the terms of the ZLIB license:

Copyright (C) 2016 Felipe Ferreira da Silva

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
