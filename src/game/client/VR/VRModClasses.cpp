#include "cbase.h"
#include "VRModClasses.h"

//*************************************************************************
//  VRCollisionBox
//*************************************************************************

VRCollisionBox::VRCollisionBox(Vector myOrigin, Vector myHalfLength, int myBoxID)
{
	boxID = myBoxID;
	origin = myOrigin;
	halfLength = myHalfLength;

	xMin = myOrigin.x - myHalfLength.x;
	xMax = myOrigin.x + myHalfLength.x;
	yMin = myOrigin.y - myHalfLength.y;
	yMax = myOrigin.y + myHalfLength.y;
	zMin = myOrigin.z - myHalfLength.z;
	zMax = myOrigin.z + myHalfLength.z;
}


bool VRCollisionBox::IsPointInBox(Vector point)
{
	return ( ( (xMin < point.x) && (point.x < xMax) ) && ( (yMin < point.y) && (point.y < yMax) ) && ( (zMin < point.z) && (point.z < zMax) ) );
}


//*************************************************************************
//  VRGestureMenuOption
//*************************************************************************

VRGestureMenuOption::VRGestureMenuOption(std::string Material, int i)
{
	Index = i;
	GestureQuadMaterial = Material;
	GestureMenu->GetGestureQuadsBounds(&UL, &UR, &LL, &LR, &Origin, &HalfLength, Index);
	OptionCollission = new VRCollisionBox(Origin, Vector(HalfLength, HalfLength, HalfLength), Index);
}

VRGestureMenuOption::~VRGestureMenuOption()
{
	delete OptionCollission;
}

//*************************************************************************
//  VRGestureMenu
//*************************************************************************

VRGestureMenu::VRGestureMenu()
{
	std::string OptionMaterials[11];
	GetXMenuMaterials(OptionMaterials, CurrentXMenu);
	for (int j = 0; j < 11; j++)
	{
		MenuOptions[j] = new VRGestureMenuOption(OptionMaterials[j], j);
	}
}

VRGestureMenu::~VRGestureMenu()
{
	delete MenuOptions;
}

void VRGestureMenu::GetXMenuMaterials(std::string *MaterialsArray, XMenu MyXMenu)
{
	MaterialsArray = AllMenuOptionMaterials[MyXMenu];
}

void VRGestureMenu::SwitchMenuType(XMenu XMenuToSet)
{
	CurrentXMenu = XMenuToSet;
}
void VRGestureMenu::SetGestureOrigin(Vector pos)
{
	GestureOrigin = pos;
}

