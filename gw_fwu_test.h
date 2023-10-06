#ifndef __GW_FWU_TEST_H__
#define __GW_FWU_TEST_H__

#include "mbed.h"
#include "gw_fwu.h"

#define SRC1_TEST_VERSION    0x1111
#define SRC2_TEST_VERSION    0x1111
#define DST1_TEST_VERSION    0x2222
#define DST2_TEST_VERSION    0x2222

#define FWU_READY_PREPARE_1             "{\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":1000,\"to_min\":500,\"level\":1}}"
#define FWU_READY_PREPARE_2             "{\"supv\":{\"id\":11,\"to\":2000},\"src\":{\"id\":21,\"to_max\":1000,\"to_min\":500},\"dst\":{\"id\":31,\"to_max\":1000,\"to_min\":500,\"level\":0}}"

extern uint32_t data_size;
extern uint8_t src1_binary[], src2_binary[];
extern fwu_parms_ctrl_s supv1_params, supv2_params;
extern fwu_params_s src1_params, src2_params;
extern fwu_params_s dst1_params, dst2_params;
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
    virtual int checkout(void) override
    {
        printf("<--- %s checkout!\r\n", class_id);
        p_class_params->status = src_checkout_status();
        reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_class_params);
        return 0;
    }
    virtual int prepare(void * params) override
    {
        if(this_case != 10)
        {
            fwu_params_s * p_params = (fwu_params_s *)params;
            p_class_params->start_addr = 0;
            p_class_params->length = p_params->length;
            p_class_params->size = p_params->size;
            p_class_params->status = src_prepare_status();
            printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, p_class_params->length);
            reply(FW_UPDATE_EVENT_PREPARED, (void *)p_class_params);
        }
        return 0;
    }
    virtual int copy(uint32_t start_addr, uint32_t length) override
    {
        printf("<--- %s copy! Addr: %X, Len: %d \r\n", class_id, start_addr, length);
        p_class_params->data = p_binary + start_addr;
        p_class_params->length = length;
        p_class_params->status = src_copy_status();
        reply(FW_UPDATE_EVENT_COPY_DONE, (void *)p_class_params);
        return 0;
    }
    virtual int finish(void) override
    {
        p_class_params->status = src_finish_status();
        reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)p_class_params);
        return 0;
    }
    uint8_t * p_binary = src2_binary;
    char * class_id = (char *)"SRC1";
    fwu_params_s * p_class_params = &src1_params;
};

class GW_Src_2: public GW_FwuMethod
{
public:
    virtual int checkout(void) override
    {
        printf("<--- %s checkout!\r\n", class_id);
        p_class_params->status = src_checkout_status();
        reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_class_params);
        return 0;
    }
    virtual int prepare(void * params) override
    {
        if(this_case != 10)
        {
            fwu_params_s * p_params = (fwu_params_s *)params;
            p_class_params->start_addr = 0;
            p_class_params->length = p_params->length;
            p_class_params->size = p_params->size;
            p_class_params->status = src_prepare_status();
            printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, p_class_params->length);
            reply(FW_UPDATE_EVENT_PREPARED, (void *)p_class_params);
        }
        return 0;
    }
    virtual int copy(uint32_t start_addr, uint32_t length) override
    {
        printf("<--- %s copy! Addr: %X, Len: %d \r\n", class_id, start_addr, length);
        p_class_params->data = p_binary + start_addr;
        p_class_params->length = length;
        p_class_params->status = src_copy_status();
        reply(FW_UPDATE_EVENT_COPY_DONE, (void *)p_class_params);
        return 0;
    }
    virtual int finish(void) override
    {
        p_class_params->status = src_finish_status();
        reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)p_class_params);
        return 0;
    }
    uint8_t * p_binary = src1_binary;
    char * class_id = (char *)"SRC2";
    fwu_params_s * p_class_params = &src2_params;
};

