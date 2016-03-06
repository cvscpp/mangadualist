/**
 * @file display_sdl.c
 * @brief handle displaying and updating the graphical components of the game
 * @created 2003-07-09
 * @date 2015-01-10 
 * @author Bruno Ethvignot
 */
/*
 * copyright (c) 1998-2015 TLK Games all rights reserved
 * $Id: display_sdl.c,v 1.44 2012/08/26 19:30:40 gurumeditation Exp $
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
#include "config.h"
#include "mangadualist.h"
#include "tools.h"
#include "assembler.h"
#include "images.h"
#include "config_file.h"
#include "display.h"
#include "electrical_shock.h"
#include "energy_gauge.h"
#include "menu_sections.h"
#include "movie.h"
#include "log_recorder.h"
#include "options_panel.h"
#include "gfx_wrapper.h"
#ifdef USE_SCALE2X
#include "scalebit.h"
#endif
#include "sprites_string.h"
#include "texts.h"

#ifdef MANGADUALIST_SDL

/* SDL surfaces */
#define MAX_OF_SURFACES 100
static SDL_Texture *public_texture = NULL;

static SDL_Window *sdlWindow = NULL;
static SDL_Renderer *sdlRenderer = NULL;

static SDL_Surface *public_surface = NULL;
/** 512x440: game's offscreen  */
static SDL_Surface *game_surface = NULL;
/** 64*184: right options panel */
static SDL_Surface *options_surface = NULL;
static SDL_Surface *score_surface = NULL;
/** 320x200: movie animation */
static SDL_Surface *movie_surface = NULL;
static Uint32 surfaces_counter = 0;
static SDL_Surface *surfaces_list[MAX_OF_SURFACES];

#ifdef USE_SDL_JOYSTICK
/** Number of available joysticks */
static Uint32 numof_joysticks = 0;
static SDL_Joystick **sdl_joysticks = NULL;
#endif

Sint32 vmode2 = 0;

static void display_movie (void);
static void display (void);

static SDL_Surface *create_surface (Uint32 width, Uint32 height);
static void get_rgb_mask (Uint32 * rmask, Uint32 * gmask, Uint32 * bmask);
static void free_surface (SDL_Surface * surface);
static void free_surfaces (void);
#ifdef USE_SDL_JOYSTICK
void display_close_joysticks (void);
#endif
void key_status (const Uint8 * k);
/** Color table in 8-bit depth */
SDL_Color *sdl_color_palette = NULL;
static const char window_tile[] = MANGADUALIST_VERSION " powered by TLK Powermanga (SDL)\0";
/** If TRUE reverses the horizontal and vertical controls */
static bool is_reverse_ctrl = FALSE;
static bool pause_will_disable = FALSE;

/**
 * Initialize SDL display
 * @return TRUE if it completed successfully or FALSE otherwise
 */
bool
display_init (void)
{
  Uint32 i;
  Uint32 sdl_flag;

  for (i = 0; i < MAX_OF_SURFACES; i++)
    {
      surfaces_list[i] = (SDL_Surface *) NULL;
    }

   window_width = 320;
   window_height = 200;

  /* initialize SDL screen */
  sdl_flag = SDL_INIT_VIDEO;
#ifdef USE_SDL_JOYSTICK
  sdl_flag |= SDL_INIT_JOYSTICK;
#endif
  if (SDL_Init (sdl_flag) != 0)
    {
      LOG_ERR ("SDL_Init() failed: %s", SDL_GetError ());
      return FALSE;
    }
#ifdef USE_SDL_JOYSTICK
  if (!display_open_joysticks ())
    {
      return FALSE;
    }
#endif

	SDL_CreateWindowAndRenderer(640, 400, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL, &sdlWindow, &sdlRenderer);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(sdlRenderer, 320, 200);

	if (!sdlWindow) {
		LOG_ERR ("sdlWindow is NULL");
		return FALSE;
	}
	
	if (!sdlRenderer){
		LOG_ERR ("sdlRenderer is NULL");
		return FALSE;
	}

	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderPresent(sdlRenderer);

	
	SDL_RendererInfo rInfo;
	
	if (SDL_GetRendererInfo(sdlRenderer, &rInfo) != 0){
		LOG_ERR ("SDL failed: %s", SDL_GetError ());
		return FALSE;
	}
	
	bits_per_pixel = SDL_BITSPERPIXEL(rInfo.texture_formats[0]);
	bytes_per_pixel = SDL_BYTESPERPIXEL(rInfo.texture_formats[0]);
	
//       if (bits_per_pixel == 16)
//         {
//           bits_per_pixel =
//             rInfo. vi->vfmt->Rshift + vi->vfmt->Gshift + vi->vfmt->Bshift;
//         }
      if (bits_per_pixel < 8)
        {

          LOG_ERR ("Mangadualist need 8 bits per pixels minimum"
                   " (256 colors)");
          return FALSE;
        }	
	
	LOG_INF ("depth of screen: %i; bytes per pixel: %i", bits_per_pixel, bytes_per_pixel);
  
  
  public_surface = SDL_CreateRGBSurface(0,320,200,bits_per_pixel,0,0,0,0);
  if (public_surface == NULL)
    {
      LOG_ERR ("SDL_CreateRGBSurface() return %s", SDL_GetError ());
      return FALSE;
    }
  LOG_INF ("SDL_CreateRGBSurface() successful window_width: %i;"
           " window_height: %i; bits_per_pixel: %i",
           320, 200, bits_per_pixel);

  public_texture = SDL_CreateTextureFromSurface(sdlRenderer, public_surface);
  if (public_texture == NULL)
    {
      LOG_ERR ("SDL_CreateTextureFromSurface() return %s", SDL_GetError ());
      return FALSE;
    }
  
  LOG_INF ("video has been successfully initialized");
  return TRUE;
}


