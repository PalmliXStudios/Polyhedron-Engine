// License here.
// 
//
// Interface that a client game dll his exports have to implement in order to
// be fully coherent with the actual client loading it in.
// 
// WID: Time to re-adjust here with new files. I agree, at last.
#pragma once

struct PlayerMove;
struct ClientEntity;

//---------------------------------------------------------------------
// CORE Export Interface.
//---------------------------------------------------------------------
class IClientGameExportCore {
public:
    virtual ~IClientGameExportCore() = default;
    
    //---------------------------------------------------------------------
	// API Version.
	// 
	// The version numbers will always be equal to those that were set in 
	// CMake at the time of building the engine/game(dll/so) binaries.
	// 
	// In an ideal world, we comply to proper version releasing rules.
	// For Polyhedron FPS, the general following rules apply:
	// --------------------------------------------------------------------
	// MAJOR: Ground breaking new features, you can expect anything to be 
	// incompatible at that.
	// 
	// MINOR : Everytime we have added a new feature, or if the API between
	// the Client / Server and belonging game counter-parts has actually 
	// changed.
	// 
	// POINT : Whenever changes have been made, and the above condition 
	// is not met.
	//---------------------------------------------------------------------
	struct APIVersion {
		int32_t major{ VERSION_MAJOR };
		int32_t minor{ VERSION_MINOR };
		int32_t point{ VERSION_POINT };
	} version;

	// Initializes the CLGame.
	virtual void Initialize() = 0;

	// Shuts down the CLGame.
	virtual void Shutdown() = 0;
};

//---------------------------------------------------------------------
// ENTITY interface to implement. 
//---------------------------------------------------------------------
class IClientGameExportEntities {
public:
    virtual ~IClientGameExportEntities()  = default;
     
    /**
    *   @brief  Parses and spawns the local class entities in the BSP Entity String.
    * 
    *   @details    When a class isn't locally registered, it'll automatically spawn
    *               a CLGBaseEntity instead which has all the default behaviors that
    *               you'd expect for it to be functional.
    * 
    *   @return True on success.
    **/
    virtual qboolean SpawnFromBSPString(const char* entities) = 0;

    /**
    *   @brief  When the client receives state updates it calls into this function so we can update
    *           the class entity belonging to the server side entity(defined by state.number).
    * 
    *           If the hashed classname differs, we allocate a new one instead. Also we ensure to 
    *           always update its ClientEntity pointer to the appropriate new one instead.
    * 
    *   @return True on success, false in case of trouble. (Should never happen, and if it does,
    *           well... file an issue lmao.)
    **/
    virtual qboolean UpdateFromState(ClientEntity *clEntity, const EntityState &state) = 0;

    /**
    *   @brief  Executed whenever an entity event is receieved.
    **/
    virtual void Event(int32_t number) = 0;

    /**
    *   @brief  Parse the server frame for server entities to add to our client view.
    *           Also applies special rendering effects to them where desired.
    **/
    virtual void AddPacketEntities() = 0;

    /**
    *   @brief  Add the view weapon render entity to the screen. Can also be used for
    *           other scenarios where a depth hack is required.
    **/
    virtual void AddViewEntities() = 0;
};

//---------------------------------------------------------------------
// MEDIA interface.
//---------------------------------------------------------------------
class IClientGameExportMedia {
public:
    //! Destructor.
    virtual ~IClientGameExportMedia() = default;



    /**
    *   @brief Called upon initialization of the renderer.
    **/
    virtual void Initialize() = 0;

    /**
    *   @brief Called when the client stops the renderer.
    * 
    *   @details    Used to unload remaining data.
    **/
    virtual void Shutdown() = 0;

    /**
    *   @brief Called when the client wants to acquire the name of a load state.
    **/
    virtual std::string GetLoadStateName(int32_t loadState) = 0;

    /**
    *   @brief  This is called when the client spawns into a server,
    *   
    *   @details    Used to register world related media (particles, view models, sounds).
    **/
    virtual void LoadWorld() = 0;
};

//---------------------------------------------------------------------
// MOVEMENT interface.
//---------------------------------------------------------------------
class IClientGameExportMovement {
public:
    //! Destructor.
    virtual ~IClientGameExportMovement() = default;

    /**
    *   @brief  Called when the movement command needs to be build for the given
    *           client networking frame.
    **/
    virtual void BuildFrameMovementCommand(uint64_t miliseconds) = 0;
    /**
    *   @brief  Finalize the movement user command before sending it to server.
    **/
    virtual void FinalizeFrameMovementCommand() = 0;
};

//---------------------------------------------------------------------
// Predict Movement (Client Side)
//---------------------------------------------------------------------
class IClientGameExportPrediction {
public:
    virtual ~IClientGameExportPrediction() = default;

    virtual void CheckPredictionError(ClientMoveCommand* moveCommand) = 0;
    virtual void PredictAngles() = 0;
    virtual void PredictMovement(uint32_t acknowledgedCommandIndex, uint32_t currentCommandIndex) = 0;

    virtual void UpdateClientSoundSpecialEffects(PlayerMove* pm) = 0;
};

//---------------------------------------------------------------------
// Screen
//---------------------------------------------------------------------
class IClientGameExportScreen {
public:
    virtual ~IClientGameExportScreen() = default;

    /**
    *   @brief  Called when the engine decides to render the 2D display.
    **/
    virtual void Initialize() = 0;
    /**
    *   @brief  Called when the client starts or when the renderer demands so
    *           after having modified its settings. Used to register basic 
    *           screen media, 2D icons etc.
    **/
    virtual void RegisterMedia() = 0;
    /**
    *   @brief  Called when the engine decides to render the 2D display.
    **/
    virtual void Shutdown() = 0;

