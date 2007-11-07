#ifndef _DELEGATE_H
#define _DELEGATE_H

#include "action.h"
#include "command.h"
#include "reply.h"

void delegate_connect(void);
reply* delegate_action(action what, command with);
void delegate_disconnect(void);

#endif
