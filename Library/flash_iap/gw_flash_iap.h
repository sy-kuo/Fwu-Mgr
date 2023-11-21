#include "mbed.h"
#include "gw_fwu.h"

#define FLASH_IAP_VERSION       0x0000
#define FLASH_IAP_MAX_LENGTH    0x2000
#define CLASS_NAME              "FlashIAP"

class GW_FlashIAP: public FlashIAP, public GW_FwuMethod
{
public:
    GW_FlashIAP(uint32_t flash_start_address, uint32_t flash_size) {
        init();
        page_size = get_page_size();
        last_sector_size = get_sector_size(get_flash_start() + get_flash_size() - 1);
        start_address = get_flash_start() + flash_start_address;
        erase_size = flash_size;
    }
    ~GW_FlashIAP() {
        deinit();
    }

    virtual int checkout(void * params) override;
    virtual int prepare(void * params) override;
    virtual int paste(uint8_t * data, uint32_t length) override;
    virtual int finish(void) override;

    void params(void);
private:
    char * class_id = (char *)CLASS_NAME;
    uint32_t last_sector_size, start_address, erase_size, page_size;
    GW_Role_Basic fwu_res;

    int flash_clear(void);
    int flash_write(size_t offset, const unsigned char* buffer, size_t buffer_length);
    int flash_read(size_t offset, unsigned char * data, size_t data_length);
};

void gw_flash_iap_init(void);