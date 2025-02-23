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
#include "TrayDialog.h"
#include "emuledlg.h"
#include "MenuCmds.h"
#include "UserMsgs.h"
#if (_WIN32_IE >= 0x600)
#include "Preferences.h"
#include "otherfunctions.h"
#include "opcodes.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	NOTIFYICONDATA_V1_TIP_SIZE	64
#define NOTIFYICONDATA_V2_TIP_SIZE 128

DWORD GetDllVersion(LPCTSTR lpszDllName)
{
    DWORD dwVersion = 0;

    HINSTANCE hinstDll = LoadLibrary(lpszDllName);
    if (hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
        if (pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            HRESULT hr = (*pDllGetVersion)(&dvi);
            if (SUCCEEDED(hr))
                dwVersion = MAKELONG(dvi.dwMajorVersion, dvi.dwMinorVersion);
        }
        FreeLibrary(hinstDll);
    }

    return dwVersion;
}

DWORD GetShellVersion()
//bool SupportExtendedToolTips()
{
    //see ms-help://MS.VSCC.2003/MS.MSDNQTR.2005JUL.1033/shellcc/platform/shell/reference/structures/notifyicondata.htm
//	static const bool bSupportsExtendedToolTips = GetDllVersion(TEXT("comctl32.dll")) >= MAKELONG(5,0);
//    static const bool bSupportsExtendedToolTips = GetDllVersion(TEXT("shell32.dll")) >= MAKELONG(5,0);
//    return bSupportsExtendedToolTips;
    static const DWORD dwShellVersion = GetDllVersion(TEXT("shell32.dll"));
    return dwShellVersion;
}

/////////////////////////////////////////////////////////////////////////////
// CTrayDialog dialog

const UINT WM_TASKBARCREATED = ::RegisterWindowMessage(_T("TaskbarCreated"));

BEGIN_MESSAGE_MAP(CTrayDialog, CTrayDialogBase)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SYSCOMMAND()
    ON_MESSAGE(UM_TRAY_ICON_NOTIFY_MESSAGE, OnTrayNotify)
    ON_REGISTERED_MESSAGE(WM_TASKBARCREATED, OnTaskBarCreated)
    ON_WM_TIMER()
END_MESSAGE_MAP()

CTrayDialog::CTrayDialog(UINT uIDD,CWnd* pParent /*=NULL*/)
    : CTrayDialogBase(uIDD, pParent)
{
    ZeroMemory(&m_nidIconData, sizeof(NOTIFYICONDATA));
    const DWORD version = GetShellVersion();
#ifdef NOTIFYICON_VERSION_4
    if (version >= MAKEDLLVERULL(6,0,6,0))
    {
        m_nidIconData.cbSize = sizeof(NOTIFYICONDATA);
        m_nidIconData.uVersion = NOTIFYICON_VERSION_4;
    }
    else if (version >= MAKEDLLVERULL(6,0,0,0))
    {
        m_nidIconData.cbSize = NOTIFYICONDATA_V3_SIZE;
        m_nidIconData.uVersion = NOTIFYICON_VERSION_4;
    }
    else
#endif
        if (version >= MAKEDLLVERULL(5,0,0,0))
        {
            m_nidIconData.cbSize = NOTIFYICONDATA_V2_SIZE;
            m_nidIconData.uVersion = NOTIFYICON_VERSION;
        }
        else
        {
            m_nidIconData.cbSize = NOTIFYICONDATA_V1_SIZE;
            m_nidIconData.uVersion = 0;
        }
    m_nidIconData.hWnd = 0;
    m_nidIconData.uCallbackMessage = UM_TRAY_ICON_NOTIFY_MESSAGE;
    m_nidIconData.hIcon = 0;
    m_nidIconData.szTip[0] = L'\0';
    m_nidIconData.uFlags = NIF_MESSAGE;

#ifdef NOTIFYICON_VERSION_4
    // NOTIFYICON_VERSION_4 requires that the tooltip is drawn by the user
    if (m_nidIconData.uVersion == NOTIFYICON_VERSION_4)
        m_nidIconData.uFlags |= NIF_SHOWTIP;
#endif
#if (_WIN32_IE >= 0x600)
    // Windows 7 and later: A registered GUID that identifies the icon. This value overrides uID and is the recommended method of identifying the icon. The NIF_GUID flag must be set in the uFlags member.
    if (thePrefs.GetWindowsVersion() >= _WINVER_7_)
    {
        m_nidIconData.uFlags |= NIF_GUID;
        CLSIDFromString(EMULE_GUID_BASE, &m_nidIconData.guidItem);
    }
    else
#endif
        m_nidIconData.uID = 1;

    m_bTrayIconVisible = false;
    m_pbMinimizeToTray = NULL;
    m_nDefaultMenuItem = 0;
    m_hPrevIconDelete = NULL;
    m_bCurIconDelete = false;
    m_bLButtonDblClk = false;
    m_bLButtonDown = false;
    m_uSingleClickTimer = 0;
}

int CTrayDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (__super::OnCreate(lpCreateStruct) == -1)
        return -1;

    ASSERT(WM_TASKBARCREATED);
    m_nidIconData.hWnd = m_hWnd;
    m_nidIconData.uID = 1;
    return 0;
}

void CTrayDialog::OnDestroy()
{
    KillSingleClickTimer();
    __super::OnDestroy();

    // shouldn't that be done before passing the message to DefWinProc?
    if (m_nidIconData.hWnd && m_nidIconData.uID > 0 && TrayIsVisible())
        VERIFY(Shell_NotifyIcon(NIM_DELETE, &m_nidIconData));
}

bool CTrayDialog::TrayIsVisible() const
{
    return m_bTrayIconVisible;
}

void CTrayDialog::TraySetIcon(HICON hIcon, const bool bDelete)
{
    ASSERT(hIcon);
    if (hIcon)
    {
        //ASSERT(m_hPrevIconDelete == NULL);
        if (m_bCurIconDelete)
        {
            ASSERT(m_nidIconData.hIcon != NULL && (m_nidIconData.uFlags & NIF_ICON));
            m_hPrevIconDelete = m_nidIconData.hIcon;
        }
        m_bCurIconDelete = bDelete;
        m_nidIconData.hIcon = hIcon;
        m_nidIconData.uFlags |= NIF_ICON;
    }
}

//void CTrayDialog::TraySetIcon(const UINT nResourceID, const bool bDelete)
//{
//	TraySetIcon(AfxGetApp()->LoadIcon(nResourceID), bDelete);
//}

//void CTrayDialog::TraySetIcon(LPCTSTR lpszResourceName, const bool bDelete)
//{
//	TraySetIcon(AfxGetApp()->LoadIcon(lpszResourceName), bDelete);
//}

void CTrayDialog::TraySetToolTip(LPCTSTR lpszToolTip)
{
    const size_t usedSize = (GetShellVersion() >= MAKEDLLVERULL(5,0,0,0)) ? NOTIFYICONDATA_V2_TIP_SIZE : NOTIFYICONDATA_V1_TIP_SIZE;
    ASSERT(_tcslen(lpszToolTip) > 0 && _tcslen(lpszToolTip) < usedSize);
    _tcsncpy(m_nidIconData.szTip, lpszToolTip, usedSize);
    m_nidIconData.szTip[usedSize - 1] = L'\0';
//	ASSERT(_tcslen(lpszToolTip) > 0 && _tcslen(lpszToolTip) < NOTIFYICONDATA_V1_TIP_SIZE);
//	_tcsncpy(m_nidIconData.szTip, lpszToolTip, NOTIFYICONDATA_V1_TIP_SIZE);
//	m_nidIconData.szTip[NOTIFYICONDATA_V1_TIP_SIZE - 1] = L'\0';
    m_nidIconData.uFlags |= NIF_TIP;

    Shell_NotifyIcon(NIM_MODIFY, &m_nidIconData);
}

