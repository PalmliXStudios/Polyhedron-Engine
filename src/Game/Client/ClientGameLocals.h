/***
*
*	License here.
*
*	@file
* 
*   Client Game Locals.
*
***/
#pragma once

// Define CGAME_INCLUDE so that files such as:
// Common/cmodel.h
// Common/cmd.h
//
#define CGAME_INCLUDE 1

// Shared.
#include "../../Shared/Shared.h"
#include "../../Shared/List.h"
#include "../../Shared/Refresh.h"

// Common.
#include "../../Common/CModel.h"
#include "../../Common/Cmd.h"
#include "../../Common/Msg.h"
#include "../../Common/Protocol.h"

// Shared Client Type & Game Header.
#include "../../Shared/CLTypes.h"
#include "../../Shared/CLGame.h"

// Shared Game "Framework".
#include "../Shared/SharedGame.h"

// ClientGame Exports Interface.
#include "../../Shared/Interfaces/IClientGameExports.h"

// ClientGame Exports Implementation.
#include "ClientGameExports.h"

//// Temporary.
#include "LevelLocals.h"
extern LevelLocals level;
// END OF TEMPORARY.

/**
*
*
*   ClientGame Frame Time.
*
*
**/
// WID: TODO: Make these part of the ClientGameImports instead.
static constexpr double		CLG_FRAMETIME	= BASE_FRAMETIME;
static constexpr double		CLG_1_FRAMETIME	= BASE_1_FRAMETIME;
static constexpr int32_t	CLG_FRAMEDIV	= BASE_FRAMERATE / 10.0;

// THESE SHOULD NOT BE ADDED TO IMPORTS.
//! MS Frametime for animations.
static constexpr float ANIMATION_FRAMETIME = BASE_FRAMETIME;//FRAMERATE_MS;

//! Float time it takes to go over a frame. 
static constexpr Frametime FRAMETIME = FRAMETIME_S;


// Preecdclarations.
class ClientGameExports;
class ClientGameworld;

//! Contains the function pointers being passed in from the engine.
extern ClientGameImport clgi;
//! Static export variable, lives as long as the client game dll lives.
extern ClientGameExports* clge;

/**
*	@brief	Common print text to screen function.
**/
void Com_Print(const char* fmt, ...);

/**
*	@brief	Common print debug text to screen function.
**/
void Com_DPrint(const char* fmt, ...);

/**
*	@brief	Common print warning text to screen function.
**/
void Com_WPrint(const char* fmt, ...);

/**
*	@brief	Common print error text to screen function.
**/
void Com_EPrint(const char* fmt, ...);

/**
*	@brief	Common print specific error type and text to screen function.
**/
void Com_Error(int32_t errorType, const char* fmt, ...);

/**
*	@brief	Common print text type of your choice to screen function.
**/
void Com_LPrintf(int32_t printType, const char* fmt, ...);



/**
*
*
*   Client Game structures and definitions.
*
*
**/
/**
*	ClientGame Trace Results.
**/
#include "Utilities/CLGTraceResult.h"

/**
*	@brief GameLocal is the main server game class.
* 
*	@details 
**/
class ClientGameLocals {
public:
    /**
	*	@brief Default constructor.
	**/
    ClientGameLocals() = default;

    /**
	*	@brief Default destructor
	**/
    ~ClientGameLocals() = default;

public:
    /**
	*	@brief Initializes the gameworld and its member objects.
	**/
    void Initialize();
    
    /**
	*	@brief Shutsdown the gamelocal.
	**/
    void Shutdown();



    /**
    *   @return A pointer to the gameworld object.
    **/
    ClientGameworld* GetGameworld();

    /**
    *   @return A pointer to the gameworld its current gamemode object.
    **/
    //IGamemode* GetGamemode();

    /**
    *   @brief  Code shortcut for accessing gameworld's client array.
    * 
    *   @return A pointer to the gameworld's clients array.
    **/
    ServerClient* GetClients();
    /**
    *   @brief  Code shortcut for acquiring gameworld's maxClients.
    * 
    *   @return The maximum allowed clients in this game.
    **/
    int32_t GetMaxClients();
    /**
    *   @brief  Code shortcut for acquiring gameworld's maxEntities.
    * 
    *   @return The maximum allowed entities in this game.
    **/
    int32_t GetMaxEntities();



