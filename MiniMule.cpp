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
#include <io.h>
#include <wininet.h>
#include <atlutil.h>
#include "emule.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "MiniMule.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "MenuCmds.h"
#include "IESecurity.h"
#include "UserMsgs.h"
#include "UploadQueue.h"
#include "DownloadQueue.h"
//>>> WiZaRd::SessionRatio //>>> WiZaRd::Ratio Indicator
#include "Statistics.h" 
#include "./Mod/ModOpcodes.h"
//<<< WiZaRd::SessionRatio //<<< WiZaRd::Ratio Indicator

#if (WINVER < 0x0500)
/* AnimateWindow() Commands */
#define AW_HOR_POSITIVE             0x00000001
#define AW_HOR_NEGATIVE             0x00000002
#define AW_VER_POSITIVE             0x00000004
#define AW_VER_NEGATIVE             0x00000008
#define AW_CENTER                   0x00000010
#define AW_HIDE                     0x00010000
#define AW_ACTIVATE                 0x00020000
#define AW_SLIDE                    0x00040000
#define AW_BLEND                    0x00080000
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern UINT g_uMainThreadId;

class CCounter
{
public:
    CCounter(int& ri)
        : m_ri(ri)
    {
        ASSERT( ri == 0 );
        ++m_ri;
    }
    ~CCounter()
    {
        --m_ri;
        ASSERT( m_ri == 0 );
    }
    int& m_ri;
};

CString CMiniMule::ratioIcons[9]; //>>> WiZaRd::Ratio Indicator

// CMiniMule dialog

IMPLEMENT_DYNCREATE(CMiniMule, CDHtmlDialog)
BEGIN_MESSAGE_MAP(CMiniMule, CDHtmlDialog)
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_WM_NCLBUTTONDBLCLK()
END_MESSAGE_MAP()

BEGIN_EVENTSINK_MAP(CMiniMule, CDHtmlDialog)
ON_EVENT(CDHtmlDialog, AFX_IDC_BROWSER, 250 /* BeforeNavigate2 */, _OnBeforeNavigate2, VTS_DISPATCH VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PBOOL)
END_EVENTSINK_MAP()

BEGIN_DHTML_EVENT_MAP(CMiniMule)
	DHTML_EVENT_ONCLICK(L"restoreWndLink", OnRestoreMainWindow)
	DHTML_EVENT_ONKEYPRESS(L"restoreWndLink", OnRestoreMainWindow)
	DHTML_EVENT_ONCLICK(L"restoreWndImg", OnRestoreMainWindow)
	DHTML_EVENT_ONKEYPRESS(L"restoreWndImg", OnRestoreMainWindow)

	DHTML_EVENT_ONCLICK(L"openIncomingLink", OnOpenIncomingFolder)
	DHTML_EVENT_ONKEYPRESS(L"openIncomingLink", OnOpenIncomingFolder)
	DHTML_EVENT_ONCLICK(L"openIncomingImg", OnOpenIncomingFolder)
	DHTML_EVENT_ONKEYPRESS(L"openIncomingImg", OnOpenIncomingFolder)

	DHTML_EVENT_ONCLICK(L"optionsLink", OnOptions)
	DHTML_EVENT_ONKEYPRESS(L"optionsLink", OnOptions)
	DHTML_EVENT_ONCLICK(L"optionsImg", OnOptions)
	DHTML_EVENT_ONKEYPRESS(L"optionsImg", OnOptions)
END_DHTML_EVENT_MAP()

CMiniMule::CMiniMule(CWnd* pParent /*=NULL*/)
    : CDHtmlDialog(CMiniMule::IDD, CMiniMule::IDH, pParent)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    m_iInCallback = 0;
    m_bResolveImages = true;
//>>> WiZaRd::Static MM
    //m_bRestoreMainWnd = false; 
	m_bVisible = false; 
//<<< WiZaRd::Static MM
    m_uAutoCloseTimer = 0;

    SetHostFlags(m_dwHostFlags
                 | DOCHOSTUIFLAG_DIALOG					// MSHTML does not enable selection of the text in the form
                 | DOCHOSTUIFLAG_DISABLE_HELP_MENU		// MSHTML does not add the Help menu item to the container's menu.
                );
}

CMiniMule::~CMiniMule()
{
}

STDMETHODIMP CMiniMule::GetOptionKeyPath(LPOLESTR* /*pchKey*/, DWORD /*dw*/)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
	TRACE(L"%hs\n", __FUNCTION__);

//OpenKey		HKCU\Software\eMule\IE
//QueryValue	HKCU\Software\eMule\IE\Show_FullURL
//QueryValue	HKCU\Software\eMule\IE\SmartDithering
//QueryValue	HKCU\Software\eMule\IE\RtfConverterFlags

