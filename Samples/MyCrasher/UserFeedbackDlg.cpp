#include "StdAfx.h"
#include "MyCrasher.h"
#include "UserFeedbackDlg.h"

static const ULONGLONG MAX_ATTACHMENT_SIZE = 10ULL * 1024 * 1024; // 10 MB

CUserFeedbackDlg::CUserFeedbackDlg(CWnd* pParent)
	: CDialog(CUserFeedbackDlg::IDD, pParent)
{
}

void CUserFeedbackDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_FEEDBACK_TITLE, m_strTitle);
	DDX_Text(pDX, IDC_EDIT_FEEDBACK_DESCRIPTION, m_strDescription);
}

BEGIN_MESSAGE_MAP(CUserFeedbackDlg, CDialog)
	ON_BN_CLICKED(IDC_BROWSE_ATTACHMENT, OnBrowseAttachment)
END_MESSAGE_MAP()

void CUserFeedbackDlg::OnBrowseAttachment()
{
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, L"All Files (*.*)|*.*||", this);
	if (fileDlg.DoModal() == IDOK)
	{
		CString path = fileDlg.GetPathName();

		// Check file size
		WIN32_FILE_ATTRIBUTE_DATA fileInfo;
		if (GetFileAttributesExW(path, GetFileExInfoStandard, &fileInfo))
		{
			ULONGLONG fileSize = ((ULONGLONG)fileInfo.nFileSizeHigh << 32) | fileInfo.nFileSizeLow;
			if (fileSize > MAX_ATTACHMENT_SIZE)
			{
				MessageBox(L"File must be less than 10 MB.", L"File Too Large", MB_OK | MB_ICONWARNING);
				return;
			}
		}

		m_strAttachmentPath = path;

		// Show just the filename in the label
		CString fileName = fileDlg.GetFileName();
		GetDlgItem(IDC_STATIC_ATTACHMENT)->SetWindowText(fileName);
	}
}
