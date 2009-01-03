#ifndef __DELEGATE_ID_H
#define __DELEGATE_ID_H

/**
 * @file delegate_id.h
 * @brief Defines the delegate_id type.
 */

/**
 * delegate_id type.
 */
typedef unsigned short delegate_id;

/**
 * delegate filter type.
 */
typedef short (*delegate_filter) (delegate_id);

#endif