//OpenKey		HKCU\Software\eMule\IE\Main
//QueryValue	HKCU\Software\eMule\IE\Main\Page_Transitions
//QueryValue	HKCU\Software\eMule\IE\Main\Use_DlgBox_Colors
//QueryValue	HKCU\Software\eMule\IE\Main\Anchor Underline
//QueryValue	HKCU\Software\eMule\IE\Main\CSS_Compat
//QueryValue	HKCU\Software\eMule\IE\Main\Expand Alt Text
//QueryValue	HKCU\Software\eMule\IE\Main\Display Inline Images
//QueryValue	HKCU\Software\eMule\IE\Main\Display Inline Videos
//QueryValue	HKCU\Software\eMule\IE\Main\Play_Background_Sounds
//QueryValue	HKCU\Software\eMule\IE\Main\Play_Animations
//QueryValue	HKCU\Software\eMule\IE\Main\Print_Background
//QueryValue	HKCU\Software\eMule\IE\Main\Use Stylesheets
//QueryValue	HKCU\Software\eMule\IE\Main\SmoothScroll
//QueryValue	HKCU\Software\eMule\IE\Main\Show image placeholders
//QueryValue	HKCU\Software\eMule\IE\Main\Disable Script Debugger
//QueryValue	HKCU\Software\eMule\IE\Main\DisableScriptDebuggerIE
//QueryValue	HKCU\Software\eMule\IE\Main\Move System Caret
//QueryValue	HKCU\Software\eMule\IE\Main\Force Offscreen Composition
//QueryValue	HKCU\Software\eMule\IE\Main\Enable AutoImageResize
//QueryValue	HKCU\Software\eMule\IE\Main\Q051873
//QueryValue	HKCU\Software\eMule\IE\Main\UseThemes
//QueryValue	HKCU\Software\eMule\IE\Main\UseHR
//QueryValue	HKCU\Software\eMule\IE\Main\Q300829
//QueryValue	HKCU\Software\eMule\IE\Main\Disable_Local_Machine_Navigate
//QueryValue	HKCU\Software\eMule\IE\Main\Cleanup HTCs
//QueryValue	HKCU\Software\eMule\IE\Main\Q331869
//QueryValue	HKCU\Software\eMule\IE\Main\AlwaysAllowExecCommand

//OpenKey		HKCU\Software\eMule\IE\Settings
//QueryValue	HKCU\Software\eMule\IE\Settings\Anchor Color
//QueryValue	HKCU\Software\eMule\IE\Settings\Anchor Color Visited
//QueryValue	HKCU\Software\eMule\IE\Settings\Anchor Color Hover
//QueryValue	HKCU\Software\eMule\IE\Settings\Always Use My Colors
//QueryValue	HKCU\Software\eMule\IE\Settings\Always Use My Font Size
//QueryValue	HKCU\Software\eMule\IE\Settings\Always Use My Font Face
//QueryValue	HKCU\Software\eMule\IE\Settings\Use Anchor Hover Color
//QueryValue	HKCU\Software\eMule\IE\Settings\MiscFlags

//OpenKey		HKCU\Software\eMule\IE\Styles

//OpenKey		HKCU\Software\eMule\IE\International
//OpenKey		HKCU\Software\eMule\IE\International\Scripts
//OpenKey		HKCU\Software\eMule\IE\International\Scripts\3

    return E_NOTIMPL;
}

void CMiniMule::DoDataExchange(CDataExchange* pDX)
{
    CDHtmlDialog::DoDataExchange(pDX);
}

BOOL CMiniMule::CreateControlSite(COleControlContainer* pContainer, COleControlSite** ppSite, UINT /*nID*/, REFCLSID /*clsid*/)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    CMuleBrowserControlSite *pBrowserSite = new CMuleBrowserControlSite(pContainer, this);
    if (!pBrowserSite)
        return FALSE;
    *ppSite = pBrowserSite;
    return TRUE;
}

CString CreateFilePathUrl(LPCTSTR pszFilePath, int nProtocol)
{
    // Do *not* use 'AtlCanonicalizeUrl' (or similar function) to convert a file path into
    // an encoded URL. Basically this works, but if the file path contains special characters
    // like e.g. Umlaute, the IE control can not open the encoded URL.
    //
    // The file path "D:\dir_�#,.-_���#'+~�`�}=])[({&%$!^�\Kopie von ### MiniMule3CyanSnow.htm"
    // can get opened successfully by the IE control *without* using any URL encoding.
    //
    // Though, regardless of using 'AtlCanonicalizeUrl' or not, there is still one special
    // case where the IE control can not open the URL. If the file starts with something like
    //	"c:\#dir\kMule.exe". For any unknown reason the sequence "c:\#" causes troubles for
    // the IE control. It does not help to escape that sequence. It always fails.
    //
    CString strEncodedFilePath;
    if (nProtocol == INTERNET_SCHEME_RES)
    {
        // "res://" protocol has to be specified with 2 slashes ("res:///" does not work)
		strEncodedFilePath = L"res://";
        strEncodedFilePath += pszFilePath;
    }
    else
    {
        ASSERT( nProtocol == INTERNET_SCHEME_FILE );
        // "file://" protocol has to be specified with 3 slashes
		strEncodedFilePath = L"file:///";
        strEncodedFilePath += pszFilePath;
    }
    return strEncodedFilePath;
}

