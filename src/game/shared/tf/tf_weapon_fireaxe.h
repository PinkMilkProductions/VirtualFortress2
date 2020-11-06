//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_FIREAXE_H
#define TF_WEAPON_FIREAXE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFFireAxe C_TFFireAxe
#endif

//=============================================================================
//
// BrandingIron class.
//
class CTFFireAxe : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFFireAxe, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFireAxe() {}
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_FIREAXE; }

private:

	CTFFireAxe( const CTFFireAxe & ) {}
};

#endif // TF_WEAPON_FIREAXE_H
