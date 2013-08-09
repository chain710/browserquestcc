#include "worldserver.h"
#include "log_macro.h"
#include "msg_constant.h"
#include "simple_ptr.h"
#include "websocket_codec.h"
#include "http_parser.h"
#include "sha1.h"
#include "base64.h"

#include <string>
#include <string.h>
#include <stdio.h>

using namespace std;

//////////////////////////////////////////////////////////////////////////
// http parser callbacks

struct http_parser_ctx_t
{
    enum state
    {
        parse_none,
        parse_field, 
        parse_value,
    };

    http_parser_ctx_t() 
    {
        clear();
    }

    void clear()
    {
        parse_state_ = parse_none;
        msg_complete_ = 0;
        field_.clear();
        wskey_.clear();
    }

    int parse_state_;
    int msg_complete_;
    std::string field_;
    std::string wskey_;
};

int on_head_field(http_parser* p, const char *at, size_t length)
{
    http_parser_ctx_t* ctx = (http_parser_ctx_t*)p->data;
    if (ctx->parse_state_ != http_parser_ctx_t::parse_field)
    {
        ctx->parse_state_ = http_parser_ctx_t::parse_field;
        ctx->field_.clear();
    }

    ctx->field_.append(at, length);
    return 0;
}

int on_head_value(http_parser* p, const char *at, size_t length)
{
    http_parser_ctx_t* ctx = (http_parser_ctx_t*) p->data;
    if (ctx->parse_state_ != http_parser_ctx_t::parse_value)
    {
        ctx->parse_state_ = http_parser_ctx_t::parse_value;
        if (ctx->field_ == "Sec-WebSocket-Key")
        {
            ctx->wskey_.clear();
        }
    }
    
    if (ctx->field_ == "Sec-WebSocket-Key")
    {
        ctx->wskey_.append(at, length);
    }

    return 0;
}

