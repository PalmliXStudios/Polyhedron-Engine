
// LICENSE HERE.

//
// clg_input.c
//
//
// Handles the client specific Player Move(PM) input processing.
//
#include "ClientGameLocal.h"

#include "Input.h"
#include "Main.h"

static cvar_t* m_filter;
static cvar_t* m_accel;
static cvar_t* m_autosens;

static cvar_t* cl_upspeed;
static cvar_t* cl_forwardspeed;
static cvar_t* cl_sidespeed;
static cvar_t* cl_yawspeed;
static cvar_t* cl_pitchspeed;
//static cvar_t* cl_run; // WID: TODO: This one is old.
static cvar_t* cl_anglespeedkey;
cvar_t* cl_run; // WID: TODO: This is the new one, so it can be externed.

static cvar_t* cl_instantpacket;

static cvar_t* sensitivity;

static cvar_t* m_pitch;
static cvar_t* m_invert;
static cvar_t* m_yaw;
static cvar_t* m_forward;
static cvar_t* m_side;

typedef struct {
    int         oldDeltaX;
    int         oldDeltaY;
} in_state_t;

static in_state_t inputState;

//
//=============================================================================
//
// KEY INPUT SAMPLING
//
// Continuous button event tracking is complicated by the fact that two different
// input sources (say, mouse button 1 and the control key) can both press the
// same button, but the button should only be released when both of the
// pressing key have been released.
// 
// When a key event issues a button command (+forward, +attack, etc), it appends
// its key number as a parameter to the command so it can be matched up with
// the release.
// 
// state bit 0 is the current state of the key
// state bit 1 is edge triggered on the up to down transition
// state bit 2 is edge triggered on the down to up transition
// 
// 
// Key_Event (int key, qboolean down, unsigned time);
// 
// +mlook src time
// 
//==============================================================================
// 
KeyBinding in_klook;
KeyBinding in_left, in_right, in_forward, in_back;
KeyBinding in_lookup, in_lookdown, in_moveleft, in_moveright;
KeyBinding in_strafe, in_speed, in_use, in_reload, in_primary_fire, in_secondary_fire;
KeyBinding in_up, in_down;

int32_t         in_impulse;
static qboolean in_mlooking;

//
//===============
// CLG_KeyDown
// 
// Processes the 'down' state of the key binding.
//================
//
static void CLG_KeyDown(KeyBinding* b)
{
    int k;
    unsigned com_eventTime = clgi.Com_GetEventTime();
    const char* c; // C++20: STRING: Added const to char*

    c = clgi.Cmd_Argv(1);
    if (c[0])
        k = atoi(c);
    else
        k = -1;        // typed manually at the console for continuous down

    if (k == b->keys[0] || k == b->keys[1])
        return;        // repeating key

    if (!b->keys[0])
        b->keys[0] = k;
    else if (!b->keys[1])
        b->keys[1] = k;
    else {
        Com_WPrint("Three keys down for a button!\n");
        return;
    }

    if (b->state & BUTTON_STATE_HELD)
        return;        // still down

    // save timeStamp
    c = clgi.Cmd_Argv(2);
    b->downtime = atoi(c);
    if (!b->downtime) {
        b->downtime = com_eventTime - 100;
    }

    b->state |= BUTTON_STATE_HELD + BUTTON_STATE_DOWN;    // down + impulse down
}

//
//===============
// CLG_KeyUp
// 
// Processes the 'up' state of the key binding.
//================
//
static void CLG_KeyUp(KeyBinding* b)
{
    int k;
    const char* c; // C++20: STRING: Added const to char*
    unsigned uptime;

    c = clgi.Cmd_Argv(1);
    if (c[0])
        k = atoi(c);
    else {
        // typed manually at the console, assume for unsticking, so clear all
        b->keys[0] = b->keys[1] = 0;
        b->state = 0;    // impulse up
        return;
    }

    if (b->keys[0] == k)
        b->keys[0] = 0;
    else if (b->keys[1] == k)
        b->keys[1] = 0;
    else
        return;        // key up without coresponding down (menu pass through)
    if (b->keys[0] || b->keys[1])
        return;        // some other key is still holding it down

    if (!(b->state & BUTTON_STATE_HELD))
        return;        // still up (this should not happen)

    // save timeStamp
    c = clgi.Cmd_Argv(2);
    uptime = atoi(c);
    if (!uptime) {
        b->msec += 10;
    }
    else if (uptime > b->downtime) {
        b->msec += uptime - b->downtime;
    }

    b->state &= ~BUTTON_STATE_HELD;        // now up
}

