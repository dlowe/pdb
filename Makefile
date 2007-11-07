# build-time dependencies:
#   gmake
#   flawfinder
#   gcc
#   gnuindent
#   doxygen
#   perl
#   prove

CC := gcc
CFLAGS := -std=c99 -Wall -Werror -pedantic -ggdb

FLAWFINDER := /usr/local/bin/flawfinder -DQ -m 3
GNUINDENT := /opt/local/bin/gnuindent -kr -hnl -nut -ncs -l78 -st
DOXYGEN := /Applications/Doxygen.app/Contents/Resources/doxygen doxygen.cfg

.DELETE_ON_ERROR %.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
	@FF_OUT=`$(FLAWFINDER) $<`; \
	if [ "$${FF_OUT}" ]; then \
                $(FLAWFINDER) $<; \
		echo "flawfinder flagged $<"; \
		false; \
	fi
	@DIFF_OUT=`$(GNUINDENT) $< | diff $< -`; \
	if [ "$${DIFF_OUT}" ]; then \
                $(GNUINDENT) $< | diff $< -; \
		echo "style mismatch in $<"; \
		false; \
	fi

SOURCES := daemon.c concurrency.c mysql_server.c pdb.c
HEADERS := daemon.h concurrency.h
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean test

all: pdb doxygen

pdb: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

test: pdb
	prove -r test

doxygen: $(SOURCES) $(HEADERS) doxygen.cfg
	rm -rf $@
	$(DOXYGEN)

clean:
	rm -f $(OBJECTS)
	rm -f pdb
	rm -rf doxygen

daemon.o: daemon.h
concurrency.o: concurrency.h
mysql_server.o: mysql_server.h
pdb.o: daemon.h concurrency.h mysql_server.h
