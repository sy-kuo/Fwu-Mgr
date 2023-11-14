#include "mbed.h"

#include "EthernetInterface.h"

#include "gw_flash_iap.h"
#include "gw_fwu_test.h"
#include "gw_test_button.h"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
FileHandle *mbed::mbed_override_console(int fd)
{
    return &serial_port;
}

EthernetInterface net;
int main()
{
    printf("Initialzie FlashIAP\r\n");

    SocketAddress a;
    net.connect();
    net.get_ip_address(&a);
    printf("IP address: %s\n", a.get_ip_address() ? a.get_ip_address() : "None");

    gw_fwu_init();
    gw_test_button_init();
    gw_flash_iap_init();
}
