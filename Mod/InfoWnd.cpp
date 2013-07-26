//this file is part of kMule
//Copyright (C)2012 WiZaRd ( strEmail.Format("%s@%s", "thewizardofdos", "gmail.com") / http://www.emulefuture.de )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "InfoWnd.h"
#include "emule.h"
#include "Preferences.h"
#include "HTRichEditCtrl.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "MuleStatusBarCtrl.h"
#include "NetworkInfoDlg.h"
// #include "HelpIDs.h"
 
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SZ_DEBUG_LOG_TITLE			L"Verbose"

// CInfoWnd dialog

IMPLEMENT_DYNAMIC(CInfoWnd, CDialog)

BEGIN_MESSAGE_MAP(CInfoWnd, CResizableDialog)
	ON_BN_CLICKED(IDC_LOGRESET, OnBnClickedResetLog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB3, OnTcnSelchangeTab3)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_CTLCOLOR()
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CInfoWnd::CInfoWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CInfoWnd::IDD, pParent)
{
	logbox = new CHTRichEditCtrl;
	analyzerLog = new CHTRichEditCtrl; //>>> WiZaRd::ClientAnalyzer
	debuglog = new CHTRichEditCtrl;
	memset(&m_cfDef, 0, sizeof m_cfDef);
	memset(&m_cfBold, 0, sizeof m_cfBold);
	StatusSelector.m_bCloseable = false;
}

CInfoWnd::~CInfoWnd()
{
	delete debuglog;
	delete analyzerLog; //>>> WiZaRd::ClientAnalyzer
	delete logbox;
}

