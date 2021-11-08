/*
// LICENSE HERE.

//
// DefaultGameMode.cpp
//
//
*/
#include "../g_local.h"          // SVGame.
#include "../effects.h"     // Effects.
#include "../entities.h"    // Entities.
#include "../utils.h"       // Util funcs.

// Server Game Base Entity.
#include "../entities/base/SVGBaseEntity.h"
#include "../entities/base/BodyCorpse.h"
#include "../entities/base/PlayerClient.h"

// Weapons.h
#include "../player/client.h"
#include "../player/hud.h"
#include "../player/weapons.h"

// Game Mode.
#include "DefaultGameMode.h"

//
// Constructor/Deconstructor.
//
DefaultGameMode::DefaultGameMode() {
    // Defaults.
    meansOfDeath = 0;
}
DefaultGameMode::~DefaultGameMode() {

}

//
// Interface functions. 
//
//===============
// DefaultGameMode::GetEntityTeamName
//
// Assigns the teamname to the string passed, returns false in case the entity
// is not part of a team at all.
//===============
qboolean DefaultGameMode::GetEntityTeamName(SVGBaseEntity* ent, std::string &teamName) {
    // Placeholder.
    teamName = "";

    return false;

    //// We'll assume that this entity needs to have a client.
    //if (!ent->GetClient())
    //    return "";

    //// Fetch the 'skin' info_valueforkey of the given client.
    //std::string clientSkin = Info_ValueForKey(ent->GetClient()->persistent.userinfo, "skin");

    //// Start scanning for a /, in case there is none, we can just return the skin as is.
    //auto slashPosition = clientSkin.find_last_of('/');
    //if (slashPosition == std::string::npos)
    //    return clientSkin;

    //// Since we did find one if we reach this code, we'll check waht our game mode flags demand.
    //if (gamemodeflags->integer & GameModeFlags::ModelTeams) {
    //    return clientSkin;
    //}

    //// Otherwise, in case we got skin teams... Return the skin specific part as team name.
    //if (gamemodeflags->integer & GameModeFlags::SkinTeams) {
    //    return clientSkin.substr(slashPosition);
    //}

    //// We should never reach this point, but... there just in case.
    //return "";
}

//===============
// DefaultGameMode::OnSameTeam
//
// Returns false either way, because yes, there is no... team in this case.
// PS: ClientTeam <-- weird function, needs C++-fying and oh.. it stinks anyhow.
//===============
qboolean DefaultGameMode::OnSameTeam(SVGBaseEntity* ent1, SVGBaseEntity* ent2) {
    //// There is only a reason to check for this in case these specific
    //// game mode flags are set.
    //if (!((int)(gamemodeflags->value) & (GameModeFlags::ModelTeams | GameModeFlags::SkinTeams)))
    //    return false;

    //// Fetch the team names of both entities.
    //std::string teamEntity1 = GetEntityTeamName(ent1);
    //std::string teamEntity2 = GetEntityTeamName(ent2);

    //// In case they are equal, return true.
    //if (!teamEntity1.empty() && !teamEntity2.empty())
    //    if (teamEntity1 != "" && teamEntity2 != "")
    //        if (teamEntity1 == teamEntity2)
    //            return true;

    // If we reached this point, we're done, no going on.
    return false;
}

//===============
// DefaultGameMode::CanDamage
//
//===============
qboolean DefaultGameMode::CanDamage(SVGBaseEntity* target, SVGBaseEntity* inflictor) {
    vec3_t  destination;
    SVGTrace trace;

    // WID: Admer, why the fuck did they rush hour these comments all the time?
    // bmodels need special checking because their origin is 0,0,0 <-- is bad.
    //
    // Solid entities need a special check, as their origin is usually 0,0,0
    // Exception to the above: the solid entity moves or has an origin brush
    if (target->GetMoveType() == MoveType::Push) {
        // Calculate destination.
        destination = target->GetAbsoluteMin() + target->GetAbsoluteMax();
        destination = vec3_scale(destination, 0.5f);
        trace = SVG_Trace(inflictor->GetOrigin(), vec3_zero(), vec3_zero(), destination, inflictor, CONTENTS_MASK_SOLID);
        if (trace.fraction == 1.0)
            return true;
        if (trace.ent == target)
            return true;
        return false;
    }

    // From here on we start tracing in various directions. Look at the code yourself to figure that one out...
    trace = SVG_Trace(inflictor->GetOrigin(), vec3_zero(), vec3_zero(), target->GetOrigin(), inflictor, CONTENTS_MASK_SOLID);
    if (trace.fraction == 1.0)
        return true;

    destination = target->GetOrigin();
    destination[0] += 15.0;
    destination[1] += 15.0;
    trace = SVG_Trace(inflictor->GetOrigin(), vec3_zero(), vec3_zero(), destination, inflictor, CONTENTS_MASK_SOLID);
    if (trace.fraction == 1.0)
        return true;

    destination = target->GetOrigin();
    destination[0] += 15.0;
    destination[1] -= 15.0;
    trace = SVG_Trace(inflictor->GetOrigin(), vec3_zero(), vec3_zero(), destination, inflictor, CONTENTS_MASK_SOLID);
    if (trace.fraction == 1.0)
        return true;

    destination = target->GetOrigin();
    destination[0] -= 15.0;
    destination[1] += 15.0;
    trace = SVG_Trace(inflictor->GetOrigin(), vec3_zero(), vec3_zero(), destination, inflictor, CONTENTS_MASK_SOLID);
    if (trace.fraction == 1.0)
        return true;

    destination = target->GetOrigin();
    destination[0] -= 15.0;
    destination[1] -= 15.0;
    trace = SVG_Trace(inflictor->GetOrigin(), vec3_zero(), vec3_zero(), destination, inflictor, CONTENTS_MASK_SOLID);
    if (trace.fraction == 1.0)
        return true;

    // If we reached this point... Well, it is false :)
    return false;
}

