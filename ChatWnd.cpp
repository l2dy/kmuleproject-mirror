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
#include "ChatWnd.h"
#include "HTRichEditCtrl.h"
#include "FriendList.h"
#include "emuledlg.h"
#include "UpDownClient.h"
#include "OtherFunctions.h"
#include "HelpIDs.h"
#include "Opcodes.h"
#include "friend.h"
#include "ClientCredits.h"
#include "IconStatic.h"
#include "UserMsgs.h"
#include "SmileySelector.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	SPLITTER_HORZ_MARGIN	0
#define	SPLITTER_HORZ_WIDTH		4
#define	SPLITTER_HORZ_RANGE_MIN	170
#define	SPLITTER_HORZ_RANGE_MAX	400


// CChatWnd dialog

IMPLEMENT_DYNAMIC(CChatWnd, CDialog)

BEGIN_MESSAGE_MAP(CChatWnd, CResizableDialog)
    ON_WM_KEYDOWN()
    ON_WM_SHOWWINDOW()
    ON_MESSAGE(UM_CLOSETAB, OnCloseTab)
    ON_WM_SYSCOLORCHANGE()
    ON_WM_CTLCOLOR()
    ON_WM_CONTEXTMENU()
    ON_WM_HELPINFO()
    ON_NOTIFY(LVN_ITEMACTIVATE, IDC_FRIENDS_LIST, OnLvnItemActivateFriendList)
    ON_NOTIFY(NM_CLICK, IDC_FRIENDS_LIST, OnNmClickFriendList)
    ON_STN_DBLCLK(IDC_FRIENDSICON, OnStnDblClickFriendIcon)
    ON_BN_CLICKED(IDC_CSEND, OnBnClickedSend)
    ON_BN_CLICKED(IDC_CCLOSE, OnBnClickedClose)
END_MESSAGE_MAP()

CChatWnd::CChatWnd(CWnd* pParent /*=NULL*/)
    : CResizableDialog(CChatWnd::IDD, pParent)
{
    icon_friend = NULL;
    icon_msg = NULL;
    m_pwndSmileySel = NULL;
}

CChatWnd::~CChatWnd()
{
    if (m_pwndSmileySel != NULL)
    {
        m_pwndSmileySel->DestroyWindow();
        delete m_pwndSmileySel;
    }
    if (icon_friend)
        VERIFY(DestroyIcon(icon_friend));
    if (icon_msg)
        VERIFY(DestroyIcon(icon_msg));
}

void CChatWnd::DoDataExchange(CDataExchange* pDX)
{
    CResizableDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CHATSEL, chatselector);
    DDX_Control(pDX, IDC_FRIENDS_LIST, m_FriendListCtrl);
    DDX_Control(pDX, IDC_FRIENDS_MSG, m_cUserInfo);
    DDX_Control(pDX, IDC_TEXT_FORMAT, m_wndFormat);
    DDX_Control(pDX, IDC_CMESSAGE, m_wndMessage);
    DDX_Control(pDX, IDC_CSEND, m_wndSend);
    DDX_Control(pDX, IDC_CCLOSE, m_wndClose);
}

void CChatWnd::OnLvnItemActivateFriendList(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
    UpdateSelectedFriendMsgDetails();
}

void CChatWnd::UpdateSelectedFriendMsgDetails()
{
    int iSel = m_FriendListCtrl.GetSelectionMark();
    if (iSel != -1)
    {
        CFriend* pFriend = (CFriend*)m_FriendListCtrl.GetItemData(iSel);
        ShowFriendMsgDetails(pFriend);
    }
    else
        ShowFriendMsgDetails(NULL);
}

