# build-time dependencies:
#   gmake
#   flawfinder
#   gcc

CC := gcc
CFLAGS := -std=c99 -Wall -Werror -pedantic

FLAWFINDER := /usr/local/bin/flawfinder
GNUINDENT := /opt/local/bin/gnuindent

.DELETE_ON_ERROR %.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
	@FF_OUT=`$(FLAWFINDER) -DQ $<`; \
	if [ "$${FF_OUT}" ]; then \
		echo $${FF_OUT}; \
		echo "flawfinder flagged $<"; \
		false; \
	fi
	@if grep -Hn '	' $<; then \
		echo "tabs in $<"; \
		false; \
	fi
	@DIFF_OUT=`$(GNUINDENT) -kr -nut -ncs -l78 -st $< | diff $< -`; \
	if [ "$${DIFF_OUT}" ]; then \
                $(GNUINDENT) -kr -nut -ncs -l78 -st $< | diff $< -; \
		echo "style mismatch in $<"; \
		false; \
	fi

SOURCES := pdb.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean

all: pdb

pdb: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS)
	rm -f pdb
