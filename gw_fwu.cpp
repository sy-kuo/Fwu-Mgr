#include "mbed.h"
#include "mbed_trace.h"

#include "gw_fwu.h"
#include "gw_fwu_test.h"

#include "Json.h"
#include "gw_tools.h"

#if MBED_CONF_MBED_TRACE_ENABLE
#define TRACE_GROUP     "FWU"
#endif

GW_FwUpdate * m_FwuMgr;

const char * whos[] = {"None", "Mgr", "Supv", "Src", "Dst"};
const char * evts[] = {"Idle", "Ready", "Checkouted", "Verified", "Prepared", "Copied", "Pasted", "Finished", "Report"};

void json_token_info(Json& json, int index)
{
    const char * valueStart = json.tokenAddress(index);
    int valueLength = json.tokenLength(index);
    char * value = (char *)calloc(valueLength + 1, 1);
    strncpy(value, valueStart, valueLength);
    printf("Parent: %d, Child: %d, type: %d", json.parent(index), json.childCount(index), json.type(index));
    printf("[%d]: %s\r\n", index, value);
    free(value);
}

int parsed_prepare(char ** parsed, char * p_prepare, char * key[], int index)
{
    int ret = EXIT_SUCCESS, i, j, KeyIndex = 0, valueLength;
    const char * valueStart;
    char * new_id;

    GW_BufLen * data, * jsonSource = new GW_BufLen(p_prepare, strlen(p_prepare));

    Json * json;
    json = new Json(jsonSource->buffer, strlen(jsonSource->buffer), strlen(jsonSource->buffer));

    if(!json->isValidJson())
    {
        tr_err ( "Invalid JSON: %s", jsonSource->buffer);
        ret = EXIT_FAILURE;
        //goto _exit_info_parse;
    }

    for(i=0;i<index;i++)
    {
        int tmp_key_index = 0;
        for(j=KeyIndex;j<json->parsedTokenCount();j++)
        {
            if(json->type(KeyIndex) == JSMN_OBJECT)
                break;
            else
                KeyIndex++;
        }

        KeyIndex = json->findKeyIndexIn(key[i], KeyIndex) + 1;
    }

    if(KeyIndex == json->parsedTokenCount() || KeyIndex == 0)
    {
        ret = EXIT_FAILURE;
        //goto _exit_info_parse;
    }

    valueStart = json->tokenAddress(KeyIndex);
    valueLength = json->tokenLength(KeyIndex);

    data = new GW_BufLen((char *)valueStart, valueLength);
    if(*parsed != NULL)
        free(*parsed);
    *parsed = (char *)calloc(data->len + 1, sizeof(char));
    strncpy(*parsed, data->buffer, data->len);

    delete data;

    //for(i=0;i<json->parsedTokenCount();i++)
    //    json_token_info(*json, i);

_exit_info_parse:
    delete json;
    delete jsonSource;

    return ret;
}

void GW_FwUpdate::checkout_parse(char * data, uint8_t & supv_index, uint8_t & src_index, uint8_t & dst_index)
{
    uint8_t id_index;
    char * parsed_key[2], * content = NULL;

    parsed_key[0] = (char *)"mgr";
    parsed_key[1] = (char *)"id";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    mgr_id = atoi(content);

    parsed_key[0] = (char *)"supv";
    parsed_key[1] = (char *)"id";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    for(id_index=0;id_index<supv_list.size();id_index++)
    {
        if(supv_list[id_index]->module_id == atoi(content))
            supv_index = id_index;
    }
    parsed_key[1] = (char *)"to";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    supv_timeout = atoi(content);

    parsed_key[0] = (char *)"src";
    parsed_key[1] = (char *)"id";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    for(id_index=0;id_index<src_list.size();id_index++)
    {
        if(src_list[id_index]->module_id == atoi(content))
            src_index = id_index;
    }
    parsed_key[1] = (char *)"to_max";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    src_params.timeout_max = atoi(content);

    parsed_key[1] = (char *)"to_min";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    src_params.timeout_max = atoi(content);

    parsed_key[0] = (char *)"dst";
    parsed_key[1] = (char *)"id";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    for(id_index=0;id_index<dst_list.size();id_index++)
    {
        if(dst_list[id_index]->module_id == atoi(content))
            dst_index = id_index;
    }
    parsed_key[1] = (char *)"to_max";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    dst_params.timeout_max = atoi(content);

    parsed_key[1] = (char *)"to_min";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    dst_params.timeout_max = atoi(content);

    parsed_key[1] = (char *)"level";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    dst_params.level = atoi(content);
}

