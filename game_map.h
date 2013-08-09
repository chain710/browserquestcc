#ifndef _GAME_MAP_H_
#define _GAME_MAP_H_

#include "entity.h"
#include "map_config.h"
#include <map>
#include <math.h>
#include <set>
#include "region_tree.h"

// map for world server, actually a resource manager
// spawn entities(mob, npc, player, item, chest)

typedef bool(*filter_func_t)(const game_entity_t&, void*);

class game_map_t
{
public:
    typedef std::map<int, game_entity_t*> entity_map_t;
    typedef std::map<int, entity_map_t> entities_map_t;
    typedef region_tree_t::move_events_t move_events_t;
    typedef region_tree_t::entities_t entity_list_t;
    typedef region_tree_t::neighbour_filter_t entity_filter_t;

    game_map_t();
    ~game_map_t();
    // create map from config
    void create_from(const map_config_t& map);
    // destory entity
    int destory_entity(int id);
    // check if valid coord
    bool is_valid_coord(int x, int y);
    // spawn
    game_entity_t* spawn(const game_entity_t& e);
    // move entity, return <0 means error, =0 means ok and stays in the zone, >0 means move to new zone
    int move_entity(game_entity_t& e, int x, int y, move_events_t* evts);
    // get entity by id
    game_entity_t* get_entity(int id);
    // get entities in group
    int generate_id() { return ++max_id_; }
    void tileidx2coord( int tile_idx, int* x, int* y ) const;
    // get nearby entities
    void get_adjacent_entities(int x, int y, entity_filter_t filter, entity_list_t* out);
    int population() const { return population_; }
    void set_target(character_t& e, int target);
    void set_move_event_radius(int r) { event_radius_ = r; }
    // TODO: check all active mob
private:
    // entities id->entity
    entity_map_t entities_;
    region_tree_t zone_;
    // active mob list? ai/actions
    std::map<int, mob_t*> active_mobs_;


    int max_id_;

    int map_w_;
    int map_h_;
    int zone_h_num_;
    int zone_v_num_;

    int population_;

    int event_radius_;
};

#endif