int on_msg_complete(http_parser* p) 
{
    http_parser_ctx_t* ctx = (http_parser_ctx_t*)p->data;
    ctx->msg_complete_ = 1;
    return 0;
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// game entity filter
bool reject_by_id(const game_entity_t& e, void* up)
{
    int reject = *(int*)up;
    return e.id() != reject;
}

bool reject_by_player(const game_entity_t& e, void* up)
{
    return e.category() == game_entity_t::EC_PLAYER && 
        e.id() != *(int*)up;
}

//////////////////////////////////////////////////////////////////////////

// template <>
// size_t IterDataSize(const game_map_t::entity_map_t::iterator& iter)
// {
//     // asumption
//     return 4;
// }
// 
// template <>
// std::string IterData(const game_map_t::entity_map_t::iterator& iter)
// {
//     char tmp[16];
//     snprintf(tmp, sizeof(tmp), "%d", iter->second->id());
//     return string(tmp);
// }

template <>
size_t IterDataSize(const game_map_t::entity_list_t::iterator& iter)
{
    // asumption
    return 4;
}

template <>
std::string IterData(const game_map_t::entity_list_t::iterator& iter)
{
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%d", (*iter)->id());
    return string(tmp);
}


world_server_t::world_server_t( app_init_option opt ):
    app_opt_(opt)
{
}

void world_server_t::create_world_map( const map_config_t& config )
{
    map_.create_from(config);
}

int world_server_t::handle_message( const msgpack_context_t& ctx, const char* data, size_t size )
{
    int pack_len = (int)size - ctx.extrabuf_len_;
    if (pack_len < 0)
    {
        L_ERROR("invalid msg len, datalen:%d, extralen:%d", (int)size, ctx.extrabuf_len_);
        return -1;
    }

    if (pack_len > 0)
    {        
        const char* pack = &data[ctx.extrabuf_len_];
        if (is_client_ws_frame(pack, pack_len))
        {
            ws_frame_t frame;
            int ret = frame.from_bytes(pack, pack_len, ws_frame_t::PWF_UNMASK_PAYLOAD);
            if (ret < 0)
            {
                L_ERROR("parse websocket frame error %d", ret);
                return -1;
            }

            if (!frame.is_fin())
            {
                L_ERROR("fragment not supported yet!%s", "");
                return -1;
            }
            
            switch (frame.get_opcode())
            {
            case WFOP_TEXT:
                break;
            case WFOP_CLOSE:
                kick_player_out(ctx);
                return 0;
            default:
                L_WARN("ws opcode %d not implemented", frame.get_opcode());
                return -1;
            }
            

            // DEBUG ONLY, SHALL BE REMOVED
            L_DEBUG("incoming msg = %s", string(frame.get_payload(), frame.get_payload_len()).c_str());
            // END
            Json::Reader reader;
            Json::Value msg;
            if (!reader.parse(frame.get_payload(), frame.get_payload()+frame.get_payload_len(), msg))
            {
                L_ERROR("parse client msg error %s, raw msg is %s", 
                    reader.getFormatedErrorMessages().c_str(), 
                    string(frame.get_payload(), frame.get_payload_len()).c_str());
                return -1;
            }

            if (!validate_msg(msg))
            {
                return -1;
            }

            dispatch_msg(ctx, msg);
        }
        else
        {
            // asume http upgrade header
            handle_http_upgrade(ctx, pack, pack_len);
        }
    }


    if (ctx.flag_ & mpf_closed_by_peer)
    {
        handle_player_disconnect(ctx);
    }
    
    return 0;
}

void world_server_t::handle_tick()
{
    // TODO: timer check

    // check all active mob if target out of range?
}

bool world_server_t::validate_msg( const Json::Value& msg )
{
    if (!msg.isArray())
    {
        L_ERROR("msg should be ARRAY, but it's %d", msg.type());
        return false;
    }

    if (msg.size() <= 0)
    {
        L_ERROR("msg array is empty%s", "");
        return false;
    }

    if (!msg[0].isInt())
    {
        L_ERROR("expect ele[0] to be INT%s", "");
        return false;
    }

    int msg_id = msg[0].asInt();
    switch (msg_id)
    {
    case MSG_HELLO: return check_msg_format(msg, "snn");
    case MSG_WHO: 
        // all should be int
        for (Json::ArrayIndex i = 0; i < msg.size(); ++i)
        {
            if (!msg[i].isInt())
            {
                L_ERROR("expected param[%d] to be integer", (int)i);
                return false;
            }
        }
        
        return true;
    case MSG_ZONE: return true;
    case MSG_CHAT: return check_msg_format(msg, "s");
    case MSG_MOVE: return check_msg_format(msg, "nn");
    case MSG_LOOTMOVE: return check_msg_format(msg, "nnn");
    case MSG_AGGRO: return check_msg_format(msg, "n");
    case MSG_ATTACK: return check_msg_format(msg, "n");
    case MSG_HIT: return check_msg_format(msg, "n");
    case MSG_HURT: return check_msg_format(msg, "nnn");
    case MSG_LOOT: return check_msg_format(msg, "n");
    case MSG_TELEPORT: return check_msg_format(msg, "nn");
    case MSG_OPEN: return check_msg_format(msg, "n");
    case MSG_CHECK: return check_msg_format(msg, "n");
    default:
        L_ERROR("undefined msg id %d", msg_id);
        return false;
    }
}

bool world_server_t::check_msg_format( const Json::Value& msg, const char* format )
{
    int expected_param_size = strlen(format) + 1;

    if ((int)msg.size() != expected_param_size)
    {
        L_ERROR("expected %d params, but %d provided", expected_param_size, (int)msg.size());
        return false;
    }
    

    for (int i = 1; i < expected_param_size; ++i)
    {
        switch (format[i-1])
        {
        case 'n':
            if (!msg[i].isInt())
            {
                L_ERROR("param[%d] is not integer", i);
                return false;
            }
            
            break;
        case 's':
            if (!msg[i].isString())
            {
                L_ERROR("param[%d] is not string", i);
                return false;
            }

            break;
        default:
            L_WARN("undefined msg format %c", format[i-1]);
            break;
        }
    }
    
    return true;
}

int world_server_t::dispatch_msg( const msgpack_context_t& ctx, const Json::Value& msg )
{
    int msg_id = msg[0].asInt();
    switch (msg_id)
    {
    case MSG_HELLO: return handle_hello(ctx, msg);
    case MSG_WHO: return handle_query_entities(ctx, msg);
    case MSG_MOVE: return handle_player_move(ctx, msg);
    case MSG_TELEPORT: return handle_teleport(ctx, msg);
    case MSG_ZONE: return handle_zone_changed(ctx, msg);
    case MSG_LOOTMOVE: return handle_lootmove(ctx, msg);
    case MSG_LOOT: return handle_loot(ctx, msg);
    case MSG_CHECK: return handle_checkpoint(ctx, msg);
    case MSG_ATTACK: return handle_attack(ctx, msg);
    case MSG_HIT: return handle_hit(ctx, msg);
    case MSG_HURT: return handle_hurt(ctx, msg);
    case MSG_CHAT:
    case MSG_AGGRO:
    case MSG_OPEN:
    default: return handle_no_implemenation(ctx, msg);
    }
}

int world_server_t::handle_no_implemenation( const msgpack_context_t& ctx, const Json::Value& msg )
{
    int msg_id = msg[0].asInt();
    L_ERROR("msg %d handler not implemented yet!", msg_id);
    return -1;
}
//////////////////////////////////////////////////////////////////////////
// put all msg handler code here

int world_server_t::handle_http_upgrade( const msgpack_context_t& ctx, const char* data, size_t data_len )
{
    // TODO: init player data

    http_parser_ctx_t pctx;
    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = &pctx;

    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_header_field = on_head_field;
    settings.on_header_value = on_head_value;
    settings.on_message_complete = on_msg_complete;

    size_t nparsed = http_parser_execute(&parser, &settings, data, data_len);
    L_TRACE("parse %d http bytes, wskey is %s", (int)nparsed, pctx.wskey_.c_str());
    if (0 == pctx.msg_complete_ || 0 == parser.upgrade)
    {
        L_ERROR("parse http upgrade failed, complete=%d, upgrade=%d", 
            pctx.msg_complete_, (int)parser.upgrade);
        return -1;
    }
    
    pctx.wskey_.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    unsigned char accept_sha1[20];
    sha1_buffer(pctx.wskey_.c_str(), pctx.wskey_.length(), accept_sha1);
    string accept_base64 = base64_encode(accept_sha1, sizeof(accept_sha1));

    send_raw_fmt(ctx, "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n", 
        accept_base64.c_str());

    send_ws_text(ctx, "go");
    return 0;
}

int world_server_t::handle_hello( const msgpack_context_t& ctx, const Json::Value& msg )
{
    const string& nick = msg[1].asString();
    int armor = msg[2].asInt();
    int weapon = msg[3].asInt();
    L_DEBUG("player %s login, armor %d, weapon %d", nick.c_str(), armor, weapon);
    game_entity_t player_prototype;
    player_prototype.init(0, game_entity_t::WARRIOR, game_entity_t::EC_PLAYER);
    player_prototype.set_position(40, 209);

    player_t *p = dynamic_cast<player_t *>(map_.spawn(player_prototype));
    if (NULL == p)
    {
        L_ERROR("spawn player failed%s", "");
    	return -1;
    }

    bind_player_to_link(ctx, p->id());
    int playerid = p->id();
    p->reset_hp(80);
    p->set_armor(armor);
    p->set_weapon(weapon);
    p->set_nick(nick.c_str());
    p->update_msg_context(ctx);

    game_map_t::entity_list_t entities;
    //game_map_t::entity_filter_t reject_self_id(reject_by_id, &playerid);
    map_.get_adjacent_entities(p->x(), p->y(), game_map_t::entity_filter_t(reject_by_id, &playerid), &entities);

    // population [msgid, world, total]
    send_ws_text(ctx, "[%d,%d,200]", MSG_POPULATION, map_.population());
    // list [msgid, id1, id2...]
    if (!entities.empty())
    {
        // entity id list
        string id_list;
        JoinStringsIterator(entities.begin(), entities.end(), ",", &id_list);
        send_ws_text(ctx, "[%d,%s]", MSG_LIST, id_list.c_str());
    }
    
    // welcome [msgid, id, name, x, y, hp]
    send_ws_text(ctx, "[%d, %d, \"%s\", %d, %d, %d]", 
        MSG_WELCOME, playerid, nick.c_str(), 
        p->x(), p->y(), p->hitpoints());

    // TODO: broadcast population change to whole world
    // here is a workaround: broadcast list msg to nearby zones
    string_ptr entity_state;
    broadcast_nearby_players(*p, "[%d,%s]", MSG_SPAWN, p->get_state(entity_state));

    // send nearby spawn to player
    for (game_map_t::entity_list_t::iterator it = entities.begin(); it != entities.end(); ++it)
    {
        send_ws_text(ctx, "[%d,%s]", MSG_SPAWN, (*it)->get_state(entity_state));
    }
    
    return 0;
}

int world_server_t::handle_query_entities( const msgpack_context_t& ctx, const Json::Value& msg )
{
    L_TRACE("handle_query_entities deprecated!%s", "");
    // return entity info
    /*
    int ids_len = msg.size();
    int id;
    game_entity_t* entity;
    string_ptr entity_state;

    for (int i = 1; i < ids_len; ++i)
    {
        id = msg[i].asInt();
        L_DEBUG("player request entity %d info", id);
        entity = map_.get_entity(id);
        if (NULL == entity)
        {
            L_WARN("find no entity %d", id);
            continue;
        }
        
        send_ws_text(ctx, "[%d,%s]", MSG_SPAWN, entity->get_state(entity_state));
    }*/
    
    return 0;
}

int world_server_t::handle_player_move( const msgpack_context_t& ctx, const Json::Value& msg )
{
    int x = msg[1].asInt();
    int y = msg[2].asInt();
    
    player_t* p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_ERROR("find no player by link(%d)", ctx.link_ctx_);
        return -1;
    }
    
    L_DEBUG("player %d move to (%d,%d)", p->id(), x, y);
    game_map_t::move_events_t evts;
    int ret = map_.move_entity(*p, x, y, &evts);
    if (ret < 0)
    {
        // error, TODO: disconnect player
        return -1;
    }
    else
    {
        p->clear_target();
    }

    // walk leave event
    string_ptr msg_buf;
    process_player_movement_events(*p, evts, 
        pack_ws_text(msg_buf, "[%d,%d,%d,%d,0]", MSG_MOVE, p->id(), p->x(), p->y()));
    return 0;
}