void GW_FwUpdate::evt_cb(int who, int event, void * data)
{
    uint8_t try_report = NO;

    if(event == FW_UPDATE_EVENT_NEXT_TEST)
    {
        this_case++;
        printf("Next test id: %d !\r\n", this_case);
        return;
    }

    if(who < FW_UPDATE_ROLE_COUNT && event < FW_UPDATE_EVENT_COUNT)
        printf("---> Who: %s, evt: %s \r\n", whos[who], evts[event]);

    switch(event)
    {
        case FW_UPDATE_EVENT_READY_CHECKOUT:
        {
            uint8_t supv_index, src_index, dst_index;
            fwu_params_s * p_ready = (fwu_params_s *)data;
            statuses.ack.supv = p_ready->status;
            statuses.res.supv = p_ready->status;
            printf("Ready data: %s \r\n", (char *)p_ready->pdata);
            checkout_parse((char *)p_ready->pdata, supv_index, src_index, dst_index);
            role_list_set(supv_index, src_index, dst_index);
            if(statuses.ack.supv != FW_UPDATE_ERROR_CODE_NULL)
            {
                statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;
                src->checkout(&infos, src_params.timeout_max);

                statuses.ack.dst = FW_UPDATE_ERROR_CODE_NULL;
                dst->checkout(&infos, dst_params.timeout_max);
            }
            break;
        }
        case FW_UPDATE_EVENT_CHECKOUTED:
        {
            fwu_params_s * p_checkout = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_SOURCE)
            {
                src_params.ver = p_checkout->ver;
                src_params.size = p_checkout->size;
                src_params.length = p_checkout->length;
                statuses.ack.src = p_checkout->status;
                statuses.res.src = p_checkout->status;
            }
            else if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                dst_params.ver = p_checkout->ver;
                dst_params.length = p_checkout->length;
                statuses.ack.dst = p_checkout->status;
                statuses.res.dst = p_checkout->status;
            }

            if(statuses.ack.src != FW_UPDATE_ERROR_CODE_NULL && statuses.ack.dst != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(statuses.ack.src == FW_UPDATE_ERROR_CODE_SUCESS && statuses.ack.dst == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    statuses.ack.mgr = FW_UPDATE_ERROR_CODE_NULL;
                    verify();
                }
                else
                {
                    printf("Checkout stop! Src:%d Dst:%d \r\n", statuses.ack.src, statuses.ack.dst);
                }
                try_report = YES;
            }
            break;
        }
        case FW_UPDATE_EVENT_VERIFY_DONE:
        {
            fwu_params_s * p_verify = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_MANAGER)
            {
                statuses.ack.mgr = p_verify->status;
                statuses.res.mgr = p_verify->status;
            }

            if(statuses.ack.mgr != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(statuses.ack.mgr == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    uint32_t max_length = src_params.length >= dst_params.length? dst_params.length: src_params.length;

                    infos.ver = src_params.ver;
                    infos.size = src_params.size;
                    infos.start_addr = 0;
                    infos.length = max_length;

                    statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;
                    src->prepare(&infos, src_params.timeout_max);

                    statuses.ack.dst = FW_UPDATE_ERROR_CODE_NULL;
                    dst->prepare(&infos, dst_params.timeout_max);
                }
                else
                {
                    printf("Verify stop! Mgr: %d \r\n", statuses.ack.mgr);
                }
                try_report = YES;
            }
            break;
        }
        case FW_UPDATE_EVENT_PREPARED:
        {
            fwu_params_s * p_prepare = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_SOURCE)
            {
                statuses.ack.src = p_prepare->status;
                statuses.res.src = p_prepare->status;
            }
            else if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                statuses.ack.dst = p_prepare->status;
                statuses.res.dst = p_prepare->status;
            }

            if(statuses.ack.src != FW_UPDATE_ERROR_CODE_NULL && statuses.ack.dst != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(statuses.ack.src == FW_UPDATE_ERROR_CODE_SUCESS && statuses.ack.dst == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;

                    infos.start_addr = infos.start_addr;
                    src->copy(infos.start_addr, infos.length, src_params.timeout_max);
                }
                else
                {
                    printf("Prepare stop! Src: %d Dst: %d \r\n", statuses.ack.src, statuses.ack.dst);
                }
                try_report = YES;
            }
            break;
        }
        case FW_UPDATE_EVENT_COPY_DONE:
        {
            fwu_params_s * p_copy = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_SOURCE)
            {
                statuses.ack.src = p_copy->status;
                statuses.res.src = p_copy->status;
            }

            if(statuses.ack.src != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(statuses.ack.src == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    infos.pdata = p_copy->pdata;
                    statuses.ack.dst = FW_UPDATE_ERROR_CODE_NULL;
                    dst->paste((uint8_t *)infos.pdata, infos.length, dst_params.timeout_max);
                }
                else
                {
                    printf("Copy stop! Src: %d \r\n", statuses.ack.src);
                }
            }
            break;
        }
        case FW_UPDATE_EVENT_PASTE_DONE:
        {
            fwu_params_s * p_paste = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                statuses.ack.dst = p_paste->status;
                statuses.res.dst = p_paste->status;
            }

            if(statuses.ack.dst != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(statuses.ack.dst == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    infos.start_addr = p_paste->start_addr;
 
                    if(infos.start_addr >= src_params.size)
                    {
                        statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;
                        src->finish(src_params.timeout_max);
                        statuses.ack.dst = FW_UPDATE_ERROR_CODE_NULL;
                        dst->finish(dst_params.timeout_max);
                    }
                    else
                    {
                        statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;
                        src->copy(infos.start_addr, infos.length, src_params.timeout_max);
                    }
                }
                else
                {
                    printf("Paste stop! Dst: %d \r\n", statuses.ack.dst);
                }
            }
            break;
        }
        case FW_UPDATE_EVENT_FINISH_DONE:
        {
            fwu_params_s * p_finish = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_SOURCE)
            {
                statuses.ack.src = p_finish->status;
                statuses.res.src = p_finish->status;
            }
            else if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                statuses.ack.dst = p_finish->status;
                statuses.res.dst = p_finish->status;
            }

            if(statuses.ack.src != FW_UPDATE_ERROR_CODE_NULL && statuses.ack.dst != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(statuses.ack.src == FW_UPDATE_ERROR_CODE_SUCESS && statuses.ack.dst == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    printf("Finish done! \r\n");
                }
                else
                {
                    printf("Finish stop! Src: %d Dst: %d \r\n", statuses.ack.src, statuses.ack.dst);
                }
                try_report = YES;
            }
            break;
        }
        case FW_UPDATE_EVENT_REPORT_DONE:
        {
            fwu_params_s * p_report = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_SUPERVISOR)
            {
                statuses.ack.supv = p_report->status;
                statuses.res.supv = p_report->status;
            }

            if(statuses.ack.supv != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(statuses.ack.supv == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                }
                else
                {
                    printf("Report stop! Supv: %d \r\n", statuses.ack.supv);
                }
            }
            break;
        }
        default:
        {
            printf("Unknown params! Who: %d, Evt: %d, Data: %s \r\n", who, event, (char *)data);
            break;
        }
    }

    if(try_report == YES)
    {
        infos.id = mgr_id;
        infos.evt_id = event;
        memcpy(&infos.statuses, &statuses.res, sizeof(fwu_roles_s));
        statuses.ack.supv = FW_UPDATE_ERROR_CODE_NULL;
        supv->report((void *)&infos, supv_timeout);
    }

    return;
}

