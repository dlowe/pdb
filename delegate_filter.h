#ifndef __DELEGATE_ID_H
#define __DELEGATE_ID_H

/**
 * @file delegate_filter.h
 * @brief Types and utility related to filtering delegates.
 *
 * Several components (delegate, db_driver, sql, map) need to specify which
 * of the delegates should participate in a given conversation. This mechanism
 * allows that filtering without having the delegate component know directly
 * about each of the filter-generating components.
 */

/**
 * delegate_id type.
 */
typedef unsigned short delegate_id;

/**
 * Possible results from a delegate_filter.
 */
typedef enum {
    DELEGATE_FILTER_USE,
    DELEGATE_FILTER_DONT_USE
} delegate_filter_result;

/**
 * delegate filter type.
 */
typedef delegate_filter_result(*delegate_filter) (delegate_id);

/**
 * Given a set of filters and a delegate_id, determines whether the delegate
 * is expected to be active.
 *
 * @param[in] filters list of filters to apply
 * @param[in] id delegate identifier
 * @return should we use this delegate?
 */
delegate_filter_result delegate_filter_reduce(delegate_filter * filters,
                                              delegate_id id);

#endif
