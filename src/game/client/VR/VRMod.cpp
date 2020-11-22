#define WIN32_LEAN_AND_MEAN
#include "cbase.h"
#include <stdio.h>
#include <d3d11.h>
#include <d3d9.h>
#include <Windows.h>
#include <openvr.h>
#include <MinHook.h>
#include <convar.h>
#include <synchapi.h>
#include <processthreadsapi.h>

#pragma comment (lib, "User32.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3d9.lib")


/*//*************************************************************************

	THIS BRANCH IS BASED ON THE NEW GMOD VRMOD CODE. IT DOESN'T INCLUDE MINHOOK ANYMORE! MAKE SURE TO CHECK THE NEW VRMOD CODE IF YOU ENCOUNTER ANY ERRORS!

//  Current errors:		



	Fixed errors:		- This time we have LNK2019 errors related to openvr-stuff

						FIX: including openvr_api.lib to the linker and linker lib directories

	
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
typedef HRESULT(APIENTRY *CreateTexture)(IDirect3DDevice9*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*);
char                    g_createTextureOrigBytes[14];
CreateTexture           g_createTexture = NULL;
ID3D11Device*           g_d3d11Device = NULL;
ID3D11Texture2D*        g_d3d11Texture = NULL;
HANDLE                  g_sharedTexture = NULL;
DWORD_PTR               g_CreateTextureAddr = NULL;
IDirect3DDevice9*		g_pD3D9Device = NULL;

//other
typedef void*           (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
float                   g_horizontalFOVLeft = 0;
float                   g_horizontalFOVRight = 0;
float                   g_aspectRatioLeft = 0;
float                   g_aspectRatioRight = 0;
float                   g_horizontalOffsetLeft = 0;
float                   g_horizontalOffsetRight = 0;
float                   g_verticalOffsetLeft = 0;
float                   g_verticalOffsetRight = 0;

//*************************************************************************
//  CreateTexture hook
//*************************************************************************
HRESULT APIENTRY CreateTextureHook(IDirect3DDevice9* pDevice, UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9** tex, HANDLE* shared_handle) {
	if (WriteProcessMemory(GetCurrentProcess(), g_createTexture, g_createTextureOrigBytes, 14, NULL) == 0)
		MessageBoxA(NULL, "WriteProcessMemory from hook failed", "", NULL);
	if (g_sharedTexture == NULL) {
        shared_handle = &g_sharedTexture;
        pool = D3DPOOL_DEFAULT;
    }
	return g_createTexture(pDevice, w, h, levels, usage, format, pool, tex, shared_handle);
};

//*************************************************************************
//    FindCreateTexture thread
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

////*************************************************************************
////    Lua function: VRMOD_GetVersion()
////    Returns: number
////*************************************************************************
//LUA_FUNCTION(VRMOD_GetVersion) {
//    LUA->PushNumber(16);
//    return 1;
//}
//
////*************************************************************************
////    Lua function: VRMOD_IsHMDPresent()
////    Returns: boolean
////*************************************************************************
//LUA_FUNCTION(VRMOD_IsHMDPresent) {
//    LUA->PushBool(vr::VR_IsHmdPresent());
//    return 1;
//}


//*************************************************************************
//    VRMOD_Init():		Initialize SteamVR and set some important globals
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

	HMODULE hMod = GetModuleHandleA("shaderapidx9.dll");
	if (hMod == NULL) Warning("GetModuleHandleA failed");
	CreateInterfaceFn CreateInterface = (CreateInterfaceFn)GetProcAddress(hMod, "CreateInterface");
	if (CreateInterface == NULL) Warning("GetProcAddress failed");
#ifdef _WIN64
	DWORD_PTR fnAddr = ((DWORD_PTR**)CreateInterface("ShaderDevice001", NULL))[0][5];
	g_pD3D9Device = *(IDirect3DDevice9**)(fnAddr + 8 + (*(DWORD_PTR*)(fnAddr + 3) & 0xFFFFFFFF));
#else
	g_pD3D9Device = **(IDirect3DDevice9***)(((DWORD_PTR**)CreateInterface("ShaderDevice001", NULL))[0][5] + 2);
#endif
	g_createTexture = ((CreateTexture**)g_pD3D9Device)[0][23];

    //return 0;
 }

 ConCommand vrmod_init("vrmod_init", VRMOD_Init, "Starts VRMod and SteamVR.");
 // IMPORTANT INFO: CONCOMMAND ONLY WORKS FOR VOID FUNCTIONS!

//*************************************************************************
//    VRMOD_SetActionManifest(fileName)
//*************************************************************************
int VRMOD_SetActionManifest(const char* fileName) {
    //const char* fileName = LUA->CheckString(1);

    char currentDir[MAX_STR_LEN];
    GetCurrentDirectory(MAX_STR_LEN, currentDir);
    char path[MAX_STR_LEN];
    sprintf_s(path, MAX_STR_LEN, "%s\\tf_port\\%s", currentDir, fileName);

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

////*************************************************************************
////    Lua function: VRMOD_SetActiveActionSets(name, ...)
////*************************************************************************
//int VRMOD_SetActiveActionSets() {
//    g_activeActionSetCount = 0;
//    for (int i = 0; i < MAX_ACTIONSETS; i++) {
//        if (LUA->GetType(i + 1) == GarrysMod::Lua::Type::STRING) {
//            const char* actionSetName = LUA->CheckString(i + 1);
//            int actionSetIndex = -1;
//            for (int j = 0; j < g_actionSetCount; j++) {
//                if (strcmp(actionSetName, g_actionSets[j].name) == 0) {
//                    actionSetIndex = j;
//                    break;
//                }
//            }
//            if (actionSetIndex == -1) {
//                g_pInput->GetActionSetHandle(actionSetName, &g_actionSets[g_actionSetCount].handle);
//                memcpy(g_actionSets[g_actionSetCount].name, actionSetName, strlen(actionSetName));
//                actionSetIndex = g_actionSetCount;
//                g_actionSetCount++;
//            }
//            g_activeActionSets[g_activeActionSetCount].ulActionSet = g_actionSets[actionSetIndex].handle;
//            g_activeActionSetCount++;
//        }
//        else {
//            break;
//        }
//    }
//    return 0;
//}

////*************************************************************************
////    Lua function: VRMOD_GetViewParameters()
////    Returns: table
////*************************************************************************
//LUA_FUNCTION(VRMOD_GetViewParameters) {
//    LUA->CreateTable();
//
//    LUA->PushNumber(g_horizontalFOVLeft);
//    LUA->SetField(-2, "horizontalFOVLeft");
//
//    LUA->PushNumber(g_horizontalFOVRight);
//    LUA->SetField(-2, "horizontalFOVRight");
//
//    LUA->PushNumber(g_aspectRatioLeft);
//    LUA->SetField(-2, "aspectRatioLeft");
//
//    LUA->PushNumber(g_aspectRatioRight);
//    LUA->SetField(-2, "aspectRatioRight");
//
//    uint32_t recommendedWidth = 0;
//    uint32_t recommendedHeight = 0;
//    g_pSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
//
//    LUA->PushNumber(recommendedWidth);
//    LUA->SetField(-2, "recommendedWidth");
//
//    LUA->PushNumber(recommendedHeight);
//    LUA->SetField(-2, "recommendedHeight");
//
//    vr::HmdMatrix34_t eyeToHeadLeft = g_pSystem->GetEyeToHeadTransform(vr::Eye_Left);
//    vr::HmdMatrix34_t eyeToHeadRight = g_pSystem->GetEyeToHeadTransform(vr::Eye_Right);
//    Vector eyeToHeadTransformPos;
//    eyeToHeadTransformPos.x = eyeToHeadLeft.m[0][3];
//    eyeToHeadTransformPos.y = eyeToHeadLeft.m[1][3];
//    eyeToHeadTransformPos.z = eyeToHeadLeft.m[2][3];
//    LUA->PushVector(eyeToHeadTransformPos);
//    LUA->SetField(-2, "eyeToHeadTransformPosLeft");
//
//    eyeToHeadTransformPos.x = eyeToHeadRight.m[0][3];
//    eyeToHeadTransformPos.y = eyeToHeadRight.m[1][3];
//    eyeToHeadTransformPos.z = eyeToHeadRight.m[2][3];
//    LUA->PushVector(eyeToHeadTransformPos);
//    LUA->SetField(-2, "eyeToHeadTransformPosRight");
//
//    return 1;
//}

//*************************************************************************
//    Lua function: VRMOD_UpdatePosesAndActions()
//*************************************************************************
int VRMOD_UpdatePosesAndActions() {
    vr::VRCompositor()->WaitGetPoses(g_poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
    g_pInput->UpdateActionState(g_activeActionSets, sizeof(vr::VRActiveActionSet_t), g_activeActionSetCount);
    return 0;
}

////*************************************************************************
////    Lua function: VRMOD_GetPoses()
////    Returns: table
////*************************************************************************
//LUA_FUNCTION(VRMOD_GetPoses) {
//    vr::InputPoseActionData_t poseActionData;
//    vr::TrackedDevicePose_t pose;
//    char poseName[MAX_STR_LEN];
//
//    LUA->CreateTable();
//
//    for (int i = -1; i < g_actionCount; i++) {
//        //select a pose
//        poseActionData.pose.bPoseIsValid = 0;
//        pose.bPoseIsValid = 0;
//        if (i == -1) {
//            pose = g_poses[0];
//            memcpy(poseName, "hmd", 4);
//        }
//        else if (strcmp(g_actions[i].type, "pose") == 0) {
//            g_pInput->GetPoseActionData(g_actions[i].handle, vr::TrackingUniverseStanding, 0, &poseActionData, sizeof(poseActionData), vr::k_ulInvalidInputValueHandle);
//            pose = poseActionData.pose;
//            strcpy_s(poseName, MAX_STR_LEN, g_actions[i].name);
//        }
//        else {
//            continue;
//        }
//        //
//        if (pose.bPoseIsValid) {
//
//            vr::HmdMatrix34_t mat = pose.mDeviceToAbsoluteTracking;
//            Vector pos;
//            Vector vel;
//            QAngle ang;
//            QAngle angvel;
//            pos.x = -mat.m[2][3];
//            pos.y = -mat.m[0][3];
//            pos.z = mat.m[1][3];
//            ang.x = asin(mat.m[1][2]) * (180.0 / 3.141592654);
//            ang.y = atan2f(mat.m[0][2], mat.m[2][2]) * (180.0 / 3.141592654);
//            ang.z = atan2f(-mat.m[1][0], mat.m[1][1]) * (180.0 / 3.141592654);
//            vel.x = -pose.vVelocity.v[2];
//            vel.y = -pose.vVelocity.v[0];
//            vel.z = pose.vVelocity.v[1];
//            angvel.x = -pose.vAngularVelocity.v[2] * (180.0 / 3.141592654);
//            angvel.y = -pose.vAngularVelocity.v[0] * (180.0 / 3.141592654);
//            angvel.z = pose.vAngularVelocity.v[1] * (180.0 / 3.141592654);
//
//            LUA->CreateTable();
//
//            LUA->PushVector(pos);
//            LUA->SetField(-2, "pos");
//
//            LUA->PushVector(vel);
//            LUA->SetField(-2, "vel");
//
//            LUA->PushAngle(ang);
//            LUA->SetField(-2, "ang");
//
//            LUA->PushAngle(angvel);
//            LUA->SetField(-2, "angvel");
//
//            LUA->SetField(-2, poseName);
//
//        }
//    }
//
//    return 1;
//}
//
////*************************************************************************
////    Lua function: VRMOD_GetActions()
////    Returns: table
////*************************************************************************
//LUA_FUNCTION(VRMOD_GetActions) {
//    vr::InputDigitalActionData_t digitalActionData;
//    vr::InputAnalogActionData_t analogActionData;
//    vr::VRSkeletalSummaryData_t skeletalSummaryData;
//
//    LUA->CreateTable();
//
//    for (int i = 0; i < g_actionCount; i++) {
//        if (strcmp(g_actions[i].type, "boolean") == 0) {
//            LUA->PushBool((g_pInput->GetDigitalActionData(g_actions[i].handle, &digitalActionData, sizeof(digitalActionData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && digitalActionData.bState));
//            LUA->SetField(-2, g_actions[i].name);
//        }
//        else if (strcmp(g_actions[i].type, "vector1") == 0) {
//            g_pInput->GetAnalogActionData(g_actions[i].handle, &analogActionData, sizeof(analogActionData), vr::k_ulInvalidInputValueHandle);
//            LUA->PushNumber(analogActionData.x);
//            LUA->SetField(-2, g_actions[i].name);
//        }
//        else if (strcmp(g_actions[i].type, "vector2") == 0) {
//            LUA->CreateTable();
//            g_pInput->GetAnalogActionData(g_actions[i].handle, &analogActionData, sizeof(analogActionData), vr::k_ulInvalidInputValueHandle);
//            LUA->PushNumber(analogActionData.x);
//            LUA->SetField(-2, "x");
//            LUA->PushNumber(analogActionData.y);
//            LUA->SetField(-2, "y");
//            LUA->SetField(-2, g_actions[i].name);
//        }
//        else if (strcmp(g_actions[i].type, "skeleton") == 0) {
//            g_pInput->GetSkeletalSummaryData(g_actions[i].handle, &skeletalSummaryData);
//            LUA->CreateTable();
//            LUA->CreateTable();
//            for (int j = 0; j < 5; j++) {
//                LUA->PushNumber(j + 1);
//                LUA->PushNumber(skeletalSummaryData.flFingerCurl[j]);
//                LUA->SetTable(-3);
//            }
//            LUA->SetField(-2, "fingerCurls");
//            LUA->SetField(-2, g_actions[i].name);
//        }
//    }
//
//    return 1;
//}

//*************************************************************************
//    VRMOD_ShareTextureBegin()
//*************************************************************************
int VRMOD_ShareTextureBegin() {

	char patch[] = "\x68\x0\x0\x0\x0\xC3\x44\x24\x04\x0\x0\x0\x0\xC3";
	*(DWORD*)(patch + 1) = (DWORD)((DWORD_PTR)CreateTextureHook);
#ifdef _WIN64
	patch[5] = '\xC7';
	*(DWORD*)(patch + 9) = (DWORD)((DWORD_PTR)CreateTextureHook >> 32);
#endif

	if (ReadProcessMemory(GetCurrentProcess(), g_createTexture, g_createTextureOrigBytes, 14, NULL) == 0)
		Warning("ReadProcessMemory failed");
	if (WriteProcessMemory(GetCurrentProcess(), g_createTexture, patch, 14, NULL) == 0)
		Warning("WriteProcessMemory failed");

    return 0;
}

//*************************************************************************
//    VRMOD_ShareTextureFinish()
//*************************************************************************
int VRMOD_ShareTextureFinish() {

	if (g_sharedTexture == NULL)
		Warning("g_sharedTexture is null");
	if (D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, NULL, D3D11_SDK_VERSION, &g_d3d11Device, NULL, NULL) != S_OK)
		Warning("D3D11CreateDevice failed");
	ID3D11Resource* res;
	if (FAILED(g_d3d11Device->OpenSharedResource(g_sharedTexture, __uuidof(ID3D11Resource), (void**)&res)))
		Warning("OpenSharedResource failed");
	if (FAILED(res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&g_d3d11Texture)))
		Warning("QueryInterface failed");

    return 0;
}

//*************************************************************************
//    VRMOD_SubmitSharedTexture()
//*************************************************************************
int VRMOD_SubmitSharedTexture() {

	if (g_d3d11Texture == NULL)
		return 0;

	IDirect3DQuery9* pEventQuery = nullptr;
	g_pD3D9Device->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);
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
    return 0;
}

//*************************************************************************
//    VRMOD_Shutdown()
//*************************************************************************
int VRMOD_Shutdown() {
	if (g_pSystem != NULL) {
		vr::VR_Shutdown();
		g_pSystem = NULL;
	}
	if (g_d3d11Device != NULL) {
		g_d3d11Device->Release();
		g_d3d11Device = NULL;
	}
	g_d3d11Texture = NULL;
	g_sharedTexture = NULL;
	g_actionCount = 0;
	g_actionSetCount = 0;
	g_activeActionSetCount = 0;
	g_pD3D9Device = NULL;
    return 0;
}


////*************************************************************************
////    Lua function: VRMOD_TriggerHaptic(actionName, delay, duration, frequency, amplitude)
////*************************************************************************
//LUA_FUNCTION(VRMOD_TriggerHaptic) {
//    const char* actionName = LUA->CheckString(1);
//    unsigned int nameLen = strlen(actionName);
//    for (int i = 0; i < g_actionCount; i++) {
//        if (strlen(g_actions[i].name) == nameLen && memcmp(g_actions[i].name, actionName, nameLen) == 0) {
//            g_pInput->TriggerHapticVibrationAction(g_actions[i].handle, LUA->CheckNumber(2), LUA->CheckNumber(3), LUA->CheckNumber(4), LUA->CheckNumber(5), vr::k_ulInvalidInputValueHandle);
//            break;
//        }
//    }
//    return 0;
//}
//
////*************************************************************************
////    Lua function: VRMOD_GetTrackedDeviceNames()
////    Returns: table
////*************************************************************************
//LUA_FUNCTION(VRMOD_GetTrackedDeviceNames) {
//    LUA->CreateTable();
//    int tableIndex = 1;
//    char name[MAX_STR_LEN];
//    for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
//        if (g_pSystem->GetStringTrackedDeviceProperty(i, vr::Prop_ControllerType_String, name, MAX_STR_LEN) > 1) {
//            LUA->PushNumber(tableIndex);
//            LUA->PushString(name);
//            LUA->SetTable(-3);
//            tableIndex++;
//        }
//    }
//    return 1;
//}
