browserquestcc
==============

mozilla's browserquest server implementation in c++, based on calypso

Partially implemented server-side message
-------------------------------
* HELLO
* WHO (deprecated, instead we initiatively push spawn messages to client)
* MOVE
* TELEPORT
* ZONE
* LOOTMOVE (actually I don't know what it's for)
* LOOT (some item function not implemented)
* CHECK (checkpoint not stored, just a stub)
* ATTACK
* HIT (fixed damage)
* HURT (fixed damage)
* CHAT (no sanitizer)
* OPEN (no random item, but can be easily done)
* AGGRO (no hatred system yet)

Protocol modification
---------------------
* Server to client
    * MOVE: [msgid, x, y, engage], add engage. entity will disengage if 0==engage
* Client to server
    * HURT: [msgid, mobid, x, y], add x and y, so server-side can update mob position when it attacks player

Client-side modification
------------------------
* Update character's target even if it already exists when handling spawn message
