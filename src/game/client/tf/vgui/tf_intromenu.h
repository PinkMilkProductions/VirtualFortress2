//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_INTROMENU_H
#define TF_INTROMENU_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_vgui_video.h"
#include "basemodelpanel.h"

#define MAX_CAPTION_LENGTH	256

class CVideoCaption
{
public:
	CVideoCaption()
	{
		m_pszString = NULL;
		m_flStartTime = 0;
		m_flDisplayTime = 0;
		m_flCaptionStart = -1;
	}

	~CVideoCaption()
	{
		if ( m_pszString && m_pszString[0] )
		{
			delete [] m_pszString;
			m_pszString = NULL;
		}
	}

	const char *m_pszString;		// the string to display (can be a localized # string)
	float		m_flStartTime;		// the offset from the beginning of the video when we should show this caption
	float		m_flDisplayTime;	// the length of time the string should be displayed once it's shown
	float		m_flCaptionStart;	// the time when the caption is shown (so we know when to turn it off
};

//-----------------------------------------------------------------------------
// Purpose: displays the Intro menu
//-----------------------------------------------------------------------------

class CTFIntroMenu : public CIntroMenu
{
private:
	DECLARE_CLASS_SIMPLE( CTFIntroMenu, CIntroMenu );

public:
	CTFIntroMenu( IViewPort *pViewPort );
	~CTFIntroMenu();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ShowPanel( bool bShow );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( KeyCode code );

	void OnTick();

	MESSAGE_FUNC( OnIntroFinished, "IntroFinished" );

private:
	void SetNextThink( float flActionThink, int iAction );
	void Shutdown( void );
	bool LoadCaptions( void );
	void UpdateCaptions( void );

private:

	CTFVideoPanel	*m_pVideo;
	CModelPanel		*m_pModel;
	CTFLabel		*m_pCaptionLabel;

#ifdef _X360
	CTFFooter		*m_pFooter;
#else
	CTFButton		*m_pBack;
	CTFButton		*m_pOK;
#endif

	float			m_flActionThink;
	int				m_iAction;

	CUtlVector< CVideoCaption* > m_Captions;
	int				m_iCurrentCaption;
	float			m_flVideoStartTime;
};


#endif // TF_INTROMENU_H