//===============
// DefaultGameMode::FindWithinRadius
//
// Returns a BaseEntityVector list containing the results of the found
// given entities that reside within the origin to radius. 
// 
// Flags can be set to determine which "solids" to exclude.
//===============
BaseEntityVector DefaultGameMode::FindBaseEnitiesWithinRadius(const vec3_t& origin, float radius, uint32_t excludeSolidFlags) {
    // List of base entities to return.
    std::vector<SVGBaseEntity*> baseEntityList;

    // Iterate over all entities, see who is nearby, and who is not.
    for (auto* radiusBaseEntity : GetBaseEntityRange<0, MAX_EDICTS>()
         | bef::IsValidPointer
         | bef::HasServerEntity
         | bef::InUse
         | bef::WithinRadius(origin, radius, excludeSolidFlags)) {

        // Push radiusEntity result item to the list.
        baseEntityList.push_back(radiusBaseEntity);
    }

    // The list might be empty, ensure to check for that ;-)
    return baseEntityList;
}

//===============
// DefaultGameMode::EntityKilled
//
// Called when an entity is killed, or at least, about to be.
// Determine how to deal with it, usually resides in a callback to Die.
//===============
void DefaultGameMode::EntityKilled(SVGBaseEntity* target, SVGBaseEntity* inflictor, SVGBaseEntity* attacker, int32_t damage, vec3_t point) {
    // Ensure health isn't exceeding limits.
    if (target->GetHealth() < -999)
        target->SetHealth(-999);

    // Set the enemy pointer to the current attacker.
    target->SetEnemy(attacker);

    // Determine whether it is a monster, and if it IS set to being dead....
    if ((target->GetServerFlags() & EntityServerFlags::Monster) && (target->GetDeadFlag() != DEAD_DEAD)) {
        target->SetServerFlags(target->GetServerFlags() | EntityServerFlags::DeadMonster);   // Now treat as a different content type

                                                                                             //        if (!(targ->monsterInfo.aiflags & AI_GOOD_GUY)) {
                                                                                             //            level.killedMonsters++;
                                                                                             //            if (coop->value && attacker->client)
                                                                                             //                attacker->client->respawn.score++;
                                                                                             //            // medics won't heal monsters that they kill themselves
                                                                                             //            if (strcmp(attacker->className, "monster_medic") == 0)
                                                                                             //                targ->owner = attacker;
                                                                                             //        }
    }

    if (target->GetMoveType() == MoveType::Push || target->GetMoveType() == MoveType::Stop || target->GetMoveType() == MoveType::None) {
        // Doors, triggers, etc
        if (target) {
            target->Die(inflictor, attacker, damage, point);
        }

        return;
    }

    //if ((targ->serverFlags & EntityServerFlags::Monster) && (targ->deadFlag != DEAD_DEAD)) {
    //    targ->Touch = NULL;
    //    monster_death_use(targ);
    //}
    if (target) {
        target->Die(inflictor, attacker, damage, point);
    }
    //targ->Die(targ, inflictor, attacker, damage, point);
}

