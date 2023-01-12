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
#include "Game/Shared/Physics/Physics.h"

/**
*	@brief	Pushes the entity. Does not change the entities velocity at all
**/
SGTraceResult SG_PushEntity( GameEntity *gePushEntity, const vec3_t &pushOffset ) {
	SGTraceResult trace;
    int32_t     mask = 0;

	GameEntity *ent = gePushEntity;

    // Calculate start for push.
    vec3_t start = ent->GetOrigin();

    // Calculate end for push.
    vec3_t end = start + pushOffset;

retry:
    if (ent->GetClipMask())
        mask = ent->GetClipMask();
    else
        mask = BrushContentsMask::Solid;

    trace = SG_Trace(start, ent->GetMins(), ent->GetMaxs(), end, ent, mask);

	//if (ent->GetMoveType() == MoveType::Push || !trace.startSolid) {
	if (!trace.startSolid) {//if (!trace.startSolid) {
		ent->SetOrigin(trace.endPosition);
	}
	//}
    //ent->SetOrigin(trace.endPosition);
    ent->LinkEntity();

    if ( trace.fraction != 1.0f ) {
		// Dispatch impact callbacks.
        SG_Impact(ent, trace);

        // if the pushed entity went away and the pusher is still there
        if ( (!trace.gameEntity || !trace.gameEntity->IsInUse()) && ent->GetMoveType() == MoveType::Push && ent->IsInUse()) {
            // move the pusher back and try again
            ent->SetOrigin( start );
            ent->LinkEntity();
            goto retry;
        }
    }

    if ( ent->IsInUse() ) {
        SG_TouchTriggers( ent );
	}

    return trace;
}


