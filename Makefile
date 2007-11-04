# build-time dependencies:
#   gmake
#   flawfinder
#   gcc

CC := gcc
CFLAGS := -std=c99 -Wall -Werror -pedantic

FLAWFINDER := /usr/local/bin/flawfinder -DQ -m 3
GNUINDENT := /opt/local/bin/gnuindent -kr -hnl -nut -ncs -l78 -st

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

SOURCES := daemon.c pdb.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean

all: pdb

pdb: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS)
	rm -f pdb
