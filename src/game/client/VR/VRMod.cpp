#include "cbase.h"
#include <stdio.h>
#include <d3d9.h>
#include "D3D11.h"
#include <Windows.h>
#include <openvr.h>
#include <MinHook.h>
#include <convar.h>
#include <synchapi.h>
#include <processthreadsapi.h>
//#include <isourcevirtualreality.h>
#include <imaterialsystem.h>
//#include <cdll_client_int.h>
#include <thread>
#include <vector>
//#include "c_baseentity.h"		// Recently added for headtracking calculations


#if defined( CLIENT_DLL )			// Client specific.
	#include "c_tf_player.h"		// Recently added for headtracking calculations
	#include <baseviewmodel_shared.h>
#else
	#include "tf_player.h"
#endif


#include <VRMod.h>


#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3d9.lib")



/*
//*************************************************************************

// NOTES:				- turns out view.cpp has void CViewRender::Render( vrect_t *rect ) which calculates the view and render flags before viewrender happens and i do my VR stuff.
						  I should move my VR rendering code from viewrender to there probably.


//  Current issues:		- Handtracked shooting works currently but doesn't register as a valid hit on the server.
						  I will have to adjust the code in, among other things, void CTFWeaponBaseGun::FireBullet( CTFPlayer *pPlayer )
						  to work correctly in both the client and server context. Thinking about doing this when i'm fixing multiplayer.
						  This also means rockets, grenades and stickies currently don't want to shoot after they spawn because that's also a server thing.
						  Medigun also doesn't aim correctly atm but that shouldn't be a server thing.



						-  Frames sent to the HMD work perfectly fine on 720p but when we try to use the recommended HMD resolutions,
						   the HMD frames become black with part of a white rectangle off-bounds.
						THINGS I TRIED: 
							1. This issue is the same wether we Use Virtual Desktop or Oculus Link
							2. I tried overriding the framebuffer resolution successfully (via g_pMaterialSystem->SetRenderTargetFrameBufferSizeOverrides(recommendedWidth, recommendedHeight)
							and same for the "materials" variable), frames still didn't work.
							3. I added debugmessages. RenderTargetSize is indeed set correctly to the right resolution, frames still didn't work.
							4. If i try calling g_pMaterialSystem->EndRenderTargetAllocation(), frames are completely black, even for 720p
							5. I tried a boatload of flags for CreateNamedRenderTargetTextureEx. None of them fixed the problem.
							Some are interesting though. Disabling the depth buffer for example seems to submit frames to the HMD even if not 720p,
							but resolution still seems to be 720p and of course there's no depth anymore making you see through walls and stuff.

						POSSIBLE SOLUTIONS:
							Look inside void CViewRender::Render( vrect_t *rect ) in view.cpp
							it also contains the interesting function void CViewRender::SetUpViews()
							The following functions from pRenderContext may also be very interesting for solving this problem:
								// Sets/gets the viewport
							virtual void				Viewport( int x, int y, int width, int height ) = 0;
							virtual void				GetViewport( int& x, int& y, int& width, int& height ) const = 0;
								// Gets the window size
							virtual void GetWindowSize( int &width, int &height ) const = 0;
								// Sets the override sizes for all render target size tests. These replace the frame buffer size.

							// Set them when you are rendering primarily to something larger than the frame buffer (as in VR mode).
							virtual void				SetRenderTargetFrameBufferSizeOverrides( int nWidth, int nHeight ) = 0;

							// Returns the (possibly overridden) framebuffer size for render target sizing.
							virtual void				GetRenderTargetFrameBufferDimensions( int & nWidth, int & nHeight ) = 0;


						abstract_class IClientRenderable van iclientrenderable.h can also be very interesting. Especially the following functions:
							virtual bool					UsesPowerOfTwoFrameBufferTexture() = 0;
							virtual bool					UsesFullFrameBufferTexture() = 0;







						- Trying to load Gravelpit crashes the game for some reason. Error is in tf_mapinfomenu.cpp






	Fixed issues:		- A whole lot of compile issues. Started by trying to modify baseplayer_shared.cpp
							Weapon_shootposition.
							Has to do something with the server project. I was adviced to fix it by adding VRMOD.cpp and .h to the server project so the function is defined there
							but now it's complaining about d3d11.h

						FIX: Making sure the External Dependencies d3d11.h for both the client and server projects are referencing the same file in the same directory.
						I had to delete the Windows 7.1A include path in the server project properties to do that.
	
	
	
	
						- When we compile the game for Release and then launch it via steam, the Source SDK 2013 Multiplayer
						  resets it's openvr.dll to it's previous way lighter version that gives us errors.

						  FIX: Launching the mod directly from hl2.exe via a .bat file ( as done in Release 2) circumvents this issue.
	
	
	
	
						- We get up to ShareTextureFinish. That one crashes the game and gives us: OpenSharedResource failedException thrown: read access violation.
						  **res** was nullptr.

						FIX:
						  - first you call ShareTextureBegin()
						  - then you CREATE a rendertarget texture (via CreateRenderTargetTexture(arguments) from IMaterialSystem class from imaterialsystem.h)
						  - then you call ShareTextureFinish() immediately after

						  Link that pointed us at a working solution:
						  https://developer.valvesoftware.com/wiki/Adding_a_Dynamic_Scope

						  After that, CreateTextureHook() got called A LOT of times, we still didn't get it working.
						  To fix that all we had to do was call g_pMaterialSystem->BeginRenderTargetAllocation(); right before creating the new RenderTarget.
	
	


						- Code won't compile because of errors with CreateThread and WaitForSingleObject in VRMOD_Sharetexturebegin(). I'm sure this has to do with the Windows Kit include files. Maybe it'll go away if we include more Windows Kit directories
						  Maybe it's a problem unique to Windows 10

						  POSSIBLE SOLUTIONS:  - add #include <Windows.h> to cbase.h and solve the compile errors we get after that.
						  - nuttige link:   https://social.msdn.microsoft.com/Forums/en-US/400c5cce-d6ed-4157-908e-41d5beb1ce13/link-error-2019-missing-definitions-from-windowsh?forum=vcgeneral

						FIX: Turns out there's a file named "protected_things.h". It redefined CreateThread to CreateThread__USE_VCR_MODE and WaitForSingleObject to WaitForSingleObject__USE_VCR_MODE
							 because some parameter was defined or something that enables extra valve safety precautions. Commenting these 2 redefinitions away fixed the problem.
							 I hope this won't come back to bite me in the ass later.
	
	
						- ConCommand vrmod_init("vrmod_init", VRMOD_Init, "Starts VRMod and SteamVR.") Won't work for some fucking reason even though it fucking should

						FIX: ConCommand only works on Void functions!

						- Latest error i keep getting is related to the code being Read only for some fucking reason. This is the same on my whole fucking harddrive and it keeps fucking resetting to Read only everytime i change it
						  FUCK WHATEVER IS RESPONSIBLE FOR THIS BULLSHIT. MIGHT BE FUCKING GIT THAT'S DOING THIS SHIT
						  UPDATE: turns out the folders aren't actually Read-only, they just seem like it. Maybe the shitty compiler doesn't know that though.

						FIX: Turns out there wasn't any read-only problem. The compile errors were caused by the change in directory structure due to moving and cloning the project with git.
							 Fixed this by deleting the project files and regenerating them with createtfmod.bat
//*************************************************************************
*/


//*************************************************************************
//  Globals
//*************************************************************************

#define MAX_STR_LEN 256
#define MAX_ACTIONS 64
#define MAX_ACTIONSETS 16

//openvr related
typedef struct {
    vr::VRActionHandle_t handle;
    char fullname[MAX_STR_LEN];
    char type[MAX_STR_LEN];
    char* name;
}action;

typedef struct {
    vr::VRActionSetHandle_t handle;
    char name[MAX_STR_LEN];
}actionSet;

vr::IVRSystem*          g_pSystem = NULL;
vr::IVRInput*           g_pInput = NULL;
vr::TrackedDevicePose_t g_poses[vr::k_unMaxTrackedDeviceCount];
actionSet               g_actionSets[MAX_ACTIONSETS];
int                     g_actionSetCount = 0;
vr::VRActiveActionSet_t g_activeActionSets[MAX_ACTIONSETS];
int                     g_activeActionSetCount = 0;
action                  g_actions[MAX_ACTIONS];
int                     g_actionCount = 0;

//directx
typedef HRESULT(APIENTRY* CreateTexture) (IDirect3DDevice9*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*);
CreateTexture           g_CreateTextureOriginal = NULL;
ID3D11Device*           g_d3d11Device = NULL;
ID3D11Texture2D*        g_d3d11Texture = NULL;
HANDLE                  g_sharedTexture = NULL;
DWORD_PTR               g_CreateTextureAddr = NULL;
IDirect3DDevice9*       g_d3d9Device = NULL;

//other
float                   g_horizontalOffsetLeft = 0;
float                   g_horizontalOffsetRight = 0;
float                   g_verticalOffsetLeft = 0;
float                   g_verticalOffsetRight = 0;
uint32_t				recommendedWidth = 960;
uint32_t				recommendedHeight = 1080;

