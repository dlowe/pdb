#ifndef _COMPONENT_H
#define _COMPONENT_H

/**
 * @file component.h
 * @brief Framework for configuration, booting and shutting down of components.
 *
 * This API allows the definition of a hierarchy of "components", which are
 * then configured, initialized and shut down by the framework. The objective
 * is to allow the components to declare the configuration values they are
 * interested in, so that there's no need for global configuration knowledge.
 */

/* 3rd party includes */
#include "confuse.h"

/** A component declares itself (in its header) via this macro. */
#define DECLARE_COMPONENT(name) extern component name ## _component

#define INITIALIZE_NONE 0
#define SHUTDOWN_NONE 0
#define OPTIONS_NONE 0
#define SUBCOMPONENTS_NONE 0

#define SUBCOMPONENT(name) & name ## _component
#define SUBCOMPONENT_END() 0

typedef struct component_struct component;
struct component_struct {
    int (*initialize)(cfg_t *);
    void (*shutdown)(void);
    cfg_opt_t *options;
    component **subcomponents;
};

/**
 * Configure and initialize all components.
 *
 * @param[in] configuration_filename The name of the configuration file.
 * @param[in] root Root of the component tree.
 * @return 1 on success, 0 on failure
 */
int component_configure(char *configuration_filename, component *root);

/**
 * Shut down all components.
 *
 * @param[in] root Root of the component tree.
 */
void component_unconfigure(component *root);

#endif
