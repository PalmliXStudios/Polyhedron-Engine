/*
// LICENSE HERE.

//
// TriggerOnce.cpp
//
// This trigger will always fire.  It is activated by the world.
//
*/
//! Main Headers.
#include "Game/Server/ServerGameMain.h"
//! Server Game Local headers.
#include "Game/Server/ServerGameLocals.h"

#include "../../Effects.h"     // Effects.
#include "../../Utilities.h"       // Util funcs.
#include "../Base/SVGBaseEntity.h"
#include "../Base/SVGBaseTrigger.h"
#include "TriggerMultiple.h" 
#include "TriggerOnce.h"

//
// Spawn Flags.
// 

// Constructor/Deconstructor.
TriggerOnce::TriggerOnce(PODEntity *svEntity) : TriggerMultiple(svEntity) {
	//
	// All callback functions best be nullptr.
	//

	//
	// Set all entity pointer references to nullptr.
	//

	//
	// Default values for members.
	//
	//lastHurtTime = 0.f;
}

// Interface functions. 
//
//===============
// TriggerOnce::Precache
//
//===============
//
void TriggerOnce::Precache() {
	Base::Precache();
}

//
//===============
// TriggerOnce::Spawn
//
//===============
//
void TriggerOnce::Spawn() {
	// Spawn base trigger.
	Base::Spawn();

	// Set wait time to -1... (So it triggers only once.)
	SetWaitTime(-1s);
}

//
//===============
// TriggerOnce::Respawn
// 
//===============
//
void TriggerOnce::Respawn() {
	Base::Respawn();
}

//
//===============
// TriggerOnce::PostSpawn
// 
//===============
//
void TriggerOnce::PostSpawn() {
	Base::PostSpawn();
}

//
//===============
// TriggerOnce::Think
//
//===============
//
void TriggerOnce::Think() {
	Base::Think();
}

//
//===============
// TriggerOnce::SpawnKey
//
//===============
//
void TriggerOnce::SpawnKey(const std::string& key, const std::string& value) {
	// This one can't be set, it is a trigger_once for a reason.
	if (key == "wait") {
		// TODO: Proper debug reporting.
	} else {
		// Parent class spawnkey.
		// We don't want it to reposition this fucker.?
		Base::SpawnKey(key, value);
	}
}

void TriggerOnce::Trigger(IServerGameEntity* activator) {
	// Parent trigger.
	Base::Trigger(activator);
}

//
//===============
// TriggerOnce::TriggerOnceThinkWait
//
// 'Think' callback, to wait out.
//===============
//
void TriggerOnce::TriggerOnceThinkWait() {
	SetNextThinkTime(GameTime::zero());
}

//
//===============
// TriggerOnce::TriggerOnceTouch
//
// 'Touch' callback, to hurt the entities touching it.
//===============
//
void TriggerOnce::TriggerOnceTouch(IServerGameEntity* self, IServerGameEntity* other, CollisionPlane* plane, CollisionSurface* surf) {
	if (this == other)
		return;

	gi.DPrintf("#1 Touched trigger once");
	if (other->GetClient()) {
		if (GetSpawnFlags() & 2)
			return;
	} else if (other->GetServerFlags() & EntityServerFlags::Monster) {
		if (!(GetSpawnFlags() & 1))
			return;
	} else {
		return;
	}
	gi.DPrintf("#2 Touched trigger once");

	//if (!vec3_equal(self->moveDirection, vec3_zero())) {
	//	vec3_t  forward;

	//	AngleVectors(other->state.angles, &forward, NULL, NULL);
	//	if (DotProduct(forward, self->moveDirection) < 0)
	//		return;
	//}

	//self->activator = other;
	SetActivator(other);
	Trigger(other);
}

//
//===============
// TriggerOnce::TriggerOnceUse
//
// 'Use' callback, whenever the trigger is activated.
//===============
//
void TriggerOnce::TriggerOnceUse(IServerGameEntity* other, IServerGameEntity* activator) {
	// Trigger itself.
	Trigger(activator);
}

//
//===============
// TriggerOnce::TriggerOnceEnable
//
// 'Use' callback, whenever the trigger wasn't, but still has to be activated.
//===============
//
void TriggerOnce::TriggerOnceEnable(IServerGameEntity* other, IServerGameEntity* activator) {
	// Set the new solid, since it wasn't Solid::Trigger when disabled.
	SetSolid(Solid::Trigger);

	// Set the new use function, to be its default.
	SetUseCallback(&TriggerOnce::TriggerOnceUse);

	// Since we changed the solid, relink the entity.
	LinkEntity();
}