void GW_FwUpdate::ready_checkout(char * params)
{
    statuses.ack.mgr = FW_UPDATE_ERROR_CODE_SUCESS;
    statuses.ack.supv = FW_UPDATE_ERROR_CODE_SUCESS;
    statuses.ack.src = FW_UPDATE_ERROR_CODE_SUCESS;
    statuses.ack.dst = FW_UPDATE_ERROR_CODE_SUCESS;

    statuses.res.mgr = FW_UPDATE_ERROR_CODE_SUCESS;
    statuses.res.supv = FW_UPDATE_ERROR_CODE_SUCESS;
    statuses.res.src = FW_UPDATE_ERROR_CODE_SUCESS;
    statuses.res.dst = FW_UPDATE_ERROR_CODE_SUCESS;

    infos.status = FW_UPDATE_ERROR_CODE_SUCESS;
    infos.pdata = (void *)params;

    evt_push(FW_UPDATE_ROLE_SUPERVISOR, FW_UPDATE_EVENT_READY_CHECKOUT, (void *)&infos);
}

void GW_FwUpdate::evt_push(int who, int event, void * data)
{
    evt(who, event, data);
}

void GW_FwUpdate::verify(void)
{
    infos.status = FW_UPDATE_ERROR_CODE_FIRMWARE_UPDATE_TO_DATE;
    if(src_params.ver != dst_params.ver)
    {
        if(dst_params.level == FW_UPDATE_VERSION_LEVEL_NORMAL)
        {
            if(src_params.ver > dst_params.ver)
            {
                infos.status = FW_UPDATE_ERROR_CODE_SUCESS;
            }
        }
        else if(dst_params.level == FW_UPDATE_VERSION_LEVEL_FORCE)
        {
            infos.status = FW_UPDATE_ERROR_CODE_SUCESS;
        }
    }
    evt_push(FW_UPDATE_ROLE_MANAGER, FW_UPDATE_EVENT_VERIFY_DONE, (void *)&infos);
}