BOOL CMiniMule::OnInitDialog()
{
	lastRatio = -1; //>>> WiZaRd::Ratio Indicator
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    ASSERT( m_iInCallback == 0 );
	CString strHtmlFile = theApp.GetSkinFileItem(L"MiniMule", L"HTML");
    if (!strHtmlFile.IsEmpty())
    {
        if (_taccess(strHtmlFile, 0) == 0)
        {
            m_strCurrentUrl = CreateFilePathUrl(strHtmlFile, INTERNET_SCHEME_FILE);
            m_nHtmlResID = 0;
            m_szHtmlResID = NULL;
            m_bResolveImages = false;
        }
    }

    if (m_strCurrentUrl.IsEmpty())
    {
        TCHAR szModulePath[MAX_PATH];
        DWORD dwModPathLen = GetModuleFileName(AfxGetResourceHandle(), szModulePath, _countof(szModulePath));
        if (dwModPathLen != 0 && dwModPathLen < _countof(szModulePath))
        {
            m_strCurrentUrl = CreateFilePathUrl(szModulePath, INTERNET_SCHEME_RES);
			m_strCurrentUrl.AppendFormat(L"/%d", m_nHtmlResID);
            m_nHtmlResID = 0;
            m_szHtmlResID = NULL;
            m_bResolveImages = true;
        }
    }

    // TODO: Only in debug build: Check the size of the dialog resource right before 'OnInitDialog'
    // to ensure the window is small enough!
    CDHtmlDialog::OnInitDialog();

    const uint8 uWndTransparency = GetTransparency();
    if (uWndTransparency)
    {
        m_uiLastTransparency = uWndTransparency; //>>> WiZaRd::Static MM
        ::SetWindowLong(m_hWnd, GWL_EXSTYLE, ::GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        BOOL (WINAPI* pfnSetLayeredWndAttribs)(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
        (FARPROC&)pfnSetLayeredWndAttribs = GetProcAddress(GetModuleHandle(L"user32"), "SetLayeredWindowAttributes");
        if (pfnSetLayeredWndAttribs)
            (*pfnSetLayeredWndAttribs)(m_hWnd, 0, (BYTE)(255 * uWndTransparency/100), LWA_ALPHA);

    }
//>>> WiZaRd::Static MM
    else
        m_uiLastTransparency = 0;
//<<< WiZaRd::Static MM

    SetWindowText(theApp.GetClientVersionString());

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMiniMule::OnClose()
{
	TRACE(L"%s\n", __FUNCTION__);
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    ASSERT( m_iInCallback == 0 );
    KillAutoCloseTimer();

    //don't really "close" - just hide it... :P
    ShowHide(true);
		
	CDHtmlDialog::OnClose();
//>>> WiZaRd::Static MM
/*
    ///////////////////////////////////////////////////////////////////////////
    // Destroy the MiniMule window

    // Solution #1: Posting a close-message to main window (can not be done with 'SendMessage') may
    // create message queue sync. problems when having high system load.
    //theApp.emuledlg->PostMessage(UM_CLOSE_MINIMULE, (WPARAM)m_bRestoreMainWnd);

    // Solution #2: 'DestroyModeless' -- posts a 'destroy' message to 'this' which will have a very
    // similar effect (and most likely problems) than using PostMessage(<main-window>).
    //DestroyModeless();

    // Solution #3: 'DestroyWindow' -- destroys the window and *deletes* 'this'. On return of
    // 'DestroyWindow' the 'this' is no longer valid! However, this should be safe because MFC
    // is also using the same 'technique' for several window classes.
    bool bRestoreMainWnd = m_bRestoreMainWnd;
    //NOTE: 'this' IS NO LONGER VALID!
    if (bRestoreMainWnd)
        theApp.emuledlg->RestoreWindow();
*/
//<<< WiZaRd::Static MM
}

void CMiniMule::OnDestroy()
{
	TRACE(L"%s\n", __FUNCTION__);
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    ASSERT( m_iInCallback == 0 );
    KillAutoCloseTimer();
    CDHtmlDialog::OnDestroy();
}

void CMiniMule::PostNcDestroy()
{
	TRACE(L"%s\n", __FUNCTION__);
    CDHtmlDialog::PostNcDestroy();
    if (theApp.emuledlg)
        theApp.emuledlg->m_pMiniMule = NULL;
    delete this;
}

void CMiniMule::Localize()
{
	SetElementHtml(L"connectedLabel", CComBSTR(GetResString(IDS_CONNECTED)));
	SetElementHtml(L"upRateLabel", CComBSTR(GetResString(IDS_PW_CON_UPLBL)));
	SetElementHtml(L"downRateLabel", CComBSTR(GetResString(IDS_PW_CON_DOWNLBL)));
	SetElementHtml(L"SessionRatioLabel", CComBSTR(GetResString(IDS_RATIO))); //>>> WiZaRd::SessionRatio
	SetElementHtml(L"completedLabel", CComBSTR(GetResString(IDS_DL_TRANSFCOMPL)));
	SetElementHtml(L"freeSpaceLabel", CComBSTR(GetResString(IDS_STATS_FREESPACE)));

    CComPtr<IHTMLElement> a;
	GetElementInterface(L"openIncomingLink", &a);
    if (a)
    {
		a->put_title(CComBSTR(RemoveAmpersand(GetResString(IDS_OPENINC))));
        a.Release();
    }
	GetElementInterface(L"optionsLink", &a);
    if (a)
    {
		a->put_title(CComBSTR(RemoveAmpersand(GetResString(IDS_EM_PREFS))));
        a.Release();
    }
	GetElementInterface(L"restoreWndLink", &a);
    if (a)
    {
		a->put_title(CComBSTR(RemoveAmpersand(GetResString(IDS_MAIN_POPUP_RESTORE))));
        a.Release();
    }

    CComPtr<IHTMLImgElement> img;
	GetElementInterface(L"openIncomingImg", &img);
    if (img)
    {
		img->put_alt(CComBSTR(RemoveAmpersand(GetResString(IDS_OPENINC))));
        img.Release();
    }
	GetElementInterface(L"optionsImg", &img);
    if (img)
    {
		img->put_alt(CComBSTR(RemoveAmpersand(GetResString(IDS_EM_PREFS))));
        img.Release();
    }
	GetElementInterface(L"restoreWndImg", &img);
    if (img)
    {
		img->put_alt(CComBSTR(RemoveAmpersand(GetResString(IDS_MAIN_POPUP_RESTORE))));
        img.Release();
    }
}

void CMiniMule::UpdateContent(UINT uUpDatarate, UINT uDownDatarate)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    if (m_bResolveImages)
    {
        static const LPCTSTR _apszConnectedImgs[] =
        {
            L"CONNECTEDNOTNOT.GIF",
            L"CONNECTEDLOWLOW.GIF",
            L"CONNECTEDHIGHHIGH.GIF"
        };

        UINT uIconIdx = theApp.emuledlg->GetConnectionStateIconIndex();
        if (uIconIdx >= _countof(_apszConnectedImgs))
        {
            ASSERT(0);
            uIconIdx = 0;
        }

        TCHAR szModulePath[MAX_PATH];
        DWORD dwModPathLen = GetModuleFileName(AfxGetResourceHandle(), szModulePath, _countof(szModulePath));
        if (dwModPathLen != 0 && dwModPathLen < _countof(szModulePath))
        {
            CString strFilePathUrl(CreateFilePathUrl(szModulePath, INTERNET_SCHEME_RES));
            CComPtr<IHTMLImgElement> elm;
            GetElementInterface(L"connectedImg", &elm);
            if (elm)
            {
                CString strResourceURL;
				strResourceURL.Format(L"%s/%s", strFilePathUrl, _apszConnectedImgs[uIconIdx]);
                elm->put_src(CComBSTR(strResourceURL));
            }
        }
    }

    SetElementHtml(L"connected", CComBSTR(theApp.IsConnected() ? GetResString(IDS_YES) : GetResString(IDS_NO)));
    SetElementHtml(L"upRate", CComBSTR(theApp.emuledlg->GetUpDatarateString(uUpDatarate)));
    SetElementHtml(L"downRate", CComBSTR(theApp.emuledlg->GetDownDatarateString(uDownDatarate)));
//>>> WiZaRd::SessionRatio
	CString strRatio = L"";
	if (theStats.sessionReceivedBytes > 0 && theStats.sessionSentBytes > 0)
	{
		if (theStats.sessionReceivedBytes<theStats.sessionSentBytes) 
			strRatio.Format(L"%.2f : 1", (float)theStats.sessionSentBytes/theStats.sessionReceivedBytes);
		else
			strRatio.Format(L"1 : %.2f", (float)theStats.sessionReceivedBytes/theStats.sessionSentBytes);
	}
	else //non avail
		strRatio.Format(L" %s", GetResString(IDS_FSTAT_WAITING));
	SetElementHtml(L"SessionRatio", CComBSTR(strRatio));
	SetRatioIcon();
//<<< WiZaRd::SessionRatio
    UINT uCompleted = 0;
    if (thePrefs.GetRemoveFinishedDownloads())
        uCompleted = thePrefs.GetDownSessionCompletedFiles();
    else if (theApp.emuledlg && theApp.emuledlg->transferwnd && theApp.emuledlg->transferwnd->GetDownloadList()->m_hWnd)
    {
        int iTotal;
        uCompleted = theApp.emuledlg->transferwnd->GetDownloadList()->GetCompleteDownloads(-1, iTotal);	 // [Ded]: -1 to get the count of all completed files in all categories
    }
    SetElementHtml(L"completed", CComBSTR(CastItoIShort(uCompleted, false, 0)));
    SetElementHtml(L"freeSpace", CComBSTR(CastItoXBytes(GetFreeTempSpace(-1), false, false)));

//>>> WiZaRd::Static MM
    const uint8 uWndTransparency = GetTransparency();
    if(m_uiLastTransparency != uWndTransparency)
    {
        m_uiLastTransparency = uWndTransparency; //>>> WiZaRd::Static MM
// Avi3k: improve code
//		m_layeredWnd.AddLayeredStyle(m_hWnd);
//		m_layeredWnd.SetTransparentPercentage(m_hWnd, thePrefs.GetMiniMuleTransparency());
        if(uWndTransparency == 0)
            ::SetWindowLong(m_hWnd, GWL_EXSTYLE, ::GetWindowLong(m_hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
        else
        {
            ::SetWindowLong(m_hWnd, GWL_EXSTYLE, ::GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
            BOOL (WINAPI* pfnSetLayeredWndAttribs)(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
            (FARPROC&)pfnSetLayeredWndAttribs = GetProcAddress(GetModuleHandle(L"user32"), "SetLayeredWindowAttributes");
            if (pfnSetLayeredWndAttribs)
                (*pfnSetLayeredWndAttribs)(m_hWnd, 0, (BYTE)(255 * uWndTransparency/100), LWA_ALPHA);
        }
// end Avi3k: improve code
    }
//<<< WiZaRd::Static MM
}

STDMETHODIMP CMiniMule::TranslateUrl(DWORD /*dwTranslate*/, OLECHAR* pchURLIn, OLECHAR** ppchURLOut)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    UNREFERENCED_PARAMETER(pchURLIn);
	TRACE(L"%hs: %ls\n", __FUNCTION__, pchURLIn);
    *ppchURLOut = NULL;
    return S_FALSE;
}

void CMiniMule::_OnBeforeNavigate2(LPDISPATCH pDisp, VARIANT* URL, VARIANT* /*Flags*/, VARIANT* /*TargetFrameName*/, VARIANT* /*PostData*/, VARIANT* /*Headers*/, BOOL* Cancel)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    CString strURL(V_BSTR(URL));
	TRACE(L"%hs: %s\n", __FUNCTION__, strURL);

    // No external links allowed!
    TCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH];
    URL_COMPONENTS Url = {0};
    Url.dwStructSize = sizeof(Url);
    Url.lpszScheme = szScheme;
	Url.dwSchemeLength = _countof(szScheme);
    if (InternetCrackUrl(strURL, 0, 0, &Url) && Url.dwSchemeLength)
    {
        if (Url.nScheme != INTERNET_SCHEME_UNKNOWN  // <absolute local file path>
                && Url.nScheme != INTERNET_SCHEME_RES	// res://...
                && Url.nScheme != INTERNET_SCHEME_FILE)	// file://...
        {
            *Cancel = TRUE;
            return;
        }
    }

    OnBeforeNavigate(pDisp, strURL);
}

void CMiniMule::OnBeforeNavigate(LPDISPATCH pDisp, LPCTSTR pszUrl)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
	TRACE(L"%hs: %s\n", __FUNCTION__, pszUrl);
    CDHtmlDialog::OnBeforeNavigate(pDisp, pszUrl);
}

void CMiniMule::OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR pszUrl)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
	TRACE(L"%hs: %s\n", __FUNCTION__, pszUrl);
    // If the HTML file contains 'OnLoad' scripts, the HTML DOM is fully accessible
    // only after 'DocumentComplete', but not after 'OnNavigateComplete'
    CDHtmlDialog::OnNavigateComplete(pDisp, pszUrl);
}

