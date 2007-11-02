# build-time dependencies:
#   gmake
#   flawfinder
#   gcc

CC := gcc
CFLAGS := -std=c99 -Wall -Werror -pedantic

SOURCES := pdb.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all flawfinder splint clean

all: flawfinder splint pdb

pdb: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS)
	rm -f pdb

flawfinder:
	@FF_OUT=`flawfinder -DQ $(SOURCES)`; \
	if [ "$${FF_OUT}" ]; then \
		echo "flawfinder found: "; \
		echo $${FF_OUT}; \
		false; \
	fi

splint:
	@#splint has serious problems on macos
	@#splint $(SOURCES)
