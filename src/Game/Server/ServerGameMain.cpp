/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

//! Main Headers.
#include "Game/Server/ServerGameMain.h"
//! Server Game Local headers.
#include "Game/Server/ServerGameLocals.h"


// Entities.
#include "Entities.h"
//#include "Entities/Worldspawn.h"
//#include "Entities/Base/SVGEntityHandle.h"
//#include "Entities/Base/SVGBasePlayer.h"

// GameModes.
#include "Gamemodes/IGamemode.h"
#include "Gamemodes/DefaultGamemode.h"
#include "Gamemodes/CoopGamemode.h"
#include "Gamemodes/DeathMatchGamemode.h"

// GameWorld.
#include "Game/Server/World/ServerGameWorld.h"

// Player related.
#include "Game/Server/Player/Client.h"      // Include Player Client header.

// Physics related.
#include "Game/Shared/Physics/Physics.h"

//-----------------
// Global Game and Engine API Imports/Exports Variables.
//
// These are used all throughout the code to communicate back and forth
// between engine and game module.
//-----------------
ServerGameImports gi;       // CLEANUP: These were game_import_t and game_export_t
ServerGameExports globals;  // CLEANUP: These were game_import_t and game_export_t

int sm_meat_index;
int snd_fry;

//-----------------
// CVars.
//-----------------
// GameMode and Server settings.
cvar_t* gamemode = nullptr;  // Stores the gamemode string: "singleplayer", "deathmatch", "coop"
cvar_t* gamemodeflags = nullptr;
cvar_t* skill = nullptr;
cvar_t* fraglimit = nullptr;
cvar_t* timelimit = nullptr;
cvar_t* password = nullptr;
cvar_t* spectator_password = nullptr;
cvar_t* needpass = nullptr;
cvar_t* filterban = nullptr;

cvar_t* maximumclients = nullptr;
cvar_t* maxspectators = nullptr;
cvar_t* maxEntities = nullptr;

cvar_t* g_select_empty = nullptr;
cvar_t* dedicated = nullptr;

cvar_t* flood_msgs = nullptr;
cvar_t* flood_persecond = nullptr;
cvar_t* flood_waitdelay = nullptr;

cvar_t* sv_cheats = nullptr;
cvar_t* sv_maplist = nullptr;

// Physics settings.
cvar_t* sv_maxvelocity = nullptr;
cvar_t* sv_gravity = nullptr;
cvar_t* sv_rollspeed = nullptr;
cvar_t* sv_rollangle = nullptr;

// View/User Control settings.
cvar_t*	 run_pitch = nullptr;
cvar_t*	 run_roll = nullptr;
cvar_t*	 bob_up = nullptr;
cvar_t*	 bob_pitch = nullptr;
cvar_t*	 bob_roll = nullptr;

// Client related. (Are monster footsteps on?)
cvar_t* cl_monsterfootsteps = nullptr;


// Developer related.
cvar_t* gun_x = nullptr;
cvar_t* gun_y = nullptr;
cvar_t* gun_z = nullptr;

cvar_t* dev_show_physwarnings = nullptr;

//-----------------
// Funcs used locally.
//-----------------
void SVG_SpawnEntities(const char *mapName, const char *entities, const char *spawnpoint);
static void SVG_SetupCVars();

void SVG_RunEntity(SGEntityHandle &entityHandle);
void SVG_WriteGame(const char *filename, qboolean autosave);
void SVG_ReadGame(const char *filename);
void SVG_WriteLevel(const char *filename);
void SVG_ReadLevel(const char *filename);
void SVG_InitGame(void);
void SVG_RunFrame(void);

//=============================================================================
//
//	SVGame Core API entry points.
//
//=============================================================================

/**
*   @brief  This will be called when the dll is initialize. This happens 
*           when a new game is started or a save game is loaded.
**/
void SVG_InitGame(void)
{
    gi.DPrintf("==== InitServerGame ====\n");

    // Initialise the type info system
    TypeInfo::SetupSuperClasses();

    // Setup CVars that we'll be working with.
    SVG_SetupCVars();

    // Setup our game locals object.
    game.Initialize();
}

