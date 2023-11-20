#ifndef __GW_FWU_H__
#define __GW_FWU_H__

#include <vector>
#include "mbed.h"
#include "gw_fwu_base.h"

#define YES  1
#define NO   0

void printHex(uint8_t * data, uint32_t len);

typedef enum {
    FW_UPDATE_VERSION_LEVEL_NORMAL,
    FW_UPDATE_VERSION_LEVEL_FORCE
} FW_UPDATE_VERSION_LEVEL_E;

class GW_Params
{
public:
    GW_Role_Basic src;
    GW_Role_Basic dst;
    GW_Role_Mgr mgr;
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

    void evt_push_ms(int who, int event, void * data, uint32_t delay_ms);

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