int world_server_t::handle_checkpoint( const msgpack_context_t& ctx, const Json::Value& msg )
{
    // TODO: implement me
    int checkpoint_id = msg[1].asInt();
    player_t *p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_ERROR("find no player by link %d", ctx.link_ctx_);
        return -1;
    }
    
    L_DEBUG("player %d reach checkpoint %d", p->id(), checkpoint_id);
    return 0;
}

int world_server_t::handle_player_disconnect( const msgpack_context_t& ctx )
{
    player_t* p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_WARN("find no player by link(%d) when disconnect", ctx.link_ctx_);
        return 0;
    }

    L_DEBUG("player %d disconnected", p->id());
    remove_player(*p);
    return 0;
}

int world_server_t::handle_teleport( const msgpack_context_t& ctx, const Json::Value& msg )
{
    // TODO: handleplayervanish -> make mob forget player, return to spawning pos
    return handle_player_move(ctx, msg);
}

int world_server_t::handle_zone_changed( const msgpack_context_t& ctx, const Json::Value& msg )
{
    // TODO: push Destroy msg to adjacent zones. WHY?
    // push nearby entities info to player
    player_t *p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_ERROR("find no player by ctx %d", ctx.link_ctx_);
        return -1;
    }
    
    int playerid = p->id();
    //game_map_t::entity_map_t entities;
    //sgame_map_t::entity_filter_t reject_self_id(reject_by_id, &playerid);
    game_map_t::entity_list_t entities;
    //map_.get_adjacent_entities(p->x(), p->y(), &entities, &reject_self_id);
    map_.get_adjacent_entities(p->x(), p->y(), game_map_t::entity_filter_t(reject_by_id, &playerid), &entities);
    if (!entities.empty())
    {
        // entity id list
        string id_list;
        JoinStringsIterator(entities.begin(), entities.end(), ",", &id_list);
        send_ws_text(ctx, "[%d,%s]", MSG_LIST, id_list.c_str());
    }

    return 0;
}

