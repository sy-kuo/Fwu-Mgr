#include "mbed.h"
#include "mbed_trace.h"
#include "https_request.h"
#include "Json.h"
#include "Base64.h"
#include "gw_https.h"

#if MBED_CONF_MBED_TRACE_ENABLE
#define TRACE_GROUP     "HTTPS"
#endif

GW_RestApi * m_RestApi;

void dump_response(HttpResponse * res)
{
    printf("Status: %d - %s \r\n", res->get_status_code(), res->get_status_message().c_str());

    printf("Headers: \r\n");
    for (size_t ix = 0; ix < res->get_headers_length(); ix++) {
        printf("\t%s: %s \r\n", res->get_headers_fields()[ix]->c_str(), res->get_headers_values()[ix]->c_str());
    }
    printf("Body (%lu bytes):\n%s \r\n", res->get_body_length(), res->get_body_as_string().c_str());
}

uint32_t content_sizeof(void * res)
{
    uint32_t size = 0;
    HttpResponse* _res = (HttpResponse *)res;
    for (size_t ix = 0; ix < _res->get_headers_length(); ix++) {
        if(strcmp("Content-Range", _res->get_headers_fields()[ix]->c_str()) == 0)
        {
            uint32_t start_addr, end_addr;
            sscanf(_res->get_headers_values()[ix]->c_str(), "bytes %d-%d/%d", &start_addr, &end_addr, &size);
            //printf("\t%s: %s \r\n", _res->get_headers_fields()[ix]->c_str(), _res->get_headers_values()[ix]->c_str());
        }
    }
    return size;
}

void GW_RestApi::task_add(uint32_t id)
{
    task_id = id;

    if(thd != NULL)
        delete thd;
    thd = new Thread;
    thd->start(callback(this, &GW_RestApi::tasks_run));
}

void GW_RestApi::tasks_run(void)
{
    if(task_id == FW_UPDATE_EVENT_CHECKOUTED)
    {
        get_endpoint((char *)p_params);

        if(bytes_range != NULL)
            delete bytes_range;
        bytes_range = new GW_BufLen("bytes=0-1");
        int res = rest_api((char *)"%s", sitefiles_id->buffer);

        fwu_res.status = res? res: FW_UPDATE_ERROR_CODE_SUCESS;
        if(fwu_res.status == FW_UPDATE_ERROR_CODE_SUCESS)
        {
            fwu_res.size = content_size;
            fwu_res.ver = SRC_HTTPS_VERSION;
            fwu_res.length = SRC_HTTPS_MAX_LENGTH;
        }
        printf("<--- %s checkout!\r\n", class_id);
    }
    else if(task_id == FW_UPDATE_EVENT_PREPARED)
    {
        int res = 0;
        fwu_res.status = res? res: FW_UPDATE_ERROR_CODE_SUCESS;
        if(fwu_res.status == FW_UPDATE_ERROR_CODE_SUCESS)
        {
            GW_Role_Basic * p_params = (GW_Role_Basic *)this->p_params;

            fwu_res.length = p_params->length;
            fwu_res.size = p_params->size;
            fwu_res.status = FW_UPDATE_ERROR_CODE_SUCESS;
            printf("<--- %s prepare! Ver: %X, Size: %d, Len: %d \r\n", class_id, p_params->ver, fwu_res.size, fwu_res.length);
        }
    }
    else if(task_id == FW_UPDATE_EVENT_COPY_DONE)
    {
        if(bytes_range != NULL)
            delete bytes_range;
        bytes_range = new GW_BufLen("bytes=%u-%u", file_addr, file_addr + file_len - 1);
        int res = rest_api((char *)"%s", sitefiles_id->buffer);

        fwu_res.status = res? res: FW_UPDATE_ERROR_CODE_SUCESS;
        if(fwu_res.status == FW_UPDATE_ERROR_CODE_SUCESS)
        {
            fwu_res.pdata = body->buffer;
            printf("<--- %s copy! %s \r\n", class_id, bytes_range->buffer);
        }
    }
    else if(task_id == FW_UPDATE_EVENT_FINISH_DONE)
    {
        fwu_res.status = FW_UPDATE_ERROR_CODE_SUCESS;
    }
    reply(task_id, (void *)&fwu_res);
}

int GW_RestApi::checkout(void * params)
{
    p_params = params;
    task_add(FW_UPDATE_EVENT_CHECKOUTED);
    return 0;
}

int GW_RestApi::prepare(void * params)
{
    p_params = params;
    task_add(FW_UPDATE_EVENT_PREPARED);
    return 0;
}

int GW_RestApi::copy(uint32_t start_addr, uint32_t length)
{
    file_addr = start_addr;
    file_len = length;
    task_add(FW_UPDATE_EVENT_COPY_DONE);
    return 0;
}

int GW_RestApi::finish(void)
{
    task_add(FW_UPDATE_EVENT_FINISH_DONE);
    return 0;
}

int GW_RestApi::get(void)
{
    act.lock();

    Base64 n;
    size_t encodedLen;
    char * dev_key = n.Encode(device_key->buffer, strlen(device_key->buffer), &encodedLen);

    GW_BufLen header_2 = GW_BufLen("Basic %s", dev_key);
    GW_BufLen url_get = GW_BufLen("%s%s", server_url->buffer, endpoint->buffer);

    free(dev_key);

    if(is_debugging)
        printf("Url: %s \r\n", url_get.buffer);

    if(body != NULL)
    {
        delete body;
        body = NULL;
    }

    HttpsRequest * get_req = new HttpsRequest(network, NULL, HTTP_GET, url_get.buffer);
    get_req->set_header("X-API-KEY", "MToxMjM=");
    get_req->set_header("Authorization", header_2.buffer);
    if(bytes_range != NULL)
        get_req->set_header("Range", bytes_range->buffer);

    HttpResponse * get_res = get_req->send();
    if (get_res)
    {
        body = new GW_BufLen((char *)get_res->get_body_as_string().c_str(), get_res->get_body_length());

        if(is_debugging)
            dump_response(get_res);

        content_size = content_sizeof(get_res);
    }
    else
        printf("No res! \r\n");

    delete get_req;
    act.unlock();

    return body == NULL? REST_API_FAILED: REST_API_SUCCESSFUL;
}