BOOL CInfoWnd::OnInitDialog()
{
	if (theApp.m_fontLog.m_hObject == NULL)
	{
		CFont* pFont = GetDlgItem(IDC_LOGRESET)->GetFont();
		LOGFONT lf;
		pFont->GetObject(sizeof lf, &lf);
		theApp.m_fontLog.CreateFontIndirect(&lf);
	}

	ReplaceRichEditCtrl(GetDlgItem(IDC_MYINFOLIST), this, GetDlgItem(IDC_LOGRESET)->GetFont());
	CResizableDialog::OnInitDialog();

	// using ES_NOHIDESEL is actually not needed, but it helps to get around a tricky window update problem!
	// If that style is not specified there are troubles with right clicking into the control for the very first time!?
#define	LOG_PANE_RICHEDIT_STYLES WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL
	CRect rect;

	GetDlgItem(IDC_LOGBOX)->GetWindowRect(rect);
	GetDlgItem(IDC_LOGBOX)->DestroyWindow();
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	if (logbox->Create(LOG_PANE_RICHEDIT_STYLES, rect, this, IDC_LOGBOX)){
		logbox->SetProfileSkinKey(L"Log");
		logbox->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		logbox->SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		if (theApp.m_fontLog.m_hObject)
			logbox->SetFont(&theApp.m_fontLog);
		logbox->ApplySkin();
		logbox->SetTitle(GetResString(IDS_SV_LOG));
		logbox->SetAutoURLDetect(FALSE);
	}

//>>> WiZaRd::ClientAnalyzer
	GetDlgItem(IDC_ANALYZER_LOG)->GetWindowRect(rect);
	GetDlgItem(IDC_ANALYZER_LOG)->DestroyWindow();
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	if (analyzerLog->Create(LOG_PANE_RICHEDIT_STYLES, rect, this, IDC_ANALYZER_LOG))
	{
		analyzerLog->SetProfileSkinKey(L"Analyzer");
		analyzerLog->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		analyzerLog->SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		if (theApp.m_fontLog.m_hObject)
			analyzerLog->SetFont(&theApp.m_fontLog);
		analyzerLog->ApplySkin();
		analyzerLog->SetTitle(L"Analyzer"); // GetResString(IDS_SV_CA)
		analyzerLog->SetAutoURLDetect(FALSE);
	}
//<<< WiZaRd::ClientAnalyzer

	GetDlgItem(IDC_DEBUG_LOG)->GetWindowRect(rect);
	GetDlgItem(IDC_DEBUG_LOG)->DestroyWindow();
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	if (debuglog->Create(LOG_PANE_RICHEDIT_STYLES, rect, this, IDC_DEBUG_LOG)){
		debuglog->SetProfileSkinKey(L"VerboseLog");
		debuglog->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		debuglog->SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		if (theApp.m_fontLog.m_hObject)
			debuglog->SetFont(&theApp.m_fontLog);
		debuglog->ApplySkin();
		debuglog->SetTitle(SZ_DEBUG_LOG_TITLE);
		debuglog->SetAutoURLDetect(FALSE);
	}

	SetAllIcons();
	Localize();

	TCITEM newitem;
	CString name;
	name = GetResString(IDS_SV_LOG);
	name.Replace(L"&", L"&&");
	newitem.mask = TCIF_TEXT | TCIF_IMAGE;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	newitem.iImage = 0;
	VERIFY( StatusSelector.InsertItem(StatusSelector.GetItemCount(), &newitem) == PaneLog );

//>>> WiZaRd::ClientAnalyzer
	name = L"Analyzer"; // GetResString(IDS_SV_LOG);
	name.Replace(L"&", L"&&");
	newitem.mask = TCIF_TEXT | TCIF_IMAGE;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	newitem.iImage = 1;
	VERIFY( StatusSelector.InsertItem(StatusSelector.GetItemCount(), &newitem) == PaneCA );
//<<< WiZaRd::ClientAnalyzer

	name = SZ_DEBUG_LOG_TITLE;
	name.Replace(L"&", L"&&");
	newitem.mask = TCIF_TEXT | TCIF_IMAGE;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	newitem.iImage = 0;
	VERIFY( StatusSelector.InsertItem(StatusSelector.GetItemCount(), &newitem) == PaneVerboseLog );

	AddAnchor(m_ctrlMyInfoFrm, TOP_RIGHT, BOTTOM_RIGHT);
	AddAnchor(m_MyInfo, TOP_RIGHT, BOTTOM_RIGHT);
	AddAnchor(StatusSelector, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGRESET, TOP_RIGHT); // avoid resizing GUI glitches with the tab control by adding this control as the last one (Z-order)
	// The resizing of those log controls (rich edit controls) works 'better' when added as last anchors (?)
	AddAnchor(*logbox, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*analyzerLog, TOP_LEFT, BOTTOM_RIGHT); //>>> WiZaRd::ClientAnalyzer
	AddAnchor(*debuglog, TOP_LEFT, BOTTOM_RIGHT);

	// Set the tab control to the bottom of the z-order. This solves a lot of strange repainting problems with
	// the rich edit controls (the log panes).
	::SetWindowPos(StatusSelector, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOSIZE);

	debug = true;
	ToggleDebugWindow();
//>>> WiZaRd::ClientAnalyzer
	analyzer = true;
	ToggleAnalyzerWindow(); 
//<<< WiZaRd::ClientAnalyzer

	debuglog->ShowWindow(SW_HIDE);
	analyzerLog->ShowWindow(SW_HIDE); //>>> WiZaRd::ClientAnalyzer
	logbox->ShowWindow(SW_SHOW);

	// optional: restore last used log pane
	/*if (thePrefs.GetRestoreLastLogPane())
	{
		if (thePrefs.GetLastLogPaneID() >= 0 && thePrefs.GetLastLogPaneID() < StatusSelector.GetItemCount())
		{
			int iCurSel = StatusSelector.GetCurSel();
			StatusSelector.SetCurSel(thePrefs.GetLastLogPaneID());
			if (thePrefs.GetLastLogPaneID() == StatusSelector.GetCurSel())
				UpdateLogTabSelection();
			else
				StatusSelector.SetCurSel(iCurSel);
		}
	}*/

	m_MyInfo.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
	m_MyInfo.SetAutoURLDetect();
	m_MyInfo.SetEventMask(m_MyInfo.GetEventMask() | ENM_LINK);

	PARAFORMAT pf = {0};
	pf.cbSize = sizeof pf;
	if (m_MyInfo.GetParaFormat(pf)){
		pf.dwMask |= PFM_TABSTOPS;
		pf.cTabCount = 4;
		pf.rgxTabs[0] = 900;
		pf.rgxTabs[1] = 1000;
		pf.rgxTabs[2] = 1100;
		pf.rgxTabs[3] = 1200;
		m_MyInfo.SetParaFormat(pf);
	}

	m_cfDef.cbSize = sizeof m_cfDef;
	if (m_MyInfo.GetSelectionCharFormat(m_cfDef)){
		m_cfBold = m_cfDef;
		m_cfBold.dwMask |= CFM_BOLD;
		m_cfBold.dwEffects |= CFE_BOLD;
	}
	
	InitWindowStyles(this);

	return true;
}

void CInfoWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MYINFO, m_ctrlMyInfoFrm);
	DDX_Control(pDX, IDC_TAB3, StatusSelector);
	DDX_Control(pDX, IDC_MYINFOLIST, m_MyInfo);
}

void CInfoWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CInfoWnd::SetAllIcons()
{
	m_ctrlMyInfoFrm.SetIcon(L"Info");

	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(L"Log"));
	iml.Add(CTempIconLoader(L"BADGUY"));
	StatusSelector.SetImageList(&iml);
	m_imlLogPanes.DeleteImageList();
	m_imlLogPanes.Attach(iml.Detach());
}

void CInfoWnd::Localize()
{
	GetDlgItem(IDC_LOGRESET)->SetWindowText(GetResString(IDS_PW_RESET));
	m_ctrlMyInfoFrm.SetWindowText(GetResString(IDS_INFO)); //GetResString(IDS_MYINFO));

	TCITEM item;
	CString name;

	name = GetResString(IDS_SV_LOG);
	name.Replace(L"&", L"&&");
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	StatusSelector.SetItem(PaneLog, &item);

//>>> WiZaRd::ClientAnalyzer
	name = L"Analyzer";
	name.Replace(L"&", L"&&");
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	StatusSelector.SetItem(PaneCA, &item);
//<<< WiZaRd::ClientAnalyzer

	name = SZ_DEBUG_LOG_TITLE;
	name.Replace(L"&", L"&&");
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	StatusSelector.SetItem(PaneVerboseLog, &item);

	UpdateLogTabSelection();
}

void CInfoWnd::OnBnClickedResetLog()
{
	int cur_sel = StatusSelector.GetCurSel();
	if (cur_sel == -1)
		return;

	theApp.emuledlg->statusbar->SetText(L"", SBarLog, 0);
	if (cur_sel == PaneVerboseLog)
		ResetDebugLog();
	else if(cur_sel == PaneCA)
		ResetAnalyzerLog();
	else /*if (cur_sel == PaneLog)*/
		ResetLog();
}

void CInfoWnd::OnTcnSelchangeTab3(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	UpdateLogTabSelection();
	*pResult = 0;
}

void CInfoWnd::UpdateLogTabSelection()
{
	int cur_sel = StatusSelector.GetCurSel();
	if (cur_sel == -1)
		return;

	CHTRichEditCtrl* show = NULL;
	if (cur_sel == PaneVerboseLog)
		show = debuglog;
	else if (cur_sel == PaneLog)
		show = logbox;
//>>> WiZaRd::ClientAnalyzer
	else if (cur_sel == PaneCA)
		show = analyzerLog;
//<<< WiZaRd::ClientAnalyzer
	else
	{
		ASSERT(0);
		return; 
	}

	analyzerLog->ShowWindow(SW_HIDE); //>>> WiZaRd::ClientAnalyzer
	logbox->ShowWindow(SW_HIDE);
	debuglog->ShowWindow(SW_HIDE);
	show->ShowWindow(SW_SHOW);
	if (show->IsAutoScroll() && (StatusSelector.GetItemState(cur_sel, TCIS_HIGHLIGHTED) & TCIS_HIGHLIGHTED))
		show->ScrollToLastLine(true);
	show->Invalidate();
	StatusSelector.HighlightItem(cur_sel, FALSE);
}