int world_server_t::handle_lootmove( const msgpack_context_t& ctx, const Json::Value& msg )
{
    int x = msg[1].asInt();
    int y = msg[2].asInt();
    int id = msg[3].asInt();
    
    player_t* p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_ERROR("find no player by link %d", ctx.link_ctx_);
        return -1;
    }

    L_DEBUG("player %d move to (%d,%d) and loot %d", p->id(), x, y, id);
    
    // move to position anyway
    game_map_t::move_events_t evts;
    int ret = map_.move_entity(*p, x, y, &evts);
    if (ret < 0)
    {
        // TODO: disconnect?
        L_ERROR("player %d move to (%d,%d) error %d", p->id(), x, y, ret);
        return -1;
    }

    game_entity_t* item = map_.get_entity(id);
    if (NULL == item)
    {
        L_ERROR("find no item %d", id);
        return -1;
    }

    if (!item->is_in_pos(x, y))
    {
        L_ERROR("item %d not in (%d,%d)", id, x, y);
        return -1;
    }
    
    p->clear_target();

    string_ptr msg_buf;
    process_player_movement_events(*p, evts, pack_ws_text(msg_buf, "[%d,%d,%d]", MSG_LOOTMOVE, p->id(), id));

    // broadcast nearby zones lootmove
    //broadcast_nearby_players(*p, "[%d,%d,%d]", MSG_LOOTMOVE, p->id(), id);
    // TODO:movecallback (mob hate or something)
    return 0;
}

