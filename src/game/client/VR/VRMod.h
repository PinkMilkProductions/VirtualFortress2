#include "cbase.h"
#include <imaterialsystem.h>

// Globals
int VRMod_Started = 0;
ITexture * RenderTarget_VRMod = NULL;



// Functions we want to share with other files (mainly viewrender.cpp for now)

void VRMOD_SubmitSharedTexture();

void VRMOD_UpdatePosesAndActions();