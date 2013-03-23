//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "emule.h"
#include "PreferencesDlg.h"
#include "emuleDlg.h" //>>> WiZaRd::Automatic Restart

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
#define PAGE_COUNT	11
#else
#define PAGE_COUNT	10
#endif

bool CPreferencesDlg::m_bRestartApp = false; //>>> WiZaRd::Automatic Restart

IMPLEMENT_DYNAMIC(CPreferencesDlg, CTreePropSheet)
BEGIN_MESSAGE_MAP(CPreferencesDlg, CTreePropSheet)
    ON_WM_DESTROY()
    ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPreferencesDlg::CPreferencesDlg()
{
    m_psh.dwFlags &= ~PSH_HASHELP;
    m_wndGeneral.m_psp.dwFlags &= ~PSH_HASHELP;
    m_wndDisplay.m_psp.dwFlags &= ~PSH_HASHELP;
    m_wndConnection.m_psp.dwFlags &= ~PSH_HASHELP;
    m_wndDirectories.m_psp.dwFlags &= ~PSH_HASHELP;
    m_wndFiles.m_psp.dwFlags &= ~PSH_HASHELP;
    m_wndStats.m_psp.dwFlags &= ~PSH_HASHELP;
    m_wndTweaks.m_psp.dwFlags &= ~PSH_HASHELP;
    m_wndSecurity.m_psp.dwFlags &= ~PSH_HASHELP;
    m_wndProxy.m_psp.dwFlags &= ~PSH_HASHELP;
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    m_wndDebug.m_psp.dwFlags &= ~PSH_HASHELP;
#endif

    AddPage(&m_wndGeneral);
    AddPage(&m_wndDisplay);
    AddPage(&m_wndConnection);
//    if(thePrefs.IsExtControlsEnabled())
    AddPage(&m_wndProxy);
    AddPage(&m_wndDirectories);
    AddPage(&m_wndFiles);
    AddPage(&m_wndStats);
    AddPage(&m_wndSecurity);
    AddPage(&m_wndTweaks);
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    AddPage(&m_wndDebug);
#endif

    // The height of the option dialog is already too large for 640x480. To show as much as
    // possible we do not show a page caption (which is an decorative element only anyway).
    SetTreeViewMode(TRUE, GetSystemMetrics(SM_CYSCREEN) >= 600, FALSE);
    SetTreeWidth(150);

    m_pPshStartPage = NULL;
    m_bSaveIniFile = false;
}

CPreferencesDlg::~CPreferencesDlg()
{
}

void CPreferencesDlg::OnDestroy()
{
    CTreePropSheet::OnDestroy();
    if (m_bSaveIniFile)
    {
        m_bSaveIniFile = false;
//>>> WiZaRd::Optimization
        // this can take a while - "entertain" the user :)
        CWaitCursor curWait;
//<<< WiZaRd::Optimization
        thePrefs.Save();
//>>> WiZaRd::Automatic Restart
        if (CPreferencesDlg::IsRestartPlanned())
        {
            if (AfxMessageBox(GetResString(IDS_SETTINGCHANGED_RESTART), MB_YESNO | MB_ICONEXCLAMATION,0)==IDYES)
            {
                theApp.PlanRestart();
                theApp.emuledlg->OnClose();
                return;
            }
        }
//<<< WiZaRd::Automatic Restart
    }
    m_pPshStartPage = GetPage(GetActiveIndex())->m_psp.pszTemplate;
}

void CPreferencesDlg::UpdateShownPages()
{
    /*
        CPropertyPage* pages[] =
        {
            &m_wndGeneral,
            &m_wndDisplay,
            &m_wndConnection,
            &m_wndProxy,
            &m_wndDirectories,
            &m_wndFiles,
            &m_wndStats,
            &m_wndSecurity,
            &m_wndTweaks,
    #if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
            &m_wndDebug
    #endif
        };
        for(int i = 0; i < _countof(pages); ++i)
        {
            int index = GetPageIndex(pages[i]);
            if(index >= 0)
                RemovePage(index);
        }

        AddPage(&m_wndGeneral);
        AddPage(&m_wndDisplay);
        AddPage(&m_wndConnection);
        if(thePrefs.IsExtControlsEnabled())
            AddPage(&m_wndProxy);
        AddPage(&m_wndDirectories);
        AddPage(&m_wndFiles);
        AddPage(&m_wndStats);
        AddPage(&m_wndSecurity);
        AddPage(&m_wndTweaks);
    #if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
        AddPage(&m_wndDebug);
    #endif
    */
}

BOOL CPreferencesDlg::OnInitDialog()
{
    ASSERT(!m_bSaveIniFile);
    BOOL bResult = CTreePropSheet::OnInitDialog();
    InitWindowStyles(this);

    for (int i = 0; i < m_pages.GetSize(); i++)
    {
        if (GetPage(i)->m_psp.pszTemplate == m_pPshStartPage)
        {
            SetActivePage(i);
            break;
        }
    }

    Localize();
    return bResult;
}

void CPreferencesDlg::Localize()
{
    SetTitle(RemoveAmpersand(GetResString(IDS_EM_PREFS)));

    m_wndGeneral.Localize();
    m_wndDisplay.Localize();
    m_wndConnection.Localize();
    m_wndDirectories.Localize();
    m_wndFiles.Localize();
    m_wndStats.Localize();
    m_wndSecurity.Localize();
    m_wndTweaks.Localize();
    m_wndProxy.Localize();

    int c = 0;

    CTreeCtrl* pTree = GetPageTreeControl();
    if (pTree)
    {
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_PW_GENERAL)));
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_PW_DISPLAY)));
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_PW_CONNECTION)));
//        if(thePrefs.IsExtControlsEnabled())
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_PW_PROXY)));
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_PW_DIR)));
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_PW_FILES)));
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_STATSSETUPINFO)));
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_SECURITY)));
        pTree->SetItemText(GetPageTreeItem(c++), RemoveAmpersand(GetResString(IDS_PW_TWEAK)));
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
        pTree->SetItemText(GetPageTreeItem(c++), _T("Debug"));
#endif
    }

    UpdateCaption();
}

void CPreferencesDlg::OnHelp()
{
    int iCurSel = GetActiveIndex();
    if (iCurSel >= 0)
    {
        CPropertyPage* pPage = GetPage(iCurSel);
        if (pPage)
        {
            HELPINFO hi = {0};
            hi.cbSize = sizeof hi;
            hi.iContextType = HELPINFO_WINDOW;
            hi.iCtrlId = 0;
            hi.hItemHandle = pPage->m_hWnd;
            hi.dwContextId = 0;
            pPage->SendMessage(WM_HELP, 0, (LPARAM)&hi);
            return;
        }
    }

    theApp.ShowHelp(0, HELP_CONTENTS);
}

BOOL CPreferencesDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (wParam == ID_HELP)
    {
        OnHelp();
        return TRUE;
    }
    if (wParam == IDOK || wParam == ID_APPLY_NOW)
        m_bSaveIniFile = true;
    return __super::OnCommand(wParam, lParam);
}

BOOL CPreferencesDlg::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    OnHelp();
    return TRUE;
}

void CPreferencesDlg::SetStartPage(UINT uStartPageID)
{
    m_pPshStartPage = MAKEINTRESOURCE(uStartPageID);
}