void CChatWnd::ShowFriendMsgDetails(CFriend* pFriend)
{
    if (pFriend)
    {
        CString buffer;

        // Name
        GetDlgItem(IDC_FRIENDS_NAME_EDIT)->SetWindowText(pFriend->GetFriendName());
        // Hash
        GetDlgItem(IDC_FRIENDS_USERHASH_EDIT)->SetWindowText(pFriend->GetFriendHash());
        // Client
        GetDlgItem(IDC_FRIENDS_CLIENTE_EDIT)->SetWindowText(pFriend->GetFriendSoft());
        // Identification
        GetDlgItem(IDC_FRIENDS_IDENTIFICACION_EDIT)->SetWindowText(pFriend->GetIdentState());
        // Upload and downloaded
        GetDlgItem(IDC_FRIENDS_DESCARGADO_EDIT)->SetWindowText(pFriend->GetFriendDownloaded());
        GetDlgItem(IDC_FRIENDS_SUBIDO_EDIT)->SetWindowText(pFriend->GetFriendUploaded());
        GetDlgItem(IDC_FRIENDS_COMMENT_EDIT)->SetWindowText(pFriend->m_strComment); //>>> WiZaRd::FriendComment
    }
    else
    {
        GetDlgItem(IDC_FRIENDS_NAME_EDIT)->SetWindowText(L"-");
        GetDlgItem(IDC_FRIENDS_USERHASH_EDIT)->SetWindowText(L"-");
        GetDlgItem(IDC_FRIENDS_CLIENTE_EDIT)->SetWindowText(L"-");
        GetDlgItem(IDC_FRIENDS_IDENTIFICACION_EDIT)->SetWindowText(L"-");
        GetDlgItem(IDC_FRIENDS_DESCARGADO_EDIT)->SetWindowText(L"-");
        GetDlgItem(IDC_FRIENDS_SUBIDO_EDIT)->SetWindowText(L"-");
        GetDlgItem(IDC_FRIENDS_COMMENT_EDIT)->SetWindowText(L"-"); //>>> WiZaRd::FriendComment
    }
}

