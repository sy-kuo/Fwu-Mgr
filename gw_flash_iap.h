#include "mbed.h"
#include "gw_fwu.h"

#define FLASH_IAP_VERSION       0x2222
#define FLASH_IAP_MAX_LENGTH    0x40

class GW_FlashIAP: public FlashIAP, public GW_FwuMethod
{
public:
    GW_FlashIAP(uint32_t flash_start_address, uint32_t flash_size) {
        init();
        page_size = get_page_size();
        last_sector_size = get_sector_size(get_flash_start() + get_flash_size() - 1);
        start_address = get_flash_start() + flash_start_address;
        erase_size = flash_size;

        thd = NULL;
        p_flash = NULL;
    }
    ~GW_FlashIAP() {
        deinit();
        if(thd != NULL)
            delete thd;

        if(p_flash != NULL)
            free(p_flash);
    }

    virtual int checkout(void * params) override;
    virtual int prepare(void * params) override;
    virtual int paste(uint8_t * data, uint32_t length) override;
    virtual int finish(void) override;

    void params(void);
private:
    Thread * thd;
    char * class_id = (char *)"FlashIAP";
    GW_Role_Basic fwu_res;

    uint8_t * p_flash, task_id;
    uint32_t len;
    uint32_t last_sector_size, start_address, erase_size, page_size;
    void * p_prepare;
    int flash_clear(void);
    int flash_write(size_t offset, const unsigned char* buffer, size_t buffer_length);
    int flash_read(size_t offset, unsigned char * data, size_t data_length);
    void task_add(uint32_t id);
    void tasks_run(void);
};

void gw_flash_iap_init(void);