/**
 * Destroy off screen surface for start and end movies
 */
void
destroy_movie_offscreen (void)
{
  if (movie_offscreen != NULL)
    {
      free_surface (movie_surface);
      movie_offscreen = NULL;
    }
  movie_surface = NULL;
}

/**
 * Create off screen surface for the start and end movies
 * @return TRUE if it completed successfully or FALSE otherwise
 */
bool
create_movie_offscreen (void)
{
  movie_surface = create_surface (display_width, display_height);
  if (movie_surface == NULL)
    {
      return FALSE;
    }
  movie_offscreen = (char *) movie_surface->pixels;
  return TRUE;
}

/**
 * Create 3 or 4 off screens surfaces for the game
 * @return TRUE if it completed successfully or FALSE otherwise
 */
bool
create_offscreens (void)
{
  /* create surface "game_offscreen" 512*440 */
  game_surface = create_surface (offscreen_width, offscreen_height);
  if (game_surface == NULL)
    {
      return FALSE;
    }
  game_offscreen = (char *) game_surface->pixels;
  offscreen_pitch = offscreen_width * bytes_per_pixel;

  options_surface = create_surface (OPTIONS_WIDTH, OPTIONS_HEIGHT);
  if (options_surface == NULL)
    {
      return FALSE;
    }
  options_offscreen = (char *) options_surface->pixels;
  score_surface =
    create_surface (score_offscreen_width, score_offscreen_height);
  if (score_surface == NULL)
    {
      return FALSE;
    }
  scores_offscreen = (char *) score_surface->pixels;
  score_offscreen_pitch = score_offscreen_width * bytes_per_pixel;
  return TRUE;
}

/**
 * Recopy 8-bit palette or create 16-bit or 24-bit palette
 * @return TRUE if it completed successfully or FALSE otherwise
 */
bool
create_palettes (void)
{
  Uint32 i;
  unsigned char *dest;
  unsigned char *src;

  /* 8-bit displays support 256 colors */
  if (bytes_per_pixel == 1)
    {
      if (sdl_color_palette == NULL)
        {
          sdl_color_palette =
            (SDL_Color *) memory_allocation (sizeof (SDL_Color) * 256);
          if (sdl_color_palette == NULL)
            {
              LOG_ERR ("'sdl_color_palette' out of memory");
              return FALSE;
            }
        }
      src = (unsigned char *) palette_24;
      for (i = 0; i < 256; i++)
        {
          sdl_color_palette[i].r = src[0];
          sdl_color_palette[i].g = src[1];
          sdl_color_palette[i].b = src[2];
          src += 3;
        }
	SDL_Palette *palette = (SDL_Palette *)malloc(sizeof(sdl_color_palette)*256);
	SDL_SetPaletteColors(palette, sdl_color_palette, 0, 256);
	SDL_SetSurfacePalette(public_surface, palette);	  
 
    }
  else
    /* 16-bit depth with 65336 colors */
    {
      if (bytes_per_pixel == 2)
        {
          if (pal16 == NULL)
            {
              pal16 = (unsigned short *) memory_allocation (256 * 2);
              if (pal16 == NULL)
                {
                  LOG_ERR ("'pal16' out of memory");
                  return FALSE;
                }
            }
          if (bits_per_pixel == 15)
            {
              convert_palette_24_to_15 (palette_24, pal16);
            }
          else
            {
              convert_palette_24_to_16 (palette_24, pal16);
            }
        }
      else
        /* 24-bit or 32-bit depth */
        {
          if (bytes_per_pixel > 2)
            {
              if (pal32 == NULL)
                {
                  pal32 = (Uint32 *) memory_allocation (256 * 4);
                  if (pal32 == NULL)
                    {
                      LOG_ERR ("'pal32' out of memory");
                      return FALSE;
                    }
                }
              dest = (unsigned char *) pal32;
              src = palette_24;
              for (i = 0; i < 256; i++)
                { 
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                  dest[3] = src[2];
                  dest[2] = src[1];
                  dest[1] = src[0];
                  dest[0] = 0;
#else
                  dest[2] = src[2];
                  dest[1] = src[1];
                  dest[0] = src[0];
                  dest[3] = 0;
#endif
                  dest += 4;
                  src += 3;
                }
            }
        }
    }
  return TRUE;
}

