#include "game_map.h"
#include "player.h"
#include "log_macro.h"
#include "worldserver.h"
#include <set>

using namespace std;

static const int ZONE_WIDTH = 28;
static const int ZONE_HEIGHT = 12;

//////////////////////////////////////////////////////////////////////////

bool is_overlap(int x, int y, const region_t& region, void* up)
{
    int *radius = (int*)up;
    float rmx = region.left_ + (float)(region.width_)/2;
    float rmy = region.top_ + (float)(region.height_)/2;
    float w = *radius+(float)(region.width_)/2;
    float h = *radius+(float)(region.height_)/2;
    return (fabs(rmx-x) <= w && fabs(rmy-y) <= h);
}

//////////////////////////////////////////////////////////////////////////

game_map_t::game_map_t():
    max_id_(0),
    map_w_(0),
    map_h_(0),
    zone_h_num_(0),
    zone_v_num_(0),
    population_(0)
{

}

game_map_t::~game_map_t()
{
    for (entity_map_t::iterator it = entities_.begin(); it != entities_.end(); ++it)
    {
        delete it->second;
        it->second = NULL;
    }

    entities_.clear();
}

void game_map_t::create_from( const map_config_t& map )
{
    map_w_ = map.width();
    map_h_ = map.height();

    zone_h_num_ = map_w_ / ZONE_WIDTH;
    if (map_w_ % ZONE_WIDTH > 0) ++zone_h_num_;

    zone_v_num_ = map_h_ / ZONE_HEIGHT;
    if (map_h_ % ZONE_HEIGHT > 0) ++zone_v_num_;

    zone_.init(map_w_, map_h_, 16, 4, 4);
    set_move_event_radius(15);

    const map_config_t::entities_t &e = map.get_entities();
    for (map_config_t::entities_t::const_iterator it = e.begin(); it != e.end(); ++it)
    {
        spawn(*it);
    }

    L_DEBUG("create map %d*%d, zone %d*%d, spawn %zu entities", 
        map_w_, map_h_, zone_h_num_, zone_v_num_, entities_.size());
}

game_entity_t* game_map_t::spawn( const game_entity_t& e )
{
    simple_ptr<game_entity_t> new_entity;
    //game_entity_t *new_entity = NULL;
    switch (e.category())
    {
    case game_entity_t::EC_PLAYER:
        new_entity.reset(new player_t(e), 1);
        if (new_entity.get()) ++population_;
        break;
    case game_entity_t::EC_MOB:
        new_entity.reset(new mob_t(e), 1);
        break;
    case game_entity_t::EC_WEAPON:
    case game_entity_t::EC_ARMOR:
    case game_entity_t::EC_OBJECT:
        new_entity.reset(new game_entity_t(e), 1);
        break;
    case game_entity_t::EC_NPC:
        new_entity.reset(new character_t(e), 1);
        break;
    default:
        L_ERROR("undefined entity category %d", e.category());
        return NULL;
    }
    
    if (NULL == new_entity.get())
    {
        L_ERROR("create entity(%d) ret NULL", e.category());
        return NULL;
    }
    
    new_entity.get()->set_id(generate_id());

    if (entities_.end() != entities_.find(new_entity.get()->id()))
    {
        L_ERROR("confict entity id %d, can not spawn", new_entity.get()->id());
        return NULL;
    }

    int ret = zone_.insert(new_entity);
    if (ret < 0)
    {
        L_ERROR("insert entity %d to map error %d", new_entity.get()->id(), ret);
        return NULL;
    }
    else
    {
        entities_[new_entity.get()->id()] = new_entity.get();
    }

    return new_entity.release();
}

bool game_map_t::is_valid_coord( int x, int y )
{
    return x >= 0 && x < map_w_
        && y >= 0 && y < map_h_;
}

int game_map_t::move_entity( game_entity_t& e, int x, int y, move_events_t* evts)
{
    // TODO: check collision
    return zone_.move(&e, x, y, region_tree_t::overlap_detector_t(is_overlap, &event_radius_), evts);
}

void game_map_t::tileidx2coord( int tile_idx, int* x, int* y ) const
{
    if (x)
    {
        if (tile_idx <= 0)
        {
            *x = 0;
        }
        else
        {
            *x = (tile_idx - 1) % map_w_;
        }
    }

    if (y)
    {
        * y = (tile_idx - 1) / map_w_;
    }
}

void game_map_t::get_adjacent_entities( int x, int y, entity_filter_t filter, entity_list_t* out )
{
    zone_.get_neighbours(x, y, region_tree_t::overlap_detector_t(is_overlap, &event_radius_), filter, out);
}

game_entity_t* game_map_t::get_entity( int id )
{
    entity_map_t::iterator iter = entities_.find(id);
    if (iter == entities_.end())
    {
        return NULL;
    }

    return iter->second;
}

int game_map_t::destory_entity( int id )
{
    entity_map_t::iterator iter = entities_.find(id);
    if (iter == entities_.end())
    {
        L_WARN("find no entity %d", id);
        return -1;
    }

    if (iter->second->category() == game_entity_t::EC_PLAYER)
    {
        --population_;
    }

    int ret = zone_.remove(iter->second);
    if (ret < 0)
    {
        L_WARN("remove entity %d from map failed", id);
    }
    

    delete iter->second;
    iter->second = NULL;
    entities_.erase(iter);
    return 0;
}

void game_map_t::set_target( character_t& e, int target )
{
    e.set_target(target);
    if (game_entity_t::EC_MOB == e.category())
    {
        // add mob to active list
        active_mobs_[e.id()] = dynamic_cast<mob_t*>(&e);
    }
}
