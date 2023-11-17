#ifndef __GW_FWU_H__
#define __GW_FWU_H__

#include <vector>
#include "mbed.h"

#define YES  1
#define NO   0

void printHex(uint8_t * data, uint32_t len);

typedef enum {
    FW_ROLE_ID_DEBUG_SUPV = 0x0001,
    FW_ROLE_ID_DEBUG_SRC = 0x0001,
    FW_ROLE_ID_DEBUG_DST = 0x0001,
} FW_ROLE_ID_E;

typedef enum {
    FW_UPDATE_ERROR_CODE_SUCESS = 0,
    FW_UPDATE_ERROR_CODE_FIRMWARE_UPDATE_TO_DATE,
    FW_UPDATE_ERROR_CODE_FIRMWARE_VERIFY_FAIL,
    FW_UPDATE_ERROR_CODE_TIMEOUT,
    FW_UPDATE_ERROR_CODE_NO_MEMORY,
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

class GW_Roles_Code
{
public:
    int32_t  mgr;
    int32_t  supv;
    int32_t  src;
    int32_t  dst;
};

class GW_Role_Basic
{
public:
    uint32_t ver;
    uint32_t size;
    uint32_t start_addr;
    uint32_t length;
    void * pdata;
    uint32_t timeout_max;
    uint32_t timeout_min;
    uint8_t level;
    int32_t  status;
};

class GW_Role_Mgr: public GW_Role_Basic
{
public:
    uint32_t id;
    uint32_t evt_id;
    uint32_t verify;
    uint32_t timeout;
    GW_Roles_Code res;
    GW_Roles_Code ack;
};

class GW_Params
{
public:
    GW_Role_Basic src;
    GW_Role_Basic dst;
    GW_Role_Mgr mgr;
};

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
    GW_Role_Basic tmp;

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
    GW_FwUpdate(bool _msg_who = false)
    {
        thd.start(callback(&evt_queue, &EventQueue::dispatch_forever));
        msg_who = _msg_who;
    }

    void ready_checkout(char * data);

    void evt_push(int who, int event, void * data);

    void supv_register(GW_FwuMethod * role, int id);

    void src_register(GW_FwuMethod * role, int id);

    void dst_register(GW_FwuMethod * role, int id);

    void test(void);
private:
    bool msg_who;
    GW_FwuMethod * supv;
    GW_FwuMethod * src;
    GW_FwuMethod * dst;

    GW_Params infos;

    std::vector<GW_FwuMethod *> supv_list;
    std::vector<GW_FwuMethod *> src_list;
    std::vector<GW_FwuMethod *> dst_list;

    void checkout_parse(char * data, uint8_t & supv_index, uint8_t & src_index, uint8_t & dst_index);

    void role_list_set(uint8_t supv_index, uint8_t src_index, uint8_t dst_index);

    void verify(void);

    void role_register(GW_FwuMethod * role, int id, FW_UPDATE_ROLE_E who);

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

int parsed_prepare(char ** parsed, char * p_prepare, char * key[], int index);

void gw_fwu_init();

extern const char * whos[], * evts[];
extern GW_FwUpdate * m_FwuMgr;

#endif