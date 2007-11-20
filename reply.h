#ifndef _REPLY_H
#define _REPLY_H

typedef struct {
    char *bytes;
    int allocated;
    int size;
} reply;

typedef enum {
    REPLY_ERROR,
    REPLY_INCOMPLETE,
    REPLY_COMPLETE
} reply_status;

#endif
