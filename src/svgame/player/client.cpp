/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

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
// General.
#include "../g_local.h"     // SVGame.
#include "../chasecamera.h" // Chase Camera.
#include "../effects.h"     // Effects.
#include "../entities.h"    // Entities.
#include "../utils.h"       // Util funcs.
#include "client.h"         // Include Player Client header.
#include "hud.h"            // Include HUD header.
#include "view.h"           // View header.
#include "weapons.h"

// ClassEntities.
#include "../entities/base/SVGBaseEntity.h"
#include "../entities/base/PlayerClient.h"

// Game modes.
#include "../gamemodes/IGameMode.h"

// Shared Game.
#include "sharedgame/sharedgame.h" // Include SG Base.
#include "sharedgame/pmove.h"   // Include SG PMove.
#include "animations.h"         // Include Player Client Animations.

void SVG_ClientUserinfoChanged(Entity *ent, char *userinfo);


//
// Gross, ugly, disgustuing hack section
//

// this function is an ugly as hell hack to fix some map flaws
//
// the coop spawn spots on some maps are SNAFU.  There are coop spots
// with the wrong targetName as well as spots with no name at all
//
// we use carnal knowledge of the maps to fix the coop spot targetnames to match
// that of the nearest named single player spot

void SP_FixCoopSpots(Entity *self)
{
    Entity *spot;
    vec3_t  d;

    spot = NULL;

    while (1) {
        spot = SVG_Find(spot, FOFS(className), "info_player_start");
        if (!spot)
            return;
        if (!spot->targetName)
            continue;
        d = self->state.origin - spot->state.origin;
        if (vec3_length(d) < 384) {
            if ((!self->targetName) || Q_stricmp(self->targetName, spot->targetName) != 0) {
                //              gi.DPrintf("FixCoopSpots changed %s at %s targetName from %s to %s\n", self->className, Vec3ToString(self->state.origin), self->targetName, spot->targetName);
                self->targetName = spot->targetName;
            }
            return;
        }
    }
}

//=======================================================================

void SVG_TossClientWeapon(PlayerClient *playerClient)
{
    gitem_t     *item;
    Entity     *drop;
    float       spread = 1.5f;

    if (!deathmatch->value)
        return;

    item = playerClient->GetActiveWeapon();
    if (!playerClient->GetClient()->persistent.inventory[playerClient->GetClient()->ammoIndex])
        item = NULL;
    if (item && (strcmp(item->pickupName, "Blaster") == 0))
        item = NULL;

    if (item) {
        //playerClient->GetClient()->aimAngles[vec3_t::Yaw] -= spread;
        //drop = SVG_DropItem(playerClient->GetServerEntity(), item);
        //playerClient->GetClient()->aimAngles[vec3_t::Yaw] += spread;
        //drop->spawnFlags = ItemSpawnFlags::DroppedPlayerItem;
    }
}

//=======================================================================

