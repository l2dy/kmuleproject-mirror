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
#pragma once
#include "TrayDialog.h"
#include "MeterIcon.h"
#include "TitleMenu.h"
//>>> Tux::SnarlSupport
#include "Mod/SnarlInterface.h"
//<<< Tux::SnarlSupport

namespace Kademlia
{
class CSearch;
class CContact;
class CEntry;
class CUInt128;
};

class CChatWnd;
class CKnownFileList;
class CMainFrameDropTarget;
class CMuleStatusBarCtrl;
class CMuleToolbarCtrl;
class CPreferencesDlg;
class CSearchDlg;
class CServerWnd;
class CSharedFilesWnd;
class CStatisticsDlg;
class CTransferDlg;
struct Status;
class CMuleSystrayDlg;
class CMiniMule;
#ifdef INFO_WND
class CInfoWnd; //>>> WiZaRd::InfoWnd
#endif

// emuleapp <-> emuleapp
#define OP_ED2KLINK				12000
#define OP_CLCOMMAND			12001
#define OP_COLLECTION			12002

#define	EMULE_HOTMENU_ACCEL		'x'
#define	EMULSKIN_BASEEXT		_T("eMuleSkin")

class CemuleDlg : public CTrayDialog
{
    friend class CMuleToolbarCtrl;
    friend class CMiniMule;

  public:
    CemuleDlg(CWnd* pParent = NULL);
    ~CemuleDlg();

    enum { IDD = IDD_EMULE_DIALOG };

    static bool IsRunning();
    void ShowConnectionState();
    void ShowNotifier(LPCTSTR pszText, int iMsgType);
    void ShowUserCount();
    void ShowMessageState(UINT iconnr);
    void SetActiveDialog(CWnd* dlg);
    CWnd* GetActiveDialog() const
    {
        return activewnd;
    }
    void ShowTransferRate(bool forceAll=false);
    void ShowPing();
    void Localize();

#ifdef HAVE_WIN7_SDK_H
    void UpdateStatusBarProgress();
    void UpdateThumbBarButtons(bool initialAddToDlg=false);
    void OnTBBPressed(UINT id);
    void EnableTaskbarGoodies(bool enable);

    enum TBBIDS
    {
        TBB_FIRST,
        TBB_THROTTLE=TBB_FIRST,
        TBB_UNTHROTTLE,
        TBB_PREFERENCES,
        TBB_LAST = TBB_PREFERENCES
    };
#endif

    // Logging
    void AddLogText(UINT uFlags, LPCTSTR pszText);

    CString	GetConnectionStateString();
    UINT GetConnectionStateIconIndex() const;
    CString	GetTransferRateString();
    CString	GetUpDatarateString(UINT uUpDatarate = -1);
    CString	GetDownDatarateString(UINT uDownDatarate = -1);

    void StopTimer();
    void DoVersioncheck(bool manual);
    void ApplyHyperTextFont(LPLOGFONT pFont);
#ifdef INFO_WND
    void ApplyLogFont(LPLOGFONT pFont); //>>> WiZaRd::InfoWnd
#endif
    void ProcessED2KLink(LPCTSTR pszData);
    void SetStatusBarPartsSize();
    int ShowPreferences(UINT uStartPageID = (UINT)-1);
    bool IsPreferencesDlgOpen() const;
    bool IsTrayIconToFlash()
    {
        return m_iMsgIcon!=0;
    }
    void SetToolTipsDelay(UINT uDelay);
    void RemoveUPnPMappings(); //>>> WiZaRd
    void StartUPnP(bool bReset = true, uint16 nForceTCPPort = 0, uint16 nForceUDPPort = 0);
    void RefreshUPnP(bool bRequestAnswer = false);
#ifdef USE_NAT_PMP
//>>> WiZaRd::NAT-PMP
    void RemoveNATPMPMappings();
    void StartNATPMP(bool bReset = true, uint16 nForceTCPPort = 0, uint16 nForceUDPPort = 0);
    void RefreshNATPMP(bool bRequestAnswer = false);
//<<< WiZaRd::NAT-PMP
#endif
    HBRUSH GetCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    virtual void TrayMinimizeToTrayChange();
    virtual void RestoreWindow();
    virtual void HtmlHelp(DWORD_PTR dwData, UINT nCmd = 0x000F);

