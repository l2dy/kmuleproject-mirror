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
#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif
#include "resource.h"
#include "./Mod/Modname.h" //>>> WiZaRd::Mod Stuff

#define	DEFAULT_TCP_PORT_OLD	4662
#define	DEFAULT_UDP_PORT_OLD	(DEFAULT_TCP_PORT_OLD+10)

#define PORTTESTURL			_T("http://porttest.emule-project.net/connectiontest.php?tcpport=%i&udpport=%i&lang=%i")

class CSearchList;
class CUploadQueue;
class CListenSocket;
class CDownloadQueue;
class UploadBandwidthThrottler;
class LastCommonRouteFinder;
class CemuleDlg;
class CClientList;
class CKnownFileList;
class CSharedFileList;
class CClientCreditsList;
class CFriendList;
class CClientUDPSocket;
class CIPFilter;
class CAbstractFile;
class CUpDownClient;
class CFirewallOpener;
class CUPnPImplWrapper;
//>>> WiZaRd::New Splash [TBH]
//class CSplashScreen;
class CSplashScreenEx;
//<<< WiZaRd::New Splash [TBH]
#ifdef USE_NAT_PMP
class CNATPMPThreadWrapper; //>>> WiZaRd::NAT-PMP
#endif
class CAntiLeechDataList; //>>> WiZaRd::ClientAnalyzer
class CAutoUpdate; //>>> WiZaRd::AutoUpdate
class CCustomSearches; //>>> WiZaRd::CustomSearches
class CModIconMapper; //>>> WiZaRd::ModIconMappings

struct SLogItem;

enum AppState
{
    APP_STATE_RUNNING = 0,
    APP_STATE_SHUTTINGDOWN,
    APP_STATE_DONE
};

class CemuleApp : public CWinApp
{
  public:
    CemuleApp(LPCTSTR lpszAppName = NULL);

    // ZZ:UploadSpeedSense -->
    UploadBandwidthThrottler* uploadBandwidthThrottler;
    LastCommonRouteFinder* lastCommonRouteFinder;
    // ZZ:UploadSpeedSense <--
    CemuleDlg*			emuledlg;
    CClientList*		clientlist;
    CKnownFileList*		knownfiles;
    CSharedFileList*	sharedfiles;
    CSearchList*		searchlist;
    CListenSocket*		listensocket;
    CUploadQueue*		uploadqueue;
    CDownloadQueue*		downloadqueue;
    CClientCreditsList*	clientcredits;
    CFriendList*		friendlist;
    CClientUDPSocket*	clientudp;
    CIPFilter*			ipfilter;
    CFirewallOpener*	m_pFirewallOpener;
    CUPnPImplWrapper*	m_pUPnPFinder;
#ifdef USE_NAT_PMP
    CNATPMPThreadWrapper*	m_pNATPMPThreadWrapper; //>>> WiZaRd::NAT-PMP
#endif
    CAntiLeechDataList* antileechlist; //>>> WiZaRd::ClientAnalyzer
    CAutoUpdate*		autoUpdater; //>>> WiZaRd::AutoUpdate
    CCustomSearches*	customSearches; //>>> WiZaRd::CustomSearches
    CModIconMapper*		theModIconMap; //>>> WiZaRd::ModIconMappings

    HANDLE				m_hMutexOneInstance;
    int					m_iDfltImageListColorFlags;
    CFont				m_fontHyperText;
    CFont				m_fontDefaultBold;
    CFont				m_fontSymbol;
#ifdef INFO_WND
    CFont				m_fontLog; //>>> WiZaRd::InfoWnd
#endif
    CFont				m_fontChatEdit;
    CBrush				m_brushBackwardDiagonal;
    static const UINT	m_nVersionMjr;
    static const UINT	m_nVersionMin;
    static const UINT	m_nVersionUpd;
    static const UINT	m_nVersionBld;
    DWORD				m_dwProductVersionMS;
    DWORD				m_dwProductVersionLS;
    CString				m_strCurVersionLong;
    CString				m_strCurVersionLongDbg;
    UINT				m_uCurVersionShort;
    UINT				m_uCurVersionCheck;
    ULONGLONG			m_ullComCtrlVer;
    AppState			m_app_state; // defines application state for shutdown
    CMutex				hashing_mut;
    CString*			pstrPendingLink;
    COPYDATASTRUCT		sendstruct;

// Implementierung
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual BOOL IsIdleMessage(MSG *pMsg);
    virtual BOOL OnIdle(LONG lCount); //>>> WiZaRd::Save CPU & Wine Compatibility

