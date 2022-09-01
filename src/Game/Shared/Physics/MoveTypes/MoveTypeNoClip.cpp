/***
*
*	License here.
*
*	@file
*
*	Both the ClientGame and the ServerGame modules share the same general Physics code.
* 
***/
#pragma once

//! Include the code base of the GameModule we're compiling against.
#include "Game/Shared/GameBindings/GameModuleImports.h"

// Physics.
#include "../Physics.h"



/**
*	@brief Logic for MoveType::(NoClip): Moves the entity based on angular- and regular- velocity. Does not clip to world or entities.
**/
void SG_Physics_NoClip(SGEntityHandle &entityHandle) {
    // Assign handle to base entity.
    GameEntity* gameEntity = *entityHandle;

    // Ensure it is a valid entity.
    if (!gameEntity) {
	    SG_Print( PrintType::DeveloperWarning, fmt::format( "{}({}): got an invalid entity handle!\n", __func__, sharedModuleName ) );
        return;
    }

	// regular thinking
	if (SG_RunThink(gameEntity)) {
		return;
	}
	if (!gameEntity->IsInUse()) {
		return;
	}

    gameEntity->SetAngles(vec3_fmaf(gameEntity->GetAngles(), FRAMETIME_S.count(), gameEntity->GetAngularVelocity()));
    gameEntity->SetOrigin(vec3_fmaf(gameEntity->GetOrigin(), FRAMETIME_S.count(), gameEntity->GetVelocity()));

    gameEntity->LinkEntity();
}