/*
==============
SVG_InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void SVG_InitClientPersistant(GameClient *client)
{
    gitem_t     *item = NULL;

    if (!client)
        return;

    //memset(&client->persistent, 0, sizeof(client->persistent));
    client->persistent = {};

    item = SVG_FindItemByPickupName("Blaster");
    client->persistent.selectedItem = ITEM_INDEX(item);
    client->persistent.inventory[client->persistent.selectedItem] = 1;

    client->persistent.activeWeapon = item;

    client->persistent.health         = 100;
    client->persistent.maxHealth     = 100;

    client->persistent.maxBullets    = 200;
    client->persistent.maxShells     = 100;
    client->persistent.maxRockets    = 50;
    client->persistent.maxGrenades   = 50;
    client->persistent.maxCells      = 200;
    client->persistent.maxSlugs      = 50;

    client->persistent.isConnected = true;
}


void SVG_InitClientRespawn(GameClient *client)
{
    if (!client)
        return;

    client->respawn = {};
    client->respawn.enterGameFrameNumber = level.frameNumber;
    client->respawn.persistentCoopRespawn = client->persistent;
}

/*
==================
SVG_SaveClientData

Some information that should be persistant, like health,
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SVG_SaveClientData(void)
{
    int     i;
    Entity *ent;

    for (i = 0 ; i < game.maximumClients ; i++) {
        ent = &g_entities[1 + i];
        if (!ent->inUse)
            continue;
        if (!ent->classEntity)
            continue;
        game.clients[i].persistent.health = ent->classEntity->GetHealth();
        game.clients[i].persistent.maxHealth = ent->classEntity->GetMaxHealth();
        game.clients[i].persistent.savedFlags = (ent->classEntity->GetFlags() & (EntityFlags::GodMode | EntityFlags::NoTarget | EntityFlags::PowerArmor));
        if (coop->value && ent->client)
            game.clients[i].persistent.score = ent->client->respawn.score;
    }
}

void SVG_FetchClientData(Entity *ent)
{
    if (!ent)
        return;

    if (!ent->classEntity)
        return;

    ent->classEntity->SetHealth(ent->client->persistent.health);
    ent->classEntity->SetMaxHealth(ent->client->persistent.maxHealth);
    ent->classEntity->SetFlags(ent->classEntity->GetFlags() | ent->client->persistent.savedFlags);
    if (coop->value && ent->client)
        ent->client->respawn.score = ent->client->persistent.score;
}



/*
=======================================================================

SelectSpawnPoint

=======================================================================
*/
/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
void SelectSpawnPoint(Entity *ent, vec3_t &origin, vec3_t &angles)
{
    SVGBaseEntity *spot = nullptr;

    // Find a single player start spot
    if (!spot) {
        //while ((spot = SVG_FindEntityByKeyValue("classname", "info_player_start", spot)) != nullptr) {
        //    if (!game.spawnpoint[0] && spot->GetTargetName().empty())
        //        break;

        //    if (!game.spawnpoint[0] || spot->GetTargetName().empty())
        //        continue;

        //    //if (Q_stricmp(game.spawnpoint, spot->GetTargetName()) == 0)
        //    if (spot->GetTargetName() == game.spawnpoint)
        //        break;
        //}

        if (!spot) {
            if (!game.spawnpoint[0]) {
                // there wasn't a spawnpoint without a target, so use any
                spot = SVG_FindEntityByKeyValue("classname", "info_player_start", spot);
            }
            if (!spot)
                gi.Error("Couldn't find spawn point %s", game.spawnpoint);
        }
    }

    if (spot) {
        origin = spot->GetOrigin();
        origin.z += 9;
        angles = spot->GetAngles();
    }
}

//======================================================================

void body_die(Entity *self, Entity *inflictor, Entity *attacker, int damage, const vec3_t& point)
{
    //int n;

    //if (self->classEntity && self->classEntity->GetHealth() < -40) {
    //    gi.Sound(self, CHAN_BODY, gi.SoundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);
    //    for (n = 0; n < 4; n++)
    //        SVG_ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
    //    self->state.origin.z -= 48;
    //    SVG_ThrowClientHead(self, damage);
    //    self->takeDamage = TakeDamage::No;
    //}
}

void SVG_RespawnClient(Entity *self)
{
    if (deathmatch->value || coop->value) {
        // Spectator's don't leave bodies
        if (self->classEntity->GetMoveType() != MoveType::NoClip && self->classEntity->GetMoveType() != MoveType::Spectator)
            game.gameMode->SpawnClientCorpse(self->classEntity);

        self->serverFlags &= ~EntityServerFlags::NoClient;
        SVG_PutClientInServer(self);

        // add a teleportation effect
        self->state.eventID = EntityEvent::PlayerTeleport;

        // hold in place briefly
        self->client->playerState.pmove.flags = PMF_TIME_TELEPORT;
        self->client->playerState.pmove.time = 14;

        self->client->respawnTime = level.time;

        return;
    }

    // restart the entire server
    gi.AddCommandString("pushmenu loadgame\n");
}