/**
 * Switch to pause when the application loses focus or disables the
 * pause if the application gains focus.
 * @param gain Whether given states were gained or lost (1/0)
 */
void
display_toggle_pause (Uint8 gain)
{
  if (gain == 0)
    {
      if (!player_pause)
        {
          if (toggle_pause ())
            {
              pause_will_disable = TRUE;
            }
        }
    }
  else
    {
      if (player_pause && pause_will_disable)
        {
          toggle_pause ();
        }
      pause_will_disable = FALSE;
    }
}

/**
 * Handle input events
 */
void
display_handle_events (void)
{
  const Uint8 *keys;
  SDL_Event event;
  SDL_KeyboardEvent *ke;
  while (SDL_PollEvent (&event))
    {
      switch (event.type)
        {
        case SDL_KEYDOWN:
          {
            ke = (SDL_KeyboardEvent *) & event;
            LOG_INF ("SDL_KEYDOWN: "
               "%i %i %i %i", ke->type, ke->keysym.sym,
               ke->keysym, ke->state);
             keys = SDL_GetKeyboardState (NULL);
            key_status (keys);
            sprites_string_key_down (ke->keysym.sym, ke->keysym.sym);
            /* save key code pressed */
            key_code_down = ke->keysym.sym;
          }
          break;

        case SDL_KEYUP:
          {
            ke = (SDL_KeyboardEvent *) & event;
            LOG_INF ("SDL_KEYUP: "
               "%i %i %i %i\n", ke->type, ke->keysym.sym,
               ke->keysym, ke->state);
            keys = SDL_GetKeyboardState (NULL);
            sprites_string_key_up (ke->keysym.sym, ke->keysym.sym);
            if (key_code_down == (Uint32) ke->keysym.sym)
              {
                /* clear key code */
                key_code_down = 0;
              }
			key_status (keys);
          }
          break;
        case SDL_JOYHATMOTION:
          if (event.jhat.value == SDL_HAT_RIGHTUP)
            {
              joy_top = 1;
              joy_right = 1;
              joy_down = 0;
              joy_left = 0;
            }
          else if (event.jhat.value == SDL_HAT_RIGHTDOWN)
            {
              joy_top = 0;
              joy_right = 1;
              joy_down = 1;
              joy_left = 0;
            }
          else if (event.jhat.value == SDL_HAT_LEFTDOWN)
            {
              joy_top = 0;
              joy_right = 0;
              joy_down = 1;
              joy_left = 1;
            }
          else if (event.jhat.value == SDL_HAT_LEFTUP)
            {
              joy_top = 1;
              joy_right = 0;
              joy_down = 0;
              joy_left = 1;
            }
          else if (event.jhat.value == SDL_HAT_UP)
            {
              joy_top = 1;
              joy_right = 0;
              joy_down = 0;
              joy_left = 0;
            }
          else if (event.jhat.value == SDL_HAT_RIGHT)
            {
              joy_top = 0;
              joy_right = 1;
              joy_down = 0;
              joy_left = 0;
            }
          else if (event.jhat.value == SDL_HAT_DOWN)
            {
              joy_top = 0;
              joy_right = 0;
              joy_down = 1;
              joy_left = 0;
            }
          else if (event.jhat.value == SDL_HAT_LEFT)
            {
              joy_top = 0;
              joy_right = 0;
              joy_down = 0;
              joy_left = 1;
            }
          else if (event.jhat.value == SDL_HAT_CENTERED)
            {
              joy_top = 0;
              joy_right = 0;
              joy_down = 0;
              joy_left = 0;
            }
          break;
        case SDL_JOYAXISMOTION:
          {
            Sint32 deadzone = 4096;
            /* x axis */
            if (event.jaxis.axis == power_conf->joy_x_axis)
              {
                if (event.jaxis.value < -deadzone)
                  {
                    joy_left = TRUE;
                    sprites_string_set_joy (IJOY_LEFT);
                    joy_right = FALSE;
                  }
                else if (event.jaxis.value > deadzone)
                  {
                    joy_left = FALSE;
                    joy_right = TRUE;
                    sprites_string_set_joy (IJOY_RIGHT);
                  }
                else
                  {
                    joy_left = FALSE;
                    joy_right = FALSE;
                    sprites_string_clr_joy (IJOY_RIGHT);
                    sprites_string_clr_joy (IJOY_LEFT);
                  }
              }
            /* y axis */
            else if (event.jaxis.axis == power_conf->joy_y_axis)
              {
                if (event.jaxis.value < -deadzone)
                  {
                    joy_down = FALSE;
                    joy_top = TRUE;
                    sprites_string_set_joy (IJOY_TOP);
                  }
                else if (event.jaxis.value > deadzone)
                  {
                    joy_down = TRUE;
                    joy_top = FALSE;
                    sprites_string_set_joy (IJOY_DOWN);
                  }
                else
                  {
                    joy_down = FALSE;
                    joy_top = FALSE;
                    sprites_string_clr_joy (IJOY_TOP);
                    sprites_string_clr_joy (IJOY_DOWN);
                  }
              }
          }
          break;
        case SDL_JOYBUTTONDOWN:
          if (event.jbutton.button == power_conf->joy_start)
            {
              start_button_down = TRUE;
            }
          else if (event.jbutton.button == power_conf->joy_fire)
            {
              fire_button_down = TRUE;
              sprites_string_set_joy (IJOY_FIRE);
            }
          else if (event.jbutton.button == power_conf->joy_option)
            {
              option_button_down = TRUE;
              sprites_string_set_joy (IJOY_OPT);
            }
          break;
        case SDL_JOYBUTTONUP:
          if (event.jbutton.button == power_conf->joy_start)
            {
              start_button_down = FALSE;
            }
          else if (event.jbutton.button == power_conf->joy_fire)
            {
              fire_button_down = FALSE;
              sprites_string_clr_joy (IJOY_FIRE);
            }
          else if (event.jbutton.button == power_conf->joy_option)
            {
              option_button_down = FALSE;
              sprites_string_clr_joy (IJOY_OPT);
            }
          break;
        case SDL_QUIT:
          quit_game = TRUE;

          break;
        case SDL_MOUSEBUTTONDOWN:
          switch (event.button.button)
            {
            case SDL_BUTTON_LEFT:
              mouse_b = 1;
              mouse_x = event.button.x / screen_pixel_size;
              mouse_y = event.button.y / screen_pixel_size;
              LOG_INF ("mouse_x = %i mouse_y = %i", mouse_x, mouse_y);
              break;
            }
          break;
        case SDL_MOUSEBUTTONUP:
          switch (event.button.button)
            {
            case SDL_BUTTON_LEFT:
              mouse_b = 0;
              break;
            }

          /* application loses/gains visibility */
        case SDL_WINDOWEVENT:
          //LOG_INF ("  > SDL_ACTIVEEVENT: %i <", event.active.state);
          if (event.window.event & SDL_WINDOWEVENT_ENTER)
            {
              LOG_DBG ("[SDL_APPMOUSEFOCUS] The app has mouse coverage; "
                       " SDL_WINDOWEVENT_FOCUS_GAINED: %i", SDL_WINDOWEVENT_FOCUS_GAINED);
            }
          if (event.window.event & SDL_WINDOWEVENT_ENTER)
            {
              LOG_DBG ("[SDL_APPINPUTFOCUS] The app has input focus; "
                       "SDL_WINDOWEVENT_FOCUS_GAINED: %i", SDL_WINDOWEVENT_FOCUS_GAINED);
              display_toggle_pause (SDL_WINDOWEVENT_FOCUS_GAINED);
            }
          if (event.window.event & SDL_WINDOWEVENT_EXPOSED)
            {
              is_iconified = event.window.event ? TRUE : FALSE;
              LOG_DBG ("[SDL_WINDOWEVENT_SHOWN]The application is active; "
                       "SDL_WINDOWEVENT_FOCUS_GAINED: %i, is_iconified: %i",
                       SDL_WINDOWEVENT_FOCUS_GAINED, is_iconified);
              display_toggle_pause (SDL_WINDOWEVENT_FOCUS_GAINED);
            }
          break;

          /* screen needs to be redrawn */
        case SDL_WINDOWEVENT_EXPOSED:
          update_all = TRUE;
          if (SDL_FillRect (public_surface, NULL, real_black_color) < 0)
            {
              LOG_ERR ("SDL_FillRect(public_surface) return %s",
                       SDL_GetError ());
            }
          break;

          /* mouse moved */
        case SDL_MOUSEMOTION:
          break;

        default:
          LOG_INF ("not supported event type: %i", event.type);
          break;
        }
    }
}

