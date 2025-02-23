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
#include <math.h>
#include "emule.h"
#include "PPgConnection.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "StatisticsDlg.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "HelpIDs.h"
#include "Firewallopener.h"
#include "ListenSocket.h"
#include "ClientUDPSocket.h"
#include "LastCommonRouteFinder.h"
#include "PreferencesDlg.h" //>>> WiZaRd::Automatic Restart


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgConnection, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgConnection, CPropertyPage)
    ON_BN_CLICKED(IDC_STARTTEST, OnStartPortTest)
    ON_EN_CHANGE(IDC_DOWNLOAD_CAP, OnSettingsChange)
    ON_EN_CHANGE(IDC_UDPPORT, OnEnChangeUDP)
    ON_EN_CHANGE(IDC_UPLOAD_CAP, OnSettingsChange)
    ON_EN_CHANGE(IDC_PORT, OnEnChangeTCP)
    ON_EN_CHANGE(IDC_MAXCON, OnSettingsChange)
    ON_EN_CHANGE(IDC_MAXSOURCEPERFILE, OnSettingsChange)
    ON_BN_CLICKED(IDC_ULIMIT_LBL, OnLimiterChange)
    ON_BN_CLICKED(IDC_DLIMIT_LBL, OnLimiterChange)
    ON_WM_HSCROLL()
    ON_WM_HELPINFO()
    ON_BN_CLICKED(IDC_OPENPORTS, OnBnClickedOpenports)
    ON_BN_CLICKED(IDC_PREF_UPNPONSTART, OnSettingsChange)
END_MESSAGE_MAP()

CPPgConnection::CPPgConnection()
    : CPropertyPage(CPPgConnection::IDD)
{
    guardian = false;
    ratioIcon = NULL; //>>> WiZaRd::Ratio Indicator
}

CPPgConnection::~CPPgConnection()
{
//>>> WiZaRd::Ratio Indicator
    if (ratioIcon)
        VERIFY(DestroyIcon(ratioIcon));
//<<< WiZaRd::Ratio Indicator
}

void CPPgConnection::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MAXDOWN_SLIDER, m_ctlMaxDown);
    DDX_Control(pDX, IDC_MAXUP_SLIDER, m_ctlMaxUp);
}

void CPPgConnection::OnEnChangeTCP()
{
    OnEnChangePorts(true);
}

void CPPgConnection::OnEnChangeUDP()
{
    OnEnChangePorts(false);
}

void CPPgConnection::OnEnChangePorts(uint8 istcpport)
{
    // ports unchanged?
    CString buffer;
    GetDlgItem(IDC_PORT)->GetWindowText(buffer);
    uint16 tcp = (uint16)_tstoi(buffer);
    GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer);
    uint16 udp = (uint16)_tstoi(buffer);

    GetDlgItem(IDC_STARTTEST)->EnableWindow(
        tcp == theApp.listensocket->GetConnectedPort() &&
        udp == theApp.clientudp->GetConnectedPort()
    );

    if (istcpport == 1)
        OnSettingsChange();
}

