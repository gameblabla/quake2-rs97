/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * This file implements the low level part of the Hunk_* memory system
 *
 * =======================================================================
 */

#include "../../../common/header/common.h"

byte *membase;
int maxhunksize;
int curhunksize;

void *
Hunk_Begin(int maxsize)
{
	maxhunksize = maxsize + sizeof(int);
	curhunksize = 0;
	
	membase = (byte *)malloc(maxhunksize);
	
	if (membase == NULL)
	{
	}

	*((int *)membase) = curhunksize;

	return membase + sizeof(int);
}

void *
Hunk_Alloc(int size)
{
	byte *buf;

	/* round to cacheline */
	size = (size + 31) & ~31;

	if (curhunksize + size > maxhunksize)
	{
		Sys_Error("Hunk_Alloc overflow");
	}

	buf = membase + sizeof(int) + curhunksize;
	curhunksize += size;
	return buf;
}

int
Hunk_End(void)
{
	byte *orig_membase = membase;
	
	membase = (byte *)realloc(membase, (curhunksize + 0xf) & 0xfffffff0);
	
	if (membase != orig_membase)
		Sys_Error("uh-oh, not the same memory bases %08x %08x\n", membase, orig_membase);
	
	return curhunksize;
}

void
Hunk_Free(void *base)
{
	byte *m;

	if (base) {
		m = ((byte *)base) - sizeof(int);
		free(m);
	}
}