//===============
// DefaultGameMode::InflictDamage
// 
//===============
void DefaultGameMode::InflictDamage(SVGBaseEntity* target, SVGBaseEntity* inflictor, SVGBaseEntity* attacker, const vec3_t& dmgDir, const vec3_t& point, const vec3_t& normal, int32_t damage, int32_t knockBack, int32_t damageFlags, int32_t mod) {
    int32_t damageTaken = 0;   // Damage taken.
    int32_t damageSaved = 0;   // Damaged saved, from being taken.

                               // Best be save than sorry.
    if (!target || !inflictor || !attacker) {
        return;
    }

    // In case this entity is not taking any damage, all bets are off, don't bother moving on.
    if (!target->GetTakeDamage())
        return;

    // WID: This sticks around, cuz of reference, but truly will be all but this itself was.
    // friendly fire avoidance
    // if enabled you can't hurt teammates (but you can hurt yourself)
    // knockBack still occurs
    //if ((targ != attacker) && ((deathmatch->value && ((int)(gamemodeflags->value) & (GameModeFlags::ModelTeams | GameModeFlags::SkinTeams))) || coop->value)) {
    //    if (game.gameMode->OnSameTeam(targ, attacker)) {
    //        if ((int)(gamemodeflags->value) & GameModeFlags::NoFriendlyFire)
    //            damage = 0;
    //        else
    //            mod |= MeansOfDeath::FriendlyFire;
    //    }
    //}
    // We resort to defaults, but keep the above as mentioned.
    SetCurrentMeansOfDeath(mod);

    // Fetch client.
    GameClient* client = target->GetClient();

    // Lame thing, regarding the sparks to use. Ancient code, keeping it for now.
    int32_t te_sparks = TempEntityEvent::Sparks;
    if (damageFlags & DamageFlags::Bullet)
        te_sparks = TempEntityEvent::BulletSparks;

    // Retrieve normalized direction.
    vec3_t dir = vec3_normalize(dmgDir);

    // Ensure there is no odd knockback issues.
    if (target->GetFlags() & EntityFlags::NoKnockBack)
        knockBack = 0;

    // Figure out the momentum to add in case KnockBacks are off. 
    if (!(damageFlags & DamageFlags::NoKnockBack)) {
        if ((knockBack) && (target->GetMoveType() != MoveType::None) && (target->GetMoveType() != MoveType::Bounce) && (target->GetMoveType() != MoveType::Push) && (target->GetMoveType() != MoveType::Stop)) {
            vec3_t  kvel = { 0.f, 0.f, 0.f };
            float   mass = 50; // Defaults to 50, otherwise... issues, this is the OG code style btw.

                               // Based on mass, if it is below 50, we wanna hook it to being 50. Otherwise...
            if (target->GetMass() > 50)
                mass = target->GetMass();

            // Determine whether attacker == target, and the client itself, that means we gotta jump back hard.
            if (target->GetClient() && attacker == target)
                kvel = vec3_scale(dir, 1600.0 * (float)knockBack / mass); // ROCKET JUMP HACK IS HERE BRUH <--
            else
                kvel = vec3_scale(dir, 500 * (float)knockBack / mass);

            // Assign the new velocity, since yeah, it's bound to knock the fuck out of us.
            target->SetVelocity(target->GetVelocity() + kvel);
        }
    }

    // Setup damages, so we can maths with them, yay. Misses code cuz we got no armors no more :P
    damageTaken = damage;       // Damage taken.
    damageSaved = 0;            // Damaged saved, from being taken.

                                // check for godmode
    if ((target->GetFlags() & EntityFlags::GodMode) && !(damageFlags & DamageFlags::IgnoreProtection)) {
        damageTaken = 0;
        damageSaved = damage;

        // Leave it for the game mode to move on and spawn this temp entity (if allowed.)
        SpawnTempDamageEntity(te_sparks, point, normal, damageSaved);
    }

    // Team damage avoidance
    if (!(damageFlags & DamageFlags::IgnoreProtection) && game.gameMode->OnSameTeam(target, attacker))
        return;

    // Inflict the actual damage, in case we got to deciding to do so based on the above.
    if (damageTaken) {
        // Check if monster, or client, in which case, we spawn blood.
        // If not... :)... Do not.
        if ((target->GetServerFlags() & EntityServerFlags::Monster) || (client)) {
            // SpawnTempDamageEntity(TempEntityEvent::Blood, point, normal, take);
            // Leave it for the game mode to move on and spawn this temp entity (if allowed.)
            SpawnTempDamageEntity(TempEntityEvent::Blood, point, dir, damageTaken);
        } else {
            // Leave it for the game mode to move on and spawn this temp entity (if allowed.)
            SpawnTempDamageEntity(te_sparks, point, normal, damageTaken);
        }

        // Adjust health based on calculated damage to take.
        target->SetHealth(target->GetHealth() - damageTaken);

        // In case health is/was below 0.
        if (target->GetHealth() <= 0) {
            // Check if monster, or client, in which case, we execute no knockbacks :)
            if ((target->GetServerFlags() & EntityServerFlags::Monster) || (client))
                target->SetFlags(target->GetFlags() | EntityFlags::NoKnockBack);

            // It's dead though, or at least we assume so... Call on to: EntityKilled.
            EntityKilled(target, inflictor, attacker, damageTaken, point);
            return;
        }
    }

    // Special damage handling for monsters.
    if (target->GetServerFlags() & EntityServerFlags::Monster) {
        // WID: Maybe do some check for monster entities here sooner or later? Who knows...
        // Gotta have them cunts react to it. But we'll see, might as well be on TakeDamage :)
        //M_ReactToDamage(targ, attacker);

        //if (!(targ->monsterInfo.aiflags & AI_DUCKED) && (take)) {
        target->TakeDamage(attacker, knockBack, damageTaken);
        //// nightmare mode monsters don't go into pain frames often
        //if (skill->value == 3)
        //    targ->debouncePainTime = level.time + 5;
        //}
    } else {
        if (client) {
            //if (!(targ->flags & EntityFlags::GodMode) && (take))
            //    targ->Pain(targ, attacker, knockBack, take);
            if (!(target->GetFlags() & EntityFlags::GodMode) && (damageTaken)) {
                target->TakeDamage(attacker, knockBack, damageTaken);
            }
        } else if (damageTaken) {
            target->TakeDamage(attacker, knockBack, damageTaken);
        }
    }

    // add to the damage inflicted on a player this frame
    // the total will be turned into screen blends and view angle kicks
    // at the end of the frame
    if (client) {
        client->damages.blood += damageTaken;
        client->damages.knockBack += knockBack;
        client->damages.from = point;
    }
}

