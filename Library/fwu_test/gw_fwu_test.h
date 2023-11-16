#ifndef __GW_FWU_TEST_H__
#define __GW_FWU_TEST_H__

#include "mbed.h"
#include "gw_fwu.h"

#define SRC1_TEST_VERSION    0x1111
#define SRC2_TEST_VERSION    0x1111
#define DST1_TEST_VERSION    0x2222
#define DST2_TEST_VERSION    0x2222

// e8d930b6-6314-4f10-a099-6e893f3ce95b 256 buffer
// a7d2dcf2-f42e-4ff5-b153-d71761f02cfb Loop

//#define FWU_READY_PREPARE_1             "{\"mgr\":{\"id\":123,\"verify\":0},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":20000,\"to_min\":500,\"level\":1}}"
#define FWU_READY_PREPARE_1             "{\"mgr\":{\"id\":123,\"verify\":0},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":23,\"params\":{\"url\":\"https://dev.internal.smartdock.apiolink.com:8000\",\"key\":\"56b5d239e1e969c21e33849ac632e67e\",\"id\":\"e8d930b6-6314-4f10-a099-6e893f3ce95b\"},\"to_max\":3000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":20000,\"to_min\":500,\"level\":1}}"
#define FWU_READY_PREPARE_2             "{\"mgr\":{\"id\":123,\"verify\":1},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":1000,\"to_min\":500,\"level\":1}}"
#define FWU_READY_PREPARE_3             "{\"mgr\":{\"id\":321,\"verify\":1},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":1000,\"to_min\":500,\"level\":0}}"
#define FWU_READY_PREPARE_4             "{\"mgr\":{\"id\":234,\"verify\":0},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":32,\"to_max\":1000,\"to_min\":500,\"level\":1}}"

extern uint8_t src1_binary[], src2_binary[];
extern GW_Role_Basic src1_info, src2_info, dst1_info, dst2_info, supv1_info, supv2_info;

extern uint8_t this_case;

int src_checkout_status(void);
int src_prepare_status(void);
int src_copy_status(void);
int src_finish_status(void);
int dst_checkout_status(void);
int dst_prepare_status(void);
int dst_paste_status(void);
int dst_finish_status(void);
int supv_report_status(void);

class GW_Src_1: public GW_FwuMethod
{
public:
    virtual int checkout(void * params) override
    {
        printf("<--- %s checkout!\r\n", class_id);
        // p_res->ver
        // p_res->size
        // p_res->length
        p_res->status = src_checkout_status();
        reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_res);
        return 0;
    }
    virtual int prepare(void * params) override
    {
        if(this_case != 10)
        {
            GW_Role_Basic * p_params = (GW_Role_Basic *)params;
            p_res->length = p_params->length;
            p_res->size = p_params->size;
            p_res->status = src_prepare_status();
            printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, p_res->length);
            reply(FW_UPDATE_EVENT_PREPARED, (void *)p_res);
        }
        return 0;
    }
    virtual int copy(uint32_t start_addr, uint32_t length) override
    {
        printf("<--- %s copy! Addr: %X, Len: %d \r\n", class_id, start_addr, length);
        p_res->pdata = p_binary + start_addr;
        p_res->status = src_copy_status();
        reply(FW_UPDATE_EVENT_COPY_DONE, (void *)p_res);
        return 0;
    }
    virtual int finish(void) override
    {
        p_res->status = src_finish_status();
        reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)p_res);
        return 0;
    }
    uint8_t * p_binary = src1_binary;
    char * class_id = (char *)"SRC1";
    GW_Role_Basic * p_res = &src1_info;
};

class GW_Src_2: public GW_FwuMethod
{
public:
    virtual int checkout(void * params) override
    {
        printf("<--- %s checkout!\r\n", class_id);
        // p_res->ver
        // p_res->size
        // p_res->length
        p_res->status = src_checkout_status();
        reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_res);
        return 0;
    }
    virtual int prepare(void * params) override
    {
        if(this_case != 10)
        {
            GW_Role_Basic * p_params = (GW_Role_Basic *)params;
            p_res->length = p_params->length;
            p_res->size = p_params->size;
            p_res->status = src_prepare_status();
            printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, p_res->length);
            reply(FW_UPDATE_EVENT_PREPARED, (void *)p_res);
        }
        return 0;
    }
    virtual int copy(uint32_t start_addr, uint32_t length) override
    {
        printf("<--- %s copy! Addr: %X, Len: %d \r\n", class_id, start_addr, length);
        p_res->pdata = p_binary + start_addr;
        p_res->status = src_copy_status();
        reply(FW_UPDATE_EVENT_COPY_DONE, (void *)p_res);
        return 0;
    }
    virtual int finish(void) override
    {
        p_res->status = src_finish_status();
        reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)p_res);
        return 0;
    }
    uint8_t * p_binary = src2_binary;
    char * class_id = (char *)"SRC2";
    GW_Role_Basic * p_res = &src2_info;
};

