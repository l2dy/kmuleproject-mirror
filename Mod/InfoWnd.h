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
#pragma once
#include "resource.h"
#include "ResizableLib\ResizableDialog.h"
#include "RichEditCtrlX.h"
#include "IconStatic.h"
#include "ClosableTabCtrl.h"

class CHTRichEditCtrl;

class CInfoWnd : public CResizableDialog
{
    DECLARE_DYNAMIC(CInfoWnd)

public:
    CInfoWnd(CWnd* pParent = NULL);   // standard constructor
    virtual ~CInfoWnd();

    void	Localize();
    void	ToggleDebugWindow();
    void	ToggleAnalyzerWindow(); //>>> WiZaRd::ClientAnalyzer
    void	UpdateMyInfo();
    void	UpdateLogTabSelection();
    void	SaveAllSettings();
    void	ShowNetworkInfo();
    CString GetMyInfoString();

// Dialog Data
    enum { IDD = IDD_INFO };

    enum ELogPaneItems
    {
        PaneLog			= 0, // those are CTabCtrl item indices
        PaneCA			= 1, //>>> WiZaRd::ClientAnalyzer
        PaneVerboseLog	= 2
    };

    CHTRichEditCtrl* logbox;
    CHTRichEditCtrl* analyzerLog; //>>> WiZaRd::ClientAnalyzer
    CHTRichEditCtrl* debuglog;
    CClosableTabCtrl StatusSelector;

private:
    CIconStatic m_ctrlMyInfoFrm;
    CImageList m_imlLogPanes;
    bool	debug;
    bool	analyzer; //>>> WiZaRd::ClientAnalyzer
    CRichEditCtrlX m_MyInfo;
    CHARFORMAT m_cfDef;
    CHARFORMAT m_cfBold;

    void	ResetLog();
    void	ResetAnalyzerLog(); //>>> WiZaRd::ClientAnalyzer
    void	ResetDebugLog();

protected:
    void	SetAllIcons();

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()
    afx_msg void OnBnClickedResetLog();
    afx_msg void OnBnConnect();
    afx_msg void OnTcnSelchangeTab3(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSysColorChange();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};