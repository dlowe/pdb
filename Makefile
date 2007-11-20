# build-time dependencies:
#   gmake
#   flawfinder
#   gcc
#   gnuindent
#   doxygen
#   perl (Test, Socket, DBI, DBD::Pg, DBD::mysql)
#   prove
#   gcov

CC := gcc
CFLAGS := -std=c99 -Wall -Werror -pedantic -ggdb
# CFLAGS := -fprofile-arcs -ftest-coverage -std=c99 -Wall -Werror -pedantic -ggdb

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

SOURCES := daemon.c concurrency.c db_driver.c mysql_driver.c pdb.c delegate.c
HEADERS := daemon.h concurrency.h db_driver.h mysql_driver.h command.h delegate.h reply.h
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all all-no-test clean test

all: all-no-test test

all-no-test: pdb doxygen

pdb: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)
	# $(CC) -o $@ $(OBJECTS) -lgcov

test: pdb
	prove -r test
	# gcov $(SOURCES)

doxygen: $(SOURCES) $(HEADERS) doxygen.cfg
	rm -rf $@
	$(DOXYGEN)

clean:
	rm -f dependencies.mk
	rm -f $(OBJECTS)
	rm -f pdb
	rm -rf doxygen
	rm -rf *.gcda *.gcno *.gcov
	rm -rf ktrace.out

-include dependencies.mk

dependencies.mk: $(SOURCES) $(HEADERS)
	$(CC) -MM $(SOURCES) > dependencies.mk
