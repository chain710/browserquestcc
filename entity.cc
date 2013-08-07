#include "entity.h"
#include <stdio.h>

game_entity_t::game_entity_t():
    id_(0),
    kind_(0), 
    category_(0), 
    x_(0), 
    y_(0)
{
}

game_entity_t::game_entity_t( const game_entity_t& c ):
    id_(c.id_), 
    kind_(c.kind_),
    category_(c.category_),
    x_(c.x_),
    y_(c.y_)
{

}

void game_entity_t::set_position( int x, int y )
{
    x_ = x;
    y_ = y;
}

char* game_entity_t::get_state( string_ptr& buf ) const
{
    format_string(&buf, 0, "%d,%d,%d,%d", id_, kind_, x_, y_);
    return buf.get();
}

void game_entity_t::init( int id, int kind, int category )
{
    id_ = id;
    kind_ = kind;
    category_ = category;
}

bool game_entity_t::is_lootable()
{
    return game_entity_t::EC_WEAPON == category_
        || game_entity_t::EC_ARMOR == category_
        || game_entity_t::EC_OBJECT == category_;
}


char* character_t::get_state( string_ptr& buf ) const
{
    if (has_target())
    {
        format_string(&buf, 0, "%d,%d,%d,%d,%d,%d", id(), kind(), x(), y(), orientation_, target_);
    }
    else
    {
        format_string(&buf, 0, "%d,%d,%d,%d,%d", id(), kind(), x(), y(), orientation_);
    }
    
    
    return buf.get();
}
