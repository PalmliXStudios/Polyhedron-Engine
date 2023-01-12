/***
*
*	License here.
*
*	@file
*
*	Contains definitions of the PlayerMove that are shared with the engine.
*
***/
#pragma once


/**
*   @brief  Engine specific PlayerMove types.
*/
struct EnginePlayerMoveType {
    static constexpr int32_t Dead = 32;     // No movement, but the ability to rotate in place
    static constexpr int32_t Freeze = 33;   // No movement at all
    static constexpr int32_t Gib = 34;      // No movement, different bounding box
};

/**
*   Player movement flags. The game is free to define up to 16 bits.
**/
static constexpr int32_t PMF_ENGINE = (1 << 0);                    // Engine flags first.
static constexpr int32_t PMF_TIME_TELEPORT = (PMF_ENGINE << 1);    // time frozen in place
static constexpr int32_t PMF_NO_PREDICTION = (PMF_ENGINE << 2);    // temporarily disables client side prediction
static constexpr int32_t PMF_EXTRAPOLATING_GROUND_MOVER = (PMF_ENGINE << 3); // temporarily disables setting client origin and velocity.
static constexpr int32_t PMF_GAME = (PMF_ENGINE << 4);             // Game flags start from here.

/**
*   This structure needs to be communicated bit-accurate from the server to the 
*   client to guarantee that prediction stays in sync, [so no floats are used.]
*													   ------------------------ <-- Not anymore :-)
*   
*   If any part of the game code modifies this struct, it will result in a 
*   prediction error of some degree.
**/
struct PlayerMoveState {
	//! Movement type for this movestate, walk, noclip, or...
    uint32_t type = 0;

	//! Player origin for this move state.
    vec3_t origin = vec3_zero();
	//! Player velocity for this move state.
    vec3_t velocity = vec3_zero();

	//! Flags indicating the state, ducked, jump held, etc.
    uint16_t flags = 0;
	//! Time (Each unit = 8ms).
    uint16_t time = 0;
	//! Player gravity.
    uint16_t gravity = 0;

    // Changed by spawns, rotating objects, and teleporters
    vec3_t deltaAngles = vec3_zero(); // Add to command angles to get view direction

    //! View offsets. (Only Z is used atm, beware.)
    vec3_t viewOffset = vec3_zero();
	//! View angles.
    vec3_t viewAngles = vec3_zero();

    //! Step offset, used for stair interpolations.
    float stepOffset = 0.f;
};

/**
*   PlayerMoveInput is part of each client user cmd.
**/
struct PlayerMoveInput {
	//! Duration of the command, in milliseconds
    uint8_t msec = 0;
	//! The final view angles for this command
    vec3_t viewAngles = vec3_zero();
	//! Directional intentions.
    int16_t forwardMove = 0, rightMove = 0, upMove = 0; 
	//! Button Bits bitmask.
    uint8_t buttons = 0;
	//! Impulse command #.
    uint8_t impulse = 0;    // Impulse cmd.
	//! The lightlevel we're standing on, currently unusable since we got no method of identifying it in RTX.
    uint8_t lightLevel = 0; // Lightlevel.
};

/**
*   ClientMoveCommand is sent to the server each client frame
**/
struct ClientMoveCommand {
    PlayerMoveInput input;	//! The player move user input packed for transferring 'over the wire'.

    uint64_t timeSent = 0;      //! Time sent, for calculating pings
    uint64_t timeReceived = 0;  //! Time rcvd, for calculating pings
    uint64_t commandNumber = 0; //! Current commandNumber for this move command frame

    struct {
		//! Simulation Time.
        uint64_t simulationTime = 0;		//! The local simulation time(relative to time at start of application) when prediction was run
		//! Ground Mover (Push Entities) Level Frame Time, and Next Frame Time, used to reproduce the delta move offset at the time of processing the move command.
		uint64_t moverLevelTime = 0;		//! The actual time of this mover at arrival of our server frame.
		uint64_t moverNextLevelTime = 0;	//! The actual extrapolated time of this mover during transition to next frame.
		//! BaseMovers (Frame based movement)
		vec3_t rotatorOffset = vec3_zero(); //! The offset set by extrapolating base movers between last valid received frame player move origin, and that of our current client frame.

		//! Predicted move results at the time of after processing the move command..
		vec3_t origin	= vec3_zero();		//! The predicted origin for this command.
		vec3_t velocity = vec3_zero();		//! The predicted velocity for this command.
        vec3_t error	= vec3_zero();		//! The prediction error for this command.
		int32_t groundEntityNumber = -1;	//! The predicted ground entity that we're standing on.
    } prediction;
};