/**
 * Copy SDL flags buttons
 * @param k Pointer to an array of snapshot of the current keyboard state
 */
void
key_status (const Uint8 * k)
{
  keys_down[K_ESCAPE] = k[SDL_SCANCODE_ESCAPE];
  keys_down[K_CTRL] = k[SDL_SCANCODE_LCTRL];
  keys_down[K_CTRL] |= k[SDL_SCANCODE_RCTRL];
  keys_down[K_RETURN] = k[SDL_SCANCODE_RETURN];
  keys_down[K_PAUSE] = k[SDL_SCANCODE_PAUSE];
  keys_down[K_SHIFT] = k[SDL_SCANCODE_LSHIFT];
  keys_down[K_SHIFT] |= k[SDL_SCANCODE_RSHIFT];
  keys_down[K_1] = k[SDL_SCANCODE_1] | k[SDL_SCANCODE_KP_1];
  keys_down[K_2] = k[SDL_SCANCODE_2] | k[SDL_SCANCODE_KP_2];
  keys_down[K_3] = k[SDL_SCANCODE_3] | k[SDL_SCANCODE_KP_3];
  keys_down[K_4] = k[SDL_SCANCODE_4] | k[SDL_SCANCODE_KP_4];
  keys_down[K_5] = k[SDL_SCANCODE_5] | k[SDL_SCANCODE_KP_5];
  keys_down[K_6] = k[SDL_SCANCODE_6] | k[SDL_SCANCODE_KP_6];
  keys_down[K_7] = k[SDL_SCANCODE_7] | k[SDL_SCANCODE_KP_7];
  keys_down[K_8] = k[SDL_SCANCODE_8] | k[SDL_SCANCODE_KP_8];
  keys_down[K_9] = k[SDL_SCANCODE_9] | k[SDL_SCANCODE_KP_9];
  keys_down[K_0] = k[SDL_SCANCODE_0] | k[SDL_SCANCODE_KP_0];
  /* about */  
  keys_down[K_F1] = k[SDL_SCANCODE_F1];
  keys_down[K_F2] = k[SDL_SCANCODE_F2];
  keys_down[K_F3] = k[SDL_SCANCODE_F3];
  /* switch between full screen and windowed mode */  
  keys_down[K_F4] = k[SDL_SCANCODE_F4];
  /* enable/disable the music */  
  keys_down[K_F5] = k[SDL_SCANCODE_F5];
  keys_down[K_F6] = k[SDL_SCANCODE_F6];
  keys_down[K_F7] = k[SDL_SCANCODE_F7];
  /* force "Game Over" */  
  keys_down[K_F8] = k[SDL_SCANCODE_F8];
  keys_down[K_F9] = k[SDL_SCANCODE_F9];
  keys_down[K_F10] = k[SDL_SCANCODE_F10];
  keys_down[K_F11] = k[SDL_SCANCODE_F11];
  /* enable/disable pause */  
  keys_down[K_F12] = k[SDL_SCANCODE_F12];
  keys_down[K_INSERT] = k[SDL_SCANCODE_INSERT];
  keys_down[K_SPACE] = k[SDL_SCANCODE_SPACE];
  if (is_reverse_ctrl)
    {
      keys_down[K_LEFT] = k[SDL_SCANCODE_DOWN];
      keys_down[K_RIGHT] = k[SDL_SCANCODE_UP];
      keys_down[K_UP] = k[SDL_SCANCODE_LEFT];
      keys_down[K_DOWN] = k[SDL_SCANCODE_RIGHT];
    }
  else
    {
      keys_down[K_LEFT] = k[SDL_SCANCODE_LEFT];
      keys_down[K_RIGHT] = k[SDL_SCANCODE_RIGHT];
      keys_down[K_UP] = k[SDL_SCANCODE_UP];
      keys_down[K_DOWN] = k[SDL_SCANCODE_DOWN];
    }
  keys_down[K_A] = k[SDL_SCANCODE_A];
  keys_down[K_F] = k[SDL_SCANCODE_F];
  keys_down[K_V] = k[SDL_SCANCODE_V];
  keys_down[K_B] = k[SDL_SCANCODE_B];
  keys_down[K_P] = k[SDL_SCANCODE_P];
  keys_down[K_Q] = k[SDL_SCANCODE_G];
  keys_down[K_S] = k[SDL_SCANCODE_S];
  /* right */
  keys_down[K_RIGHT] |= k[SDL_SCANCODE_KP_6];
  /* left */
  keys_down[K_LEFT] |= k[SDL_SCANCODE_KP_4];
  /* up */
  keys_down[K_UP] |= k[SDL_SCANCODE_KP_8];
  /* down */
  keys_down[K_DOWN] |= k[SDL_SCANCODE_KP_5];
  /* power-up (aka Ctrl key) */
  keys_down[K_CTRL] |= k[SDL_SCANCODE_KP_2];
  /* fire (aka space bar) */
  keys_down[K_SPACE] |= k[SDL_SCANCODE_KP_0];
  /* fire (aka space ENTER) */
  if (k[SDL_SCANCODE_RETURN] && !is_playername_input () &&
      menu_section != SECTION_ORDER)
    {
      keys_down[K_SPACE] |= k[SDL_SCANCODE_RETURN];
    }
  keys_down[K_C] = k[SDL_SCANCODE_C];
  keys_down[K_G] = k[SDL_SCANCODE_G];
  keys_down[K_E] = k[SDL_SCANCODE_E];
  /* Volume control */
  keys_down[K_PAGEUP] = k[SDL_SCANCODE_PAGEUP];
  keys_down[K_PAGEDOWN] = k[SDL_SCANCODE_PAGEDOWN];
}

