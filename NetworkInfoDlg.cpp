#include "stdafx.h"
#include "emule.h"
#include "NetworkInfoDlg.h"
#include "RichEditCtrlX.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "kademlia/kademlia/prefs.h"
#include "kademlia/kademlia/indexed.h"
#include "clientlist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	PREF_INI_SECTION	_T("NetworkInfoDlg")


IMPLEMENT_DYNAMIC(CNetworkInfoDlg, CDialog)

BEGIN_MESSAGE_MAP(CNetworkInfoDlg, CResizableDialog)
END_MESSAGE_MAP()

CNetworkInfoDlg::CNetworkInfoDlg(CWnd* pParent /*=NULL*/)
    : CResizableDialog(CNetworkInfoDlg::IDD, pParent)
{
}

CNetworkInfoDlg::~CNetworkInfoDlg()
{
}

void CNetworkInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_NETWORK_INFO, m_info);
}

BOOL CNetworkInfoDlg::OnInitDialog()
{
    ReplaceRichEditCtrl(GetDlgItem(IDC_NETWORK_INFO), this, GetDlgItem(IDC_NETWORK_INFO_LABEL)->GetFont());
    CResizableDialog::OnInitDialog();
    InitWindowStyles(this);

    AddAnchor(IDC_NETWORK_INFO, TOP_LEFT, BOTTOM_RIGHT);
    EnableSaveRestore(PREF_INI_SECTION);

    SetWindowText(GetResString(IDS_NETWORK_INFO));

    SetDlgItemText(IDC_NETWORK_INFO_LABEL, GetResString(IDS_NETWORK_INFO));

    m_info.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
    m_info.SetAutoURLDetect();
    m_info.SetEventMask(m_info.GetEventMask() | ENM_LINK);

    CHARFORMAT cfDef = {0};
    CHARFORMAT cfBold = {0};

    PARAFORMAT pf = {0};
    pf.cbSize = sizeof pf;
    if (m_info.GetParaFormat(pf))
    {
        pf.dwMask |= PFM_TABSTOPS;
        pf.cTabCount = 4;
        pf.rgxTabs[0] = 900;
        pf.rgxTabs[1] = 1000;
        pf.rgxTabs[2] = 1100;
        pf.rgxTabs[3] = 1200;
        m_info.SetParaFormat(pf);
    }

    cfDef.cbSize = sizeof cfDef;
    if (m_info.GetSelectionCharFormat(cfDef))
    {
        cfBold = cfDef;
        cfBold.dwMask |= CFM_BOLD;
        cfBold.dwEffects |= CFE_BOLD;
    }

    CreateNetworkInfo(m_info, cfDef, cfBold, true);

    return TRUE;
}

