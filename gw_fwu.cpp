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
    supv_params.timeout = atoi(content);

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

    fwu_parms_ctrl_s * ctrl_data = (fwu_parms_ctrl_s *)data;
    fwu_params_s * src_data = (fwu_params_s *)data;
    fwu_params_s * dst_data = (fwu_params_s *)data;

    if(who < FW_UPDATE_ROLE_COUNT && event < FW_UPDATE_EVENT_COUNT)
        printf("---> Who: %s, evt: %s \r\n", whos[who], evts[event]);

    if(who == FW_UPDATE_ROLE_MANAGER)
    {
        mgr_params.status = ctrl_data->status;
        supv_params.mgr_status = mgr_params.status;
    }
    else if(who == FW_UPDATE_ROLE_SUPERVISOR)
    {
        supv_params.status = ctrl_data->status;
        supv_params.supv_status = supv_params.status;

    }
    else if(who == FW_UPDATE_ROLE_SOURCE)
    {
        src_params.status = src_data->status;
        supv_params.src_status = src_params.status;
    }
    else if(who == FW_UPDATE_ROLE_DESTINATION)
    {
        dst_params.status = dst_data->status;
        supv_params.dst_status = dst_params.status;
    }

    switch(event)
    {
        case FW_UPDATE_EVENT_READY_CHECKOUT:
        {
            uint8_t supv_index, src_index, dst_index;
            printf("Prepare data: %s \r\n", (char *)ctrl_data->pdata);
            checkout_parse((char *)ctrl_data->pdata, supv_index, src_index, dst_index);
            role_list_set(supv_index, src_index, dst_index);
            if(supv_params.status != FW_UPDATE_ERROR_CODE_NULL)
            {
                src_params.status = FW_UPDATE_ERROR_CODE_NULL;
                src->checkout(src_params.timeout_max);
                dst_params.status = FW_UPDATE_ERROR_CODE_NULL;
                dst->checkout(dst_params.timeout_max);
            }
            break;
        }
        case FW_UPDATE_EVENT_CHECKOUTED:
        {
            if(who == FW_UPDATE_ROLE_SOURCE)
            {
                src_params.ver = src_data->ver;
                src_params.size = src_data->size;
                src_params.length = src_data->length;
            }
            else if(who == FW_UPDATE_ROLE_DESTINATION)
            {
                dst_params.ver = dst_data->ver;
                dst_params.length = dst_data->length;
            }

            if(src_params.status != FW_UPDATE_ERROR_CODE_NULL && dst_params.status != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(src_params.status == FW_UPDATE_ERROR_CODE_SUCESS && dst_params.status == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    mgr_params.status = FW_UPDATE_ERROR_CODE_NULL;
                    verify();
                }
                else
                {
                    printf("Checkout stop! Src:%d Dst:%d \r\n", src_params.status, dst_params.status);
                }
                try_report = YES;
            }
            break;
        }
        case FW_UPDATE_EVENT_VERIFY_DONE:
        {
            if(mgr_params.status != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(mgr_params.status == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    uint32_t max_length = src_params.length >= dst_params.length? dst_params.length: src_params.length;
                    src_params.length = max_length;
                    dst_params.length = max_length;
                    dst_params.ver = src_params.ver;
                    dst_params.size = src_params.size;
                    dst_params.start_addr = 0;

                    src_params.status = FW_UPDATE_ERROR_CODE_NULL;
                    src->prepare(&src_params, src_params.timeout_max);
                    dst_params.status = FW_UPDATE_ERROR_CODE_NULL;
                    dst->prepare(&dst_params, dst_params.timeout_max);
                }
                else
                {
                    printf("Verify stop! Mgr: %d \r\n", mgr_params.status);
                }
                try_report = YES;
            }
            break;
        }
        case FW_UPDATE_EVENT_PREPARED:
        {
            if(src_params.status != FW_UPDATE_ERROR_CODE_NULL && dst_params.status != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(src_params.status == FW_UPDATE_ERROR_CODE_SUCESS && dst_params.status == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    src_params.status = FW_UPDATE_ERROR_CODE_NULL;
                    src->copy(dst_data->start_addr, dst_data->length, src_params.timeout_max);
                }
                else
                {
                    printf("Prepare stop! Src: %d Dst: %d \r\n", src_params.status, dst_params.status);
                }
                try_report = YES;
            }
            break;
        }
        case FW_UPDATE_EVENT_COPY_DONE:
        {
            if(src_params.status != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(src_params.status == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    dst_params.status = FW_UPDATE_ERROR_CODE_NULL;
                    dst->paste(src_data->data, src_data->length, dst_params.timeout_max);
                }
                else
                {
                    printf("Copy stop! Src: %d \r\n", src_params.status);
                }
            }
            break;
        }
        case FW_UPDATE_EVENT_PASTE_DONE:
        {
            if(dst_params.status != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(dst_params.status == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                    if(dst_data->start_addr >= src_params.size)
                    {
                        src_params.status = FW_UPDATE_ERROR_CODE_NULL;
                        src->finish(src_params.timeout_max);
                        dst_params.status = FW_UPDATE_ERROR_CODE_NULL;
                        dst->finish(dst_params.timeout_max);
                    }
                    else
                    {
                        src_params.status = FW_UPDATE_ERROR_CODE_NULL;
                        src->copy(dst_data->start_addr, dst_data->length, src_params.timeout_max);
                    }
                }
                else
                {
                    printf("Paste stop! Dst: %d \r\n", dst_params.status);
                }
            }
            break;
        }
        case FW_UPDATE_EVENT_FINISH_DONE:
        {
            if(src_params.status != FW_UPDATE_ERROR_CODE_NULL && dst_params.status != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(src_params.status == FW_UPDATE_ERROR_CODE_SUCESS && dst_params.status == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                }
                else
                {
                    printf("Finish stop! Src: %d Dst: %d \r\n", src_params.status, dst_params.status);
                }
                try_report = YES;
            }
            break;
        }
        case FW_UPDATE_EVENT_REPORT_DONE:
        {
            if(supv_params.status != FW_UPDATE_ERROR_CODE_NULL)
            {
                if(supv_params.status == FW_UPDATE_ERROR_CODE_SUCESS)
                {
                }
                else
                {
                    printf("Report stop! Supv: %d \r\n", supv_params.status);
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
        supv_params.evt_id = event;
        supv_params.status = FW_UPDATE_ERROR_CODE_NULL;
        supv->report((void *)&supv_params, supv_params.timeout);
    }

    return;
}

void GW_FwUpdate::ready_checkout(char * params)
{
    supv_params.mgr_status = FW_UPDATE_ERROR_CODE_SUCESS;
    supv_params.supv_status = FW_UPDATE_ERROR_CODE_SUCESS;
    supv_params.src_status = FW_UPDATE_ERROR_CODE_SUCESS;
    supv_params.dst_status = FW_UPDATE_ERROR_CODE_SUCESS;

    mgr_params.pdata = (void *)params;
    mgr_params.status = FW_UPDATE_ERROR_CODE_SUCESS;
    evt_push(FW_UPDATE_ROLE_SUPERVISOR, FW_UPDATE_EVENT_READY_CHECKOUT, (void *)&mgr_params);
}

void GW_FwUpdate::evt_push(int who, int event, void * data)
{
    evt(who, event, data);
}

void GW_FwUpdate::verify(void)
{
    printf("Verifying! Src: %X. Dst: %X \r\n", src_params.ver, dst_params.ver);
    mgr_params.status = FW_UPDATE_ERROR_CODE_FIRMWARE_UPDATE_TO_DATE;
    if(src_params.ver != dst_params.ver)
    {
        if(dst_params.level == FW_UPDATE_VERSION_LEVEL_NORMAL)
        {
            if(src_params.ver > dst_params.ver)
            {
                mgr_params.status = FW_UPDATE_ERROR_CODE_SUCESS;
            }
        }
        else if(dst_params.level == FW_UPDATE_VERSION_LEVEL_FORCE)
        {
            mgr_params.status = FW_UPDATE_ERROR_CODE_SUCESS;
        }
    }
    evt_push(FW_UPDATE_ROLE_MANAGER, FW_UPDATE_EVENT_VERIFY_DONE, (void *)&mgr_params);
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

    printf("Start ....xx \r\n");
}