/**
*	@brief Logic for MoveType::(Toss, TossSlide, Bounce, Fly and FlyMissile)
**/
void SG_Physics_Toss(SGEntityHandle& entityHandle) {
    // Get Gameworld.
	SGGameWorld *gameWorld = GetGameWorld();

	// Assign handle to base entity.
    GameEntity* ent = *entityHandle;

    // Ensure it is a valid entity.
    if (!ent) {
	    SG_Print( PrintType::DeveloperWarning, fmt::format( "{}({}): got an invalid entity handle!\n", __func__, sharedModuleName ) );
        return;
    }

	// Let the entity think.
	SG_RunThink( ent );

	// Has to be in use.
    if ( !ent->IsInUse() ) {
        return;
	}

    // If not a team captain, so movement will be handled elsewhere
    if ( ent->GetFlags() & EntityFlags::TeamSlave ) {
        return;
	}

	// Refresh the ground entity for said MoveType entities:
	if( ent->GetMoveType() == MoveType::Bounce || ent->GetMoveType() == MoveType::TossSlide) {/*MOVETYPE_BOUNCE || ent->movetype == MOVETYPE_BOUNCEGRENADE ) {*/
		if( ent->GetVelocity().z > 0.1f) {
			ent->SetGroundEntity( SGEntityHandle( nullptr, -1 ) );
		}
	}

	// Check whether the ground entity has disappeared(aka not in use).
	GameEntity *entGroundEntity = SGGameWorld::ValidateEntity( ent->GetGroundEntityHandle() );
	//	if( ent->groundentity && ent->groundentity != world && !ent->groundentity->r.inuse ) {
	if (entGroundEntity && !entGroundEntity->IsInUse()) {
		ent->SetGroundEntity( SGEntityHandle( nullptr, -1 ) ); //ent->groundentity = NULL;
	}

	// Calculate old speed based on velocity vec length.
	float oldSpeed = vec3_length( ent->GetVelocity() );

	// Check if the ent still has a valid ground entity.
	entGroundEntity = SGGameWorld::ValidateEntity( ent->GetGroundEntityHandle() );
	if ( entGroundEntity && ent->GetMoveType() != MoveType::TossSlide ) {
		// Exit if there's no velocity(speed) activity.
		if( !oldSpeed ) {
			return;
		}

		// Special movetype Toss behavior.
		if( ent->GetMoveType() == MoveType::Toss) {
			// 8 = 1 Unit on the Quake scale I think.
			if( ent->GetVelocity().z >= 8) {
				// it's moving in-air so unset the ground entity.
				ent->SetGroundEntity( SGEntityHandle( nullptr, -1 ) );
			} else {
				// Otherwise, let it fall to a stop.
				ent->SetVelocity(vec3_zero());
				ent->SetAngularVelocity(vec3_zero());
				// Dispatch Stop callback.
				ent->DispatchStopCallback( );
				return;
			}
		}
	}

	// Get origin.
	const vec3_t oldOrigin = ent->GetOrigin(); // VectorCopy( ent->s.origin, old_origin );

	// As long as there's any acceleration at all, calculate it for the current frame.
	if( ent->GetAcceleration() != 0) {
		// Negative acceleration and a low velocity makes it come to a halt.
		if( ent->GetAcceleration() < 0 && vec3_length(ent->GetVelocity()) < 8) {
			ent->SetVelocity(vec3_zero());
			//VectorClear( ent->velocity );
		} else {
			//VectorNormalize2
			vec3_t acceldir = vec3_zero();
			VectorNormalize2( ent->GetVelocity(), acceldir);
			acceldir = vec3_scale( acceldir, ent->GetAcceleration()  * FRAMETIME_S.count());

			// Add directional acceleration to velocity.
			ent->SetVelocity(ent->GetVelocity() + acceldir);//VectorAdd( ent->velocity, acceldir, ent->velocity );
		}
	}

	// Check velocity and maintain it in bounds.
	SG_BoundVelocity( ent );

	// Add gravitational forces.
	if( ent->GetMoveType()  != MoveType::Fly && !ent->GetGroundEntityHandle()) {
		SG_AddGravity( ent );
	}

	// Hacks
	//if( ent->s.type != ET_PLASMA && ent->s.type != ET_ROCKET ) {
	//	// move angles
	//	VectorMA( ent->s.angles, FRAMETIME_S, ent->avelocity, ent->s.angles );
	//}

	// move origin
	const vec3_t move = vec3_scale( ent->GetVelocity(), FRAMETIME_S.count() ); //VectorScale( ent->velocity, FRAMETIME_S, move );

	SGTraceResult traceResult = SG_PushEntity( ent, move );
	if( !ent->IsInUse() ) {
		return;
	}

	// In case the trace hit anything...
	//if( traceResult.podEntity ) {
		const int32_t entMoveType = ent->GetMoveType();

		// 1 = default backOff value.
		float backOff = 1;

		// Adjust value for specific moveTypes.
		if( entMoveType == MoveType::Bounce) {
			backOff = 1.5;
		}/* else if( entMoveType == MoveType::BounceGrenade) {
			backOff = 1.4;
		} */

		// Clip velocity.
		ent->SetVelocity( SG_ClipVelocity( ent->GetVelocity(), traceResult.plane.normal ) );

		// Store clipped velocity.
		const vec3_t clippedVelocity = ent->GetVelocity();

		// stop if on ground
		if( entMoveType == MoveType::Bounce ) {// || entMoveType == MoveType::BounceGrenade ) {
			// stop dead on allsolid

			// LA: hopefully will fix grenades bouncing down slopes
			// method taken from Darkplaces sourcecode
			if( traceResult.allSolid ||
				( IsWalkablePlane( traceResult.plane ) && fabsf( vec3_dot( traceResult.plane.normal, ent->GetVelocity() ) ) < 60 ) ) {
				GameEntity *geTrace = traceResult.gameEntity;
				// Update ground entity.
				ent->SetGroundEntity(geTrace);
				ent->SetGroundEntityLinkCount((geTrace ? geTrace->GetLinkCount() : 0));

				// Zero out (angular-)velocities.
				ent->SetVelocity(vec3_zero());
				ent->SetAngularVelocity(vec3_zero());

				// Dispatch a Stop callback.
				ent->DispatchStopCallback( );
			}
		} else {
			// Get ground brush contents.
			const int32_t groundContents = traceResult.contents;
			// Mask to use for 'current' contents.
			const int32_t currentContentsMask = (BrushContents::Current0 | BrushContents::Current90 | BrushContents::Current180 | BrushContents::Current270 | BrushContents::CurrentUp | BrushContents::CurrentDown );
			// For move TossSlide we do special handling so it can apply currents as well.
			if ( entMoveType == MoveType::TossSlide && 
				( groundContents & currentContentsMask ) == currentContentsMask ) {
				// New velocity that gets set depending on the current.
				vec3_t groundCurrentVelocity = vec3_zero();

				if ( groundContents & BrushContents::Current0 ) {
					groundCurrentVelocity.x += 1.0;
				}
				if ( groundContents & BrushContents::Current90 ) {
					groundCurrentVelocity.y += 1.0;
				}
				if ( groundContents & BrushContents::Current180 ) {
					groundCurrentVelocity.x -= 1.0;
				}
				if ( groundContents & BrushContents::Current270 ) {
					groundCurrentVelocity.y -= 1.0;
				}
				if ( groundContents & BrushContents::CurrentUp ) {
					groundCurrentVelocity.z += 1.0;
				}
				if ( groundContents & BrushContents::CurrentDown ) {
					groundCurrentVelocity.z -= 1.0;
				}

				// Only apply the new 'currents' velocity if caught on a current surface.
				if (!vec3_equal(groundCurrentVelocity, vec3_zero())) {
					// Normalize ground current velocity.
					groundCurrentVelocity = vec3_normalize(groundCurrentVelocity);

					
					// Apply based on speed current.
					constexpr float PHYS_SPEED_CURRENT = 80.f;
					ent->SetVelocity( vec3_fmaf( clippedVelocity, PHYS_SPEED_CURRENT, groundCurrentVelocity));
				}


			// Otherwise, execute regular stopping behavior.
			} else {

				// in movetype_toss things stop dead when touching ground
	#if 0
				SG_CheckGround( ent );

				if( ent->groundentity ) {
	#else

				// Walkable or trapped inside solid brush
				if( traceResult.allSolid || IsWalkablePlane( traceResult.plane ) ) {
					GameEntity *geTrace = ( traceResult.gameEntity ? traceResult.gameEntity : (GameEntity*)gameWorld->GetWorldspawnGameEntity() );
					// Update ground entity.
					ent->SetGroundEntity( geTrace );
					ent->SetGroundEntityLinkCount( (geTrace ? geTrace->GetLinkCount() : 0) );
	#endif
					if (vec3_length(ent->GetVelocity()) < 0.0001f) {
						// Zero out (angular-)velocities.
						ent->SetVelocity( vec3_zero() );
						ent->SetAngularVelocity( vec3_zero() );
					}

					// Dispatch a Stop callback.
					ent->DispatchStopCallback( );
				}
			}
		//}
	}

    //
	// Check for whether we're transitioning in or out of a liquid(mostly, water, it is considered as, water...).
	//
	// Were we in water?
    const bool wasInWater = (ent->GetWaterType() & BrushContentsMask::Liquid ? true : false);

	// Get content type for current origin.
	const int32_t pointContentType = SG_PointContents(ent->GetOrigin());
	const bool isInWater = (pointContentType & BrushContentsMask::Liquid);

	// Set the watertype(or whichever the contents are.)
	ent->SetWaterType(pointContentType);
	
	// Update entity's waterlevel.
	if( isInWater ) {
		ent->SetWaterLevel(1);
	} else {
		ent->SetWaterLevel(0);
	}

	// Play transitioning sounds.
#ifdef SHAREDGAME_SERVERGAME
    // Determine what sound to play.
    if (!wasInWater && isInWater) {
        gi.PositionedSound(oldOrigin, game.world->GetPODEntities(), SoundChannel::Auto, gi.PrecacheSound("misc/h2ohit1.wav"), 1, 1, 0);
	} else if (wasInWater && !isInWater) {
        gi.PositionedSound(ent->GetOrigin(), game.world->GetPODEntities(), SoundChannel::Auto, gi.PrecacheSound("misc/h2ohit1.wav"), 1, 1, 0);
	}
#endif
	//if( !wasInWater && isInWater ) {
	//	SG_PositionedSound( oldOrigin, SoundChannel::Auto, SG_SoundIndex( S_HIT_WATER ), Attenuation::Idle );
	//} else if( wasInWater && !isInWater ) {
	//	SG_PositionedSound( ent->GetOrigin(), SoundChannel::Auto, SG_SoundIndex(S_HIT_WATER), Attenuation::Idle );
	//}

	// Move followers.
	for( GameEntity *follower = ent->GetTeamChainEntity(); follower; follower = follower->GetTeamChainEntity() ) {
		// Set follower origin to ent origin.
		follower->SetOrigin(ent->GetOrigin());

		// Link follower.
		follower->LinkEntity();
	}
}