// Extra Virtual Fortress 2 globals
int Virtual_Fortress_2_version = 1;

// Globals for VRMOD_GetViewParameters()
Vector eyeToHeadTransformPosLeft;
Vector eyeToHeadTransformPosRight;

// Globals for VRMOD_GetPoses()	
struct TrackedDevicePoseStruct {
	std::string TrackedDeviceName;
	Vector TrackedDevicePos;
	Vector TrackedDeviceVel;
	QAngle TrackedDeviceAng;
	QAngle TrackedDeviceAngVel;
};

TrackedDevicePoseStruct TrackedDevicesPoses[vr::k_unMaxTrackedDeviceCount];

// Globals for VRMOD_GetTrackedDeviceNames()
std::vector<std::string> TrackedDevicesNames(6);     // Vector (sort of dynamic array) with initial space for 6 tracked devices: the HMD, left controller, right controller, left foot tracker, right foot tracker, hip tracker

// Globals for VRMOD_GetActions()
struct ActionsBoolStruct {
	std::string ActionName;
	bool BoolData;
};

ActionsBoolStruct ActionsBool[MAX_ACTIONS];

struct ActionsVector1Struct {
	std::string ActionName;
	float FloatData;
};

ActionsVector1Struct ActionsVector1[MAX_ACTIONS];

struct ActionsVector2Struct {
	std::string ActionName;
	float FloatData1;
	float FloatData2;
};

ActionsVector2Struct ActionsVector2[MAX_ACTIONS];

struct ActionsSkeletonStruct {
	std::string ActionName;
	float FloatFingerCurlData1;
	float FloatFingerCurlData2;
	float FloatFingerCurlData3;
	float FloatFingerCurlData4;
	float FloatFingerCurlData5;
};

ActionsSkeletonStruct ActionsSkeleton[MAX_ACTIONS];

// Globals for Headtracking
#if defined( CLIENT_DLL )			// Client specific.
	C_TFPlayer *pPlayer = NULL;													// The player character
#else								// Server specific
	CTFPlayerShared *pPlayer = NULL;
#endif
int LocalPlayerIndex = 0;
//CTFPlayer *pPlayer = NULL;
float VR_scale = 52.49;				// 1 meter = 52.49 Hammer Units		// The VR scale used if we want 1:1 based on map units
//float VR_scale = 39.37012415030996;											// Alternative scale if we want to base it on 1:1 realistic character units
Vector VR_origin = Vector(0, 0, 0);											// The absolute world position of our Tracked VR origin point.

Vector VR_hmd_pos_abs;														// Absolute position of the HMD
QAngle VR_hmd_ang_abs;														// The angle the HMD makes in the global coordinate system
Vector VR_controller_left_pos_abs;											// left controller pose is in TrackedDevicesPoses[6]
QAngle VR_controller_left_ang_abs;
Vector VR_controller_right_pos_abs;											// right controller pose is in TrackedDevicesPoses[7]
QAngle VR_controller_right_ang_abs;

float ipd;																	// The Inter Pupilary Distance
float eyez;																	// Eyes local height with respect to the hmd origin
std::string ActiveActionSetNames[MAX_ACTIONSETS];							// Needed for setting the active action sets

Vector VR_hmd_forward;
Vector VR_hmd_right;
Vector VR_hmd_up;

Vector VR_controller_left_forward;
Vector VR_controller_left_right;
Vector VR_controller_left_up;

Vector VR_controller_right_forward;
Vector VR_controller_right_right;
Vector VR_controller_right_up;

Vector Player_forward;
Vector Player_right;
Vector Player_up;

QAngle PlayerEyeAngles;

Vector VR_playspace_forward;
Vector VR_playspace_right;
Vector VR_playspace_up;
QAngle ZeroAngle = QAngle(0, 0, 0);

QAngle VR_Player_Spawn_HMD_Angles = QAngle(0, 0, 0);						// Used for making the player face the right direction when spawning

bool switch_up_just_switched = false;										// For Action controlled weapon switching
bool switch_down_just_switched = false;
int switch_up_counter = 0;
int switch_down_counter = 0;
int Trigger_active_counter = 0;

int GestureSelection = 0;
int GestureMenuIndex = 0;													// For gesture menu selection

Vector GestureOrigin = Vector(0, 0, 0);
std::string GestureQuadMaterials[6] = {"", "", "", "", "", ""};
Vector GestureForward, GestureRight, GestureUp;
float GestureMinSelectionDistance = 8.0f;


Vector WeaponOffset = Vector(0, 0, 0);										// For viewmodel and shooting positions stuff

// Convars that the players can set themselves if they want to
ConVar VRMOD_ipd_scale("VRMOD_ipd_scale", "52.49", 0, "sets the scale used exclusively for IPD distance. Valid inputs between 1 and 100", true, 1.0f, true, 100.0f);






//*************************************************************************
//  CreateTexture hook																			// converted properly for Virtual Fortress 2 now.
//*************************************************************************
HRESULT APIENTRY CreateTextureHook(IDirect3DDevice9* pDevice, UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9** tex, HANDLE* shared_handle) {
		if (g_sharedTexture == NULL) {
			shared_handle = &g_sharedTexture;
			pool = D3DPOOL_DEFAULT;
			g_d3d9Device = pDevice;
		}
		return g_CreateTextureOriginal(pDevice, w, h, levels, usage, format, pool, tex, shared_handle);
};

//*************************************************************************
//    FindCreateTexture thread																	// converted properly for Virtual Fortress 2 now.
//*************************************************************************
DWORD WINAPI FindCreateTexture(LPVOID lParam) {
    IDirect3D9* dx = Direct3DCreate9(D3D_SDK_VERSION);
    if (dx == NULL) {
        return 1;
    }

    HWND window = CreateWindowA("BUTTON", " ", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (window == NULL) {
        dx->Release();
        return 2;
    }

    IDirect3DDevice9* d3d9Device = NULL;

    D3DPRESENT_PARAMETERS params;
    ZeroMemory(&params, sizeof(params));
    params.Windowed = TRUE;
    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.hDeviceWindow = window;
    params.BackBufferFormat = D3DFMT_UNKNOWN;

    //calling CreateDevice on the main thread seems to start causing random lua errors
    if (dx->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params, &d3d9Device) != D3D_OK) {
        dx->Release();
        DestroyWindow(window);
        return 3;
    }

    g_CreateTextureAddr = ((DWORD_PTR*)(((DWORD_PTR*)d3d9Device)[0]))[23];

    d3d9Device->Release();
    dx->Release();
    DestroyWindow(window);

    return 0;
}

//*************************************************************************
//    Lua function: VRMOD_GetVersion()															// should be converted properly for Virtual Fortress 2 now
//    Returns: number
//*************************************************************************
void VRMOD_GetVersion() {
	Msg("Current Virtual Fortress 2 version is : %i", Virtual_Fortress_2_version);
    return;
}
#if defined( CLIENT_DLL )			// Client specific.
ConCommand vrmod_getversion("vrmod_getversion", VRMOD_GetVersion, "Returns the current version of the Virtual Fortress 2 mod.");
#endif
//*************************************************************************
//    VRMOD_Init():		Initialize SteamVR and set some important globals						// converted properly for Virtual Fortress 2 now.
//*************************************************************************
 void VRMOD_Init() {
    vr::HmdError error = vr::VRInitError_None;

    g_pSystem = vr::VR_Init(&error, vr::VRApplication_Scene);
    if (error != vr::VRInitError_None) {
        Warning("VR_Init failed");
    }

    if (!vr::VRCompositor()) {
        Warning("VRCompositor failed");
    }

    vr::HmdMatrix44_t proj = g_pSystem->GetProjectionMatrix(vr::Hmd_Eye::Eye_Left, 1, 10);
    float xscale = proj.m[0][0];
    float xoffset = proj.m[0][2];
    float yscale = proj.m[1][1];
    float yoffset = proj.m[1][2];
    float tan_px = fabsf((1.0f - xoffset) / xscale);
    float tan_nx = fabsf((-1.0f - xoffset) / xscale);
    float tan_py = fabsf((1.0f - yoffset) / yscale);
    float tan_ny = fabsf((-1.0f - yoffset) / yscale);
    float w = tan_px + tan_nx;
    float h = tan_py + tan_ny;
    g_horizontalFOVLeft = atan(w / 2.0f) * 180 / 3.141592654 * 2;
    //g_verticalFOV = atan(h / 2.0f) * 180 / 3.141592654 * 2;
    g_aspectRatioLeft = w / h;
    g_horizontalOffsetLeft = xoffset;
    g_verticalOffsetLeft = yoffset;

    proj = g_pSystem->GetProjectionMatrix(vr::Hmd_Eye::Eye_Right, 1, 10);
    xscale = proj.m[0][0];
    xoffset = proj.m[0][2];
    yscale = proj.m[1][1];
    yoffset = proj.m[1][2];
    tan_px = fabsf((1.0f - xoffset) / xscale);
    tan_nx = fabsf((-1.0f - xoffset) / xscale);
    tan_py = fabsf((1.0f - yoffset) / yscale);
    tan_ny = fabsf((-1.0f - yoffset) / yscale);
    w = tan_px + tan_nx;
    h = tan_py + tan_ny;
    g_horizontalFOVRight = atan(w / 2.0f) * 180 / 3.141592654 * 2;
    g_aspectRatioRight = w / h;
    g_horizontalOffsetRight = xoffset;
    g_verticalOffsetRight = yoffset;

	AngleVectors(ZeroAngle, &VR_playspace_forward, &VR_playspace_right, &VR_playspace_up);		// Extra thing i added for headtracking

    return;
 }