//===============
// DefaultGameMode::InflictDamage
// 
//===============
void DefaultGameMode::InflictRadiusDamage(SVGBaseEntity* inflictor, SVGBaseEntity* attacker, float damage, SVGBaseEntity* ignore, float radius, int32_t mod) {
    // Damage point counter for radius sum ups.
    float   points = 0.f;

    // Actual entity loop pointer.
    SVGBaseEntity* ent = nullptr;

    // N&C: From Yamagi Q2, to prevent issues.
    if (!inflictor || !attacker) {
        return;
    }

    // Find entities within radius.
    BaseEntityVector radiusEntities = FindBaseEnitiesWithinRadius(inflictor->GetOrigin(), radius, Solid::Not);

    //while ((ent = SVG_FindEntitiesWithinRadius(ent, inflictor->GetOrigin(), radius)) != NULL) {
    for (auto& baseEntity : radiusEntities) {
        //// Continue in case this entity has to be ignored from applying damage.
        if (baseEntity == ignore)
            continue;

        // Continue in case this entity CAN'T take any damage.
        if (!baseEntity->GetTakeDamage())
            continue;

        // Calculate damage points.
        vec3_t velocity = baseEntity->GetMins() + baseEntity->GetMaxs();
        velocity = vec3_fmaf(baseEntity->GetOrigin(), 0.5f, velocity);
        velocity -= inflictor->GetOrigin();
        points = damage - 0.5f * vec3_length(velocity);

        // In case the attacker is the own entity, half damage.
        if (ent == attacker)
            points = points * 0.5f;

        // Apply damage points.
        if (points > 0) {
            // Ensure whether we CAN actually apply damage.
            if (CanDamage(baseEntity, inflictor)) {
                // Calculate direcion.
                vec3_t dir = baseEntity->GetOrigin() - inflictor->GetOrigin();

                // Apply damages.
                InflictDamage(baseEntity, inflictor, attacker, dir, inflictor->GetOrigin(), vec3_zero(), (int)points, (int)points, DamageFlags::IndirectFromRadius, mod);
            }
        }
    }
}

//===============
// DefaultGameMode::SetCurrentMeansOfDeath
// 
//===============
void DefaultGameMode::SetCurrentMeansOfDeath(int32_t meansOfDeath) {
    this->meansOfDeath = meansOfDeath;
}

//===============
// DefaultGameMode::GetCurrentMeansOfDeath
// 
//===============
const int32_t& DefaultGameMode::GetCurrentMeansOfDeath() {
    return this->meansOfDeath;
}

