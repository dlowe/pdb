
/* system includes */

/* project includes */
#include "sql.h"

#define CFG_PARTITIONED_TABLE "partitioned_table"

#define CFG_KEY "key"
#define CFG_KEY_DEFAULT "id"

static int sql_initialize(cfg_t * configuration)
{
    return 1;
}

static void sql_shutdown(void)
{
}

static cfg_opt_t partitioned_table_options[] = {
    CFG_STR(CFG_KEY, CFG_KEY_DEFAULT, 0),
    CFG_END()
};

static cfg_opt_t options[] = {
    CFG_SEC(CFG_PARTITIONED_TABLE, partitioned_table_options,
            CFGF_TITLE | CFGF_MULTI),
    CFG_END()
};

component sql_component = {
    sql_initialize,
    sql_shutdown,
    options,
    SUBCOMPONENTS_NONE
};
