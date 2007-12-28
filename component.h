#ifndef _COMPONENT_H
#define _COMPONENT_H

/* 3rd party includes */
#include "confuse.h"

typedef struct component_struct component;
struct component_struct {
    int (*initialize)(cfg_t *);
    void (*shutdown)(void);
    cfg_opt_t *options;
    component **subcomponents;
};

int component_configure(char *configuration_filename, component *root);
void component_unconfigure(component *root);

#endif
