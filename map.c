
/* system includes */

/* project includes */
#include "map.h"

#define CFG_MAP_TABLE "map_table"

#define CFG_KEY "key"
#define CFG_KEY_DEFAULT "id"

#define CFG_PARTITION_ID "partition_id"
#define CFG_PARTITION_ID_DEFAULT "partition_id"

static int map_initialize(cfg_t * configuration)
{
    return 1;
}

static void map_shutdown(void)
{
}

static cfg_opt_t map_table_options[] = {
    CFG_STR(CFG_KEY, CFG_KEY_DEFAULT, 0),
    CFG_STR(CFG_PARTITION_ID, CFG_PARTITION_ID_DEFAULT, 0),
    CFG_END()
};

static cfg_opt_t options[] = {
    CFG_SEC(CFG_MAP_TABLE, map_table_options, CFGF_TITLE | CFGF_MULTI),
    CFG_END()
};

/** @ingroup components */
component map_component = {
    map_initialize,
    map_shutdown,
    options,
    SUBCOMPONENTS_NONE
};