BOOL CChatWnd::OnInitDialog()
{
    CResizableDialog::OnInitDialog();
    InitWindowStyles(this);
    SetAllIcons();

    m_wndMessage.SetLimitText(MAX_CLIENT_MSG_LEN);
    if (theApp.m_fontChatEdit.m_hObject)
    {
        m_wndMessage.SendMessage(WM_SETFONT, (WPARAM)theApp.m_fontChatEdit.m_hObject, FALSE);
        CRect rcEdit;
        m_wndMessage.GetWindowRect(&rcEdit);
        ScreenToClient(&rcEdit);
        rcEdit.top -= 2;
        rcEdit.bottom += 2;
        m_wndMessage.MoveWindow(&rcEdit, FALSE);
    }

    chatselector.Init(this);
    m_FriendListCtrl.Init();

    CRect rcSpl;
    m_FriendListCtrl.GetWindowRect(rcSpl);
    ScreenToClient(rcSpl);
    rcSpl.left = rcSpl.right + SPLITTER_HORZ_MARGIN;
    rcSpl.right = rcSpl.left + SPLITTER_HORZ_WIDTH;
    m_wndSplitterHorz.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER_FRIEND);

    // Vista: Remove the TBSTYLE_TRANSPARENT to avoid flickering (can be done only after the toolbar was initially created with TBSTYLE_TRANSPARENT !?)
    m_wndFormat.ModifyStyle((theApp.m_ullComCtrlVer >= MAKEDLLVERULL(6, 16, 0, 0)) ? TBSTYLE_TRANSPARENT : 0, TBSTYLE_TOOLTIPS);
    m_wndFormat.SetExtendedStyle(m_wndFormat.GetExtendedStyle() | TBSTYLE_EX_MIXEDBUTTONS);
    TBBUTTON atb[1] = {0};
    atb[0].iBitmap = 0;
    atb[0].idCommand = IDC_SMILEY;
    atb[0].fsState = TBSTATE_ENABLED;
    atb[0].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
    atb[0].iString = -1;
    m_wndFormat.AddButtons(_countof(atb), atb);

    CSize size;
    m_wndFormat.GetMaxSize(&size);
    if (size.cx < 24) // avoid glitch with COMCTL32 v5.81 and Win2000
        size.cx = 24;
    if (size.cy < 22) // avoid glitch with COMCTL32 v5.81 and Win2000
        size.cy = 22;
    ::SetWindowPos(m_wndFormat, NULL, 0, 0, size.cx, size.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    AddAnchor(IDC_FRIENDSICON, TOP_LEFT);
    AddAnchor(IDC_FRIENDS_LBL, TOP_LEFT);
    AddAnchor(IDC_FRIENDS_NAME, BOTTOM_LEFT);
    AddAnchor(IDC_FRIENDS_USERHASH, BOTTOM_LEFT);
    AddAnchor(IDC_FRIENDS_CLIENT, BOTTOM_LEFT);
    AddAnchor(IDC_FRIENDS_IDENT, BOTTOM_LEFT);
    AddAnchor(IDC_FRIENDS_UPLOADED, BOTTOM_LEFT);
    AddAnchor(IDC_FRIENDS_DOWNLOADED, BOTTOM_LEFT);
    AddAnchor(IDC_FRIENDS_COMMENT, BOTTOM_LEFT); //>>> WiZaRd::FriendComment
    AddAnchor(m_wndSplitterHorz, TOP_LEFT, BOTTOM_LEFT);
    AddAnchor(m_wndFormat, BOTTOM_LEFT);
    AddAnchor(m_wndMessage, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(m_wndSend, BOTTOM_RIGHT);
    AddAnchor(m_wndClose, BOTTOM_RIGHT);

    int iPosStatInit = rcSpl.left;
    int iPosStatNew = thePrefs.GetSplitterbarPositionFriend();
    if (iPosStatNew > SPLITTER_HORZ_RANGE_MAX)
        iPosStatNew = SPLITTER_HORZ_RANGE_MAX;
    else if (iPosStatNew < SPLITTER_HORZ_RANGE_MIN)
        iPosStatNew = SPLITTER_HORZ_RANGE_MIN;
    rcSpl.left = iPosStatNew;
    rcSpl.right = iPosStatNew + SPLITTER_HORZ_WIDTH;
    if (iPosStatNew != iPosStatInit)
    {
        m_wndSplitterHorz.MoveWindow(rcSpl);
        DoResize(iPosStatNew - iPosStatInit);
    }

    Localize();
    theApp.friendlist->ShowFriends();

    return TRUE;
}

void CChatWnd::DoResize(int iDelta)
{
    CSplitterControl::ChangeWidth(&m_FriendListCtrl, iDelta);
    m_FriendListCtrl.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    CSplitterControl::ChangeWidth(&m_cUserInfo, iDelta);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_FRIENDS_NAME_EDIT), iDelta);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_FRIENDS_USERHASH_EDIT), iDelta);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_FRIENDS_CLIENTE_EDIT), iDelta);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_FRIENDS_IDENTIFICACION_EDIT), iDelta);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_FRIENDS_SUBIDO_EDIT), iDelta);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_FRIENDS_DESCARGADO_EDIT), iDelta); //>>> WiZaRd::FriendComment
    CSplitterControl::ChangeWidth(&chatselector, -iDelta, CW_RIGHTALIGN);
    CSplitterControl::ChangePos(GetDlgItem(IDC_MESSAGES_LBL), -iDelta, 0);
    CSplitterControl::ChangePos(GetDlgItem(IDC_MESSAGEICON), -iDelta, 0);
    CSplitterControl::ChangePos(&m_wndFormat, -iDelta, 0);
    CSplitterControl::ChangePos(&m_wndMessage, -iDelta, 0);
    CSplitterControl::ChangeWidth(&m_wndMessage, -iDelta);

    CRect rcSpl;
    m_wndSplitterHorz.GetWindowRect(rcSpl);
    ScreenToClient(rcSpl);
    thePrefs.SetSplitterbarPositionFriend(rcSpl.left);

    RemoveAnchor(m_FriendListCtrl);
    AddAnchor(m_FriendListCtrl, TOP_LEFT, BOTTOM_LEFT);
    RemoveAnchor(m_cUserInfo);
    AddAnchor(m_cUserInfo, BOTTOM_LEFT, BOTTOM_LEFT);
    RemoveAnchor(chatselector);
    AddAnchor(chatselector, TOP_LEFT, BOTTOM_RIGHT);
    RemoveAnchor(IDC_MESSAGES_LBL);
    AddAnchor(IDC_MESSAGES_LBL, TOP_LEFT);
    RemoveAnchor(IDC_MESSAGEICON);
    AddAnchor(IDC_MESSAGEICON, TOP_LEFT);
    RemoveAnchor(IDC_FRIENDS_NAME_EDIT);
    AddAnchor(IDC_FRIENDS_NAME_EDIT, BOTTOM_LEFT);
    RemoveAnchor(IDC_FRIENDS_USERHASH_EDIT);
    AddAnchor(IDC_FRIENDS_USERHASH_EDIT, BOTTOM_LEFT);
    RemoveAnchor(IDC_FRIENDS_CLIENTE_EDIT);
    AddAnchor(IDC_FRIENDS_CLIENTE_EDIT, BOTTOM_LEFT);
    RemoveAnchor(IDC_FRIENDS_IDENTIFICACION_EDIT);
    AddAnchor(IDC_FRIENDS_IDENTIFICACION_EDIT, BOTTOM_LEFT);
    RemoveAnchor(IDC_FRIENDS_SUBIDO_EDIT);
    AddAnchor(IDC_FRIENDS_SUBIDO_EDIT, BOTTOM_LEFT);
    RemoveAnchor(IDC_FRIENDS_DESCARGADO_EDIT);
    AddAnchor(IDC_FRIENDS_DESCARGADO_EDIT, BOTTOM_LEFT);
