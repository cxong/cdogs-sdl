/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __WADERRNO_H__
#define __WADERRNO_H__

// This is meant to be used like <errno.h>. WAD errors and such are raised here.
// All calls into wad_* functions manipulate this value.

// Add "extern int waderrno;" to use.

#define WADERROR_NO_ERROR				0
#define WADERROR_FILE_ERROR				1
#define WADERROR_FILE_NOT_A_WAD			2
#define WADERROR_WAD_INVALID			3
#define WADERROR_OUT_OF_MEMORY			4
#define WADERROR_CANNOT_CLOSE			5
#define WADERROR_CANNOT_COMMIT			6
#define WADERROR_NOT_SUPPORTED			7
#define WADERROR_INDEX_OUT_OF_RANGE		8
#define WADERROR_COUNT					9

/**
 * WAD error number.
 * This field must be extern'ed to read.
 */
int waderrno;

/**
 * Get the string representation of an error.
 */
char* strwaderror(int n);

#endif
