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
 * This is the "heart" of the id Tech 2 refresh engine. This file
 * implements the main window in which Quake II is running. The window
 * itself is created by the SDL backend, but here the refresh module is
 * loaded, initialized and it's interaction with the operating system
 * implemented. This code is also the interconnect between the input
 * system (the mouse) and the keyboard system, both are here tied
 * together with the refresher. The direct interaction between the
 * refresher and those subsystems are the main cause for the very
 * acurate and precise input controls of the id Tech 2.
 *
 * This implementation works for Windows and unixoid systems, but
 * other platforms may need an own implementation!
 *
 * =======================================================================
 */

#include <assert.h>
#include <errno.h>

#include "../../client/header/client.h"
#include "../../client/header/keyboard.h"

#ifdef ZIP
// if we build with zip support, zlib is available and we can use that for better PNG compression
#include <zlib.h>

static unsigned char*
compress_for_stbiw(unsigned char *data, int data_len, int *out_len, int quality)
{
	uLongf bufSize = compressBound(data_len);
	unsigned char* buf = malloc(bufSize);
	if(buf == NULL)  return NULL;
	if(compress2(buf, &bufSize, data, data_len, quality) != Z_OK)
	{
		free(buf);
		return NULL;
	}
	*out_len = bufSize;

	return buf;
}
#define STBIW_ZLIB_COMPRESS compress_for_stbiw
#endif // ZIP

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "header/stb_image_write.h"

qboolean VID_LoadRefresh(void);

typedef struct vidmode_s
{
	const char *description;
	int width, height;
	int mode;
} vidmode_t;

/* This must be the same as in videomenu.c! */
vidmode_t vid_modes[] = {
#if defined(ARCADE_MINI)
	{"Mode  0:  480x272", 480, 272, 1},
#elif defined(PAPK3S)
	{"Mode  0:  800x480", 800, 480, 1},
#else
	{"Mode  0:  320x240", 320, 240, 0},
	{"Mode  1:  400x300", 400, 300, 1},
	{"Mode  2:  512x384", 512, 384, 2},
	{"Mode  3:  640x400", 640, 400, 3},
	{"Mode  4:  640x480", 640, 480, 4},
	{"Mode  5:  800x500", 800, 500, 5},
	{"Mode  6:  800x600", 800, 600, 6},
	{"Mode  7:  960x720", 960, 720, 7},
	{"Mode  8: 1024x480", 1024, 480, 8},
	{"Mode  9: 1024x640", 1024, 640, 9},
	{"Mode 10: 1024x768", 1024, 768, 10},
	{"Mode 11: 1152x768", 1152, 768, 11},
	{"Mode 12: 1152x864", 1152, 864, 12},
	{"Mode 13: 1280x800", 1280, 800, 13},
	{"Mode 14: 1280x720", 1280, 720, 14},
	{"Mode 15: 1280x960", 1280, 960, 15},
	{"Mode 16: 1280x1024", 1280, 1024, 16},
	{"Mode 17: 1366x768", 1366, 768, 17},
	{"Mode 18: 1440x900", 1440, 900, 18},
	{"Mode 19: 1600x1200", 1600, 1200, 19},
	{"Mode 20: 1680x1050", 1680, 1050, 20},
	{"Mode 21: 1920x1080", 1920, 1080, 21},
	{"Mode 22: 1920x1200", 1920, 1200, 22},
	{"Mode 23: 2048x1536", 2048, 1536, 23},
	{"Mode 24: 2560x1080", 2560, 1080, 24},
	{"Mode 25: 2560x1440", 2560, 1440, 25},
	{"Mode 26: 2560x1600", 2560, 1600, 26},
	{"Mode 27: 3440x1440", 3440, 1440, 27},
	{"Mode 28: 3840x1600", 3840, 1600, 28},
	{"Mode 29: 3840x2160", 3840, 2160, 29},
	{"Mode 30: 4096x2160", 4096, 2160, 30},
	{"Mode 31: 5120x2880", 5120, 2880, 31},
#endif
};

/* Console variables that we need to access from this module */
cvar_t *vid_gamma;
extern cvar_t *vid_fullscreen;
cvar_t *vid_renderer;

