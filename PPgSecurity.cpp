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
#include <share.h>
#include "emule.h"
#include "PPgSecurity.h"
#include "OtherFunctions.h"
#include "IPFilter.h"
#include "Preferences.h"
#include "CustomAutoComplete.h"
#include "HttpDownloadDlg.h"
#include "emuledlg.h"
#include "HelpIDs.h"
//>>> WiZaRd::IPFilter-Update
#include ".\Mod\extractfile.h"
//#include "ZipFile.h"
//#include "GZipFile.h"
//#include "RarFile.h"
//<<< WiZaRd::IPFilter-Update
#include "Log.h"
#include "./kademlia/kademlia/Kademlia.h" //>>> WiZaRd::IPFiltering

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool GetMimeType(LPCTSTR pszFilePath, CString& rstrMimeType);

#define	IPFILTERUPDATEURL_STRINGS_PROFILE	_T("AC_IPFilterUpdateURLs.dat")

IMPLEMENT_DYNAMIC(CPPgSecurity, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgSecurity, CPropertyPage)
    ON_BN_CLICKED(IDC_RELOADFILTER, OnReloadIPFilter)
    ON_BN_CLICKED(IDC_EDITFILTER, OnEditIPFilter)
    ON_EN_CHANGE(IDC_FILTERLEVEL, OnSettingsChange)
    ON_BN_CLICKED(IDC_LOADURL, OnLoadIPFFromURL)
    ON_EN_CHANGE(IDC_UPDATEURL, OnEnChangeUpdateUrl)
    ON_BN_CLICKED(IDC_DD,OnDDClicked)
    ON_WM_HELPINFO()
    ON_BN_CLICKED(IDC_RUNASUSER, OnBnClickedRunAsUser)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_SEESHARE1, OnSettingsChange)
    ON_BN_CLICKED(IDC_SEESHARE2, OnSettingsChange)
    ON_BN_CLICKED(IDC_SEESHARE3, OnSettingsChange)
    ON_BN_CLICKED(IDC_ENABLEOBFUSCATION, OnObfuscatedRequestedChange)
    ON_BN_CLICKED(IDC_ONLYOBFUSCATED, OnSettingsChange)
    ON_BN_CLICKED(IDC_CHECK_FILE_OPEN, OnSettingsChange)
    ON_BN_CLICKED(IDC_IPFILTER_AUTOUPDATE, OnSettingsChange)
END_MESSAGE_MAP()

CPPgSecurity::CPPgSecurity()
    : CPropertyPage(CPPgSecurity::IDD)
{
    m_pacIPFilterURL = NULL;
}

CPPgSecurity::~CPPgSecurity()
{
}

void CPPgSecurity::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
}

void CPPgSecurity::LoadSettings(void)
{
    CString strBuffer;

    strBuffer.Format(_T("%i"), thePrefs.filterlevel);
    GetDlgItem(IDC_FILTERLEVEL)->SetWindowText(strBuffer);
    CheckDlgButton(IDC_IPFILTER_AUTOUPDATE, thePrefs.IsAutoUpdateIPFilterEnabled());

    if ((thePrefs.GetWindowsVersion() == _WINVER_XP_ || thePrefs.GetWindowsVersion() == _WINVER_2K_ || thePrefs.GetWindowsVersion() == _WINVER_2003_)
            && thePrefs.m_nCurrentUserDirMode == 2)
        GetDlgItem(IDC_RUNASUSER)->EnableWindow(TRUE);
    else
        GetDlgItem(IDC_RUNASUSER)->EnableWindow(FALSE);
    CheckDlgButton(IDC_RUNASUSER, thePrefs.IsRunAsUserEnabled());

    {
        GetDlgItem(IDC_ENABLEOBFUSCATION)->EnableWindow(TRUE);
        GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(TRUE);
    }

    if (thePrefs.IsClientCryptLayerRequested())
    {
        CheckDlgButton(IDC_ENABLEOBFUSCATION,1);
        GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(TRUE);
    }
    else
    {
        CheckDlgButton(IDC_ENABLEOBFUSCATION,0);
        GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(FALSE);
    }

    CheckDlgButton(IDC_ONLYOBFUSCATED, thePrefs.IsClientCryptLayerRequired());
    CheckDlgButton(IDC_CHECK_FILE_OPEN, thePrefs.GetCheckFileOpen());

    ASSERT(vsfaEverybody == 0);
    ASSERT(vsfaFriends == 1);
    ASSERT(vsfaNobody == 2);
    CheckRadioButton(IDC_SEESHARE1, IDC_SEESHARE3, IDC_SEESHARE1 + thePrefs.m_iSeeShares);
}

