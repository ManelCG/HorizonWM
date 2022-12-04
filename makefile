BASENAME = horizonwm
WMNAME = HorizonWM
VERSION = "0.1.0"

SDIR = src

IDIR = include
LOCALENAME = $(BASENAME)

PROGRAMEXTRAFLAGS = -DHORIZONPATH=$(MEAD_PATH) -DWALLPAPERCMD=\"$(MEAD_PATH)/customiz3d/menu.sh\" -DROFIFULLCNFG=\"$(HOME)/.config/rofi/config.rasi\" -DROFIBARCNFG=\"$(HOME)/.config/rofi/bar.rasi\"



CCCMD = gcc
CFLAGS = -I$(IDIR) -Wall -Wno-deprecated-declarations -pedantic -Os -I/usr/X11R6/include -I/usr/include/freetype2 -lXrender -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DXINERAMA -lX11 -lXinerama $(PROGRAMEXTRAFLAGS) -lfontconfig -lXft  -DLOCALE_=\"$(LOCALENAME)\" -pthread -DWMNAME=\"$(WMNAME)\"

debug: CC = $(CCCMD) -DDEBUG_ALL -DVERSION=\"$(VERSION)_DEBUG\"
debug: BDIR = build

release: CC = $(CCCMD) -DVERSION=\"$(VERSION)\"
release: BDIR = build

windows: CC = $(CCCMD) -DVERSION=\"$(VERSION)\" -mwindows
windows: BDIR = build
windows_GTKENV: BDIR = build

install: CC = $(CCCMD) -DMAKE_INSTALL -DVERSION=\"$(VERSION)\"
install: PROGDIR = /usr/lib/$(BASENAME)
install: BDIR = $(PROGDIR)/bin

archlinux: CC = $(CCCMD) -DMAKE_INSTALL -DVERSION=\"$(VERSION)\"

locale: LOCALEDIR = locale

ODIR=.obj/linux
DODIR=.obj/debug
LDIR=lib

LIBS = -lm -lpthread

_DEPS = config.h drw.h util.h spawn_programs.h horizonwm_type_definitions.h bar_modules.h helper_scripts.h global_vars.h menu_scripts.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = dwm.o drw.o util.o spawn_programs.o bar_modules.o helper_scripts.o menu_scripts.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
WOBJ = $(patsubst %,$(WODIR)/%,$(_OBJ))
DOBJ = $(patsubst %,$(DODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(DODIR)/%.o: $(SDIR)/%.c $(DEPS)
	mkdir -p $(DODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

release: $(OBJ)
	mkdir -p $(BDIR)
	mkdir -p $(ODIR)
	$(CC) -o $(BDIR)/$(BASENAME) $^ $(CFLAGS) $(LIBS)

debug: $(DOBJ)
	mkdir -p $(BDIR)
	mkdir -p $(DODIR)
	$(CC) -o $(BDIR)/$(BASENAME)_DEBUG $^ $(CFLAGS) $(LIBS)

install: $(OBJ)
	mkdir -p $(PROGDIR)
	mkdir -p $(BDIR)
	mkdir -p $(ODIR)
	mkdir -p /usr/lib/$(BASENAME)
	$(CC) -o $(BDIR)/$(BASENAME) $^ $(CFLAGS) $(LIBS)
	ln -sf $(BDIR)/$(BASENAME) /usr/bin/$(BASENAME)
	#cp -ru assets/ /usr/lib/$(BASENAME)
	#cp -ru locale/ /usr/lib/$(BASENAME)
	cp LICENSE /usr/lib/$(BASENAME)
	#cp assets/$(BASENAME).desktop /usr/share/applications/
	#cp assets/app_icon/256.png /usr/share/pixmaps/$(BASENAME).png

archlinux: $(OBJ) $(OBJ_GUI)
	mkdir -p $(BDIR)/usr/lib/$(BASENAME)
	mkdir -p $(BDIR)/usr/share/applications
	mkdir -p $(BDIR)/usr/share/pixmaps
	mkdir -p $(BDIR)/usr/bin/
	mkdir -p $(ODIR)
	$(CC) -o $(BDIR)/usr/bin/$(BASENAME) $^ $(CFLAGS) $(LIBS)
	#cp -ru assets/ $(BDIR)/usr/lib/$(BASENAME)/
	#cp -ru locale/ $(BDIR)/usr/lib/$(BASENAME)/
	cp LICENSE $(BDIR)/usr/lib/$(BASENAME)/
	#cp assets/$(BASENAME).desktop $(BDIR)/usr/share/applications/
	#cp assets/app_icon/256.png $(BDIR)/usr/share/pixmaps/$(BASENAME).png

locale: $(LOCALEDIR)/es/LC_MESSAGES/$(LOCALENAME).mo $(LOCALEDIR)/ru/LC_MESSAGES/$(LOCALENAME).mo $(LOCALEDIR)/ca/LC_MESSAGES/$(LOCALENAME).mo

$(LOCALEDIR)/%/LC_MESSAGES/$(LOCALENAME).mo: $(LOCALEDIR)/%/$(LOCALENAME).po
	msgfmt --output-file=$(LOCALEDIR)/$*/LC_MESSAGES/$(LOCALENAME).mo $(LOCALEDIR)/$*/$(LOCALENAME).po
$(LOCALEDIR)/%/$(LOCALENAME).po: $(LOCALEDIR)/$(LOCALENAME).pot
	msgmerge --update $(LOCALEDIR)/$*/$(LOCALENAME).po $(LOCALEDIR)/$(LOCALENAME).pot
	mkdir -p $(LOCALEDIR)/$*/LC_MESSAGES
$(LOCALEDIR)/$(LOCALENAME).pot: $(SDIR)/*
	xgettext --keyword=_ --language=C --from-code=UTF-8 --add-comments --sort-output -o $(LOCALEDIR)/$(LOCALENAME).pot $(SDIR)/*.c

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(WODIR)/*.o $(DODIR)/*.o *~ core $(INCDIR)/*~

.PHONY: all
all: release clean
