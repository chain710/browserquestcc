#ifndef _MAP_CONFIG_H_
#define _MAP_CONFIG_H_

#include <vector>
#include <map>
#include <string>
#include "entity.h"

struct coordinate_t
{
    coordinate_t(): x_(0), y_(0) {}
    coordinate_t(int x, int y): x_(x), y_(y) {}
    int x_;
    int y_;
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

    map_config_t();
    int load(const char* config_path);
    int width() const { return width_; }
    int height() const { return height_; }
    // tile_idx startswith 1, (x,y) startswith (0,0)
    void tileidx2coord(int tile_idx, int* x, int* y);
    const entities_t& get_entities() const { return entities_; }
private:
    const map_config_t::kind_category_item* get_kind_info(const char* name) const;

    entities_t entities_;
    kind_map_t kinds_;
    int width_;
    int height_;
};

#endif