/**
 * Display on the screen
 */
void
display_update_window (void)
{
  /* movie is playing? */
  if (movie_surface != NULL)
    {
      update_all = TRUE;
      display_movie ();
    }
  else
    {
    display ();
    }
}

/**
 * Return a pointer to a rectangular area
 * @param x X-coordintate of the upper-left corner of the rectangle
 * @param y Y-coordintate of the upper-left corner of the rectangle
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @return Pointer to SDL_Rect structure
 */
static void
get_rect (SDL_Rect * rect, Sint16 x, Sint16 y, Sint16 w, Sint16 h)
{
  rect->x = x;
  rect->y = y;
  rect->w = w;
  rect->h = h;
}

/** 
 * Display start movie and end movie
 */
static void
display_movie (void)
{
  SDL_Rect rsour;
  Sint32 i;

  switch (bytes_per_pixel)
    {
    case 1:
      for (i = 0; i < (display_width * display_height); i++)
        {
          movie_offscreen[i] = movie_buffer[i];
        }
      break;
    case 2:
      conv8_16 ((char *) movie_buffer, movie_offscreen, pal16,
                display_width * display_height);
      break;
    case 3:
      conv8_24 ((char *) movie_buffer, movie_offscreen, pal32PlayAnim,
                display_width * display_height);
      break;
    case 4:
      conv8_32 ((char *) movie_buffer, movie_offscreen, pal32PlayAnim,
                display_width * display_height);
      break;
    }

    get_rect (&rsour, 0, 0, (Sint16) display_width, (Sint16) display_height);
    if (SDL_BlitSurface (movie_surface, &rsour, public_surface, &rsour) < 0) {
		LOG_ERR ("SDL_BlitSurface() return %s, display [%d,%d]", SDL_GetError (), (Sint16)display_width, (Sint16)display_height);
    }

    SDL_UpdateTexture(public_texture, NULL, public_surface->pixels, public_surface->pitch);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, public_texture, NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
}