/*
* only called when persistent.isSpectator changes
* note that resp.isSpectator should be the opposite of persistent.isSpectator here
*/
void spectator_respawn(Entity *ent)
{
    int i, numspec;

    // If the user wants to become a isSpectator, make sure he doesn't
    // exceed max_spectators
    if (ent->client->persistent.isSpectator) {
        // Test if the isSpectator password was correct, if not, error and return.
        char *value = Info_ValueForKey(ent->client->persistent.userinfo, "isspectator");
        if (*spectator_password->string &&
            strcmp(spectator_password->string, "none") &&
            strcmp(spectator_password->string, value)) {
            // Report error message by centerprinting it to client.
            gi.CPrintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");

            // Enable isSpectator state.
            ent->client->persistent.isSpectator = false;

            // Let the client go out of its isSpectator mode by using a StuffCmd.
            gi.StuffCmd(ent, "isspectator 0\n");
            return;
        }

        // Count actual active spectators
        for (i = 1, numspec = 0; i <= maximumClients->value; i++)
            if (g_entities[i].inUse && g_entities[i].client->persistent.isSpectator)
                numspec++;

        if (numspec >= maxspectators->value) {
            // Report error message by centerprinting it to client.
            gi.CPrintf(ent, PRINT_HIGH, "Server isSpectator limit is full.\n");

            // Enable isSpectator state.
            ent->client->persistent.isSpectator = false;

            // Let the client go out of its isSpectator mode by using a StuffCmd.
            gi.StuffCmd(ent, "isspectator 0\n");
            return;
        }
    } else {
        // He was a isSpectator and wants to join the game 
        // He must have the right password
        // Test if the isSpectator password was correct, if not, error and return.
        char *value = Info_ValueForKey(ent->client->persistent.userinfo, "password");
        if (*password->string && strcmp(password->string, "none") &&
            strcmp(password->string, value)) {
            // Report error message by centerprinting it to client.
            gi.CPrintf(ent, PRINT_HIGH, "Password incorrect.\n");

            // Enable isSpectator state.
            ent->client->persistent.isSpectator = true;

            // Let the client go in its isSpectator mode by using a StuffCmd.
            gi.StuffCmd(ent, "isspectator 1\n");
            return;
        }
    }

    // clear client on respawn
    ent->client->respawn.score = ent->client->persistent.score = 0;

    ent->serverFlags &= ~EntityServerFlags::NoClient;
    SVG_PutClientInServer(ent);

    // add a teleportation effect
    if (!ent->client->persistent.isSpectator)  {
        // send effect
        gi.WriteByte(SVG_CMD_MUZZLEFLASH);
        gi.WriteShort(ent - g_entities);
        gi.WriteByte(MuzzleFlashType::Login);
        gi.Multicast(ent->state.origin, MultiCast::PVS);

        // hold in place briefly
        ent->client->playerState.pmove.flags = PMF_TIME_TELEPORT;
        ent->client->playerState.pmove.time = 14;
    }

    ent->client->respawnTime = level.time;

    if (ent->client->persistent.isSpectator)
        gi.BPrintf(PRINT_HIGH, "%s has moved to the sidelines\n", ent->client->persistent.netname);
    else
        gi.BPrintf(PRINT_HIGH, "%s joined the game\n", ent->client->persistent.netname);
}

//==============================================================