/* Global variables used internally by this module */
viddef_t viddef;                /* global video state; used by other modules */

#define VID_NUM_MODES (sizeof(vid_modes) / sizeof(vid_modes[0]))
#define MAXPRINTMSG 4096

/*
 * Console command to re-start the video mode and refresh. We do this
 * simply by setting the modified flag for the vid_fullscreen variable, which will
 * cause the entire video mode and refreshto be reset on the next frame.
 */
void
VID_Restart_f(void)
{
	vid_fullscreen->modified = true;
}

void
VID_ListModes_f(void)
{
	int i;
	Com_Printf("Supported video modes (r_mode):\n");
	for(i=0; i<VID_NUM_MODES; ++i)
	{
		Com_Printf("  %s\n", vid_modes[i].description);
	}
	Com_Printf("  Mode -1: r_customwidth x r_customheight\n");
}

qboolean
VID_GetModeInfo(int *width, int *height, int mode)
{
	if ((mode < 0) || (mode >= VID_NUM_MODES))
	{
		return false;
	}

	*width = vid_modes[mode].width;
	*height = vid_modes[mode].height;

	return true;
}

void
VID_NewWindow(int width, int height)
{
	viddef.width = width;
	viddef.height = height;
}

/*
 * This function gets called once just before drawing each frame, and
 * it's sole purpose in life is to check to see if any of the video mode
 * parameters have changed, and if they have to update the refresh
 * and/or video mode to match.
 */
void
VID_CheckChanges(void)
{
	if (vid_fullscreen->modified)
	{
		S_StopAllSounds();

		/* refresh has changed */
		cl.refresh_prepped = false;
		cl.cinematicpalette_active = false;
		cls.disable_screen = true;

		VID_LoadRefresh();

		cls.disable_screen = false;
	}
}

void
VID_Init(void)
{
	/* Create the video variables so we know how to start the graphics drivers */
	vid_fullscreen = Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = Cvar_Get("vid_gamma", "1.2", CVAR_ARCHIVE);
	vid_renderer = Cvar_Get("vid_renderer", "soft", CVAR_ARCHIVE);

	/* Add some console commands that we want to handle */
	Cmd_AddCommand("vid_restart", VID_Restart_f);
	Cmd_AddCommand("vid_listmodes", VID_ListModes_f);

	/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}

// called with image data of width*height pixel which comp bytes per pixel (must be 3 or 4 for RGB or RGBA)
// expects the pixels data to be row-wise, starting at top left
void VID_WriteScreenshot( int width, int height, int comp, const void* data )
{
}

// Structure containing functions exported from refresh DLL
refexport_t	re;
void *reflib_handle = NULL;		// Handle to refresh DLL
qboolean ref_active = false;    /* Is the refresher being used? */

void Key_MarkAllUp(void);

extern int GLimp_Init(void);
extern qboolean GLimp_InitGraphics(int fullscreen, int *pwidth, int *pheight);
extern void VID_ShutdownWindow(void);

extern refexport_t SDL_GetRefAPI(refimport_t imp);

qboolean
VID_LoadRefresh(void)
{
	refimport_t		ri;
	GetRefAPI_t		GetRefAPI;

	// If the refresher is already active
	// we'll shut it down
	VID_Shutdown();

	// Log it!
	Com_Printf("----- refresher initialization -----\n");
	Com_Printf("LoadLibrary(%s)\n", "SDL");
	
	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Com_VPrintf = Com_VPrintf;
	ri.Sys_Error = Com_Error;
	ri.FS_LoadFile = FS_LoadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_Gamedir = FS_Gamedir;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_SetValue = Cvar_SetValue;
	ri.Vid_GetModeInfo = VID_GetModeInfo;
	ri.Vid_MenuInit = VID_MenuInit;
	ri.Vid_NewWindow = VID_NewWindow;
	ri.Vid_WriteScreenshot = VID_WriteScreenshot;

	ri.Vid_ShutdownWindow = VID_ShutdownWindow;
	ri.GLimp_Init = GLimp_Init;
	ri.GLimp_InitGraphics = GLimp_InitGraphics;

	re = SDL_GetRefAPI( ri );

	// Declare the refresher as active
	ref_active = true;

	if (re.api_version != API_VERSION)
	{
		VID_Shutdown();
		Com_Error (ERR_FATAL, "%s has incompatible api_version %d", "SDL", re.api_version);
	}

	// Initiate the refresher
	if (!re.Init())
	{
		VID_Shutdown(); // Isn't that just too bad? :(
		Com_Printf("ERROR: Loading %s as rendering backend failed!\n", "SDL");
		Com_Printf("------------------------------------\n\n");
		return false; // TODO: try again with default renderer?
	}

	/* Ensure that all key states are cleared */
	Key_MarkAllUp();

	Com_Printf("Successfully loaded %s as rendering backend\n", "SDL");
	Com_Printf("------------------------------------\n\n");
	return true;
}

