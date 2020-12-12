#include "cbase.h"
//#include <stdio.h>
#include <stdint.h>
//#include <Windows.h>
#include <imaterialsystem.h>

// Globals
int VRMod_Started = 0;
ITexture * RenderTarget_VRMod = NULL;
float g_horizontalFOVLeft = 0;
float g_horizontalFOVRight = 0;
float g_aspectRatioLeft = 0;
float g_aspectRatioRight = 0;
//uint32_t recommendedWidth = 640;
//uint32_t recommendedHeight = 720;



// Functions we want to share with other files (mainly viewrender.cpp for now)

void VRMOD_SubmitSharedTexture();

void VRMOD_UpdatePosesAndActions();

int VRMOD_GetRecWidth();

int VRMOD_GetRecHeight();