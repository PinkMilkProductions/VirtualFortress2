#include "cbase.h"
#include <string>


class VRCollisionBox
{
public:
	// the origin is the center of the cube
	Vector origin;
	// contains the half of the x, y and z-length (can be different numbers)
	Vector halfLength;
	// a simple ID for the box so you know which one has the collision
	int boxID;

	VRCollisionBox(Vector myOrigin, Vector myHalfLength, int myBoxID);

	// returns wether the point is inside the collisionbox
	bool IsPointInBox(Vector point);

private:
	float xMin;
	float xMax;
	float yMin;
	float yMax;
	float zMin;
	float zMax;
};

class VRGestureMenuOption
{
	public:
		//An index for differentiating between different menu option objects
		int Index;
		// Upper Left vertex
		Vector UL;
		// Upper Right vertex
		Vector UR;
		// Lower Left vertex
		Vector LL;
		// Lower Right vertex
		Vector LR;
		// Center of the quad and the collissionbox
		Vector Origin;
		// Float halfLength of the quad
		float HalfLength;
		// Material of the Quad
		std::string GestureQuadMaterial = "";
		// The accompanying collission for the option
		VRCollisionBox *OptionCollission;

		VRGestureMenuOption(std::string Material, int i);
		~VRGestureMenuOption();

};

class VRGestureMenu
{
public:
	enum XMenu { Main, ClassSelect, Voice1, Voice2, Voice3, Build, Destroy, Disguise };
	int GestureMenuIndex;
	int GestureNumItems;
	float GestureMinSelectionDistance;
	//std::string GestureQuadMaterials[6] = { "", "", "", "", "", "" };
	VRGestureMenuOption *MenuOptions[11];
	Vector GestureOrigin = Vector(0, 0, 0);

	VRGestureMenu();
	~VRGestureMenu();
	// Sets the gesture origin
	void SetGestureOrigin(Vector pos);
	// Selects the gestureMenu option based on where the pos is
	int SelectGestureOption(Vector GesturePos);
	// Gets the bounds for the Gesture Options
	void GetGestureQuadsBounds(Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR, Vector *Origin, float *HalfLength, int i);
	// Renders the gesture quads
	void RenderGestureQuads();
	void SwitchMenuType(XMenu XMenuToSet);

private:
	XMenu CurrentXMenu = Main;
	std::string AllMenuOptionMaterials[8][11] = {
		// Main
		{"effects/speech_voice", "effects/speech_voice", "backpack/player/items/all_class/all_laugh_taunt_large", "effects/speech_voice", "hud/hud_icon_capture", 
		"hud/ico_teamswitch","","","","",""},
		// ClassSelect	
		{"hud/leaderboard_class_scout", "hud/leaderboard_class_soldier", "hud/leaderboard_class_pyro", "hud/leaderboard_class_demo", "hud/leaderboard_class_heavy", 
		"hud/leaderboard_class_engineer", "hud/leaderboard_class_medic", "hud/leaderboard_class_sniper", "hud/leaderboard_class_spy", "hud/ico_teamswitch", "hud/arrow_big_down"},
		// Voice1			
		{"effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice", 
		"effects/speech_voice","effects/speech_voice", "effects/speech_voice", "", "hud/arrow_big_down"},
		// Voice2
		{"effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice",
		"effects/speech_voice","effects/speech_voice", "effects/speech_voice", "", "hud/arrow_big_down"},
		// Voice3
		{"effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice", "effects/speech_voice",
		"effects/speech_voice","effects/speech_voice", "effects/speech_voice", "", "hud/arrow_big_down"},
		// Build
		{"hud/eng_build_sentry_blueprint", "hud/eng_build_dispenser_blueprint", "hud/eng_build_tele_entrance_blueprint", "hud/eng_build_tele_exit_blueprint", 
		"", "","", "", "", "", ""},
		// Destroy
		{"hud/eng_build_sentry_blueprint", "hud/eng_build_dispenser_blueprint", "hud/eng_build_tele_entrance_blueprint", "hud/eng_build_tele_exit_blueprint",
		"", "","", "", "", "", ""},
		// Disguise
		{"hud/leaderboard_class_scout", "hud/leaderboard_class_soldier", "hud/leaderboard_class_pyro", "hud/leaderboard_class_demo", "hud/leaderboard_class_heavy",
		"hud/leaderboard_class_engineer", "hud/leaderboard_class_medic", "hud/leaderboard_class_sniper", "hud/leaderboard_class_spy", "hud/ico_teamswitch", "hud/arrow_big_down"}
	};

	void GetXMenuMaterials(std::string *MaterialsArray, XMenu MyXMenu);
	//void SetMenuOptionMaterials();

};
VRGestureMenu *GestureMenu;