#include "map_config.h"
#include "utility.h"
#include "log_macro.h"

#include <stdlib.h>
#include <json/json.h>
#include <string>

using namespace std;

static map_config_t::kind_category_item KIND_TO_CATEGORY[] = {
    {"warrior", game_entity_t::EC_PLAYER, game_entity_t::WARRIOR}, 

    {"rat", game_entity_t::EC_MOB, game_entity_t::RAT},
    {"skeleton", game_entity_t::EC_MOB, game_entity_t::SKELETON},
    {"goblin", game_entity_t::EC_MOB, game_entity_t::GOBLIN},
    {"ogre", game_entity_t::EC_MOB, game_entity_t::OGRE},
    {"spectre", game_entity_t::EC_MOB, game_entity_t::SPECTRE},
    {"deathknight", game_entity_t::EC_MOB, game_entity_t::DEATHKNIGHT},
    {"crab", game_entity_t::EC_MOB, game_entity_t::CRAB},
    {"snake", game_entity_t::EC_MOB, game_entity_t::SNAKE},
    {"bat", game_entity_t::EC_MOB, game_entity_t::BAT},
    {"wizard", game_entity_t::EC_MOB, game_entity_t::WIZARD},
    {"eye", game_entity_t::EC_MOB, game_entity_t::EYE},
    {"skeleton2", game_entity_t::EC_MOB, game_entity_t::SKELETON2},
    {"boss", game_entity_t::EC_MOB, game_entity_t::BOSS},

    {"sword1", game_entity_t::EC_WEAPON, game_entity_t::SWORD1},
    {"sword2", game_entity_t::EC_WEAPON, game_entity_t::SWORD2},
    {"axe", game_entity_t::EC_WEAPON, game_entity_t::AXE},
    {"redsword", game_entity_t::EC_WEAPON, game_entity_t::REDSWORD},
    {"bluesword", game_entity_t::EC_WEAPON, game_entity_t::BLUESWORD},
    {"goldensword", game_entity_t::EC_WEAPON, game_entity_t::GOLDENSWORD},
    {"morningstar", game_entity_t::EC_WEAPON, game_entity_t::MORNINGSTAR},

    {"firefox", game_entity_t::EC_ARMOR, game_entity_t::FIREFOX},
    {"clotharmor", game_entity_t::EC_ARMOR, game_entity_t::CLOTHARMOR},
    {"leatherarmor", game_entity_t::EC_ARMOR, game_entity_t::LEATHERARMOR},
    {"mailarmor", game_entity_t::EC_ARMOR, game_entity_t::MAILARMOR},
    {"platearmor", game_entity_t::EC_ARMOR, game_entity_t::PLATEARMOR},
    {"redarmor", game_entity_t::EC_ARMOR, game_entity_t::REDARMOR},
    {"goldenarmor", game_entity_t::EC_ARMOR, game_entity_t::GOLDENARMOR},

    {"flask", game_entity_t::EC_OBJECT, game_entity_t::FLASK},
    {"cake", game_entity_t::EC_OBJECT, game_entity_t::CAKE},
    {"burger", game_entity_t::EC_OBJECT, game_entity_t::BURGER},
    {"chest", game_entity_t::EC_OBJECT, game_entity_t::CHEST},
    {"firepotion", game_entity_t::EC_OBJECT, game_entity_t::FIREPOTION},

    {"guard", game_entity_t::EC_NPC, game_entity_t::GUARD},
    {"villagegirl", game_entity_t::EC_NPC, game_entity_t::VILLAGEGIRL},
    {"villager", game_entity_t::EC_NPC, game_entity_t::VILLAGER},
    {"coder", game_entity_t::EC_NPC, game_entity_t::CODER},
    {"scientist", game_entity_t::EC_NPC, game_entity_t::SCIENTIST},
    {"priest", game_entity_t::EC_NPC, game_entity_t::PRIEST},
    {"king", game_entity_t::EC_NPC, game_entity_t::KING},
    {"rick", game_entity_t::EC_NPC, game_entity_t::RICK},
    {"nyan", game_entity_t::EC_NPC, game_entity_t::NYAN},
    {"sorcerer", game_entity_t::EC_NPC, game_entity_t::SORCERER},
    {"agent", game_entity_t::EC_NPC, game_entity_t::AGENT},
    {"octocat", game_entity_t::EC_NPC, game_entity_t::OCTOCAT},
    {"beachnpc", game_entity_t::EC_NPC, game_entity_t::BEACHNPC},
    {"forestnpc", game_entity_t::EC_NPC, game_entity_t::FORESTNPC},
    {"desertnpc", game_entity_t::EC_NPC, game_entity_t::DESERTNPC},
    {"lavanpc", game_entity_t::EC_NPC, game_entity_t::LAVANPC},
};


