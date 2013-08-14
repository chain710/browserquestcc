#ifndef _WORLD_SERVER_H_
#define _WORLD_SERVER_H_

/*
 *	WORLD CONFIG
 * mapinfo
 */
#include <json/json.h>
#include <app_interface.h>
#include <string>
#include <map>
#include <stdarg.h>
#include "game_map.h"
#include "player.h"
#include "slice.h"
#include "string_helper.h"

class world_server_t
{
public:
    world_server_t(app_init_option opt);
    void create_world_map(const map_config_t& config);
    int handle_message(const msgpack_context_t& ctx, const char* data, size_t size);
    void handle_tick();

private:

    //////////////////////////////////////////////////////////////////////////
    // msg handler

    int handle_http_upgrade(const msgpack_context_t& ctx, const char* data, size_t data_len);

    int handle_player_disconnect(const msgpack_context_t& ctx);

    int dispatch_msg(const msgpack_context_t& ctx, const Json::Value& msg);

    // handle hello
    int handle_hello(const msgpack_context_t& ctx, const Json::Value& msg);
    // handle who: list entities
    int handle_query_entities(const msgpack_context_t& ctx, const Json::Value& msg);
    // player move
    int handle_player_move(const msgpack_context_t& ctx, const Json::Value& msg);
    // client checkpoint
    int handle_checkpoint(const msgpack_context_t& ctx, const Json::Value& msg);
    // teleport
    int handle_teleport(const msgpack_context_t& ctx, const Json::Value& msg);
    // player's zone changed
    int handle_zone_changed(const msgpack_context_t& ctx, const Json::Value& msg);
    // player engage
    int handle_attack(const msgpack_context_t& ctx, const Json::Value& msg);
    // player hit mob
    int handle_hit(const msgpack_context_t& ctx, const Json::Value& msg);
    // handle lootmove
    int handle_lootmove(const msgpack_context_t& ctx, const Json::Value& msg);
    // player loot item
    int handle_loot(const msgpack_context_t& ctx, const Json::Value& msg);
    // mob attack player
    int handle_hurt(const msgpack_context_t& ctx, const Json::Value& msg);
    // player chat msg
    int handle_chat(const msgpack_context_t& ctx, const Json::Value& msg);
    // open chest
    int handle_open_chest(const msgpack_context_t& ctx, const Json::Value& msg);
    // mob hate
    int handle_aggro(const msgpack_context_t& ctx, const Json::Value& msg);
    // just a stub
    int handle_no_implemenation(const msgpack_context_t& ctx, const Json::Value& msg);

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // msg helpers

    // player run out of mob's region
    int on_player_leave_mob_region(player_t& player, mob_t& mob);

    // send msg to stay/enter entities, clear leave entity's target
    void process_player_movement_events(const player_t& player, const game_map_t::move_events_t& evts, const slice_t& msg);
    void process_mob_movement_events(const mob_t& mob, const game_map_t::move_events_t& evts);

    // drop item at x,y
    void drop_item_at( int category, int kind, int x, int y );

    // broadcast player movement to adjacent zones
    void broadcast_nearby_players(const game_entity_t& src, const char* fmt, ...);
    void broadcast_region_players( const game_entity_t& src, const char* fmt, ... );
    void vbroadcast_to_entities( const game_map_t::entity_list_t& entities, const char* fmt, va_list ap );

    //////////////////////////////////////////////////////////////////////////
    // return 1 if entity is dead, 0 if ok, <0 if error
    int on_character_hurt( character_t& entity, character_t& attacker, int dmg );
    int on_player_move( player_t& player, int x, int y );
    // check if msg valid
    bool validate_msg(const Json::Value& msg);
    // check msg data field type
    bool check_msg_format(const Json::Value& msg, const char* format);
    // send json data to client
    int send_json(const msgpack_context_t& ctx, const Json::Value& data);
    // send string data with wshead to client
    int send_ws_text(const msgpack_context_t& ctx, const char* fmt, ...);
    int vsend_ws_text(const msgpack_context_t& ctx, const char* fmt, va_list ap);
    slice_t vpack_ws_text( string_ptr& dst, const char* fmt, va_list ap );
    slice_t pack_ws_text( string_ptr& dst, const char* fmt, ... );
    // send raw data to client
    int send_raw_fmt(const msgpack_context_t& ctx, const char* fmt, ...);
    int send_raw(const msgpack_context_t& ctx, const char* data, size_t data_len);

    player_t* get_player_by_link(const msgpack_context_t& ctx);
    void remove_player(const player_t& player);
    void bind_player_to_link(const msgpack_context_t& ctx, int player_id);
    void kick_player_out(const msgpack_context_t& ctx);

    int capacity_;  // how many players
    game_map_t map_;
    app_init_option app_opt_;
    // linkctx to playerid
    std::map<int, int> players_;

    time_t last_spawn_mob_time_;
    time_t now_;
};

#endif
