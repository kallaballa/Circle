#Copyright (C) 2007 L. Donnie Smith

include ../defs.mak

APP_NAME = wmgui

SOURCES = main.c interface.c support.c

CFLAGS += -pthread -I/usr/include/gtk-2.0 -I/usr/lib64/gtk-2.0/include -I/usr/include/pango-1.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/libdrm -I/usr/include/libpng16 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/freetype2 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I../libcwiid
LDFLAGS += -L../libcwiid
LDLIBS += -lgtk-x11-2.0 -lgdk-x11-2.0 -lpangocairo-1.0 -latk-1.0 -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lfontconfig -lfreetype -lgthread-2.0 -pthread -lglib-2.0 -lm -lcwiid -lbluetooth -lpthread -lrt
INST_DIR = ${exec_prefix}/bin

include $(COMMON)/include/app.mak

distclean: clean
	rm Makefile

.PHONY: distclean
