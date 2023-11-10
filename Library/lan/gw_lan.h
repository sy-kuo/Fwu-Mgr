#ifndef __GW_LAN_H__
#define __GW_LAN_H__

#include "mbed.h"

class GW_Lan
{
    public:
        Thread * thd;
        NetworkInterface * network;
        void init(void);
        void connect(void);
        void status_callback(nsapi_event_t status, intptr_t param);
};

void gw_lan_init(void);

#endif
