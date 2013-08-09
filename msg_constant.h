#ifndef _MSG_CONSTANT_H_
#define _MSG_CONSTANT_H_

enum msg_id_t
{
    MSG_HELLO = 0,
    MSG_WELCOME = 1,
    MSG_SPAWN = 2,
    MSG_DESPAWN = 3,    // item looted, mob/player killed
    MSG_MOVE = 4,
    MSG_LOOTMOVE = 5,
    MSG_AGGRO = 6,
    MSG_ATTACK = 7,
    MSG_HIT = 8,
    MSG_HURT = 9,
    MSG_HEALTH = 10,    // current hp [msgid,hp,is_regen(regen tick only)]
    MSG_CHAT = 11,
    MSG_LOOT = 12,
    MSG_EQUIP = 13,
    MSG_DROP = 14,
    MSG_TELEPORT = 15,
    MSG_DAMAGE = 16,
    MSG_POPULATION = 17,
    MSG_KILL = 18,
    MSG_LIST = 19,
    MSG_WHO = 20,
    MSG_ZONE = 21,
    MSG_DESTROY = 22,   // tell client to release some entity's resource
    MSG_HP = 23,    // max hp [msgid, maxhp], firepotion&equipitem
    MSG_BLINK = 24,
    MSG_OPEN = 25,
    MSG_CHECK = 26,
};

#endif