//===============
// DefaultGameMode::SpawnClientCorpse
// 
// Spawns a dead body entity for the given client.
//===============
void DefaultGameMode::SpawnClientCorpse(SVGBaseEntity* ent) {
    // Ensure it is an entity.
    if (!ent)
        return;

    // Ensure it is a client.
    if (!ent->GetClient())
        return;

    // Unlink the player client entity.
    ent->UnlinkEntity();

    // Grab a body from the queue, and cycle to the next one.
    Entity *bodyEntity = &g_entities[game.maximumClients + level.bodyQue + 1];
    level.bodyQue = (level.bodyQue + 1) % BODY_QUEUE_SIZE;

    // Send an effect on this body, in case it already has a model index.
    // This'll cause a body not to just "disappear", but actually play some
    // bloody particles over there.
    if (bodyEntity->state.modelIndex) {
        gi.WriteByte(SVG_CMD_TEMP_ENTITY);
        gi.WriteByte(TempEntityEvent::Blood);
        gi.WriteVector3(bodyEntity->state.origin);
        gi.WriteVector3(vec3_zero());
        gi.Multicast(bodyEntity->state.origin, MultiCast::PVS);
    }

    // Create the class entity for this queued bodyEntity.
    SVGBaseEntity *bodyClassEntity = SVG_CreateClassEntity<BodyCorpse>(bodyEntity, false);

    // Unlink the body entity, in case it was linked before.
    bodyClassEntity->UnlinkEntity();

    // Copy over the bodies state of the current entity into the body entity.
    bodyClassEntity->SetState(ent->GetState());
    // Change its number so it is accurately set to the one belonging to bodyEntity.
    // (Has to happen since we first copied over an entire entity state.)
    bodyClassEntity->SetNumber(bodyEntity - g_entities);
    // Set the event ID for this frame to OtherTeleport.
    bodyClassEntity->SetEventID(EntityEvent::OtherTeleport);

    // Copy over the serverflags from ent.
    bodyClassEntity->SetServerFlags(ent->GetServerFlags());
    bodyClassEntity->SetMins(ent->GetMins());
    bodyClassEntity->SetMaxs(ent->GetMaxs());
    bodyClassEntity->SetAbsoluteMin(ent->GetAbsoluteMin());
    bodyClassEntity->SetAbsoluteMax(ent->GetAbsoluteMax());
    bodyClassEntity->SetSize(ent->GetSize());
    bodyClassEntity->SetVelocity(ent->GetVelocity());
    bodyClassEntity->SetAngularVelocity(ent->GetAngularVelocity());
    bodyClassEntity->SetSolid(ent->GetSolid());
    bodyClassEntity->SetClipMask(ent->GetClipMask());
    bodyClassEntity->SetOwner(ent->GetOwner());
    bodyClassEntity->SetMoveType(ent->GetMoveType());
    bodyClassEntity->SetGroundEntity(ent->GetGroundEntity());

    // Set the die callback, and set its take damage.
    bodyClassEntity->SetDieCallback(&BodyCorpse::BodyCorpseDie);
    bodyClassEntity->SetTakeDamage(TakeDamage::Yes);

    // Link it in for collision etc.
    bodyClassEntity->LinkEntity();
}

//===============
// DefaultGameMode::SpawnTempDamageEntity
// 
// Sends a message to all clients in the current PVS, spawning a temp entity for
// displaying damage entities client side. (Sparks, what have ya.)
//===============
void DefaultGameMode::SpawnTempDamageEntity(int32_t type, const vec3_t& origin, const vec3_t& normal, int32_t damage) {
    // WID: Ensure the effect can't send more damage. But that is unimplemented for the clients atm to even detect...
    if (damage > 255)
        damage = 255;

    // Write away.
    gi.WriteByte(SVG_CMD_TEMP_ENTITY);
    gi.WriteByte(type);
    //  gi.WriteByte (damage); // <-- This was legacy crap, might wanna implement it ourselves eventually.
    gi.WriteVector3(origin);
    gi.WriteVector3(normal);
    gi.Multicast(origin, MultiCast::PVS);
}

//===============
// DefaultGameMode::CalculateDamageVelocity
// 
// Default implementation for calculating velocity damage.
//===============
vec3_t DefaultGameMode::CalculateDamageVelocity(int32_t damage) {
    // Pick random velocities.
    vec3_t velocity = {
        100.0f * crandom(),
        100.0f * crandom(),
        200.0f + 100.0f * random()
    };

    // Scale velocities.
    if (damage < 50)
        velocity = vec3_scale(velocity, 0.7f);
    else
        velocity = vec3_scale(velocity, 1.2f);

    // Return.
    return velocity;
}

