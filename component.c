/* system includes */
#include <stdlib.h>

/* 3rd party includes */
#include "confuse.h"

/* project includes */
#include "component.h"

/**
 * This higher-order function maps a callback across the tree of components.
 *
 * @param[in] node the current component node
 * @param[in,out] state pointer to any state required by the callback
 * @param[in] callback callback function to be called for each node
 */
static void component_map(component * node, void *state,
                          void (*callback) (component *, void *))
{
    callback(node, state);
    if (node->subcomponents != SUBCOMPONENTS_NONE) {
        for (int i = 0; node->subcomponents[i] != SUBCOMPONENT_END(); ++i) {
            component_map(node->subcomponents[i], state, callback);
        }
    }
}

/**
 * Arguments to option_list_callback
 */
typedef struct {
    cfg_opt_t *options;
    int count;
    short error;
} option_list_callback_state;

/**
 * component callback function for aggregating configuration options.
 *
 * @param[in] c The current component
 * @param[in,out] s Pointer to option_list_callback_state
 */
static void option_list_callback(component * c, void *s)
{
    option_list_callback_state *state = (option_list_callback_state *) s;

    if (state->error) {
        return;
    }

    if (c->options != OPTIONS_NONE) {
        for (int i = 0; c->options[i].name; ++i) {
            state->options = realloc(state->options,
                                     (state->count + 1) * sizeof(cfg_opt_t));
            if (!state->options) {
                state->error = 1;
                return;
            }

            state->options[state->count] = c->options[i];
            ++(state->count);
        }
    }
}

/**
 * Arguments to initializer_callback
 */
typedef struct {
    cfg_t *configuration;
    short error;
} initializer_callback_state;

/**
 * component callback function for calling initializers.
 *
 * @param[in] c The current component
 * @param[in,out] s Pointer to initializer_callback_state
 */
static void initializer_callback(component * c, void *s)
{
    initializer_callback_state *state = (initializer_callback_state *) s;

    if (state->error) {
        return;
    }

    if (c->initialize != INITIALIZE_NONE) {
        if (!c->initialize(state->configuration)) {
            state->error = 1;
        }
    }
}

int component_configure(const char *configuration_filename, component * root)
{
    option_list_callback_state option_state;
    option_state.options = 0;
    option_state.count = 0;
    option_state.error = 0;
    component_map(root, &option_state, option_list_callback);
    if (option_state.error) {
        return 0;
    }

    initializer_callback_state init_state;
    init_state.error = 0;
    init_state.configuration = cfg_init(option_state.options, CFGF_NONE);
    if (!init_state.configuration) {
        free(option_state.options);
        return 0;
    }

    if (cfg_parse(init_state.configuration, configuration_filename)
        != CFG_SUCCESS) {
        cfg_free(init_state.configuration);
        free(option_state.options);
        return 0;
    }
    component_map(root, &init_state, initializer_callback);
    if (init_state.error) {
        cfg_free(init_state.configuration);
        free(option_state.options);
        return 0;
    }

    cfg_free(init_state.configuration);
    free(option_state.options);
    return 1;
}


/**
 * component callback function for calling shutdowns.
 *
 * @param[in] c The current component
 * @param[in] s NULL pointer
 */
static void shutdown_callback(component * c, void *void_state)
{
    if (c->shutdown != SHUTDOWN_NONE) {
        c->shutdown();
    }
}

void component_unconfigure(component * root)
{
    component_map(root, NULL, shutdown_callback);
}
