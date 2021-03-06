
/* system includes */
#include <stdlib.h>
#include <string.h>

/* project includes */
#include "sql.h"
#include "log.h"

typedef struct {
    char *name;
    char *key;
} partitioned_table;

#define CFG_PARTITIONED_TABLE "partitioned_table"

#define CFG_KEY "key"
#define CFG_KEY_DEFAULT "id"

static partitioned_table *partitioned_tables = 0;
static int partitioned_table_count = 0;

static int sql_initialize(cfg_t * configuration)
{
    partitioned_table_count = cfg_size(configuration, CFG_PARTITIONED_TABLE);
    partitioned_tables =
        malloc(sizeof(partitioned_table) * partitioned_table_count);
    if (!partitioned_tables) {
        partitioned_table_count = 0;
        return 0;
    }

    for (int i = 0; i < partitioned_table_count; ++i) {
        cfg_t *partitioned_table_config =
            cfg_getnsec(configuration, CFG_PARTITIONED_TABLE, i);

        partitioned_tables[i].name =
            strdup(cfg_title(partitioned_table_config));
        if (!partitioned_tables[i].name) {
            return 0;
        }
        partitioned_tables[i].key =
            strdup(cfg_getstr(partitioned_table_config, CFG_KEY));
        if (!partitioned_tables[i].key) {
            return 0;
        }
    }
    return 1;
}

sql_type sql_get_type(char *sql)
{
    if (strncmp(sql, "update", 6) == 0) {
        char *p = sql + 7;

        if (strcspn(p, " ")) {
            size_t end_of_table = strcspn(p, " ");
            char save = p[end_of_table];
            p[end_of_table] = 0;
            sql_table_type table_type = sql_get_table_type(p);
            p[end_of_table] = save;

            switch (table_type) {
            case SQL_TABLE_TYPE_MASTER:
                return SQL_TYPE_MASTER;
            case SQL_TABLE_TYPE_PARTITIONED:
                return SQL_TYPE_PARTITIONED;
            };
        }
    }
    return SQL_TYPE_PARTITIONED;
}

static char *sql_get_table_key(char *table)
{
    for (int i = 0; i < partitioned_table_count; ++i) {
        lo(LOG_DEBUG, "sql_get_table_key(): %s == %s?", table,
           partitioned_tables[i].name);
        if (strcmp(table, partitioned_tables[i].name) == 0) {
            return partitioned_tables[i].key;
        }
    }
    return NULL;
}

long *sql_get_map_keys(char *sql)
{
    if (strncmp(sql, "update", 6) == 0) {
        char *p = sql + 7;

        if (strcspn(p, " ")) {
            size_t end_of_table = strcspn(p, " ");
            char save = p[end_of_table];
            p[end_of_table] = '\0';
            char *key = sql_get_table_key(p);
            p[end_of_table] = save;

            /* slide p past the table name and following space */
            p += end_of_table;
            p += strspn(p, " ");

            lo(LOG_DEBUG, "sql_get_map_keys: after finding table: %s", p);

            char *w = strstr(p, "where");
            if (w != NULL) {
                /* slide p past the 'where' token and following space */
                p = w;
                p += strspn(p, " ");

                lo(LOG_DEBUG, "sql_get_map_keys: after finding where: %s", p);

                char *k = strstr(p, key);
                if (k != NULL) {
                    /* slide p past the key */
                    p = k + strlen(key);
                    p += strspn(p, " ");

                    lo(LOG_DEBUG, "sql_get_map_keys: after finding %s: %s",
                       key, p);

                    if (*p == '=') {
                        ++p;
                        p += strspn(p, " ");

                        lo(LOG_DEBUG, "sql_get_map_keys: after finding =: %s",
                           p);

                        long map_key = atol(p);

                        lo(LOG_DEBUG, "sql_get_map_keys: XX: %ld", map_key);
                    }
                }
            }
        }
    }

    return NULL;
}

sql_table_type sql_get_table_type(char *table)
{
    for (int i = 0; i < partitioned_table_count; ++i) {
        lo(LOG_DEBUG, "sql_get_table_type(): %s == %s?", table,
           partitioned_tables[i].name);
        if (strcmp(table, partitioned_tables[i].name) == 0) {
            return SQL_TABLE_TYPE_PARTITIONED;
        }
    }
    return SQL_TABLE_TYPE_MASTER;
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

/** @ingroup components */
component sql_component = {
    sql_initialize,
    sql_shutdown,
    options,
    SUBCOMPONENTS_NONE
};