BOOL CTrayDialog::TrayShow()
{
    BOOL bSuccess = FALSE;
//>>> WiZaRd::FiX?
    // Sometimes m_bTrayIconVisible is true but the icon isn't shown? There's no harm in trying to add the icon again as far as I know...
//	if (!m_bTrayIconVisible)
//<<< WiZaRd::FiX?
    {
        bSuccess = Shell_NotifyIcon(NIM_ADD, &m_nidIconData);
        if (bSuccess)
            m_bTrayIconVisible = true;
    }
    return bSuccess;
}

BOOL CTrayDialog::TrayHide()
{
    BOOL bSuccess = FALSE;
//>>> WiZaRd::FiX?
    // Sometimes m_bTrayIconVisible is true but the icon isn't shown? There's no harm in trying to delete the icon again as far as I know...
//	if (!m_bTrayIconVisible)
//<<< WiZaRd::FiX?
    {
        bSuccess = Shell_NotifyIcon(NIM_DELETE, &m_nidIconData);
        if (bSuccess)
            m_bTrayIconVisible = false;
    }
    return bSuccess;
}

BOOL CTrayDialog::TrayUpdate()
{
    BOOL bSuccess = FALSE;
    if (m_bTrayIconVisible)
    {
        bSuccess = Shell_NotifyIcon(NIM_MODIFY, &m_nidIconData);
        if (!bSuccess)
        {
            //ASSERT(0);
            return false; // don't delete 'm_hPrevIconDelete' because it's still attached to the tray
        }
    }

    if (m_hPrevIconDelete != NULL)
    {
        VERIFY(::DestroyIcon(m_hPrevIconDelete));
        m_hPrevIconDelete = NULL;
    }

    return bSuccess;
}

BOOL CTrayDialog::TraySetMenu(const UINT nResourceID)
{
    BOOL bSuccess = m_mnuTrayMenu.LoadMenu(nResourceID);
    ASSERT(bSuccess);
    return bSuccess;
}

BOOL CTrayDialog::TraySetMenu(LPCTSTR lpszMenuName)
{
    BOOL bSuccess = m_mnuTrayMenu.LoadMenu(lpszMenuName);
    ASSERT(bSuccess);
    return bSuccess;
}

BOOL CTrayDialog::TraySetMenu(HMENU hMenu)
{
    m_mnuTrayMenu.Attach(hMenu);
    return TRUE;
}

LRESULT CTrayDialog::OnTrayNotify(WPARAM wParam, LPARAM lParam)
{
    CPoint pt;
    UINT uMsg = 0;
    UINT uID = 0;
#ifdef NOTIFYICON_VERSION_4
    if (m_nidIconData.uVersion == NOTIFYICON_VERSION_4)
    {
        // contains notification events, such as NIN_BALLOONSHOW, NIN_POPUPOPEN, or WM_CONTEXTMENU.
        uMsg = LOWORD(lParam);
        // contains the icon ID. Icon IDs are restricted to a length of 16 bits
        uID = HIWORD(lParam);

        pt.x = GET_X_LPARAM(wParam);
        pt.y = GET_Y_LPARAM(wParam);
    }
    else
#endif
    {
        uMsg = (UINT)lParam;
        uID = (UINT)wParam;

        GetCursorPos(&pt);
    }

    if (uID != 1)
        return 0;

    switch (uMsg)
    {
        case WM_MOUSEMOVE:
            ClientToScreen(&pt);
            OnTrayMouseMove(pt);
            break;

        case WM_LBUTTONDOWN:
            GetCursorPos(&pt);
            ClientToScreen(&pt);
            OnTrayLButtonDown(pt);
            break;

        case WM_LBUTTONUP:
            // Handle the WM_LBUTTONUP only if we know that there was also an according
            // WM_LBUTTONDOWN or WM_LBUTTONDBLCLK on our tray bar icon. If we would handle
            // WM_LBUTTONUP without checking this, we may get a single WM_LBUTTONUP message
            // whereby the according WM_LBUTTONDOWN message was meant for some other tray bar
            // icon.
            if (m_bLButtonDblClk)
            {
                KillSingleClickTimer();
                RestoreWindow();
                m_bLButtonDblClk = false;
            }
            else if (m_bLButtonDown)
            {
                if (m_uSingleClickTimer == 0)
                {
                    if (!IsWindowVisible())
                        m_uSingleClickTimer = SetTimer(IDT_SINGLE_CLICK, 300, NULL);
                }
                m_bLButtonDown = false;
            }
            break;

        case WM_LBUTTONDBLCLK:
            KillSingleClickTimer();
            GetCursorPos(&pt);
            ClientToScreen(&pt);
            OnTrayLButtonDblClk(pt);
            break;

        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            KillSingleClickTimer();
            GetCursorPos(&pt);
            //ClientToScreen(&pt);
            OnTrayRButtonUp(pt);
            break;

        case WM_RBUTTONDBLCLK:
            KillSingleClickTimer();
            GetCursorPos(&pt);
            ClientToScreen(&pt);
            OnTrayRButtonDblClk(pt);
            break;
    }
    return 1;
}