void CMiniMule::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR pszUrl)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    if (theApp.emuledlg->m_pMiniMule == NULL)
    {
        // FIX ME
        // apparently in some rare cases (high cpu load, fast double clicks) this function is called when the object is destroyed already
        ASSERT(0);
        return;
    }

    CCounter cc(m_iInCallback);

	TRACE(L"%hs: %s\n", __FUNCTION__, pszUrl);
    // If the HTML file contains 'OnLoad' scripts, the HTML DOM is fully accessible
    // only after 'DocumentComplete', but not after 'OnNavigateComplete'
    CDHtmlDialog::OnDocumentComplete(pDisp, pszUrl);

    if (m_bResolveImages)
    {
        TCHAR szModulePath[MAX_PATH];
        DWORD dwModPathLen = GetModuleFileName(AfxGetResourceHandle(), szModulePath, _countof(szModulePath));
        if (dwModPathLen != 0 && dwModPathLen < _countof(szModulePath))
        {
            CString strFilePathUrl(CreateFilePathUrl(szModulePath, INTERNET_SCHEME_RES));

            static const struct
            {
                LPCTSTR pszImgId;
                LPCTSTR pszResourceId;
			} _aImg[] =
			{
				{ L"connectedImg",		L"CONNECTEDNOTNOT.GIF" },
				{ L"uploadImg",			L"UPLOAD.GIF" },
				{ L"downloadImg",		L"DOWNLOAD.GIF" },
				{ L"completedImg",		L"COMPLETED.GIF" },
				{ L"freeSpaceImg",		L"FREESPACE.GIF" },
				{ L"restoreWndImg",		L"RESTOREWINDOW.GIF" },
				{ L"openIncomingImg",	L"OPENINCOMING.GIF" },
				{ L"optionsImg",		L"PREFERENCES.GIF" }
            };

			for (int i = 0; i < _countof(_aImg); ++i)
            {
                CComPtr<IHTMLImgElement> elm;
                GetElementInterface(_aImg[i].pszImgId, &elm);
                if (elm)
                {
                    CString strResourceURL;
					strResourceURL.Format(L"%s/%s", strFilePathUrl, _aImg[i].pszResourceId);
                    elm->put_src(CComBSTR(strResourceURL));
                }
            }

            CComPtr<IHTMLTable> elm;
			GetElementInterface(L"table", &elm);
            if (elm)
            {
                CString strResourceURL;
				strResourceURL.Format(L"%s/%s", strFilePathUrl, L"TABLEBACKGND.GIF");
                elm->put_background(CComBSTR(strResourceURL));
                elm.Release();
            }
        }
    }

    Localize();

    if (m_spHtmlDoc)
    {
        CComQIPtr<IHTMLElement> body;
        if (m_spHtmlDoc->get_body(&body) == S_OK && body)
        {
            // NOTE: The IE control will always use the size of the associated dialog resource (IDD_MINIMULE)
            // as the minimum window size. 'scrollWidth' and 'scrollHeight' will therefore never return values
            // smaller than the size of that window. To have the auto-size working correctly even for
            // very small window sizes, the size of the dialog resource should therefore be kept very small!
            // TODO: Only in debug build: Check the size of the dialog resource right before 'OnInitDialog'.
            CComQIPtr<IHTMLElement2> body2 = body;
            long lScrollWidth = 0;
            long lScrollHeight = 0;
            if (body2->get_scrollWidth(&lScrollWidth) == S_OK && lScrollWidth > 0 && body2->get_scrollHeight(&lScrollHeight) == S_OK && lScrollHeight > 0)
                AutoSizeAndPosition(CSize(lScrollWidth, lScrollHeight));
        }
    }
}