    CTransferDlg*	transferwnd;
#ifdef INFO_WND
    CInfoWnd*		infoWnd; //>>> WiZaRd::InfoWnd
#endif
    CServerWnd*		serverwnd;
    CPreferencesDlg* preferenceswnd;
    CSharedFilesWnd* sharedfileswnd;
    CSearchDlg*		searchwnd;
    CChatWnd*		chatwnd;
    CMuleStatusBarCtrl* statusbar;
    CStatisticsDlg*  statisticswnd;
    CReBarCtrl		m_ctlMainTopReBar;
    CMuleToolbarCtrl* toolbar;
    CWnd*			activewnd;
    uint8			status;

//>>> Snarl Support
  private:
    bool	m_bSnarlRegistered;
    SnarlInterface* snarlInterface;
  public:
    void	HookSnarl();
    void	UnhookSnarl();
//<<< Snarl Support
//>>> WiZaRd::MediaInfoDLL Update
    void	UpdateMediaInfoDLL();
//<<< WiZaRd::MediaInfoDLL Update
    afx_msg void OnClose(); //>>> WiZaRd::Automatic Restart

  protected:
    HICON			m_hIcon;
    bool			ready;
    bool			m_bStartMinimizedChecked;
    bool			m_bStartMinimized;
    WINDOWPLACEMENT m_wpFirstRestore;
    HICON			connicons[3];
    HICON			transicons[4];
    HICON			imicons[3];
    HICON			m_icoSysTrayCurrent;
    HICON			usericon;
    CMeterIcon		m_TrayIcon;
    HICON			m_icoSysTrayConnected;		// do not use those icons for anything else than the traybar!!!
    HICON			m_icoSysTrayDisconnected;	// do not use those icons for anything else than the traybar!!!
    HICON			m_icoSysTrayFirewalled;		// do not use those icons for anything else than the traybar!!!
    int				m_iMsgIcon;
    UINT			m_uLastSysTrayIconCookie;
    UINT			m_uUpDatarate;
    UINT			m_uDownDatarate;
    CImageList		imagelist;
    CTitleMenu		trayPopup;
    CMuleSystrayDlg* m_pSystrayDlg;
    CMainFrameDropTarget* m_pDropTarget;
    CMenu			m_SysMenuOptions;
    CMenu			m_menuUploadCtrl;
    CMenu			m_menuDownloadCtrl;
    char			m_acVCDNSBuffer[MAXGETHOSTSTRUCT];
    bool			m_iMsgBlinkState;
    bool			m_bConnectRequestDelayedForUPnP;
#ifdef USE_NAT_PMP
    bool			m_bConnectRequestDelayedForNATPMP; //>>> WiZaRd::NAT-PMP
#endif
    bool			m_bKadSuspendDisconnect;
    bool			m_bInitedCOM;
#ifdef HAVE_WIN7_SDK_H
    CComPtr<ITaskbarList3>	m_pTaskbarList;
    THUMBBUTTON		m_thbButtons[TBB_LAST+1];

    TBPFLAG			m_currentTBP_state;
    float			m_prevProgress;
    HICON			m_ovlIcon;
#endif

    // Mini Mule
    CMiniMule* m_pMiniMule;
    void DestroyMiniMule();
    void CreateMiniMule();

    CMap<UINT, UINT, LPCTSTR, LPCTSTR> m_mapTbarCmdToIcon;
    void CreateToolbarCmdIconMap();
    LPCTSTR GetIconFromCmdId(UINT uId);

    // Startup Timer
    UINT_PTR m_hTimer;
    static void CALLBACK StartupTimer(HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime);

    // UPnP TimeOutTimer
    UINT_PTR m_hUPnPTimeOutTimer;
    static void CALLBACK UPnPTimeOutTimer(HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime);
#ifdef USE_NAT_PMP
//>>> WiZaRd::NAT-PMP
    UINT_PTR m_hNATPMPTimeOutTimer;
    static void CALLBACK NATPMPTimeOutTimer(HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime);
//<<< WiZaRd::NAT-PMP
#endif

