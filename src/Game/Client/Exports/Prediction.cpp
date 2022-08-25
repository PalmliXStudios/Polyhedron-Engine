/***
*
*	License here.
*
*	@file
*
*	ClientGame Player Prediction handling interface implementation.
*
***/
//! Main Headers.
#include "Game/Client/ClientGameMain.h"
//! Client Game Local headers.
#include "Game/Client/ClientGameLocals.h"

// Exports Interfaces.
#include "Game/Client/Exports/Entities.h"
#include "Game/Client/Exports/Prediction.h"
#include "Game/Client/Exports/View.h"

// ClientBinding to SharedGame for SG_GetEntityNumber utility.
#include "Game/Shared/GameBindings/ClientBinding.h"

// Distance that is allowed to be taken as a delta before we reset it.
static const double MAX_DELTA_ORIGIN = (2400.0 * (1.00 / BASE_FRAMERATE));


// GameWorld.
#include "../World/ClientGameWorld.h"

/**
*   @brief  Checks for prediction incorectness. If found, corrects it.
**/
void ClientGamePrediction::CheckPredictionError(ClientMoveCommand* moveCommand) {
    const PlayerMoveState* in = &cl->frame.playerState.pmove;
    ClientPredictedState* out = &cl->predictedState;

    // if prediction was not run (just spawned), don't sweat it
    if (moveCommand->prediction.simulationTime == 0) {
        out->viewOrigin = in->origin;
        out->viewOffset = in->viewOffset;
        out->viewAngles = in->viewAngles;
        out->stepOffset = 0.f;

        out->error = vec3_zero();
        return;
    }

    // Subtract what the server returned from our predicted origin for that frame
    out->error = moveCommand->prediction.error = (moveCommand->prediction.origin - in->origin);

    // If the error is too large, it was likely a teleport or respawn, so ignore it
    const float len = vec3_length(out->error);
    if (len > .1) {
        if (len > MAX_DELTA_ORIGIN) {
            CLG_Print( PrintType::DeveloperWarning, fmt::format( "CLG_PredictionError: if (len > MAX_DELTA_ORIGIN): {}\n", Vec3ToString(out->error) ) );

            out->viewOrigin = in->origin;
            out->viewOffset = in->viewOffset;
            out->viewAngles = in->viewAngles;
            out->stepOffset = 0.f;

            out->error = vec3_zero();
        }
        else {
            CLG_Print( PrintType::DeveloperWarning, fmt::format( "CLG_PredictionError: {}\n", Vec3ToString(out->error) ) );
        }
    }
}

/**
*   @brief  Adds the delta angles to the view angles. Required for other
*           (especially rotating) objects to be able to push the player around properly.
**/
void ClientGamePrediction::PredictAngles() {
    // Add delta predicted angles to our view angles. (Allows for doors and other objects to push the player properly.)
    cl->predictedState.viewAngles[0] = cl->viewAngles[0] + cl->frame.playerState.pmove.deltaAngles[0];
    cl->predictedState.viewAngles[1] = cl->viewAngles[1] + cl->frame.playerState.pmove.deltaAngles[1];
    cl->predictedState.viewAngles[2] = cl->viewAngles[2] + cl->frame.playerState.pmove.deltaAngles[2];
}