BOOL CPPgConnection::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    InitWindowStyles(this);

    lastRatio = -1; //>>> WiZaRd::Ratio Indicator

    LoadSettings();
    Localize();

    OnEnChangePorts(2);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgConnection::LoadSettings(void)
{
    if (m_hWnd)
    {
        if (thePrefs.maxupload != 0)
            thePrefs.maxdownload = thePrefs.GetMaxDownload();

        CString strBuffer;

        strBuffer.Format(_T("%d"), thePrefs.udpport);
        GetDlgItem(IDC_UDPPORT)->SetWindowText(strBuffer);

        strBuffer.Format(_T("%d"), thePrefs.maxGraphDownloadRate);
        GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(strBuffer);

        m_ctlMaxDown.SetRange(1, thePrefs.maxGraphDownloadRate);
        SetRateSliderTicks(m_ctlMaxDown);

        if (thePrefs.maxGraphUploadRate != UNLIMITED)
            strBuffer.Format(_T("%d"), thePrefs.maxGraphUploadRate);
        else
            strBuffer = _T("0");
        GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(strBuffer);

        m_ctlMaxUp.SetRange(1, thePrefs.GetMaxGraphUploadRate(true));
        SetRateSliderTicks(m_ctlMaxUp);

        CheckDlgButton(IDC_DLIMIT_LBL, (thePrefs.maxdownload != UNLIMITED));
        CheckDlgButton(IDC_ULIMIT_LBL, (thePrefs.maxupload != UNLIMITED));

        m_ctlMaxDown.SetPos((thePrefs.maxdownload != UNLIMITED) ? thePrefs.maxdownload : thePrefs.maxGraphDownloadRate);
        m_ctlMaxUp.SetPos((thePrefs.maxupload != UNLIMITED) ? thePrefs.maxupload : thePrefs.GetMaxGraphUploadRate(true));

        strBuffer.Format(_T("%d"), thePrefs.port);
        GetDlgItem(IDC_PORT)->SetWindowText(strBuffer);

        strBuffer.Format(_T("%d"), thePrefs.maxconnections);
        GetDlgItem(IDC_MAXCON)->SetWindowText(strBuffer);

        if (thePrefs.maxsourceperfile == 0xFFFF)
            GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText(_T("0"));
        else
        {
            strBuffer.Format(_T("%d"), thePrefs.maxsourceperfile);
            GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText(strBuffer);
        }

        // don't try on XP SP2 or higher, not needed there anymore
        if (thePrefs.GetWindowsVersion() == _WINVER_XP_ && IsRunningXPSP2() == 0 && theApp.m_pFirewallOpener->DoesFWConnectionExist())
            GetDlgItem(IDC_OPENPORTS)->ShowWindow(SW_SHOW);
        else
            GetDlgItem(IDC_OPENPORTS)->ShowWindow(SW_HIDE);


        if (thePrefs.GetWindowsVersion() != _WINVER_95_ && thePrefs.GetWindowsVersion() != _WINVER_98_ && thePrefs.GetWindowsVersion() != _WINVER_NT4_)
            GetDlgItem(IDC_PREF_UPNPONSTART)->EnableWindow(true);
        else
            GetDlgItem(IDC_PREF_UPNPONSTART)->EnableWindow(false);

        if (thePrefs.IsUPnPEnabled())
            CheckDlgButton(IDC_PREF_UPNPONSTART, 1);
        else
            CheckDlgButton(IDC_PREF_UPNPONSTART, 0);

        ShowLimitValues();
        OnLimiterChange();
    }
}

