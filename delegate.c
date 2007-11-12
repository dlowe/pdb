/* project includes */
#include "action.h"
#include "command.h"
#include "delegate.h"
#include "reply.h"

void delegate_connect(void)
{
}

/**
 * @return a list of replies gathered from delegate servers.
 */
static reply *gather_replies(void) {
    
}

reply *delegate_action(action what, command with)
{
    switch (what) {
    case ACTION_NONE:
        return 0;
    case ACTION_NOOP_ALL:
        return gather_replies();
    };
    return 0;
}

void delegate_disconnect(void)
{
}