void CreateNetworkInfo(CRichEditCtrlX& rCtrl, CHARFORMAT& rcfDef, CHARFORMAT& rcfBold, bool bFullInfo)
{
    CString buffer;

    if (bFullInfo)
    {
        ///////////////////////////////////////////////////////////////////////////
        // Ports Info
        ///////////////////////////////////////////////////////////////////////////
        rCtrl.SetSelectionCharFormat(rcfBold);
        rCtrl << GetResString(IDS_CLIENT) << L"\r\n";
        rCtrl.SetSelectionCharFormat(rcfDef);

        rCtrl << GetResString(IDS_PW_NICK) << L":\t" << thePrefs.GetUserNick() << L"\r\n";
        rCtrl << GetResString(IDS_CD_UHASH) << L"\t" << md4str((uchar*)thePrefs.GetUserHash()) << L"\r\n";
        rCtrl << GetResString(IDS_IP) << L":\t" << ipstr(theApp.GetPublicIP()) << L"\r\n";
//>>> WiZaRd::IPv6 [Xanatos]
#ifdef IPV6_SUPPORT
        if (!theApp.GetPublicIPv6().IsNull())
            rCtrl << GetResString(IDS_IP) << L" (v6):\t" << ipstr(theApp.GetPublicIPv6()) << L"\r\n";
#endif
//<<< WiZaRd::IPv6 [Xanatos]
        rCtrl << _T("TCP-") << GetResString(IDS_PORT) << L":\t" << thePrefs.GetPort() << L"\r\n";
        rCtrl << _T("UDP-") << GetResString(IDS_PORT) << L":\t" << thePrefs.GetUDPPort() << L"\r\n";
        rCtrl << L"\r\n";
    }

    ///////////////////////////////////////////////////////////////////////////
    // Kademlia
    ///////////////////////////////////////////////////////////////////////////
    rCtrl.SetSelectionCharFormat(rcfBold);
    rCtrl << GetResString(IDS_KADEMLIA) << _T(" ") << GetResString(IDS_NETWORK) << L"\r\n";
    rCtrl.SetSelectionCharFormat(rcfDef);

    rCtrl << GetResString(IDS_STATUS) << L":\t";
    if (Kademlia::CKademlia::IsConnected())
    {
        if (Kademlia::CKademlia::IsFirewalled())
            rCtrl << GetResString(IDS_FIREWALLED);
        else
            rCtrl << GetResString(IDS_KADOPEN);
        if (Kademlia::CKademlia::IsRunningInLANMode())
            rCtrl << _T(" (") << GetResString(IDS_LANMODE) << _T(")");
        rCtrl << L"\r\n";
        rCtrl << _T("UDP ") + GetResString(IDS_STATUS) << L":\t";
        if (Kademlia::CUDPFirewallTester::IsFirewalledUDP(true))
            rCtrl << GetResString(IDS_FIREWALLED);
        else if (Kademlia::CUDPFirewallTester::IsVerified())
            rCtrl << GetResString(IDS_KADOPEN);
        else
        {
            CString strTmp = GetResString(IDS_UNVERIFIED);
            strTmp.MakeLower();
            rCtrl << GetResString(IDS_KADOPEN) + _T(" (") + strTmp + _T(")");
        }
        rCtrl << L"\r\n";

        CString IP;
        IP = ipstr(ntohl(Kademlia::CKademlia::GetPrefs()->GetIPAddress()));
        buffer.Format(_T("%s:%i"), IP, thePrefs.GetUDPPort());
        rCtrl << GetResString(IDS_IP) << _T(":") << GetResString(IDS_PORT) << L":\t" << buffer << L"\r\n";

        buffer.Format(_T("%u"),Kademlia::CKademlia::GetPrefs()->GetIPAddress());
        rCtrl << GetResString(IDS_ID) << L":\t" << buffer << L"\r\n";
        if (Kademlia::CKademlia::GetPrefs()->GetUseExternKadPort() && Kademlia::CKademlia::GetPrefs()->GetExternalKadPort() != 0
                && Kademlia::CKademlia::GetPrefs()->GetInternKadPort() != Kademlia::CKademlia::GetPrefs()->GetExternalKadPort())
        {
            buffer.Format(_T("%u"), Kademlia::CKademlia::GetPrefs()->GetExternalKadPort());
            rCtrl << GetResString(IDS_EXTERNUDPPORT) << L":\t" << buffer << L"\r\n";
        }

        if (Kademlia::CUDPFirewallTester::IsFirewalledUDP(true))
        {
            rCtrl << GetResString(IDS_BUDDY) << L":\t";
            switch (theApp.clientlist->GetBuddyStatus())
            {
                case Disconnected:
                    rCtrl << GetResString(IDS_BUDDYNONE);
                    break;
                case Connecting:
                    rCtrl << GetResString(IDS_CONNECTING);
                    break;
                case Connected:
                    rCtrl << GetResString(IDS_CONNECTED);
                    break;
            }
            rCtrl << L"\r\n";
        }

        if (bFullInfo)
        {

            CString sKadID;
            Kademlia::CKademlia::GetPrefs()->GetKadID(&sKadID);
            rCtrl << GetResString(IDS_CD_UHASH) << L"\t" << sKadID << L"\r\n";

            rCtrl << GetResString(IDS_UUSERS) << L":\t" << GetFormatedUInt(Kademlia::CKademlia::GetKademliaUsers()) << _T(" (Experimental: ") <<  GetFormatedUInt(Kademlia::CKademlia::GetKademliaUsers(true)) << _T(")\r\n");
            //rCtrl << GetResString(IDS_UUSERS) << L":\t" << GetFormatedUInt(Kademlia::CKademlia::GetKademliaUsers()) << L"\r\n";
            rCtrl << GetResString(IDS_PW_FILES) << L":\t" << GetFormatedUInt(Kademlia::CKademlia::GetKademliaFiles()) << L"\r\n";
            rCtrl <<  GetResString(IDS_INDEXED) << _T(":\r\n");
            buffer.Format(GetResString(IDS_KADINFO_SRC) , Kademlia::CKademlia::GetIndexed()->m_uTotalIndexSource);
            rCtrl << buffer;
            buffer.Format(GetResString(IDS_KADINFO_KEYW), Kademlia::CKademlia::GetIndexed()->m_uTotalIndexKeyword);
            rCtrl << buffer;
            buffer.Format(_T("\t%s: %u\r\n"), GetResString(IDS_NOTES), Kademlia::CKademlia::GetIndexed()->m_uTotalIndexNotes);
            rCtrl << buffer;
            buffer.Format(_T("\t%s: %u\r\n"), GetResString(IDS_THELOAD), Kademlia::CKademlia::GetIndexed()->m_uTotalIndexLoad);
            rCtrl << buffer;
        }
    }
    else if (Kademlia::CKademlia::IsRunning())
        rCtrl << GetResString(IDS_CONNECTING) << L"\r\n";
    else
        rCtrl << GetResString(IDS_DISCONNECTED) << L"\r\n";
    rCtrl << L"\r\n";

    rCtrl << GetResString(IDS_KADCONTACTLAB) << L":\t" << GetFormatedUInt(GetKadContactCount()) << L"\r\n";
    rCtrl << GetResString(IDS_KADSEARCHLAB) << L":\t" << GetFormatedUInt(GetKadSearchCount()) << L"\r\n";
}
