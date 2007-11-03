# build-time dependencies:
#   gmake
#   flawfinder
#   gcc

CC := gcc
CFLAGS := -std=c99 -Wall -Werror -pedantic

SOURCES := pdb.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean
.PHONY: check flawfinder splint no-tabs

all: check pdb

pdb: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS)
	rm -f pdb

check: flawfinder no-tabs

flawfinder:
	@FF_OUT=`flawfinder -DQ $(SOURCES)`; \
	if [ "$${FF_OUT}" ]; then \
		echo "flawfinder found: "; \
		echo $${FF_OUT}; \
		false; \
	fi

splint:
	splint $(SOURCES)

no-tabs:
	@if grep -Hn '	' $(SOURCES); then \
		echo "evil tabs!"; \
		false; \
	fi