class GW_Dst_1: public GW_FwuMethod
{
public:
    virtual int checkout(void * params) override
    {
        if(this_case != 11)
        {
            printf("<--- %s checkout!\r\n", class_id);
            // p_res->ver
            // p_res->length
            p_res->status = dst_checkout_status();
            reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_res);
        }
        return 0;
    }
    virtual int prepare(void * params) override
    {
        GW_Role_Basic * p_params = (GW_Role_Basic *)params;
        p_res->start_addr = 0;
        p_res->length = p_params->length;
        p_res->size = p_params->size;
        p_res->status = dst_prepare_status();
        printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, p_res->length);
        reply(FW_UPDATE_EVENT_PREPARED, (void *)p_res);
        return 0;
    }
    virtual int paste(uint8_t * data, uint32_t length) override
    {
        printf("<--- %s paste! Addr: %X, Len: %d\r\n", class_id, p_res->start_addr, length);
        printHex(data, length);
        p_res->start_addr += p_res->length;
        p_res->status = dst_paste_status();
        reply(FW_UPDATE_EVENT_PASTE_DONE, (void *)p_res);
        return 0;
    }
    virtual int finish(void) override
    {
        p_res->status = dst_finish_status();
        reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)p_res);
        return 0;
    }
    char * class_id = (char *)"DST1";
    GW_Role_Basic * p_res = &dst1_info;
};

class GW_Dst_2: public GW_FwuMethod
{
public:
    virtual int checkout(void * params) override
    {
        if(this_case != 11)
        {
            printf("<--- %s checkout!\r\n", class_id);
            // p_res->ver
            // p_res->length
            p_res->status = dst_checkout_status();
            reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_res);
        }
        return 0;
    }
    virtual int prepare(void * params) override
    {
        GW_Role_Basic * p_params = (GW_Role_Basic *)params;
        p_res->start_addr = 0;
        p_res->length = p_params->length;
        p_res->size = p_params->size;
        p_res->status = dst_prepare_status();
        printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, p_res->length);
        reply(FW_UPDATE_EVENT_PREPARED, (void *)p_res);
        return 0;
    }
    virtual int paste(uint8_t * data, uint32_t length) override
    {
        printf("<--- %s paste! Addr: %X, Len: %d\r\n", class_id, p_res->start_addr, length);
        printHex(data, length);
        p_res->start_addr += p_res->length;
        p_res->status = dst_paste_status();
        reply(FW_UPDATE_EVENT_PASTE_DONE, (void *)p_res);
        return 0;
    }
    virtual int finish(void) override
    {
        p_res->status = dst_finish_status();
        reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)p_res);
        return 0;
    }
    char * class_id = (char *)"DST2";
    GW_Role_Basic * p_res = &dst2_info;
};

class GW_Supv_1: public GW_FwuMethod
{
public:
    virtual int report(void * params) override
    {
        GW_Role_Mgr * p_info = (GW_Role_Mgr *)params;
        printf("<--- %s report! Id: %d Evt: %s! Status code mgr: %d supv: %d src: %d dst: %d \r\n", class_id, p_info->id, evts[p_info->evt_id], p_info->res.mgr, p_info->res.supv, p_info->res.src, p_info->res.dst);
        p_res->status = supv_report_status();
        reply(FW_UPDATE_EVENT_REPORT_DONE, (void *)p_res);
        return 0;
    }
    char * class_id = (char *)"SUPV1";
    GW_Role_Basic * p_res = &supv1_info;
};

class GW_Supv_2: public GW_FwuMethod
{
public:
    virtual int report(void * params) override
    {
        GW_Role_Mgr * p_info = (GW_Role_Mgr *)params;
        printf("<--- %s report! Id: %d Evt: %s! Status code mgr: %d supv: %d src: %d dst: %d \r\n", class_id, p_info->id, evts[p_info->evt_id], p_info->res.mgr, p_info->res.supv, p_info->res.src, p_info->res.dst);
        p_res->status = supv_report_status();
        reply(FW_UPDATE_EVENT_REPORT_DONE, (void *)p_res);
        return 0;
    }
    char * class_id = (char *)"SUPV2";
    GW_Role_Basic * p_res = &supv2_info;
};

extern GW_Supv_1 supv_1;
extern GW_Src_1 src_1;
extern GW_Dst_1 dst_1;
extern GW_Supv_2 supv_2;
extern GW_Src_2 src_2;
extern GW_Dst_2 dst_2;

#endif