
/* system includes */
#include <stdlib.h>

/* project includes */
#include "delegate_filter.h"

delegate_filter_result delegate_filter_reduce(delegate_filter * filters,
                                              delegate_id id)
{
    delegate_filter_result r = DELEGATE_FILTER_USE;
    if (filters != NULL) {
        int n = 0;
        while (filters[n] != NULL) {
            if (filters[n] (id)) {
                r = DELEGATE_FILTER_DONT_USE;
                break;
            }
            ++n;
        }
    }
    return r;
}
