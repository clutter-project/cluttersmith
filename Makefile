# Hytte a generic buildfiles to build single executable directory projects
# depending only on pkg-config ability to build. 
#
# Setting additional CFLAGS like $ export CFLAGS=-Wall -Werror # can help you
# track issues down better after compilation.
#
# Authored by Øyvind Kolås 2007-2008 <pippin@gimp.org> 
# placed in the Public Domain.
#

PROJECT_NAME=$(shell basename `pwd`)

# Dependencies of the project.
PKGMODULES = glib-2.0 gio-2.0 clutter-1.0 nbtk-1.0 clutter-gst-0.10

#extra CFLAGS
CFLAGS=-Wall 
#extra LIBS
LIBS=-rdynamic

TEST_PARAMS=json/menu.json -id actor   # parameters passed to binary when testing

# the locations used for web, git-sync and snapshot targets:

DOCS_REMOTE=pippin@gimp.org:public_html/$(PROJECT_NAME)
GIT_RSYNC_REMOTE=$(DOCS_REMOTE)/$(PROJECT_NAME).git
SNAPSHOT_REMOTE=$(DOCS_REMOTE)/snapshots


#
# end of project/user high level configuration.
#

# This makefile uses the current directory as the only target binary, and
# expects a single of the .c files to contain a main function.

BINARY=$(PROJECT_NAME)
all: DOCUMENTATION $(BINARY) clutterbug.so

DOCUMENTATION:
	@test -d docs && make -C docs || true
web: DOCUMENTATION
	@test -d docs && scp docs/*.html $(DOCS_REMOTE) || true


TIMESTAMP=$(shell date +%Y%m%d-%H%M)
PACKAGE=../$(BINARY)-$(TIMESTAMP).tar.bz2

INSTALL=$(shell which install)

# The help available also contains brief information about the different
#
# build rules supported.
help: 
	@echo '$(PROJECT_NAME) make targets.'
	@echo ''
	@echo '  (none)   builds $(PROJECT_NAME)'
	@echo '  dist     create $(PACKAGE)'
	@echo '  clean    rm *.o *~ and foo and bar'
	@echo '  test     ./$(BINARY)'
	@echo '  gdb      gdb ./$(BINARY)'
	@echo '  gdb2     gdb ./$(BINARY) --g-fatal-warnings'
	@echo '  valgrind valgrind ./$(BINARY)'
	@echo '  git-sync rsync git repo at $(GIT_RSYNC_REMOTE)'
	@echo '  snapshot put tarball at $(SNAPSHOT_REMOVE)'
	@echo '  help     this help'
	@test -d docs && echo '  web      publish website'||true
	@echo ''


LIBS+= $(shell pkg-config --libs $(PKGMODULES) | sed -e 's/-Wl,\-\-export\-dynamic//')
INCS= $(shell pkg-config --cflags $(PKGMODULES)) -I.. -I.


CFILES  = $(wildcard *.c) $(wildcard */*.c)
HFILES  = $(wildcard *.h) $(wildcard */*.h)
# we rebuild on every .h file change this could be improved by making
# deps files.
OBJECTS = $(subst ./,,$(CFILES:.c=.o))

%.o: %.c $(HFILES) 
	@$(CC) -g $(CFLAGS) $(INCS) -c $< -o $@\
	    && echo "$@ compiled" \
	    || ( echo "$@ **** compile failed ****"; false)
$(BINARY): $(OBJECTS)
	@$(CC) -o $@ $(OBJECTS) $(LIBS) \
	    && echo "$@ linked" \
	    || (echo "$@ **** linking failed ****"; false)

test: run
run: $(BINARY)
	@echo -n "$$ "
	./$(BINARY) $(TEST_PARAMS)


PREFIX=/usr/local

install: $(BINARY)
	$(INSTALL) $(BINARY) $(PREFIX)/bin/
	@test -d docs && make -C docs clean || true


clean:
	@rm -fvr *.o */*.o $(BINARY) *~ */*~ *.patch $(CLEAN_LOCAL)
	@test -d docs && make -C docs clean || true


gdb: all
	gdb --args ./$(BINARY)
gdb2: all
	gdb --args ./$(BINARY) -demo --g-fatal-warnings
valgrind: all
	valgrind --leak-check=full ./$(BINARY)
callgrind: all
	valgrind --tool=callgrind ./$(BINARY)



../$(BINARY)-$(TIMESTAMP).tar.bz2: clean $(CFILES) $(HFILES)
	cd ..;tar cjvf $(PROJECT_NAME)-$(TIMESTAMP).tar.bz2 $(PROJECT_NAME)/*
	@ls -slah ../$(PROJECT_NAME)-$(TIMESTAMP).tar.bz2

dist: $(PACKAGE) 
snapshot: dist
	scp $(PACKAGE) $(SNAPSHOT_REMOTE)
git-sync:
	git checkout master
	git-update-server-info
	rsync --delete -avprl .git/* $(GIT_RSYNC_REMOTE)


#### custom rules for project, could override a specific .o file for instance
#
#all_local: foo
#all: all_local
#
#foo: Makefile
#	touch foo
#
#CLEAN_LOCAL=foo


clutterbug.so: *.c Makefile
	gcc -DCLUTTERBUG -g -shared -ldl *.c -o clutterbug.so `pkg-config clutter-1.0 clutter-gtk-0.10 nbtk-1.0 --libs --cflags` 

CLUTTER_HEADERS=/usr/local/include/clutter-1.0/clutter/
NBTK_HEADERS=/usr/local/include/nbtk-1.0/nbtk/

init-types.c:  
	echo "#include <clutter/clutter.h>" > $@
	echo "#include <nbtk/nbtk.h>" >> $@
	echo "void init_types(void){gint i = 0;" >> $@
	for i in `grep CLUTTER_TYPE_ $(CLUTTER_HEADERS)*.h | sed "s/.*CLUTTER_TYPE_/CLUTTER_TYPE_/" | sed "s/[^_A-Z].*$$//"|sort|uniq`;do \
	    echo "i = $$i;" >> $@\
	 ;done
	for i in `grep NBTK_TYPE_ $(NBTK_HEADERS)*.h | sed "s/.*NBTK_TYPE_/NBTK_TYPE_/" | sed "s/[^_A-Z].*$$//"|sort|uniq`;do \
	    echo "i = $$i;" >> $@\
	 ;done
	echo "}" >> $@