UINT GetTaskbarPos(HWND hwndTaskbar)
{
    if (hwndTaskbar != NULL)
    {
        // See also: Q179908
        APPBARDATA abd = {0};
        abd.cbSize = sizeof abd;
        abd.hWnd = hwndTaskbar;
        SHAppBarMessage(ABM_GETTASKBARPOS, &abd);

        // SHAppBarMessage may fail to get the rectangle...
        CRect rcAppBar(abd.rc);
        if (rcAppBar.IsRectEmpty() || rcAppBar.IsRectNull())
            GetWindowRect(hwndTaskbar, &abd.rc);

        if (abd.rc.top == abd.rc.left && abd.rc.bottom > abd.rc.right)
            return ABE_LEFT;
        else if (abd.rc.top == abd.rc.left && abd.rc.bottom < abd.rc.right)
            return ABE_TOP;
        else if (abd.rc.top > abd.rc.left)
            return ABE_BOTTOM;
        return ABE_RIGHT;
    }
    return ABE_BOTTOM;
}

void CMiniMule::AutoSizeAndPosition(CSize sizClient)
{
    TRACE("AutoSizeAndPosition: %dx%d\n", sizClient.cx, sizClient.cy);
    CSize sizDesktop(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    if (sizClient.cx > sizDesktop.cx/2)
        sizClient.cx = sizDesktop.cx/2;
    if (sizClient.cy > sizDesktop.cy/2)
        sizClient.cy = sizDesktop.cy/2;

    CRect rcWnd;
    GetWindowRect(&rcWnd);
    if (sizClient.cx > 0 && sizClient.cy > 0)
    {
        CRect rcClient(0, 0, sizClient.cx, sizClient.cy);
        AdjustWindowRectEx(&rcClient, GetStyle(), FALSE, GetExStyle());
        rcClient.OffsetRect(-rcClient.left, -rcClient.top);
        rcWnd = rcClient;
    }

    CRect rcTaskbar(0, sizDesktop.cy - 34, sizDesktop.cx, sizDesktop.cy);
	HWND hWndTaskbar = ::FindWindow(L"Shell_TrayWnd", NULL);
    if (hWndTaskbar)
        ::GetWindowRect(hWndTaskbar, &rcTaskbar);
    CPoint ptWnd;
    UINT uTaskbarPos = GetTaskbarPos(hWndTaskbar);
    switch (uTaskbarPos)
    {
    case ABE_TOP:
        ptWnd.x = sizDesktop.cx - 8 - rcWnd.Width();
        ptWnd.y = rcTaskbar.Height() + 8;
        break;
    case ABE_LEFT:
        ptWnd.x = rcTaskbar.Width() + 8;
        ptWnd.y = sizDesktop.cy - 8 - rcWnd.Height();
        break;
    case ABE_RIGHT:
        ptWnd.x = sizDesktop.cx - rcTaskbar.Width() - 8 - rcWnd.Width();
        ptWnd.y = sizDesktop.cy - 8 - rcWnd.Height();
        break;
    default:
        ASSERT( uTaskbarPos == ABE_BOTTOM );
        ptWnd.x = sizDesktop.cx - 8 - rcWnd.Width();
        ptWnd.y = sizDesktop.cy - rcTaskbar.Height() - 8 - rcWnd.Height();
        break;
    }

//>>> WiZaRd::Static MM
    //ensure that the dialog isn't visible right after startup...
//	SetWindowPos(NULL, ptWnd.x, ptWnd.y, rcWnd.Width(), rcWnd.Height(), SWP_NOZORDER | SWP_SHOWWINDOW);
    SetWindowPos(NULL, ptWnd.x, ptWnd.y, rcWnd.Width(), rcWnd.Height(), SWP_NOZORDER);
//<<< WiZaRd::Static MM
}

void CMiniMule::CreateAutoCloseTimer()
{
    if (m_uAutoCloseTimer == 0)
        m_uAutoCloseTimer = SetTimer(IDT_AUTO_CLOSE_TIMER, 3000, NULL);
}

void CMiniMule::KillAutoCloseTimer()
{
    if (m_uAutoCloseTimer != 0)
    {
        VERIFY( KillTimer(m_uAutoCloseTimer) );
        m_uAutoCloseTimer = 0;
    }
}

void CMiniMule::OnTimer(UINT nIDEvent)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    if (GetAutoClose() && nIDEvent == m_uAutoCloseTimer)
    {
        KillAutoCloseTimer();

        CPoint pt;
        GetCursorPos(&pt);
        CRect rcWnd;
        GetWindowRect(&rcWnd);
        if (!rcWnd.PtInRect(pt))
//>>> WiZaRd::Static MM
            //PostMessage(WM_CLOSE);
			ShowHide(true);
//<<< WiZaRd::Static MM
        else
            CreateAutoCloseTimer();
    }
    CDHtmlDialog::OnTimer(nIDEvent);
}

