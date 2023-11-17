#include "mbed.h"

#include "EthernetInterface.h"

#include "gw_flash_iap.h"
#include "gw_https.h"
#include "gw_fwu_test.h"
#include "gw_fwu_debug.h"

// e8d930b6-6314-4f10-a099-6e893f3ce95b 256 buffer
// a7d2dcf2-f42e-4ff5-b153-d71761f02cfb Loop

// Debug case
#define FWU_READY_PREPARE_0             "{\"mgr\":{\"id\":123,\"verify\":0},\"supv\":{\"id\":1,\"to\":2000},\"src\":{\"id\":1,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":1,\"to_max\":1000,\"to_min\":500,\"level\":1}}"
// 256 size buffer binary
#define FWU_READY_PREPARE_1             "{\"mgr\":{\"id\":123,\"verify\":0},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":23,\"params\":{\"url\":\"https://dev.internal.smartdock.apiolink.com:8000\",\"key\":\"56b5d239e1e969c21e33849ac632e67e\",\"id\":\"e8d930b6-6314-4f10-a099-6e893f3ce95b\"},\"to_max\":3000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":20000,\"to_min\":500,\"level\":1}}"
// Simple loop binary
#define FWU_READY_PREPARE_2             "{\"mgr\":{\"id\":123,\"verify\":0},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":23,\"params\":{\"url\":\"https://dev.internal.smartdock.apiolink.com:8000\",\"key\":\"56b5d239e1e969c21e33849ac632e67e\",\"id\":\"a7d2dcf2-f42e-4ff5-b153-d71761f02cfb\"},\"to_max\":3000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":20000,\"to_min\":500,\"level\":1}}"

#define FWU_READY_PREPARE_3             "{\"mgr\":{\"id\":123,\"verify\":0},\"supv\":{\"id\":1,\"to\":2000},\"src\":{\"id\":1,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":1,\"to_max\":1000,\"to_min\":500,\"level\":1}}"
#define FWU_READY_PREPARE_4             "{\"mgr\":{\"id\":321,\"verify\":1},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":1000,\"to_min\":500,\"level\":0}}"
#define FWU_READY_PREPARE_5             "{\"mgr\":{\"id\":234,\"verify\":0},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":32,\"to_max\":1000,\"to_min\":500,\"level\":1}}"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
FileHandle *mbed::mbed_override_console(int fd)
{
    return &serial_port;
}

EthernetInterface m_Network;
int main()
{
    printf("Https & FlashIAP test!\r\n");

    GW_RestApi_params https_params = {
        .network = &m_Network
    };

    m_Network.connect();

    gw_fwu_init();
    gw_flash_iap_init();
    gw_https_init();
    gw_fwu_debug_init();
    m_RestApi->params_set(https_params);

    while(1)
    {
        char buf[256] = {0};
        if(serial_port.read(buf, sizeof(buf)))
        {
            printf("Task: %s \r\n", buf);
            if(buf[0] == '0')
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_0);
            else if(buf[0] == '1')
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_1);
            else if(buf[0] == '2')
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_2);
            else if(buf[0] == '3')
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_3);
            else
                m_FwuMgr->ready_checkout((char *)FWU_READY_PREPARE_4);
        }
    }
}