//===============
// DefaultGameMode::OnLevelExit
// 
// Default implementation for exiting levels.
//===============
void DefaultGameMode::OnLevelExit() {
    // Create the command to use for switching to the next game map.
    std::string command = "gamemap \"";
    command += level.intermission.changeMap;
    command += +"\"";

    // Add the gamemap command to the 
    gi.AddCommandString(command.c_str());
    // Reset the changeMap string, intermission time, and regular level time.
    level.intermission.changeMap = NULL;
    level.intermission.exitIntermission = 0;
    level.intermission.time = 0;

    // End the server frames for all clients.
    SVG_ClientEndServerFrames();

    // Fetch the WorldSpawn entity number.
    int32_t stateNumber = g_entities[0].state.number;

    // Fetch the corresponding base entity.
    SVGBaseEntity* entity = g_baseEntities[stateNumber];

    // Loop through the server entities, and run the base entity frame if any exists.
    for (int32_t i = 0; i < globals.numberOfEntities; i++) {
        // Acquire state number.
        stateNumber = g_entities[i].state.number;

        // Fetch the corresponding base entity.
        SVGBaseEntity* entity = g_baseEntities[stateNumber];

        // Is it even valid?
        if (entity == nullptr)
            continue;

        // Don't go on if it isn't in use.
        if (!entity->IsInUse())
            continue;

        // Continue in case... cuz we know...
        if (!entity->GetClient())
            continue;

        // Ensure an entity its health is reset to default.
        if (entity->GetHealth() > entity->GetClient()->persistent.maxHealth)
            entity->SetHealth(entity->GetClient()->persistent.maxHealth);
    }
}

//===============
// DefaultGameMode::ClientBeginServerFrame
// 
// Does logic checking for a client's start of a server frame. In case there
// is a "level.intermission.time" set, it'll flat out return.
// 
// This basically allows for the game to disable fetching user input that makes
// our movement tick. And/or shoot weaponry while in intermission time.
//===============
void DefaultGameMode::ClientBeginServerFrame(PlayerClient* player) {
    // Ensure we aren't in an intermission time.
    if (level.intermission.time)
        return;

    // Fetch the client.
    GameClient* client = player->GetClient();

    // This has to go ofc.... lol. What it simply does though, is determine whether there is 
    // a need to respawn as spectator.
    //if (deathmatch->value &&
    //    client->persistent.isSpectator != client->respawn.isSpectator &&
    //    (level.time - client->respawnTime) >= 5) {
    //    spectator_respawn(ent->GetServerEntity());
    //    return;
    //}

    // Run weapon animations in case this has not been done by user input itself.
    // (Idle animations, and general weapon thinking when a weapon is not in action.)
    if (!client->weaponThunk && !client->respawn.isSpectator)
        SVG_ThinkWeapon(player);
    else
        client->weaponThunk = false;

    // Check if the player is actually dead or not. If he is, we're going to enact on
    // the user input that's been given to us. When fired, we'll respawn.
    int32_t buttonMask = 0;
    if (player->GetDeadFlag()) {
        // Wait for any button just going down
        if (level.time > client->respawnTime) {
            // In old code, the need to hit a key was only set in DM mode.
            // I figured, let's keep it like this instead.
            //if (deathmatch->value)
            buttonMask = BUTTON_ATTACK;
            //else
            //buttonMask = -1;

            if ((client->latchedButtons & buttonMask) ||
                (deathmatch->value && ((int)gamemodeflags->value & GameModeFlags::ForceRespawn))) {
                SVG_RespawnClient(player->GetServerEntity());
                client->latchedButtons = 0;
            }
        }
        return;
    }

    //// add player trail so monsters can follow
    //if (!deathmatch->value)
    //    if (!visible(ent, SVG_PlayerTrail_LastSpot()))
    //        SVG_PlayerTrail_Add(ent->state.oldOrigin);

    // Reset the latched buttons.
    client->latchedButtons = 0;
}

//===============
// DefaultGameMode::ClientCanConnect
// 
// Checks for whether it is OK for a client to connect go here.
//===============
qboolean DefaultGameMode::ClientCanConnect(Entity* serverEntity, char* userInfo) {
    // Check to see if they are on the banned IP list
    char *value = Info_ValueForKey(userInfo, "ip");
    if (SVG_FilterPacket(value)) {
        Info_SetValueForKey(userInfo, "rejmsg", "Banned.");
        return false;
    }

    // Check for a password, and if there is one, does it match?
    value = Info_ValueForKey(userInfo, "password");
    if (*password->string && strcmp(password->string, "none") &&
        strcmp(password->string, value)) {
        Info_SetValueForKey(userInfo, "rejmsg", "Password required or incorrect.");
        return false;
    }

    // Return true, client is allowed to connect.
    return true;
}