void GW_FwUpdate::role_list_set(uint8_t supv_index, uint8_t src_index, uint8_t dst_index)
{
    supv = supv_list[supv_index];
    src = src_list[src_index];
    dst = dst_list[dst_index];
    who_is(supv, FW_UPDATE_ROLE_SUPERVISOR);
    who_is(src, FW_UPDATE_ROLE_SOURCE);
    who_is(dst, FW_UPDATE_ROLE_DESTINATION);
}

template <class Role>
void GW_FwUpdate::role_register(Role * role, int id, FW_UPDATE_ROLE_E who)
{
    GW_FwuMethod * p_role = role;
    p_role->module_id = id;
    std::vector<GW_FwuMethod *> * p_list;
    if(who == FW_UPDATE_ROLE_SUPERVISOR)
        p_list = &supv_list;
    else if(who == FW_UPDATE_ROLE_SOURCE)
        p_list = &src_list;
    else if(who == FW_UPDATE_ROLE_DESTINATION)
        p_list = &dst_list;
    p_list->push_back(p_role);
    reply_set(p_role);
}

template <class Role>
void GW_FwUpdate::supv_register(Role * role, int id)
{
    role_register(role, id, FW_UPDATE_ROLE_SUPERVISOR);
}

template <class Role>
void GW_FwUpdate::src_register(Role * role, int id)
{
    role_register(role, id, FW_UPDATE_ROLE_SOURCE);
}

template <class Role>
void GW_FwUpdate::dst_register(Role * role, int id)
{
    role_register(role, id, FW_UPDATE_ROLE_DESTINATION);
}

template <class Role>
void GW_FwUpdate::who_is(Role * role, FW_UPDATE_ROLE_E who)
{
    role->who = who;
}

template <class Role>
void GW_FwUpdate::reply_set(Role * role)
{
    role->f_reply = std::bind(&GW_FwUpdate::evt_push, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void gw_fwu_init()
{
    m_FwuMgr = new GW_FwUpdate();

    m_FwuMgr->supv_register(&supv_1, 11);
    m_FwuMgr->supv_register(&supv_2, 12);
    m_FwuMgr->src_register(&src_1, 21);
    m_FwuMgr->src_register(&src_2, 22);
    m_FwuMgr->dst_register(&dst_1, 31);
    m_FwuMgr->dst_register(&dst_2, 32);

    printf("Start .... \r\n");
}