void CMiniMule::RestoreMainWindow()
{
    if (theApp.emuledlg->IsRunning() && !theApp.emuledlg->IsWindowVisible())
    {
        if (!theApp.emuledlg->IsPreferencesDlgOpen())
        {
            KillAutoCloseTimer();
//>>> WiZaRd::Static MM
            //m_bRestoreMainWnd = true;
            //PostMessage(WM_CLOSE);
			ShowHide(true);
			theApp.emuledlg->RestoreWindow();
//<<< WiZaRd::Static MM
        }
        else if (thePrefs.IsErrorBeepEnabled())
            MessageBeep(MB_OK);
    }
}

void CMiniMule::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
    CDHtmlDialog::OnNcLButtonDblClk(nHitTest, point);
    if (nHitTest == HTCAPTION)
        RestoreMainWindow();
}

HRESULT CMiniMule::OnRestoreMainWindow(IHTMLElement* /*pElement*/)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    CCounter cc(m_iInCallback);
    RestoreMainWindow();
    return S_OK;
}

HRESULT CMiniMule::OnOpenIncomingFolder(IHTMLElement* /*pElement*/)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    CCounter cc(m_iInCallback);
    if (theApp.emuledlg->IsRunning())
    {
        theApp.emuledlg->SendMessage(WM_COMMAND, MP_HM_OPENINC);
        if (GetAutoClose())
//>>> WiZaRd::Static MM
            //PostMessage(WM_CLOSE);
			ShowHide(true);
//<<< WiZaRd::Static MM
    }
    return S_OK;
}

