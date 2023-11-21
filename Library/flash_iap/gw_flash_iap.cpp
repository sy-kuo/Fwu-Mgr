#include "gw_flash_iap.h"

GW_FlashIAP * m_FlashIAP;

int GW_FlashIAP::checkout(void * params)
{
    printf("<--- %s checkout!\r\n", class_id);
    fwu_res.ver = FLASH_IAP_VERSION;
    fwu_res.length = FLASH_IAP_MAX_LENGTH;
    fwu_res.status = FW_UPDATE_ERROR_CODE_SUCESS;
    reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)&fwu_res);
    return 0;
}

int GW_FlashIAP::prepare(void * params)
{
    int res = flash_clear();
    fwu_res.status = res? res: FW_UPDATE_ERROR_CODE_SUCESS;
    if(fwu_res.status == FW_UPDATE_ERROR_CODE_SUCESS)
    {
        GW_Role_Basic * p_params = (GW_Role_Basic *)params;

        fwu_res.start_addr = 0;
        fwu_res.length = p_params->length;
        fwu_res.size = p_params->size;
        printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, fwu_res.length);
    }
    reply(FW_UPDATE_EVENT_PREPARED, (void *)&fwu_res);
    return 0;
}

int GW_FlashIAP::paste(uint8_t * data, uint32_t length)
{
    int res = flash_write(fwu_res.start_addr, data, length);
    fwu_res.status = res? res: FW_UPDATE_ERROR_CODE_SUCESS;
    if(fwu_res.status == FW_UPDATE_ERROR_CODE_SUCESS)
    {
        fwu_res.start_addr += length;
        //printHex(data, fwu_res.length);
        //printf("<--- %s paste! Addr: %X, Len: %d\r\n", class_id, fwu_res.start_addr - fwu_res.length, fwu_res.length);
    }
    reply(FW_UPDATE_EVENT_PASTE_DONE, (void *)&fwu_res);
    return 0;
}

int GW_FlashIAP::finish(void)
{
    fwu_res.status = FW_UPDATE_ERROR_CODE_SUCESS;
    reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)&fwu_res);
    return 0;
}

int GW_FlashIAP::flash_clear(void)
{
    return erase(start_address, erase_size);
}

int GW_FlashIAP::flash_write(size_t offset, const unsigned char * buffer, size_t buffer_length)
{
    int result = 0;
    uint32_t address = start_address + offset;

    size_t index = 0;
    while (index < buffer_length)
    {
        result = program(buffer + index, address + index, page_size);
        if (result != 0)
            break;
        index += page_size;
    }
    return result;
}

int GW_FlashIAP::flash_read(size_t offset, unsigned char * data, size_t data_length)
{
    int result = 0;
    size_t address = start_address + offset;

    size_t index = 0;
    while (index < data_length)
    {
        uint32_t read_length = (data_length - index) > page_size? page_size: data_length - index;

        result = read(data + index, address + index, read_length);
        if (result != 0)
            break;
        index += read_length;
    }
    return result;
}

void GW_FlashIAP::params(void)
{
    uint8_t buffer[256];

    memset(buffer, 0xFF, sizeof(buffer));
    flash_read(0, buffer, sizeof(buffer));
    printHex(buffer, sizeof(buffer));

    printf("start address: %X \r\n", get_flash_start());
    printf("flash size: %X \r\n", get_flash_size());
    printf("page size: %X \r\n", page_size);
    printf("last sector size: %X \r\n", last_sector_size);
    printf("Erase flash: %X %X \r\n", start_address, erase_size);
}

void gw_flash_iap_init(void)
{
    m_FlashIAP = new GW_FlashIAP(MBED_CONF_APP_ESTIMATED_APPLICATION_SIZE, MBED_CONF_APP_ESTIMATED_APPLICATION_SIZE);
    m_FlashIAP->params();

    m_FwuMgr->dst_register(m_FlashIAP, 33);
}