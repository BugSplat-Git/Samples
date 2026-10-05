#pragma once

#include "afxwin.h"
#include "Resource.h"

class CUserFeedbackDlg : public CDialog
{
public:
	CUserFeedbackDlg(CWnd* pParent = NULL);
	enum { IDD = IDD_USERFEEDBACK_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	DECLARE_MESSAGE_MAP()

	afx_msg void OnBrowseAttachment();

public:
	CString m_strTitle;
	CString m_strDescription;
	CString m_strAttachmentPath;
};