#if defined( CLIENT_DLL )			// Client specific.
 ConCommand vrmod_init("vrmod_init", VRMOD_Init, "Starts VRMod and SteamVR.");
#endif
//*************************************************************************
//    VRMOD_SetActionManifest(fileName)													// Probably converted properly for Virtual Fortress 2 now.
//*************************************************************************
int VRMOD_SetActionManifest(const char* fileName) {

    char currentDir[MAX_STR_LEN];
    GetCurrentDirectory(MAX_STR_LEN, currentDir);
    char path[MAX_STR_LEN];
    sprintf_s(path, MAX_STR_LEN, "%s\\SteamVrActionManifest\\%s", currentDir, fileName);

    g_pInput = vr::VRInput();
    if (g_pInput->SetActionManifestPath(path) != vr::VRInputError_None) {
		Warning("SetActionManifestPath failed");
    }

    FILE* file = NULL;
    fopen_s(&file, path, "r");
    if (file == NULL) {
		Warning("failed to open action manifest");
    }

    memset(g_actions, 0, sizeof(g_actions));

    char word[MAX_STR_LEN];
    while (fscanf_s(file, "%*[^\"]\"%[^\"]\"", word, MAX_STR_LEN) == 1 && strcmp(word, "actions") != 0);
    while (fscanf_s(file, "%[^\"]\"", word, MAX_STR_LEN) == 1) {
        if (strchr(word, ']') != nullptr)
            break;
        if (strcmp(word, "name") == 0) {
            if (fscanf_s(file, "%*[^\"]\"%[^\"]\"", g_actions[g_actionCount].fullname, MAX_STR_LEN) != 1)
                break;
            g_actions[g_actionCount].name = g_actions[g_actionCount].fullname;
            for (int i = 0; i < strlen(g_actions[g_actionCount].fullname); i++) {
                if (g_actions[g_actionCount].fullname[i] == '/')
                    g_actions[g_actionCount].name = g_actions[g_actionCount].fullname + i + 1;
            }
            g_pInput->GetActionHandle(g_actions[g_actionCount].fullname, &(g_actions[g_actionCount].handle));
        }
        if (strcmp(word, "type") == 0) {
            if (fscanf_s(file, "%*[^\"]\"%[^\"]\"", g_actions[g_actionCount].type, MAX_STR_LEN) != 1)
                break;
        }
        if (g_actions[g_actionCount].fullname[0] && g_actions[g_actionCount].type[0]) {
            g_actionCount++;
            if (g_actionCount == MAX_ACTIONS)
                break;
        }
    }

    fclose(file);

    return 0;
}

//*************************************************************************
//    Lua function: VRMOD_SetActiveActionSets(name, ...)												// Probably converted properly for Virtual Fortress 2 now.
//*************************************************************************
void VRMOD_SetActiveActionSets() {
    g_activeActionSetCount = 0;
    for (int i = 0; i < MAX_ACTIONSETS; i++) {
        //if (LUA->GetType(i + 1) == GarrysMod::Lua::Type::STRING) {
		if (ActiveActionSetNames[i].c_str()) {
            //const char* actionSetName = LUA->CheckString(i + 1);
			const char* actionSetName = ActiveActionSetNames[i].c_str();
			// Alternative version (if the above line doesn't work correctly):
			//const char* actionSetName = actionSetNames[i + 1].c_str;
            int actionSetIndex = -1;
            for (int j = 0; j < g_actionSetCount; j++) {
                if (strcmp(actionSetName, g_actionSets[j].name) == 0) {
                    actionSetIndex = j;
                    break;
                }
            }
            if (actionSetIndex == -1) {
                g_pInput->GetActionSetHandle(actionSetName, &g_actionSets[g_actionSetCount].handle);
                memcpy(g_actionSets[g_actionSetCount].name, actionSetName, strlen(actionSetName));
                actionSetIndex = g_actionSetCount;
                g_actionSetCount++;
            }
            g_activeActionSets[g_activeActionSetCount].ulActionSet = g_actionSets[actionSetIndex].handle;
            g_activeActionSetCount++;
        }
        else {
            break;
        }
    }
	return;
    //return 0;
}

//*************************************************************************
//    Lua function: VRMOD_GetViewParameters()											// IMPORTANT FOR HEADTRACKING !!! Properly adjusted for Virtual Fortress 2 now.
//    Returns: table
//*************************************************************************
void VRMOD_GetViewParameters() {

    //uint32_t recommendedWidth = 0;
    //uint32_t recommendedHeight = 0;
    g_pSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);

    vr::HmdMatrix34_t eyeToHeadLeft = g_pSystem->GetEyeToHeadTransform(vr::Eye_Left);
    vr::HmdMatrix34_t eyeToHeadRight = g_pSystem->GetEyeToHeadTransform(vr::Eye_Right);
    eyeToHeadTransformPosLeft.x = eyeToHeadLeft.m[0][3];
    eyeToHeadTransformPosLeft.y = eyeToHeadLeft.m[1][3];
    eyeToHeadTransformPosLeft.z = eyeToHeadLeft.m[2][3];

    eyeToHeadTransformPosRight.x = eyeToHeadRight.m[0][3];
    eyeToHeadTransformPosRight.y = eyeToHeadRight.m[1][3];
    eyeToHeadTransformPosRight.z = eyeToHeadRight.m[2][3];

	return;
}

