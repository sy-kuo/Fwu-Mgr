#include "gw_fwu_base.h"


int GW_FwuMethod::reply(int event, void * data)
{
    //if(tmr.remaining_time())
    timeout_inactivate();
    f_reply(who, event, data);
    return 0;
}

int GW_FwuMethod::checkout(void * params, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_CHECKOUTED;
    timeout_activate(timeout_ms);
    checkout(params);
    return 0;
}

int GW_FwuMethod::prepare(void * params, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_PREPARED;
    timeout_activate(timeout_ms);
    prepare(params);
    return 0;
}

int GW_FwuMethod::copy(uint32_t start_addr, uint32_t length, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_COPY_DONE;
    timeout_activate(timeout_ms);
    copy(start_addr, length);
    return 0;
}

int GW_FwuMethod::paste(uint8_t * data, uint32_t length, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_PASTE_DONE;
    timeout_activate(timeout_ms);
    paste(data, length);
    return 0;
}

int GW_FwuMethod::finish(uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_FINISH_DONE;
    timeout_activate(timeout_ms);
    finish();
    return 0;
}

int GW_FwuMethod::report(void * params, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_REPORT_DONE;
    timeout_activate(timeout_ms);
    report(params);
    return 0;
}

void GW_FwuMethod::timeout_activate(uint32_t timeout_ms)
{
    tmr.attach(callback(this, &GW_FwuMethod::timeout), std::chrono::milliseconds(timeout_ms));
}

void GW_FwuMethod::timeout_inactivate(void)
{
    tmr.detach();
}

void GW_FwuMethod::timeout(void)
{
    tmp.status = FW_UPDATE_ERROR_CODE_TIMEOUT;
    f_reply(who, evt, &tmp);
}