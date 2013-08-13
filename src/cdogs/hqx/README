hqx Library README
==================

Introduction
------------
hqx is a fast, high-quality magnification filter designed for pixel art.

Install
-------
NOTE: DevIL library and development headers are required.

    ./configure
    make && make install

For more information refer to INSTALL.

Usage
-----
hqx -s scaleBy input output
Where scaleBy is either 2, 3 or 4

For example:
    hqx -s 4 test.png out.png

Example
-------
#include <stdint.h>
#include <hqx.h>

uint32_t *src; // Pointer to source bitmap in RGB format
size_t width, height; // Size of source bitmap

/*
 * Code to init src, width & height
 */

uint32_t *dest = (uint32_t *) malloc(width * 4 * height * 4 * sizeof(uint32_t));
hqxInit();
hq4x_32(src, dest, width, height);

Implementation
--------------
The first step is an analysis of the 3x3 area of the source pixel. At first, we
calculate the color difference between the central pixel and its 8 nearest
neighbors. Then that difference is compared to a predefined threshold, and these
pixels are sorted into two categories: "close" and "distant" colored. There are
8 neighbors, so we are getting 256 possible combinations.

For the next step, which is filtering, a lookup table with 256 entries is used,
one entry per each combination of close/distant colored neighbors. Each entry
describes how to mix the colors of the source pixels from 3x3 area to get
interpolated pixels of the filtered image.

The present implementation is using YUV color space to calculate color
differences, with more tolerance on Y (brightness) component, then on color
components U and V. That color space conversion is quite easy to implement if
the format of the source image is 16 bit per pixel, using a simple lookup table.
It is also possible to calculate the color differences and compare them to a
threshold very fast, using MMX instructions.

Creating a lookup table was the most difficult part - for each combination the
most probable vector representation of the area has to be determined, with the
idea of edges between the different colored areas of the image to be preserved,
with the edge direction to be as close to a correct one as possible. That vector
representation is then rasterised with higher (3x) resolution using
anti-aliasing, and the result is stored in the lookup table.

The filter was not designed for photographs, but for images with clear sharp
edges, like line graphics or cartoon sprites. It was also designed to be fast
enough to process 256x256 images in real-time.