//*************************************************************************
//    Lua function: VRMOD_UpdatePosesAndActions()																	// Properly adjusted for Virtual Fortress 2 now.
//*************************************************************************
void VRMOD_UpdatePosesAndActions() {
    vr::VRCompositor()->WaitGetPoses(g_poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
    g_pInput->UpdateActionState(g_activeActionSets, sizeof(vr::VRActiveActionSet_t), g_activeActionSetCount);		// UNCOMMENT THIS ONCE WE'VE GOT THE ACTION FUNCTIONS WORKING

	return;
}



//*************************************************************************
//    Lua function: VRMOD_GetPoses()														// IMPORTANT FOR HEADTRACKING !!!   Should be properly adjusted for Virtual Fortress 2 now.
//    Returns: table
//*************************************************************************
void VRMOD_GetPoses() {	

    vr::InputPoseActionData_t poseActionData;
    vr::TrackedDevicePose_t pose;
    char poseName[MAX_STR_LEN];

    //LUA->CreateTable();

    for (int i = -1; i < g_actionCount; i++) {
        //select a pose
        poseActionData.pose.bPoseIsValid = 0;
        pose.bPoseIsValid = 0;
        if (i == -1) {
            pose = g_poses[0];
            memcpy(poseName, "hmd", 4);
        }
        else if (strcmp(g_actions[i].type, "pose") == 0) {
            //g_pInput->GetPoseActionData(g_actions[i].handle, vr::TrackingUniverseStanding, 0, &poseActionData, sizeof(poseActionData), vr::k_ulInvalidInputValueHandle);
			g_pInput->GetPoseActionDataRelativeToNow(g_actions[i].handle, vr::TrackingUniverseStanding, 0, &poseActionData, sizeof(poseActionData), vr::k_ulInvalidInputValueHandle);
            pose = poseActionData.pose;
            strcpy_s(poseName, MAX_STR_LEN, g_actions[i].name);
        }
        else {
            continue;
        }
        
        if (pose.bPoseIsValid) {

            vr::HmdMatrix34_t mat = pose.mDeviceToAbsoluteTracking;
            Vector pos;
            Vector vel;
            QAngle ang;
            QAngle angvel;
            pos.x = -mat.m[2][3];
            pos.y = -mat.m[0][3];
            pos.z = mat.m[1][3];
            ang.x = asin(mat.m[1][2]) * (180.0 / 3.141592654);
            ang.y = atan2f(mat.m[0][2], mat.m[2][2]) * (180.0 / 3.141592654);
            ang.z = atan2f(-mat.m[1][0], mat.m[1][1]) * (180.0 / 3.141592654);
            vel.x = -pose.vVelocity.v[2];
            vel.y = -pose.vVelocity.v[0];
            vel.z = pose.vVelocity.v[1];
            angvel.x = -pose.vAngularVelocity.v[2] * (180.0 / 3.141592654);
            angvel.y = -pose.vAngularVelocity.v[0] * (180.0 / 3.141592654);
            angvel.z = pose.vAngularVelocity.v[1] * (180.0 / 3.141592654);

			// Set the globals so that we can use these results for other functions
			TrackedDevicePoseStruct MyTrackedDevicePoseStruct;
			MyTrackedDevicePoseStruct.TrackedDeviceName = poseName;
			MyTrackedDevicePoseStruct.TrackedDevicePos = pos;
			MyTrackedDevicePoseStruct.TrackedDeviceVel = vel;
			MyTrackedDevicePoseStruct.TrackedDeviceAng = ang;
			MyTrackedDevicePoseStruct.TrackedDeviceAngVel = angvel;
			TrackedDevicesPoses[i + 1] = MyTrackedDevicePoseStruct;
            /*LUA->CreateTable();

            LUA->PushVector(pos);
            LUA->SetField(-2, "pos");

            LUA->PushVector(vel);
            LUA->SetField(-2, "vel");

            LUA->PushAngle(ang);
            LUA->SetField(-2, "ang");

            LUA->PushAngle(angvel);
            LUA->SetField(-2, "angvel");

            LUA->SetField(-2, poseName);
			*/
        }
    }

	return;
    //return 1;
}

////*************************************************************************
////    Lua function: VRMOD_GetActions()																	// Should be properly adjusted for Virtual Fortress 2 now.
////    Returns: table
////*************************************************************************
void VRMOD_GetActions() {
    vr::InputDigitalActionData_t digitalActionData;
    vr::InputAnalogActionData_t analogActionData;
    vr::VRSkeletalSummaryData_t skeletalSummaryData;

    //LUA->CreateTable();

    for (int i = 0; i < g_actionCount; i++) {
        if (strcmp(g_actions[i].type, "boolean") == 0) {
            //LUA->PushBool((g_pInput->GetDigitalActionData(g_actions[i].handle, &digitalActionData, sizeof(digitalActionData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && digitalActionData.bState));
			bool bool1 = (g_pInput->GetDigitalActionData(g_actions[i].handle, &digitalActionData, sizeof(digitalActionData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && digitalActionData.bState);
            //LUA->SetField(-2, g_actions[i].name);
			ActionsBoolStruct MyActionsBool;
			MyActionsBool.ActionName = g_actions[i].name;
			MyActionsBool.BoolData = bool1;
			ActionsBool[i] = MyActionsBool;

        }
        else if (strcmp(g_actions[i].type, "vector1") == 0) {
            g_pInput->GetAnalogActionData(g_actions[i].handle, &analogActionData, sizeof(analogActionData), vr::k_ulInvalidInputValueHandle);
			float float1 = analogActionData.x;
            //LUA->PushNumber(analogActionData.x);
            //LUA->SetField(-2, g_actions[i].name);
			ActionsVector1Struct MyActionsVector1;
			MyActionsVector1.ActionName = g_actions[i].name;
			MyActionsVector1.FloatData = float1;
			ActionsVector1[i] = MyActionsVector1;
        }
        else if (strcmp(g_actions[i].type, "vector2") == 0) {
            //LUA->CreateTable();
            g_pInput->GetAnalogActionData(g_actions[i].handle, &analogActionData, sizeof(analogActionData), vr::k_ulInvalidInputValueHandle);
			float float2 = analogActionData.x;
			float float3 = analogActionData.y;
            /*LUA->PushNumber(analogActionData.x);
            LUA->SetField(-2, "x");
            LUA->PushNumber(analogActionData.y);
            LUA->SetField(-2, "y");
            LUA->SetField(-2, g_actions[i].name);
			*/
			ActionsVector2Struct MyActionsVector2;
			MyActionsVector2.ActionName = g_actions[i].name;
			MyActionsVector2.FloatData1 = float2;
			MyActionsVector2.FloatData2 = float3;
			ActionsVector2[i] = MyActionsVector2;
        }
        else if (strcmp(g_actions[i].type, "skeleton") == 0) {
            g_pInput->GetSkeletalSummaryData(g_actions[i].handle, vr::VRSummaryType_FromAnimation, &skeletalSummaryData);
            //LUA->CreateTable();
            //LUA->CreateTable();
			ActionsSkeletonStruct MyActionsSkeleton;
			MyActionsSkeleton.ActionName = g_actions[i].name;
			MyActionsSkeleton.FloatFingerCurlData1 = skeletalSummaryData.flFingerCurl[0];
			MyActionsSkeleton.FloatFingerCurlData2 = skeletalSummaryData.flFingerCurl[1];
			MyActionsSkeleton.FloatFingerCurlData3 = skeletalSummaryData.flFingerCurl[2];
			MyActionsSkeleton.FloatFingerCurlData4 = skeletalSummaryData.flFingerCurl[3];
			MyActionsSkeleton.FloatFingerCurlData5 = skeletalSummaryData.flFingerCurl[4];
            //for (int j = 0; j < 5; j++) {
                //LUA->PushNumber(j + 1);
                //LUA->PushNumber(skeletalSummaryData.flFingerCurl[j]);
                //LUA->SetTable(-3);
            //}
           // LUA->SetField(-2, "fingerCurls");
           // LUA->SetField(-2, g_actions[i].name);
			ActionsSkeleton[i] = MyActionsSkeleton;
        }
    }

	return;

    //return 1;
}

//*************************************************************************
//    VRMOD_ShareTextureBegin()																// converted properly for Virtual Fortress 2 now.
//*************************************************************************
void VRMOD_ShareTextureBegin() {
    HWND activeWindow = GetActiveWindow();
    if (activeWindow == NULL) {
        Warning("GetActiveWindow failed");
    }

    //hiding and restoring the game window is a workaround to d3d9 CreateDevice
    //failing on the second thread if the game is fullscreen
    ShowWindow(activeWindow, SW_HIDE);
    HANDLE thread = CreateThread(NULL, 0, FindCreateTexture, 0, 0, NULL);
    if (thread == NULL) {
        Warning("CreateThread failed");
    }
    WaitForSingleObject(thread, 1000);  // Needs to be at least 27 milliseconds before we start seeing CreateTextureHook() being called
    ShowWindow(activeWindow, SW_RESTORE);
    DWORD exitCode = 4;
    GetExitCodeThread(thread, &exitCode);
    CloseHandle(thread);
    if (exitCode != 0) {
        if (exitCode == 1) {
			Warning("Direct3DCreate9 failed");
        }
        else if (exitCode == 2) {
			Warning("CreateWindowA failed");
        }
        else if (exitCode == 3) {
			Warning("CreateDevice failed");
        }
        else {
			Warning("GetExitCodeThread failed");
        }
    }

    if (g_CreateTextureAddr == NULL) {
		Warning("g_CreateTextureAddr is null");
    }

    g_CreateTextureOriginal = (CreateTexture)g_CreateTextureAddr;

    if (MH_Initialize() != MH_OK) {
		Warning("MH_Initialize failed");
    }

    if (MH_CreateHook((DWORD_PTR*)g_CreateTextureAddr, &CreateTextureHook, reinterpret_cast<void**>(&g_CreateTextureOriginal)) != MH_OK) {
		Warning("MH_CreateHook failed");
    }

    if (MH_EnableHook((DWORD_PTR*)g_CreateTextureAddr) != MH_OK) {
		Warning("MH_EnableHook failed");
    }

    return;
}


//*************************************************************************
//    VRMOD_ShareTextureFinish()															// converted properly for Virtual Fortress 2 now.
//*************************************************************************
void VRMOD_ShareTextureFinish() {
    if (g_sharedTexture == NULL) {
		Warning("g_sharedTexture is null");
    }

    if (D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, NULL, D3D11_SDK_VERSION, &g_d3d11Device, NULL, NULL) != S_OK) {
		Warning("D3D11CreateDevice failed");
    }

    ID3D11Resource* res;
    if (FAILED(g_d3d11Device->OpenSharedResource(g_sharedTexture, __uuidof(ID3D11Resource), (void**)&res))) {
		Warning("OpenSharedResource failed");
    }

    if (FAILED(res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&g_d3d11Texture))) {
		Warning("QueryInterface failed");
    }

    MH_DisableHook((DWORD_PTR*)g_CreateTextureAddr);
    MH_RemoveHook((DWORD_PTR*)g_CreateTextureAddr);
    if (MH_Uninitialize() != MH_OK) {
		Warning("MH_Uninitialize failed");
    }

    return;
}


//*************************************************************************
//    VRMOD_SubmitSharedTexture()																// converted properly for Virtual Fortress 2 now.
//*************************************************************************
void VRMOD_SubmitSharedTexture() {
    if (g_d3d11Texture == NULL)
        return;

    IDirect3DQuery9* pEventQuery = nullptr;
    g_d3d9Device->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);
    if (pEventQuery != nullptr)
    {
        pEventQuery->Issue(D3DISSUE_END);
        while (pEventQuery->GetData(nullptr, 0, D3DGETDATA_FLUSH) != S_OK);
        pEventQuery->Release();
    }

    vr::Texture_t vrTexture = { g_d3d11Texture, vr::TextureType_DirectX, vr::ColorSpace_Auto };

    vr::VRTextureBounds_t textureBounds;

    //submit Left eye
    textureBounds.uMin = 0.0f + g_horizontalOffsetLeft * 0.25f;
    textureBounds.uMax = 0.5f + g_horizontalOffsetLeft * 0.25f;
    textureBounds.vMin = 0.0f - g_verticalOffsetLeft * 0.5f;
    textureBounds.vMax = 1.0f - g_verticalOffsetLeft * 0.5f;

    vr::VRCompositor()->Submit(vr::EVREye::Eye_Left, &vrTexture, &textureBounds);

    //submit Right eye
    textureBounds.uMin = 0.5f + g_horizontalOffsetRight * 0.25f;
    textureBounds.uMax = 1.0f + g_horizontalOffsetRight * 0.25f;
    textureBounds.vMin = 0.0f - g_verticalOffsetRight * 0.5f;
    textureBounds.vMax = 1.0f - g_verticalOffsetRight * 0.5f;

    vr::VRCompositor()->Submit(vr::EVREye::Eye_Right, &vrTexture, &textureBounds);

    return;
}


