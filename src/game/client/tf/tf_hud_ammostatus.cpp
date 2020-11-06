//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "tf_shareddefs.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "tf_controls.h"
#include "in_buttons.h"
#include "tf_imagepanel.h"
#include "c_team.h"
#include "c_tf_player.h"
#include "ihudlcd.h"
#include "tf_hud_ammostatus.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


DECLARE_HUDELEMENT( CTFHudWeaponAmmo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudWeaponAmmo::CTFHudWeaponAmmo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudWeaponAmmo" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	hudlcd->SetGlobalStat( "(ammo_primary)", "0" );
	hudlcd->SetGlobalStat( "(ammo_secondary)", "0" );
	hudlcd->SetGlobalStat( "(weapon_print_name)", "" );
	hudlcd->SetGlobalStat( "(weapon_name)", "" );

	m_pInClip = NULL;
	m_pInClipShadow = NULL;
	m_pInReserve = NULL;
	m_pInReserveShadow = NULL;
	m_pNoClip = NULL;
	m_pNoClipShadow = NULL;

	m_nAmmo	= -1;
	m_nAmmo2 = -1;
	m_hCurrentActiveWeapon = NULL;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudAmmoWeapons.res" );

	m_pInClip = dynamic_cast<CTFLabel *>( FindChildByName( "AmmoInClip" ) );
	m_pInClipShadow = dynamic_cast<CTFLabel *>( FindChildByName( "AmmoInClipShadow" ) );

	m_pInReserve = dynamic_cast<CTFLabel *>( FindChildByName( "AmmoInReserve" ) );
	m_pInReserveShadow = dynamic_cast<CTFLabel *>( FindChildByName( "AmmoInReserveShadow" ) );

	m_pNoClip = dynamic_cast<CTFLabel *>( FindChildByName( "AmmoNoClip" ) );
	m_pNoClipShadow = dynamic_cast<CTFLabel *>( FindChildByName( "AmmoNoClipShadow" ) );

	m_nAmmo	= -1;
	m_nAmmo2 = -1;
	m_hCurrentActiveWeapon = NULL;
	m_flNextThink = 0.0f;

	UpdateAmmoLabels( false, false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudWeaponAmmo::ShouldDraw( void )
{
	// Get the player and active weapon.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
	{
		return false;
	}

	CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

	if ( !pWeapon )
	{
		return false;
	}

	if ( pWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::UpdateAmmoLabels( bool bPrimary, bool bReserve, bool bNoClip )
{
	if ( m_pInClip && m_pInClipShadow )
	{
		if ( m_pInClip->IsVisible() != bPrimary )
		{
			m_pInClip->SetVisible( bPrimary );
			m_pInClipShadow->SetVisible( bPrimary );
		}
	}

	if ( m_pInReserve && m_pInReserveShadow )
	{
		if ( m_pInReserve->IsVisible() != bReserve )
		{
			m_pInReserve->SetVisible( bReserve );
			m_pInReserveShadow->SetVisible( bReserve );
		}
	}

	if ( m_pNoClip && m_pNoClipShadow )
	{
		if ( m_pNoClip->IsVisible() != bNoClip )
		{
			m_pNoClip->SetVisible( bNoClip );
			m_pNoClipShadow->SetVisible( bNoClip );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get ammo info from the weapon and update the displays.
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( m_flNextThink < gpGlobals->curtime )
	{
		hudlcd->SetGlobalStat( "(weapon_print_name)", pWeapon ? pWeapon->GetPrintName() : " " );
		hudlcd->SetGlobalStat( "(weapon_name)", pWeapon ? pWeapon->GetName() : " " );

		if ( !pPlayer || !pWeapon || !pWeapon->UsesPrimaryAmmo() )
		{
			hudlcd->SetGlobalStat( "(ammo_primary)", "n/a" );
			hudlcd->SetGlobalStat( "(ammo_secondary)", "n/a" );

			// turn off our ammo counts
			UpdateAmmoLabels( false, false, false );

			m_nAmmo = -1;
			m_nAmmo2 = -1;
		}
		else
		{
			// Get the ammo in our clip.
			int nAmmo1 = pWeapon->Clip1();
			int nAmmo2 = 0;
			// Clip ammo not used, get total ammo count.
			if ( nAmmo1 < 0 )
			{
				nAmmo1 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			}
			// Clip ammo, so the second ammo is the total ammo.
			else
			{
				nAmmo2 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			}
			
			hudlcd->SetGlobalStat( "(ammo_primary)", VarArgs( "%d", nAmmo1 ) );
			hudlcd->SetGlobalStat( "(ammo_secondary)", VarArgs( "%d", nAmmo2 ) );

			if ( m_nAmmo != nAmmo1 || m_nAmmo2 != nAmmo2 || m_hCurrentActiveWeapon.Get() != pWeapon )
			{
				m_nAmmo = nAmmo1;
				m_nAmmo2 = nAmmo2;
				m_hCurrentActiveWeapon = pWeapon;

				if ( m_hCurrentActiveWeapon.Get()->UsesClipsForAmmo1() )
				{
					UpdateAmmoLabels( true, true, false );

					SetDialogVariable( "Ammo", m_nAmmo );
					SetDialogVariable( "AmmoInReserve", m_nAmmo2 );
				}
				else
				{
					UpdateAmmoLabels( false, false, true );
					SetDialogVariable( "Ammo", m_nAmmo );
				}
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}