void
VID_Shutdown(void)
{
	if (ref_active)
	{
		/* Shut down the renderer */
		re.Shutdown();
		//Sys_FreeLibrary(reflib_handle);
		reflib_handle = NULL;
		memset(&re, 0, sizeof(re));
	}

	// Declare the refresher as inactive
	ref_active = false;
}

// ======== wrappers for functions from refresh lib ========

void
R_BeginRegistration(char *map)
{
	if(ref_active)
	{
		re.BeginRegistration(map);
	}
}

struct model_s*
R_RegisterModel(char *name)
{
	if(ref_active)
	{
		return re.RegisterModel(name);
	}
	return NULL;
}

struct image_s*
R_RegisterSkin(char *name)
{
	if(ref_active)
	{
		return re.RegisterSkin(name);
	}
	return NULL;
}

void
R_SetSky(char *name, float rotate, vec3_t axis)
{
	if(ref_active)
	{
		re.SetSky(name, rotate, axis);
	}
}

void
R_EndRegistration(void)
{
	if(ref_active)
	{
		re.EndRegistration();
	}
}

void
R_RenderFrame(refdef_t *fd)
{
	if(ref_active)
	{
		re.RenderFrame(fd);
	}
}

struct image_s*
Draw_FindPic(char *name)
{
	if(ref_active)
	{
		return re.DrawFindPic(name);
	}
	return NULL;
}


void
Draw_GetPicSize(int *w, int *h, char *name)
{
	if(ref_active)
	{
		re.DrawGetPicSize(w, h, name);
	}
}

void
Draw_StretchPic(int x, int y, int w, int h, char *name)
{
	if(ref_active)
	{
		re.DrawStretchPic(x, y, w, h, name);
	}
}

void
Draw_PicScaled(int x, int y, char *pic, float factor)
{
	if(ref_active)
	{
		re.DrawPicScaled(x, y, pic, factor);
	}
}

void
Draw_CharScaled(int x, int y, int num, float scale)
{
	if(ref_active)
	{
		re.DrawCharScaled(x, y, num, scale);
	}
}

void
Draw_TileClear(int x, int y, int w, int h, char *name)
{
	if(ref_active)
	{
		re.DrawTileClear(x, y, w, h, name);
	}
}

void
Draw_Fill(int x, int y, int w, int h, int c)
{
	if(ref_active)
	{
		re.DrawFill(x, y, w, h, c);
	}
}

void
Draw_FadeScreen(void)
{
	if(ref_active)
	{
		re.DrawFadeScreen();
	}
}

void
Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte *data)
{
	if(ref_active)
	{
		re.DrawStretchRaw(x, y, w, h, cols, rows, data);
	}
}

void
R_SetPalette(const unsigned char *palette)
{
	if(ref_active)
	{
		re.SetPalette(palette);
	}
}

void
R_BeginFrame(float camera_separation)
{
	if(ref_active)
	{
		re.BeginFrame(camera_separation);
	}
}

void
R_EndFrame(void)
{
	if(ref_active)
	{
		re.EndFrame();
	}
}

qboolean
R_IsVSyncActive(void)
{
	if(ref_active)
	{
		return re.IsVSyncActive();
	}
	return false;
}
