#ifndef _COMPONENT_H
#define _COMPONENT_H

/** @defgroup components Components
 *
 * Components are the high-level building blocks of this software. They're
 * also the parts of the system which can read configuration.
 *
 * Components are represented as a tree, internally: each node has a config
 * reader function, a shutdown function, a libconfuse cfg_opt_t object which
 * describes its configuration, and (optionally) a list of subcomponents.
 *
 * For the most part, the tree structure also describes the expected
 * coupling between the components -- i.e. while a parent may call
 * into its children, children don't call into their parents or peers.
 */

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

/** A component declares its subcomponents via this macro */
#define SUBCOMPONENT(name) & name ## _component
#define SUBCOMPONENT_END() 0

//typedef struct component_struct component;

/**
 * Component definition record.
 */
typedef struct component_struct {
    int (*initialize) (cfg_t *); /**< config reading function */
    void (*shutdown) (void);     /**< shutdown function */
    cfg_opt_t *options;          /**< config options for this component */
    struct component_struct **subcomponents;   /**< children of this component */
} component;

/**
 * Configure and initialize all components.
 *
 * @param[in] configuration_filename The name of the configuration file.
 * @param[in] root Root of the component tree.
 * @return 1 on success, 0 on failure
 */
int component_configure(const char *configuration_filename, component * root);

/**
 * Shut down all components.
 *
 * @param[in] root Root of the component tree.
 */
void component_unconfigure(component * root);

#endif
