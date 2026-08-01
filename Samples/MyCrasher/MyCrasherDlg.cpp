// MyCrasherDlg.cpp : This test application is an example of how to integrate the BugSplat library. 
// This application has serveral dialog buttons all hooked up to different ways to crash a native application.
// Select a button, get a crash, send it to BugSplat, then look at the BugSplat website to see the record of your activity.
//

#include "StdAfx.h"
#include "shlwapi.h"
#include "MyCrasher.h"
#include "MyCrasherDlg.h"
#include "UserFeedbackDlg.h"
#include <vector>
#include <fpieee.h>
#include <excpt.h>
#include <float.h>
#include <eh.h>

/////////////////////////////////////////////////////////////////////////////
// CMyCrasherDlg dialog

CMyCrasherDlg::CMyCrasherDlg(BugSplat* bugsplat, CWnd* pParent /*=NULL*/)
	: CDialog(CMyCrasherDlg::IDD, pParent)
	, g_BugSplat(bugsplat)
{
	//{{AFX_DATA_INIT(CMyCrasherDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMyCrasherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyCrasherDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_PROBLEMCOMBO, m_cbProblem);
}

BEGIN_MESSAGE_MAP(CMyCrasherDlg, CDialog)
	//{{AFX_MSG_MAP(CMyCrasherDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SIMILATE_PROBLEM, OnSimulateProblem)
	ON_BN_CLICKED(IDC_CREATE_REPORT, OnCreateReport)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_PROBLEMCOMBO, OnCbnSelchangeProblemcombo)
	ON_BN_CLICKED(IDC_SENDADDITIONALFILES, OnBnClickedSendadditionalfiles)
	ON_BN_CLICKED(IDC_CHK_ENABLEHANGDETECT, OnBnClickedChkEnableHangDetect)
	ON_BN_CLICKED(IDC_USERFEEDBACK, OnBnClickedUserFeedback)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyCrasherDlg message handlers

BOOL CMyCrasherDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// lets populate the dialog
	CString str;
	str.LoadString(IDS_SELECTPROBLEM);
	m_cbProblem.AddString(str);

	str.LoadString(IDS_PROBLEM_MEM_EXCEPTION);  m_cbProblem.AddString(str);
	str.LoadString(IDS_PROBLEM_STACK_OVERFLOW); m_cbProblem.AddString(str);
	str.LoadString(IDS_PROBLEM_DIVBYZERO);      m_cbProblem.AddString(str);
	str.LoadString(IDS_PROBLEM_ITERATION_LOCK); m_cbProblem.AddString(str);
	str.LoadString(IDS_PROBLEM_ABORT);          m_cbProblem.AddString(str);

	m_cbProblem.SetCurSel(0);
	OnCbnSelchangeProblemcombo();

	CheckDlgButton(IDC_CHK_ENABLEHANGDETECT, BST_CHECKED);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMyCrasherDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMyCrasherDlg::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}