/**
*   @brief  Process the actual predict movement simulation.
**/
void ClientGamePrediction::PredictMovement(uint32_t acknowledgedCommandIndex, uint32_t currentCommandIndex) {
    // Player Move object.
    PlayerMove pm = {};

    // Only continue if there is an acknowledged command index, or a current command index.
    if (!acknowledgedCommandIndex || !currentCommandIndex)
        return;

    // Setup base trace calls.
    pm.Trace = PM_Trace;
    pm.PointContents = PM_PointContents;

    // Restore ground entity for this frame.
    pm.groundEntityNumber = cl->predictedState.groundEntityNumber; //	pm.groundEntityPtr = cl->predictedState.groundEntityPtr;

    // Copy current state to pmove
    pm.state = cl->frame.playerState.pmove;
#if USE_SMOOTH_DELTA_ANGLES
    pm.state.deltaAngles = clge->view->GetViewCamera()->GetViewDeltaAngles(); //cl->deltaAngles;
#endif
	
    // Run frames in order.
	// First run the player move prediction process in order on all previously backed up acknowledged
	// user input command frames.
    while (++acknowledgedCommandIndex <= currentCommandIndex) {
        // Fetch the command.
        ClientMoveCommand* cmd = &cl->clientUserCommands[acknowledgedCommandIndex & CMD_MASK];

        // If the command has an msec value it means movement has taken place and we prepare for 
        // processing another simulation.
        if (cmd->input.msec) {
            // Saved for prediction error checking.
            cmd->prediction.simulationTime = clgi.GetRealTime();

            // Assign the move command.
            pm.moveCommand = *cmd;
						
            // Simulate the move command.
            PMove( &pm );

            // Update player move client side audio effects.
            UpdateClientSoundSpecialEffects( &pm );
			
			// Execute touch callbacks and "predict" against other entities.
			DispatchPredictedTouchCallbacks( &pm );
        }

        // Save for error detection
        cmd->prediction.origin = pm.state.origin;
    }

	// Run the player move prediction process using the current pending frame user input.
    if (cl->moveCommand.input.msec) {
        // Saved for prediction error checking.
        cl->moveCommand.prediction.simulationTime = clgi.GetRealTime();

		// Process final player move command.
        pm.moveCommand = cl->moveCommand;
        pm.moveCommand.input.forwardMove = cl->localMove[0];
        pm.moveCommand.input.rightMove = cl->localMove[1];
        pm.moveCommand.input.upMove = cl->localMove[2];
        PMove(&pm);
		
        // Update player move client side audio effects.
        UpdateClientSoundSpecialEffects(&pm);

		// Execute touch callbacks and "predict" against other entities.		
		DispatchPredictedTouchCallbacks(&pm);

        // Save for error detection
        cl->moveCommand.prediction.origin = pm.state.origin;
    }

    // Copy results out for rendering
    // TODO: This isn't really the nicest way of preventing these "do not get stuck" budges.
    // Perhaps take another look at pmove to correct this in the future.
    //if ( vec3_distance( cl->predictedState.viewOrigin, pm.state.origin ) > 0.03125f ) {
		cl->predictedState.viewOrigin = pm.state.origin;
    //}
    cl->predictedState.velocity	  = pm.state.velocity;
    cl->predictedState.viewOffset = pm.state.viewOffset;
    cl->predictedState.stepOffset = pm.state.stepOffset;
    cl->predictedState.viewAngles = pm.viewAngles;

    cl->predictedState.groundEntityNumber = pm.groundEntityNumber;
}

/**
*   @brief  Update the client side audio state.
**/
void ClientGamePrediction::UpdateClientSoundSpecialEffects(PlayerMove* pm)
{
    static int underwater;

    // Ensure that cl != NULL, it'd be odd but hey..
    if (cl == NULL) {
        return;
    }

    if ((pm->waterLevel == 3) && !underwater) {
        underwater = 1;
        cl->snd_is_underwater = 1; // OAL: snd_is_underwater moved to client struct.
                                   // TODO: DO!
        if (cl->snd_is_underwater_enabled)
            clgi.SFX_Underwater_Enable();
    }

    if ((pm->waterLevel < 3) && underwater) {
        underwater = 0;
        cl->snd_is_underwater = 0; // OAL: snd_is_underwater moved to client struct.

                                   // TODO: DO!
        if (cl->snd_is_underwater_enabled)
            clgi.SFX_Underwater_Disable();
    }
}

/**
*	@brief	Called by DispatchPredictTouchCallbacks to apply the current player move results
*			to the actual player entity itself.
**/
void ClientGamePrediction::PlayerMoveToClientEntity( PlayerMove *pm ) {
	// Get Game World.
	ClientGameWorld *gameWorld = GetGameWorld();
	auto *gePlayer = gameWorld->GetClientGameEntity();
    gePlayer->SetOrigin( pm->state.origin );
    gePlayer->SetVelocity( pm->state.velocity );
    gePlayer->SetMins( pm->mins );
    gePlayer->SetMaxs( pm->maxs );
    gePlayer->SetViewHeight( pm->state.viewOffset[2] );
    gePlayer->SetWaterLevel( pm->waterLevel );
    gePlayer->SetWaterType( pm->waterType );
	// Resolve the perhaps new Ground Entity.

	if ( gameWorld ) {
		GameEntity *geGround = gameWorld->GetGameEntityByIndex( pm->groundEntityNumber );

		if ( geGround ) {
			gePlayer->SetGroundEntity( geGround );
			gePlayer->SetGroundEntityLinkCount( geGround->GetLinkCount() );
		}
		else {
			gePlayer->SetGroundEntity( SGEntityHandle() );
			gePlayer->SetGroundEntityLinkCount(0);
		}
	} else {
		gePlayer->SetGroundEntity( SGEntityHandle() );
	}
};