//===============
// SVG_ShutdownGame
//
// Whenever a "game" or aka a "match" ends, this gets called.
//===============
void SVG_ShutdownGame(void) {
    gi.DPrintf("==== SVG_ShutdownGame ====\n");

    // Notify game object about the shutdown so it can let its members fire
    // their last act for cleaning up.
    game.Shutdown();

    // Free all memory that was tagged with TAG_LEVEL or TAG_GAME.
    gi.FreeTags(TAG_LEVEL);
    gi.FreeTags(TAG_GAME);
}

// TODO: Move elsewhere...
qboolean SVG_CanSaveGame(qboolean isDedicatedServer) { 
    IGamemode* gamemode = GetGameMode();

    if (!gamemode)
        return false;

    return gamemode->CanSaveGame(isDedicatedServer);
}

/**
*   @brief  Returns a pointer to the structure with all entry points
*           and global variables.
**/
extern "C" {
q_exported ServerGameExports* GetServerGameAPI(ServerGameImports* import)
{
    gi = *import;

    // Setup the API version.
    globals.apiversion = {
        SVGAME_API_VERSION_MAJOR,
        SVGAME_API_VERSION_MINOR,
        SVGAME_API_VERSION_POINT,
    };

    globals.Init = SVG_InitGame;
    globals.Shutdown = SVG_ShutdownGame;
    globals.SpawnEntities = SVG_SpawnEntities;

    globals.WriteGame = SVG_WriteGame;
    globals.ReadGame = SVG_ReadGame;
    globals.WriteLevel = SVG_WriteLevel;
    globals.ReadLevel = SVG_ReadLevel;

    globals.ClientThink = SVG_ClientThink;
    globals.ClientConnect = SVG_ClientConnect;
    globals.ClientUserinfoChanged = SVG_ClientUserinfoChanged;
    globals.ClientDisconnect = SVG_ClientDisconnect;
    globals.ClientBegin = SVG_ClientBegin;
    globals.ClientCommand = SVG_ClientCommand;

    globals.RunFrame = SVG_RunFrame;

    globals.ServerCommand = SVG_ServerCommand;

    globals.CanSaveGame = SVG_CanSaveGame;

    globals.entitySize = sizeof(Entity);

    return &globals;
}
}; // extern "C".



//=============================================================================
//
//	SVGame Wrappers, these are here so that functions in q_shared.cpp can do
//  their thing. This should actually, need a nicer solution such as using a
//  .lib instead, that signifies the "core" utility.
//
//=============================================================================
#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c can link
void Com_LPrintf(int32_t printType, const char *fmt, ...)
{
    va_list     argptr;
    char        text[MAX_STRING_CHARS];

    if (printType == PrintType::Developer) {
        return;
    }

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    gi.DPrintf("%s", text);
}

void Com_Error(int32_t errorType, const char *fmt, ...)
{
    va_list     argptr;
    char        text[MAX_STRING_CHARS];

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    gi.Error(ErrorType::Drop, "%s", text);
}
#endif