BOOL CPPgConnection::OnApply()
{
    TCHAR buffer[510];
    int lastmaxgu = thePrefs.maxGraphUploadRate;
    int lastmaxgd = thePrefs.maxGraphDownloadRate;
    bool bRestartApp = false;

    if (GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowTextLength())
    {
        GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowText(buffer, 20);
        thePrefs.SetMaxGraphDownloadRate(_tstoi(buffer));
    }

    m_ctlMaxDown.SetRange(1, thePrefs.GetMaxGraphDownloadRate(), TRUE);
    SetRateSliderTicks(m_ctlMaxDown);

    if (GetDlgItem(IDC_UPLOAD_CAP)->GetWindowTextLength())
    {
        GetDlgItem(IDC_UPLOAD_CAP)->GetWindowText(buffer, 20);
        thePrefs.SetMaxGraphUploadRate(_tstoi(buffer));
    }

    m_ctlMaxUp.SetRange(1, thePrefs.GetMaxGraphUploadRate(true), TRUE);
    SetRateSliderTicks(m_ctlMaxUp);

    {
        uint16 ulSpeed;

        if (!IsDlgButtonChecked(IDC_ULIMIT_LBL))
            ulSpeed = UNLIMITED;
        else
            ulSpeed = (uint16)m_ctlMaxUp.GetPos();

        if (thePrefs.GetMaxGraphUploadRate(true) < ulSpeed && ulSpeed != UNLIMITED)
            ulSpeed = (uint16)(thePrefs.GetMaxGraphUploadRate(true) * 0.8);

        if (ulSpeed > thePrefs.GetMaxUpload())
        {
            // make USS go up to higher ul limit faster
            theApp.lastCommonRouteFinder->InitiateFastReactionPeriod();
        }

        thePrefs.SetMaxUpload(ulSpeed);
    }

    if (thePrefs.GetMaxUpload() != UNLIMITED)
        m_ctlMaxUp.SetPos(thePrefs.GetMaxUpload());

    if (!IsDlgButtonChecked(IDC_DLIMIT_LBL))
        thePrefs.SetMaxDownload(UNLIMITED);
    else
        thePrefs.SetMaxDownload(m_ctlMaxDown.GetPos());

    if (thePrefs.GetMaxGraphDownloadRate() < thePrefs.GetMaxDownload() && thePrefs.GetMaxDownload() != UNLIMITED)
        thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate() * 0.8));

    if (thePrefs.GetMaxDownload() != UNLIMITED)
        m_ctlMaxDown.SetPos(thePrefs.GetMaxDownload());

    if (GetDlgItem(IDC_PORT)->GetWindowTextLength())
    {
        GetDlgItem(IDC_PORT)->GetWindowText(buffer, 20);
        uint16 nNewPort = ((uint16)_tstoi(buffer)) ? (uint16)_tstoi(buffer) : (uint16)thePrefs.port;
        if (nNewPort != thePrefs.port)
        {
            thePrefs.port = nNewPort;
            if (theApp.IsPortchangeAllowed())
                theApp.listensocket->Rebind();
            else
                bRestartApp = true;
        }
    }

    if (GetDlgItem(IDC_MAXSOURCEPERFILE)->GetWindowTextLength())
    {
        GetDlgItem(IDC_MAXSOURCEPERFILE)->GetWindowText(buffer, 20);
        thePrefs.maxsourceperfile = (_tstoi(buffer)) ? _tstoi(buffer) : 1;
    }

    if (GetDlgItem(IDC_UDPPORT)->GetWindowTextLength())
    {
        GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer, 20);
        uint16 nNewPort = (uint16)_tstoi(buffer) ? (uint16)_tstoi(buffer) : (uint16)0;
        if (nNewPort != thePrefs.udpport)
        {
            thePrefs.udpport = nNewPort;
            if (theApp.IsPortchangeAllowed())
                theApp.clientudp->Rebind();
            else
                bRestartApp = true;
        }
    }

    GetDlgItem(IDC_UDPPORT)->EnableWindow(TRUE);

    if (lastmaxgu != thePrefs.maxGraphUploadRate)
        theApp.emuledlg->statisticswnd->SetARange(false, thePrefs.GetMaxGraphUploadRate(true));
    if (lastmaxgd!=thePrefs.maxGraphDownloadRate)
        theApp.emuledlg->statisticswnd->SetARange(true, thePrefs.maxGraphDownloadRate);

    UINT tempcon = thePrefs.maxconnections;
    if (GetDlgItem(IDC_MAXCON)->GetWindowTextLength())
    {
        GetDlgItem(IDC_MAXCON)->GetWindowText(buffer, 20);
        tempcon = (_tstoi(buffer)) ? _tstoi(buffer) : CPreferences::GetRecommendedMaxConnections();
    }

    if (tempcon > (unsigned)::GetMaxWindowsTCPConnections())
    {
        CString strMessage;
        strMessage.Format(GetResString(IDS_PW_WARNING), GetResString(IDS_PW_MAXC), ::GetMaxWindowsTCPConnections());
        int iResult = AfxMessageBox(strMessage, MB_ICONWARNING | MB_YESNO);
        if (iResult != IDYES)
        {
            //TODO: set focus to max connection?
            strMessage.Format(_T("%d"), thePrefs.maxconnections);
            GetDlgItem(IDC_MAXCON)->SetWindowText(strMessage);
            tempcon = ::GetMaxWindowsTCPConnections();
        }
    }
    thePrefs.maxconnections = tempcon;

    if (IsDlgButtonChecked(IDC_PREF_UPNPONSTART) != 0)
    {
        if (!thePrefs.IsUPnPEnabled())
        {
            thePrefs.m_bEnableUPnP = true;
            theApp.emuledlg->StartUPnP();
        }
    }
    else
        thePrefs.m_bEnableUPnP = false;

    SetModified(FALSE);
    LoadSettings();

    theApp.emuledlg->ShowConnectionState();

    if (bRestartApp)
//>>> WiZaRd::Automatic Restart
        //AfxMessageBox(GetResString(IDS_NOPORTCHANGEPOSSIBLE));
        CPreferencesDlg::PlanRestart();
//<<< WiZaRd::Automatic Restart

    OnEnChangePorts(2);

    return CPropertyPage::OnApply();
}