/**
*	@brief	Dispatch touch callbacks for all predicted touched entities.
**/
void ClientGamePrediction::DispatchPredictedTouchCallbacks(PlayerMove *pm) {
	// Get gameworld.
	ClientGameWorld *gameWorld = GetGameWorld();

	// Execute touch callbacks as long as movetype isn't noclip, or spectator.
	auto *gePlayer = gameWorld->GetClientGameEntity();
	if (gePlayer && pm && cl->bsp) {//}&& cl->cm.cache) {
		const int32_t playerMoveType = gePlayer->GetMoveType();
	    if (playerMoveType != MoveType::NoClip && playerMoveType  != MoveType::Spectator) {
			// Setup origin, mins and maxs for UTIL_TouchTriggers as well as the ground entity.
			//player->SetOrigin(pm->state.origin);
			//player->SetMins(pm->mins);
			//player->SetMaxs(pm->maxs);
		// Update entity properties based on results of the player move simulation.
			// What if we try and set it here?
			PlayerMoveToClientEntity( pm );

			// Let the world know about the current entity we're running.
			level.currentEntity = gePlayer;

			// Dispatch touch trigger callbacks on the player entity for each touched entity.
			SG_TouchTriggers( gePlayer );

			// Solid touch logic.
			int32_t i = 0;
			int32_t j = 0;
            
			for ( i = 0 ; i < pm->numTouchedEntities; i++ ) {
				// Skip touch events on the same entities.
				for ( j = 0; j < i ; j++ ) {
					const int32_t touchEntityNumberA = SG_GetEntityNumber( pm->touchedEntityTraces[j].podEntity );
					const int32_t touchEntityNumberB = SG_GetEntityNumber( pm->touchedEntityTraces[i].podEntity );
					if ( touchEntityNumberA == touchEntityNumberB ) {
						break;
					}
				}
				if ( j != i ) {
					continue; // Duplicated.
				}

				
				// Get Touched Entity.
				const int32_t touchedEntityNumber = SG_GetEntityNumber( pm->touchedEntityTraces[i].podEntity );

				// Get and validate our touched entity.
				GameEntity *geOther = ClientGameWorld::ValidateEntity( gameWorld->GetPODEntityByIndex( touchedEntityNumber ) );

				// Continue in case it's invalid.
				if ( !geOther ) {
					continue;
				}

				// Dispatch Touch.
				geOther->DispatchTouchCallback( geOther, gePlayer, NULL, NULL );
			}
		}
	}
}

/**
*   @brief  Player Move Simulation Trace Wrapper.
**/
TraceResult ClientGamePrediction::PM_Trace(const vec3_t& start, const vec3_t& mins, const vec3_t& maxs, const vec3_t& end) {
    TraceResult cmTrace;
    
    cmTrace = clgi.Trace(start, mins, maxs, end, 0, BrushContentsMask::PlayerSolid);

    return cmTrace;
}

/**
*   @brief  Player Move Simulation PointContents Wrapper.
**/
int32_t ClientGamePrediction::PM_PointContents(const vec3_t &point) {
	// Get world brush contents at 'point' coordinate.
    PODEntity* ent = nullptr;
    mmodel_t* cmodel = nullptr;

    int32_t contents = clgi.CM_PointContents(point, cl->bsp->nodes);

    for (int32_t i = 0; i < cl->numSolidEntities; i++) {
        ent = cl->solidEntities[i];

        if (ent->currentState.solid != PACKED_BBOX) // special value for bmodel
            continue;

        cmodel = cl->clipModels[ent->currentState.modelIndex];
        if (!cmodel)
            continue;

        contents |= clgi.CM_TransformedPointContents(
            point, cmodel->headNode,
            ent->currentState.origin,
            ent->currentState.angles);
    }

	// Return.
    return contents;
}