//=============================================================================
//
//	Initialization utility functions.
//
//=============================================================================
/**
*   @brief  Sets up all server game cvars and/or fetches those belonging to the engine.
**/
static void SVG_SetupCVars() {
    dev_show_physwarnings = gi.cvar("dev_show_physwarnings", "0", 0);

    //FIXME: sv_ prefix is wrong for these
    sv_rollspeed = gi.cvar("sv_rollspeed", "200", 0);
    sv_rollangle = gi.cvar("sv_rollangle", "2", 0);
    sv_maxvelocity = gi.cvar("sv_maxvelocity", "2000", 0);
    sv_gravity = gi.cvar("sv_gravity", std::to_string(Worldspawn::DEFAULT_GRAVITY).c_str(), 0);

    // Noset vars
    dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);

    // Latched vars
    sv_cheats = gi.cvar("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
    gi.cvar("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_LATCH);
    gi.cvar("gamedate", __DATE__, CVAR_SERVERINFO | CVAR_LATCH);

    maximumclients = gi.cvar("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
    maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
    gamemode = gi.cvar("gamemode", 0, 0);
    
    //deathmatch = gi.cvar("deathmatch", "0", CVAR_LATCH);
    //coop = gi.cvar("coop", "0", CVAR_LATCH);
    skill = gi.cvar("skill", "1", CVAR_SERVERINFO | CVAR_LATCH);

    // Change anytime vars
    gamemodeflags = gi.cvar("gamemodeflags", "0", CVAR_SERVERINFO);
    fraglimit = gi.cvar("fraglimit", "0", CVAR_SERVERINFO);
    timelimit = gi.cvar("timelimit", "0", CVAR_SERVERINFO);
    password = gi.cvar("password", "", CVAR_USERINFO);
    spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
    needpass = gi.cvar("needpass", "0", CVAR_SERVERINFO);
    filterban = gi.cvar("filterban", "1", 0);

    g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);

    run_pitch = gi.cvar("run_pitch", "0.002", 0);
    run_roll = gi.cvar("run_roll", "0.005", 0);
    bob_up = gi.cvar("bob_up", "0.005", 0);
    bob_pitch = gi.cvar("bob_pitch", "0.002", 0);
    bob_roll = gi.cvar("bob_roll", "0.002", 0);

    // Flood control
    flood_msgs = gi.cvar("flood_msgs", "4", 0);
    flood_persecond = gi.cvar("flood_persecond", "4", 0);
    flood_waitdelay = gi.cvar("flood_waitdelay", "10", 0);

    // DM map list
    sv_maplist = gi.cvar("sv_maplist", "", 0);

    // Monster footsteps.
    cl_monsterfootsteps = gi.cvar("cl_monsterfootsteps", "1", 0);
}



//
//=============================================================================
//
//	Main Entry Point callback functions for the SVGame DLL.
//
//=============================================================================
//
void SVG_SpawnEntities(const char* mapName, const char* entities, const char* spawnpoint) {
    // Acquire game world pointer.
    ServerGameWorld* gameworld = GetGameWorld();

    // Spawn entities.
    gameworld->PrepareBSPEntities(mapName, entities, spawnpoint);
}

/*
=================
SVG_EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void SVG_EndDMLevel(void)
{
    //Entity     *ent;
    char *s, *t, *f;
    static const char *seps = " ,\n\r";

    // Stay on same level flag
    if ((int)gamemodeflags->value & GameModeFlags::SameLevel) {
        SVG_HUD_BeginIntermission(nullptr);//SVG_CreateTargetChangeLevel(level.mapName));
        return;
    }

    // see if it's in the map list
    if (*sv_maplist->string) {
        s = strdup(sv_maplist->string);
        f = NULL;
        t = strtok(s, seps);
        while (t != NULL) {
            if (PH_StringCompare(t, level.mapName) == 0) {
                // it's in the list, go to the next one
                t = strtok(NULL, seps);
                if (t == NULL) { // end of list, go to first one
                    if (f == NULL) // there isn't a first one, same level
                        SVG_HUD_BeginIntermission(nullptr);//SVG_CreateTargetChangeLevel(level.mapName));
                    else
                        SVG_HUD_BeginIntermission(nullptr);//SVG_CreateTargetChangeLevel(f));
                } else
                    SVG_HUD_BeginIntermission(nullptr);//SVG_CreateTargetChangeLevel(f));
                free(s);
                return;
            }
            if (!f)
                f = t;
            t = strtok(NULL, seps);
        }
        free(s);
    }

    if (level.nextMap[0]) { // go to a specific map
        SVG_HUD_BeginIntermission(nullptr);//SVG_CreateTargetChangeLevel(level.mapName));
	} else {  // search for a changelevel
        //ent = SVG_Find(NULL, FOFS(classname), "target_changelevel");
        //if (!ent) {
        //    // the map designer didn't include a changelevel,
        //    // so create a fake ent that goes back to the same level
        //    SVG_HUD_BeginIntermission(SVG_CreateTargetChangeLevel(level.mapName));
        //    return;
        //}
        //SVG_HUD_BeginIntermission(ent);
    }
}


/*
=================
SVG_CheckNeedPass
=================
*/
void SVG_CheckNeedPass(void)
{
    int need;

    // if password or spectator_password has changed, update needpass
    // as needed
    if (password->modified || spectator_password->modified) {
        password->modified = spectator_password->modified = false;

        need = 0;

        if (*password->string && PH_StringCompare(password->string, "none"))
            need |= 1;
        if (*spectator_password->string && PH_StringCompare(spectator_password->string, "none"))
            need |= 2;

        gi.cvar_set("needpass", va("%d", need));
    }
}

/*
=================
SVG_CheckDMRules
=================
*/
void SVG_CheckDMRules(void)
{
    // Get server entities array.
    Entity* serverEntities = game.world->GetPODEntities();

    int         i;
    ServerClient   *cl;
    ServerClient* clients = game.GetClients();
    if (level.intermission.time != GameTime::zero())
        return;

    //if (!deathmatch->value)
    //    return;

    if (timelimit->value) {
		Frametime frameTimeLimit(timelimit->value);

		if (frameTimeLimit != Frametime::zero() && level.time >= frameTimeLimit) {
//        if (level.time >= Frametime(timelimit->value)) {
            gi.BPrintf(PRINT_HIGH, "Timelimit hit.\n");
            SVG_EndDMLevel();
            return;
        }
    }

    if (fraglimit->value) {
        for (i = 0 ; i < maximumclients->value ; i++) {
            cl = clients + i;
            if (!serverEntities[i + 1].inUse)
                continue;

            if (cl->respawn.score >= fraglimit->value) {
                gi.BPrintf(PRINT_HIGH, "Fraglimit hit.\n");
                SVG_EndDMLevel();
                return;
            }
        }
    }
}



/*







*/

/**
*	@brief	Checks the entity for whether its hashed classname still matches that of its current state.
*			If it does not match then it'll update the currentState's hashed classname.
**/
void SVG_UpdateHashedClassName(PODEntity *podEntity) {
	if (podEntity) {
		//if (podEntity->gameEntity) {
		//	// Keep it up to date with whatever the game entities type info 
		//	podEntity->previousState.hashedClassname = podEntity->currentState.hashedClassname;
		//	podEntity->currentState.hashedClassname = podEntity->gameEntity->GetTypeInfo()->hashedMapClass;
		//} else {
		//	podEntity->currentState.hashedClassname = 0;
		//}
		// The TypeInfo AllocateInstance has allocated a new game entity. We make sure to update the
		// 'wired' current state data to match it so that all client's can update their own local packet
		// entity derived instance.
		if ( podEntity->hashedClassname != podEntity->currentState.hashedClassname ) {
			podEntity->previousState.hashedClassname = podEntity->currentState.hashedClassname;
			podEntity->currentState.hashedClassname = podEntity->hashedClassname;
		}
	}
}

static void SVG_RunNonPusherEntitiesFrame() {
	    // Treat each object in turn "even the world gets a chance to Think", it does. 
	ServerGameWorld *gameWorld = GetGameWorld();
    for ( int32_t i = 1; i < globals.numberOfEntities; i++ ) {
		// Get POD Entity.
		PODEntity *podEntity = gameWorld->GetPODEntityByIndex( i );
		// Create its handle.
		SGEntityHandle geHandle = podEntity;
		// Validate it in order to get a safe game entity.
		GameEntity *gameEntity = ServerGameWorld::ValidateEntity( geHandle );
		
		// Make sure it has: [PODEntity ptr], [GameEntity ptr], [POD In Use], [NO Movetype: Push or Stop]
		// TODO: Should really be checking game entity inuse also hmm?
		if ( !podEntity || !gameEntity || /*!podEntity->inUse || */!gameEntity->IsInUse()
			|| ( gameEntity->GetMoveType() == MoveType::Push || gameEntity->GetMoveType() == MoveType::Stop ) ) {
            continue;
        }

        // Admer: entity was marked for removal at the previous tick
        if (podEntity && gameEntity && ( gameEntity->GetServerFlags() & EntityServerFlags::Remove ) ) {
            // Free server entity.
            game.world->FreePODEntity( podEntity );
            // Skip further processing of this entity, it's removed.
            continue;
        }


        // Let the level data know which entity we are processing right now.
        level.currentEntity = gameEntity;

        // Store previous(old) origin.
        gameEntity->SetOldOrigin(gameEntity->GetOrigin());

        // For non client entities: If the ground entity moved, make sure we are still on it
		if ( !gameEntity->GetClient() ) {
			// Validate handle.
			GameEntity *geGroundEntity = ServerGameWorld::ValidateEntity( gameEntity->GetGroundEntityHandle() );
			// Redo a Check for Ground if the link count does not match to its ground link count.
			if ( geGroundEntity && ( geGroundEntity->GetLinkCount() != gameEntity->GetGroundEntityLinkCount() ) ) {
				// Reset ground entity.
				gameEntity->SetGroundEntity( SGEntityHandle( nullptr, -1 ) );
				// Ensure we only check for it in case it is required (ie, certain movetypes do not want this...)
				if ( !(gameEntity->GetFlags() & ( EntityFlags::Swim | EntityFlags::Fly )) && (gameEntity->GetServerFlags() & EntityServerFlags::Monster) ) {
					// Check for a new ground entity that resides below this entity.
					SG_CheckGround( gameEntity ); //SVG_StepMove_CheckGround(gameEntity);
				}
			}
		}

        // Time to begin a server frame for all of our clients. (This has to ha
        if (i > 0 && i <= game.GetMaxClients()) {
            // Ensure the entity is in posession of a client that controls it.
            ServerClient* client = gameEntity->GetClient();
            if (!client) {
                continue;
            }

            // If the entity is NOT a SVGBasePlayer (sub-)class, skip.
            if (!gameEntity->GetTypeInfo()->IsSubclassOf(SVGBasePlayer::ClassInfo)) {
                continue;
            }

            // Last but not least, begin its server frame.
            GetGameMode()->ClientBeginServerFrame(dynamic_cast<SVGBasePlayer*>(gameEntity), client);

            // Continue to next iteration.
            continue;
        }

        // Last but not least, "run" process the entity.
	    //SVG_RunEntity(entityHandle);
		SGEntityHandle entityHandle = gameEntity;
		SG_RunEntity(entityHandle);

		// Update the entities Hashed Classname, it might've changed during logic processing.
		SVG_UpdateHashedClassName(podEntity);
    }
}
static void SVG_RunPusherEntitiesFrame() {
    // Treat each object in turn "even the world gets a chance to Think", it does. 
	ServerGameWorld *gameWorld = GetGameWorld();
    for ( int32_t i = 1; i < globals.numberOfEntities; i++ ) {
		// Get POD Entity.
		PODEntity *podEntity = gameWorld->GetPODEntityByIndex( i );
		// Create its handle.
		SGEntityHandle geHandle = podEntity;
		// Validate it in order to get a safe game entity.
		GameEntity *gameEntity = ServerGameWorld::ValidateEntity( geHandle );
		
		// If invalid for whichever reason, warn and continue to next iteration.
        if ( !podEntity || !gameEntity || /*!podEntity->inUse || */!gameEntity->IsInUse()
			|| ( gameEntity->GetMoveType() != MoveType::Push && gameEntity->GetMoveType() != MoveType::Stop ) ) {

            continue; //Com_DPrint("ClientGameEntites::RunFrame: Entity #%i is nullptr\n", entityNumber);
        }

        // Admer: entity was marked for removal at the previous tick
        if (podEntity && gameEntity && ( gameEntity->GetServerFlags() & EntityServerFlags::Remove ) ) {
            // Free server entity.
            game.world->FreePODEntity( podEntity );
            // Be sure to unset the server entity on this SVGBaseEntity for the current frame.
            // 
            // Other entities may wish to point at this entity for the current tick. By unsetting
            // the server entity we can prevent malicious situations from happening.
            //gameEntity->SetPODEntity(nullptr);

            // Skip further processing of this entity, it's removed.
            continue;
        }


        // Let the level data know which entity we are processing right now.
        level.currentEntity = gameEntity;

        // Store previous(old) origin.
        gameEntity->SetOldOrigin(gameEntity->GetOrigin());

        // Last but not least, "run" process the entity.
	    //SVG_RunEntity(entityHandle);
		SGEntityHandle entityHandle = gameEntity;
		SG_RunEntity(entityHandle);

		// Update the entities Hashed Classname, it might've changed during logic processing.
		SVG_UpdateHashedClassName(podEntity);
    }
}
/**
*	@brief	Advances the world's time by FRAMERATE_MS and iterates through all entities, client entities, non pushers,
*			and pusher entities, executing their 'think' and 'physics' tick logic.
**/
void SVG_RunFrame(void) {
	// Increment and use the frame number to calculate the current level time with.
	level.frameNumber++;
    level.time += FRAMERATE_MS; //level.frameNumber * FRAMERATE_MS;
	
    // Check for whether an intermission point wants to exit this level.
    if (level.intermission.exitIntermission) {
        GetGameMode()->OnLevelExit();
        return;
    }

	// #0: Run all non pusher entities first so that we don't end up with them moving in between push frames.
	// (That can cause them to get stick on rotators.)
	SVG_RunNonPusherEntitiesFrame();
	// #1: Now that they've had their chance to move for this frame, push them, or 'drive' them as 'riders' if standing on top.
	SVG_RunPusherEntitiesFrame();

    // See if it is time to end a deathmatch.
    SVG_CheckDMRules();
    // See if needpass needs updated.
    SVG_CheckNeedPass();
    // Build the playerstate_t structures for all players in this frame.
    SVG_ClientEndServerFrames();
}

/**
*	@brief	Called for all client at the end of the RunFrame function. Iterates over the clients in descending order.
*			(This allows us to adjust the player if it has been pushed around by other entities, or damage, etc.)
**/
void SVG_ClientEndServerFrames(void) {
    // Go through each client and calculate their final view for the state.
    // ( This allows us to adjust the player if it has been pushed around by other entities, or damage, etc. )
    ServerGameWorld* gameWorld = GetGameWorld();
	for (int32_t clientIndex = game.GetMaxClients(); clientIndex >= 0; clientIndex--) {
		// Client entities are always + 1, because the entities array starts indexing with 0 as Worldspawn.
		PODEntity *podClient = gameWorld->GetPODEntityByIndex( 1 + clientIndex );
        // Acquire player entity pointer.
        GameEntity *geClient = ServerGameWorld::ValidateEntity(podClient, true, true);

        // Make sure it is a valid pointer.
        if ( !geClient) { 
            continue;
        }
		// Make sure that the actual entity is of a SVGBasePlayer derived type.
		if ( !geClient->IsSubclassOf<SVGBasePlayer>() ) {
			continue;
		}

        // Save to cast now.
        SVGBasePlayer *gePlayer = static_cast< SVGBasePlayer* >( geClient );

        // Acquire server client.
        ServerClient *client = gePlayer->GetClient();

        // Notify game mode about this client ending its server frame.
        gameWorld->GetGameMode()->ClientEndServerFrame( gePlayer, client );
    }
}