/*
===========
SVG_PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void SVG_PutClientInServer(Entity *ent)
{
    int     index;
    vec3_t  spawn_origin, spawn_angles;
    GameClient   *client;
    int     i;

    ClientRespawnData    resp;

    // ensure the entity is valid
    if (!ent) {
        return;
    }

    // find a spawn point
    // do it before setting health back up, so farthest
    // ranging doesn't count this client
    SelectSpawnPoint(ent, spawn_origin, spawn_angles);

    index = ent - g_entities - 1;
    client = ent->client;

    // deathmatch wipes most client data every spawn
    if (deathmatch->value) {
        char        userinfo[MAX_INFO_STRING];

        // Store the respawn values.
        resp = client->respawn;

        // Store user info.
        memcpy(userinfo, client->persistent.userinfo, sizeof(userinfo));

        // Initialize a fresh client persistent state.
        SVG_InitClientPersistant(client);

        // Check for changed user info.
        SVG_ClientUserinfoChanged(ent, userinfo);
        //    } else if (coop->value) {
        ////      int         n;
        //        char        userinfo[MAX_INFO_STRING];
        //
        //        resp = client->respawn;
        //        memcpy(userinfo, client->persistent.userinfo, sizeof(userinfo));
        //        // this is kind of ugly, but it's how we want to handle keys in coop
        ////      for (n = 0; n < game.numberOfItems; n++)
        ////      {
        ////          if (itemlist[n].flags & ItemFlags::IsKey)
        ////              resp.persistentCoopRespawn.inventory[n] = client->persistent.inventory[n];
        ////      }
        //        client->persistent = resp.persistentCoopRespawn;
        //        SVG_ClientUserinfoChanged(ent, userinfo);
        //        if (resp.score > client->persistent.score)
        //            client->persistent.score = resp.score;
    } else {
        resp = {};
    }

    // Store persistent client data.
    ClientPersistantData saved = client->persistent;

    // Reset the client data.
    *client = {};

    // Restore persistent client data.
    client->persistent = saved;

    // In case of death, initialize a fresh client persistent data.
    if (client->persistent.health <= 0)
        SVG_InitClientPersistant(client);

    // Last but not least, respawn.
    client->respawn = resp;

    //
    // Spawn client class entity.
    //
    //    SVG_FreeClassEntity(ent);

    //ent->className = "PlayerClient";
    PlayerClient* playerClientEntity = (PlayerClient*)(ent->classEntity);
    playerClientEntity->SetClient(&game.clients[index]);
    playerClientEntity->Precache();
    playerClientEntity->Spawn();
    playerClientEntity->PostSpawn();

    // copy some data from the client to the entity
    SVG_FetchClientData(ent);

    //
    // clear entity values
    //
    //ent->groundEntityPtr = NULL;
    //ent->client = &game.clients[index];
    //ent->takeDamage = TakeDamage::Aim;
    //ent->classEntity->SetMoveType(MoveType::Walk);
    //ent->viewHeight = 22;
    //ent->inUse = true;
    //ent->className = "PlayerClient";
    //ent->mass = 200;
    //ent->solid = Solid::BoundingBox;
    //ent->deadFlag = DEAD_NO;
    //ent->airFinishedTime = level.time + 12;
    //ent->clipMask = CONTENTS_MASK_PLAYERSOLID;
    //ent->model = "players/male/tris.md2";
    ////ent->Pain = SVG_Player_Pain;
    ////ent->Die = SVG_Player_Die;
    //ent->waterLevel = 0;
    //ent->waterType = 0;
    //ent->flags &= ~EntityFlags::NoKnockBack;
    //ent->serverFlags &= ~EntityServerFlags::DeadMonster;

    //ent->mins = vec3_scale(PM_MINS, PM_SCALE);
    //ent->maxs = vec3_scale(PM_MAXS, PM_SCALE);
    //ent->velocity = vec3_zero();

    // Clear playerstate values
    //memset(&ent->client->playerState, 0, sizeof(client->playerState));
    client->playerState = {};

    // Assign spawn origin to player state origin.
    client->playerState.pmove.origin = spawn_origin;

    // Assign spawn origin to the entity state origin, ensure that it is off-ground.
    ent->state.origin = ent->state.oldOrigin = spawn_origin + vec3_t{ 0.f, 0.f, 1.f };

    // Set FOV, fixed, or custom.
    if (deathmatch->value && ((int)gamemodeflags->value & GameModeFlags::FixedFOV)) {
        client->playerState.fov = 90;
    } else {
        client->playerState.fov = atoi(Info_ValueForKey(client->persistent.userinfo, "fov"));
        if (client->playerState.fov < 1)
            client->playerState.fov = 90;
        else if (client->playerState.fov > 160)
            client->playerState.fov = 160;
    }

    client->playerState.gunIndex = gi.ModelIndex(client->persistent.activeWeapon->viewModel);

    // Clear certain entity state values
    ent->state.effects = 0;
    ent->state.modelIndex = 255;        // Will use the skin specified model
    ent->state.modelIndex2 = 255;       // Custom gun model
                                        // sknum is player num and weapon number
                                        // weapon number will be added in changeweapon
    ent->state.skinNumber = ent - g_entities - 1;

    ent->state.frame = 0;

    // set the delta angle
    for (i = 0 ; i < 3 ; i++) {
        client->playerState.pmove.deltaAngles[i] = spawn_angles[i] - client->respawn.commandViewAngles[i];
    }

    ent->state.angles[vec3_t::Pitch] = 0;
    ent->state.angles[vec3_t::Yaw] = spawn_angles[vec3_t::Yaw];
    ent->state.angles[vec3_t::Roll] = 0;

    // Setup angles for playerstate and client aim.
    client->playerState.pmove.viewAngles = ent->state.angles; // VectorCopy(ent->state.angles, client->playerState.pmove.viewAngles);
    client->aimAngles = ent->state.angles; //VectorCopy(ent->state.angles, client->aimAngles);

                                           // spawn a isSpectator
    if (client->persistent.isSpectator) {
        client->chaseTarget = NULL;

        client->respawn.isSpectator = true;

        ent->classEntity->SetMoveType(MoveType::Spectator);
        ent->solid = Solid::Not;
        ent->serverFlags |= EntityServerFlags::NoClient;
        ent->client->playerState.gunIndex = 0;
        gi.LinkEntity(ent);
        return;
    } else
        client->respawn.isSpectator = false;

    if (!SVG_KillBox(ent->classEntity)) {
        // could't spawn in?
    }

    gi.LinkEntity(ent);

    // force the current weapon up
    client->newWeapon = client->persistent.activeWeapon;
    SVG_ChangeWeapon((PlayerClient*)ent->classEntity);
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
deathmatch mode, so clear everything out before starting them.
=====================
*/
void SVG_ClientBeginDeathmatch(Entity *ent)
{
    SVG_InitEntity(ent);

    SVG_InitClientRespawn(ent->client);

    // locate ent at a spawn point
    SVG_PutClientInServer(ent);

    if (level.intermission.time) {
        HUD_MoveClientToIntermission(ent);
    } else {
        // send effect
        gi.WriteByte(SVG_CMD_MUZZLEFLASH);
        gi.WriteShort(ent - g_entities);
        gi.WriteByte(MuzzleFlashType::Login);
        gi.Multicast(ent->state.origin, MultiCast::PVS);
    }

    gi.BPrintf(PRINT_HIGH, "%s entered the game\n", ent->client->persistent.netname);

    // make sure all view stuff is valid
    SVG_ClientEndServerFrame((PlayerClient*)ent->classEntity);
}