int world_server_t::handle_attack( const msgpack_context_t& ctx, const Json::Value& msg )
{
    int mob_id = msg[1].asInt();
    player_t *p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_ERROR("find no player by link %d", ctx.link_ctx_);
        return -1;
    }

    L_DEBUG("player %d attack mob %d", p->id(), mob_id);
    mob_t *mob = dynamic_cast<mob_t*>(map_.get_entity(mob_id));
    if (NULL == mob)
    {
        L_ERROR("mob %d doesnt exist", mob_id);
        return -1;
    }

    // set target
    map_.set_target(*p, mob_id);
    broadcast_nearby_players(*p, "[%d,%d,%d]", MSG_ATTACK, p->id(), mob_id);
    return 0;
}

int world_server_t::handle_hit( const msgpack_context_t& ctx, const Json::Value& msg )
{
    int mob_id = msg[1].asInt();
    player_t *p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_ERROR("find no player by link %d", ctx.link_ctx_);
        return -1;
    }

    L_DEBUG("player %d hit mob %d", p->id(), mob_id);
    mob_t *mob = dynamic_cast<mob_t*>(map_.get_entity(mob_id));
    if (NULL == mob)
    {
        L_ERROR("mob %d doesnt exist", mob_id);
        return -1;
    }

    int dmg = 50;

    mob->recv_dmg(dmg);
    int ret = on_character_hurt(*mob, *p, dmg);
    if (0 == ret)
    {
        // add mob to active list
        map_.set_target(*mob, p->id());
        broadcast_nearby_players(*mob, "[%d,%d,%d]", MSG_ATTACK, mob_id, p->id());
    }
    
    // mob recvdmg(-dmg)
    // handleMobHate
        // increase mob hate
        // player add hater
        // choose target if necessary(broadcast attacker)
    // handleHurtEntity
        // if playerhurt push health to player
        // if mob hurt push mob dmg to player
        // if entity die
            // broadcast despawn
            // player: clear haters... mob rechoose target
            // mob: push kill msg to player, broadcast drop item
            // remove it
    return 0;
}

int world_server_t::handle_hurt( const msgpack_context_t& ctx, const Json::Value& msg )
{
    int mob_id = msg[1].asInt();
    int mob_x = msg[2].asInt();
    int mob_y = msg[3].asInt();
    int ret;

    player_t *p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_ERROR("find no player by link %d", ctx.link_ctx_);
        return -1;
    }

    L_DEBUG("mob %d attack player %d at (%d, %d)", mob_id, p->id(), mob_x, mob_y);
    mob_t *mob = dynamic_cast<mob_t*>(map_.get_entity(mob_id));
    if (NULL == mob)
    {
        L_ERROR("mob %d doesnt exist", mob_id);
        return -1;
    }

    if (!mob->is_in_pos(mob_x, mob_y))
    {
        game_map_t::move_events_t evts;
        ret = map_.move_entity(*mob, mob_x, mob_y, &evts);
        if (ret < 0)
        {
            L_ERROR("move mob %d to %d error %d", mob_id, mob_x, mob_y);
            return -1;
        }

        process_mob_movement_events(*mob, evts);
    }

    const int damage = 1;
    p->recv_dmg(damage);
    ret = on_character_hurt(*p, *mob, damage);
    
    return 0;
}