//
//===============
// CLG_KeyClear
// 
// Clears the key binding state.
//================
//
void CLG_KeyClear(KeyBinding* b)
{
    unsigned com_eventTime = clgi.Com_GetEventTime();
    b->msec = 0;
    b->state &= ~BUTTON_STATE_DOWN;        // clear impulses
    if (b->state & BUTTON_STATE_HELD) {
        b->downtime = com_eventTime; // still down
    }
}

//
//===============
// CLG_KeyState
// 
// Returns the fraction of the frame that the key was down
//================
//
static float CLG_KeyState(KeyBinding* key)
{
    unsigned com_eventTime = clgi.Com_GetEventTime();
    unsigned msec = key->msec;
    float val;

    if (key->state & BUTTON_STATE_HELD) {
        // still down
        if (com_eventTime > key->downtime) {
            msec += com_eventTime - key->downtime;
        }
    }

    // special case for instant packet
    if (!cl->moveCommand.input.msec) {
        return (float)(key->state & BUTTON_STATE_HELD);
    }

    val = (float)msec / cl->moveCommand.input.msec;

    return Clampf(val, 0, 1);
}


//
//=============================================================================
//
//	KEY INPUT SAMPLER FUNCTIONS.
//
//=============================================================================
//
static void IN_KLookDown(void) { CLG_KeyDown(&in_klook); }
static void IN_KLookUp(void) { CLG_KeyUp(&in_klook); }
static void IN_UpDown(void) { CLG_KeyDown(&in_up); }
static void IN_UpUp(void) { CLG_KeyUp(&in_up); }
static void IN_DownDown(void) { CLG_KeyDown(&in_down); }
static void IN_DownUp(void) { CLG_KeyUp(&in_down); }
static void IN_LeftDown(void) { CLG_KeyDown(&in_left); }
static void IN_LeftUp(void) { CLG_KeyUp(&in_left); }
static void IN_RightDown(void) { CLG_KeyDown(&in_right); }
static void IN_RightUp(void) { CLG_KeyUp(&in_right); }
static void IN_ForwardDown(void) { CLG_KeyDown(&in_forward); }
static void IN_ForwardUp(void) { CLG_KeyUp(&in_forward); }
static void IN_BackDown(void) { CLG_KeyDown(&in_back); }
static void IN_BackUp(void) { CLG_KeyUp(&in_back); }
static void IN_LookupDown(void) { CLG_KeyDown(&in_lookup); }
static void IN_LookupUp(void) { CLG_KeyUp(&in_lookup); }
static void IN_LookdownDown(void) { CLG_KeyDown(&in_lookdown); }
static void IN_LookdownUp(void) { CLG_KeyUp(&in_lookdown); }
static void IN_MoveleftDown(void) { CLG_KeyDown(&in_moveleft); }
static void IN_MoveleftUp(void) { CLG_KeyUp(&in_moveleft); }
static void IN_MoverightDown(void) { CLG_KeyDown(&in_moveright); }
static void IN_MoverightUp(void) { CLG_KeyUp(&in_moveright); }
static void IN_SpeedDown(void) { CLG_KeyDown(&in_speed); }
static void IN_SpeedUp(void) { CLG_KeyUp(&in_speed); }
static void IN_StrafeDown(void) { CLG_KeyDown(&in_strafe); }
static void IN_StrafeUp(void) { CLG_KeyUp(&in_strafe); }