//>>> WiZaRd::FriendComment
    RemoveAnchor(IDC_FRIENDS_COMMENT_EDIT);
    AddAnchor(IDC_FRIENDS_COMMENT_EDIT, BOTTOM_LEFT);
//<<< WiZaRd::FriendComment
    RemoveAnchor(m_wndSplitterHorz);
    AddAnchor(m_wndSplitterHorz, TOP_LEFT, BOTTOM_LEFT);
    RemoveAnchor(m_wndFormat);
    AddAnchor(m_wndFormat, BOTTOM_LEFT);
    RemoveAnchor(m_wndMessage);
    AddAnchor(m_wndMessage, BOTTOM_LEFT, BOTTOM_RIGHT);
    RemoveAnchor(m_wndSend);
    AddAnchor(m_wndSend, BOTTOM_RIGHT);
    RemoveAnchor(m_wndClose);
    AddAnchor(m_wndClose, BOTTOM_RIGHT);

    CRect rcWnd;
    GetWindowRect(rcWnd);
    ScreenToClient(rcWnd);
    m_wndSplitterHorz.SetRange(rcWnd.left + SPLITTER_HORZ_RANGE_MIN + SPLITTER_HORZ_WIDTH/2,
                               rcWnd.left + SPLITTER_HORZ_RANGE_MAX - SPLITTER_HORZ_WIDTH/2);

    Invalidate();
    UpdateWindow();
}

LRESULT CChatWnd::DefWindowProc(UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        case WM_PAINT:
            if (m_wndSplitterHorz)
            {
                CRect rcWnd;
                GetWindowRect(rcWnd);
                if (rcWnd.Width() > 0)
                {
                    CRect rcSpl;
                    m_FriendListCtrl.GetWindowRect(rcSpl);
                    ScreenToClient(rcSpl);
                    rcSpl.left = rcSpl.right + SPLITTER_HORZ_MARGIN;
                    rcSpl.right = rcSpl.left + SPLITTER_HORZ_WIDTH;
                    ScreenToClient(rcWnd);
                    rcSpl.bottom = rcWnd.bottom - 6;
                    m_wndSplitterHorz.MoveWindow(rcSpl, TRUE);
                }
            }
            break;

        case WM_NOTIFY:
            if (wParam == IDC_SPLITTER_FRIEND)
            {
                SPC_NMHDR* pHdr = (SPC_NMHDR*)lParam;
                DoResize(pHdr->delta);
            }
            break;

        case WM_SIZE:
            if (m_wndSplitterHorz)
            {
                CRect rcWnd;
                GetWindowRect(rcWnd);
                ScreenToClient(rcWnd);
                m_wndSplitterHorz.SetRange(rcWnd.left + SPLITTER_HORZ_RANGE_MIN + SPLITTER_HORZ_WIDTH/2,
                                           rcWnd.left + SPLITTER_HORZ_RANGE_MAX - SPLITTER_HORZ_WIDTH/2);
            }
            break;
    }
    return CResizableDialog::DefWindowProc(uMessage, wParam, lParam);
}

