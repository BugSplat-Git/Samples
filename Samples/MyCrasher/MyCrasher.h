// MyCrasher.h : main header file for the MYCRASHER application
//

// Define your BugSplat database, application name, and application version by updating the lines below
#define BUGSPLAT_DATABASE L"Fred"
#define APPLICATION_NAME L"MyCrasher"
#define APPLICATION_VERSION L"7.0.0"

#ifndef BUGSPLAT_DATABASE
#error "Please define a BugSplat database"
#endif

#if !defined(AFX_MYCRASHER_H__481164BF_62A8_4E7D_B46A_1F94612AE2F6__INCLUDED_)
#define AFX_MYCRASHER_H__481164BF_62A8_4E7D_B46A_1F94612AE2F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "Resource.h"		// main symbols

#include "CmdLine.h"
#include "BugSplat.h"

//BugSplat g_BugSplat;

/////////////////////////////////////////////////////////////////////////////
// CMyCrasherApp:
// See MyCrasher.cpp for the implementation of this class
//

class CMyCrasherApp : public CWinApp
{
public:
	CMyCrasherApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyCrasherApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMyCrasherApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MYCRASHER_H__481164BF_62A8_4E7D_B46A_1F94612AE2F6__INCLUDED_)
