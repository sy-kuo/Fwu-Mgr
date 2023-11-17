#include "gw_fwu_base.h"

void GW_FwuMethod::task_add(uint32_t id, uint32_t timeout_ms)
{
    task_id = id;

    if(thd != NULL)
        delete thd;
    thd = new Thread;
    thd->start(callback(this, &GW_FwuMethod::tasks_run));

    timeout_activate(timeout_ms);
}

void GW_FwuMethod::tasks_run(void)
{
    if(task_id == FW_UPDATE_EVENT_CHECKOUTED)
    {
        checkout(p_params);
    }
    else if(task_id == FW_UPDATE_EVENT_PREPARED)
    {
        prepare(p_params);
    }
    else if(task_id == FW_UPDATE_EVENT_COPY_DONE)
    {
        copy(_address, _length);
    }
    else if(task_id == FW_UPDATE_EVENT_PASTE_DONE)
    {
        paste(_pdata, _length);
    }
    else if(task_id == FW_UPDATE_EVENT_FINISH_DONE)
    {
        finish();
    }
    else if(task_id == FW_UPDATE_EVENT_REPORT_DONE)
    {
        report(p_params);
    }
}

int GW_FwuMethod::checkout(void * params, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_CHECKOUTED;
    p_params = params;
    task_add(FW_UPDATE_EVENT_CHECKOUTED, timeout_ms);
    return 0;
}

int GW_FwuMethod::prepare(void * params, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_PREPARED;
    p_params = params;
    task_add(FW_UPDATE_EVENT_PREPARED, timeout_ms);
    return 0;
}

int GW_FwuMethod::copy(uint32_t start_addr, uint32_t length, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_COPY_DONE;
    _address = start_addr;
    _length = length;
    task_add(FW_UPDATE_EVENT_COPY_DONE, timeout_ms);
    return 0;
}

int GW_FwuMethod::paste(uint8_t * data, uint32_t length, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_PASTE_DONE;
    _pdata = data;
    _length = length;
    task_add(FW_UPDATE_EVENT_PASTE_DONE, timeout_ms);
    return 0;
}

int GW_FwuMethod::finish(uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_FINISH_DONE;
    task_add(FW_UPDATE_EVENT_FINISH_DONE, timeout_ms);
    return 0;
}

int GW_FwuMethod::report(void * params, uint32_t timeout_ms)
{
    evt = FW_UPDATE_EVENT_REPORT_DONE;
    p_params = params;
    task_add(FW_UPDATE_EVENT_REPORT_DONE, timeout_ms);
    return 0;
}

int GW_FwuMethod::reply(int event, void * data)
{
    //if(tmr.remaining_time())
    timeout_inactivate();
    f_reply(who, event, data);
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