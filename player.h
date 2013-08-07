#ifndef _PLAYER_H_
#define _PLAYER_H_
#include <app_interface.h>
#include <string>
#include "entity.h"

class player_t:public character_t
{
public:
    player_t();
    explicit player_t(const game_entity_t& e);
    virtual char* get_state(string_ptr& buf) const;
    void update_msg_context(const msgpack_context_t &ctx) { msgctx_ = ctx; }
    void set_armor(int a) { armor_ = a; }
    void set_weapon(int w) { weapon_ = w; }
    int armor() const { return armor_; }
    int weapon() const { return weapon_; }
    void set_nick(const char* nick) { nick_ = nick? nick: ""; }
    const msgpack_context_t& msgctx() const { return msgctx_; }
    msgpack_context_t& msgctx() { return msgctx_; }
private:
    std::string nick_;
    int armor_;
    int weapon_;
    int max_hp_;
    int hitpoints_;
    msgpack_context_t msgctx_;

    // TODO: haters
};

#endif
