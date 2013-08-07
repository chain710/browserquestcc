#ifndef _GAME_ENTITY_H_
#define _GAME_ENTITY_H_

#include <string.h>
#include "string_helper.h"

class game_entity_t
{
public:
    enum entity_category_t
    {
        EC_PLAYER = 0,
        EC_MOB = 1,
        EC_WEAPON = 2,
        EC_ARMOR = 3,
        EC_OBJECT = 4,
        EC_NPC = 5,
    };

    enum entity_kind_t
    {
        WARRIOR = 1,

        // Mobs
        RAT = 2,
        SKELETON = 3,
        GOBLIN = 4,
        OGRE = 5,
        SPECTRE = 6,
        CRAB = 7,
        BAT = 8,
        WIZARD = 9,
        EYE = 10,
        SNAKE = 11,
        SKELETON2 = 12,
        BOSS = 13,
        DEATHKNIGHT = 14,

        // Armors
        FIREFOX = 20,
        CLOTHARMOR = 21,
        LEATHERARMOR = 22,
        MAILARMOR = 23,
        PLATEARMOR = 24,
        REDARMOR = 25,
        GOLDENARMOR = 26,

        // Objects
        FLASK = 35,
        BURGER = 36,
        CHEST = 37,
        FIREPOTION = 38,
        CAKE = 39,

        // NPCs
        GUARD = 40,
        KING = 41,
        OCTOCAT = 42,
        VILLAGEGIRL = 43,
        VILLAGER = 44,
        PRIEST = 45,
        SCIENTIST = 46,
        AGENT = 47,
        RICK = 48,
        NYAN = 49,
        SORCERER = 50,
        BEACHNPC = 51,
        FORESTNPC = 52,
        DESERTNPC = 53,
        LAVANPC = 54,
        CODER = 55,

        // Weapons
        SWORD1 = 60,
        SWORD2 = 61,
        REDSWORD = 62,
        GOLDENSWORD = 63,
        MORNINGSTAR = 64,
        AXE = 65,
        BLUESWORD = 66,
    };

    game_entity_t();
    game_entity_t(const game_entity_t& c);
    virtual ~game_entity_t() {}
    void set_id(int id) { id_ = id; }
    void init(int id, int kind, int category);
    void set_position(int x, int y);
    virtual char* get_state(string_ptr& buf) const;

    int id() const { return id_; }
    int kind() const { return kind_; }
    int category() const { return category_; }
    int x() const { return x_; }
    int y() const { return y_; }
    bool is_in_pos(int x, int y) const { return x_ == x && y_ == y; }
    bool is_lootable();
private:
    int id_;
    short kind_;
    short category_;  // npc, mob, player .etc
    int x_;
    int y_;
};

class character_t: public game_entity_t
{
public:
    enum orientation_t
    {
        UP = 1,
        DOWN = 2,
        LEFT = 3,
        RIGHT = 4,
    };

    character_t():
        orientation_(DOWN),
        target_(0)
    {}
    explicit character_t(const game_entity_t& e):
        game_entity_t(e),
        orientation_(DOWN),
        target_(0)
    {
    }
    void set_orientation(int o) { orientation_ = o; }
    int orientation() const { return orientation_; }
    void reset_hp(int hp) { max_hp_ = hp; hitpoints_ = hp; }
    int regen_hp(int regen)
    {
        int hp = regen + hitpoints_;
        hitpoints_ = (hp < max_hp_)? hp: max_hp_;
        return hitpoints_;
    }

    int max_hp() const { return max_hp_; }
    int hitpoints() const { return hitpoints_; }

    virtual char* get_state(string_ptr& buf) const;
    void set_target(int t) { target_ = t; }
    void clear_target() { target_ = 0; }
    int target() const { return target_; }
    bool has_target() const { return target_ > 0; }
    bool is_dead() const { return hitpoints_ <= 0; }
    void recv_dmg(int dmg) { hitpoints_ -= dmg; }
private:
    int orientation_;
    int max_hp_;
    int hitpoints_;
    int target_;
};

class mob_t: public character_t
{
public:
    enum
    {
        MAX_TARGET_SIZE = 4,
    };
    mob_t()
    {
        reset_hp(100);
    }
    explicit mob_t(const game_entity_t& e):
    character_t(e)
    {
        reset_hp(100);
    }
};

#endif