int world_server_t::handle_loot( const msgpack_context_t& ctx, const Json::Value& msg )
{
    int loot_id = msg[1].asInt();
    player_t* p = get_player_by_link(ctx);
    if (NULL == p)
    {
        L_ERROR("find no player by link %d", ctx.link_ctx_);
        return -1;
    }

    L_DEBUG("player %d loot entity %d", p->id(), loot_id);
    game_entity_t* entity = map_.get_entity(loot_id);
    if (NULL == entity)
    {
        L_ERROR("find no entity %d", loot_id);
        return -1;
    }

    if (!entity->is_lootable())
    {
        L_ERROR("entity %d kind(%d) is not lootable", loot_id, entity->kind());
        return -1;
    }

    int kind = entity->kind();
    int category = entity->category();
    // broadcast despawn
    broadcast_nearby_players(*p, "[%d,%d]",MSG_DESPAWN, loot_id);
    // remove entity
    map_.destory_entity(loot_id);
    entity = NULL;

    switch (category)
    {
    case game_entity_t::EC_ARMOR:
        p->set_armor(loot_id);
        broadcast_nearby_players(*p, "[%d,%d,%d]", MSG_EQUIP, p->id(), kind);
        // TODO: modify hp
        break;
    case game_entity_t::EC_WEAPON:
        p->set_weapon(loot_id);
        broadcast_nearby_players(*p, "[%d,%d,%d]", MSG_EQUIP, p->id(), kind);
        break;
    case game_entity_t::EC_OBJECT:
        // if is healing potion push new health msg
        switch (kind)
        {
        case game_entity_t::FLASK:
            p->regen_hp(40);
            send_ws_text(ctx, "[%d,%d]", MSG_HEALTH, p->hitpoints());
            break;
        case game_entity_t::BURGER:
            p->regen_hp(100);
            send_ws_text(ctx, "[%d,%d]", MSG_HEALTH, p->hitpoints());
            break;
        case game_entity_t::CHEST:
        case game_entity_t::FIREPOTION:
            // FIREPOTION equip firefox armor for 15 sec and send new max hp msg
        case game_entity_t::CAKE:
            L_WARN("loot object kind %d not implemented yet!", kind);
            break;
        default:
            L_ERROR("undefined object kind %d", kind);
            break;
        }

        break;
    default:
        L_ERROR("undefined entity category %d", category);
        break;
    }

    return 0;
}

// end of msg handlers
//////////////////////////////////////////////////////////////////////////

int world_server_t::send_json( const msgpack_context_t& ctx, const Json::Value& data )
{
    Json::FastWriter writer;
    // ugly memcpy :(
    string json_str = writer.write(data);
    return send_ws_text(ctx, "%s", json_str.c_str());
}

int world_server_t::send_ws_text( const msgpack_context_t& ctx, const char* fmt, ... )
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vsend_ws_text(ctx, fmt, ap);
    va_end(ap);

    return ret;
}

int world_server_t::send_raw_fmt( const msgpack_context_t& ctx, const char* fmt, ... )
{
    string_ptr str;
    va_list ap;
    va_start(ap, fmt);
    int str_len = vformat_string(&str, 0, fmt, ap);
    va_end(ap);
    return send_raw(ctx, str.get(), str_len);
}

int world_server_t::vsend_ws_text( const msgpack_context_t& ctx, const char* fmt, va_list ap )
{
    string_ptr str;
    slice_t slice = vpack_ws_text(str, fmt, ap);
    if (slice.empty())
    {
        L_ERROR("vpack_ws_text error%s", "");
        return -1;
    }
    
    return send_raw(ctx, slice.data(), slice.size());
}

int world_server_t::send_raw( const msgpack_context_t& ctx, const char* data, size_t data_len )
{
    int ret = calypso_send_msgpack_by_ctx(app_opt_.msg_queue_, &ctx, data, data_len);
    if (ret < 0)
    {
        L_ERROR("send msg(%p, %zu) to link[%d] fd:%d error(%d)", data, data_len, ctx.link_ctx_, ctx.link_fd_, ret);
        return ret;
    }
    else
    {
        L_TRACE("send msg(%p, %zu) to link[%d] fd:%d succ", data, data_len, ctx.link_ctx_, ctx.link_fd_);
    }

    return ret;
}

player_t* world_server_t::get_player_by_link( const msgpack_context_t& ctx )
{
    map<int,int>::iterator iter = players_.find(ctx.link_ctx_);
    if (iter == players_.end())
    {
        return NULL;
    }
    
    return dynamic_cast<player_t*>(map_.get_entity(iter->second));
}

void world_server_t::remove_player( const player_t& player )
{
    // broadcast player despawn
    broadcast_nearby_players(player, "[%d,%d]", MSG_DESPAWN, player.id());

    map<int,int>::iterator iter = players_.find(player.msgctx().link_ctx_);
    if (iter == players_.end())
    {
        return;
    }

    map_.destory_entity(player.id());
    players_.erase(iter);
}

void world_server_t::bind_player_to_link( const msgpack_context_t& ctx, int player_id )
{
    map<int,int>::iterator iter = players_.find(ctx.link_ctx_);
    if (iter != players_.end() && iter->second != player_id)
    {
        L_WARN("find player %d bound to link(%d), MUST destory it first", iter->second, ctx.link_ctx_);
        player_t* p = get_player_by_link(ctx);
        if (p)
        {
            remove_player(*p);
        }
    }

    players_[ctx.link_ctx_] = player_id;
    L_DEBUG("player %d binds to link %d", player_id, ctx.link_ctx_);
}


