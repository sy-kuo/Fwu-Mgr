#ifndef __GW_FWU_TEST_H__
#define __GW_FWU_TEST_H__

#include "mbed.h"
#include "gw_fwu.h"

#define SRC1_TEST_VERSION    0x1111
#define SRC2_TEST_VERSION    0x1111
#define DST1_TEST_VERSION    0x2222
#define DST2_TEST_VERSION    0x2222

#define FWU_READY_PREPARE_1             "{\"mgr\":{\"id\":123},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":1000,\"to_min\":500,\"level\":1}}"
#define FWU_READY_PREPARE_2             "{\"mgr\":{\"id\":321},\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":1000,\"to_min\":500,\"level\":0}}"

extern uint32_t data_size;
extern uint8_t src1_binary[], src2_binary[];
extern fwu_params_s src1_info, src2_info, dst1_info, dst2_info, supv1_info, supv2_info;

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

void printHex(uint8_t * data, uint32_t len);

class GW_Src_1: public GW_FwuMethod
{
public:
    virtual int checkout(void * params) override
    {
        printf("<--- %s checkout!\r\n", class_id);
        p_res->status = src_checkout_status();
        reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_res);
        return 0;
    }
    virtual int prepare(void * params) override
    {
        if(this_case != 10)
        {
            fwu_params_s * p_params = (fwu_params_s *)params;
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
    fwu_params_s * p_res = &src1_info;
};

class GW_Src_2: public GW_FwuMethod
{
public:
    virtual int checkout(void * params) override
    {
        printf("<--- %s checkout!\r\n", class_id);
        p_res->status = src_checkout_status();
        reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_res);
        return 0;
    }
    virtual int prepare(void * params) override
    {
        if(this_case != 10)
        {
            fwu_params_s * p_params = (fwu_params_s *)params;
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
    fwu_params_s * p_res = &src2_info;
};

class GW_Dst_1: public GW_FwuMethod
{
public:
    virtual int checkout(void * params) override
    {
        if(this_case != 11)
        {
            printf("<--- %s checkout!\r\n", class_id);
            p_res->status = dst_checkout_status();
            reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_res);
        }
        return 0;
    }
    virtual int prepare(void * params) override
    {
        fwu_params_s * p_params = (fwu_params_s *)params;
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
    fwu_params_s * p_res = &dst1_info;
};

class GW_Dst_2: public GW_FwuMethod
{
public:
    virtual int checkout(void * params) override
    {
        if(this_case != 11)
        {
            printf("<--- %s checkout!\r\n", class_id);
            p_res->status = dst_checkout_status();
            reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_res);
        }
        return 0;
    }
    virtual int prepare(void * params) override
    {
        fwu_params_s * p_params = (fwu_params_s *)params;
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
    fwu_params_s * p_res = &dst2_info;
};

class GW_Supv_1: public GW_FwuMethod
{
public:
    virtual int report(void * params) override
    {
        memcpy(p_res, (fwu_params_s *)params, sizeof(fwu_params_s));
        printf("<--- %s report! Id: %d Evt: %s! Mgr: %d Supv: %d Src: %d Dst: %d \r\n", class_id, p_res->id, evts[p_res->evt_id], p_res->statuses.mgr, p_res->statuses.supv, p_res->statuses.src, p_res->statuses.dst);
        p_res->status = supv_report_status();
        reply(FW_UPDATE_EVENT_REPORT_DONE, (void *)p_res);
        return 0;
    }
    char * class_id = (char *)"SUPV1";
    fwu_params_s * p_res = &supv1_info;
};

class GW_Supv_2: public GW_FwuMethod
{
public:
    virtual int report(void * params) override
    {
        memcpy(p_res, (fwu_params_s *)params, sizeof(fwu_params_s));
        printf("<--- %s report! Id: %d Evt: %s! Mgr: %d Supv: %d Src: %d Dst: %d \r\n", class_id, p_res->id, evts[p_res->evt_id], p_res->statuses.mgr, p_res->statuses.supv, p_res->statuses.src, p_res->statuses.dst);
        p_res->status = supv_report_status();
        reply(FW_UPDATE_EVENT_REPORT_DONE, (void *)p_res);
        return 0;
    }
    char * class_id = (char *)"SUPV2";
    fwu_params_s * p_res = &supv2_info;
};

extern GW_Supv_1 supv_1;
extern GW_Src_1 src_1;
extern GW_Dst_1 dst_1;
extern GW_Supv_2 supv_2;
extern GW_Src_2 src_2;
extern GW_Dst_2 dst_2;

#endif