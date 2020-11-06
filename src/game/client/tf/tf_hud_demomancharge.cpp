//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "tf_weaponbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDemomanChargeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudDemomanChargeMeter, EditablePanel );

public:
	CHudDemomanChargeMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	vgui::ContinuousProgressBar *m_pChargeMeter;
};

DECLARE_HUDELEMENT( CHudDemomanChargeMeter );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDemomanChargeMeter::CHudDemomanChargeMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDemomanCharge" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDemomanChargeMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudDemomanCharge.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudDemomanChargeMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) || !pPlayer->IsAlive() )
	{
		return false;
	}

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
	{
		return false;
	}

	int iWeaponID = pWpn->GetWeaponID();

	if ( iWeaponID != TF_WEAPON_PIPEBOMBLAUNCHER )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDemomanChargeMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	ITFChargeUpWeapon *pChargeupWeapon = dynamic_cast< ITFChargeUpWeapon *>( pWpn );

	if ( !pWpn || !pChargeupWeapon )
		return;

	if ( m_pChargeMeter )
	{
		float flChargeMaxTime = pChargeupWeapon->GetChargeMaxTime();

		if ( flChargeMaxTime != 0 )
		{
			float flChargeBeginTime = pChargeupWeapon->GetChargeBeginTime();

			if ( flChargeBeginTime > 0 )
			{
				float flTimeCharged = max( 0, gpGlobals->curtime - flChargeBeginTime );
				float flPercentCharged = min( 1.0, flTimeCharged / flChargeMaxTime );

				m_pChargeMeter->SetProgress( flPercentCharged );
			}
			else
			{
				m_pChargeMeter->SetProgress( 0.0f );
			}
		}
	}
}