BOOL CPPgSecurity::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    InitWindowStyles(this);

    LoadSettings();
    Localize();

    if (thePrefs.GetUseAutocompletion())
    {
        if (!m_pacIPFilterURL)
        {
            m_pacIPFilterURL = new CCustomAutoComplete();
            m_pacIPFilterURL->AddRef();
            if (m_pacIPFilterURL->Bind(::GetDlgItem(m_hWnd, IDC_UPDATEURL), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST | ACO_FILTERPREFIXES))
                m_pacIPFilterURL->LoadList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + IPFILTERUPDATEURL_STRINGS_PROFILE);
        }
        SetDlgItemText(IDC_UPDATEURL, m_pacIPFilterURL->GetItem(0).IsEmpty() ? MOD_IPPFILTER_URL : m_pacIPFilterURL->GetItem(0));
        if (theApp.m_fontSymbol.m_hObject)
        {
            GetDlgItem(IDC_DD)->SetFont(&theApp.m_fontSymbol);
            GetDlgItem(IDC_DD)->SetWindowText(_T("6")); // show a down-arrow
        }
    }
    else
    {
        SetDlgItemText(IDC_UPDATEURL, MOD_IPPFILTER_URL);
        GetDlgItem(IDC_DD)->ShowWindow(SW_HIDE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgSecurity::OnApply()
{
    TCHAR buffer[510];
    if (GetDlgItem(IDC_FILTERLEVEL)->GetWindowTextLength())
    {
        GetDlgItem(IDC_FILTERLEVEL)->GetWindowText(buffer, 4);
        int iNewFilterLevel = _tstoi(buffer);
        if (iNewFilterLevel >= 0 && (UINT)iNewFilterLevel != thePrefs.filterlevel)
        {
            thePrefs.filterlevel = iNewFilterLevel;
        }
    }
    thePrefs.m_bAutoUpdateIPFilter = IsDlgButtonChecked(IDC_IPFILTER_AUTOUPDATE) != 0;

    thePrefs.m_bRunAsUser = IsDlgButtonChecked(IDC_RUNASUSER)!=0;

    thePrefs.m_bCryptLayerRequested = IsDlgButtonChecked(IDC_ENABLEOBFUSCATION) != 0;
    thePrefs.m_bCryptLayerRequired = IsDlgButtonChecked(IDC_ONLYOBFUSCATED) != 0;
    thePrefs.m_bCheckFileOpen = IsDlgButtonChecked(IDC_CHECK_FILE_OPEN) != 0;

    if (IsDlgButtonChecked(IDC_SEESHARE1))
        thePrefs.m_iSeeShares = vsfaEverybody;
    else if (IsDlgButtonChecked(IDC_SEESHARE2))
        thePrefs.m_iSeeShares = vsfaFriends;
    else
        thePrefs.m_iSeeShares = vsfaNobody;

    LoadSettings();
    SetModified(FALSE);
    return CPropertyPage::OnApply();
}

void CPPgSecurity::Localize(void)
{
    if (m_hWnd)
    {
        SetWindowText(GetResString(IDS_SECURITY));
        GetDlgItem(IDC_STATIC_IPFILTER)->SetWindowText(GetResString(IDS_IPFILTER));
        GetDlgItem(IDC_RELOADFILTER)->SetWindowText(GetResString(IDS_SF_RELOAD));
        GetDlgItem(IDC_EDITFILTER)->SetWindowText(GetResString(IDS_EDIT));
        GetDlgItem(IDC_STATIC_FILTERLEVEL)->SetWindowText(GetResString(IDS_FILTERLEVEL)+_T(":"));
        GetDlgItem(IDC_IPFILTER_AUTOUPDATE)->SetWindowText(GetResString(IDS_IPFILTER_AUTOUPDATE));

        GetDlgItem(IDC_SEC_MISC)->SetWindowText(GetResString(IDS_PW_MISC));
        SetDlgItemText(IDC_RUNASUSER,GetResString(IDS_RUNASUSER));

        SetDlgItemText(IDC_STATIC_UPDATEFROM,GetResString(IDS_UPDATEFROM));
        SetDlgItemText(IDC_LOADURL,GetResString(IDS_LOADURL));

        GetDlgItem(IDC_SEEMYSHARE_FRM)->SetWindowText(GetResString(IDS_PW_SHARE));
        GetDlgItem(IDC_SEESHARE1)->SetWindowText(GetResString(IDS_PW_EVER));
        GetDlgItem(IDC_SEESHARE2)->SetWindowText(GetResString(IDS_FSTATUS_FRIENDSONLY));
        GetDlgItem(IDC_SEESHARE3)->SetWindowText(GetResString(IDS_PW_NOONE));

        GetDlgItem(IDC_ONLYOBFUSCATED)->SetWindowText(GetResString(IDS_ONLYOBFUSCATED));
        GetDlgItem(IDC_ENABLEOBFUSCATION)->SetWindowText(GetResString(IDS_ENABLEOBFUSCATION));
        GetDlgItem(IDC_SEC_OBFUSCATIONBOX)->SetWindowText(GetResString(IDS_PROTOCOLOBFUSCATION));
        GetDlgItem(IDC_CHECK_FILE_OPEN)->SetWindowText(GetResString(IDS_CHECK_FILE_OPEN));
    }
}

void CPPgSecurity::OnReloadIPFilter()
{
    CWaitCursor curHourglass;
    theApp.ipfilter->LoadFromDefaultFile();
    Kademlia::CKademlia::RemoveFilteredContacts(); //>>> WiZaRd::IPFiltering
}

void CPPgSecurity::OnEditIPFilter()
{
    ShellExecute(NULL, _T("open"), thePrefs.GetTxtEditor(),
                 _T("\"") + thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + DFLT_IPFILTER_FILENAME _T("\""), NULL, SW_SHOW);
}

void CPPgSecurity::OnLoadIPFFromURL()
{
    CString url;
    GetDlgItemText(IDC_UPDATEURL,url);
    if (!CheckURL(url))
        url = MOD_IPPFILTER_URL;

    // add entered URL to LRU list even if it's not yet known whether we can download from this URL (it's just more convenient this way)
    if (m_pacIPFilterURL && m_pacIPFilterURL->IsBound())
        m_pacIPFilterURL->AddItem(url, 0);

    CString strTempFilePath;
    _tmakepathlimit(strTempFilePath.GetBuffer(MAX_PATH), NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), DFLT_IPFILTER_FILENAME, _T("tmp"));
    strTempFilePath.ReleaseBuffer();

    CHttpDownloadDlg dlgDownload;
    dlgDownload.m_strTitle = GetResString(IDS_DWL_IPFILTERFILE);
    dlgDownload.m_sURLToDownload = url;
    dlgDownload.m_sFileToDownloadInto = strTempFilePath;
    if (dlgDownload.DoModal() != IDOK)
    {
        (void)_tremove(strTempFilePath);
        CString strError = GetResString(IDS_DWLIPFILTERFAILED);
        if (!dlgDownload.GetError().IsEmpty())
            strError += _T("\r\n\r\n") + dlgDownload.GetError();
        AfxMessageBox(strError, MB_ICONERROR);
        return;
    }

    if (!Extract(strTempFilePath, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + DFLT_IPFILTER_FILENAME, DFLT_IPFILTER_FILENAME, true, 2, DFLT_IPFILTER_FILENAME, L"guarding.p2p"))
        return;
    theApp.ipfilter->LoadFromDefaultFile();
    OnReloadIPFilter();

    // In case we received an invalid IP-filter file (e.g. an 404 HTML page with HTTP status "OK"),
    // warn the user that there are no IP-filters available any longer.
    if (theApp.ipfilter->GetIPFilter().GetCount() == 0)
    {
        CString strLoaded;
        strLoaded.Format(GetResString(IDS_IPFILTERLOADED), theApp.ipfilter->GetIPFilter().GetCount());
        CString strError;
        strError.Format(_T("%s\r\n\r\n%s"), GetResString(IDS_DWLIPFILTERFAILED), strLoaded);
        AfxMessageBox(strError, MB_ICONERROR);
    }
}

void CPPgSecurity::OnDestroy()
{
    DeleteDDB();
    CPropertyPage::OnDestroy();
}

void CPPgSecurity::DeleteDDB()
{
    if (m_pacIPFilterURL)
    {
        m_pacIPFilterURL->SaveList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + IPFILTERUPDATEURL_STRINGS_PROFILE);
        m_pacIPFilterURL->Unbind();
        m_pacIPFilterURL->Release();
        m_pacIPFilterURL = NULL;
    }
}

