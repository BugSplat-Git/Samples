#include "StdAfx.h"
#include "MyCrasher.h"
#include "MyCrasherDlg.h"

// The complete BugSplat getting started guide is available at https://docs.bugsplat.com
BugSplat g_BugSplat(BUGSPLAT_DATABASE, APPLICATION_NAME, APPLICATION_VERSION);

BEGIN_MESSAGE_MAP(CMyCrasherApp, CWinApp)
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

// CMyCrasherApp construction
CMyCrasherApp::CMyCrasherApp()
{
    // The VS debugger takes precedence over BugSplat's exception handling
    if (IsDebuggerPresent()) 
    {
        MessageBox(NULL, L"Run this application without the debugger to allow BugSplat exception handling", L"Information", MB_OK);
        exit(0);
    }


	// Use to set the default user name.  Useful for quiet-mode applications that don't prompt for user/email/description at crash time.
	g_BugSplat.SetUser(L"Fred");

	// Use to set the default user email.  Useful for quiet-mode applications that don't prompt for user/email/description at crash time.
	g_BugSplat.SetEmail(L"fred@bugsplat.com");

	// Use to set the default user description.  Useful for quiet-mode applications that don't prompt for user/email/description at crash time.
	g_BugSplat.SetUserDescription(L"I was stabbing the Orc in the right eye with my enchanted lance");
}

// The one and only CMyCrasherApp object
CMyCrasherApp theApp;

// CMyCrasherApp initialization
BOOL CMyCrasherApp::InitInstance()
{
    CCommandLineInfoEx cmdInfo;
    ParseCommandLine(cmdInfo); 
    
    // Force a crash if the crash option is specified
    if (cmdInfo.GetOption((CString)"crash")) {
        g_BugSplat.SetQuietMode(true);  // Don't let the BugSplat dialog appear
        *(int *) 0 = 0; // Generate a Memory Exception
    } 
    else if (cmdInfo.GetOption((CString)"crash2")) {
        *(int *) 0 = 0; // Generate a Memory Exception
    }

    // display main dialog
    CMyCrasherDlg dlg(&g_BugSplat);
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();

    // Since the dialog has been closed, return FALSE so that we exit the
    // application, rather than start the application's message pump.
    return FALSE;
}
