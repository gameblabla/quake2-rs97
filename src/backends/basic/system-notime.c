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
 * This file implements all system dependent generic functions.
 *
 * =======================================================================
 */

#include <dirent.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../../common/header/common.h"
#include "../../common/header/glob.h"
#include "../generic/header/input.h"

// Pointer to game library
static void *game_library;

// Evil hack to determine if stdin is available
qboolean stdin_active = true;

// Console logfile
extern FILE	*logfile;

/* ================================================================ */

void
Sys_Quit(void)
{
#ifndef DEDICATED_ONLY
	CL_Shutdown();
#endif

	if (logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}

	Qcommon_Shutdown();

	printf("------------------------------------\n");

	exit(0);
}

void
Sys_Init(void)
{
}

/* ================================================================ */

char *
Sys_ConsoleInput(void)
{
	return NULL;
}

void
Sys_ConsoleOutput(char *string)
{
	fputs(string, stdout);
}

/* ================================================================ */

long long
Sys_Microseconds(void)
{
	return 0;
}

int
Sys_Milliseconds(void)
{
	return 0;
}

void
Sys_Nanosleep(int nanosec)
{
}

/* ================================================================ */

/* The musthave and canhave arguments are unused in YQ2. We
   can't remove them since Sys_FindFirst() and Sys_FindNext()
   are defined in shared.h and may be used in custom game DLLs. */

static char findbase[MAX_OSPATH];
static char findpath[MAX_OSPATH];
static char findpattern[MAX_OSPATH];
static DIR *fdir;

char *
Sys_FindFirst(char *path, unsigned musthave, unsigned canhave)
{

	return NULL;
}

char *
Sys_FindNext(unsigned musthave, unsigned canhave)
{
	return NULL;
}

void
Sys_FindClose(void)
{
	fdir = NULL;
}

/* ================================================================ */

void
Sys_UnloadGame(void)
{
}


void
Sys_Mkdir(char *path)
{
	mkdir(path, 0755);
}

qboolean
Sys_IsDir(const char *path)
{

	return false;
}

qboolean
Sys_IsFile(const char *path)
{

	return false;
}

char *
Sys_GetHomeDir(void)
{
	return "./";
}

/* ================================================================ */

void
Sys_GetWorkDir(char *buffer, size_t len)
{
	buffer[0] = '\0';
}

qboolean
Sys_SetWorkDir(char *path)
{
	return false;
}