void CPPgConnection::Localize(void)
{
    if (m_hWnd)
    {
        SetWindowText(GetResString(IDS_PW_CONNECTION));
        GetDlgItem(IDC_CAPACITIES_FRM)->SetWindowText(GetResString(IDS_PW_CON_CAPFRM));
        GetDlgItem(IDC_DCAP_LBL)->SetWindowText(GetResString(IDS_PW_CON_DOWNLBL));
        GetDlgItem(IDC_UCAP_LBL)->SetWindowText(GetResString(IDS_PW_CON_UPLBL));
        GetDlgItem(IDC_LIMITS_FRM)->SetWindowText(GetResString(IDS_PW_CON_LIMITFRM));
        GetDlgItem(IDC_DLIMIT_LBL)->SetWindowText(GetResString(IDS_PW_DOWNL));
        GetDlgItem(IDC_ULIMIT_LBL)->SetWindowText(GetResString(IDS_PW_UPL));
        GetDlgItem(IDC_KBS2)->SetWindowText(GetResString(IDS_KBYTESPERSEC));
        GetDlgItem(IDC_KBS3)->SetWindowText(GetResString(IDS_KBYTESPERSEC));
        ShowLimitValues();
        GetDlgItem(IDC_MAXCONN_FRM)->SetWindowText(GetResString(IDS_PW_CONLIMITS));
        GetDlgItem(IDC_MAXCONLABEL)->SetWindowText(GetResString(IDS_PW_MAXC));
        GetDlgItem(IDC_CLIENTPORT_FRM)->SetWindowText(GetResString(IDS_PW_CLIENTPORT));
        GetDlgItem(IDC_MAXSRC_FRM)->SetWindowText(GetResString(IDS_PW_MAXSOURCES));
        GetDlgItem(IDC_MAXSRCHARD_LBL)->SetWindowText(GetResString(IDS_HARDLIMIT));
        GetDlgItem(IDC_OPENPORTS)->SetWindowText(GetResString(IDS_FO_PREFBUTTON));
        SetDlgItemText(IDC_STARTTEST, GetResString(IDS_STARTTEST));
        GetDlgItem(IDC_PREF_UPNPONSTART)->SetWindowText(GetResString(IDS_UPNPSTART));
    }
}

void CPPgConnection::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SetModified(TRUE);

    if (pScrollBar->GetSafeHwnd() == m_ctlMaxUp.m_hWnd)
    {
//>>> WiZaRd::SessionRatio
        //no need to limit that here, it will be limited dynamically!
        /*
                UINT maxup = m_ctlMaxUp.GetPos();
                UINT maxdown = m_ctlMaxDown.GetPos();
                if (maxup < 4 && maxup*3 < maxdown)
                {
                    m_ctlMaxDown.SetPos(maxup*3);
                }
                if (maxup < 10 && maxup*4 < maxdown)
                {
                    m_ctlMaxDown.SetPos(maxup*4);
                }
        */
//<<< WiZaRd::SessionRatio
    }
    else if (pScrollBar->GetSafeHwnd() == m_ctlMaxDown.m_hWnd)
    {
//>>> WiZaRd::SessionRatio
        //could be removed, too... but I like the idea of animating the users to upload :)
//<<< WiZaRd::SessionRatio
        UINT maxup = m_ctlMaxUp.GetPos();
        UINT maxdown = m_ctlMaxDown.GetPos();
        if (maxdown < 13 && maxup*3 < maxdown)
        {
            m_ctlMaxUp.SetPos((int)ceil((double)maxdown/3));
        }
        if (maxdown < 41 && maxup*4 < maxdown)
        {
            m_ctlMaxUp.SetPos((int)ceil((double)maxdown/4));
        }
    }

    ShowLimitValues();

    UpdateData(false);
    CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgConnection::ShowLimitValues()
{
    CString buffer;

    if (!IsDlgButtonChecked(IDC_ULIMIT_LBL))
        buffer = L"";
    else
        buffer.Format(_T("%u %s"), m_ctlMaxUp.GetPos(), GetResString(IDS_KBYTESPERSEC));
    GetDlgItem(IDC_KBS4)->SetWindowText(buffer);

    if (!IsDlgButtonChecked(IDC_DLIMIT_LBL))
        buffer = L"";
    else
        buffer.Format(_T("%u %s"), m_ctlMaxDown.GetPos(), GetResString(IDS_KBYTESPERSEC));
    GetDlgItem(IDC_KBS1)->SetWindowText(buffer);
    SetRatioIcon(); //>>> WiZaRd::Ratio Indicator
}

void CPPgConnection::OnLimiterChange()
{
    m_ctlMaxDown.ShowWindow(IsDlgButtonChecked(IDC_DLIMIT_LBL) ? SW_SHOW : SW_HIDE);
    m_ctlMaxUp.ShowWindow(IsDlgButtonChecked(IDC_ULIMIT_LBL) ? SW_SHOW : SW_HIDE);

    ShowLimitValues();
    SetModified(TRUE);
}

