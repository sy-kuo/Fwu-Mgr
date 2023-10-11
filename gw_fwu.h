#ifndef __GW_FWU_H__
#define __GW_FWU_H__

#include <vector>
#include "mbed.h"

#define YES  1
#define NO   0

typedef enum {
    FW_UPDATE_ERROR_CODE_SUCESS = 0,
    FW_UPDATE_ERROR_CODE_FIRMWARE_UPDATE_TO_DATE,
    FW_UPDATE_ERROR_CODE_TIMEOUT,
    FW_UPDATE_ERROR_CODE_NULL = 0xFFFFFFFF,
} FW_UPDATE_ERROR_CODE_E;

typedef enum {
    FW_UPDATE_ROLE_NONE,
    FW_UPDATE_ROLE_MANAGER,
    FW_UPDATE_ROLE_SUPERVISOR,
    FW_UPDATE_ROLE_SOURCE,
    FW_UPDATE_ROLE_DESTINATION,
    FW_UPDATE_ROLE_COUNT
} FW_UPDATE_ROLE_E;

typedef enum {
    FW_UPDATE_EVENT_IDLE,
    FW_UPDATE_EVENT_READY_CHECKOUT,
    FW_UPDATE_EVENT_CHECKOUTED,
    FW_UPDATE_EVENT_VERIFY_DONE,
    FW_UPDATE_EVENT_PREPARED,
    FW_UPDATE_EVENT_COPY_DONE,
    FW_UPDATE_EVENT_PASTE_DONE,
    FW_UPDATE_EVENT_FINISH_DONE,
    FW_UPDATE_EVENT_REPORT_DONE,
    FW_UPDATE_EVENT_COUNT,
    FW_UPDATE_EVENT_NEXT_TEST
} FW_UPDATE_EVENT_E;

typedef enum {
    FW_UPDATE_VERSION_LEVEL_NORMAL,
    FW_UPDATE_VERSION_LEVEL_FORCE
} FW_UPDATE_VERSION_LEVEL_E;

typedef struct
{
    int32_t  mgr;
    int32_t  supv;
    int32_t  src;
    int32_t  dst;
} fwu_roles_s;

typedef struct
{
    fwu_roles_s ack;
    fwu_roles_s res;
} fwu_statuses_s;

typedef struct
{
    uint32_t ver;
    uint32_t length;
    uint32_t size;
    void * pdata;
    int32_t  status;
} fwu_ready_s;

typedef struct
{
    uint32_t ver;
    uint32_t length;
    uint32_t size;
    void * pdata;
    int32_t  status;
} fwu_checkout_s;

typedef struct
{
    int32_t  status;
} fwu_verify_s;

typedef struct
{
    uint32_t ver;
    uint32_t size;
    uint32_t start_addr;
    uint32_t length;
    uint8_t level;
    uint32_t timeout_max;
    uint32_t timeout_min;
    void * pdata;
    int32_t  status;
} fwu_prepare_s;

typedef struct
{
    uint32_t length;
    uint8_t * pdata;
    int32_t  status;
} fwu_copy_s;

typedef struct
{
    uint32_t start_addr;
    uint32_t length;
    uint32_t size;
    int32_t  status;
} fwu_paste_s;

typedef struct
{
    int32_t  status;
} fwu_finish_s;

typedef struct
{
    uint32_t id;
    uint32_t evt_id;
    fwu_roles_s statuses;
    int32_t  status;
} fwu_report_s;

typedef struct
{
    uint32_t id;
    void * pdata;
    int32_t  status;
} fwu_parms_mgr_s;

typedef struct
{
    uint32_t timeout;
    void * pdata;
    int32_t  status;
} fwu_parms_supv_s;

typedef struct
{
    uint32_t ver;
    uint32_t size;
    uint32_t start_addr;
    uint32_t length;
    uint8_t level;
    uint32_t timeout_max;
    uint32_t timeout_min;
    void * pdata;
    int32_t  status;
} fwu_params_s;

class GW_FwuMethod
{
public:
    virtual int checkout(void * params) {return 0;};
    virtual int prepare(void * params) {return 0;};
    virtual int copy(uint32_t start_addr, uint32_t length) {return 0;};
    virtual int paste(uint8_t * data, uint32_t length) {return 0;};
    virtual int finish(void) {return 0;};
    virtual int report(void * params) {return 0;};

