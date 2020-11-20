/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2000-2002 Mr. Hyde and Mad Dog

This file is part of Lazarus Quake 2 Mod source code.

Lazarus Quake 2 Mod source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Lazarus Quake 2 Mod source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lazarus Quake 2 Mod source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

//
// laz_misc.h- Miscellaneous declarations that really don't belong in q_shared.h...
//

//
// MD2 format info
//
typedef struct
{
	int		ident;
	int		version;

	int		skinwidth;
	int		skinheight;
	int		framesize;	// byte size of each frame

	int		num_skins;
	int		num_xyz;
	int		num_st;		// greater than num_xyz for seams
	int		num_tris;
	int		num_glcmds;	// dwords in strip/fan command list
	int		num_frames;

	int		ofs_skins;	// each skin is a MAX_SKINNAME string
	int		ofs_st;		// byte offset from start for stverts
	int		ofs_tris;	// offset for dtriangles
	int		ofs_frames;	// offset for first frame
	int		ofs_glcmds;	
	int		ofs_end;	// end of file

} dmdl_t;


//
// Entity ID enum
//
typedef enum
{
ENTITY_DONT_USE_THIS_ONE,
ENTITY_ITEM_HEALTH,
ENTITY_ITEM_HEALTH_SMALL,
ENTITY_ITEM_HEALTH_LARGE,
ENTITY_ITEM_HEALTH_MEGA,
ENTITY_INFO_PLAYER_START,
ENTITY_INFO_PLAYER_DEATHMATCH,
ENTITY_INFO_PLAYER_COOP,
ENTITY_INFO_PLAYER_INTERMISSION,
ENTITY_FUNC_PLAT,
ENTITY_FUNC_BUTTON,
ENTITY_FUNC_DOOR,
ENTITY_FUNC_DOOR_SECRET,
ENTITY_FUNC_DOOR_ROTATING,
ENTITY_FUNC_ROTATING,
ENTITY_FUNC_TRAIN,
ENTITY_FUNC_WATER,
ENTITY_FUNC_CONVEYOR,
ENTITY_FUNC_AREAPORTAL,
ENTITY_FUNC_CLOCK,
ENTITY_FUNC_WALL,
ENTITY_FUNC_OBJECT,
ENTITY_FUNC_TIMER,
ENTITY_FUNC_EXPLOSIVE,
ENTITY_FUNC_KILLBOX,
ENTITY_TARGET_ACTOR,
ENTITY_TARGET_ANIMATION,
ENTITY_TARGET_BLASTER,
ENTITY_TARGET_CHANGELEVEL,
ENTITY_TARGET_CHARACTER,
ENTITY_TARGET_CROSSLEVEL_TARGET,
ENTITY_TARGET_CROSSLEVEL_TRIGGER,
ENTITY_TARGET_EARTHQUAKE,
ENTITY_TARGET_EXPLOSION,
ENTITY_TARGET_GOAL,
ENTITY_TARGET_HELP,
ENTITY_TARGET_LASER,
ENTITY_TARGET_LIGHTRAMP,
ENTITY_TARGET_SECRET,
ENTITY_TARGET_SPAWNER,
ENTITY_TARGET_SPEAKER,
ENTITY_TARGET_SPLASH,
ENTITY_TARGET_STRING,
ENTITY_TARGET_TEMP_ENTITY,
ENTITY_TRIGGER_ALWAYS,
ENTITY_TRIGGER_COUNTER,
ENTITY_TRIGGER_ELEVATOR,
ENTITY_TRIGGER_GRAVITY,
ENTITY_TRIGGER_HURT,
ENTITY_TRIGGER_KEY,
ENTITY_TRIGGER_ONCE,
ENTITY_TRIGGER_MONSTERJUMP,
ENTITY_TRIGGER_MULTIPLE,
ENTITY_TRIGGER_PUSH,
ENTITY_TRIGGER_RELAY,
ENTITY_VIEWTHING,
ENTITY_WORLDSPAWN,
ENTITY_LIGHT,
ENTITY_LIGHT_MINE1,
ENTITY_LIGHT_MINE2,
ENTITY_INFO_NOTNULL,
ENTITY_PATH_CORNER,
ENTITY_POINT_COMBAT,
ENTITY_MISC_EXPLOBOX,
ENTITY_MISC_BANNER,
ENTITY_MISC_SATELLITE_DISH,
ENTITY_MISC_ACTOR,
ENTITY_MISC_GIB_ARM,
ENTITY_MISC_GIB_LEG,
ENTITY_MISC_GIB_HEAD,
ENTITY_MISC_INSANE,
ENTITY_MISC_DEADSOLDIER,
ENTITY_MISC_VIPER,
ENTITY_MISC_VIPER_BOMB,
ENTITY_MISC_BIGVIPER,
ENTITY_MISC_STROGG_SHIP,
ENTITY_MISC_TELEPORTER,
ENTITY_MISC_TELEPORTER_DEST,
ENTITY_MISC_BLACKHOLE,
ENTITY_MISC_EASTERTANK,
ENTITY_MISC_EASTERCHICK,
ENTITY_MISC_EASTERCHICK2,
ENTITY_MONSTER_BERSERK,
ENTITY_MONSTER_GLADIATOR,
ENTITY_MONSTER_GUNNER,
ENTITY_MONSTER_INFANTRY,
ENTITY_MONSTER_SOLDIER_LIGHT,
ENTITY_MONSTER_SOLDIER,
ENTITY_MONSTER_SOLDIER_SS,
ENTITY_MONSTER_TANK,
ENTITY_MONSTER_MEDIC,
ENTITY_MONSTER_FLIPPER,
ENTITY_MONSTER_CHICK,
ENTITY_MONSTER_PARASITE,
ENTITY_MONSTER_FLYER,
ENTITY_MONSTER_BRAIN,
ENTITY_MONSTER_FLOATER,
ENTITY_MONSTER_HOVER,
ENTITY_MONSTER_MUTANT,
ENTITY_MONSTER_SUPERTANK,
ENTITY_MONSTER_BOSS2,
ENTITY_MONSTER_BOSS3_STAND,
ENTITY_MONSTER_JORG,
ENTITY_MONSTER_COMMANDER_BODY,
ENTITY_TURRET_BREACH,
ENTITY_TURRET_BASE,
ENTITY_TURRET_DRIVER,
ENTITY_CRANE_BEAM,
ENTITY_CRANE_HOIST,
ENTITY_CRANE_HOOK,
ENTITY_CRANE_CONTROL,
ENTITY_CRANE_RESET,
ENTITY_FUNC_BOBBINGWATER,
ENTITY_FUNC_DOOR_SWINGING,
ENTITY_FUNC_FORCE_WALL,
ENTITY_FUNC_MONITOR,
ENTITY_FUNC_PENDULUM,
ENTITY_FUNC_PIVOT,
ENTITY_FUNC_PUSHABLE,
ENTITY_FUNC_REFLECT,
ENTITY_FUNC_TRACKCHANGE,
ENTITY_FUNC_TRACKTRAIN,
ENTITY_FUNC_TRAINBUTTON,
ENTITY_FUNC_VEHICLE,
ENTITY_HINT_PATH,
ENTITY_INFO_TRAIN_START,
ENTITY_MISC_LIGHT,
ENTITY_MODEL_SPAWN,
ENTITY_MODEL_TRAIN,
ENTITY_MODEL_TURRET,
ENTITY_MONSTER_MAKRON,
ENTITY_PATH_TRACK,
ENTITY_TARGET_ANGER,
ENTITY_TARGET_ATTRACTOR,
ENTITY_TARGET_CD,
ENTITY_TARGET_CHANGE,
ENTITY_TARGET_CLONE,
ENTITY_TARGET_EFFECT,
ENTITY_TARGET_FADE,
ENTITY_TARGET_FAILURE,
ENTITY_TARGET_FOG,
ENTITY_TARGET_FOUNTAIN,
ENTITY_TARGET_LIGHTSWITCH,
ENTITY_TARGET_LOCATOR,
ENTITY_TARGET_LOCK,
ENTITY_TARGET_LOCK_CLUE,
ENTITY_TARGET_LOCK_CODE,
ENTITY_TARGET_LOCK_DIGIT,
ENTITY_TARGET_MONITOR,
ENTITY_TARGET_MONSTERBATTLE,
ENTITY_TARGET_MOVEWITH,
ENTITY_TARGET_PRECIPITATION,
ENTITY_TARGET_ROCKS,
ENTITY_TARGET_ROTATION,
ENTITY_TARGET_SET_EFFECT,
ENTITY_TARGET_SKILL,
ENTITY_TARGET_SKY,
ENTITY_TARGET_PLAYBACK,
ENTITY_TARGET_TEXT,
ENTITY_THING,
ENTITY_TREMOR_TRIGGER_MULTIPLE,
ENTITY_TRIGGER_BBOX,
ENTITY_TRIGGER_DISGUISE,
ENTITY_TRIGGER_FOG,
ENTITY_TRIGGER_INSIDE,
ENTITY_TRIGGER_LOOK,
ENTITY_TRIGGER_MASS,
ENTITY_TRIGGER_SCALES,
ENTITY_TRIGGER_SPEAKER,
ENTITY_TRIGGER_SWITCH,
ENTITY_TRIGGER_TELEPORTER,
ENTITY_TRIGGER_TRANSITION,
ENTITY_BOLT,
ENTITY_DEBRIS,
ENTITY_GIB,
ENTITY_GIBHEAD,
ENTITY_GRENADE,
ENTITY_HANDGRENADE,
ENTITY_ROCKET,
ENTITY_CHASECAM,
ENTITY_CAMPLAYER,
ENTITY_PLAYER_NOISE,
ENTITY_TARGET_GRFOG,
ENTITY_TRIGGER_GRFOG,
ENTITY_TRIGGER_REVERB_PRESET,
ENTITY_TRIGGER_REVERB,
ENTITY_TRIGGER_GODRAYS,
ENTITY_TRIGGER_SUN


} entity_id;


// ==================
//#define DEG2RAD( a ) ( a * M_PI ) / 180.0F