void CTrayDialog::KillSingleClickTimer()
{
    if (m_uSingleClickTimer)
    {
        VERIFY(KillTimer(m_uSingleClickTimer));
        m_uSingleClickTimer = 0;
    }
}

void CTrayDialog::OnTimer(UINT nIDEvent)
{
    if (nIDEvent == m_uSingleClickTimer)
    {
        TRACE("%s: nIDEvent=%u\n", __FUNCTION__, nIDEvent);
        // Kill that timer before calling 'OnTrayLButtonUp' which may create the MiniMule window asynchronously!
        KillSingleClickTimer();
        OnTrayLButtonUp(CPoint(0, 0));
    }
    CDialogMinTrayBtn<CResizableDialog>::OnTimer(nIDEvent);
}

void CTrayDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
    if (m_pbMinimizeToTray != NULL && *m_pbMinimizeToTray)
    {
        if ((nID & 0xFFF0) == SC_MINIMIZE)
        {
            if (TrayShow())
                ShowWindow(SW_HIDE);
        }
        else
            CTrayDialogBase::OnSysCommand(nID, lParam);
    }
    else if ((nID & 0xFFF0) == MP_MINIMIZETOTRAY)
    {
        if (TrayShow())
            ShowWindow(SW_HIDE);
    }
    else
        __super::OnSysCommand(nID, lParam);
}

void CTrayDialog::TraySetMinimizeToTray(bool* pbMinimizeToTray)
{
    m_pbMinimizeToTray = pbMinimizeToTray;
}

void CTrayDialog::TrayMinimizeToTrayChange()
{
    if (m_pbMinimizeToTray == NULL)
        return;
    if (*m_pbMinimizeToTray)
        MinTrayBtnHide();
    else
        MinTrayBtnShow();
}

void CTrayDialog::OnTrayRButtonUp(CPoint /*pt*/)
{
}

void CTrayDialog::OnTrayLButtonDown(CPoint /*pt*/)
{
    m_bLButtonDown = true;
}

void CTrayDialog::OnTrayLButtonUp(CPoint /*pt*/)
{
}

void CTrayDialog::OnTrayLButtonDblClk(CPoint /*pt*/)
{
    m_bLButtonDblClk = true;
}

void CTrayDialog::OnTrayRButtonDblClk(CPoint /*pt*/)
{
}

void CTrayDialog::OnTrayMouseMove(CPoint /*pt*/)
{
}

LRESULT CTrayDialog::OnTaskBarCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_bTrayIconVisible)
    {
        BOOL bResult = Shell_NotifyIcon(NIM_ADD, &m_nidIconData);
        if (!bResult)
            m_bTrayIconVisible = false;
    }
    return 0;
}

void CTrayDialog::RestoreWindow()
{
    ShowWindow(SW_SHOW);
	SetForegroundWindow();
}