    int reply(int event, void * data)
    {
        //if(tmr.remaining_time())
        timeout_inactivate();
        f_reply(who, event, data);
        return 0;
    }

    uint16_t module_id;
    friend class GW_FwUpdate;
private:
    FW_UPDATE_ROLE_E who;
    FW_UPDATE_EVENT_E evt;
    std::function<void(int who, int event, void * data)> f_reply;
    Timeout tmr;
    fwu_params_s tmp;

    int checkout(void * params, uint32_t timeout_ms)
    {
        evt = FW_UPDATE_EVENT_CHECKOUTED;
        timeout_activate(timeout_ms);
        checkout(params);
        return 0;
    }
    int prepare(void * params, uint32_t timeout_ms)
    {
        evt = FW_UPDATE_EVENT_PREPARED;
        timeout_activate(timeout_ms);
        prepare(params);
        return 0;
    }
    int copy(uint32_t start_addr, uint32_t length, uint32_t timeout_ms)
    {
        evt = FW_UPDATE_EVENT_COPY_DONE;
        timeout_activate(timeout_ms);
        copy(start_addr, length);
        return 0;
    }
    int paste(uint8_t * data, uint32_t length, uint32_t timeout_ms)
    {
        evt = FW_UPDATE_EVENT_PASTE_DONE;
        timeout_activate(timeout_ms);
        paste(data, length);
        return 0;
    }
    int finish(uint32_t timeout_ms)
    {
        evt = FW_UPDATE_EVENT_FINISH_DONE;
        timeout_activate(timeout_ms);
        finish();
        return 0;
    }
    int report(void * params, uint32_t timeout_ms)
    {
        evt = FW_UPDATE_EVENT_REPORT_DONE;
        timeout_activate(timeout_ms);
        report(params);
        return 0;
    }
    void timeout_activate(uint32_t timeout_ms)
    {
        tmr.attach(callback(this, &GW_FwuMethod::timeout), std::chrono::milliseconds(timeout_ms));
    }
    void timeout_inactivate(void)
    {
        tmr.detach();
    }
    void timeout(void)
    {
        tmp.status = FW_UPDATE_ERROR_CODE_TIMEOUT;
        f_reply(who, evt, &tmp);
    }
};

class GW_FwUpdate
{
public:
    GW_FwUpdate()
    {
        thd.start(callback(&evt_queue, &EventQueue::dispatch_forever));
    }

    void ready_checkout(char * data);

    void evt_push(int who, int event, void * data);

    template <class Role>
    void supv_register(Role * role, int id);

    template <class Role>
    void src_register(Role * role, int id);

    template <class Role>
    void dst_register(Role * role, int id);

private:
    GW_FwuMethod * supv;
    GW_FwuMethod * src;
    GW_FwuMethod * dst;

    fwu_statuses_s statuses;

    fwu_ready_s ready_s;
    fwu_checkout_s checkout_s;
    fwu_verify_s verify_s;
    fwu_prepare_s prepare_s;
    fwu_copy_s copy_s;
    fwu_paste_s paste_s;
    fwu_report_s report_s;

    fwu_parms_mgr_s mgr_params;
    fwu_parms_supv_s supv_params;
    fwu_params_s src_params;
    fwu_params_s dst_params;

    std::vector<GW_FwuMethod *> supv_list;
    std::vector<GW_FwuMethod *> src_list;
    std::vector<GW_FwuMethod *> dst_list;

    void checkout_parse(char * data, uint8_t & supv_index, uint8_t & src_index, uint8_t & dst_index);

    void role_list_set(uint8_t supv_index, uint8_t src_index, uint8_t dst_index);

    void verify(void);

    template <class Role>
    void role_register(Role * role, int id, FW_UPDATE_ROLE_E who);

    template <class Role>
    void who_is(Role * role, FW_UPDATE_ROLE_E who);

    template <class Role>
    void reply_set(Role * who);

    Thread thd;
    EventQueue evt_queue;

    Event<void(int, int, void *)> evt = evt_queue.event(this, &GW_FwUpdate::evt_cb);
    void timeout_cb(void);
    void evt_cb(int who, int event, void * data);
};

void gw_fwu_init();

extern const char * whos[], * evts[];
extern GW_FwUpdate * m_FwuMgr;

#endif