//*************************************************************************
//    VRMOD_Start()																				// converted properly for Virtual Fortress 2 now.
//*************************************************************************
void VRMOD_Start() {

    g_pSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
	// Temporarily change resolution untill we can use the actual recommended resolution without messing up rendering
	recommendedWidth = 960;
	recommendedHeight = 1080;

	VRMOD_ShareTextureBegin();
	g_pMaterialSystem->BeginRenderTargetAllocation();
	//RenderTarget_VRMod = g_pMaterialSystem->CreateNamedRenderTargetTextureEx("vrmod_rt", 2 * recommendedWidth, recommendedHeight, RT_SIZE_DEFAULT, g_pMaterialSystem->GetBackBufferFormat(), MATERIAL_RT_DEPTH_SHARED, TEXTUREFLAGS_NOMIP);
	RenderTarget_VRMod = materials->CreateNamedRenderTargetTextureEx("vrmod_rt", 2 * recommendedWidth, recommendedHeight, RT_SIZE_DEFAULT, materials->GetBackBufferFormat(), MATERIAL_RT_DEPTH_SHARED, TEXTUREFLAGS_NOMIP);
	VRMOD_ShareTextureFinish();


	VRMOD_SetActionManifest("vrmod_action_manifest.txt");	// Newly added for headtracking.
	ActiveActionSetNames[0] = "/actions/vrmod";
	VRMOD_SetActiveActionSets();


	#if defined( CLIENT_DLL )			// Client specific.
		//pPlayer = (C_TFPlayer *)C_BasePlayer::GetLocalPlayer();
		pPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		LocalPlayerIndex = GetLocalPlayerIndex();
	#else								// Server specific
		pPlayer = (CTFPlayerShared *)UTIL_PlayerByIndex(LocalPlayerIndex);
	#endif

	RenderTarget_VRMod_GUI = materials->CreateNamedRenderTargetTextureEx("_rt_gui", 2 * recommendedWidth, recommendedHeight, RT_SIZE_DEFAULT, materials->GetBackBufferFormat(), MATERIAL_RT_DEPTH_SHARED, TEXTUREFLAGS_NOMIP);
	//VRMOD_UtilSetOrigin(pPlayer->EyePosition());
	#if defined( CLIENT_DLL )			// Client specific.
		VRMOD_UtilHandleTracking();
	#endif
	VRMod_Started = 1;

}
#if defined( CLIENT_DLL )			// Client specific.
ConCommand vrmod_start("vrmod_start", VRMOD_Start, "Finally starts VRMod");
#endif

//*************************************************************************
//    VRMOD_Shutdown()																			// converted properly for Virtual Fortress 2 now.
//*************************************************************************
void VRMOD_Shutdown() {
    if (g_pSystem != NULL) {
        vr::VR_Shutdown();
        g_pSystem = NULL;
    }
	VRMod_Started = 0;
	g_pMaterialSystem->EndRenderTargetAllocation();
    if (g_d3d11Device != NULL) {
        g_d3d11Device->Release();
        g_d3d11Device = NULL;
    }
    g_d3d11Texture = NULL;
    g_sharedTexture = NULL;
    g_CreateTextureAddr = NULL;
    g_actionCount = 0;
    g_actionSetCount = 0;
    g_activeActionSetCount = 0;
    g_d3d9Device = NULL;
	
    return;
}
#if defined( CLIENT_DLL )			// Client specific.
ConCommand vrmod_shutdown("vrmod_shutdown", VRMOD_Shutdown, "Stops VRMod and SteamVR and cleans up.");
#endif

//*************************************************************************
//    Lua function: VRMOD_TriggerHaptic(actionName, delay, duration, frequency, amplitude)				// converted properly for Virtual Fortress 2 now.
//*************************************************************************
void VRMOD_TriggerHaptic(const char* actionName, float delay, float duration, float frequency, float amplitude) {
    unsigned int nameLen = strlen(actionName);
    for (int i = 0; i < g_actionCount; i++) {
        if (strlen(g_actions[i].name) == nameLen && memcmp(g_actions[i].name, actionName, nameLen) == 0) {
			g_pInput->TriggerHapticVibrationAction(g_actions[i].handle, delay, duration, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
            break;
        }
    }
	return;
}

//*************************************************************************
//    Lua function: VRMOD_GetTrackedDeviceNames()														// Should work properly for Virtual Fortress 2 now.
//    Returns: table
//*************************************************************************
void VRMOD_GetTrackedDeviceNames() {
    //LUA->CreateTable();
    //int tableIndex = 1;
    char name[MAX_STR_LEN];
    for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
        if (g_pSystem->GetStringTrackedDeviceProperty(i, vr::Prop_ControllerType_String, name, MAX_STR_LEN) > 1) {
            //LUA->PushNumber(tableIndex);
            //LUA->PushString(name);
            //LUA->SetTable(-3);
            //tableIndex++;
			TrackedDevicesNames[i] = name;
        }
    }
	return ;
    //return 1;
}
#if defined( CLIENT_DLL )			// Client specific.
ConCommand vrmod_get_tracked_device_names("vrmod_get_tracked_device_names", VRMOD_GetTrackedDeviceNames, "Debug info.");
#endif

int VRMOD_GetRecWidth()																					// works properly for Virtual Fortress 2 now.
{
	g_pSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
	return recommendedWidth;
}

int VRMOD_GetRecHeight()																				// works properly for Virtual Fortress 2 now.
{
	g_pSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
	return recommendedHeight;
}


// Headtracking functions

// Sets vr origin so that hmd will be at given pos
void VRMOD_UtilSetOrigin(Vector pos)
{
	VRMOD_GetPoses;
	Vector VR_hmd_pos_local = TrackedDevicesPoses[0].TrackedDevicePos;   // The hmd should be device 0 i think, implement something more robust later.
	//VR_origin = pos + (VR_origin - VR_hmd_pos_local);
	VR_origin = pos + (VR_origin - (VR_hmd_pos_local * VR_scale));
	return;
}

// I should probably implement this for turning the character
// Rotates origin while maintaining hmd pos
void VRMOD_UtilSetOriginAngle(QAngle ang) 
{
	return;
}


