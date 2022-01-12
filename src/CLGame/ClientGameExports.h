// License here.
// 
//
// ClientGameExports implementation.
#pragma once

#include "shared/interfaces/IClientGameExports.h"

//---------------------------------------------------------------------
// MAIN interface to implement. It holds pointers to actual sub interfaces,
// which one of course has to implement as well.
//---------------------------------------------------------------------
class ClientGameExports : public IClientGameExports {
public:
    //---------------------------------------------------------------------
    // General.
    //---------------------------------------------------------------------
    // Calculates the FOV the client is running. (Important to have in order.)
    float ClientCalculateFieldOfView(float fieldOfViewX, float width, float height) final;
    // Called when a demo is being seeked through.
    void DemoSeek() final;

    //---------------------------------------------------------------------
    // Frame and State related.
    //---------------------------------------------------------------------
    // Called after all downloads are done. (Aka, a map has started.)
    // Not used for demos.
    void ClientBegin() final;
    // Called upon whenever a client disconnects, for whichever reason.
    // Could be him quiting, or pinging out etc.
    void ClientClearState() final;
    // Called each VALID client frame. Handle per VALID frame basis 
    // things here.
    void ClientDeltaFrame() final;
    // Called each client frame. Handle per frame basis things here.
    void ClientFrame() final;
    // Called when a disconnect even occures. Including those for Com_Error
    void ClientDisconnect() final;

    //---------------------------------------------------------------------
    // Update related.
    //---------------------------------------------------------------------
    // Updates the origin. (Used by the engine for determining current audio position too.)
    void ClientUpdateOrigin() final;
    // Called when there is a needed retransmit of user info variables.
    void ClientUpdateUserinfo(cvar_t* var, from_t from) final;

private:
    // Utility function for ClientUpdateOrigin
    float LerpFieldOfView(float oldFieldOfView, float newFieldOfView, float lerp);
};