void CChatWnd::StartSession(CUpDownClient* client)
{
    if (!client->GetUserName())
        return;
    theApp.emuledlg->SetActiveDialog(this);
    chatselector.StartSession(client, true);
}

void CChatWnd::OnShowWindow(BOOL bShow, UINT /*nStatus*/)
{
    if (bShow)
        chatselector.ShowChat();
}

BOOL CChatWnd::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        // Don't handle Ctrl+Tab in this window. It will be handled by main window.
        if (pMsg->wParam == VK_TAB && GetAsyncKeyState(VK_CONTROL) < 0)
            return FALSE;

        if (pMsg->wParam == VK_RETURN)
        {
            if (pMsg->hwnd == m_wndMessage)
                OnBnClickedSend();
        }

        if (pMsg->hwnd == m_wndMessage && (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN))
        {
            ScrollHistory(pMsg->wParam == VK_DOWN);
            return TRUE;
        }
    }
    else if (pMsg->message == WM_KEYUP)
    {
        if (pMsg->hwnd == m_FriendListCtrl.m_hWnd)
            OnLvnItemActivateFriendList(0, 0);
    }

    return CResizableDialog::PreTranslateMessage(pMsg);
}

void CChatWnd::OnNmClickFriendList(NMHDR *pNMHDR, LRESULT *pResult)
{
    OnLvnItemActivateFriendList(pNMHDR, pResult);
    *pResult = 0;
}

void CChatWnd::SetAllIcons()
{
    if (icon_friend)
        VERIFY(DestroyIcon(icon_friend));
    if (icon_msg)
        VERIFY(DestroyIcon(icon_msg));
    icon_friend = theApp.LoadIcon(_T("Friend"), 16, 16);
    icon_msg = theApp.LoadIcon(_T("Message"), 16, 16);
    ((CStatic*)GetDlgItem(IDC_MESSAGEICON))->SetIcon(icon_msg);
    ((CStatic*)GetDlgItem(IDC_FRIENDSICON))->SetIcon(icon_friend);
    m_cUserInfo.SetIcon(_T("Info"));

    CImageList iml;
    iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
    iml.Add(CTempIconLoader(_T("Smiley_Smile")));
    CImageList *pimlOld = m_wndFormat.SetImageList(&iml);
    iml.Detach();
    if (pimlOld)
        pimlOld->DeleteImageList();
}

void CChatWnd::Localize()
{
    GetDlgItem(IDC_FRIENDS_LBL)->SetWindowText(GetResString(IDS_CW_FRIENDS));
    GetDlgItem(IDC_MESSAGES_LBL)->SetWindowText(GetResString(IDS_CW_MESSAGES));
    m_cUserInfo.SetWindowText(GetResString(IDS_INFO));
    GetDlgItem(IDC_FRIENDS_DOWNLOADED)->SetWindowText(GetResString(IDS_CHAT_DOWNLOADED));
    GetDlgItem(IDC_FRIENDS_UPLOADED)->SetWindowText(GetResString(IDS_CHAT_UPLOADED));
    GetDlgItem(IDC_FRIENDS_IDENT)->SetWindowText(GetResString(IDS_CHAT_IDENT));
    GetDlgItem(IDC_FRIENDS_CLIENT)->SetWindowText(GetResString(IDS_CD_CSOFT));
    GetDlgItem(IDC_FRIENDS_NAME)->SetWindowText(GetResString(IDS_CD_UNAME));
    GetDlgItem(IDC_FRIENDS_COMMENT)->SetWindowText(GetResString(IDS_COMMENT)); //>>> WiZaRd::FriendComment
    GetDlgItem(IDC_FRIENDS_USERHASH)->SetWindowText(GetResString(IDS_CD_UHASH));
    m_wndSend.SetWindowText(GetResString(IDS_CW_SEND));
    m_wndClose.SetWindowText(GetResString(IDS_CW_CLOSE));
    m_wndFormat.SetBtnText(IDC_SMILEY, _T("Smileys"));
    m_FriendListCtrl.Localize();
}