/**
 * Display window in 320*200, orignal size of the game
 */
static void
display (void)
{
  SDL_Rect rdest;
  SDL_Rect rsour;
  Sint32 optx, opty;
  rsour.x = (Sint16) offscreen_clipsize;
  rsour.y = (Sint16) offscreen_clipsize;
  rsour.w = (Uint16) offscreen_width_visible;
  rsour.h = (Uint16) offscreen_height_visible;
  get_rect (&rdest, 0, 16, (Sint16) display_width, (Sint16) display_height);
  if (SDL_BlitSurface (game_surface, &rsour, public_surface, &rdest) < 0)
    {
      LOG_ERR ("SDL_BlitSurface(game_surface) return %s", SDL_GetError ());
    }
  if (update_all)
    {
      rsour.x = 0;
      rsour.y = 0;
      rsour.w = OPTIONS_WIDTH;
      rsour.h = OPTIONS_HEIGHT;
      get_rect (&rdest, (Sint16) offscreen_width_visible, 16, OPTIONS_WIDTH,
                OPTIONS_HEIGHT);
      if (SDL_BlitSurface (options_surface, &rsour, public_surface, &rdest))
        {
          LOG_ERR ("SDL_BlitSurface(options_surface) return %s",
                   SDL_GetError ());
        }

      /* display score panel */
      rsour.x = 0;
      rsour.y = 0;
      rsour.w = (Uint16) score_offscreen_width;
      rsour.h = SCORES_HEIGHT;
      get_rect (&rdest, 0, 0, (Sint16) display_width, SCORES_HEIGHT);
      if (SDL_BlitSurface (score_surface, &rsour, public_surface, &rdest) < 0)
        {
          LOG_ERR ("SDL_BlitSurface(score_surface) return %s",
                   SDL_GetError ());
        }
      opt_refresh_index = -1;
      update_all = FALSE;
    }
  else
    {
      /* display options from option panel */
      while (opt_refresh_index >= 0)
        {
          optx = options_refresh[opt_refresh_index].coord_x;
          opty = options_refresh[opt_refresh_index--].coord_y;
          rsour.x = (Sint16) optx;
          rsour.y = (Sint16) opty;
          rsour.w = 28;
          rsour.h = 28;
          get_rect (&rdest, (Sint16) (offscreen_width_visible + optx),
                    (Sint16) (16 + opty), 28, 28);
          if (SDL_BlitSurface
              (options_surface, &rsour, public_surface, &rdest) < 0)
            {
              LOG_ERR ("SDL_BlitSurface(options_surface) return %s",
                       SDL_GetError ());
            }
        }
      if (score_x2_refresh || !update_all)
        {
          rsour.x = 41;
          rsour.y = 171;
          rsour.w = 14;
          rsour.h = 8;
          get_rect (&rdest, 297, 187, 14, 8);
          if (SDL_BlitSurface
              (options_surface, &rsour, public_surface, &rdest) < 0)
            {
              LOG_ERR ("SDL_BlitSurface(options_surface) return %s",
                       SDL_GetError ());
            }
          score_x2_refresh = FALSE;
        }
      if (score_x4_refresh || !update_all)
        {
          rsour.x = 41;
          rsour.y = 5;
          rsour.w = 14;
          rsour.h = 8;
          get_rect (&rdest, 297, 21, 14, 8);
          if (SDL_BlitSurface
              (options_surface, &rsour, public_surface, &rdest) < 0)
            {
              LOG_ERR ("SDL_BlitSurface(options_surface) return %s",
                       SDL_GetError ());
            }
          score_x4_refresh = FALSE;
        }

      /* display player's energy */
      if (energy_gauge_spaceship_is_update)
        {
          rsour.x = 210;
          rsour.y = 3;
          rsour.w = 100;
          rsour.h = 9;
          get_rect (&rdest, 210, 3, 100, 9);
          optx =
            SDL_BlitSurface (score_surface, &rsour, public_surface, &rdest);
          if (SDL_BlitSurface (score_surface, &rsour, public_surface, &rdest)
              < 0)
            {
              LOG_ERR ("SDL_BlitSurface(score_surface) return %s",
                       SDL_GetError ());
            }
          energy_gauge_spaceship_is_update = FALSE;
        }

      /* display big-boss's energy */
      if (energy_gauge_guard_is_update)
        {
          rsour.x = 10;
          rsour.y = 3;
          rsour.w = 45;
          rsour.h = 9;
          get_rect (&rdest, 10, 3, 45, 9);
          if (SDL_BlitSurface (score_surface, &rsour, public_surface, &rdest)
              < 0)
            {
              LOG_ERR ("SDL_BlitSurface(score_surface) return %s",
                       SDL_GetError ());
            }
          energy_gauge_guard_is_update = FALSE;
        }

      /* display score number */
      if (is_player_score_displayed)
        {
          rsour.x = 68;
          rsour.y = 0;
          rsour.w = 128;
          rsour.h = 16;
          get_rect (&rdest, 68, 0, 128, 16);
          if (SDL_BlitSurface (score_surface, &rsour, public_surface, &rdest)
              < 0)
            {
              LOG_ERR ("SDL_BlitSurface(score_surface) return %s",
                       SDL_GetError ());
            }
          is_player_score_displayed = FALSE;
        }
    }
    SDL_UpdateTexture(public_texture, NULL, public_surface->pixels, public_surface->pitch);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, public_texture, NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
  
  /* SDL_UpdateRect (public_surface, 0, 16, 256, 184); */

}

