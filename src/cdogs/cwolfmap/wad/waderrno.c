/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdio.h>
#include "waderrno.h"
 
static char *waderr[WADERROR_COUNT] = {
	"No error.",
	"File error.",
	"Not a WAD file.",
	"WAD pointer invalid.",
	"Out of memory.",
	"Problem closing WAD.",
	"Cannot commit WAD entry list.",
	"Operation not supported in this implementation.",
	"Index out of range.",
};

char* strwaderror(int n)
{
	if (n < 0 || n >= WADERROR_COUNT)
		return NULL;
	else
		return waderr[n];
}