/**
*   +primaryfire Button down/up.
**/
static void IN_PrimaryFireDown(void)
{
    CLG_KeyDown(&in_primary_fire);

    if (cl_instantpacket->integer && clgi.GetClienState() == ClientConnectionState::Active) {// && cls->netchan) {
        cl->sendPacketNow = true;
    }
}
static void IN_SecondaryFireDown(void)
{
    CLG_KeyDown(&in_secondary_fire);

    if (cl_instantpacket->integer && clgi.GetClienState() == ClientConnectionState::Active) {// && cls->netchan) {
        cl->sendPacketNow = true;
    }
}

/**
*   +secondaryfire Button down/up.
**/
static void IN_PrimaryFireUp(void)
{
    CLG_KeyUp(&in_primary_fire);
}
static void IN_SecondaryFireUp(void)
{
    CLG_KeyUp(&in_secondary_fire);
}

/**
*   +reload Button down/up.
**/
static void IN_ReloadDown(void)
{
    CLG_KeyDown(&in_reload);

    if (cl_instantpacket->integer && clgi.GetClienState() == ClientConnectionState::Active) {// && cls.netChannel) {
        cl->sendPacketNow = true;
    }
}
static void IN_ReloadUp(void)
{
    CLG_KeyUp(&in_reload);
}

/**
*   +use Button down/up.
**/
static void IN_UseDown(void)
{
    CLG_KeyDown(&in_use);

    if (cl_instantpacket->integer && clgi.GetClienState() == ClientConnectionState::Active) {// && cls.netChannel) {
        cl->sendPacketNow = true;
    }
}
static void IN_UseUp(void)
{
    CLG_KeyUp(&in_use);
}
static void IN_Impulse(void)
{
    in_impulse = atoi(clgi.Cmd_Argv(1));
}
static void IN_CenterView(void)
{
    cl->viewAngles.x = -cl->frame.playerState.pmove.deltaAngles[0];
}
static void IN_MLookDown(void)
{
    in_mlooking = true;
}
static void IN_MLookUp(void)
{
    in_mlooking = false;
}



//
//=============================================================================
//
//	SAMPLED INPUT PROCESSING
//
//=============================================================================
//
//
//===============
// CLG_MouseMove
// 
// Handles the mouse move based input adjustment.
//================
//
void CLG_MouseMove() {
    int deltaX, deltaY;
    float motionX, motionY;
    float speed;

    if (!clgi.Mouse_GetMotion(&deltaX, &deltaY)) {
        return;
    }
    if (clgi.Key_GetDest() & (KEY_MENU | KEY_CONSOLE)) {
        return;
    }

    if (m_filter->integer) {
        motionX = (deltaX + inputState.oldDeltaX) * 0.5f;
        motionY = (deltaY + inputState.oldDeltaY) * 0.5f;
    }
    else {
        motionX = deltaX;
        motionY = deltaY;
    }

    inputState.oldDeltaX = deltaX;
    inputState.oldDeltaY = deltaY;

    if (!motionX && !motionY) {
        return;
    }

    clgi.Cvar_ClampValue(m_accel, 0, 1);

    speed = std::sqrtf(motionX * motionX + motionY * motionY);
    speed = sensitivity->value + speed * m_accel->value;

    motionX *= speed;
    motionY *= speed;

    if (m_autosens->integer) {
        motionX *= cl->fov_x * cl->autosens_x;
        motionY *= cl->fov_y * cl->autosens_y;
    }

    // Add mouse X/Y movement
    cl->viewAngles[vec3_t::Yaw] -= m_yaw->value * motionX;
    cl->viewAngles[vec3_t::Pitch] += m_pitch->value * motionY * (m_invert->integer ? -1.f : 1.f);
}