void CPPgConnection::OnHelp()
{
    theApp.ShowHelp(eMule_FAQ_Preferences_Connection);
}

BOOL CPPgConnection::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (wParam == ID_HELP)
    {
        OnHelp();
        return TRUE;
    }
    return __super::OnCommand(wParam, lParam);
}

BOOL CPPgConnection::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    OnHelp();
    return TRUE;
}

void CPPgConnection::OnBnClickedOpenports()
{
    OnApply();
    theApp.m_pFirewallOpener->RemoveRule(EMULE_DEFAULTRULENAME_UDP);
    theApp.m_pFirewallOpener->RemoveRule(EMULE_DEFAULTRULENAME_TCP);
    bool bAlreadyExisted = false;
    if (theApp.m_pFirewallOpener->DoesRuleExist(thePrefs.GetPort(), NAT_PROTOCOL_TCP) || theApp.m_pFirewallOpener->DoesRuleExist(thePrefs.GetUDPPort(), NAT_PROTOCOL_UDP))
    {
        bAlreadyExisted = true;
    }
    bool bResult = theApp.m_pFirewallOpener->OpenPort(thePrefs.GetPort(), NAT_PROTOCOL_TCP, EMULE_DEFAULTRULENAME_TCP, false);
    if (thePrefs.GetUDPPort() != 0)
        bResult = bResult && theApp.m_pFirewallOpener->OpenPort(thePrefs.GetUDPPort(), NAT_PROTOCOL_UDP, EMULE_DEFAULTRULENAME_UDP, false);
    if (bResult)
    {
        if (!bAlreadyExisted)
            AfxMessageBox(GetResString(IDS_FO_PREF_SUCCCEEDED), MB_ICONINFORMATION | MB_OK);
        else
            // TODO: actually we could offer the user to remove existing rules
            AfxMessageBox(GetResString(IDS_FO_PREF_EXISTED), MB_ICONINFORMATION | MB_OK);
    }
    else
        AfxMessageBox(GetResString(IDS_FO_PREF_FAILED), MB_ICONSTOP | MB_OK);
}

void CPPgConnection::OnStartPortTest()
{
    CString buffer;

    GetDlgItem(IDC_PORT)->GetWindowText(buffer);
    uint16 tcp = (uint16)_tstoi(buffer);

    GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer);
    uint16 udp = (uint16)_tstoi(buffer);

    TriggerPortTest(tcp, udp);
}

void CPPgConnection::SetRateSliderTicks(CSliderCtrl& rRate)
{
    rRate.ClearTics();
    int iMin = 0, iMax = 0;
    rRate.GetRange(iMin, iMax);
    int iDiff = iMax - iMin;
    if (iDiff > 0)
    {
        CRect rc;
        rRate.GetWindowRect(&rc);
        if (rc.Width() > 0)
        {
            int iTic;
            int iPixels = rc.Width() / iDiff;
            if (iPixels >= 6)
                iTic = 1;
            else
            {
                iTic = 10;
                while (rc.Width() / (iDiff / iTic) < 8)
                    iTic *= 10;
            }
            if (iTic)
            {
                for (int i = ((iMin+(iTic-1))/iTic)*iTic; i < iMax; /**/)
                {
                    rRate.SetTic(i);
                    i += iTic;
                }
            }
            rRate.SetPageSize(iTic);
        }
    }
}

//>>> WiZaRd::Ratio Indicator
void CPPgConnection::SetRatioIcon()
{
    CString strBuffer = L"";
    GetDlgItem(IDC_UPLOAD_CAP)->GetWindowText(strBuffer);

    UINT maxUpload = (UINT)_tstoi(strBuffer);
    if (maxUpload == 0)
        maxUpload = thePrefs.GetMaxGraphUploadRate(true);
    UINT upload = m_ctlMaxUp.GetPos();
    if (upload == 0)
        upload = UNLIMITED;

    const int ratio = GetRatioSmileyIndex(upload, maxUpload);
    if (ratio == lastRatio)
        return;

    lastRatio = ratio;

    if (ratioIcon)
        VERIFY(DestroyIcon(ratioIcon));

    ratioIcon = theApp.LoadIcon(strRatioSmilies[ratio], 16, 16);
    ((CStatic*)GetDlgItem(IDC_RATIOICON))->SetIcon(ratioIcon);
}
//<<< WiZaRd::Ratio Indicator