LRESULT CChatWnd::OnCloseTab(WPARAM wParam, LPARAM /*lParam*/)
{
    TCITEM item = {0};
    item.mask = TCIF_PARAM;
    if (chatselector.GetItem((int)wParam, &item))
        chatselector.EndSession(((CChatItem*)item.lParam)->client);
    return TRUE;
}

void CChatWnd::ScrollHistory(bool down)
{
    CChatItem* ci = chatselector.GetCurrentChatItem();
    if (ci == NULL)
        return;

    if ((ci->history_pos == 0 && !down) || (ci->history_pos == ci->history.GetCount() && down))
        return;
    if (down)
        ++ci->history_pos;
    else
        --ci->history_pos;

    CString strBuffer = (ci->history_pos == ci->history.GetCount()) ? L"" : ci->history.GetAt(ci->history_pos);
    m_wndMessage.SetWindowText(strBuffer);
    m_wndMessage.SetSel(strBuffer.GetLength(), strBuffer.GetLength());
}

void CChatWnd::OnSysColorChange()
{
    CResizableDialog::OnSysColorChange();
    SetAllIcons();
}

void CChatWnd::UpdateFriendlistCount(UINT count)
{
    CString strTemp;
    strTemp.Format(_T(" (%i)"), count);
    GetDlgItem(IDC_FRIENDS_LBL)->SetWindowText(GetResString(IDS_CW_FRIENDS) + strTemp);
}

BOOL CChatWnd::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    theApp.ShowHelp(eMule_FAQ_GUI_Messages);
    return TRUE;
}

void CChatWnd::OnStnDblClickFriendIcon()
{
    theApp.emuledlg->ShowPreferences(IDD_PPG_SECURITY);
}

BOOL CChatWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case IDC_SMILEY:
            OnBnClickedSmiley();
            break;
    }
    return CResizableDialog::OnCommand(wParam, lParam);
}

void CChatWnd::OnBnClickedSmiley()
{
    if (m_pwndSmileySel)
    {
        m_pwndSmileySel->DestroyWindow();
        delete m_pwndSmileySel;
        m_pwndSmileySel = NULL;
    }
    m_pwndSmileySel = new CSmileySelector;

    CRect rcBtn;
    m_wndFormat.GetWindowRect(&rcBtn);
    rcBtn.top -= 2;

    if (!m_pwndSmileySel->Create(this, &rcBtn, &m_wndMessage))
    {
        delete m_pwndSmileySel;
        m_pwndSmileySel = NULL;
    }
}

void CChatWnd::OnBnClickedClose()
{
//>>> Tux::Fix
    CString strMessage;
    m_wndMessage.GetWindowText(strMessage);
    if (!strMessage.IsEmpty())
        m_wndMessage.SetWindowText(L"");
//<<< Tux::Fix
    chatselector.EndSession();
}

void CChatWnd::OnBnClickedSend()
{
    CString strMessage;
    m_wndMessage.GetWindowText(strMessage);
    strMessage.Trim();
    if (!strMessage.IsEmpty())
    {
        if (chatselector.SendMessage(strMessage))
            m_wndMessage.SetWindowText(L"");
    }

    m_wndMessage.SetFocus();
}

HBRUSH CChatWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = theApp.emuledlg->GetCtlColor(pDC, pWnd, nCtlColor);
    if (hbr)
        return hbr;
    return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}
