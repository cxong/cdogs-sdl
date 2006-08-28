/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-------------------------------------------------------------------------------

 utils.c - miscellaneous utilities
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <stdlib.h>

#include "utils.h"

int debug = 0;

void *	sys_mem_alloc(unsigned int size)
{
	void * new = calloc(1, size);
	
	if (new == NULL) {
		printf("### Memory allocation of %d bytes failed! ###\n", size);
		exit(1);
	}
	
	debug("%d bytes allocated, at 0x%p\n", size, new);
	
	return new;
}

void *	sys_mem_realloc(void *ptr, unsigned int size)
{
	void * new = realloc(ptr, size);
	
	if (new == NULL) {
		printf("### Memory reallocation failed! ###\n");
		return ptr;
	}
	
	debug("memory reallocated 0x%p -> 0x%p (now %d bytes)\n", ptr, new, size);
	return new;
}

void	sys_mem_free(void *ptr)
{
	if (!ptr) return;
	debug("freeing memory at: 0x%p\n", ptr);
	free(ptr);
}