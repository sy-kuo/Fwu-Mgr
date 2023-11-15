#include "mbed.h"

#include "EthernetInterface.h"

#include "gw_flash_iap.h"
#include "gw_https.h"
#include "gw_fwu_test.h"
#include "gw_test_button.h"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
FileHandle *mbed::mbed_override_console(int fd)
{
    return &serial_port;
}

EthernetInterface m_Network;
int main()
{
    printf("Initialzie FlashIAP\r\n");

    GW_RestApi_params https_params = {
        .network = &m_Network
    };
/*
    GW_RestApi_params https_params = {
        .network = &m_Network, \
        .server_url = (char *)"https://dev.internal.smartdock.apiolink.com:8000", \
        .device_key = (char *)"56b5d239e1e969c21e33849ac632e67e" \
    };
*/
    m_Network.connect();

    gw_fwu_init();
    gw_test_button_init();
    //gw_flash_iap_init();
    gw_https_init();
    m_RestApi->params_set(https_params);

    while(1)
    {
        ThisThread::sleep_for(1000ms);
    }
}