#ifdef USE_SDL_JOYSTICK
/**
 * Opens all joysticks available
 */
bool
display_open_joysticks (void)
{
  Uint32 i;
  display_close_joysticks ();
  numof_joysticks = SDL_NumJoysticks ();
  LOG_INF ("number of joysticks available: %i", numof_joysticks);
  if (numof_joysticks < 1)
    {
      return TRUE;
    }
  sdl_joysticks =
    (SDL_Joystick **) memory_allocation (sizeof (SDL_Joystick *) *
                                         numof_joysticks);
  if (sdl_joysticks == NULL)
    {
      LOG_ERR ("'sdl_joysticks' out of memory");
      return FALSE;
    }
  for (i = 0; i < numof_joysticks; i++)
    {
      sdl_joysticks[i] = SDL_JoystickOpen (i);
      if (sdl_joysticks[i] == NULL)
        {
          LOG_ERR ("couldn't open joystick 0: %s", SDL_GetError ());
        }
      else
        {
          LOG_DBG ("- joystick  : %s", SDL_JoystickName (0));
          LOG_DBG ("- axes      : %d",
                   SDL_JoystickNumAxes (sdl_joysticks[i]));
          LOG_DBG ("- buttons   : %d",
                   SDL_JoystickNumButtons (sdl_joysticks[i]));
          LOG_DBG ("- trackballs: %d",
                   SDL_JoystickNumBalls (sdl_joysticks[i]));
          LOG_DBG ("- hats      : %d",
                   SDL_JoystickNumHats (sdl_joysticks[i]));
        }
    }
  return TRUE;
}

/**
 * Closes all open joysticks previously
 */
void
display_close_joysticks (void)
{
  Uint32 i;
  if (numof_joysticks < 1 || sdl_joysticks == NULL)
    {
      return;
    }
  for (i = 0; i < numof_joysticks; i++)
    {
      if (sdl_joysticks[i] != NULL)
        {
          SDL_JoystickClose (sdl_joysticks[i]);
          sdl_joysticks[i] = NULL;
        }
    }
  free_memory ((char *) sdl_joysticks);
  sdl_joysticks = NULL;
  numof_joysticks = 0;
}
#endif

/**
 * Shuts down all SDL subsystems and frees the resources allocated to them
 */
void
display_free (void)
{
  free_surfaces ();
  game_offscreen = NULL;
  game_surface = NULL;
  scores_offscreen = NULL;
  options_surface = NULL;
  options_offscreen = NULL;
  score_surface = NULL;
#ifdef USE_SDL_JOYSTICK
  display_close_joysticks ();
#endif
  SDL_Quit ();
  LOG_INF ("SDL_Quit()");
  if (pal16 != NULL)
    {
      free_memory ((char *) pal16);
      pal16 = NULL;
    }
  if (pal32 != NULL)
    {
      free_memory ((char *) pal32);
      pal32 = NULL;
    }
  if (sdl_color_palette != NULL)
    {
      free_memory ((char *) sdl_color_palette);
      sdl_color_palette = NULL;
    }
}

