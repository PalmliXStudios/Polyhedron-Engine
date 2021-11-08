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

#include "../g_local.h"
#include "../entities.h"
#include "../entities/base/SVGBaseEntity.h"
#include "../entities/base/PlayerClient.h"
#include "hud.h"
#include "animations.h"

// The actual current player entity that this C++ unit is acting on.
static PlayerClient *currentProcessingPlayer;

// The current client belonging to the player.
static GameClient   *currentProcessingClient;

static vec3_t  forward, right, up;
static float   XYSpeed;

static float   bobMove;
static int     bobCycle;       // odd cycles are right foot going forward
static float   bobFracsin;     // sin(bobfrac*M_PI)

//
//===============
// SVG_CalcRoll
// 
//
//===============
//
static float SVG_CalcRoll(const vec3_t &angles, const vec3_t &velocity)
{
    float   sign;
    float   side;
    float   value;

    side = vec3_dot(velocity, right);
    sign = side < 0 ? -1 : 1;
    side = fabs(side);

    value = sv_rollangle->value;

    if (side < sv_rollspeed->value)
        side = side * value / sv_rollspeed->value;
    else
        side = value;

    return side * sign;

}

//
//===============
// SVG_Player_ApplyDamageFeedback
// 
// Handles color blends and view kicks
//===============
//
static void SVG_Player_ApplyDamageFeedback(PlayerClient *ent)
{
    float   side;
    float   realcount, count, kick;
    vec3_t  v;
    int     r, l;
    static  vec3_t  power_color = {0.0f, 1.0f, 0.0f};
    static  vec3_t  acolor = {1.0f, 1.0f, 1.0f};
    static  vec3_t  bcolor = {1.0f, 0.0f, 0.0f};

    //client = player->client;

    // flash the backgrounds behind the status numbers
    currentProcessingClient->playerState.stats[STAT_FLASHES] = 0;
    if (currentProcessingClient->damages.blood)
        currentProcessingClient->playerState.stats[STAT_FLASHES] |= 1;
    if (currentProcessingClient->damages.armor && !(ent->GetFlags() & EntityFlags::GodMode))
        currentProcessingClient->playerState.stats[STAT_FLASHES] |= 2;

    // total points of damage shot at the player this frame
    count = (currentProcessingClient->damages.blood + currentProcessingClient->damages.armor + currentProcessingClient->damages.powerArmor);
    if (count == 0)
        return;     // didn't take any damage

    // start a pain animation if still in the player model
    if (currentProcessingClient->animation.priorityAnimation < PlayerAnimation::Pain && ent->GetModelIndex() == 255) {
        static int      i;

        currentProcessingClient->animation.priorityAnimation = PlayerAnimation::Pain;
        if (currentProcessingClient->playerState.pmove.flags & PMF_DUCKED) {
            ent->SetFrame(FRAME_crpain1 - 1);
            currentProcessingClient->animation.endFrame = FRAME_crpain4;
        } else {
            i = (i + 1) % 3;
            switch (i) {
            case 0:
                ent->SetFrame(FRAME_pain101 - 1);
                currentProcessingClient->animation.endFrame = FRAME_pain104;
                break;
            case 1:
                ent->SetFrame(FRAME_pain201 - 1);
                currentProcessingClient->animation.endFrame = FRAME_pain204;
                break;
            case 2:
                ent->SetFrame(FRAME_pain301 - 1);
                currentProcessingClient->animation.endFrame = FRAME_pain304;
                break;
            }
        }
    }

    realcount = count;
    if (count < 10)
        count = 10; // always make a visible effect

    // Play an apropriate pain sound
    if ((level.time > ent->GetDebouncePainTime()) && !(ent->GetFlags() & EntityFlags::GodMode)) {
        r = 1 + (rand() & 1);
        ent->SetDebouncePainTime(level.time + 0.7f);
        if (ent->GetHealth() < 25)
            l = 25;
        else if (ent->GetHealth() < 50)
            l = 50;
        else if (ent->GetHealth() < 75)
            l = 75;
        else
            l = 100;
        SVG_Sound(ent, CHAN_VOICE, gi.SoundIndex(va("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
    }

    // The total alpha of the blend is always proportional to count.
    if (currentProcessingClient->damageAlpha < 0.f)
        currentProcessingClient->damageAlpha = 0.f;
    currentProcessingClient->damageAlpha += count * 0.01f;
    if (currentProcessingClient->damageAlpha < 0.2f)
        currentProcessingClient->damageAlpha = 0.2f;
    if (currentProcessingClient->damageAlpha > 0.6f)
        currentProcessingClient->damageAlpha = 0.6f;     // don't go too saturated

    // The color of the blend will vary based on how much was absorbed
    // by different armors.
    vec3_t blendColor = vec3_zero();
    if (currentProcessingClient->damages.powerArmor)
        blendColor = vec3_fmaf(blendColor, (float)currentProcessingClient->damages.powerArmor / realcount, power_color);
    if (currentProcessingClient->damages.armor)
        blendColor = vec3_fmaf(blendColor, (float)currentProcessingClient->damages.armor / realcount, acolor);
    if (currentProcessingClient->damages.blood)
        blendColor = vec3_fmaf(blendColor, (float)currentProcessingClient->damages.blood / realcount, bcolor);
    currentProcessingClient->damageBlend = blendColor;


    //
    // Calculate view angle kicks
    //
    kick = abs(currentProcessingClient->damages.knockBack);
    if (kick && ent->GetHealth() > 0) { // kick of 0 means no view adjust at all
        kick = kick * 100 / ent->GetHealth();

        if (kick < count * 0.5f)
            kick = count * 0.5f;
        if (kick > 50)
            kick = 50;

        vec3_t kickVec = currentProcessingClient->damages.from - ent->GetOrigin();
        kickVec = vec3_normalize(kickVec);

        side = vec3_dot(kickVec, right);
        currentProcessingClient->viewDamage.roll = kick * side * 0.3f;

        side = -vec3_dot(kickVec, forward);
        currentProcessingClient->viewDamage.pitch = kick * side * 0.3f;

        currentProcessingClient->viewDamage.time = level.time + DAMAGE_TIME;
    }

    //
    // clear totals
    //
    currentProcessingClient->damages.blood = 0;
    currentProcessingClient->damages.armor = 0;
    currentProcessingClient->damages.powerArmor = 0;
    currentProcessingClient->damages.knockBack = 0;
}

//
//===============
// SVG_CalculateViewOffset
// 
// Calculates t
//
// fall from 128 : 400 = 160000
// fall from 256 : 580 = 336400
// fall from 384 : 720 = 518400
// fall from 512 : 800 = 640000
// fall from 640 : 960 =
//
// damage = deltavelocity * deltavelocity * 0.0001
// 
//===============
//
static void SVG_CalculateViewOffset(PlayerClient *ent)
{
    float       bob;
    float       ratio;
    float       delta;

    //
    // Calculate new kick angle vales. (
    // 
    // If dead, set a fixed angle and don't add any kick
    if (ent->GetDeadFlag()) {
        currentProcessingClient->playerState.kickAngles = vec3_zero();

        currentProcessingClient->playerState.pmove.viewAngles[vec3_t::Roll] = 40;
        currentProcessingClient->playerState.pmove.viewAngles[vec3_t::Pitch] = -15;
        currentProcessingClient->playerState.pmove.viewAngles[vec3_t::Yaw] = currentProcessingClient->killerYaw;
    } else {
        // Fetch client kick angles.
        vec3_t newKickAngles = currentProcessingClient->playerState.kickAngles = currentProcessingClient->kickAngles; //ent->client->playerState.kickAngles;

        // Add pitch(X) and roll(Z) angles based on damage kick
        ratio = ((currentProcessingClient->viewDamage.time - level.time) / DAMAGE_TIME) * FRAMETIME;
        if (ratio < 0) {
            ratio = currentProcessingClient->viewDamage.pitch = currentProcessingClient->viewDamage.roll = 0;
        }
        newKickAngles[vec3_t::Pitch] += ratio * currentProcessingClient->viewDamage.pitch;
        newKickAngles[vec3_t::Roll] += ratio * currentProcessingClient->viewDamage.roll;

        // Add pitch based on fall kick
        ratio = ((currentProcessingClient->fallTime - level.time) / FALL_TIME) * FRAMETIME;;
        if (ratio < 0)
            ratio = 0;
        newKickAngles[vec3_t::Pitch] += ratio * currentProcessingClient->fallValue;

        // Add angles based on velocity
        delta = vec3_dot(ent->GetVelocity(), forward) * FRAMETIME;;
        newKickAngles[vec3_t::Pitch] += delta * run_pitch->value;

        delta = vec3_dot(ent->GetVelocity(), right) * FRAMETIME;;
        newKickAngles[vec3_t::Roll] += delta * run_roll->value;

        // Add angles based on bob
        delta = bobFracsin * bob_pitch->value * XYSpeed * FRAMETIME;;
        if (currentProcessingClient->playerState.pmove.flags & PMF_DUCKED)
            delta *= 6;     // crouching
        newKickAngles[vec3_t::Pitch] += delta;
        delta = bobFracsin * bob_roll->value * XYSpeed;
        if (currentProcessingClient->playerState.pmove.flags & PMF_DUCKED)
            delta *= 6;     // crouching
        if (bobCycle & 1)
            delta = -delta;
        newKickAngles[vec3_t::Roll] += delta;

        // Last but not least, assign new kickangles to player state.
        currentProcessingClient->playerState.kickAngles = newKickAngles;
    }

    //
    // Calculate new view offset.
    //
    // Start off with the base entity viewheight. (Set by Player Move code.)
    vec3_t newViewOffset = {
        0.f,
        0.f,
        (float)ent->GetViewHeight()
    };
        
    // Add fall impact view punch height.
    ratio = (currentProcessingClient->fallTime - level.time) / FALL_TIME;
    if (ratio < 0)
        ratio = 0;
    newViewOffset.z -= ratio * currentProcessingClient->fallValue * 0.4f;

    // Add bob height.
    bob = bobFracsin * XYSpeed * bob_up->value * FRAMETIME;;
    if (bob > 6)
        bob = 6;
    newViewOffset.z += bob;

    // Add kick offset
    newViewOffset += currentProcessingClient->kickOrigin;

    // Clamp the new view offsets, and finally assign them to the player state.
    // Clamping ensures that they never exceed the non visible, but physically 
    // there, player bounding box.
    currentProcessingClient->playerState.pmove.viewOffset = vec3_clamp(newViewOffset,
        //{ -14, -14, -22 },
        //{ 14,  14, 30 }
        ent->GetMins(),
        ent->GetMaxs()
    );
}

//
//===============
// SVG_CalculateGunOffset
// 
//===============
//
static void SVG_CalculateGunOffset(PlayerClient *ent)
{
    int     i;
    float   delta;

    // gun angles from bobbing
    currentProcessingClient->playerState.gunAngles[vec3_t::Roll] = XYSpeed * bobFracsin * 0.005;
    currentProcessingClient->playerState.gunAngles[vec3_t::Yaw]  = XYSpeed * bobFracsin * 0.01;
    if (bobCycle & 1) {
        currentProcessingClient->playerState.gunAngles[vec3_t::Roll] = -currentProcessingClient->playerState.gunAngles[vec3_t::Roll];
        currentProcessingClient->playerState.gunAngles[vec3_t::Yaw]  = -currentProcessingClient->playerState.gunAngles[vec3_t::Yaw];
    }

    currentProcessingClient->playerState.gunAngles[vec3_t::Pitch] = XYSpeed * bobFracsin * 0.005;

    // gun angles from delta movement
    for (i = 0 ; i < 3 ; i++) {
        delta = currentProcessingClient->oldViewAngles[i] - currentProcessingClient->playerState.pmove.viewAngles[i];
        if (delta > 180)
            delta -= 360;
        if (delta < -180)
            delta += 360;
        if (delta > 45)
            delta = 45;
        if (delta < -45)
            delta = -45;
        if (i == vec3_t::Yaw)
            currentProcessingClient->playerState.gunAngles[vec3_t::Roll] += 0.1 * delta;
        currentProcessingClient->playerState.gunAngles[i] += 0.2 * delta;
    }

    // gun height
    currentProcessingClient->playerState.gunOffset = vec3_zero();
//  ent->playerState->gunorigin[2] += bob;

    // gun_x / gun_y / gun_z are development tools
    for (i = 0 ; i < 3 ; i++) {
        currentProcessingClient->playerState.gunOffset[i] += forward[i] * (gun_y->value);
        currentProcessingClient->playerState.gunOffset[i] += right[i] * gun_x->value;
        currentProcessingClient->playerState.gunOffset[i] += up[i] * (-gun_z->value);
    }
}

//
//===============
// SV_AddBlend
// 
//===============
//
static void SV_AddBlend(float r, float g, float b, float a, float *v_blend)
{
    float   a2, a3;

    if (a <= 0)
        return;
    a2 = v_blend[3] + (1 - v_blend[3]) * a; // new total alpha
    a3 = v_blend[3] / a2;   // fraction of color from old

    v_blend[0] = v_blend[0] * a3 + r * (1 - a3);
    v_blend[1] = v_blend[1] * a3 + g * (1 - a3);
    v_blend[2] = v_blend[2] * a3 + b * (1 - a3);
    v_blend[3] = a2;
}

//
//===============
// SVG_CalculateBlend
// 
//===============
//
static void SVG_CalculateBlend(PlayerClient *ent)
{
    // Clear blend values.
    currentProcessingClient->playerState.blend[0] = currentProcessingClient->playerState.blend[1] =
        currentProcessingClient->playerState.blend[2] = currentProcessingClient->playerState.blend[3] = 0;

    // Calculate view origin to use for PointContents.
    vec3_t viewOrigin = ent->GetOrigin() + currentProcessingClient->playerState.pmove.viewOffset;
    int32_t contents = gi.PointContents(viewOrigin);

	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
        currentProcessingClient->playerState.rdflags |= RDF_UNDERWATER;
    else
        currentProcessingClient->playerState.rdflags &= ~RDF_UNDERWATER;

    if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
        SV_AddBlend(1.0, 0.3, 0.0, 0.6, currentProcessingClient->playerState.blend);
    else if (contents & CONTENTS_SLIME)
        SV_AddBlend(0.0, 0.1, 0.05, 0.6, currentProcessingClient->playerState.blend);
    else if (contents & CONTENTS_WATER)
        SV_AddBlend(0.5, 0.3, 0.2, 0.4, currentProcessingClient->playerState.blend);

    // add for damage
    if (currentProcessingClient->damageAlpha > 0)
        SV_AddBlend(currentProcessingClient->damageBlend[0], currentProcessingClient->damageBlend[1]
                    , currentProcessingClient->damageBlend[2], currentProcessingClient->damageAlpha, currentProcessingClient->playerState.blend);

    if (currentProcessingClient->bonusAlpha > 0)
        SV_AddBlend(0.85, 0.7, 0.3, currentProcessingClient->bonusAlpha, currentProcessingClient->playerState.blend);

    // drop the damage value
    currentProcessingClient->damageAlpha -= 0.06;
    if (currentProcessingClient->damageAlpha < 0)
        currentProcessingClient->damageAlpha = 0;

    // drop the bonus value
    currentProcessingClient->bonusAlpha -= 0.1;
    if (currentProcessingClient->bonusAlpha < 0)
        currentProcessingClient->bonusAlpha = 0;
}

//
//===============
// SVG_Player_CheckFallingDamage
// 
//===============
//
static void SVG_Player_CheckFallingDamage(PlayerClient *ent)
{
    float   delta;
    int     damage;
    vec3_t  dir;

    if (ent->GetModelIndex() != 255)
        return;     // not in the player model

    if (ent->GetMoveType() == MoveType::NoClip || ent->GetMoveType() == MoveType::Spectator)
        return;

    // Calculate delta velocity.
    vec3_t velocity = ent->GetVelocity();

    if ((currentProcessingClient->oldVelocity[2] < 0) && (velocity[2] > currentProcessingClient->oldVelocity[2]) && (!ent->GetGroundEntity())) {
        delta = currentProcessingClient->oldVelocity[2];
    } else {
        if (!ent->GetGroundEntity())
            return;
        delta = velocity[2] - currentProcessingClient->oldVelocity[2];
    }
    delta = delta * delta * 0.0001;

    // never take falling damage if completely underwater
    if (ent->GetWaterLevel() == 3)
        return;
    if (ent->GetWaterLevel() == 2)
        delta *= 0.25;
    if (ent->GetWaterLevel() == 1)
        delta *= 0.5;

    if (delta < 1)
        return;

    if (delta < 15) {
        ent->SetEventID(EntityEvent::Footstep);
        return;
    }

    currentProcessingClient->fallValue = delta * 0.5;
    if (currentProcessingClient->fallValue > 40)
        currentProcessingClient->fallValue = 40;
    currentProcessingClient->fallTime = level.time + FALL_TIME;

    if (delta > 30) {
        if (ent->GetHealth() > 0) {
            if (delta >= 55)
                ent->SetEventID(EntityEvent::FallFar);
            else
                ent->SetEventID(EntityEvent::Fall);
        }
        ent->SetDebouncePainTime(level.time);   // no normal pain sound
        damage = (delta - 30) / 2;
        if (damage < 1)
            damage = 1;
        dir = { 0.f, 0.f, 1.f };

        if (!deathmatch->value || !((int)gamemodeflags->value & GameModeFlags::NoFalling))
            SVG_InflictDamage(ent, SVG_GetWorldClassEntity(), SVG_GetWorldClassEntity(), dir, ent->GetOrigin(), vec3_zero(), damage, 0, 0, MeansOfDeath::Falling);
    } else {
        ent->SetEventID(EntityEvent::FallShort);
        return;
    }
}

//
//===============
// SVG_Player_CheckWorldEffects
// 
//===============
//
static void SVG_Player_CheckWorldEffects(void)
{
    int         waterlevel, oldWaterLevel;

    if (!currentProcessingPlayer)
        return;

    if (currentProcessingPlayer->GetMoveType() == MoveType::NoClip || currentProcessingPlayer->GetMoveType() == MoveType::Spectator) {
        currentProcessingPlayer->SetAirFinishedTime(level.time + 12 * FRAMETIME); // don't need air
        return;
    }

    // Retreive waterlevel.
    waterlevel = currentProcessingPlayer->GetWaterLevel();
    oldWaterLevel = currentProcessingClient->oldWaterLevel;
    currentProcessingClient->oldWaterLevel = waterlevel;

    //
    // if just entered a water volume, play a sound
    //
    if (!oldWaterLevel && waterlevel) {
        SVG_PlayerNoise(currentProcessingPlayer, currentProcessingPlayer->GetOrigin(), PNOISE_SELF);
        if (currentProcessingPlayer->GetWaterType() & CONTENTS_LAVA)
            SVG_Sound(currentProcessingPlayer, CHAN_BODY, gi.SoundIndex("player/lava_in.wav"), 1, ATTN_NORM, 0);
        else if (currentProcessingPlayer->GetWaterType() & CONTENTS_SLIME)
            SVG_Sound(currentProcessingPlayer, CHAN_BODY, gi.SoundIndex("player/watr_in.wav"), 1, ATTN_NORM, 0);
        else if (currentProcessingPlayer->GetWaterType() & CONTENTS_WATER)
            SVG_Sound(currentProcessingPlayer, CHAN_BODY, gi.SoundIndex("player/watr_in.wav"), 1, ATTN_NORM, 0);
        currentProcessingPlayer->SetFlags(currentProcessingPlayer->GetFlags() | EntityFlags::InWater);

        // clear damage_debounce, so the pain sound will play immediately
        currentProcessingPlayer->SetDebounceDamageTime(level.time - 1 * FRAMETIME);
    }

    //
    // if just completely exited a water volume, play a sound
    //
    if (oldWaterLevel && ! waterlevel) {
        SVG_PlayerNoise(currentProcessingPlayer, currentProcessingPlayer->GetOrigin(), PNOISE_SELF);
        SVG_Sound(currentProcessingPlayer, CHAN_BODY, gi.SoundIndex("player/watr_out.wav"), 1, ATTN_NORM, 0);
        currentProcessingPlayer->SetFlags(currentProcessingPlayer->GetFlags() & ~EntityFlags::InWater);
    }

    //
    // check for head just going under water
    //
    if (oldWaterLevel != 3 && waterlevel == 3) {
        SVG_Sound(currentProcessingPlayer, CHAN_BODY, gi.SoundIndex("player/watr_un.wav"), 1, ATTN_NORM, 0);
    }

    //
    // check for head just coming out of water
    //
    if (oldWaterLevel == 3 && waterlevel != 3) {
        if (currentProcessingPlayer->GetAirFinishedTime() < level.time) {
            // gasp for air
            SVG_Sound(currentProcessingPlayer, CHAN_VOICE, gi.SoundIndex("player/gasp1.wav"), 1, ATTN_NORM, 0);
            SVG_PlayerNoise(currentProcessingPlayer, currentProcessingPlayer->GetOrigin(), PNOISE_SELF);
        } else  if (currentProcessingPlayer->GetAirFinishedTime() < level.time + 11 * FRAMETIME) {
            // just break surface
            SVG_Sound(currentProcessingPlayer, CHAN_VOICE, gi.SoundIndex("player/gasp2.wav"), 1, ATTN_NORM, 0);
        }
    }

    //
    // check for drowning
    //
    if (waterlevel == 3) {
        // if out of air, start drowning
        if (currentProcessingPlayer->GetAirFinishedTime() < level.time) {
            // drown!
            if (currentProcessingPlayer->GetNextDrownTime() < level.time
                && currentProcessingPlayer->GetHealth() > 0) {
                currentProcessingPlayer->SetNextDrownTime(level.time + 1);

                // take more damage the longer underwater
                currentProcessingPlayer->SetDamage(currentProcessingPlayer->GetDamage() + 2);
                if (currentProcessingPlayer->GetDamage() > 15)
                    currentProcessingPlayer->SetDamage(15);

                // play a gurp sound instead of a normal pain sound
                if (currentProcessingPlayer->GetHealth() <= currentProcessingPlayer->GetDamage())
                    SVG_Sound(currentProcessingPlayer, CHAN_VOICE, gi.SoundIndex("player/drown1.wav"), 1, ATTN_NORM, 0);
                else if (rand() & 1)
                    SVG_Sound(currentProcessingPlayer, CHAN_VOICE, gi.SoundIndex("*gurp1.wav"), 1, ATTN_NORM, 0);
                else
                    SVG_Sound(currentProcessingPlayer, CHAN_VOICE, gi.SoundIndex("*gurp2.wav"), 1, ATTN_NORM, 0);

                currentProcessingPlayer->SetDebouncePainTime(level.time);

                SVG_InflictDamage(currentProcessingPlayer, SVG_GetWorldClassEntity(), SVG_GetWorldClassEntity(), vec3_zero(), currentProcessingPlayer->GetOrigin(), vec3_zero(), currentProcessingPlayer->GetDamage(), 0, DamageFlags::NoArmorProtection, MeansOfDeath::Water);
            }
        }
    } else {
        currentProcessingPlayer->SetAirFinishedTime(level.time + 12 * FRAMETIME);
        currentProcessingPlayer->SetDamage(2);
    }

    //
    // check for sizzle damage
    //
    if (waterlevel && (currentProcessingPlayer->GetWaterType() & (CONTENTS_LAVA | CONTENTS_SLIME))) {
        if (currentProcessingPlayer->GetWaterType() & CONTENTS_LAVA) {
            if (currentProcessingPlayer->GetHealth() > 0
                && currentProcessingPlayer->GetDebouncePainTime() <= level.time) {
                if (rand() & 1)
                    SVG_Sound(currentProcessingPlayer, CHAN_VOICE, gi.SoundIndex("player/burn1.wav"), 1, ATTN_NORM, 0);
                else
                    SVG_Sound(currentProcessingPlayer, CHAN_VOICE, gi.SoundIndex("player/burn2.wav"), 1, ATTN_NORM, 0);
                currentProcessingPlayer->SetDebouncePainTime(level.time + 1 * FRAMETIME);
            }

            SVG_InflictDamage(currentProcessingPlayer, SVG_GetWorldClassEntity(), SVG_GetWorldClassEntity(), vec3_zero(), currentProcessingPlayer->GetOrigin(), vec3_zero(), 3 * waterlevel, 0, 0, MeansOfDeath::Lava);
        }

        if (currentProcessingPlayer->GetWaterType() & CONTENTS_SLIME) {
            SVG_InflictDamage(currentProcessingPlayer, SVG_GetWorldClassEntity(), SVG_GetWorldClassEntity(), vec3_zero(), currentProcessingPlayer->GetOrigin(), vec3_zero(), 1 * waterlevel, 0, 0, MeansOfDeath::Slime);
        }
    }
}

//
//===============
// SVG_SetClientEffects
// 
//===============
//
static void SVG_SetClientEffects(PlayerClient *ent)
{
    ent->SetEffects(0);
    ent->SetRenderEffects(0);

    if (ent->GetHealth() <= 0 || level.intermission.time)
        return;

    // show cheaters!!!
    if (ent->GetFlags() & EntityFlags::GodMode) {
        ent->SetRenderEffects(ent->GetRenderEffects() | (RenderEffects::RedShell | RenderEffects::GreenShell | RenderEffects::BlueShell));
    }
}

//
//===============
// SVG_SetClientEvent
// 
//===============
//
static void SVG_SetClientEvent(PlayerClient *ent)
{
    if (ent->GetEventID())
        return;

    if (ent->GetGroundEntity() && XYSpeed > 225) {
        if ((int)(currentProcessingClient->bobTime + bobMove) != bobCycle)
            ent->SetEventID(EntityEvent::Footstep);
    }
}

//
//===============
// SVG_SetClientSound
// 
//===============
//
static void SVG_SetClientSound(PlayerClient *ent)
{
    const char    *weap; // C++20: STRING: Added const to char*

    if (currentProcessingClient->persistent.activeWeapon)
        weap = currentProcessingClient->persistent.activeWeapon->className;
    else
        weap = "";

    if (ent->GetWaterLevel() && (ent->GetWaterType() & (CONTENTS_LAVA | CONTENTS_SLIME)))
        ent->SetSound(snd_fry);
    else if (strcmp(weap, "weapon_railgun") == 0)
        ent->SetSound(gi.SoundIndex("weapons/rg_hum.wav"));
    else if (strcmp(weap, "weapon_bfg") == 0)
        ent->SetSound(gi.SoundIndex("weapons/bfg_hum.wav"));
    else if (currentProcessingClient->weaponSound)
        ent->SetSound(currentProcessingClient->weaponSound);
    else
        ent->SetSound(0);
}

//
//===============
// SVG_SetClientAnimationFrame
// 
//===============
//
static void SVG_SetClientAnimationFrame(PlayerClient *ent)
{
    qboolean isDucking = false;
    qboolean isRunning = false;

    if (!ent)
        return;

    if (ent->GetModelIndex() != 255)
        return;     // not in the player model

    //client = ent->client;

    if (currentProcessingClient->playerState.pmove.flags & PMF_DUCKED)
        isDucking = true;
    else
        isDucking = false;
    if (XYSpeed)
        isRunning = true;
    else
        isRunning = false;

    // check for stand/duck and stop/go transitions
    if (isDucking != currentProcessingClient->animation.isDucking && currentProcessingClient->animation.priorityAnimation < PlayerAnimation::Death)
        goto newanim;
    if (isRunning != currentProcessingClient->animation.isRunning && currentProcessingClient->animation.priorityAnimation == PlayerAnimation::Basic)
        goto newanim;
    if (!ent->GetGroundEntity() && currentProcessingClient->animation.priorityAnimation <= PlayerAnimation::Wave)
        goto newanim;

    if (currentProcessingClient->animation.priorityAnimation == PlayerAnimation::Reverse) {
        if (ent->GetFrame() > currentProcessingClient->animation.endFrame) {
            ent->SetFrame(ent->GetFrame() - 1);
            return;
        }
    } else if (ent->GetFrame() < currentProcessingClient->animation.endFrame) {
        // continue an animation
        ent->SetFrame(ent->GetFrame() + 1);
        return;
    }

    if (currentProcessingClient->animation.priorityAnimation == PlayerAnimation::Death)
        return;     // stay there
    if (currentProcessingClient->animation.priorityAnimation == PlayerAnimation::Jump) {
        if (!ent->GetGroundEntity())
            return;     // stay there
        currentProcessingClient->animation.priorityAnimation = PlayerAnimation::Wave;
        ent->SetFrame(FRAME_jump3);
        currentProcessingClient->animation.endFrame = FRAME_jump6;
        return;
    }

newanim:
    // return to either a running or standing frame
    currentProcessingClient->animation.priorityAnimation = PlayerAnimation::Basic;
    currentProcessingClient->animation.isDucking = isDucking;
    currentProcessingClient->animation.isRunning = isRunning;

    if (!ent->GetGroundEntity()) {
        currentProcessingClient->animation.priorityAnimation = PlayerAnimation::Jump;
        if (ent->GetFrame() != FRAME_jump2)
            ent->SetFrame(FRAME_jump1);
        currentProcessingClient->animation.endFrame = FRAME_jump2;
    } else if (isRunning) {
        // running
        if (isDucking) {
            ent->SetFrame(FRAME_crwalk1);
            currentProcessingClient->animation.endFrame = FRAME_crwalk6;
        } else {
            ent->SetFrame(FRAME_run1);
            currentProcessingClient->animation.endFrame = FRAME_run6;
        }
    } else {
        // standing
        if (isDucking) {
            ent->SetFrame(FRAME_crstnd01);
            currentProcessingClient->animation.endFrame = FRAME_crstnd19;
        } else {
            ent->SetFrame(FRAME_stand01);
            currentProcessingClient->animation.endFrame = FRAME_stand40;
        }
    }
}

//
//===============
// SVG_ClientEndServerFrame
//
// Called for each player at the end of the server frame and right 
// after spawning.
//===============
//
void SVG_ClientEndServerFrame(PlayerClient *ent)
{
    float   bobTime;

    if (!ent || !ent->GetClient()) {
        return;
    }

    // Setup the current player and entity being processed.
    currentProcessingPlayer = ent;
    currentProcessingClient = ent->GetClient();

    //
    // If the origin or velocity have changed since ClientThink(),
    // update the pmove values.  This will happen when the client
    // is pushed by a bmodel or kicked by an explosion.
    //
    // If it wasn't updated here, the view position would lag a frame
    // behind the body position when pushed -- "sinking into plats"
    //
    currentProcessingClient->playerState.pmove.origin = ent->GetOrigin();
    currentProcessingClient->playerState.pmove.velocity = ent->GetVelocity();

    //
    // If the end of unit layout is displayed, don't give
    // the player any normal movement attributes
    //
    if (level.intermission.time) {
        // FIXME: add view drifting here?
        currentProcessingClient->playerState.blend[3] = 0;
        currentProcessingClient->playerState.fov = 90;
        SVG_HUD_SetClientStats(ent->GetServerEntity());
        return;
    }

    vec3_vectors(currentProcessingClient->aimAngles, &forward, &right, &up);

    // Burn from lava, etc
    SVG_Player_CheckWorldEffects();

    //
    // Set model angles from view angles so other things in
    // the world can tell which direction you are looking
    //
    vec3_t newPlayerAngles = currentProcessingPlayer->GetAngles();

    if (currentProcessingClient->aimAngles[vec3_t::Pitch] > 180)
        newPlayerAngles[vec3_t::Pitch] = (-360 + currentProcessingClient->aimAngles[vec3_t::Pitch]) / 3;
    else
        newPlayerAngles[vec3_t::Pitch] = currentProcessingClient->aimAngles[vec3_t::Pitch] / 3;
    newPlayerAngles[vec3_t::Yaw] = currentProcessingClient->aimAngles[vec3_t::Yaw];
    newPlayerAngles[vec3_t::Roll] = 0;
    newPlayerAngles[vec3_t::Roll] = SVG_CalcRoll(newPlayerAngles, ent->GetVelocity()) * 4;

    // Last but not least, after having calculated the Pitch, Yaw, and Roll, set the new angles.
    currentProcessingPlayer->SetAngles(newPlayerAngles);

    //
    // Calculate the player its X Y axis' speed and calculate the cycle for
    // bobbing based on that.
    //
    vec3_t playerVelocity = ent->GetVelocity();
    XYSpeed = std::sqrtf(playerVelocity[0] * playerVelocity[0] + playerVelocity[1] * playerVelocity[1]);

    if (XYSpeed < 5 || !(currentProcessingClient->playerState.pmove.flags & PMF_ON_GROUND)) {
        // Special handling for when not on ground.
        bobMove = 0;

        // Start at beginning of cycle again (See the else if statement.)
        currentProcessingClient->bobTime = 0;
    } else if (ent->GetGroundEntity() || ent->GetWaterLevel() == 2) {
        // So bobbing only cycles when on ground.
        if (XYSpeed > 450)
            bobMove = 0.25;
        else if (XYSpeed > 210)
            bobMove = 0.125;
        else if (!ent->GetGroundEntity() && ent->GetWaterLevel() == 2 && XYSpeed > 100)
            bobMove = 0.225;
        else if (XYSpeed > 100)
            bobMove = 0.0825;
        else if (!ent->GetGroundEntity() && ent->GetWaterLevel() == 2)
            bobMove = 0.1625;
        else
            bobMove = 0.03125;
    }

    // Generate bob time.
    currentProcessingClient->bobTime += bobMove;
    bobTime = currentProcessingClient->bobTime;

    if (currentProcessingClient->playerState.pmove.flags & PMF_DUCKED)
        bobTime *= 2;   // N&C: Footstep tweak.

    bobCycle = (int)bobTime;
    bobFracsin = std::fabsf(std::sinf(bobTime * M_PI));

    // Detect hitting the floor, and apply damage appropriately.
    SVG_Player_CheckFallingDamage(ent);

    // Apply all other the damage taken this frame
    SVG_Player_ApplyDamageFeedback(ent);

    // Determine the new frame's view offsets
    SVG_CalculateViewOffset(ent);

    // Determine the gun offsets
    SVG_CalculateGunOffset(ent);

    // Determine the full screen color blend
    // must be after viewOffset, so eye contents can be
    // accurately determined
    // FIXME: with client prediction, the contents
    // should be determined by the client
    SVG_CalculateBlend(ent);

    // Set the stats to display for this client (one of the chase isSpectator stats or...)
    if (currentProcessingClient->respawn.isSpectator)
        SVG_HUD_SetSpectatorStats(ent->GetServerEntity());
    else
        SVG_HUD_SetClientStats(ent->GetServerEntity());

    SVG_HUD_CheckChaseStats(ent->GetServerEntity());

    SVG_SetClientEvent(ent);

    SVG_SetClientEffects(ent);

    SVG_SetClientSound(ent);

    SVG_SetClientAnimationFrame(ent);

    // Store velocity and view angles.
    currentProcessingClient->oldVelocity = ent->GetVelocity();
    currentProcessingClient->oldViewAngles = currentProcessingClient->playerState.pmove.viewAngles;

    // Reset weapon kicks to zer0.
    currentProcessingClient->kickOrigin = vec3_zero();
    currentProcessingClient->kickAngles = vec3_zero();

    // if the scoreboard is up, update it
    /*if (currentProcessingClient->showScores && !(level.frameNumber & 31)) {
        SVG_HUD_GenerateDMScoreboardLayout(ent, ent->GetEnemy());
        gi.Unicast(ent->GetServerEntity(), false);
    }*/
}