map_config_t::map_config_t():
    width_(0),
    height_(0)
{
    // init kinds
    int kind_num = sizeof(KIND_TO_CATEGORY)/sizeof(kind_category_item);
    for (int i = 0; i < kind_num; ++i)
    {
        kinds_[KIND_TO_CATEGORY[i].kind_name_] = KIND_TO_CATEGORY[i];
    }
}

int map_config_t::load( const char* config_path )
{
    // TODO: read roamingAreas(mob areas), grid(map.collisions)
    Json::Reader reader;
    Json::Value root;

    string json_raw;
    int ret = read_all_text(config_path, json_raw);
    if (ret < 0)
    {
        L_ERROR("read from file %s failed(%d)", config_path, ret);
        return -1;
    }
    
    if (!reader.parse(json_raw, root))
    {
        L_ERROR("parse map data error %s", reader.getFormattedErrorMessages().c_str());
        return -1;
    }

    width_ = root.get("width", 0).asInt();
    height_ = root.get("height", 0).asInt();
    
    Json::Value& static_chests = root["staticChests"];
    Json::Value& static_entities = root["staticEntities"];
    Json::Value& mob_areas = root["roamingAreas"];
    Json::Value& collisions = root["collisions"];

    game_entity_t tmp;
    for (Json::Value::ArrayIndex i = 0; i < static_chests.size(); ++i)
    {
        tmp.init(0, game_entity_t::CHEST, game_entity_t::EC_OBJECT);
        tmp.set_position(static_chests[i].get("x", 0).asInt(), static_chests[i].get("y", 0).asInt());

        entities_.push_back(tmp);
    }

    Json::Value::Members members = static_entities.getMemberNames();
    int tile_idx, x, y;
    const kind_category_item* kind_info;
    const char* entity_name;
    for (Json::Value::Members::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        entity_name = static_entities[*it].asCString();
        kind_info = get_kind_info(entity_name);
        if (NULL == kind_info)
        {
            L_ERROR("find no kind %s", entity_name);
            continue;
        }

        tile_idx = strtol(it->c_str(), NULL, 10);
        tileidx2coord(tile_idx, &x, &y);
        tmp.init(0, kind_info->kind_, kind_info->category_);
        tmp.set_position(x, y);

        entities_.push_back(tmp);
    }

    for (Json::Value::ArrayIndex i = 0; i < collisions.size(); ++i)
    {
        collisions_.insert(collisions[i].asInt());
    }

    // mob spawning area
    for (Json::Value::ArrayIndex i = 0; i < mob_areas.size(); ++i)
    {
        const char* type = mob_areas[i]["type"].asCString();
        kind_info = get_kind_info(type);
        if (NULL == kind_info)
        {
            L_ERROR("find no kind info by %s", type);
            continue;
        }
        
        mob_areas_.push_back(spawning_area_t(
            mob_areas[i].get("x", 0).asInt(), 
            mob_areas[i].get("y", 0).asInt(),
            mob_areas[i].get("width", 0).asInt(),
            mob_areas[i].get("height", 0).asInt(), 
            kind_info->kind_, 
            kind_info->category_));
    }
    
    L_DEBUG("load %zu entities", entities_.size());
    return 0;
}

void map_config_t::tileidx2coord( int tile_idx, int* x, int* y )
{
    // x starts from 1, y starts from 0, tile_idx starts from 1
    if (x)
    {
        if (tile_idx <= 0)
        {
            *x = 1;
        }
        else
        {
            *x = 1 + ((tile_idx - 1) % width_);
        }
    }
    
    if (y)
    {
        * y = (tile_idx - 1) / width_;
    }
}

const map_config_t::kind_category_item* map_config_t::get_kind_info( const char* name ) const
{
    kind_map_t::const_iterator it = kinds_.find(name);
    if (it == kinds_.end())
    {
        return NULL;
    }

    return &it->second;
}
