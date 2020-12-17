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

// Globals for Headtracking
float VR_scale = 1;															// The VR scale used to match player real life height with the character height.
Vector VR_origin;															// The absolute world position of our Tracked VR origin point.
Vector VR_hmd_pos_abs;														// Absolute position of the HMD
QAngle VR_hmd_ang_abs;														// The angle the HMD makes in the global coordinate system
float ipd;																	// The Inter Pupilary Distance
float eyez;																	// Eyes local height with respect to the hmd origin



// Functions we want to share with other files (mainly viewrender.cpp for now)

void VRMOD_SubmitSharedTexture();

void VRMOD_UpdatePosesAndActions();

void VRMOD_UtilHandleTracking();

int VRMOD_GetRecWidth();

int VRMOD_GetRecHeight();