    // ed2k link functions
    void		AddEd2kLinksToDownload(CString strLinks, int cat);
    void		SearchClipboard();
    void		IgnoreClipboardLinks(CString strLinks)
    {
        m_strLastClipboardContents = strLinks;
    }
    void		PasteClipboard(int cat = 0);
    bool		IsEd2kFileLinkInClipboard();
    bool		IsEd2kLinkInClipboard(LPCSTR pszLinkType, int iLinkTypeLen);
    LPCTSTR		GetProfileFile()
    {
        return m_pszProfileName;
    }

    CString		CreateKadSourceLink(const CAbstractFile* f);

    // clipboard (text)
    bool		CopyTextToClipboard(CString strText);
    CString		CopyTextFromClipboard();

    void		OnlineSig();
    void		UpdateReceivedBytes(UINT bytesToAdd);
    void		UpdateSentBytes(UINT bytesToAdd, bool sentToFriend = false);
    int			GetFileTypeSystemImageIdx(LPCTSTR pszFilePath, int iLength = -1, bool bNormalsSize = false);
    HIMAGELIST	GetSystemImageList()
    {
        return m_hSystemImageList;
    }
    HIMAGELIST	GetBigSystemImageList()
    {
        return m_hBigSystemImageList;
    }
    CSize		GetSmallSytemIconSize()
    {
        return m_sizSmallSystemIcon;
    }
    CSize		GetBigSytemIconSize()
    {
        return m_sizBigSystemIcon;
    }
    void		CreateBackwardDiagonalBrush();
    void		CreateAllFonts();
    const CString &GetDefaultFontFaceName();
    bool		IsPortchangeAllowed();
    bool		IsConnected();
    bool		IsConnecting();
    bool		IsFirewalled();
    UINT		GetID();
    UINT		GetPublicIP(bool bIgnoreKadIP = false) const;	// return current (valid) public IP or 0 if unknown
    void		SetPublicIP(const UINT dwIP);
    void		ResetStandByIdleTimer();

    // because nearly all icons we are loading are 16x16, the default size is specified as 16 and not as 32 nor LR_DEFAULTSIZE
    HICON		LoadIcon(LPCTSTR lpszResourceName, int cx = 16, int cy = 16, UINT uFlags = LR_DEFAULTCOLOR) const;
    HICON		LoadIcon(UINT nIDResource) const;
    HBITMAP		LoadImage(LPCTSTR lpszResourceName, LPCTSTR pszResourceType) const;
    HBITMAP		LoadImage(UINT nIDResource, LPCTSTR pszResourceType) const;
    bool		LoadSkinColor(LPCTSTR pszKey, COLORREF& crColor) const;
    bool		LoadSkinColorAlt(LPCTSTR pszKey, LPCTSTR pszAlternateKey, COLORREF& crColor) const;
    CString		GetSkinFileItem(LPCTSTR lpszResourceName, LPCTSTR pszResourceType) const;
    void		ApplySkin(LPCTSTR pszSkinProfile);
    void		EnableRTLWindowsLayout();
    void		DisableRTLWindowsLayout();
    void		UpdateDesktopColorDepth();
    void		UpdateLargeIconSize();
    bool		IsXPThemeActive() const;
    bool		IsVistaThemeActive() const;
    bool		IsWinSock2Available() const;

    bool		GetLangHelpFilePath(CString& strResult);
    void		SetHelpFilePath(LPCTSTR pszHelpFilePath);
    void		ShowHelp(UINT uTopic, UINT uCmd = HELP_CONTEXT);
    bool		ShowWebHelp(UINT uTopic);