void world_server_t::broadcast_nearby_players( const game_entity_t& src, const char* fmt, ... )
{
    int src_id = src.id();
    game_map_t::entity_list_t entities;
    map_.get_adjacent_entities(src.x(), src.y(), game_map_t::entity_filter_t(reject_by_player, &src_id), &entities);
    if (!entities.empty())
    {
        string_ptr str;
        va_list ap;
        va_start(ap, fmt);
        slice_t slice = vpack_ws_text(str, fmt, ap);
        va_end(ap);

        player_t *p;
        for (game_map_t::entity_list_t::const_iterator iter = entities.begin(); iter != entities.end(); ++iter)
        {
            p = dynamic_cast<player_t*>(*iter);
            if (NULL == p)
            {
                L_WARN("player %d is null, data might be corrupted", (*iter)->id());
                continue;
            }
            
            send_raw(p->msgctx(), slice.data(), slice.size());
        }
    }
}

slice_t world_server_t::pack_ws_text( string_ptr& dst, const char* fmt, ... )
{
    va_list ap;
    va_start(ap, fmt);
    slice_t ret = vpack_ws_text(dst, fmt, ap);
    va_end(ap);

    return ret;
}

slice_t world_server_t::vpack_ws_text( string_ptr& dst, const char* fmt, va_list ap )
{
    const int reserve_len = 128;
    int str_len = vformat_string(&dst, reserve_len, fmt, ap);
    if (str_len < 0)
    {
        L_ERROR("vformat_string error(%d)", str_len);
        return slice_t();
    }

    ws_frame_t frame;
    frame.set_opcode(WFOP_TEXT);
    frame.set_payload(str_len, NULL);
    L_DEBUG("pack websocket text \"%s\"", dst.get()+reserve_len);
    int ws_pack_len = frame.pack_head(dst.get(), reserve_len, ws_frame_t::PWF_MOVE_TO_END);
    if (ws_pack_len > reserve_len || ws_pack_len < 0)
    {
        L_ERROR("WTF! pack websocket frame overflow %d?", ws_pack_len);
        return slice_t();
    }

    return slice_t(dst.get()+reserve_len-ws_pack_len, ws_pack_len+str_len);
}

void world_server_t::kick_player_out( const msgpack_context_t& ctx )
{
    msgpack_context_t rsp = ctx;
    rsp.flag_ |= mpf_close_link;
    L_DEBUG("shutdown link %d", ctx.link_ctx_);
    handle_player_disconnect(ctx);
    int ret = calypso_send_msgpack_by_ctx(app_opt_.msg_queue_, &rsp, NULL, 0);
    if (ret < 0)
    {
        L_ERROR("send shutdown link msg failed %d", ret);
    }
}

int world_server_t::on_character_hurt( character_t& victim, character_t& attacker, int dmg )
{
    player_t* p;
    switch (victim.category())
    {
    case game_entity_t::EC_PLAYER:
        // push health to player
        p = dynamic_cast<player_t*>(&victim);
        send_ws_text(p->msgctx(), "[%d,%d]", MSG_HEALTH, victim.hitpoints());
        break;
    case game_entity_t::EC_MOB:
        // push mob dmg to player
        p = dynamic_cast<player_t*>(&attacker);
        send_ws_text(p->msgctx(), "[%d,%d,%d]", MSG_DAMAGE, victim.id(), dmg);
        break;
    default: break;
    }
    
    if (victim.is_dead())
    {
        switch (victim.category())
        {
        case game_entity_t::EC_PLAYER:
            // remove player
            remove_player(*p);
            attacker.clear_target();
            // TODO:mob choose another target

            // notify attacker move and disengage
            broadcast_nearby_players(attacker, "[%d,%d,%d,%d,0]", MSG_MOVE, attacker.id(), attacker.x(), attacker.y());
            break;
        case game_entity_t::EC_MOB:
            // push kill to player
            send_ws_text(p->msgctx(), "[%d,%d]", MSG_KILL, victim.kind());
            // broadcast despawn
            broadcast_nearby_players(victim, "[%d,%d]", MSG_DESPAWN, victim.id());
            // broadcast item drop
            
            // DONT BREAK, FALL THROUGHT HERE!
        default:
            map_.destory_entity(victim.id());
            return 1;
        }
    }

    return 0;
}

int world_server_t::on_player_leave_mob_region( player_t& player, mob_t& mob )
{
    L_DEBUG("player %d run out of mob %d's range", player.id(), mob.id());
    return 0;
}