/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
extern void DebugShitForEntitiesLulz();

void SVG_ClientBegin(Entity *ent)
{
    // Fetch this entity's client.
    ent->client = game.clients + (ent - g_entities - 1);

    // Let the game mode decide from here on out.
    game.gameMode->ClientBegin(ent);

    // Called to make sure all view stuff is valid
    SVG_ClientEndServerFrame((PlayerClient*)ent->classEntity);
}

/*
===========
SVG_ClientUserinfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void SVG_ClientUserinfoChanged(Entity *ent, char *userinfo)
{
    char    *s;
    int     playernum;

    // check for malformed or illegal info strings
    if (!Info_Validate(userinfo)) {
        strcpy(userinfo, "\\name\\badinfo\\skin\\male/grunt");
    }

    // set name
    s = Info_ValueForKey(userinfo, "name");
    strncpy(ent->client->persistent.netname, s, sizeof(ent->client->persistent.netname) - 1);

    // set isSpectator
    s = Info_ValueForKey(userinfo, "isspectator");
    // spectators are only supported in deathmatch
    if (deathmatch->value && *s && strcmp(s, "0"))
        ent->client->persistent.isSpectator = true;
    else
        ent->client->persistent.isSpectator = false;

    // set skin
    s = Info_ValueForKey(userinfo, "skin");

    playernum = ent - g_entities - 1;

    // combine name and skin into a configstring
    gi.configstring(ConfigStrings::PlayerSkins + playernum, va("%s\\%s", ent->client->persistent.netname, s));

    // fov
    if (deathmatch->value && ((int)gamemodeflags->value & GameModeFlags::FixedFOV)) {
        ent->client->playerState.fov = 90;
    } else {
        ent->client->playerState.fov = atoi(Info_ValueForKey(userinfo, "fov"));
        if (ent->client->playerState.fov < 1)
            ent->client->playerState.fov = 90;
        else if (ent->client->playerState.fov > 160)
            ent->client->playerState.fov = 160;
    }

    // handedness
    s = Info_ValueForKey(userinfo, "hand");
    if (strlen(s)) {
        ent->client->persistent.hand = atoi(s);
    }

    // save off the userinfo in case we want to check something later
    strncpy(ent->client->persistent.userinfo, userinfo, sizeof(ent->client->persistent.userinfo) - 1);
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean SVG_ClientConnect(Entity *ent, char *userinfo)
{
    // Check if the game mode allows for this specific client to connect to it.
    if (!game.gameMode->ClientCanConnect(ent, userinfo))
        return false;

    // If we reached this point, game mode states that they can connect.
    // As such, we assign the client to this server entity.
    ent->client = game.clients + (ent - g_entities - 1);

    // If there is already a body waiting for us (a loadgame), just
    // take it, otherwise spawn one from scratch
    if (ent->inUse == false) {
        // Clear the respawning variables
        SVG_InitClientRespawn(ent->client);
        if (!game.autoSaved || !ent->client->persistent.activeWeapon)
            SVG_InitClientPersistant(ent->client);
    }

    // Check for changed user info.
    SVG_ClientUserinfoChanged(ent, userinfo);

    // Inform the game mode about it.
    game.gameMode->ClientConnect(ent);

    ent->serverFlags = 0; // make sure we start with known default
    ent->client->persistent.isConnected = true;
    return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void SVG_ClientDisconnect(Entity *ent)
{
    //int     playernum;

    // Ensure this entity has a client.
    if (!ent->client)
        return;
    // Ensure it has a class entity also.
    if (!ent->classEntity)
        return;

    // Since it does, we pass it on to the game mode.
    game.gameMode->ClientDisconnect((PlayerClient*)ent->classEntity);

    // FIXME: don't break skins on corpses, etc
    //playernum = ent-g_entities-1;
    //gi.configstring (ConfigStrings::PlayerSkins+playernum, "");
}


//==============================================================


Entity *pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t q_gameabi PM_Trace(const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end)
{
    if (pm_passent->classEntity && pm_passent->classEntity->GetHealth() > 0)
        return gi.Trace(start, mins, maxs, end, pm_passent, CONTENTS_MASK_PLAYERSOLID);
    else
        return gi.Trace(start, mins, maxs, end, pm_passent, CONTENTS_MASK_DEADSOLID);
}

unsigned CheckBlock(void *b, int c)
{
    int v, i;
    v = 0;
    for (i = 0 ; i < c ; i++)
        v += ((byte *)b)[i];
    return v;
}
void PrintPMove(PlayerMove *pm)
{
    unsigned    c1, c2;

    c1 = CheckBlock(&pm->state, sizeof(pm->state));
    c2 = CheckBlock(&pm->moveCommand, sizeof(pm->moveCommand));
    Com_Printf("sv %3i:%i %i\n", pm->moveCommand.input.impulse, c1, c2);
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void SVG_ClientThink(Entity *serverEntity, ClientMoveCommand *moveCommand)
{
    GameClient* client = nullptr;
    PlayerClient *classEntity = nullptr;
    Entity* other = nullptr;


    PlayerMove pm = {};

    // Sanity checks.
    if (!serverEntity) {
        Com_Error(ErrorType::ERR_DROP, "%s: has a NULL *ent!\n", __FUNCTION__);
    }
    if (!serverEntity->client)
        Com_Error(ErrorType::ERR_DROP, "%s: *ent has no client to think with!\n", __FUNCTION__);

    if (!serverEntity->classEntity)
        return;

    // Store the current entity to be run from SVG_RunFrame.
    level.currentEntity = serverEntity->classEntity;

    // Fetch the entity client.
    client = serverEntity->client;

    // Fetch the class entity.
    classEntity = (PlayerClient*)serverEntity->classEntity;

    if (level.intermission.time) {
        client->playerState.pmove.type = EnginePlayerMoveType::Freeze;
        // can exit intermission after five seconds
        if (level.time > level.intermission.time + 5.0
            && (moveCommand->input.buttons & BUTTON_ANY))
            level.intermission.exitIntermission = true;
        return;
    }

    pm_passent = serverEntity;

    if (client->chaseTarget) {
        // Angles are fetched from the client we are chasing.
        client->respawn.commandViewAngles[0] = moveCommand->input.viewAngles[0];
        client->respawn.commandViewAngles[1] = moveCommand->input.viewAngles[1];
        client->respawn.commandViewAngles[2] = moveCommand->input.viewAngles[2];
    } else {

        // set up for pmove
        memset(&pm, 0, sizeof(pm));

        if ( classEntity->GetMoveType() == MoveType::NoClip )
            client->playerState.pmove.type = PlayerMoveType::Noclip;
        else if ( classEntity->GetMoveType() == MoveType::Spectator )
            client->playerState.pmove.type = PlayerMoveType::Spectator;
        else if (classEntity->GetModelIndex() != 255 )
            client->playerState.pmove.type = EnginePlayerMoveType::Gib;
        else if ( classEntity->GetDeadFlag() )
            client->playerState.pmove.type = EnginePlayerMoveType::Dead;
        else
            client->playerState.pmove.type = PlayerMoveType::Normal;

        client->playerState.pmove.gravity = sv_gravity->value;

        // Copy over the latest playerstate its pmove state.
        pm.state = client->playerState.pmove;

        // Move over entity state values into the player move state so it is up to date.
        pm.state.origin = classEntity->GetOrigin();
        pm.state.velocity = classEntity->GetVelocity();
        pm.moveCommand = *moveCommand;
        if (classEntity->GetGroundEntity())
            pm.groundEntityPtr = classEntity->GetGroundEntity()->GetServerEntity();
        else
            pm.groundEntityPtr = nullptr;

        pm.Trace = PM_Trace;
        pm.PointContents = gi.PointContents;

        // perform a pmove
        PMove(&pm);

        // Save client pmove results.
        client->playerState.pmove = pm.state;

        // Move over needed results to the entity and its state.
        classEntity->SetOrigin(pm.state.origin);
        classEntity->SetVelocity(pm.state.velocity);
        classEntity->SetMins(pm.mins);
        classEntity->SetMaxs(pm.maxs);
        classEntity->SetViewHeight(pm.state.viewOffset[2]);
        classEntity->SetWaterLevel(pm.waterLevel);
        classEntity->SetWaterType(pm.waterType);

        // Check for jumping sound.
        if (classEntity->GetGroundEntity() && !pm.groundEntityPtr && (pm.moveCommand.input.upMove >= 10) && (pm.waterLevel == 0)) {
            gi.Sound(serverEntity, CHAN_VOICE, gi.SoundIndex("*jump1.wav"), 1, ATTN_NORM, 0);
            SVG_PlayerNoise(classEntity, classEntity->GetOrigin(), PNOISE_SELF);
        }

        if (pm.groundEntityPtr)
            classEntity->SetGroundEntity(pm.groundEntityPtr->classEntity);
        else
            classEntity->SetGroundEntity(nullptr);

        // Copy over the user command angles so they are stored for respawns.
        // (Used when going into a new map etc.)
        client->respawn.commandViewAngles[0] = moveCommand->input.viewAngles[0];
        client->respawn.commandViewAngles[1] = moveCommand->input.viewAngles[1];
        client->respawn.commandViewAngles[2] = moveCommand->input.viewAngles[2];

        // Store entity link count in case we have a ground entity pointer.
        if (pm.groundEntityPtr)
            classEntity->SetGroundEntityLinkCount(pm.groundEntityPtr->linkCount);

        // Special treatment for angles in case we are dead. Target the killer entity yaw angle.
        if (classEntity->GetDeadFlag()) {
            client->playerState.pmove.viewAngles[vec3_t::Roll] = 40;
            client->playerState.pmove.viewAngles[vec3_t::Pitch] = -15;
            client->playerState.pmove.viewAngles[vec3_t::Yaw] = client->killerYaw;
        } else {
            // Otherwise, store the resulting view angles accordingly.
            client->aimAngles = pm.viewAngles;
            client->playerState.pmove.viewAngles = pm.viewAngles;
        }

        // Link it back in for collision testing.
        classEntity->LinkEntity();

        // Only check for trigger and object touches if not one of these movetypes.
        if (classEntity->GetMoveType() != MoveType::NoClip && classEntity->GetMoveType() != MoveType::Spectator)
            UTIL_TouchTriggers(classEntity);

        // touch other objects
        int32_t i = 0;
        int32_t j = 0;
        for (i = 0 ; i < pm.numTouchedEntities; i++) {
            other = pm.touchedEntities[i];
            for (j = 0 ; j < i ; j++)
                if (pm.touchedEntities[j] == other)
                    break;
            if (j != i)
                continue;   // duplicated
                            //if (!other->Touch)
                            //    continue;
                            //other->Touch(other, ent, NULL, NULL);
            if (!other->classEntity)
                continue;
            other->classEntity->Touch(other->classEntity, classEntity, NULL, NULL);
        }

    }

    client->oldButtons = client->buttons;
    client->buttons = moveCommand->input.buttons;
    client->latchedButtons |= client->buttons & ~client->oldButtons;

    // save light level the player is standing on for
    // monster sighting AI
    //ent->lightLevel = moveCommand->input.lightLevel;

    // fire weapon from final position if needed
    if (client->latchedButtons & BUTTON_ATTACK) {
        if (client->respawn.isSpectator) {

            client->latchedButtons = 0;

            if (client->chaseTarget) {
                client->chaseTarget = NULL;
                client->playerState.pmove.flags &= ~PMF_NO_PREDICTION;
            } else
                SVG_GetChaseTarget(classEntity);

        } else if (!client->weaponThunk) {
            client->weaponThunk = true;
            SVG_ThinkWeapon(classEntity);
        }
    }

    if (client->respawn.isSpectator) {
        if (moveCommand->input.upMove >= 10) {
            if (!(client->playerState.pmove.flags & PMF_JUMP_HELD)) {
                client->playerState.pmove.flags |= PMF_JUMP_HELD;
                if (client->chaseTarget)
                    SVG_ChaseNext(classEntity);
                else
                    SVG_GetChaseTarget(classEntity);
            }
        } else
            client->playerState.pmove.flags &= ~PMF_JUMP_HELD;
    }

    // update chase cam if being followed
    for (int i = 1; i <= maximumClients->value; i++) {
        other = g_entities + i;
        if (other->inUse && other->client->chaseTarget == serverEntity)
            SVG_UpdateChaseCam(classEntity);
    }
}