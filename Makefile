CC = gcc
PKGCONFIG = $(shell which pkg-config)
CFLAGS = `$(PKGCONFIG) --cflags gtk+-3.0`
LIBS = `$(PKGCONFIG) --libs gtk+-3.0` `$(PKGCONFIG) --libs jansson` -lcrypto
GLIB_COMPILE_RESOURCES = `$(PKGCONFIG) --variable=glib_compile_resources gio-2.0`
GLIB_COMPILE_SCHEMAS = `$(PKGCONFIG) --variable=glib_compile_schemas gio-2.0`

SRC = resources.c gonepassapp.c appwindow.c main.c prefsdialog.c unlockdialog.c

OBJS = $(SRC:.c=.o)

all: gonepassapp

org.gtk.gonepassapp.gschema.valid: org.gtk.gonepassapp.gschema.xml
	$(GLIB_COMPILE_SCHEMAS) --strict --dry-run --schema-file=$< && mkdir -p $(@D) && touch $@

gschemas.compiled: org.gtk.gonepassapp.gschema.valid
	$(GLIB_COMPILE_SCHEMAS) .

resources.c: gonepassapp.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies gonepassapp.gresource.xml)
	$(GLIB_COMPILE_RESOURCES) gonepassapp.gresource.xml --target=$@ --sourcedir=. --generate-source

%.o: %.c
	$(CC) -c -g -O0 -o $(@F) $(CFLAGS) $<

gonepassapp: $(OBJS) gschemas.compiled
	$(CC) -o $(@F) $(LIBS) $(OBJS)

clean:
	rm -f org.gtk.gonepassapp.gschema.valid
	rm -f gschemas.compiled
	rm -f resources.c
	rm -f $(OBJS)
	rm -f gonepassapp
