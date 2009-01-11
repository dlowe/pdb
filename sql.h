#ifndef __SQL_H
#define __SQL_H

/**
 * @file sql.h
 * @brief SQL parsing
 * 
 * The sql component should be exclusively used by the server component.
 */

#include "component.h"
#include "delegate_filter.h"

/** @cond */
DECLARE_COMPONENT(sql);
/** @endcond */

typedef enum {
    SQL_TABLE_TYPE_MASTER,
    SQL_TABLE_TYPE_PARTITIONED
} sql_table_type;

/**
 * Determine the type of a given table (master or partitioned)
 *
 * @param[in] table the name of the table
 * @return table type
 */
sql_table_type sql_get_table_type(char *table);

#endif