    // Elandal:ThreadSafeLogging -->
    // thread safe log calls
    void			HandleLogQueues();

    void			QueueDebugLogLine(bool bAddToStatusBar, LPCTSTR line,...);
    void			QueueDebugLogLineEx(UINT uFlags, LPCTSTR line,...);
    void			ClearDebugLogQueue(bool bDebugPendingMsgs = false);

    void			QueueLogLine(bool bAddToStatusBar, LPCTSTR line,...);
    void			QueueLogLineEx(UINT uFlags, LPCTSTR line,...);
    void			ClearLogQueue(bool bDebugPendingMsgs = false);
    // Elandal:ThreadSafeLogging <--

    bool			DidWeAutoStart()
    {
        return m_bAutoStart;
    }

  protected:
    bool ProcessCommandline();
    void SetTimeOnTransfer();
    static BOOL CALLBACK SearchEmuleWindow(HWND hWnd, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnHelp();

    HIMAGELIST m_hSystemImageList;
    CMapStringToPtr m_aExtToSysImgIdx;
    CSize m_sizSmallSystemIcon;

    HIMAGELIST m_hBigSystemImageList;
    CMapStringToPtr m_aBigExtToSysImgIdx;
    CSize m_sizBigSystemIcon;

    CString		m_strDefaultFontFaceName;
    bool		m_bGuardClipboardPrompt;
    CString		m_strLastClipboardContents;

    // Elandal:ThreadSafeLogging -->
    // thread safe log calls
    CCriticalSection m_queueLock;
    CTypedPtrList<CPtrList, SLogItem*> m_QueueDebugLog;
    CTypedPtrList<CPtrList, SLogItem*> m_QueueLog;
    // Elandal:ThreadSafeLogging <--

    UINT		m_dwPublicIP;
    bool		m_bAutoStart;
    WSADATA		m_wsaData;

    // Splash screen
//>>> WiZaRd::New Splash [TBH]
  private:
    //CSplashScreen*	m_pSplashWnd;
    CSplashScreenEx*	m_pSplashWnd;
    DWORD			m_dwSplashTime;
  public:
    void			ShowSplash();
    void			DestroySplash();
    bool			IsSplashActive() const
    {
        return m_pSplashWnd != NULL;
    }
    void			SetSplashText(const CString& s);
//<<< WiZaRd::New Splash [TBH]

  private:
    UINT     m_wTimerRes;
//>>> WiZaRd::Easy ModVersion
  public:
    CString  GetClientVersionString() const;
    CString  GetClientVersionStringBase(const bool bDebug = false) const;
//<<< WiZaRd::Easy ModVersion
//>>> WiZaRd::Automatic Restart
  public:
    bool	IsRestartPlanned() const
    {
        return m_bRestartApp;
    }
    void	PlanRestart()
    {
        m_bRestartApp = true;
    }
  private:
    bool	m_bRestartApp;
//<<< WiZaRd::Automatic Restart
//>>> WiZaRd::IPv6 [Xanatos]
#ifdef IPV6_SUPPORT
  public:
    const CAddress&	GetPublicIPv6() const			{return m_PublicIPv6;}
    void		SetPublicIPv6(const CAddress& IP)	{m_PublicIPv6 = IP;}
    void		UpdateIPv6();
  private:
    CAddress m_PublicIPv6;
#endif
//<<< WiZaRd::IPv6 [Xanatos]
};

extern CemuleApp theApp;


//////////////////////////////////////////////////////////////////////////////
// CTempIconLoader

class CTempIconLoader
{
  public:
    // because nearly all icons we are loading are 16x16, the default size is specified as 16 and not as 32 nor LR_DEFAULTSIZE
    CTempIconLoader(LPCTSTR pszResourceID, int cx = 16, int cy = 16, UINT uFlags = LR_DEFAULTCOLOR);
    CTempIconLoader(UINT uResourceID, int cx = 16, int cy = 16, UINT uFlags = LR_DEFAULTCOLOR);
    ~CTempIconLoader();

    operator HICON() const
    {
        return this == NULL ? NULL : m_hIcon;
    }

  protected:
    HICON m_hIcon;
};