#if defined( CLIENT_DLL )			// Client specific.
void VRMOD_UtilHandleTracking()
{
	VRMOD_GetPoses();
	PlayerEyeAngles = pPlayer->EyeAngles();
	QAngle EyeNoYaw = QAngle(0, PlayerEyeAngles.y, PlayerEyeAngles.z);

	QAngle VR_hmd_ang_local = TrackedDevicesPoses[0].TrackedDeviceAng;					// The hmd should be device 0 i think, implement something more robust later.
	Vector VR_hmd_pos_local = TrackedDevicesPoses[0].TrackedDevicePos;					// The hmd should be device 0 i think, implement something more robust later.

	AngleVectors((VR_hmd_ang_local + EyeNoYaw), &VR_hmd_forward, &VR_hmd_right, &VR_hmd_up);			// Get the direction vectors from the local HMD Qangle

	AngleVectors(EyeNoYaw, &Player_forward, &Player_right, &Player_up);					// Get the direction vectors relative to the way the player's currently facing

	float VR_hmd_pos_local_playspace_forward = DotProduct(VR_hmd_pos_local, VR_playspace_forward);
	float VR_hmd_pos_local_playspace_right = DotProduct(VR_hmd_pos_local, VR_playspace_right);
	float VR_hmd_pos_local_playspace_up = DotProduct(VR_hmd_pos_local, VR_playspace_up);

	Vector VR_hmd_pos_local_in_world = VR_hmd_pos_local_playspace_forward * Player_forward + VR_hmd_pos_local_playspace_right * Player_right + VR_hmd_pos_local_playspace_up * Player_up;
	VR_hmd_pos_local_in_world = VR_hmd_pos_local_in_world * VR_scale;
	//VR_hmd_pos_local_in_world.x = VR_hmd_pos_local_in_world.x * 52.49;
	//VR_hmd_pos_local_in_world.y = VR_hmd_pos_local_in_world.y * 52.49;
	//VR_hmd_pos_local_in_world.z = VR_hmd_pos_local_in_world.z * 39.37012415030996;

	VR_hmd_pos_abs = pPlayer->EyePosition() - Vector(0, 0, 64 * 52.49 / 39.37012415030996) + VR_hmd_pos_local_in_world;		// 64 Hammer Units is standing player height.

									
	VR_hmd_ang_abs = EyeNoYaw + VR_hmd_ang_local - VR_Player_Spawn_HMD_Angles;

	VRMOD_GetViewParameters();
	ipd = eyeToHeadTransformPosRight.x * 2;
	eyez = eyeToHeadTransformPosRight.z;


	// Handtracking

	Vector VR_controller_left_pos_local = TrackedDevicesPoses[6].TrackedDevicePos;											// left controller pose is in TrackedDevicesPoses[6]
	QAngle VR_controller_left_ang_local = TrackedDevicesPoses[6].TrackedDeviceAng;

	Vector VR_controller_right_pos_local = TrackedDevicesPoses[7].TrackedDevicePos;											// right controller pose is in TrackedDevicesPoses[7]
	QAngle VR_controller_right_ang_local = TrackedDevicesPoses[7].TrackedDeviceAng;

	AngleVectors(VR_controller_left_ang_local, &VR_controller_left_forward, &VR_controller_left_right, &VR_controller_left_up);			// Get the direction vectors from the local Left controller Qangle
	AngleVectors(VR_controller_right_ang_local, &VR_controller_right_forward, &VR_controller_right_right, &VR_controller_right_up);			// Get the direction vectors from the local Right controller Qangle

	float VR_controller_left_pos_local_playspace_forward = DotProduct(VR_controller_left_pos_local, VR_playspace_forward);
	float VR_controller_left_pos_local_playspace_right = DotProduct(VR_controller_left_pos_local, VR_playspace_right);
	float VR_controller_left_pos_local_playspace_up = DotProduct(VR_controller_left_pos_local, VR_playspace_up);

	float VR_controller_right_pos_local_playspace_forward = DotProduct(VR_controller_right_pos_local, VR_playspace_forward);
	float VR_controller_right_pos_local_playspace_right = DotProduct(VR_controller_right_pos_local, VR_playspace_right);
	float VR_controller_right_pos_local_playspace_up = DotProduct(VR_controller_right_pos_local, VR_playspace_up);


	Vector VR_controller_left_pos_local_in_world = VR_controller_left_pos_local_playspace_forward * Player_forward + VR_controller_left_pos_local_playspace_right * Player_right + VR_controller_left_pos_local_playspace_up * Player_up;
	VR_controller_left_pos_local_in_world = VR_controller_left_pos_local_in_world * VR_scale;


	Vector VR_controller_right_pos_local_in_world = VR_controller_right_pos_local_playspace_forward * Player_forward + VR_controller_right_pos_local_playspace_right * Player_right + VR_controller_right_pos_local_playspace_up * Player_up;
	VR_controller_right_pos_local_in_world = VR_controller_right_pos_local_in_world * VR_scale;


	VR_controller_left_pos_abs = pPlayer->EyePosition() - Vector(0, 0, 64 * 52.49 / 39.37012415030996) + VR_controller_left_pos_local_in_world;		// 64 Hammer Units is standing player height.

	VR_controller_right_pos_abs = pPlayer->EyePosition() - Vector(0, 0, 64 * 52.49 / 39.37012415030996) + VR_controller_right_pos_local_in_world;		// 64 Hammer Units is standing player height.

	// controller angles
	VR_controller_left_ang_abs = EyeNoYaw + VR_controller_left_ang_local;

	VR_controller_right_ang_abs = EyeNoYaw + VR_controller_right_ang_local;

	// Process action input
	VRMOD_Process_input();

	return;
}
#endif


void VRMOD_SetGestureOrigin(Vector pos)
{
	GestureOrigin = pos;
	return;
}

#if defined( CLIENT_DLL )			// Client specific.
int VRMOD_SelectGestureDirection(Vector GesturePos, Vector Forward, Vector Right, Vector Up)
{
	int SelectedDirection = 0;		// Default value = At center, not far enough to select a real direction
	Vector GestureDistance = GesturePos - GestureOrigin;

	// Express distance in local controller axes
	float GestureDistance_forward = DotProduct(GestureDistance, Forward);
	float GestureDistance_right = DotProduct(GestureDistance, Right);
	float GestureDistance_up = DotProduct(GestureDistance, Up);

	GestureDistance = GestureDistance_forward * Forward + GestureDistance_right * Right + GestureDistance_up * Up;

	// Determine biggest abs component of Vector (-> Direction the player selected)
	Vector GestureDistanceAbs;
	VectorAbs(GestureDistance, GestureDistanceAbs);
	float GestureDistanceAbsMax = VectorMaximum(GestureDistanceAbs);

	if (GestureDistanceAbsMax > GestureMinSelectionDistance)		// If we're far enough from the GestureOrigin to make a selection
	{
		if (GestureDistanceAbsMax == GestureDistanceAbs.x) {		// If our max is the X axis
			if (GestureDistance.x < 0.0f) {			// If negative X axis selected
				SelectedDirection = 1;
			}
			if (GestureDistance.x > 0.0f) {			// If positive X axis selected
				SelectedDirection = 2;
			}
		}

		if (GestureDistanceAbsMax == GestureDistanceAbs.y) {		// If our max is the Y axis
			if (GestureDistance.y < 0.0f) {			// If negative Y axis selected
				SelectedDirection = 3;
			}
			if (GestureDistance.y > 0.0f) {			// If positive Y axis selected
				SelectedDirection = 4;
			}
		}

		if (GestureDistanceAbsMax == GestureDistanceAbs.z) {		// If our max is the Z axis
			if (GestureDistance.z < 0.0f) {			// If negative Z axis selected
				SelectedDirection = 1;
			}
			if (GestureDistance.z > 0.0f) {			// If positive Z axis selected
				SelectedDirection = 2;
			}
		}
	}

	return SelectedDirection;

}
#endif

