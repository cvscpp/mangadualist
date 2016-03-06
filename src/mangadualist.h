/**
 * @file mangadualist.h
 * @brief global header file
 * @date 2015-06-28
 * @author Jean-Michel Martin de Santero
 * @author Bruno Ethvignot
 */
/*
 * copyright (c) 1998-2015 TLK Games all rights reserved
 * $Id: powermanga.h,v 1.109 2012/08/26 15:44:26 gurumeditation Exp $
 *
 * Powermanga is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Powermanga is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */
#ifndef __MANGADUALIST__
#define __MANGADUALIST__
#if !defined(PACKAGE_STRING)
#define PACKAGE_STRING "Mangadualist 0.0.1"
#endif
#define MANGADUALIST_VERSION PACKAGE_STRING " 2016-03-06 "

#if !defined(MANGADUALIST_SDL) && !defined(MANGADUALIST_X11)
#define MANGADUALIST_SDL
#endif

#if defined(MANGADUALIST_X11)
#undef MANGADUALIST_SDL
#else
#if !defined(MANGADUALIST_SDL)
#define MANGADUALIST_SDL
#endif
#endif

#include <assert.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <dirent.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <unistd.h>

#ifndef __cplusplus
#if defined(HAVE_STDBOOL_H)
    /* The C language implementation does correctly provide the standard header
     * file "stdbool.h".  */
#include <stdbool.h>
#else
  /* The C language implementation does not provide the standard header file
   * "stdbool.h" as required by ISO/IEC 9899:1999.  Try to compensate for this
   * braindamage below.  */
#if !defined(bool)
#define bool    int
#endif
#if !defined(true)
#define true    1
#endif
#if !defined(false)
#define false   0
#endif
#endif
#endif

#ifndef SCOREFILE
#define SCOREFILE "mangadualist-scores"
#endif
#ifndef PREFIX
#define PREFIX ""
#endif

#ifndef MANGADUALIST_SDL

/** Use X Window for display */
#include <X11/keysym.h>
#include <X11/keysymdef.h>
typedef unsigned char Uint8;
typedef signed char Sint8;
typedef signed short Sint16;
typedef unsigned short Uint16;
typedef signed int Sint32;
typedef unsigned int Uint32;

/** Else use SDL */
#else
#include <SDL2/SDL.h>

#endif
/** Devel flag */
  /* #define DEVELOPPEMENT */

#ifdef PNG_EXPORT_ENABLE
#include <png.h>
#endif

#ifdef USE_SDLMIXER
#if defined(MANGADUALIST_X11)
#include <SDL2/SDL.h>
#endif
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mixer.h>
#endif

/* #define SHAREWARE_VERSION */
/** Maximum number of levels in the game, range 0 to 41 */
#define MAX_NUM_OF_LEVELS 41
#define TWO_PI 6.28318530718f
#define PI 3.14159265359f
#define HALF_PI 1.57079632679f
#define PI_PLUS_HALF_PI 4.712388980385f
#define PI_BY_16 0.19634954085f
/** Value epsilon for the comparison of the floating with absolute error */
#define EPS 0.001
/** Index of progression in a curve  */
#define POS_CURVE 0
/** Maximum of number of images in the TLK logo animation */
#define TLKLOGO_MAXOF_IMAGES 64
typedef enum
{
  XCOORD,
  YCOORD
} COORD_ENUM;

#ifdef __cplusplus
extern "C"
{
#endif
  /* "inits_game.c" file */
  bool inits_game (void);
#ifdef PNG_EXPORT_ENABLE
  bool tlk_games_logo_extract ();
#endif
  void release_game (void);
#ifdef UNDER_DEVELOPMENT
  /* "special_keys.c" file */
  void special_keys (void);
#endif
  /* "mangadualist.c" file */
  bool update_frame ();
  bool toggle_pause ();
  /** If TRUE display "GAME OVER" */
  extern bool gameover_enable;
  extern Sint32 global_counter;
  extern Uint32 loops_counter;
  extern bool player_pause;
  extern bool is_pause_draw;
  extern Sint32 player_score;
  extern bool quit_game;
  /** Current level number from 0 to 41 */
  extern Sint32 num_level;
#ifdef __cplusplus
}
#endif

#endif
