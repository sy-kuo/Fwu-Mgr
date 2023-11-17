#ifndef __GW_FWU_DEBUG_H__
#define __GW_FWU_DEBUG_H__

#include "mbed.h"
#include "gw_fwu_base.h"

#define SRC1_TEST_VERSION    0x1111
#define SRC2_TEST_VERSION    0x1111
#define DST1_TEST_VERSION    0x2222
#define DST2_TEST_VERSION    0x2222

class GW_Src_Debug: public GW_FwuMethod
{
public:
    GW_Src_Debug(const char * id): class_id((char *)id)
    {
        thd = NULL;
    }
    ~GW_Src_Debug()
    {
    }
    virtual int checkout(void * params) override;
    virtual int prepare(void * params) override;
    virtual int copy(uint32_t start_addr, uint32_t length) override;
    virtual int finish(void) override;
    void source_add(uint8_t * p_src, uint32_t size);
    char * class_id;
    void * p_params;
    uint8_t * p_bytes;
    uint8_t task_id;
    uint32_t file_addr, file_len, content_size;
    Thread * thd;
    GW_Role_Basic fwu_res;
    
    void task_add(uint32_t id);
    void tasks_run(void);
};

class GW_Dst_Debug: public GW_FwuMethod
{
public:
    GW_Dst_Debug(const char * id): class_id((char *)id)
    {
        thd = NULL;
        p_bytes = NULL;
    }
    ~GW_Dst_Debug()
    {
    }
    virtual int checkout(void * params) override;
    virtual int prepare(void * params) override;
    virtual int paste(uint8_t * data, uint32_t length) override;
    virtual int finish(void) override;
    char * class_id;
    void * p_params;
    uint8_t * p_bytes, task_id;
    Thread * thd;
    GW_Role_Basic fwu_res;

    void task_add(uint32_t id);
    void tasks_run(void);
};

class GW_Supv_Debug: public GW_FwuMethod
{
public:
    GW_Supv_Debug(const char * id): class_id((char *)id)
    {
        thd = NULL;
    }
    ~GW_Supv_Debug()
    {
    }
    virtual int report(void * params) override;
    char * class_id;
    void * p_params;
    uint8_t task_id;
    Thread * thd;
    GW_Role_Basic fwu_res;
    void task_add(uint32_t id);
    void tasks_run(void);
};

extern GW_Supv_Debug * supv_debug;
extern GW_Src_Debug * src_debug;
extern GW_Dst_Debug * dst_debug;

extern uint8_t src_debug_binary1[], src_debug_binary2[];

void gw_fwu_debug_init(void);

#endif