void CInfoWnd::ToggleDebugWindow()
{
	int cur_sel = StatusSelector.GetCurSel();
	if (thePrefs.GetVerbose() && !debug)
	{
		TCITEM newitem;
		CString name = SZ_DEBUG_LOG_TITLE;
		name.Replace(L"&", L"&&");
		newitem.mask = TCIF_TEXT | TCIF_IMAGE;
		newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		newitem.iImage = 0;
		StatusSelector.InsertItem(PaneVerboseLog, &newitem);
		debug = true;
	}
	else if (!thePrefs.GetVerbose() && debug)
	{
		if (cur_sel == PaneVerboseLog)
		{
			StatusSelector.SetCurSel(PaneLog);
			StatusSelector.SetFocus();
		}
		debuglog->ShowWindow(SW_HIDE);
		analyzerLog->ShowWindow(SW_HIDE); //>>> WiZaRd::ClientAnalyzer
		logbox->ShowWindow(SW_SHOW);
		StatusSelector.DeleteItem(PaneVerboseLog);
		debug = false;
	}
}

//>>> WiZaRd::ClientAnalyzer
void CInfoWnd::ToggleAnalyzerWindow()
{
	int cur_sel = StatusSelector.GetCurSel();
	if (thePrefs.GetLogAnalyzerEvents() && !analyzer)
	{
		TCITEM newitem;
		CString name = L"Analyzer"; // GetResString(IDS_SV_LOG);;
		name.Replace(L"&", L"&&");
		newitem.mask = TCIF_TEXT | TCIF_IMAGE;
		newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		newitem.iImage = 1;
		StatusSelector.InsertItem(PaneCA, &newitem);
		analyzer = true;
	}
	else if (!thePrefs.GetLogAnalyzerEvents() && analyzer)
	{
		if (cur_sel == PaneCA)
		{
			StatusSelector.SetCurSel(PaneLog);
			StatusSelector.SetFocus();
		}
		debuglog->ShowWindow(SW_HIDE);
		analyzerLog->ShowWindow(SW_HIDE); //>>> WiZaRd::ClientAnalyzer
		logbox->ShowWindow(SW_SHOW);
		StatusSelector.DeleteItem(PaneCA);
		analyzer = false;
	}
}
//<<< WiZaRd::ClientAnalyzer

void CInfoWnd::UpdateMyInfo()
{
	m_MyInfo.SetRedraw(FALSE);
	m_MyInfo.SetWindowText(L"");
	CreateNetworkInfo(m_MyInfo, m_cfDef, m_cfBold, true);
	m_MyInfo.SetRedraw(TRUE);
	m_MyInfo.Invalidate();
}

CString CInfoWnd::GetMyInfoString() 
{
	CString buffer;
	m_MyInfo.GetWindowText(buffer);

	return buffer;
}

void CInfoWnd::ShowNetworkInfo()
{
	CNetworkInfoDlg dlg;
	dlg.DoModal();
}

void CInfoWnd::SaveAllSettings()
{
	//thePrefs.SetLastLogPaneID(StatusSelector.GetCurSel());
}

BOOL CInfoWnd::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	//theApp.ShowHelp(eMule_FAQ_GUI_Log);
	return TRUE;
}

HBRUSH CInfoWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = theApp.emuledlg->GetCtlColor(pDC, pWnd, nCtlColor);
	if (hbr)
		return hbr;
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CInfoWnd::ResetLog()
{
	logbox->Reset();
}

//>>> WiZaRd::ClientAnalyzer
void CInfoWnd::ResetAnalyzerLog()
{
	analyzerLog->Reset();
}
//<<< WiZaRd::ClientAnalyzer

void CInfoWnd::ResetDebugLog()
{
	debuglog->Reset();
}

//>>> WiZaRd::InfoWnd
void CemuleDlg::ApplyLogFont(LPLOGFONT plf)
{
	theApp.m_fontLog.DeleteObject();
	if (theApp.m_fontLog.CreateFontIndirect(plf))
	{
		thePrefs.SetLogFont(plf);
		infoWnd->logbox->SetFont(&theApp.m_fontLog);
		infoWnd->analyzerLog->SetFont(&theApp.m_fontLog); //>>> WiZaRd::ClientAnalyzer
		infoWnd->debuglog->SetFont(&theApp.m_fontLog);
	}
}
//<<< WiZaRd::InfoWnd