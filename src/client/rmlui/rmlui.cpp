// LICENSE HERE.

//
// client/rmlui/rmlui.cpp
//
//
// RmlUI Wrapper API that is client game friendly if needed.
//

#include "librmlui.h"


// Client includes.
#include "../client.h"
#include "client/sound/vorbis.h"
#include "client/gamemodule.h"

// RmlUI includes.
#include "rmlui.h"

//////////////////////
static Rml::Context* context = NULL;
static Rml::ElementDocument* document = NULL;

static RmlUiRenderInterface rmlRenderInterface;
static RmlUISystemInterface rmlSystemInterface;
static RmlUIFileInterface rmlFileInterface;


//
//=============================================================================
// RMLUI_Init
// 
// Initializes the RMLUI library.
//=============================================================================
//
void RMLUI_Init(void) {

	// Begin by installing the custom interfaces.
	Rml::SetRenderInterface(&rmlRenderInterface);
	Rml::SetSystemInterface(&rmlSystemInterface);
	Rml::SetFileInterface(&rmlFileInterface);

	// Now we can initialize RmlUi.
	Rml::Initialise();

	// Load FontFaces.
	struct FontFace {
		Rml::String filename;
		bool fallback_face;
	};
	FontFace font_faces[] = {
		{ "LatoLatin-Regular.ttf",    false },
		{ "LatoLatin-Italic.ttf",     false },
		{ "LatoLatin-Bold.ttf",       false },
		{ "LatoLatin-BoldItalic.ttf", false },
		{ "NotoEmoji-Regular.ttf",    true  },
	};

	for (const FontFace& face : font_faces)
	{
		Rml::LoadFontFace("fonts/" + face.filename, face.fallback_face);
	}

	// Create a context next.
	context = Rml::CreateContext("main", Rml::Vector2i( 1280, 720 ));
	if (!context)
	{
		Rml::Shutdown();
		return;
	}

	// If you want to use the debugger, initialize it now.
	Rml::Debugger::Initialise(context);

	// Now we are ready to load our document.
	document = context->LoadDocument("fonts/demo.rml");
	if (!document)
	{
		Rml::Shutdown();
		return;
	}

	document->Show();
}

//
//=============================================================================
// RMLUI_ProcessKeyDown
// 
// True if the event was not consumed, false if it was.
//=============================================================================
//
bool RMLUI_ProcessKeyDown(SDL_Keycode key) {
	if (!context)
		return qfalse;

	// Enable/Disable RMLUI Debugger.
	if (key == SDLK_F8)
	{
	    Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
		return false; // Consumed.
	}

	return context->ProcessKeyDown(rmlSystemInterface.TranslateKey(key), RmlUISystemInterface::GetKeyModifiers());
}

//
//=============================================================================
// RMLUI_ProcessKeyUp
// 
// True if the event was not consumed, false if it was.
//=============================================================================
//
bool RMLUI_ProcessKeyUp(SDL_Keycode key) {
	if (!context)
		return qfalse;

	// Enable/Disable RMLUI Debugger.
	//if (key == SDLK_F8)
	//{
	//	Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
	//	return false; // Consumed.
	//}

	return context->ProcessKeyUp(rmlSystemInterface.TranslateKey(key), RmlUISystemInterface::GetKeyModifiers());
}

//
//=============================================================================
// RMLUI_ProcessTextInput
// 
// 
//=============================================================================
//
bool RMLUI_ProcessTextInput(const char* text) {
	if (!context)
		return qfalse;

	return context->ProcessTextInput(Rml::String(text));
}

//
//=============================================================================
// RMLUI_ProcessMouseMove
// 
// True if the mouse is not interacting with any elements in the context 
// (see 'IsMouseInteracting'), otherwise false.
//=============================================================================
//
bool RMLUI_ProcessMouseMove(int x, int y) {
	if (!context)
		return qfalse;

	return context->ProcessMouseMove(x, y, RmlUISystemInterface::GetKeyModifiers());
}

//
//=============================================================================
// RMLUI_ProcessMouseWheel
// 
// True if the event was not consumed (ie, was prevented from propagating by an element), false if it was.
//=============================================================================
//
bool RMLUI_ProcessMouseWheel(float delta) {
	if (!context)
		return qfalse;

	return context->ProcessMouseWheel(delta, RmlUISystemInterface::GetKeyModifiers());
}

//
//=============================================================================
// RMLUI_ProcessMouseButtonUp
// 
// True if the mouse is not interacting with any elements in the context 
// (see 'IsMouseInteracting'), otherwise false.
//=============================================================================
//
bool RMLUI_ProcessMouseButtonUp(int button) {
	if (!context)
		return qfalse;

	return context->ProcessMouseButtonUp(rmlSystemInterface.TranslateMouseButton(button), RmlUISystemInterface::GetKeyModifiers());
}

//
//=============================================================================
// RMLUI_ProcessMouseButtonDown
// 
// True if the mouse is not interacting with any elements in the context 
// (see 'IsMouseInteracting'), otherwise false.
//=============================================================================
//
bool RMLUI_ProcessMouseButtonDown(int button) {
	if (!context)
		return qfalse;

	return context->ProcessMouseButtonDown(rmlSystemInterface.TranslateMouseButton(button), RmlUISystemInterface::GetKeyModifiers());
}

//
//=============================================================================
// RMLUI_UpdateFrame
// 
// Updates the RMLUI Context if initialized and a menu is active.
//=============================================================================
//
void RMLUI_UpdateFrame(void) {
	if (!context)
		return;
	if (!(Key_GetDest() & KEY_MENU)) {
		return;
	}
	context->Update();
}

//
//=============================================================================
// RMLUI_RenderFrame
// 
// Renders the RMLUI Context if initialized and a menu is active.
//=============================================================================
//
void RMLUI_RenderFrame(void) {
	if (!context)
		return;
	if (!(Key_GetDest() & KEY_MENU)) {
		return;
	}

	context->Render();
}

//
//=============================================================================
// RMLUI_Shutdown
// 
// Shutdowns RMLUI library.
//=============================================================================
//
void RMLUI_Shutdown(void) {
	// Shutting down RmlUi releases all its resources, including elements, documents, and contexts.
	Rml::Shutdown();
}