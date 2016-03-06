
gamesdir = $(prefix)/games
scoredir = /var/games/mangadualist
score = mangadualist.hi

games_PROGRAMS = mangadualist
mangadualist_SOURCES = $(SOURCES_MAIN) $(SOURCES_C) $(SOURCES_ASM)
mangadualist_CPFLAGS = -DPREFIX=\"$(prefix)\" \
                    -DSCOREFILE=\"$(scoredir)/$(score)\" \
                    @XLIB_CFLAGS@ @SDL_CFLAGS@ 
mangadualist_LDADD = @XLIB_LIBS@ @SDL_LIBS@ -lm

install-data-hook:
	-chown root:games "$(DESTDIR)/$(gamesdir)/mangadualist"
	-chmod 2755 "$(DESTDIR)/$(gamesdir)/mangadualist"
	$(INSTALL) -d $(DESTDIR)/$(scoredir)
	-for suffix in "-easy" "" "-hard"; do \
	  touch "$(DESTDIR)/$(scoredir)/$(score)$$suffix"; \
	  chown root:games "$(DESTDIR)/$(scoredir)/$(score)$$suffix"; \
	  chmod 644 "$(DESTDIR)/$(scoredir)/$(score)$$suffix"; \
	done

if ASSEMBLY
#SOURCES_EXTRA = assembler.S assembler_opt.S assembler.h
SOURCES_ASM = assembler.S assembler_opt.S assembler.h
else
#SOURCES_EXTRA = gfxroutines.c gfxroutines.h
SOURCES_C = gfxroutines.c gfxroutines.h
endif

SOURCES_MAIN = \
  mangadualist.c \
  bonus.c \
  bonus.h \
  congratulations.c \
  congratulations.h \
  config_file.c \
  config_file.h \
  curve_phase.c \
  curve_phase.h \
  display.c \
  display.h \
  display_sdl.c \
  display_x11.c \
  electrical_shock.c \
  enemies.c \
  enemies.h \
  energy_gauge.c \
  energy_gauge.h \
  explosions.c \
  explosions.h \
  extra_gun.c \
  extra_gun.h \
  gfx_wrapper.c \
  gfx_wrapper.h \
  grid_phase.c \
  grid_phase.h \
  guardians.c \
  guardians.h \
  images.c \
  images.h \
  inits_game.c \
  lispreader.c \
  lispreader.h \
  lonely_foes.c \
  lonely_foes.h \
  main.c \
  menu.c \
  menu.h \
  menu_sections.c \
  menu_sections.h \
  meteors_phase.c \
  meteors_phase.h \
  movie.c \
  movie.h \
  log_recorder.c \
  log_recorder.h \
  options_panel.c \
  options_panel.h \
  mangadualist.h \
  scalebit.c \
  scalebit.h \
  scale2x.c \
  scale2x.h \
  scale3x.c \
  scale3x.h \
  sdl_mixer.c \
  sdl_mixer.h \
  satellite_protections.c \
  satellite_protections.h \
  scrolltext.c \
  scrolltext.h \
  spaceship.c \
  spaceship.h \
  spaceship_weapons.c \
  spaceship_weapons.h \
  special_keys.c \
  sprites_string.c \
  sprites_string.h \
  shots.c \
  shots.h \
  shockwave.c \
  shockwave.h \
  starfield.c \
  starfield.h \
  text_overlay.c \
  text_overlay.h \
  texts.c \
  texts.h \
  tools.c \
  tools.h