//===============
// DefaultGameMode::ClientConnect
// 
// Client is connecting, what do? :)
//===============
void DefaultGameMode::ClientConnect(Entity* serverEntity) {
    if (!serverEntity) {
        gi.DPrintf("ClientConnect executed with invalid (nullptr) serverEntity");
        return;
    }

    // This is default behaviour for this function.
    if (game.maximumClients > 1)
        gi.DPrintf("%s connected\n", serverEntity->client->persistent.netname);
}

//===============
// DefaultGameMode::ClientBegin
// 
// Called when a client is ready to be placed in the game after connecting.
//===============
void DefaultGameMode::ClientBegin(Entity* serverEntity) {
    if (!serverEntity) {
        gi.DPrintf("ClientBegin executed with invalid (nullptr) serverEntity");
        return;
    }
    // If there is already a body waiting for us (a loadgame), just
    // take it, otherwise spawn one from scratch
    if (serverEntity->inUse == true) {
        // The client has cleared the client side viewAngles upon
        // connecting to the server, which is different than the
        // state when the game is saved, so we need to compensate
        // with deltaangles
        for (int32_t i = 0; i < 3; i++)
            serverEntity->client->playerState.pmove.deltaAngles[i] = serverEntity->client->playerState.pmove.viewAngles[i];
    } else {
        SVG_InitClientRespawn(serverEntity->client);
        SVG_PutClientInServer(serverEntity);
        //serverEntity->classEntity->Respawn();
    }

    if (level.intermission.time) {
        HUD_MoveClientToIntermission(serverEntity);
    } else {
        // send effect if in a multiplayer game
        if (game.maximumClients > 1) {
            gi.WriteByte(SVG_CMD_MUZZLEFLASH);
            //gi.WriteShort(ent - g_entities);
            gi.WriteShort(serverEntity->state.number);
            gi.WriteByte(MuzzleFlashType::Login);
            gi.Multicast(serverEntity->state.origin, MultiCast::PVS);

            gi.BPrintf(PRINT_HIGH, "%s entered the game\n", serverEntity->client->persistent.netname);
        }
    }
}

//===============
// DefaultGameMode::ClientDisconnect.
// 
// Does logic checking for a client's start of a server frame. In case there
// is a "level.intermission.time" set, it'll flat out return.
// 
// This basically allows for the game to disable fetching user input that makes
// our movement tick. And/or shoot weaponry while in intermission time.
//===============
void DefaultGameMode::ClientDisconnect(PlayerClient* player) {
    // Fetch the client.
    GameClient* client = player->GetClient();

    // Print who disconnected.
    gi.BPrintf(PRINT_HIGH, "%s disconnected\n", client->persistent.netname);

    // Send effect
    if (player->IsInUse()) {
        gi.WriteByte(SVG_CMD_MUZZLEFLASH);
        //gi.WriteShort(ent - g_entities);
        gi.WriteShort(player->GetNumber());
        gi.WriteByte(MuzzleFlashType::Logout);
        gi.Multicast(player->GetOrigin(), MultiCast::PVS);
    }

    // Unset this entity, after all, it's about to disconnect so.
    // We don't want it having any model, collision, sound, event, effects...
    // and ensure it is not in use anymore, also change its classname.
    player->UnlinkEntity();
    player->SetModelIndex(0);
    player->SetSound(0);
    player->SetEventID(0);
    player->SetEffects(0);
    player->SetSolid(Solid::Not);
    player->SetInUse(false);
    player->SetClassName("disconnected");

    // Ensure a state is stored for that this client is not connected anymore.
    client->persistent.isConnected = false;

    // FIXME: don't break skins on corpses, etc
    //playernum = ent-g_entities-1;
    //gi.configstring (ConfigStrings::PlayerSkins+playernum, "");
}