    void StartConnection();
    void CloseConnection();
    void MinimizeWindow();
    void PostStartupMinimized();
    void UpdateTrayIcon(int iPercent);
    void ShowConnectionStateIcon();
    void ShowTransferStateIcon();
    void ShowUserStateIcon();
    void AddSpeedSelectorMenus(CMenu* addToMenu);
    int  GetRecMaxUpload();
    void ShowToolPopup(bool toolsonly = false);
    void SetAllIcons();
    bool CanClose();
    int MapWindowToToolbarButton(CWnd* pWnd) const;
    CWnd* MapToolbarButtonToWindow(int iButtonID) const;
    int GetNextWindowToolbarButton(int iButtonID, int iDirection = 1) const;
    bool IsWindowToolbarButton(int iButtonID) const;
    void SetTaskbarIconColor();

    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnCancel();
    virtual void OnOK();
    virtual void OnTrayRButtonUp(CPoint pt);
    virtual void OnTrayLButtonUp(CPoint pt);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    DECLARE_MESSAGE_MAP()
    // afx_msg void OnClose(); //>>> WiZaRd::Automatic Restart
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType,int cx,int cy);
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedConnect();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnBnClickedHotmenu();
    afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
    afx_msg void OnSysColorChange();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
    afx_msg BOOL OnQueryEndSession();
    afx_msg void OnEndSession(BOOL bEnding);
    afx_msg LRESULT OnUserChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnKickIdle(UINT nWhy, long lIdleCount);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg BOOL OnChevronPushed(UINT id, NMHDR *pnm, LRESULT *pResult);
    afx_msg LRESULT OnPowerBroadcast(WPARAM wParam, LPARAM lParam);

    // quick-speed changer -- based on xrmb
    afx_msg void QuickSpeedUpload(UINT nID);
    afx_msg void QuickSpeedDownload(UINT nID);
    afx_msg void QuickSpeedOther(UINT nID);
    // end of quick-speed changer

    afx_msg LRESULT OnWMData(WPARAM wParam,LPARAM lParam);
    afx_msg LRESULT OnFileHashed(WPARAM wParam,LPARAM lParam);
    afx_msg LRESULT OnHashFailed(WPARAM wParam,LPARAM lParam);
    afx_msg LRESULT OnFileAllocExc(WPARAM wParam,LPARAM lParam);
    afx_msg LRESULT OnFileCompleted(WPARAM wParam,LPARAM lParam);
    afx_msg LRESULT OnFileOpProgress(WPARAM wParam,LPARAM lParam);

    //Framegrabbing
    afx_msg LRESULT OnFrameGrabFinished(WPARAM wParam,LPARAM lParam);

    afx_msg LRESULT OnAreYouEmule(WPARAM, LPARAM);

#ifdef HAVE_WIN7_SDK_H
    afx_msg LRESULT OnTaskbarBtnCreated(WPARAM, LPARAM);
#endif

//>>> WiZaRd::Static MM
    /*
        // Mini Mule
        afx_msg LRESULT OnCloseMiniMule(WPARAM wParam, LPARAM lParam);
    */
//<<< WiZaRd::Static MM

    // Terminal Services
    afx_msg LRESULT OnConsoleThreadEvent(WPARAM wParam, LPARAM lParam);

    // UPnP
    afx_msg LRESULT OnUPnPResult(WPARAM wParam, LPARAM lParam);

#ifdef USE_NAT_PMP
    afx_msg LRESULT OnNATPMPResult(WPARAM wParam, LPARAM lParam); //>>> WiZaRd::NAT-PMP
#endif

//>>> WiZaRd::7zip
    afx_msg LRESULT OnSevenZipJobDone(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSevenZipJobFailed(WPARAM wParam, LPARAM lParam);
//<<< WiZaRd::7zip
    afx_msg LRESULT OnUploadTimer(WPARAM wParam, LPARAM lParam);  //>>> WiZaRd::Catch exceptions

//>>> WiZaRd::Fix crash on exit
  private:
    CWinThread* m_AICHSyncThread;
//<<< WiZaRd::Fix crash on exit
};


enum EEMuleAppMsgs
{
    //thread messages
    TM_FINISHEDHASHING = WM_APP + 10,
    TM_HASHFAILED,
    TM_FRAMEGRABFINISHED,
    TM_FILEALLOCEXC,
    TM_FILECOMPLETED,
    TM_FILEOPPROGRESS,
    TM_CONSOLETHREADEVENT,
//>>> WiZaRd::7zip
    TM_SEVENZIP_JOB_DONE,
    TM_SEVENZIP_JOB_FAILED,
//<<< WiZaRd::7zip
    TM_UPLOAD_TIMER, //>>> WiZaRd::Catch exceptions
};

enum EWebinterfaceOrders
{
    WEBGUIIA_WINFUNC = 1,
    WEBGUIIA_UPD_CATTABS,
    WEBGUIIA_UPD_SFUPDATE,
    WEBGUIIA_DISCONNECT,
    WEBGUIIA_SHARED_FILES_RELOAD,
    WEBGUIIA_SHOWSTATISTICS,
    WEBGUIIA_DELETEALLSEARCHES,
    WEBGUIIA_KAD_BOOTSTRAP,
    WEBGUIIA_KAD_START,
    WEBGUIIA_KAD_STOP,
    WEBGUIIA_KAD_RCFW
};
