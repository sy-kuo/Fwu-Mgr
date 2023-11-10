#include "mbed.h"
#include "mbed_trace.h"

#include "gw_lan.h"

#if MBED_CONF_MBED_TRACE_ENABLE
#define TRACE_GROUP     "LAN"
#endif

GW_Lan m_Lan;

void GW_Lan::status_callback(nsapi_event_t status, intptr_t param)
{
    SocketAddress sockaddr;
    auto rc = network->get_ip_address(&sockaddr);

    switch (param) {
        case NSAPI_STATUS_LOCAL_UP:
            break;
        case NSAPI_STATUS_GLOBAL_UP:
            if(thd != NULL)
            {
                delete thd;
                thd = NULL;
            }

            /*
            if (rc == NSAPI_ERROR_OK)
                app_evt_push(APP_EVENT_IP_ADDRESS_NEW, network);
            */
            break;
        case NSAPI_STATUS_DISCONNECTED:
            break;
        case NSAPI_STATUS_CONNECTING:
            /*
            if(thd != NULL)
                app_evt_push(APP_EVENT_NETWORK_CONNECT, network);
            else if (rc != NSAPI_ERROR_OK)
                app_evt_push(APP_EVENT_IP_ADDRESS_LOST, &rc);
            */
            break;
        default:
            break;
    }
}

void GW_Lan::connect(void)
{
    tr_info("Connecting to the network");
    network = NetworkInterface::get_default_instance();
    if (network == NULL) {
        tr_err("No Network interface found.");
    }

    network->set_blocking(false);
    network->add_event_listener(callback(&m_Lan, &GW_Lan::status_callback));

    auto rc = network->connect();
    if (rc != 0) {
        tr_err("Connection error: %d", rc);
    }
}

void GW_Lan::init(void)
{
    thd = new Thread;
    thd->start(callback(&m_Lan, &GW_Lan::connect));
}

void gw_lan_init(void)
{
    m_Lan.init();
}
