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
 * This file is the starting point of the program. It does some platform
 * specific initialization stuff and calls the common initialization code.
 *
 * =======================================================================
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef _TINSPIRE
#include <libndls.h>
#endif

#include "../../common/header/common.h"

FILE* logfp;

void registerHandler(void);

int
main(int argc, char *argv[])
{
	#ifdef _TINSPIRE
	enable_relative_paths(argv);
	#endif
	
	// register signal handler
	registerHandler();

	// Setup FPU if necessary
	Sys_SetupFPU();
	
#ifdef _TINSPIRE
	logfp = fopen("log.txt.tns", "w");
#else
	logfp = fopen("log.txt", "w");
#endif
	
	// Initialize the game.
	// Never returns.
	Qcommon_Init(argc, argv);
	
	if (logfp) fclose(logfp);

	return 0;
}

