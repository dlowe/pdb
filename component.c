/* system includes */
#include <stdlib.h>

/* 3rd party includes */
#include "confuse.h"

/* project includes */
#include "component.h"

static void component_map(component * node, void *state,
                          void (*callback) (component *, void *))
{
    callback(node, state);
    if (node->subcomponents) {
        for (int i = 0; node->subcomponents[i]; ++i) {
            component_map(node->subcomponents[i], state, callback);
        }
    }
}

typedef struct {
    cfg_opt_t *options;
    int count;
    short error;
} option_list_callback_state;

static void option_list_callback(component * c, void *s)
{
    option_list_callback_state *state = (option_list_callback_state *) s;

    if (state->error) {
        return;
    }

    if (c->options) {
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

typedef struct {
    cfg_t *configuration;
    short error;
} initializer_callback_state;

static void initializer_callback(component * c, void *s)
{
    initializer_callback_state *state = (initializer_callback_state *) s;

    if (state->error) {
        return;
    }

    if (c->initialize) {
        c->initialize(state->configuration);
    }
}

int component_configure(char *configuration_filename, component * root)
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

static void shutdown_callback(component * c, void *s)
{
    if (c->shutdown) {
        c->shutdown();
    }
}

void component_unconfigure(component * root)
{
    component_map(root, NULL, shutdown_callback);
}
