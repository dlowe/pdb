#ifndef __SQL_H
#define __SQL_H

/**
 * @file sql.h
 * @brief SQL parsing
 * 
 * The sql component should be exclusively used by the server component.
 */

#include "component.h"
#include "delegate_id.h"

DECLARE_COMPONENT(sql);

typedef enum {
    SQL_TABLE_TYPE_MASTER,
    SQL_TABLE_TYPE_PARTITIONED
} sql_table_type;

sql_table_type sql_get_table_type(char *table);

#endif