void CMyCrasherDlg::OnCreateReport()
{
	const __wchar_t* xml = L"<report><process>"
		"<exception>"
		"<code>FATAL ERROR</code>"
		"<explanation>This is an error code explanation</explanation>"
		"<func><![CDATA[MyConsoleCrasher!MemoryException]]></func>"
		"<file>/www/bugsplatAutomation/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>143</line>"
		"<registers>"
		"<cs>0023</cs>"
		"<ds>002b</ds>"
		"<eax>00000011</eax>"
		"<ebp>00affb58</ebp>"
		"<ebx>00858000</ebx>"
		"<ecx>43bf1e0e</ecx>"
		"<edi>00affb58</edi>"
		"<edx>014480b4</edx>"
		"<efl>00010202</efl>"
		"</registers>"
		"</exception>"
		"<modules numloaded=\"2\">"
		"<module>"
		"<name>MyConsoleCrasher</name>"
		"<order>1</order>"
		"<address>01320000-01457000</address>"
		"<path>C:/www/BugsplatAutomation/BugsplatAutomation/bin/x64/Release/temp/BugSplat/bin/MyConsoleCrasher.exe</path>"
		"<symbolsloaded>deferred</symbolsloaded>"
		"<fileversion/>"
		"<productversion/>"
		"<checksum>00000000</checksum>"
		"<timedatestamp>SatJun1501:18:092019</timedatestamp>"
		"</module>"
		"<module>"
		"<name>BugSplatRc</name>"
		"<order>2</order>"
		"<address>01320000-01457000</address>"
		"<path>C:/www/BugsplatAutomation/BugsplatAutomation/bin/x64/Release/BugSplatRc.dll</path>"
		"<symbolsloaded>deferred</symbolsloaded>"
		"<fileversion/>"
		"<productversion/>"
		"<checksum>00000000</checksum>"
		"<timedatestamp>SatJun1501:18:092019</timedatestamp>"
		"</module>"
		"</modules>"
		"<threads count=\"2\">"
		"<thread id=\"0\" current=\"yes\" event=\"yes\" framecount=\"3\">"
		"<frame>"
		"<symbol><![CDATA[MyConsoleCrasher!MemoryException]]></symbol>"
		"<file>/www/bugsplatAutomation/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>143</line>"
		"<offset>0x35</offset>"
		"</frame>"
		"<frame>"
		"<symbol><![CDATA[MyConsoleCrasher!wmain]]></symbol>"
		"<file>C:/www/BugsplatAutomation/BugsplatAutomation/BugSplat/samples/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>83</line>"
		"<offset>0x239</offset>"
		"</frame>"
		"<frame>"
		"<symbol><![CDATA[MyConsoleCrasher!__scrt_wide_environment_policy::initialize_environment]]></symbol>"
		"<file>d:/agent/_work/4/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl</file>"
		"<line>90</line>"
		"<offset>0x43</offset>"
		"</frame>"
		"</thread>"
		"<thread id=\"1\" current=\"no\" event=\"no\" framecount=\"3\">"
		"<frame>"
		"<symbol><![CDATA[my2ConsoleCrasher!MemoryException]]></symbol>"
		"<file>/www/bugsplatAutomation/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>143</line>"
		"<offset>0x35</offset>"
		"</frame>"
		"<frame>"
		"<symbol><![CDATA[my2ConsoleCrasher!wmain]]></symbol>"
		"<file>C:/www/BugsplatAutomation/BugsplatAutomation/BugSplat/samples/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>83</line>"
		"<offset>0x239</offset>"
		"</frame>"
		"<frame>"
		"<symbol><![CDATA[my2ConsoleCrasher!__scrt_wide_environment_policy::initialize_environment]]></symbol>"
		"<file>d:/agent/_work/4/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl</file>"
		"<line>90</line>"
		"<offset>0x43</offset>"
		"</frame>"
		"</thread>"
		"</threads></process></report>";	
	
	g_BugSplat->CreateXmlReport(xml);
}

// ******************************************************************************
// When compiling in release the contents of some of our sample 'crashing' 
// methods  get optimized out... this pragma disables the optimization
// so that we can get a crash. 
// ******************************************************************************
#pragma optimize( "", off)

#pragma warning (disable : 4717)
#pragma warning (disable : 4748)
void StackOverflow(int depth)
{
	char blockdata[10000];
#if _MSC_VER < 1400
	// VC 7.1 or earlier...
	sprintf(blockdata, "Overflow: %d\n", depth);
#else
	// VC 8 or later
	sprintf_s(blockdata, 10000, "Overflow: %d\n", depth);
#endif

	StackOverflow(depth + 1);
}
#pragma warning (default : 4717)
#pragma warning (default: 4748)

void GenerateMemoryException()
{
	// Dereference null pointer
	*(int*)0 = 0;
}

void GenerateStackOverflow()
{
	StackOverflow(0);
}

void GenerateDivideByZero()
{
	int x, y;
	x = 5;
	y = 0;
	int nRes = x / y;
}

void GenerateApplicationHang()
{
	// Infinite loop that causes hang
	for (int i = 0; i < 1000; i++)
	{
		if (i == 10)
			i = 0;
		Sleep(10);
	}
}

void GenerateAbort()
{
	abort();
}

