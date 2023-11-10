#include "gw_flash_iap.h"

GW_FlashIAP * m_FlashIAP;

void GW_FlashIAP::task_add(uint32_t id)
{
    task_id = id;

    if(thd != NULL)
    {
        delete thd;
        thd = NULL;
    }
    thd = new Thread;
    thd->start(callback(this, &GW_FlashIAP::tasks_run));
}

void GW_FlashIAP::tasks_run(void)
{
    if(task_id == FW_UPDATE_EVENT_CHECKOUTED)
    {
        fwu_res.ver = FLASH_IAP_VERSION;
        fwu_res.length = FLASH_IAP_MAX_LENGTH;
        fwu_res.status = FW_UPDATE_ERROR_CODE_SUCESS;
        printf("<--- %s checkout!\r\n", class_id);
    }
    else if(task_id == FW_UPDATE_EVENT_PREPARED)
    {
        int res = flash_clear();
        fwu_res.status = res? res: FW_UPDATE_ERROR_CODE_SUCESS;
        if(fwu_res.status == FW_UPDATE_ERROR_CODE_SUCESS)
        {
            GW_Role_Basic * p_params = (GW_Role_Basic *)p_prepare;

            if(p_flash != NULL)
            {
                free(p_flash);
                p_flash = NULL;
            }
            p_flash = (uint8_t *)calloc(p_params->length, 1);

            if(p_flash != NULL)
            {
                fwu_res.start_addr = 0;
                fwu_res.length = p_params->length;
                fwu_res.size = p_params->size;
                printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, fwu_res.length);
            }
            else
                fwu_res.status = FW_UPDATE_ERROR_CODE_NO_MEMORY;
        }
    }
    else if(task_id == FW_UPDATE_EVENT_PASTE_DONE)
    {
        int res = flash_write(fwu_res.start_addr, p_flash, fwu_res.length);
        fwu_res.status = res? res: FW_UPDATE_ERROR_CODE_SUCESS;
        if(fwu_res.status == FW_UPDATE_ERROR_CODE_SUCESS)
        {
            fwu_res.start_addr += fwu_res.length;
            printHex(p_flash, fwu_res.length);
            printf("<--- %s paste! Addr: %X, Len: %d\r\n", class_id, fwu_res.start_addr - fwu_res.length, fwu_res.length);
        }
    }
    else if(task_id == FW_UPDATE_EVENT_FINISH_DONE)
    {
        if(p_flash != NULL)
        {
            free(p_flash);
            p_flash = NULL;
        }

        fwu_res.status = FW_UPDATE_ERROR_CODE_SUCESS;
    }
    reply(task_id, (void *)&fwu_res);
}

int GW_FlashIAP::checkout(void * params)
{
    task_add(FW_UPDATE_EVENT_CHECKOUTED);
    return 0;
}

int GW_FlashIAP::prepare(void * params)
{
    p_prepare = params;
    task_add(FW_UPDATE_EVENT_PREPARED);
    return 0;
}

int GW_FlashIAP::paste(uint8_t * data, uint32_t length)
{
    memset(p_flash, 0, length);
    memcpy(p_flash, data, length);
    task_add(FW_UPDATE_EVENT_PASTE_DONE);
    return 0;
}

int GW_FlashIAP::finish(void)
{
    task_add(FW_UPDATE_EVENT_FINISH_DONE);
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

    m_FwuMgr->dst_register(m_FlashIAP, 31);
}