    /**
    *   
    **/
    
    /**
    *   
    **/
private:
    /**
    *   @brief Create the world member object and initialize it.
    **/
    void CreateWorld();
    /**
    *   @brief De-initialize the world and destroy it.
    **/
    void DestroyWorld();


    // TODO: Add Get methods and privatize the members below.
public:
    //! Gameworld.
    ClientGameworld* world = nullptr;

    //! needed for coop respawns
    //! Can't store spawnpoint32_t in level, because
    //! it would get overwritten by the savegame restore
    char spawnpoint[512];

    //! Will be set to latched cvar equivelants due to having to access them a lot.
    //int32_t maxClients = 0;
    //int32_t maxEntities = 0;

    //! Used to store Cross level triggers.
    int32_t serverflags = 0;

    //! Did we autosave?
    qboolean autoSaved = false;
};

// Custom load state enumerator.
//
// Rename LOAD_CUSTOM_# or add your own.
// Once the load stage is set, the client will inquire the
// CLG_GetMediaLoadStateName function for a matching display string.
typedef enum {
    LOAD_CUSTOM_START = LOAD_SOUNDS + 1,    // DO NOT TOUCH.
    LOAD_CUSTOM_0,  // Let thy will be known, rename to your hearts content.
    LOAD_CUSTOM_1,  // Let thy will be known, rename to your hearts content.
    LOAD_CUSTOM_2   // Let thy will be known, rename to your hearts content.
    // You can add more here if you desire so.
} clg_load_state_t;

/**
*	Client player model settings.
**/
static constexpr int32_t CL_PLAYER_MODEL_DISABLED       = 0;
static constexpr int32_t CL_PLAYER_MODEL_ONLY_GUN       = 1;
static constexpr int32_t CL_PLAYER_MODEL_FIRST_PERSON   = 2;
static constexpr int32_t CL_PLAYER_MODEL_THIRD_PERSON   = 3;

/**
*	Core - Used to access the client's internals.
**/
extern ClientGameImport clgi;
extern ClientState      *cl;
extern ClientShared     *cs;

/**
*	Game - Specific to the game itself.
**/
// Stores parameters parsed from a temporary entity message.s
extern tent_params_t   teParameters;
// Stores parameters parsed from a muzzleflash message.
extern mz_params_t     mzParameters;


/***
*
*
*	CVars - Externed so they can be accessed all over the CG Module.
*
*
***/
// Client.
extern cvar_t* cl_chat_notify;
extern cvar_t* cl_chat_sound;
extern cvar_t* cl_chat_filter;

extern cvar_t* cl_disable_explosions;
extern cvar_t* cl_explosion_sprites;
extern cvar_t* cl_explosion_frametime;
extern cvar_t* cl_disable_particles;
extern cvar_t* cl_footsteps;
extern cvar_t* cl_gunalpha;
extern cvar_t* cl_kickangles;
extern cvar_t* cl_monsterfootsteps;
extern cvar_t* cl_noglow;
extern cvar_t* cl_noskins;
extern cvar_t* cl_player_model;
extern cvar_t* cl_predict;
extern cvar_t* cl_rollhack;
extern cvar_t* cl_thirdperson_angle;
extern cvar_t* cl_thirdperson_range;
extern cvar_t* cl_vwep;

// Refresh... TODO: Move.
extern cvar_t* cvar_pt_beam_lights;

// Server.
extern cvar_t* sv_paused;

// User Info.
extern cvar_t* info_fov;
extern cvar_t* info_hand;
extern cvar_t* info_msg;
extern cvar_t* info_name;
extern cvar_t* info_password;
extern cvar_t* info_skin;
extern cvar_t* info_spectator;
extern cvar_t* info_uf;
extern cvar_t* info_in_bspmenu;     // Is set to 1  at boot time when loading mainmenu.bsp, and is set 
                                    // to 1 when disconnecting from a server hence, once again, loading mainmenu.bsp
// Video.
extern cvar_t* vid_rtx;     // 1 if we're in RTX mode, 0 if not.