void CMyCrasherDlg::OnSimulateProblem()
{
	int nIndex = m_cbProblem.GetCurSel();

	switch (nIndex)
	{
	case 1:// generate Memory Exception			
		GenerateMemoryException();
		break;

	case 2:// generate Stack Overflow
		GenerateStackOverflow();
		break;

	case 3:// generate Int DivByZero error
		GenerateDivideByZero();
		break;

	case 4://generate an application hang
		GenerateApplicationHang();
		break;

	case 5: //generate abort
		GenerateAbort();
		break;
	}
}

#pragma optimize( "", on)

void CMyCrasherDlg::OnBnClickedSendadditionalfiles()
{
	CButton* pBtn = (CButton*)GetDlgItem(IDC_SENDADDITIONALFILES);
	if (pBtn->GetCheck() == 0) return;

	// Create some files in the %temp% directory and attach them
	wchar_t filePath[MAX_PATH];
	wchar_t tempPath[MAX_PATH];
	GetTempPathW(MAX_PATH, tempPath);

	// Create first file
	swprintf_s(filePath, MAX_PATH, L"%sfile1.txt", tempPath);
	FILE* pFile1 = nullptr;
	if (_wfopen_s(&pFile1, filePath, L"w") == 0 && pFile1 != nullptr)
	{
		fwprintf(pFile1, L"Exception Code = 0x12345\n");
		fclose(pFile1);
		g_BugSplat->AddAttachment(filePath);
	}

	// Create second file
	GetTempPathW(MAX_PATH, tempPath);
	swprintf_s(filePath, MAX_PATH, L"%sfile2.txt", tempPath);
	FILE* pFile2 = nullptr;
	if (_wfopen_s(&pFile2, filePath, L"w") == 0 && pFile2 != nullptr)
	{
		const wchar_t* crashFolder = g_BugSplat->GetCrashFolder();
		fwprintf(pFile2, L"Crash reporting is so clutch!  minidump path = %s\n", crashFolder);
		fclose(pFile2);
		g_BugSplat->AddAttachment(filePath);
	}
}


void CMyCrasherDlg::OnCbnSelchangeProblemcombo()
{
	if (!::IsWindow(m_hWnd))
		return;

	int nIndex = m_cbProblem.GetCurSel();
	UINT nResId = -1;
	CString str;
	switch (nIndex)
	{
	case 1:
		str.LoadString(IDS_PROBLEM_MEM_EXCEPTION_DESC);
		break;
	case 2:
		str.LoadString(IDS_PROBLEM_STACK_OVERFLOW_DESC);
		break;
	case 3:
		str.LoadString(IDS_PROBLEM_DIVBYZERO_DESC);
		break;
	case 4:
		str.LoadString(IDS_PROBLEM_ITERATION_LOCK_DESC);
		break;
	default:
		str.Empty();
		break;

	}

	GetDlgItem(IDC_STATIC_DESCRIPTION)->SetWindowText(str);
	GetDlgItem(IDC_SIMILATE_PROBLEM)->EnableWindow(nIndex != 0);
}


void CMyCrasherDlg::OnBnClickedChkEnableHangDetect()
{
	CButton* pBtn = (CButton*)GetDlgItem(IDC_CHK_ENABLEHANGDETECT);
	if (pBtn->GetCheck() == 0)
	{
		g_BugSplat->SetHangDetectionTimeout(0); // Disable hang detection
	}
	else
	{
		g_BugSplat->SetHangDetectionTimeout(5000); // Enable hang detection, timeout after 5 seconds
	}
}


void CMyCrasherDlg::OnBnClickedUserFeedback()
{
	CUserFeedbackDlg dlg(this);

	if (dlg.DoModal() == IDOK)
	{
		if (dlg.m_strAttachmentPath.IsEmpty())
		{
			g_BugSplat->PostFeedback(dlg.m_strTitle, dlg.m_strDescription);
		}
		else
		{
			std::vector<const wchar_t*> attachments = { (LPCTSTR)dlg.m_strAttachmentPath };
			g_BugSplat->PostFeedback(dlg.m_strTitle, dlg.m_strDescription, attachments);
		}

		MessageBox(L"User feedback sent to BugSplat!", L"Feedback Sent", MB_OK | MB_ICONINFORMATION);
	}
}