#if defined( CLIENT_DLL )			// Client specific.
void VRMOD_Process_input()
{

	/*
	- g_actions[0] = boolean_primaryfire
	- g_actions[1] = boolean_secondaryfire
	- g_actions[4] = boolean_spawnmenu
	- g_actions[11] = vector2_walkdirection
	- g_actions[12] = vector2_right_joystick
	- g_actions[14] = boolean_walk
	- g_actions[16] = boolean_turnleft
	- g_actions[17] = boolean_turnright
	- g_actions[19] = boolean_reload
	- g_actions[20] = boolean_jump	
	- g_actions[25] = boolean_changeweapon_up
	- g_actions[26] = boolean_changeweapon_down
	- g_actions[27] = boolean_crouch
	- g_actions[28] = boolean_medic
	*/
	
	VRMOD_GetActions();

	// Movement
	if (ActionsBool[14].BoolData == true)		//if the action: input boolean_walk is true
	{
		if (ActionsVector2[11].FloatData2 > 0.5)	// if vector2_walkdirection.y > 0.5
		{
			engine->ClientCmd("-back");
			engine->ClientCmd("+forward");
		}
		else if (ActionsVector2[11].FloatData2 < -0.5)		// if vector2_walkdirection.y < -0.5
		{
			engine->ClientCmd("-forward");
			engine->ClientCmd("+back");
		}
		else
		{
			engine->ClientCmd("-back");
			engine->ClientCmd("-forward");
		}

		if (ActionsVector2[11].FloatData1 > 0.5)		// if vector2_walkdirection.x > 0.5
		{
			engine->ClientCmd("-moveleft");
			engine->ClientCmd("+moveright");
		}
		else if (ActionsVector2[11].FloatData1 < -0.5)		// if vector2_walkdirection.x < -0.5
		{
			engine->ClientCmd("-moveright");
			engine->ClientCmd("+moveleft");
		}
		else
		{
			engine->ClientCmd("-moveright");
			engine->ClientCmd("-moveleft");
		}
	}
	else
	{
		engine->ClientCmd("-forward");
		engine->ClientCmd("-back");
		engine->ClientCmd("-moveleft");
		engine->ClientCmd("-moveright");
	}

	// Jumping
	if (ActionsBool[20].BoolData == true)	// if boolean_jump is true
	{
		engine->ClientCmd("+jump");
	}
	else
	{
		engine->ClientCmd("-jump");
	}

	// Crouching
	if (ActionsBool[27].BoolData == true)	// if boolean_crouch is true
	{
		engine->ClientCmd("+duck");
	}
	else
	{
		engine->ClientCmd("-duck");
	}

	// Right Trigger

	if  (ActionsBool[0].BoolData == true)		 // if boolean_primaryfire is true
	{
		Trigger_active_counter++;
	}
	else 
	{
		Trigger_active_counter = 0;
	}
	
	C_TFWeaponBase * Weapon = pPlayer->GetActiveTFWeapon();
	int WeaponID = Weapon->GetWeaponID();
	//const CTFWeaponInfo WeaponInfo = Weapon->GetTFWpnData();
	//if (WeaponInfo.m_iWeaponType == TF_WPN_TYPE_PDA)		// If the currently active weapon is the PDA
	//if (pPlayer->IsPlayerClass(TF_CLASS_ENGINEER) )				// If the player is currently engineer
	if (WeaponID == TF_WEAPON_PDA_ENGINEER_BUILD)		// If the currently active weapon is the PDA
	{				
			if (Trigger_active_counter == 20)					// If we just started holding down the trigger
			{
				VRMOD_SetGestureOrigin(VRMOD_GetRightControllerAbsPos());
				engine->ClientCmd("r_drawviewmodel 0");
				GestureMenuIndex = 1;
			}
			else if (Trigger_active_counter > 20)				// If we are holding down the trigger
			{
				GestureMinSelectionDistance = 10.0f;

				GestureSelection = VRMOD_SelectGestureDirection(VRMOD_GetRightControllerAbsPos(), VR_controller_right_forward, VR_controller_right_right, VR_controller_right_up);

				GestureQuadMaterials[0] = "hud/eng_build_sentry_blueprint";
				GestureQuadMaterials[1] = "hud/eng_build_dispenser_blueprint";
				GestureQuadMaterials[2] = "hud/eng_build_tele_entrance_blueprint";
				GestureQuadMaterials[3] = "hud/eng_build_tele_exit_blueprint";
				GestureQuadMaterials[4] = "";
				GestureQuadMaterials[5] = "";

				AngleVectors(VRMOD_GetRightControllerAbsAngle(), &GestureForward, &GestureRight, &GestureUp);
				

				//GestureForward = VR_controller_right_forward;
				//GestureRight = VR_controller_right_right;
				//GestureUp = VR_controller_right_up;
				
			}
			else if (GestureMenuIndex != 0)						// If we made our selection
			{
				GestureMenuIndex = 0;
				engine->ClientCmd("r_drawviewmodel 1");
				switch (GestureSelection)
				{
				case 0:
					break;
				case 1:
					engine->ClientCmd("build 2");				// Build a sentry
					break;
				case 2:
					engine->ClientCmd("build 0");				// Build a dispenser
					break;
				case 3:
					engine->ClientCmd("build 1");				// Build a teleporter entrance
					break;
				case 4:
					engine->ClientCmd("build 3");				// Build a teleporter exit
					break;
				default:
					break;
				}
			}
	}
	else 
	{
		if (ActionsBool[0].BoolData == true)	// if boolean_primaryfire is true
		{
			engine->ClientCmd("+attack");
		}
		else
		{
			engine->ClientCmd("-attack");
		}

		if (ActionsBool[1].BoolData == true)	// if boolean_secondaryfire is true
		{
			engine->ClientCmd("+attack2");
		}
		else
		{
			engine->ClientCmd("-attack2");
		}
	}


	// Weapon switching
	if ((ActionsBool[25].BoolData == true) && (!switch_up_just_switched))	// if boolean_changeweapon_up is true
	{
		engine->ClientCmd("invprev");
		switch_up_counter = 0;
		switch_up_just_switched = true;
	}
	else
	{
		switch_up_counter += 1;
		if (switch_up_counter > 20)
		{
			switch_up_just_switched = false;
		}
		
	}

	if ((ActionsBool[26].BoolData == true) && (!switch_down_just_switched))	// if boolean_changeweapon_down is true
	{
		engine->ClientCmd("invnext");
		switch_down_counter = 0;
		switch_down_just_switched = true;
	}
	else
	{
		switch_down_counter += 1;
		if (switch_down_counter > 20)
		{
			switch_down_just_switched = false;
		}
		
	}

	// Reloading
	if (ActionsBool[19].BoolData == true)	// if boolean_reload is true
	{
		engine->ClientCmd("+reload");
	}
	else
	{
		engine->ClientCmd("-reload");
	}

	// Calling for a medic
	if (ActionsBool[28].BoolData == true)	// if boolean_medic is true
	{
		engine->ClientCmd("voicemenu 0 0");
	}



	// Turning
	
	//"cl_yawspeed", "cl_pitchspeed"

	if (ActionsBool[16].BoolData == true)	// if boolean_turnleft is true
	{
		engine->ClientCmd("-right");
		engine->ClientCmd("+left");
	}
	else
	{
		engine->ClientCmd("-left");
	}

	if (ActionsBool[17].BoolData == true)	// if boolean_turnright is true
	{
		engine->ClientCmd("-left");
		engine->ClientCmd("+right");
	}
	else
	{
		engine->ClientCmd("-right");
	}



	// Spawn a contextual menu. Currently just taunts
	if (ActionsBool[4].BoolData == true)	// if boolean_spawnmenu is true
	{
		engine->ClientCmd("taunt");
	}





	return;
}
#endif


// --------------------------------------------------------------------
// Purpose: Returns the bounds in world space where the game should 
//			position the HUD.
// --------------------------------------------------------------------
void GetHUDBounds(Vector *pViewer, Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR)
{

	Vector vHalfWidth = (-VRMOD_GetPlayerRight()) * -32;
	Vector vHalfHeight = VRMOD_GetPlayerUp() * 18;
	Vector vHUDOrigin = VRMOD_GetViewOriginLeft() + VRMOD_GetPlayerForward() * 40;

	*pViewer = VRMOD_GetViewOriginLeft();
	*pUL = vHUDOrigin - vHalfWidth + vHalfHeight;
	*pUR = vHUDOrigin + vHalfWidth + vHalfHeight;
	*pLL = vHUDOrigin - vHalfWidth - vHalfHeight;
	*pLR = vHUDOrigin + vHalfWidth - vHalfHeight;
}


#if defined( CLIENT_DLL )			// Client specific.
// --------------------------------------------------------------------
// Purpose: Renders the HUD in the world.
// --------------------------------------------------------------------
void RenderHUDQuad(bool bBlackout, bool bTranslucent)
{

	Vector vHead, vUL, vUR, vLL, vLR;
	GetHUDBounds(&vHead, &vUL, &vUR, &vLL, &vLR);

	CMatRenderContextPtr pRenderContext(materials);


	IMaterial *mymat = NULL;
	if (bTranslucent)
	{
		mymat = materials->FindMaterial("vgui/inworldui", TEXTURE_GROUP_VGUI);
	}
	else
	{
		mymat = materials->FindMaterial("vgui/inworldui_opaque", TEXTURE_GROUP_VGUI);
	}
	Assert(!mymat->IsErrorMaterial());

	IMesh *pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, mymat);

	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

	meshBuilder.Position3fv(vLR.Base());
	meshBuilder.TexCoord2f(0, 1, 1);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3fv(vLL.Base());
	meshBuilder.TexCoord2f(0, 0, 1);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3fv(vUR.Base());
	meshBuilder.TexCoord2f(0, 1, 0);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3fv(vUL.Base());
	meshBuilder.TexCoord2f(0, 0, 0);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.End();
	pMesh->Draw();

	if (bBlackout)
	{
		Vector vbUL, vbUR, vbLL, vbLR;
		// "Reflect" the HUD bounds through the viewer to find the ones behind the head.
		vbUL = 2 * vHead - vLR;
		vbUR = 2 * vHead - vLL;
		vbLL = 2 * vHead - vUR;
		vbLR = 2 * vHead - vUL;

		IMaterial *mymat = materials->FindMaterial("vgui/black", TEXTURE_GROUP_VGUI);
		IMesh *pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, mymat);

		// Tube around the outside.
		CMeshBuilder meshBuilder;
		meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 8);

		meshBuilder.Position3fv(vLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.End();
		pMesh->Draw();

		// Cap behind the viewer.
		meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

		meshBuilder.Position3fv(vbUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv(vbLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.End();
		pMesh->Draw();
	}

	RenderVRCrosshair();
	if (GestureMenuIndex != 0)
	{
		RenderGestureQuads();
	}
}
#endif

#if defined( CLIENT_DLL )			// Client specific.
void GetVRCrosshairBounds(Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR)
{
	Vector CrosshairForward, CrosshairRight, CrosshairUp;
	AngleVectors(VRMOD_GetRecommendedViewmodelAbsAngle(), &CrosshairForward, &CrosshairRight, &CrosshairUp);			// Get the direction vectors from the Viewmodel QAngle

	Vector vHalfWidth = (-CrosshairRight) * -16.7;
	Vector vHalfHeight = CrosshairUp * 16.7;
	Vector vHUDOrigin = VRMOD_GetRecommendedViewmodelAbsPos() + CrosshairForward * 500;

	*pUL = vHUDOrigin - vHalfWidth + vHalfHeight;
	*pUR = vHUDOrigin + vHalfWidth + vHalfHeight;
	*pLL = vHUDOrigin - vHalfWidth - vHalfHeight;
	*pLR = vHUDOrigin + vHalfWidth - vHalfHeight;
}
#endif

#if defined( CLIENT_DLL )			// Client specific.
void RenderVRCrosshair()
{
	Vector vUL, vUR, vLL, vLR;
	GetVRCrosshairBounds(&vUL, &vUR, &vLL, &vLR);

	CMatRenderContextPtr pRenderContext(materials);


	IMaterial *mymat = NULL;
	mymat = materials->FindMaterial("vgui/crosshairs/default", TEXTURE_GROUP_VGUI);

	Assert(!mymat->IsErrorMaterial());

	IMesh *pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, mymat);

	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

	meshBuilder.Position3fv(vLR.Base());
	meshBuilder.TexCoord2f(0, 1, 1);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3fv(vLL.Base());
	meshBuilder.TexCoord2f(0, 0, 1);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3fv(vUR.Base());
	meshBuilder.TexCoord2f(0, 1, 0);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.Position3fv(vUL.Base());
	meshBuilder.TexCoord2f(0, 0, 0);
	meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

	meshBuilder.End();
	pMesh->Draw();
	return;
}
#endif