//
//===============
// CLG_AdjustAngles
// 
// Moves the local angle positions
//================
//
void CLG_AdjustAngles(int msec)
{
    float speed;

    if (in_speed.state & BUTTON_STATE_HELD)
        speed = msec * cl_anglespeedkey->value * 0.001f;
    else
        speed = msec * 0.001f;

    cl->viewAngles[vec3_t::Yaw] -= speed * cl_yawspeed->value * CLG_KeyState(&in_right);
    cl->viewAngles[vec3_t::Yaw] += speed * cl_yawspeed->value * CLG_KeyState(&in_left);
    cl->viewAngles[vec3_t::Pitch] -= speed * cl_pitchspeed->value * CLG_KeyState(&in_lookup);
    cl->viewAngles[vec3_t::Pitch] += speed * cl_pitchspeed->value * CLG_KeyState(&in_lookdown);
}

//
//===============
// CLG_BaseMove
// 
// Build and return the intended movement vector
//================
//
vec3_t CLG_BaseMove(const vec3_t& inMove)
{
    vec3_t outMove = inMove;

    if (in_strafe.state & 1) {
        outMove[1] += cl_sidespeed->value * CLG_KeyState(&in_right);
        outMove[1] -= cl_sidespeed->value * CLG_KeyState(&in_left);
    }

    outMove[1] += cl_sidespeed->value * CLG_KeyState(&in_moveright);
    outMove[1] -= cl_sidespeed->value * CLG_KeyState(&in_moveleft);

    outMove[2] += cl_upspeed->value * CLG_KeyState(&in_up);
    outMove[2] -= cl_upspeed->value * CLG_KeyState(&in_down);

    if (!(in_klook.state & 1)) {
        outMove[0] += cl_forwardspeed->value * CLG_KeyState(&in_forward);
        outMove[0] -= cl_forwardspeed->value * CLG_KeyState(&in_back);
    }

    return outMove;
}

//
//===============
// CLG_ClampSpeed
// 
// Returns the clamped movement speeds.
//================
//
vec3_t CLG_ClampSpeed(const vec3_t& inMove)
{
    vec3_t outMove = inMove;

    // If movement ever starts feeling wrong after all, then move this code out of it.
#if 1
    float speed = cl_forwardspeed->value; // TODO: FIX PM_ //pmoveParams->maxspeed;

    outMove[0] = Clampf(outMove[0], -speed, speed);
    outMove[1] = Clampf(outMove[1], -speed, speed);
    outMove[2] = Clampf(outMove[2], -speed, speed);
#endif

    return outMove;
}

//
//===============
// CLG_ClampPitch
// 
// Ensures the Pitch is clamped to prevent camera issues.
//================
//
void CLG_ClampPitch(void)
{
    float pitch;

    pitch = cl->frame.playerState.pmove.deltaAngles[vec3_t::Pitch];
    if (pitch > 180)
        pitch -= 360;

    if (cl->viewAngles[vec3_t::Pitch] + pitch < -360)
        cl->viewAngles[vec3_t::Pitch] += 360; // wrapped
    if (cl->viewAngles[vec3_t::Pitch] + pitch > 360)
        cl->viewAngles[vec3_t::Pitch] -= 360; // wrapped

    if (cl->viewAngles[vec3_t::Pitch] + pitch > 89)
        cl->viewAngles[vec3_t::Pitch] = 89 - pitch;
    if (cl->viewAngles[vec3_t::Pitch] + pitch < -89)
        cl->viewAngles[vec3_t::Pitch] = -89 - pitch;
}

