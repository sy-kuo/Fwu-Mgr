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
    infos.id = atoi(content);

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
    infos.timeout = atoi(content);

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
    infos.src.timeout_max = atoi(content);

    parsed_key[1] = (char *)"to_min";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    infos.src.timeout_max = atoi(content);

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
    infos.dst.timeout_max = atoi(content);

    parsed_key[1] = (char *)"to_min";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    infos.dst.timeout_max = atoi(content);

    parsed_key[1] = (char *)"level";
    parsed_prepare(&content, (char *)data, parsed_key, 2);
    infos.dst.level = atoi(content);
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
            infos.statuses.ack.supv = p_ready->status;
            infos.statuses.res.supv = p_ready->status;
            printf("Ready data: %s \r\n", (char *)p_ready->pdata);
            checkout_parse((char *)p_ready->pdata, supv_index, src_index, dst_index);
            role_list_set(supv_index, src_index, dst_index);
            if(infos.statuses.ack.supv != FW_UPDATE_ERROR_CODE_NULL)
            {
                infos.statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;
                src->checkout(&infos.mgr, infos.src.timeout_max);

                infos.statuses.ack.dst = FW_UPDATE_ERROR_CODE_NULL;
                dst->checkout(&infos.mgr, infos.dst.timeout_max);
            }
            break;
        }
        case FW_UPDATE_EVENT_CHECKOUTED:
        {
            fwu_params_s * p_checkout = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_SOURCE)
            {
                infos.src.ver = p_checkout->ver;
                infos.src.size = p_checkout->size;
                infos.src.length = p_checkout->length;
                infos.statuses.ack.src = p_checkout->status;
                infos.statuses.res.src = p_checkout->status;
            }
            else if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                infos.dst.ver = p_checkout->ver;
                infos.dst.length = p_checkout->length;
                infos.statuses.ack.dst = p_checkout->status;
                infos.statuses.res.dst = p_checkout->status;
            }

            if(infos.statuses.ack.src != FW_UPDATE_ERROR_CODE_NULL && infos.statuses.ack.dst != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(infos.statuses.ack.src == FW_UPDATE_ERROR_CODE_SUCESS && infos.statuses.ack.dst == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    infos.statuses.ack.mgr = FW_UPDATE_ERROR_CODE_NULL;
                    verify();
                }
                else
                {
                    printf("Checkout stop! Src:%d Dst:%d \r\n", infos.statuses.ack.src, infos.statuses.ack.dst);
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
                infos.statuses.ack.mgr = p_verify->status;
                infos.statuses.res.mgr = p_verify->status;
            }

            if(infos.statuses.ack.mgr != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(infos.statuses.ack.mgr == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    uint32_t max_length = infos.src.length >= infos.dst.length? infos.dst.length: infos.src.length;

                    infos.mgr.ver = infos.src.ver;
                    infos.mgr.size = infos.src.size;
                    infos.mgr.start_addr = 0;
                    infos.mgr.length = max_length;

                    infos.statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;
                    src->prepare(&infos.mgr, infos.src.timeout_max);

                    infos.statuses.ack.dst = FW_UPDATE_ERROR_CODE_NULL;
                    dst->prepare(&infos.mgr, infos.dst.timeout_max);
                }
                else
                {
                    printf("Verify stop! Mgr: %d \r\n", infos.statuses.ack.mgr);
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
                infos.statuses.ack.src = p_prepare->status;
                infos.statuses.res.src = p_prepare->status;
            }
            else if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                infos.statuses.ack.dst = p_prepare->status;
                infos.statuses.res.dst = p_prepare->status;
            }

            if(infos.statuses.ack.src != FW_UPDATE_ERROR_CODE_NULL && infos.statuses.ack.dst != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(infos.statuses.ack.src == FW_UPDATE_ERROR_CODE_SUCESS && infos.statuses.ack.dst == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    infos.statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;

                    infos.mgr.start_addr = infos.mgr.start_addr;
                    src->copy(infos.mgr.start_addr, infos.mgr.length, infos.src.timeout_max);
                }
                else
                {
                    printf("Prepare stop! Src: %d Dst: %d \r\n", infos.statuses.ack.src, infos.statuses.ack.dst);
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
                infos.statuses.ack.src = p_copy->status;
                infos.statuses.res.src = p_copy->status;
            }

            if(infos.statuses.ack.src != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(infos.statuses.ack.src == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    infos.mgr.pdata = p_copy->pdata;
                    infos.statuses.ack.dst = FW_UPDATE_ERROR_CODE_NULL;
                    dst->paste((uint8_t *)infos.mgr.pdata, infos.mgr.length, infos.dst.timeout_max);
                }
                else
                {
                    printf("Copy stop! Src: %d \r\n", infos.statuses.ack.src);
                }
            }
            break;
        }
        case FW_UPDATE_EVENT_PASTE_DONE:
        {
            fwu_params_s * p_paste = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                infos.statuses.ack.dst = p_paste->status;
                infos.statuses.res.dst = p_paste->status;
            }

            if(infos.statuses.ack.dst != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(infos.statuses.ack.dst == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    infos.mgr.start_addr = p_paste->start_addr;
 
                    if(infos.mgr.start_addr >= infos.src.size)
                    {
                        infos.statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;
                        src->finish(infos.src.timeout_max);
                        infos.statuses.ack.dst = FW_UPDATE_ERROR_CODE_NULL;
                        dst->finish(infos.dst.timeout_max);
                    }
                    else
                    {
                        infos.statuses.ack.src = FW_UPDATE_ERROR_CODE_NULL;
                        src->copy(infos.mgr.start_addr, infos.mgr.length, infos.src.timeout_max);
                    }
                }
                else
                {
                    printf("Paste stop! Dst: %d \r\n", infos.statuses.ack.dst);
                }
            }
            break;
        }
        case FW_UPDATE_EVENT_FINISH_DONE:
        {
            fwu_params_s * p_finish = (fwu_params_s *)data;
            if(who == FW_UPDATE_ROLE_SOURCE)
            {
                infos.statuses.ack.src = p_finish->status;
                infos.statuses.res.src = p_finish->status;
            }
            else if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                infos.statuses.ack.dst = p_finish->status;
                infos.statuses.res.dst = p_finish->status;
            }

            if(infos.statuses.ack.src != FW_UPDATE_ERROR_CODE_NULL && infos.statuses.ack.dst != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(infos.statuses.ack.src == FW_UPDATE_ERROR_CODE_SUCESS && infos.statuses.ack.dst == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    printf("Finish done! \r\n");
                }
                else
                {
                    printf("Finish stop! Src: %d Dst: %d \r\n", infos.statuses.ack.src, infos.statuses.ack.dst);
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
                infos.statuses.ack.supv = p_report->status;
                infos.statuses.res.supv = p_report->status;
            }

            if(infos.statuses.ack.supv != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(infos.statuses.ack.supv == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                }
                else
                {
                    printf("Report stop! Supv: %d \r\n", infos.statuses.ack.supv);
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
        infos.mgr.id = infos.id;
        infos.mgr.evt_id = event;
        memcpy(&infos.mgr.statuses, &infos.statuses.res, sizeof(fwu_roles_s));
        infos.statuses.ack.supv = FW_UPDATE_ERROR_CODE_NULL;
        supv->report((void *)&infos.mgr, infos.timeout);
    }

    return;
}

void GW_FwUpdate::ready_checkout(char * params)
{
    infos.statuses.ack.mgr = FW_UPDATE_ERROR_CODE_SUCESS;
    infos.statuses.ack.supv = FW_UPDATE_ERROR_CODE_SUCESS;
    infos.statuses.ack.src = FW_UPDATE_ERROR_CODE_SUCESS;
    infos.statuses.ack.dst = FW_UPDATE_ERROR_CODE_SUCESS;

    infos.statuses.res.mgr = FW_UPDATE_ERROR_CODE_SUCESS;
    infos.statuses.res.supv = FW_UPDATE_ERROR_CODE_SUCESS;
    infos.statuses.res.src = FW_UPDATE_ERROR_CODE_SUCESS;
    infos.statuses.res.dst = FW_UPDATE_ERROR_CODE_SUCESS;

    infos.mgr.status = FW_UPDATE_ERROR_CODE_SUCESS;
    infos.mgr.pdata = (void *)params;

    evt_push(FW_UPDATE_ROLE_SUPERVISOR, FW_UPDATE_EVENT_READY_CHECKOUT, (void *)&infos.mgr);
}

void GW_FwUpdate::evt_push(int who, int event, void * data)
{
    evt(who, event, data);
}

void GW_FwUpdate::verify(void)
{
    infos.mgr.status = FW_UPDATE_ERROR_CODE_FIRMWARE_UPDATE_TO_DATE;
    if(infos.src.ver != infos.dst.ver)
    {
        if(infos.dst.level == FW_UPDATE_VERSION_LEVEL_NORMAL)
        {
            if(infos.src.ver > infos.dst.ver)
            {
                infos.mgr.status = FW_UPDATE_ERROR_CODE_SUCESS;
            }
        }
        else if(infos.dst.level == FW_UPDATE_VERSION_LEVEL_FORCE)
        {
            infos.mgr.status = FW_UPDATE_ERROR_CODE_SUCESS;
        }
    }
    evt_push(FW_UPDATE_ROLE_MANAGER, FW_UPDATE_EVENT_VERIFY_DONE, (void *)&infos.mgr);
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