#if defined( CLIENT_DLL )			// Client specific.
void GetGestureQuadsBounds(Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR, int i, Vector Forward, Vector Right, Vector Up, float MinSelectionDistance = 4.0f)
{
	
	Vector vHalfWidth = VR_hmd_right * 6;
	Vector vHalfHeight = VR_hmd_up * 6;
	Vector vHUDOrigin;

	switch (i)
	{
	case 0:
		vHUDOrigin = GestureOrigin + (-Right) * MinSelectionDistance;
		break;
	case 1:
		vHUDOrigin = GestureOrigin + Right * MinSelectionDistance;
		break;
	case 2:
		vHUDOrigin = GestureOrigin + (-Forward) * MinSelectionDistance;
		break;
	case 3:
		vHUDOrigin = GestureOrigin + Forward * MinSelectionDistance;
		break;
	case 4:
		vHUDOrigin = GestureOrigin + (-Up) * MinSelectionDistance;
		break;
	case 5:
		vHUDOrigin = GestureOrigin + Up * MinSelectionDistance;
		break;
	default:
		vHUDOrigin = GestureOrigin;
		break;
	}

	*pUL = vHUDOrigin - vHalfWidth + vHalfHeight;
	*pUR = vHUDOrigin + vHalfWidth + vHalfHeight;
	*pLL = vHUDOrigin - vHalfWidth - vHalfHeight;
	*pLR = vHUDOrigin + vHalfWidth - vHalfHeight;

	return;
}
#endif


#if defined( CLIENT_DLL )			// Client specific.
void RenderGestureQuads()
{
	for (int i = 0; i < 6; i++)
	{
		if (!GestureQuadMaterials[i].empty())
		{
			Vector vUL, vUR, vLL, vLR;
			GetGestureQuadsBounds(&vUL, &vUR, &vLL, &vLR, i, GestureForward, GestureRight, GestureUp, GestureMinSelectionDistance);

			CMatRenderContextPtr pRenderContext(materials);

			pRenderContext->OverrideDepthEnable(true, true);

			IMaterial *mymat = NULL;
			mymat = materials->FindMaterial(GestureQuadMaterials[i].c_str(), TEXTURE_GROUP_VGUI);

			Assert(!mymat->IsErrorMaterial());

			IMesh *pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, mymat);

			CMeshBuilder meshBuilder;
			meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

			meshBuilder.Position3fv(vLR.Base());
			meshBuilder.TexCoord2f(0, 1, 1);
			meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

			meshBuilder.Position3fv(vLL.Base());
			meshBuilder.TexCoord2f(0, 0, 1);
			meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

			meshBuilder.Position3fv(vUR.Base());
			meshBuilder.TexCoord2f(0, 1, 0);
			meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

			meshBuilder.Position3fv(vUL.Base());
			meshBuilder.TexCoord2f(0, 0, 0);
			meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

			meshBuilder.End();
			pMesh->Draw();

			pRenderContext->OverrideDepthEnable(false, false);
		}
	}

	return;
}
#endif

void VRMOD_Show_poses()
{
	Msg(" local HMD pos x = %.2f \n", TrackedDevicesPoses[0].TrackedDevicePos.x);
	Msg(" local HMD pos y = %.2f \n", TrackedDevicesPoses[0].TrackedDevicePos.y);
	Msg(" local HMD pos z = %.2f \n", TrackedDevicesPoses[0].TrackedDevicePos.z);

	Msg(" local HMD ang x = %.2f \n", TrackedDevicesPoses[0].TrackedDeviceAng.x);
	Msg(" local HMD ang y = %.2f \n", TrackedDevicesPoses[0].TrackedDeviceAng.y);
	Msg(" local HMD ang z = %.2f \n", TrackedDevicesPoses[0].TrackedDeviceAng.z);

	Msg(" local IPD = %.2f \n", ipd);
	Msg(" local eyeZ = %.2f \n", eyez);

	Msg(" absolute left controller pos x = %.2f \n", VR_controller_left_pos_abs.x);
	Msg(" absolute left controller pos y = %.2f \n", VR_controller_left_pos_abs.y);
	Msg(" absolute left controller pos z = %.2f \n", VR_controller_left_pos_abs.z);

	Msg(" absolute left controller angle x = %.2f \n", VR_controller_left_ang_abs.x);
	Msg(" absolute left controller angle y = %.2f \n", VR_controller_left_ang_abs.y);
	Msg(" absolute left controller angle z = %.2f \n", VR_controller_left_ang_abs.z);

	Msg(" absolute right controller pos x = %.2f \n", VR_controller_right_pos_abs.x);
	Msg(" absolute right controller pos y = %.2f \n", VR_controller_right_pos_abs.y);
	Msg(" absolute right controller pos z = %.2f \n", VR_controller_right_pos_abs.z);

	Msg(" absolute right controller angle x = %.2f \n", VR_controller_right_ang_abs.x);
	Msg(" absolute right controller angle y = %.2f \n", VR_controller_right_ang_abs.y);
	Msg(" absolute right controller angle z = %.2f \n", VR_controller_right_ang_abs.z);

}
#if defined( CLIENT_DLL )			// Client specific.
ConCommand vrmod_show_poses("vrmod_show_poses", VRMOD_Show_poses, "Debug info.");
#endif
void VRMOD_General_Debug()
{
	actionSet DebugActionSet = g_actionSets[0];
	TrackedDevicePoseStruct DebugTrackedDevicesPoses = TrackedDevicesPoses[0];
	vr::VRActiveActionSet_t DebugActiveActionSets = g_activeActionSets[0];
	action DebugActions = g_actions[0];
	int DebugActiveActionSetCount = g_activeActionSetCount;
	int DebugActionSetCount = g_actionSetCount;
	int DebugActionCount = g_actionCount;

	Msg("End of Debug function\n");
}
#if defined( CLIENT_DLL )			// Client specific.
ConCommand vrmod_general_debug("vrmod_general_debug", VRMOD_General_Debug, "Debug info.");
#endif
QAngle VRMOD_GetViewAngle()
{
	return VR_hmd_ang_abs;
}

Vector VRMOD_GetViewOriginLeft()
{

	Vector view_temp_origin;

	view_temp_origin = VR_hmd_pos_abs + (VR_hmd_forward * (-(eyez * VR_scale)));
	//view_temp_origin = view_temp_origin + (VR_hmd_right * (-((ipd * VR_scale) / 2)));
	view_temp_origin = view_temp_origin + (VR_hmd_right * (-((ipd * VRMOD_ipd_scale.GetFloat()) / 2)));

	return view_temp_origin;
}

Vector VRMOD_GetViewOriginRight()
{

	Vector view_temp_origin;

	view_temp_origin = VR_hmd_pos_abs + (VR_hmd_forward * (-(eyez * VR_scale)));
	//view_temp_origin = view_temp_origin + (VR_hmd_right * (ipd * VR_scale));
	view_temp_origin = view_temp_origin + (VR_hmd_right * (ipd * VRMOD_ipd_scale.GetFloat()));

	return view_temp_origin;
}


Vector VRMOD_GetRecommendedViewmodelAbsPos()
{
	Vector VModelPos = Vector(0, 0, 0);

#if defined( CLIENT_DLL )			// Client specific.
	C_BaseViewModel* VModel = pPlayer->GetViewModel();
	CTFWeaponBase *pWeapon = assert_cast<CTFWeaponBase*>(VModel->GetWeapon());
	if (pWeapon)
	{
		WeaponOffset = pWeapon->GetViewOffset();
		VModelPos = VRMOD_GetRightControllerAbsPos() - WeaponOffset;
	}
	else
	{
		VModelPos = VRMOD_GetRightControllerAbsPos();
	}
#else								// Server specific
	VModelPos = VRMOD_GetRightControllerAbsPos() - WeaponOffset;
#endif
	return VR_controller_right_pos_abs;
}


QAngle VRMOD_GetRecommendedViewmodelAbsAngle()
{
	return (VR_controller_right_ang_abs - QAngle(-45, 0, 0));
}

QAngle VRMOD_GetRightControllerAbsAngle()
{
	return VR_controller_right_ang_abs;
}

Vector VRMOD_GetRightControllerAbsPos()
{
	return VR_controller_right_pos_abs;
}


// Used for making the player face the right direction when spawning
void VRMOD_SetSpawnPlayerHMDAngles()
{
	VR_Player_Spawn_HMD_Angles = TrackedDevicesPoses[0].TrackedDeviceAng;
}

Vector VRMOD_GetPlayerForward()
{
	return Player_forward;
}

Vector VRMOD_GetPlayerRight()
{
	return Player_right;
}

Vector VRMOD_GetPlayerUp()
{
	return Player_up;
}