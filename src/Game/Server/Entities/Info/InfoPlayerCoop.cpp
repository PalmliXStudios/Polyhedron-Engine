/*
// LICENSE HERE.

//
// PlayerStart.cpp
//
//
*/
//! Main Headers.
#include "Game/Server/ServerGameMain.h"
//! Server Game Local headers.
#include "Game/Server/ServerGameLocals.h"

#include "../Base/SVGBaseEntity.h"      // BaseEntity.
#include "InfoPlayerStart.h"
#include "InfoPlayerCoop.h"     // Class.

// Constructor/Deconstructor.
InfoPlayerCoop::InfoPlayerCoop(PODEntity *svEntity) 
    : InfoPlayerStart(svEntity) {

}

// Interface functions. 
//===============
// InfoPlayerStart::Precache
//
//===============
void InfoPlayerCoop::Precache() {
    Base::Precache();
}

//===============
// InfoPlayerStart::Spawn
//
//===============
void InfoPlayerCoop::Spawn() {
    Base::Spawn();
}

//===============
// InfoPlayerCoop::PostSpawn
//
//===============
void InfoPlayerCoop::PostSpawn() {
    Base::PostSpawn();
}

//===============
// InfoPlayerCoop::Think
//
//===============
void InfoPlayerCoop::Think() {
    // Parent think.
    Base::Think();
}

//===============
// InfoPlayerCoop::SpawnKey
//
//===============
void InfoPlayerCoop::SpawnKey(const std::string& key, const std::string& value) {
    Base::SpawnKey(key, value);
}
