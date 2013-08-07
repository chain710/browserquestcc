#include "log_macro.h"
#include "websocket_codec.h"
#include "worldserver.h"
#include "utility.h"
#include "app_interface.h"

#include <string.h>
#include <string>
#include <json/json.h>

using namespace std;
static map_config_t _map_config;

//////////////////////////////////////////////////////////////////////////
// utility

int find_html_header_end(const char* in, size_t in_len)
{
    // find from rear
    const char* HEADER_END = "\r\n\r\n";
    if (NULL == in || in_len < 4)
    {
        return -1;
    }

    // search from end
    unsigned int end;
    memcpy(&end, HEADER_END, 4);
    for (int i = (int)in_len - 4; i >= 0; --i)
    {
        unsigned int cmp = * (unsigned int*)&in[i];
        if (cmp == end)
        {
            return i;
        }
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// application entry

void* app_initialize( app_init_option opt, const char* bootstrap_config )
{
    world_server_t* server = new world_server_t(opt);
    if (server)
    {
        server->create_world_map(_map_config);
    }
    
    return server;
}

void app_finalize( void* app_inst )
{
    world_server_t* server = (world_server_t*) app_inst;
    delete server;
}

void app_handle_tick( void* app_inst )
{
    world_server_t* server = (world_server_t*) app_inst;
    server->handle_tick();
}

int app_get_msgpack_size( const msgpack_context_t* ctx, const char* data, size_t size, char* extrabuf, size_t* extrabuf_len )
{
    if (extrabuf_len)
    {
        *extrabuf_len = 0;
    }
    
    int ret;
    if (is_client_ws_frame(data, size))
    {
        ret = get_ws_frame_len(data, size);
        if (ret < 0)
        {
            L_ERROR("get_ws_frame_len error %d", ret);
            return -1;
        }

        return ret;
    }

    // TODO: limit http head size
    // html header
    ret = find_html_header_end(data, size);
    if (ret < 0)
    {
        return 0;
    }

    L_DEBUG("find full http head, len=%d", ret+4);
    return ret + 4;
}

int app_handle_msgpack( void* app_inst, const msgpack_context_t* ctx, const char* data, size_t size )
{
    world_server_t* server = (world_server_t*) app_inst;
    return server->handle_message(*ctx, data, size);
}

app_handler_t get_app_handler()
{
    app_handler_t h;
    memset(&h, 0, sizeof(h));
    h.init_ = app_initialize;
    h.fina_ = app_finalize;
    h.get_msgpack_size_ = app_get_msgpack_size;
    h.handle_msgpack_ = app_handle_msgpack;
    h.handle_tick_ = app_handle_tick;
    return h;
}
int app_global_init(const char* bootstrap_config)
{
    L_TRACE("app_global_init%s", "");

    string json_raw;
    int ret = read_all_text(bootstrap_config, json_raw);
    if (ret < 0)
    {
        L_ERROR("read text from %s error(%d)", bootstrap_config, ret);
        return -1;
    }
    
    Json::Reader reader;
    Json::Value conf;
    if (!reader.parse(json_raw, conf))
    {
        L_ERROR("parse bootstrap config error %s", reader.getFormattedErrorMessages().c_str());
        return -1;
    }
    
    ret = _map_config.load(conf.get("map", "map.json").asCString());
    if (ret < 0)
    {
        L_ERROR("load map data error(%d)", ret);
        return -1;
    }
    
    L_INFO("map data load succesfully!%s", "");
    return 0;
}

void app_global_reload()
{
    L_TRACE("app_global_reload%s", "");
}

void app_global_fina()
{
    L_TRACE("app_global_fina%s", "");
}
//////////////////////////////////////////////////////////////////////////
