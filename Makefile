DESTDIR    =
PREFIX     =/usr/local
WEB_DIR    =/usr/local/share/c-ruri/cgi-bin
CC         =gcc
CFLAGS     =-Wall -g -O3
AR         =ar
CPPFLAGS   =
LDFLAGS    =
LIBS       = "-l:libhiredis.a"
CFLAGS_ALL =$(LDFLAGS) $(CFLAGS) $(CPPFLAGS)

all: ruri.cgi ruri libruri.a ruri.h
install: all
	mkdir -p     $(DESTDIR)$(WEB_DIR)
	mkdir -p     $(DESTDIR)$(PREFIX)/bin
	mkdir -p     $(DESTDIR)$(PREFIX)/lib
	mkdir -p     $(DESTDIR)$(PREFIX)/include
	cp ruri.cgi  $(DESTDIR)$(WEB_DIR)
	cp ruri      $(DESTDIR)$(PREFIX)/bin
	cp libruri.a $(DESTDIR)$(PREFIX)/lib
	cp ruri.h    $(DESTDIR)$(PREFIX)/include
clean:
	rm -f ruri.cgi ruri libruri.a

ruri.cgi: ruri.cgi.c ruri.h
	$(CC) -o $@ ruri.cgi.c $(CFLAGS_ALL) $(LDFLAGS) $(LIBS)
ruri: ruri.c ruri.h libruri.a
	$(CC) -o $@ ruri.c libruri.a $(CFLAGS_ALL) $(LDFLAGS) $(LIBS)
libruri.a: ruri.h ruri-lib.c
	$(CC) -c ruri-lib.c $(CFLAGS_ALL) $(LDFLAGS) $(LIBS)
	$(AR) -crs $@ ruri-lib.o
	rm -f ruri-lib.o



## -- license --
ifneq ($(PREFIX),)
install: install-license
install-license: LICENSE
	mkdir -p $(DESTDIR)$(PREFIX)/share/doc/c-ruri
	cp LICENSE $(DESTDIR)$(PREFIX)/share/doc/c-ruri
endif
## -- license --
