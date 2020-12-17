#include "cbase.h"
#include <stdint.h>
#include <imaterialsystem.h>

// Globals
int VRMod_Started = 0;
ITexture * RenderTarget_VRMod = NULL;
float g_horizontalFOVLeft = 0;
float g_horizontalFOVRight = 0;
float g_aspectRatioLeft = 0;
float g_aspectRatioRight = 0;



// Functions we want to share with other files (mainly viewrender.cpp for now)

void VRMOD_SubmitSharedTexture();

void VRMOD_UpdatePosesAndActions();

void VRMOD_UtilSetOrigin(Vector pos);

void VRMOD_UtilHandleTracking();

QAngle VRMOD_GetViewAngle();

Vector VRMOD_GetViewOriginLeft();

Vector VRMOD_GetViewOriginRight();

int VRMOD_GetRecWidth();

int VRMOD_GetRecHeight();