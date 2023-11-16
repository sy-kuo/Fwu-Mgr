#include "mbed.h"

#include "EthernetInterface.h"

#include "gw_flash_iap.h"
#include "gw_https.h"
#include "gw_fwu_test.h"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
FileHandle *mbed::mbed_override_console(int fd)
{
    return &serial_port;
}

EthernetInterface m_Network;
int main()
{
    printf("Https & FlashIAP test 1\r\n");

    GW_RestApi_params https_params = {
        .network = &m_Network
    };

    m_Network.connect();

    gw_fwu_init();
    gw_flash_iap_init();
    gw_https_init();
    m_RestApi->params_set(https_params);

    while(1)
    {
        char buf[256] = {0};
        if(serial_port.read(buf, sizeof(buf)))
        {
            printf("Task: %s \r\n", buf);
            if(buf[0] == '1')
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_2);
            else if(buf[0] == '2')
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_3);
            else if(buf[0] == '3')
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_4);
            else
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_1);
        }
    }
}
