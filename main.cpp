#include "mbed.h"
#include "gw_fwu_test.h"
#include <sstream>

#include "gw_test_button.h"

static BufferedSerial serial_port(USBTX, USBRX, 115200);
FileHandle *mbed::mbed_override_console(int fd)
{
    return &serial_port;
}

int main()
{
    gw_fwu_init();
    gw_test_button_init();
}
/*
#include <functional>
#include <iostream>
#include <string>

class Work {
public:
    std::function<void(char * msg)> logger;
    void do_sth()
    {
        logger("on log");
    }
};

class P {
public:
    void log(char * msg)
    {
        printf("Log: %s \r\n", msg);
    }
};

int main()
{
    printf("Main \r\n");
    Work w;
    P p;
    w.logger = std::bind(&P::log, p, std::placeholders::_1);
    w.do_sth();
}*/