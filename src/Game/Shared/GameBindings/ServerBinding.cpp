/***
*
*	License here.
*
*	@file
*
*	Implementations for the SharedGame(SG) that wrap around the required ServerGame(SVG) 
*	functionalities.
* 
***/
// SharedGame header itself.
#include "Game/Shared/GameBindings/GameModuleImports.h"


/***
*
*
*	Error and Print Functions.
*
*
***/
/**
*	@brief	Wraps around Com_Error.
*	@param	errorType		One of the error types in ErrorType::
*	@param	errorMessage	A string, possibly formatted using fmt::format.
**/
void SG_Error( int32_t errorType, const std::string &errorMessage ) {
	// Call and pass on the arguments to our imported Com_Error.
	gi.Error( errorType, "%s", errorMessage.c_str() );
}

/**
*	@brief	Wraps around Com_LPrintf
*	@param	printType		One of the print types in PrintType::
*	@param	printMessage	A string, possibly formatted using fmt::format.
**/
void SG_Print( int32_t printType, const std::string &printMessage ) {
	// Call and pass on the arguments to our imported Com_LPrintf.
	gi.BPrintf( printType, "%s", printMessage.c_str() );
}



/***
*
*
*	Entity Functions.
*
*
***/
/**
*	@brief	An easy way to acquire the proper entity number from a POD Entity.
*	@return	-1 if the entity was a (nullptr).
**/
const int32_t SG_GetEntityNumber( const PODEntity *podEntity ) {
	// Test for a nullptr entity and return -1 if compliant.
	if ( podEntity == nullptr ) {
		return -1;
	}

	// Return the appropriate entity number.
	return podEntity->currentState.number;
}

/**
*	@brief	An easy way to acquire the proper entity number from a Game Entity.
*	@return	On success: POD Entity Number. On failure: -1 if the Game Entity or its belonging POD Entity was a (nullptr).
**/
const int32_t SG_GetEntityNumber( GameEntity *gameEntity ) {
	// Test for a nullptr entity and return -1 if compliant.
	if ( gameEntity == nullptr ) {
		return -1;
	}

	// Return the appropriate entity number.
	return SG_GetEntityNumber( gameEntity->GetPODEntity() );
}

/**
*	@brief	An easy way to acquire the proper POD Entity by its number.
*	@return	(nullptr) in case of failure. Entity might be nonexistent.
**/
PODEntity *SG_GetPODEntityByNumber( const int32_t entityNumber ) {
	ServerGameWorld *world = GetGameWorld();
	if ( world ) {
		return world->GetPODEntityByIndex( entityNumber );
	}
	return nullptr;
}
/**
*	@brief	An easy way to acquire the proper POD Entity by its number.
*	@return	(nullptr) in case of failure. Entity might be nonexistent.
**/
GameEntity *SG_GetGameEntityByNumber( const int32_t entityNumber ) {
	ServerGameWorld *world = GetGameWorld();
	if ( world ) {
		return world->GetGameEntityByIndex( entityNumber );
	}
	return nullptr;
}



/***
*
*
*	CVar Access.
*
*
***/
/**
*	@return	Pointer to sv_maxvelocity cvar that resides on the Server side.
**/
cvar_t *GetSVMaxVelocity() {
	return sv_maxvelocity;
}
/**
*	@return	Pointer to sv_gravity cvar that resides on the Server side.
**/
cvar_t *GetSVGravity() {
	return sv_gravity;
}