void world_server_t::process_player_movement_events( const player_t& player, const game_map_t::move_events_t& evts, const slice_t& msg )
{
    string_ptr destroy_msgbuf, spawn_msgbuf;
    slice_t destroy_msg, spawn_msg;
    for (game_map_t::move_events_t::const_iterator i = evts.begin(); i != evts.end(); ++i)
    {
        if (game_entity_t::EC_PLAYER == i->entity_->category())
        {
            player_t* p = dynamic_cast<player_t*>(i->entity_);
            switch (i->evt_)
            {
            case region_tree_t::REVT_ENTER:
                if (spawn_msg.empty())
                {
                    string_ptr state_buf;
                    spawn_msg = pack_ws_text(spawn_msgbuf, "[%d,%s]", MSG_SPAWN, player.get_state(state_buf));
                }
                
                send_raw(p->msgctx(), spawn_msg.data(), spawn_msg.size());
                break;
            case region_tree_t::REVT_STAY:
                send_raw(p->msgctx(), msg.data(), msg.size());
                break;
            case region_tree_t::REVT_LEAVE:
                // send destory msg when leave, save client memory
                if (destroy_msg.empty())
                {
                    destroy_msg = pack_ws_text(destroy_msgbuf, "[%d,%d]", MSG_DESTROY, player.id());
                }
                
                send_raw(p->msgctx(), destroy_msg.data(), destroy_msg.size());
                break;
            default:
                L_ERROR("unknown player movement event %u", i->evt_);
                break;
            }
        }
        else if (game_entity_t::EC_MOB == i->entity_->category())
        {
            mob_t* mob = dynamic_cast<mob_t*>(i->entity_);
            if (i->evt_ == region_tree_t::REVT_LEAVE && mob->target() == player.id())
            {
                // TODO: broadcast nearby players(mob movement), give up chasing
                L_WARN("mob %d should clear target %d now!", mob->id(), player.id());
            }
        }

        // NOTE: how to prevent frequent leaving/entering?
        switch (i->evt_)
        {
        case region_tree_t::REVT_LEAVE:
            {
                string_ptr leave_msgbuf;
                slice_t leave_msg = pack_ws_text(leave_msgbuf, "[%d,%d]", MSG_DESTROY, i->entity_->id());
                send_raw(player.msgctx(), leave_msg.data(), leave_msg.size());
            }
            break;
        case region_tree_t::REVT_ENTER:
            {
                string_ptr enter_msgbuf, state_buf;
                slice_t enter_msg = pack_ws_text(enter_msgbuf, "[%d,%s]", MSG_SPAWN, i->entity_->get_state(state_buf));
                send_raw(player.msgctx(), enter_msg.data(), enter_msg.size());
            }
            break;
        default:
            break;
        }
    }
}

void world_server_t::process_mob_movement_events( const mob_t& mob, const game_map_t::move_events_t& evts )
{
    string_ptr move_msgbuf, destroy_msgbuf, spawn_msgbuf;
    slice_t move_msg = pack_ws_text(move_msgbuf, "[%d,%d,%d,%d,1]", MSG_MOVE, mob.id(), mob.x(), mob.y());
    slice_t destroy_msg;
    slice_t spawn_msg;

    for (game_map_t::move_events_t::const_iterator i = evts.begin(); i != evts.end(); ++i)
    {
        if (game_entity_t::EC_PLAYER == i->entity_->category())
        {
            player_t* p = dynamic_cast<player_t*>(i->entity_);
            switch (i->evt_)
            {
            case region_tree_t::REVT_ENTER:
                if (spawn_msg.empty())
                {
                    string_ptr state_buf;
                    spawn_msg = pack_ws_text(spawn_msgbuf, "[%d,%s]", MSG_SPAWN, mob.get_state(state_buf));
                }
                
                send_raw(p->msgctx(), spawn_msg.data(), spawn_msg.size());
                break;
            case region_tree_t::REVT_STAY:
                send_raw(p->msgctx(), move_msg.data(), move_msg.size());
                break;
            case region_tree_t::REVT_LEAVE:
                if (destroy_msg.empty())
                {
                    destroy_msg = pack_ws_text(destroy_msgbuf, "[%d,%d]", MSG_DESTROY, mob.id());
                }
                
                send_raw(p->msgctx(), destroy_msg.data(), destroy_msg.size());
                break;
            default:
                L_ERROR("unknown mob movement event %u", i->evt_);
                break;
            }
        }
    }
}
