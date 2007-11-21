/* project includes */
#include "packet.h"

packet packet_null(void)
{
    packet null;
    null.bytes = 0;
    null.allocated = 0;
    null.size = 0;
    return null;
}
