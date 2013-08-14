#ifndef _MAP_CONFIG_H_
#define _MAP_CONFIG_H_

#include <vector>
#include <map>
#include <string>
#include <set>
#include "entity.h"

struct spawning_area_t
{
    spawning_area_t():
        x_(0), y_(0), width_(0), height_(0), kind_(0), category_(0)
    {
    }
    spawning_area_t(int x, int y, int w, int h, int k, int c):
        x_(x), y_(y), width_(w), height_(h), kind_(k), category_(c)
    {
    }
    int x_;
    int y_;
    int width_;
    int height_;
    int kind_;
    int category_;
};

class map_config_t
{
public:
    struct kind_category_item
    {
        const char* kind_name_;
        int category_;
        int kind_;
    };

    typedef std::vector<game_entity_t> entities_t;
    typedef std::map<std::string, kind_category_item> kind_map_t;
    typedef std::vector<spawning_area_t> spawning_areas_t;
    typedef std::set<int> collision_tileidxs_t;

    map_config_t();
    int load(const char* config_path);
    int width() const { return width_; }
    int height() const { return height_; }
    // tile_idx startswith 1, (x,y) startswith (0,0)
    void tileidx2coord(int tile_idx, int* x, int* y);
    const entities_t& get_entities() const { return entities_; }
    const collision_tileidxs_t& get_collisions() const { return collisions_; }
    const spawning_areas_t& get_spawning_areas() const { return mob_areas_; }
private:
    const map_config_t::kind_category_item* get_kind_info(const char* name) const;

    entities_t entities_;
    kind_map_t kinds_;
    collision_tileidxs_t collisions_;
    int width_;
    int height_;

    spawning_areas_t mob_areas_;
};

#endif