/**
 * Clear the main offscreen
 */
void
display_clear_offscreen (void)
{
  SDL_Rect rect;
  rect.x = (Sint16) offscreen_clipsize;
  rect.y = (Sint16) offscreen_clipsize;
  rect.w = (Uint16) offscreen_width_visible;
  rect.h = (Uint16) offscreen_height_visible;
  if (SDL_FillRect (game_surface, &rect, real_black_color) < 0)
    {
      LOG_ERR ("SDL_FillRect(game_surface) return %s", SDL_GetError ());
    }
}

/**
 * Create an empty SDL surface
 * @param width
 * @param height
 * @return SDL Surface
 */
static SDL_Surface *
create_surface (Uint32 width, Uint32 height)
{
  Uint32 i, rmask, gmask, bmask;
  SDL_Surface *surface;
  Sint32 index = -1;
  for (i = 0; i < MAX_OF_SURFACES; i++)
    {
      if (surfaces_list[i] == NULL)
        {
          index = i;
          break;
        }
    }
  if (index < 0)
    {
      LOG_ERR ("out of 'surfaces_list' list");
      return NULL;
    }
  get_rgb_mask (&rmask, &gmask, &bmask);
  surface =
    SDL_CreateRGBSurface (0, width, height, bits_per_pixel,
                          rmask, gmask, bmask, 0);
  if (surface == NULL)
    {
      LOG_ERR ("SDL_CreateRGBSurface() return %s", SDL_GetError ());
      return NULL;
    }
  if (bytes_per_pixel == 1) {
	SDL_Palette *palette = (SDL_Palette *)malloc(sizeof(sdl_color_palette)*256);
	SDL_SetPaletteColors(palette, sdl_color_palette, 0, 256);
	SDL_SetSurfacePalette(surface, palette);
  }
  surfaces_list[index] = surface;
  surfaces_counter++;
  LOG_DBG ("SDL_CreateRGBSurface(%i,%i,%i)", width, height, bits_per_pixel);
  return surface;
}

static void
get_rgb_mask (Uint32 * rmask, Uint32 * gmask, Uint32 * bmask)
{
  switch (bits_per_pixel)
    {
    case 15:
      *rmask = 0x7c00;
      *gmask = 0x03e0;
      *bmask = 0x001f;
      break;
    case 16:
      *rmask = 0xf800;
      *gmask = 0x03e0;
      *bmask = 0x001f;
      break;
    default:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
      *rmask = 0x00ff0000;
      *gmask = 0x0000ff00;
      *bmask = 0x000000ff;
#else
      *rmask = 0x000000ff;
      *gmask = 0x0000ff00;
      *bmask = 0x00ff0000;
#endif
      break;
    }
}

/**
 * Release a SDL surface
 * @param surface pointer to surface to release
 */
static void
free_surface (SDL_Surface * surface)
{
  Uint32 w;
  Uint32 h;
  Uint32 i;
  for (i = 0; i < MAX_OF_SURFACES; i++)
    {
      if (surfaces_list[i] == surface)
        {
          w = surface->w;
          h = surface->h;
          SDL_FreeSurface (surface);
          surfaces_list[i] = NULL;
          surfaces_counter--;
          LOG_DBG ("SDL_FreeSurface: %ix%i", w, h);
          break;
        }
    }
}

/**
 * Release all SDL surfaces
 */
static void
free_surfaces (void)
{
  Uint32 i;
  Uint32 w;
  Uint32 h;
  for (i = 0; i < MAX_OF_SURFACES; i++)
    {
      if (surfaces_list[i] == NULL)
        {
          continue;
        }
      w = surfaces_list[i]->w;
      h = surfaces_list[i]->h;
      SDL_FreeSurface (surfaces_list[i]);
      surfaces_list[i] = NULL;
      surfaces_counter--;
      LOG_DBG ("SDL_FreeSurface: %ix%i", w, h);
    }
}


void do_fullscreen(bool fstate){
	if (fstate){
		SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else {
		SDL_SetWindowFullscreen(sdlWindow, 0);
	}
}


/*

320x200 mode

+------------------------------+
!  ^         512               !
!<-!- - - - - - - - - - - -  ->!
!                              !
!  !  +------------------+     !
!     !      score       !     !
! 4!  !--------------+---!     !
! 4   !  ^           ! o !     !
! 0!  !  !   256     ! p !     !
!     !<- - - - - -> ! t !     !
!  !  !  !           ! i !     !
!     !   1          ! o !     !
!  !  !  !8          ! n !     !
!     !   4          ! s !     !
!  !  !  !           !<64!     !
!     +--------------+---+     !
!  !   <- - - - - - - - >      !
!            320               !
!  !                           !
+------------------------------+

*/



#endif