BOOL CPPgSecurity::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {

        if (pMsg->wParam == VK_ESCAPE)
            return FALSE;

        if (pMsg->wParam == VK_DELETE && m_pacIPFilterURL && m_pacIPFilterURL->IsBound() && pMsg->hwnd == GetDlgItem(IDC_UPDATEURL)->m_hWnd)
        {
            if (GetAsyncKeyState(VK_MENU)<0 || GetAsyncKeyState(VK_CONTROL)<0)
                m_pacIPFilterURL->Clear();
            else
                m_pacIPFilterURL->RemoveSelectedItem();
        }

        if (pMsg->wParam == VK_RETURN)
        {
            if (pMsg->hwnd == GetDlgItem(IDC_UPDATEURL)->m_hWnd)
            {
                if (m_pacIPFilterURL && m_pacIPFilterURL->IsBound())
                {
                    CString strText;
                    GetDlgItem(IDC_UPDATEURL)->GetWindowText(strText);
                    if (!strText.IsEmpty())
                    {
                        GetDlgItem(IDC_UPDATEURL)->SetWindowText(_T("")); // this seems to be the only chance to let the dropdown list to disapear
                        GetDlgItem(IDC_UPDATEURL)->SetWindowText(strText);
                        ((CEdit*)GetDlgItem(IDC_UPDATEURL))->SetSel(strText.GetLength(), strText.GetLength());
                    }
                }
                return TRUE;
            }
        }
    }

    return CPropertyPage::PreTranslateMessage(pMsg);
}