HRESULT CMiniMule::OnOptions(IHTMLElement* /*pElement*/)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    CCounter cc(m_iInCallback);
    if (theApp.emuledlg->IsRunning())
    {
        // showing the 'Pref' dialog will process the message queue -> timer messages will be dispatched -> kill auto close timer!
        KillAutoCloseTimer();
        if (theApp.emuledlg->ShowPreferences() == -1)
        {
            if (thePrefs.IsErrorBeepEnabled())
                MessageBeep(MB_OK);
        }
        if (GetAutoClose())
            CreateAutoCloseTimer();
    }
    return S_OK;
}

STDMETHODIMP CMiniMule::ShowContextMenu(DWORD /*dwID*/, POINT* /*ppt*/, IUnknown* /*pcmdtReserved*/, IDispatch* /*pdispReserved*/)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    CCounter cc(m_iInCallback);
    // Avoid IE context menu
    return S_OK;	// S_OK = Host displayed its own user interface (UI). MSHTML will not attempt to display its UI.
}

STDMETHODIMP CMiniMule::TranslateAccelerator(LPMSG lpMsg, const GUID* /*pguidCmdGroup*/, DWORD /*nCmdID*/)
{
    ASSERT( GetCurrentThreadId() == g_uMainThreadId );
    CCounter cc(m_iInCallback);
    // Allow only some basic keys
    //
    //TODO: Allow the ESC key (for closing the window); does currently not work properly because
    // we don't get a callback that the window was just hidden(!) by MSHTML.
    switch (lpMsg->message)
    {
    case WM_CHAR:
        switch (lpMsg->wParam)
        {
        case ' ':			// SPACE - Activate a link
            return S_FALSE;	// S_FALSE = Let the control process the key stroke.
        }
        break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        switch (lpMsg->wParam)
        {
        case VK_TAB:		// Cycling through controls which can get the focus
        case VK_SPACE:		// Activate a link
            return S_FALSE; // S_FALSE = Let the control process the key stroke.
        case VK_ESCAPE:
            //TODO: Small problem here.. If the options dialog was open and was closed with ESC,
            //we still get an ESC here too and the HTML window would be closed too..
            //PostMessage(WM_CLOSE);
            break;
        }
        break;
    }

    // Avoid any IE shortcuts (especially F5 (Refresh) which messes up the content)
    return S_OK;	// S_OK = Don't let the control process the key stroke.
}

