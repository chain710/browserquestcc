#include "player.h"
#include <stdio.h>

player_t::player_t():
    nick_(""),
    armor_(0), 
    weapon_(0),
    max_hp_(0),
    hitpoints_(0)
{
    memset(&msgctx_, 0, sizeof(msgctx_));
}

player_t::player_t( const game_entity_t& e ):
    character_t(e),
    nick_(""),
    armor_(0), 
    weapon_(0),
    max_hp_(0),
    hitpoints_(0)
{
    memset(&msgctx_, 0, sizeof(msgctx_));
}

char* player_t::get_state( string_ptr& buf ) const
{
    if (has_target())
    {
        format_string(&buf, 0, "%d,%d,%d,%d,\"%s\",%d,%d,%d,%d", 
            id(), kind(), x(), y(), nick_.c_str(), orientation(), armor_, weapon_, target());
    }
    else
    {
        format_string(&buf, 0, "%d,%d,%d,%d,\"%s\",%d,%d,%d", 
            id(), kind(), x(), y(), nick_.c_str(), orientation(), armor_, weapon_);
    }
    return buf.get();
}