void CPPgSecurity::OnEnChangeUpdateUrl()
{
    CString strUrl;
    GetDlgItemText(IDC_UPDATEURL, strUrl);
    GetDlgItem(IDC_LOADURL)->EnableWindow(!strUrl.IsEmpty());
}

void CPPgSecurity::OnDDClicked()
{
    CWnd* box=GetDlgItem(IDC_UPDATEURL);
    box->SetFocus();
    box->SetWindowText(_T(""));
    box->SendMessage(WM_KEYDOWN,VK_DOWN,0x00510001);
}

void CPPgSecurity::OnHelp()
{
    theApp.ShowHelp(eMule_FAQ_Preferences_Security);
}

BOOL CPPgSecurity::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (wParam == ID_HELP)
    {
        OnHelp();
        return TRUE;
    }
    return __super::OnCommand(wParam, lParam);
}

BOOL CPPgSecurity::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    OnHelp();
    return TRUE;
}

void CPPgSecurity::OnBnClickedRunAsUser()
{
    if (((CButton*)GetDlgItem(IDC_RUNASUSER))->GetCheck() == BST_CHECKED)
    {
        if (AfxMessageBox(GetResString(IDS_RAU_WARNING),MB_OKCANCEL | MB_ICONINFORMATION,0) == IDCANCEL)
            ((CButton*)GetDlgItem(IDC_RUNASUSER))->SetCheck(BST_UNCHECKED);
    }
    OnSettingsChange();
}

void CPPgSecurity::OnObfuscatedRequestedChange()
{
    if (IsDlgButtonChecked(IDC_ENABLEOBFUSCATION) == 0)
    {
        GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(FALSE);
        CheckDlgButton(IDC_ONLYOBFUSCATED, 0);
    }
    else
    {
        GetDlgItem(IDC_ENABLEOBFUSCATION)->EnableWindow(TRUE);
        GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(TRUE);
    }
    OnSettingsChange();
}
