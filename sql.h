#ifndef __SQL_H
#define __SQL_H

/**
 * @file sql.h
 * @brief SQL parsing
 * 
 * The sql component should be exclusively used by the server component.
 */

#include "component.h"

DECLARE_COMPONENT(sql);

short sql_requires_mapping(char *sql);

#endif
