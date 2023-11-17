#ifndef __GW_FWU_BASE_H__
#define __GW_FWU_BASE_H__

#include "mbed.h"

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

class GW_FwuMethod
{
public:
    virtual int checkout(void * params) {return 0;};
    virtual int prepare(void * params) {return 0;};
    virtual int copy(uint32_t start_addr, uint32_t length) {return 0;};
    virtual int paste(uint8_t * data, uint32_t length) {return 0;};
    virtual int finish(void) {return 0;};
    virtual int report(void * params) {return 0;};

    int reply(int event, void * data);

    uint16_t module_id;
    friend class GW_FwUpdate;
private:
    FW_UPDATE_ROLE_E who;
    FW_UPDATE_EVENT_E evt;
    std::function<void(int who, int event, void * data)> f_reply;
    Timeout tmr;
    GW_Role_Basic tmp;

    int checkout(void * params, uint32_t timeout_ms);
    int prepare(void * params, uint32_t timeout_ms);
    int copy(uint32_t start_addr, uint32_t length, uint32_t timeout_ms);
    int paste(uint8_t * data, uint32_t length, uint32_t timeout_ms);
    int finish(uint32_t timeout_ms);
    int report(void * params, uint32_t timeout_ms);
    void timeout_activate(uint32_t timeout_ms);
    void timeout_inactivate(void);
    void timeout(void);
};

#endif