void ReplaceAll(char ** infos, const std::string& from, const std::string& to)
{
    char * new_infos = *infos;
    size_t start_pos = 0;
    std::string str(*infos);
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }

    new_infos = (char *)realloc(*infos, strlen(str.c_str()) + 1);
    *infos = new_infos;
    strcpy(*infos, str.c_str());
}

void CopyString(char ** dst, char * src)
{
    if(*dst != NULL)
    {
        free(*dst);
        *dst = NULL;
    }
    *dst = (char *)calloc(strlen(src) + 1, sizeof(char));
    strcpy(*dst, src);
}

void GW_RestApi::get_endpoint(char * p_cfg)
{
#define CONTENT_KEY_NAME             "content"
#define BINARY_URL_KEY_NAME          "url"
#define BINARY_URL_KEY_LENGTH        2

    char * parsed_key[2], * content;
    int res;
    GW_RestApi_params params = {0};

    is_debugging = false;

    content = NULL;
    parsed_key[0] = (char *)"url";
    parsed_prepare(&content, (char *)p_cfg, parsed_key, 1);
    params.server_url = content;

    content = NULL;
    parsed_key[0] = (char *)"key";
    parsed_prepare(&content, (char *)p_cfg, parsed_key, 1);
    params.device_key = content;

    content = NULL;
    parsed_key[0] = (char *)"id";
    parsed_prepare(&content, (char *)p_cfg, parsed_key, 1);
    params.id = content;

    printf("Url: %s \r\n", params.server_url);
    printf("Key: %s \r\n", params.device_key);
    printf("id: %s \r\n", params.id);

    params_set(params);

    if(sitefiles_id != NULL)
    {
        delete sitefiles_id;
        sitefiles_id = NULL;
    }

    rest_api((char *)"/sitefiles/%s", id->buffer);

    parsed_key[0] = (char *)CONTENT_KEY_NAME;
    parsed_key[1] = (char *)BINARY_URL_KEY_NAME;
    res = body_parse(parsed_key, BINARY_URL_KEY_LENGTH);
    if(res == EXIT_SUCCESS)
        sitefiles_id = new GW_BufLen((char *)parsed_body->buffer);

    delete parsed_body;
    parsed_body = NULL;

    return;
}

void GW_RestApi::json_token_info(Json& json, int index)
{
    const char * valueStart = json.tokenAddress(index);
    int valueLength = json.tokenLength(index);
    char * value = (char *)calloc(valueLength + 1, 1);
    strncpy(value, valueStart, valueLength);
    printf("Parent: %d, Child: %d, type: %d \r\n", json.parent(index), json.childCount(index), json.type(index));
    printf("[%d]: %s \r\n", index, value);
}

int GW_RestApi::body_parse(char * key[], int index)
{
    int ret = EXIT_SUCCESS, i, j, KeyIndex = 0, valueLength;
    const char * valueStart;
    char * new_id;

    GW_BufLen * jsonSource = new GW_BufLen(body->buffer, body->len);

    Json * json;
    json = new Json(jsonSource->buffer, strlen(jsonSource->buffer), strlen(jsonSource->buffer));

    if(!json->isValidJson())
    {
        tr_err ( "Invalid JSON: %s", jsonSource->buffer);
        ret = EXIT_FAILURE;
        goto _exit_info_parse;
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
        goto _exit_info_parse;
    }

    valueStart = json->tokenAddress(KeyIndex);
    valueLength = json->tokenLength(KeyIndex);

    if(parsed_body != NULL)
        delete parsed_body;
    parsed_body = new GW_BufLen((char *)valueStart, valueLength);

    //printf("Parsed info: %s \r\n\r\n", parsed_body->buffer);

    //for(i=0;i<json.parsedTokenCount();i++)
    //    json_token_info(json, i);

_exit_info_parse:
    delete json;
    delete jsonSource;

    return ret;
}

int GW_RestApi::rest_api(char * fmt, ...)
{
#define REST_API_ENDPOINT_MAX_LENGTH       512
    char buffer[REST_API_ENDPOINT_MAX_LENGTH] = {0};

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    if(endpoint != NULL)
        delete endpoint;
    endpoint = new GW_BufLen((char *)buffer);
    return get();
}

void GW_RestApi::params_set(GW_RestApi_params & cfg)
{
    if(cfg.network != NULL)
    {
        network = cfg.network;

        SocketAddress s;
        network->get_ip_address(&s);
        printf("IP address: %s\n", s.get_ip_address() ? s.get_ip_address() : "None");
    }

    if(cfg.server_url != NULL)
    {
        if(server_url != NULL)
            delete server_url;
        server_url = new GW_BufLen(cfg.server_url);
    }

    if(cfg.device_key != NULL)
    {
        if(device_key != NULL)
            delete device_key;
        device_key = new GW_BufLen(cfg.device_key);
    }

    if(cfg.id != NULL)
    {
        if(id != NULL)
            delete id;
        id = new GW_BufLen(cfg.id);
    }
}

void gw_https_init(void)
{
    m_RestApi = new GW_RestApi();
    m_FwuMgr->src_register(m_RestApi, 23);
    printf("Https init! \r\n");
}