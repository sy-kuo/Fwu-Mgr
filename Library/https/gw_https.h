#ifndef __GW_HTTPS_H__
#define __GW_HTTPS_H__

#include "mbed.h"

#include "Json.h"
#include "gw_tools.h"
#include "gw_fwu.h"

#define SRC_HTTPS_VERSION       0x1111
#define SRC_HTTPS_MAX_LENGTH    0x2000

#define CLASS_NAME              "Https"

typedef enum {
    REST_API_SUCCESSFUL,
    REST_API_FAILED
} REST_API_E;

class GW_RestApi_params
{
public:
    NetworkInterface * network;
    char * server_url;
    char * device_key;
    char * id;
};

class GW_RestApi_buffer
{
public:
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t length;
    uint32_t size;
    char * buffer;
    uint8_t is_running;
};

class GW_RestApi: public GW_FwuMethod
{
public:
    GW_RestApi(){
        server_url = NULL;
        device_key = NULL;
        id = NULL;
        endpoint = NULL;
        endpoint_var = NULL;
        parsed_body = NULL;
        bytes_range = NULL;
        body = NULL;
        sitefiles_id = NULL;
    }
    ~GW_RestApi(){
        delete server_url;
        delete device_key;
        delete id;
        delete endpoint;
        delete endpoint_var;
        delete parsed_body;
        delete bytes_range;
        delete body;
        delete sitefiles_id;
    }

    int body_parse(char * key[], int index);
    void params_set(GW_RestApi_params & cfg);
    int rest_api(char * fmt, ...);

    virtual int checkout(void * params) override;
    virtual int prepare(void * params) override;
    virtual int copy(uint32_t start_addr, uint32_t length) override;
    virtual int finish(void) override;
private:
    bool is_debugging;
    char * class_id = (char *)CLASS_NAME;
    uint32_t file_addr, file_len, content_size;
    GW_Role_Basic fwu_res;
    NetworkInterface * network;
    GW_BufLen * server_url;
    GW_BufLen * device_key;
    GW_BufLen * id;
    GW_BufLen * endpoint;
    GW_BufLen * endpoint_var;
    GW_BufLen * parsed_body;
    GW_BufLen * bytes_range;
    GW_BufLen * body;
    GW_BufLen * sitefiles_id;

    int get();
    void get_all_images(void);
    void json_token_info(Json& json, int index);
    void get_endpoint(char * p_cfg);
};

void gw_https_init(void);
extern GW_RestApi * m_RestApi;
#endif