    /**
    *   @brief  Called when the engine decides to render the 2D display.
    **/
    virtual void RenderScreen() = 0;
    /**
    *   @brief  Called when the screen mode has changed.
    **/
    virtual void ScreenModeChanged() = 0;
    /**
    *   @brief  Called when the client wants to render the loading screen.
    **/
    virtual void DrawLoadScreen() = 0;
    /**
    *   @brief  Called when the client wants to render the pause screen.
    **/
    virtual void DrawPauseScreen() = 0;
};

//---------------------------------------------------------------------
// ServerMessage Parsing.
//---------------------------------------------------------------------
class IClientGameExportServerMessage {
public:
    virtual ~IClientGameExportServerMessage() = default;

    /**
    *   @brief  Breaks up playerskin into name(optional), modeland skin components.
    *           If model or skin are found to be invalid, replaces them with sane defaults.
    **/
    virtual qboolean ParsePlayerSkin(char* name, char* model, char* skin, const char* str) = 0;
    /**
    *   @brief  Breaks up playerskin into name(optional), modeland skin components.
    *           If model or skin are found to be invalid, replaces them with sane defaults.
    **/
    virtual qboolean UpdateConfigString(int32_t index, const char* str) = 0;
    
    /**
    *   @brief  Called at the start of receiving a server message.
    **/
    virtual void Start() = 0;
    /**
    *   @brief  Actually parses the server message, and handles it accordingly.
    *   @return True if the message was succesfully parsed. False in case the message was unkown, or corrupted, etc.
    **/
    virtual qboolean ParseMessage(int32_t serverCommand) = 0;
    /**
    *   @brief  Handles the demo message during playback.
    *   @return True if the message was succesfully parsed. False in case the message was unkown, or corrupted, etc.
    **/
    virtual qboolean SeekDemoMessage(int32_t demoCommand) = 0;
    /**
    *   @brief  Called when we're done receiving a server message.
    **/
    virtual void End(int32_t realTime) = 0;
};

//---------------------------------------------------------------------
// View
//---------------------------------------------------------------------
class IClientGameExportView {
public:
    virtual ~IClientGameExportView() = default;

    // Called right after the engine clears the scene, and begins a new one.
    virtual void PreRenderView() = 0;
    // Called whenever the engine wants to clear the scene.
    virtual void ClearScene() = 0;
    // Called whenever the engine wants to render a valid frame.
    virtual void RenderView() = 0;
    // Called right after the engine renders the scene, and prepares to
    // finish up its current frame loop iteration.
    virtual void PostRenderView() = 0;
};

//---------------------------------------------------------------------
// MAIN interface to implement. It holds pointers to actual sub interfaces,
// which one of course has to implement as well.
//---------------------------------------------------------------------
class IClientGameExports {
public:
    //! Default destructor.
    virtual ~IClientGameExports() = default;


    /***
    *
    *
    *   Interface Accessors.
    *
    *
    ***/
    /**
    *   @return A pointer to the client game's core interface.
    **/
    virtual IClientGameExportCore *GetCoreInterface() = 0;

    /**
    *   @return A pointer to the client game module's entities interface.
    **/
    virtual IClientGameExportEntities *GetEntityInterface() = 0;

    /**
    *   @return A pointer to the client game module's media interface.
    **/
    virtual IClientGameExportMedia *GetMediaInterface() = 0;

    /**
    *   @return A pointer to the client game module's movement interface.
    **/
    virtual IClientGameExportMovement *GetMovementInterface() = 0;

    /**
    *   @return A pointer to the client game module's prediction interface.
    **/
    virtual IClientGameExportPrediction *GetPredictionInterface() = 0;

    /**
    *   @return A pointer to the client game module's screen interface.
    **/
    virtual IClientGameExportScreen *GetScreenInterface() = 0;

    /**
    *   @return A pointer to the client game module's servermessage interface.
    **/
    virtual IClientGameExportServerMessage *GetServerMessageInterface() = 0;

    /**
    *   @return A pointer to the client game module's view interface.
    **/
    virtual IClientGameExportView *GetViewInterface() = 0;
    


    /***
    *
    *
    *   General.
    *
    *
    ***/
    // Calculates the FOV the client is running. (Important to have in order.)
    virtual float ClientCalculateFieldOfView(float fieldOfViewX, float width, float height) = 0;

    // Called upon whenever a client disconnects, for whichever reason.
    // Could be him quiting, or pinging out etc.
    virtual void ClientClearState() = 0;

    // Called when a demo is being seeked through.
    virtual void DemoSeek() = 0;

#ifdef _DEBUG
    /**
    *   @brief  For debugging problems when out-of-date entity origin is referenced.
    **/
    virtual void CheckEntityPresent(int32_t entityNumber, const std::string &what) = 0;
#endif

    // Called after all downloads are done. (Aka, a map has started.)
    // Not used for demos.
    virtual void ClientBegin() = 0;
    // Called each VALID client frame. Handle per VALID frame basis 
    // things here.
    virtual void ClientDeltaFrame() = 0;
    // Called each client frame. Handle per frame basis things here.
    virtual void ClientFrame() = 0;
    // Called when a disconnect even occures. Including those for Com_Error
    virtual void ClientDisconnect() = 0;

    // Updates the origin. (Used by the engine for determining current audio position too.)
    virtual void ClientUpdateOrigin() = 0;

    // Called when there is a needed retransmit of user info variables.
    virtual void ClientUpdateUserinfo(cvar_t* var, from_t from) = 0;
};