//
//===============
// CLG_RegisterInput
// 
// Register input messages and binds them to a callback function.
// Bindings are set in the config files, and/or the options menu.
// 
// For more information, it still works like in q2pro.
//===============
//
void CLG_RegisterInput(void)
{
    // Register Input Key bind Commands.
    clgi.Cmd_AddCommand("centerview", IN_CenterView);
    clgi.Cmd_AddCommand("+moveup", IN_UpDown);
    clgi.Cmd_AddCommand("-moveup", IN_UpUp);
    clgi.Cmd_AddCommand("+movedown", IN_DownDown);
    clgi.Cmd_AddCommand("-movedown", IN_DownUp);
    clgi.Cmd_AddCommand("+left", IN_LeftDown);
    clgi.Cmd_AddCommand("-left", IN_LeftUp);
    clgi.Cmd_AddCommand("+right", IN_RightDown);
    clgi.Cmd_AddCommand("-right", IN_RightUp);
    clgi.Cmd_AddCommand("+forward", IN_ForwardDown);
    clgi.Cmd_AddCommand("-forward", IN_ForwardUp);
    clgi.Cmd_AddCommand("+back", IN_BackDown);
    clgi.Cmd_AddCommand("-back", IN_BackUp);
    clgi.Cmd_AddCommand("+lookup", IN_LookupDown);
    clgi.Cmd_AddCommand("-lookup", IN_LookupUp);
    clgi.Cmd_AddCommand("+lookdown", IN_LookdownDown);
    clgi.Cmd_AddCommand("-lookdown", IN_LookdownUp);
    clgi.Cmd_AddCommand("+strafe", IN_StrafeDown);
    clgi.Cmd_AddCommand("-strafe", IN_StrafeUp);
    clgi.Cmd_AddCommand("+moveleft", IN_MoveleftDown);
    clgi.Cmd_AddCommand("-moveleft", IN_MoveleftUp);
    clgi.Cmd_AddCommand("+moveright", IN_MoverightDown);
    clgi.Cmd_AddCommand("-moveright", IN_MoverightUp);
    clgi.Cmd_AddCommand("+speed", IN_SpeedDown);
    clgi.Cmd_AddCommand("-speed", IN_SpeedUp);
    clgi.Cmd_AddCommand("+primaryfire", IN_PrimaryFireDown);
    clgi.Cmd_AddCommand("-primaryfire", IN_PrimaryFireUp);
    clgi.Cmd_AddCommand("+secondaryfire", IN_SecondaryFireDown);
    clgi.Cmd_AddCommand("-secondaryfire", IN_SecondaryFireUp);
    clgi.Cmd_AddCommand("+reload", IN_ReloadDown);
    clgi.Cmd_AddCommand("-reload", IN_ReloadUp);
    clgi.Cmd_AddCommand("+use", IN_UseDown);
    clgi.Cmd_AddCommand("-use", IN_UseUp);
    clgi.Cmd_AddCommand("impulse", IN_Impulse);

    // Create Cvars.
    cl_upspeed = clgi.Cvar_Get("cl_upspeed", "300", 0);
    cl_forwardspeed = clgi.Cvar_Get("cl_forwardspeed", "300", 0);
    cl_sidespeed = clgi.Cvar_Get("cl_sidespeed", "300", 0);
    cl_yawspeed = clgi.Cvar_Get("cl_yawspeed", "0.140", 0);
    cl_pitchspeed = clgi.Cvar_Get("cl_pitchspeed", "0.150", CVAR_CHEAT);
    cl_anglespeedkey = clgi.Cvar_Get("cl_anglespeedkey", "1.5", CVAR_CHEAT);
    cl_run = clgi.Cvar_Get("cl_run", "1", CVAR_ARCHIVE);

    // Fetch CVars.
    cl_instantpacket = clgi.Cvar_Get("cl_instantpacket", "0", 0);

    sensitivity = clgi.Cvar_Get("sensitivity", "0", 0);

    m_autosens = clgi.Cvar_Get("m_autosens", "0", 0);
    m_pitch = clgi.Cvar_Get("m_pitch", "", 0);
    m_invert = clgi.Cvar_Get("m_invert", "", 0);
    m_yaw = clgi.Cvar_Get("m_yaw", "", 0);
    m_forward = clgi.Cvar_Get("m_forward", "", 0);
    m_side = clgi.Cvar_Get("m_side", "", 0);
    m_filter = clgi.Cvar_Get("m_filter", "", 0);
    m_accel = clgi.Cvar_Get("m_accel", "", 0);
}


//
//=============================================================================
//
//	INPUT MODULE ENTRY FUNCTIONS.
//
//=============================================================================
//