class GW_Dst_1: public GW_FwuMethod
{
public:
    virtual int checkout(void) override
    {
        printf("<--- %s checkout!\r\n", class_id);
        if(this_case != 11)
        {
            p_class_params->status = dst_checkout_status();
            reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_class_params);
        }
        return 0;
    }
    virtual int prepare(void * params) override
    {
        fwu_params_s * p_params = (fwu_params_s *)params;
        p_class_params->start_addr = 0;
        p_class_params->length = p_params->length;
        p_class_params->size = p_params->size;
        p_class_params->status = dst_prepare_status();
        printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, p_class_params->length);
        reply(FW_UPDATE_EVENT_PREPARED, (void *)p_class_params);
        return 0;
    }
    virtual int paste(uint8_t * data, uint32_t length) override
    {
        printf("<--- %s paste! Addr: %X, Len: %d\r\n", class_id, p_class_params->start_addr, length);
        printHex(data, length);
        p_class_params->start_addr += p_class_params->length;
        p_class_params->status = dst_paste_status();
        if(p_class_params->start_addr <= p_class_params->size)
            reply(FW_UPDATE_EVENT_PASTE_DONE, (void *)p_class_params);
        return 0;
    }
    virtual int finish(void) override
    {
        p_class_params->status = dst_finish_status();
        reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)p_class_params);
        return 0;
    }
    char * class_id = (char *)"DST1";
    fwu_params_s * p_class_params = &dst1_params;
};

class GW_Dst_2: public GW_FwuMethod
{
public:
    virtual int checkout(void) override
    {
        printf("<--- %s checkout!\r\n", class_id);
        if(this_case != 11)
        {
            p_class_params->status = dst_checkout_status();
            reply(FW_UPDATE_EVENT_CHECKOUTED, (void *)p_class_params);
        }
        return 0;
    }
    virtual int prepare(void * params) override
    {
        fwu_params_s * p_params = (fwu_params_s *)params;
        p_class_params->start_addr = 0;
        p_class_params->length = p_params->length;
        p_class_params->size = p_params->size;
        p_class_params->status = dst_prepare_status();
        printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, p_params->size, p_class_params->length);
        reply(FW_UPDATE_EVENT_PREPARED, (void *)p_class_params);
        return 0;
    }
    virtual int paste(uint8_t * data, uint32_t length) override
    {
        printf("<--- %s paste! Addr: %X, Len: %d\r\n", class_id, p_class_params->start_addr, length);
        printHex(data, length);
        p_class_params->start_addr += p_class_params->length;
        p_class_params->status = dst_paste_status();
        reply(FW_UPDATE_EVENT_PASTE_DONE, (void *)p_class_params);
        return 0;
    }
    virtual int finish(void) override
    {
        p_class_params->status = dst_finish_status();
        reply(FW_UPDATE_EVENT_FINISH_DONE, (void *)p_class_params);
        return 0;
    }
    char * class_id = (char *)"DST2";
    fwu_params_s * p_class_params = &dst2_params;
};

class GW_Supv_1: public GW_FwuMethod
{
public:
    virtual int report(void * params) override
    {
        fwu_parms_ctrl_s * p_supv = (fwu_parms_ctrl_s *)params;
        printf("<--- %s report! Evt: %s Mgr: %d Supv: %d Src: %d Dst: %d \r\n", class_id, evts[p_supv->evt_id], p_supv->mgr_status, p_supv->supv_status, p_supv->src_status, p_supv->dst_status);
        p_class_params->status = supv_report_status();
        reply(FW_UPDATE_EVENT_REPORT_DONE, (void *)p_class_params);
        return 0;
    }
    char * class_id = (char *)"SUPV1";
    fwu_parms_ctrl_s * p_class_params = &supv1_params;
};

class GW_Supv_2: public GW_FwuMethod
{
public:
    virtual int report(void * params) override
    {
        fwu_parms_ctrl_s * p_supv = (fwu_parms_ctrl_s *)params;
        printf("<--- %s report! Evt: %s Mgr: %d Supv: %d Src: %d Dst: %d \r\n", class_id, evts[p_supv->evt_id], p_supv->mgr_status, p_supv->supv_status, p_supv->src_status, p_supv->dst_status);
        p_class_params->status = supv_report_status();
        reply(FW_UPDATE_EVENT_REPORT_DONE, (void *)p_class_params);
        return 0;
    }
    char * class_id = (char *)"SUPV2";
    fwu_parms_ctrl_s * p_class_params = &supv2_params;
};

extern GW_Supv_1 supv_1;
extern GW_Src_1 src_1;
extern GW_Dst_1 dst_1;
extern GW_Supv_2 supv_2;
extern GW_Src_2 src_2;
extern GW_Dst_2 dst_2;

#endif