//>>> WiZaRd::Static MM
void CMiniMule::ShowHide(const bool bHide)
{
	if(m_bVisible == !bHide)
		return;
	m_bVisible = !bHide;

    //we should update before showing it because that way the transparency should be correct
    if(!bHide)
    {
        //we should ensure that the MM is shown on the taskbar
        //if we don't set the pos here then it will show up on the same
        //position where we hid (closed) it
        CComQIPtr<IHTMLElement> body;
        if (m_spHtmlDoc->get_body(&body) == S_OK && body)
        {
            // NOTE: The IE control will always use the size of the associated dialog resource (IDD_MINIMULE)
            // as the minimum window size. 'scrollWidth' and 'scrollHeight' will therefore never return values
            // smaller than the size of that window. To have the auto-size working correctly even for
            // very small window sizes, the size of the dialog resource should therefore be kept very small!
            // TODO: Only in debug build: Check the size of the dialog resource right before 'OnInitDialog'.
            CComQIPtr<IHTMLElement2> body2 = body;
            long lScrollWidth = 0;
            long lScrollHeight = 0;
            if (body2->get_scrollWidth(&lScrollWidth) == S_OK && lScrollWidth > 0 && body2->get_scrollHeight(&lScrollHeight) == S_OK && lScrollHeight > 0)
                AutoSizeAndPosition(CSize(lScrollWidth, lScrollHeight));
        }

        UINT eMuleInOverall = theApp.downloadqueue->GetDatarate();
        UINT eMuleOutOverall = theApp.uploadqueue->GetDatarate();
        UpdateContent(eMuleOutOverall, eMuleInOverall);
    }
    BOOL (WINAPI *pfnAnimateWindow)(HWND hWnd, DWORD dwTime, DWORD dwFlags);
    (FARPROC&)pfnAnimateWindow = GetProcAddress(GetModuleHandle(L"user32"), "AnimateWindow");
    if((!thePrefs.IsRunningAeroGlassTheme() || bHide) && pfnAnimateWindow)
    {
        HWND hWndTaskbar = ::FindWindow(L"Shell_TrayWnd", 0);
        CRect rcTaskbar;
        if(hWndTaskbar)
            ::GetWindowRect(hWndTaskbar, &rcTaskbar);
        //was AW_BLEND | AW_CENTER
        //for a roll-effect remove the AW_SLIDE | part
        UINT rollFlag = (AW_BLEND | AW_CENTER) | (bHide ? AW_HIDE : NULL);
        /*const UINT taskPos = GetTaskbarPos(hWndTaskbar);
        if(taskPos == ABE_TOP || taskPos == ABE_BOTTOM)
        {
            // Taskbar is on top or on bottom
            BOOL bTop = taskPos == ABE_TOP;
            if(bHide)
                bTop = !bTop;
            //if we SHOW it then we must roll/slide from the taskbar...
            //if we HIDE it then we must roll/slide to the taskbar...
            rollFlag |= bTop ? AW_VER_POSITIVE : AW_VER_NEGATIVE;
        }
        else
        {
            // Taskbar is on left or on right
            BOOL bLeft = taskPos == ABE_LEFT;
            if(bHide)
                bLeft = !bLeft;
            //if we SHOW it then we must roll/slide from the taskbar...
            //if we HIDE it then we must roll/slide to the taskbar...
            rollFlag |= bLeft ? AW_HOR_POSITIVE : AW_HOR_NEGATIVE;
        }*/
        //I think it looks nicer with a bit more time :)
        //(*pfnAnimateWindow)(m_hWnd, 200, rollFlag);
        (*pfnAnimateWindow)(m_hWnd, 500, rollFlag);
    }
    else
    {
        ShowWindow(bHide ? SW_HIDE : SW_SHOW);
    }
}
//<<< WiZaRd::Static MM

bool CMiniMule::GetAutoClose() const
{
    return theApp.GetProfileInt(MOD_VERSION_PLAIN, L"MiniMuleAutoClose", 0)!=0;
}

uint8 CMiniMule::GetTransparency() const
{
    return (uint8)min(theApp.GetProfileInt(MOD_VERSION_PLAIN, L"MiniMuleTransparency", 0), 255);
}

//>>> WiZaRd::Ratio Indicator
void CMiniMule::SetRatioIcon()
{	
	int ratio = min(8, int(GetRatioDouble(theStats.sessionSentBytes, theStats.sessionReceivedBytes)/0.125));
	if(ratio == lastRatio)
		return;

	lastRatio = ratio;

	if(ratioIcons[0].IsEmpty())
	{
		TCHAR szModulePath[MAX_PATH];
		DWORD dwModPathLen = GetModuleFileName(AfxGetResourceHandle(), szModulePath, _countof(szModulePath));
		if(dwModPathLen != 0 && dwModPathLen < _countof(szModulePath))
		{
			CString strFilePathUrl = CreateFilePathUrl(szModulePath, INTERNET_SCHEME_RES);
			
			for (int i = 0; i < ARRSIZE(strRatioSmilies); ++i)
				ratioIcons[i].Format(L"%s/%s.GIF", strFilePathUrl, strRatioSmilies[i]);
		}
	}

	if(!ratioIcons[ratio].IsEmpty())
	{
		CComPtr<IHTMLImgElement> elm;
		GetElementInterface(L"ratioImg", &elm);
		if(elm)
			elm->put_src(CComBSTR(ratioIcons[ratio]));
	}
}
//<<< WiZaRd::Ratio Indicator