//===============
// DefaultGameMode::ClientUpdateObituary
// 
//===============
void DefaultGameMode::ClientUpdateObituary(SVGBaseEntity* self, SVGBaseEntity* inflictor, SVGBaseEntity* attacker) {
    std::string message = ""; // String stating what happened to whichever entity. "suicides", "was squished" etc.
    std::string messageAddition = ""; // String stating what is additioned to it, "'s shrapnell" etc. Funny stuff.

                                      // Goes to COOP GAME MODE.
                                      //if (coop->value && attacker->GetClient())
                                      //    meansOfDeath |= MeansOfDeath::FriendlyFire;

    qboolean friendlyFire = meansOfDeath & MeansOfDeath::FriendlyFire;
    int32_t finalMeansOfDeath = meansOfDeath & ~MeansOfDeath::FriendlyFire;

    switch (finalMeansOfDeath) {
    case MeansOfDeath::Suicide:
        message = "suicides";
        break;
    case MeansOfDeath::Falling:
        message = "cratered";
        break;
    case MeansOfDeath::Crush:
        message = "was squished";
        break;
    case MeansOfDeath::Water:
        message = "sank like a rock";
        break;
    case MeansOfDeath::Slime:
        message = "melted";
        break;
    case MeansOfDeath::Lava:
        message = "does a back flip into the lava";
        break;
    case MeansOfDeath::Explosive:
    case MeansOfDeath::Barrel:
        message = "blew up";
        break;
    case MeansOfDeath::Exit:
        message = "found a way out";
        break;
    case MeansOfDeath::Splash:
    case MeansOfDeath::TriggerHurt:
        message = "was in the wrong place";
        break;
    }
    if (attacker == self) {
        switch (finalMeansOfDeath) {
        case MeansOfDeath::GrenadeSplash:
            message = "tripped on his own grenade";
            break;
        case MeansOfDeath::RocketSplash:
            message = "blew himself up";
            break;
        default:
            message = "killed himself";
            break;
        }
    }
    if (message != "") {
        gi.BPrintf(PRINT_MEDIUM, "%s %s.\n", self->GetClient()->persistent.netname, message.c_str());
        // WID: We can uncomment these in case we end up making a SinglePlayerMode after all.
        //if (deathmatch->value)
        //    self->GetClient()->respawn.score--;
        self->SetEnemy(NULL);
        return;
    }

    // Set the attacker to self.
    self->SetEnemy(attacker);

    // If we have an attacker, and it IS a client...
    if (attacker && attacker->GetClient()) {
        switch (finalMeansOfDeath) {
        case MeansOfDeath::Blaster:
            message = "was blasted by";
            break;
        case MeansOfDeath::Shotgun:
            message = "was gunned down by";
            break;
        case MeansOfDeath::SuperShotgun:
            message = "was blown away by";
            messageAddition = "'s super shotgun";
            break;
        case MeansOfDeath::Machinegun:
            message = "was machinegunned by";
            break;
        case MeansOfDeath::Grenade:
            message = "was popped by";
            messageAddition = "'s grenade";
            break;
        case MeansOfDeath::GrenadeSplash:
            message = "was shredded by";
            messageAddition = "'s shrapnel";
            break;
        case MeansOfDeath::Rocket:
            message = "ate";
            messageAddition = "'s rocket";
            break;
        case MeansOfDeath::RocketSplash:
            message = "almost dodged";
            messageAddition = "'s rocket";
            break;
        case MeansOfDeath::TeleFrag:
            message = "tried to invade";
            messageAddition = "'s personal space";
            break;
        }
        if (message != "") {
            gi.BPrintf(PRINT_MEDIUM, "%s %s %s%s\n", self->GetClient()->persistent.netname, message.c_str(), attacker->GetClient()->persistent.netname, messageAddition.c_str());
            // WID: We can uncomment these in case we end up making a SinglePlayerMode after all.
            //if (deathmatch->value) {
            //    if (friendlyFire)
            //        attacker->GetClient()->respawn.score--;
            //    else
            //        attacker->GetClient()->respawn.score++;
            //}
            return;
        }
    }

    // Check for monster deaths here.
    if (attacker->GetServerFlags() & EntityServerFlags::Monster) {
        // Fill in message here
        // aka if (attacker->classname == "monster_1337h4x0r")
        // Then we do...
        // Also we gotta adjust that ->classname thing, but this is a template, cheers :)
        //if (!message.empty()) {
        //    gi.BPrintf(PRINT_MEDIUM, "%s %s %s%s\n", self->GetClient()->persistent.netname, message.c_str(), attacker->GetClassName(), messageAddition.c_str());
        //    if (deathmatch->value) {
        //        if (friendlyFire)
        //            attacker->GetClient()->respawn.score--;
        //        else
        //            attacker->GetClient()->respawn.score++;
        //    }
        //    return;
        //}
    }

    // 
    gi.BPrintf(PRINT_MEDIUM, "%s died.\n", self->GetClient()->persistent.netname);

    // WID: We can uncomment these in case we end up making a SinglePlayerMode after all.
    //if (deathmatch->value)
    //    self->GetClient()->respawn.score--;
}

//===============
// DefaultGameMode::PutClientInServer
// 
// Can be used to legit respawn a client at a spawn point.
// For SinglePlayer you want to take it a bit easy with this function.
// For Multiplayer games however, you definitely want to use this function.
//
// SP games: Use it once... (or at load time)
// MP games: Use it every respawn.
//===============
void DefaultGameMode::PutClientInServer(PlayerClient *ent) {
    // Find a spawn point for this client to be "placed"/"put" at.

}