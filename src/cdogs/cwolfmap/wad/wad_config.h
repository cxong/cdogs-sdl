/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __WADCONFIG_H__
#define __WADCONFIG_H__

// Configuration for memory management in other systems.

#ifndef WAD_MALLOC
#define WAD_MALLOC(s)		malloc((s))
#endif

#ifndef WAD_FREE
#define WAD_FREE(s)			free((s))
#endif

#ifndef WAD_REALLOC
#define WAD_REALLOC(p,s)	realloc((p),(s))
#endif

#ifndef WAD_CALLOC
#define WAD_CALLOC(n,s)		calloc((n),(s))
#endif

#endif
