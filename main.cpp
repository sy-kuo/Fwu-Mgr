#include "mbed.h"
#include "gw_fwu_test.h"
#include <sstream>

#include "gw_flash_iap.h"
#include "gw_test_button.h"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
FileHandle *mbed::mbed_override_console(int fd)
{
    return &serial_port;
}

int main()
{
    printf("Initialzie FlashIAP\r\n");

    gw_fwu_init();
    gw_test_button_init();
    gw_flash_iap_init();
}
