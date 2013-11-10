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
#include "QueueListCtrl.h"
#include "OtherFunctions.h"
#include "MenuCmds.h"
#include "ClientDetailDialog.h"
#include "Exceptions.h"
#include "emuledlg.h"
#include "FriendList.h"
#include "UploadQueue.h"
#include "UpDownClient.h"
#include "TransferDlg.h"
#include "MemDC.h"
#include "SharedFileList.h"
#include "ClientCredits.h"
#include "PartFile.h"
#include "ChatWnd.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "kademlia/net/KademliaUDPListener.h"
#include "Log.h"
#include "./Mod/ModIconMapping.h" //>>> WiZaRd::ModIconMappings

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CQueueListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CQueueListCtrl, CMuleListCtrl)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnClick)
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetDispInfo)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnNmDblClk)
    ON_WM_CONTEXTMENU()
    ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CQueueListCtrl::CQueueListCtrl()
    : CListCtrlItemWalk(this)
{
    SetGeneralPurposeFind(true);
    SetSkinKey(L"QueuedLv");

    // Barry - Refresh the queue every 10 secs
    VERIFY((m_hTimer = ::SetTimer(NULL, NULL, 10000, QueueUpdateTimer)) != NULL);
    if (thePrefs.GetVerbose() && !m_hTimer)
        AddDebugLogLine(true,_T("Failed to create 'queue list control' timer - %s"),GetErrorMessage(GetLastError()));
}

CQueueListCtrl::~CQueueListCtrl()
{
    if (m_hTimer)
        VERIFY(::KillTimer(NULL, m_hTimer));
}

void CQueueListCtrl::Init()
{
    SetPrefsKey(_T("QueueListCtrl"));
    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

    InsertColumn(0, GetResString(IDS_QL_USERNAME),	LVCFMT_LEFT, DFLT_CLIENTNAME_COL_WIDTH);
    InsertColumn(1, GetResString(IDS_FILE),			LVCFMT_LEFT, DFLT_FILENAME_COL_WIDTH);
    InsertColumn(2, GetResString(IDS_FILEPRIO),		LVCFMT_LEFT, DFLT_PRIORITY_COL_WIDTH);
    InsertColumn(3, GetResString(IDS_QL_RATING),	LVCFMT_LEFT,  60);
    InsertColumn(4, GetResString(IDS_SCORE),		LVCFMT_LEFT,  60);
    InsertColumn(5, GetResString(IDS_ASKED),		LVCFMT_LEFT,  60);
    InsertColumn(6, GetResString(IDS_LASTSEEN),		LVCFMT_LEFT, 110);
    InsertColumn(7, GetResString(IDS_ENTERQUEUE),	LVCFMT_LEFT, 110);
    InsertColumn(8, GetResString(IDS_BANNED),		LVCFMT_LEFT,  60);
    InsertColumn(9, GetResString(IDS_UPSTATUS),		LVCFMT_LEFT, DFLT_PARTSTATUS_COL_WIDTH);
    InsertColumn(10, GetResString(IDS_CD_CSOFT),	LVCFMT_LEFT, DFLT_CLIENTSOFT_COL_WIDTH);
#ifdef _DEBUG
    InsertColumn(11, GetResString(IDS_COMPLSOURCES),LVCFMT_LEFT,   70);
#endif

    SetAllIcons();
    Localize();
    LoadSettings();
    SetSortArrow();
    SortItems(SortProc, GetSortItem() + (GetSortAscending() ? 0 : 100));
}

void CQueueListCtrl::Localize()
{
    CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
    HDITEM hdi;
    hdi.mask = HDI_TEXT;

    CString strRes;
    strRes = GetResString(IDS_QL_USERNAME);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(0, &hdi);

    strRes = GetResString(IDS_FILE);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(1, &hdi);

    strRes = GetResString(IDS_FILEPRIO);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(2, &hdi);

    strRes = GetResString(IDS_QL_RATING);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(3, &hdi);

    strRes = GetResString(IDS_SCORE);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(4, &hdi);

    strRes = GetResString(IDS_ASKED);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(5, &hdi);

    strRes = GetResString(IDS_LASTSEEN);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(6, &hdi);

    strRes = GetResString(IDS_ENTERQUEUE);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(7, &hdi);

    strRes = GetResString(IDS_BANNED);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(8, &hdi);

    strRes = GetResString(IDS_UPSTATUS);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(9, &hdi);

    strRes = GetResString(IDS_CD_CSOFT);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(10, &hdi);

#ifdef _DEBUG
    strRes = GetResString(IDS_COMPLSOURCES);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(11, &hdi);
#endif
}

void CQueueListCtrl::OnSysColorChange()
{
    CMuleListCtrl::OnSysColorChange();
    SetAllIcons();
}

void CQueueListCtrl::SetAllIcons()
{
    ApplyImageList(NULL);
    m_ImageList.DeleteImageList();
    m_ImageList.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
    FillClientIconImageList(m_ImageList);
    m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
    m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("OverlayObfu"))), 2);
    m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("OverlaySecureObfu"))), 3);
    // Apply the image list also to the listview control, even if we use our own 'DrawItem'.
    // This is needed to give the listview control a chance to initialize the row height.
    ASSERT((GetStyle() & LVS_SHAREIMAGELISTS) != 0);
    VERIFY(ApplyImageList(m_ImageList) == NULL);
}

void CQueueListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if (!theApp.emuledlg->IsRunning())
        return;
    if (!lpDrawItemStruct->itemData)
        return;

    CCustomMemDC dc(CDC::FromHandle(lpDrawItemStruct->hDC), &lpDrawItemStruct->rcItem);
    BOOL bCtrlFocused;
    InitItemMemDC(dc, lpDrawItemStruct, bCtrlFocused);
    CRect cur_rec(lpDrawItemStruct->rcItem);
    CRect rcClient;
    GetClientRect(&rcClient);
    const CUpDownClient *client = (CUpDownClient *)lpDrawItemStruct->itemData;

    CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
    int iCount = pHeaderCtrl->GetItemCount();
    cur_rec.right = cur_rec.left - sm_iLabelOffset;
    cur_rec.left += sm_iIconOffset;
    for (int iCurrent = 0; iCurrent < iCount; iCurrent++)
    {
        int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
        if (!IsColumnHidden(iColumn))
        {
            UINT uDrawTextAlignment;
            int iColumnWidth = GetColumnWidth(iColumn, uDrawTextAlignment);
            cur_rec.right += iColumnWidth;
            if (cur_rec.left < cur_rec.right && HaveIntersection(rcClient, cur_rec))
            {
                TCHAR szItem[1024];
                GetItemDisplayText(client, iColumn, szItem, _countof(szItem));
                switch (iColumn)
                {
                    case 0:
                    {
//>>> WiZaRd::ClientAnalyzer
                        int plusminus = 0;
                        int iCAIconIndex = client->GetAnalyzerIconIndex();
                        if (iCAIconIndex != -1)
                        {
                            int iIconPosY = (cur_rec.Height() > 16) ? ((cur_rec.Height() - 16) / 2) : 1;
                            POINT point = { cur_rec.left, cur_rec.top + iIconPosY };
                            m_ImageList.Draw(dc, 18 + iCAIconIndex, point, ILD_NORMAL);
                            cur_rec.left += 17;
                            plusminus += 17;
                        }
//<<< WiZaRd::ClientAnalyzer
                        int iImage = GetClientImageIndex(client->IsFriend(), client->GetClientSoft(), client->Credits() && client->Credits()->GetScoreRatio(client->GetIP()) > 1, client->ExtProtocolAvailable());

                        UINT nOverlayImage = 0;
                        if ((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED))
                            nOverlayImage |= 1;
                        if (client->IsObfuscatedConnectionEstablished())
                            nOverlayImage |= 2;
                        int iIconPosY = (cur_rec.Height() > 16) ? ((cur_rec.Height() - 16) / 2) : 1;
                        POINT point = { cur_rec.left, cur_rec.top + iIconPosY };
                        m_ImageList.Draw(dc, iImage, point, ILD_NORMAL | INDEXTOOVERLAYMASK(nOverlayImage));

                        cur_rec.left += 16 + sm_iLabelOffset;

//>>> WiZaRd::ModIconMappings
                        int icoindex = client->GetModIconIndex();
                        if (icoindex != MODMAP_NONE)
                        {
                            POINT point = { cur_rec.left, cur_rec.top + iIconPosY };
                            theApp.theModIconMap->DrawModIcon(dc, icoindex, point, ILD_NORMAL);
                            cur_rec.left += 17;
                            plusminus += 17;
                        }
//<<< WiZaRd::ModIconMappings

                        dc.DrawText(szItem, -1, &cur_rec, MLC_DT_TEXT | uDrawTextAlignment);
                        cur_rec.left -= 16;
                        cur_rec.right -= sm_iSubItemInset;
                        cur_rec.left -= plusminus; //>>> WiZaRd::ClientAnalyzer
                        break;
                    }

                    case 9:
                        if (client->GetUpPartCount())
                        {
                            cur_rec.bottom--;
                            cur_rec.top++;
                            client->DrawUpStatusBar(dc, &cur_rec, false, thePrefs.UseFlatBar());
                            cur_rec.bottom++;
                            cur_rec.top--;
                        }
                        break;

                    default:
                        dc.DrawText(szItem, -1, &cur_rec, MLC_DT_TEXT | uDrawTextAlignment);
                        break;
                }
            }
            cur_rec.left += iColumnWidth;
        }
    }

    DrawFocusRect(dc, lpDrawItemStruct->rcItem, lpDrawItemStruct->itemState & ODS_FOCUS, bCtrlFocused, lpDrawItemStruct->itemState & ODS_SELECTED);
}

void CQueueListCtrl::GetItemDisplayText(const CUpDownClient *client, int iSubItem, LPTSTR pszText, int cchTextMax)
{
    if (pszText == NULL || cchTextMax <= 0)
    {
        ASSERT(0);
        return;
    }
    pszText[0] = L'\0';
    switch (iSubItem)
    {
        case 0:
            if (client->GetUserName() == NULL)
                _sntprintf(pszText, cchTextMax, _T("(%s)"), GetResString(IDS_UNKNOWN));
            else
                _tcsncpy(pszText, client->GetUserName(), cchTextMax);
            break;

        case 1:
        {
            const CKnownFile *file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
            _tcsncpy(pszText, file != NULL ? file->GetFileName() : L"", cchTextMax);
            break;
        }

        case 2:
        {
            const CKnownFile *file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
            if (file)
                _tcsncpy(pszText, file->GetUpPriorityDisplayString(), cchTextMax); //>>> WiZaRd::PowerShare
            break;
        }

        case 3:
            _sntprintf(pszText, cchTextMax, _T("%i"), client->GetScore(false, false, true));
            break;

        case 4:
            if (client->HasLowID())
            {
//>>> WiZaRd::Fix for LowID slots only on connection [VQB]
                //if (client->m_bAddNextConnect)
                //    _sntprintf(pszText, cchTextMax, _T("%i ****"),client->GetScore(false));
                if (client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)
                    _sntprintf(pszText, cchTextMax, L"%i **** Awaited reconnect %s", client->GetScore(false), CastSecondsToHM((::GetTickCount()-client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)/1000));
//<<< WiZaRd::Fix for LowID slots only on connection [VQB]
                else
                    _sntprintf(pszText, cchTextMax, _T("%i (%s)"),client->GetScore(false), GetResString(IDS_IDLOW));
            }
            else
                _sntprintf(pszText, cchTextMax, _T("%i"), client->GetScore(false));
            break;

        case 5:
            _sntprintf(pszText, cchTextMax, _T("%i"), client->GetAskedCount());
            break;

        case 6:
            _tcsncpy(pszText, CastSecondsToHM((GetTickCount() - client->GetLastUpRequest()) / 1000), cchTextMax);
            break;

        case 7:
            _tcsncpy(pszText, CastSecondsToHM((GetTickCount() - client->GetWaitStartTime()) / 1000), cchTextMax);
            break;

        case 8:
            _tcsncpy(pszText, GetResString(client->IsBanned() ? IDS_YES : IDS_NO), cchTextMax);
            break;

        case 9:
            _tcsncpy(pszText, GetResString(IDS_UPSTATUS), cchTextMax);
            break;

        case 10:
            _tcsncpy(pszText, client->DbgGetFullClientSoftVer(), cchTextMax);
            break;

#ifdef _DEBUG
        case 11:
            if (client->GetUpCompleteSourcesCount() != _UI16_MAX)
                _sntprintf(pszText, cchTextMax, L"%u", client->GetUpCompleteSourcesCount());
            else
                _tcsncpy(pszText, L"-", cchTextMax);
            break;
#endif
    }
    pszText[cchTextMax - 1] = L'\0';
}

void CQueueListCtrl::OnLvnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
    if (theApp.emuledlg->IsRunning())
    {
        // Although we have an owner drawn listview control we store the text for the primary item in the listview, to be
        // capable of quick searching those items via the keyboard. Because our listview items may change their contents,
        // we do this via a text callback function. The listview control will send us the LVN_DISPINFO notification if
        // it needs to know the contents of the primary item.
        //
        // But, the listview control sends this notification all the time, even if we do not search for an item. At least
        // this notification is only sent for the visible items and not for all items in the list. Though, because this
        // function is invoked *very* often, do *NOT* put any time consuming code in here.
        //
        // Vista: That callback is used to get the strings for the label tips for the sub(!) items.
        //
        NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
        if (pDispInfo->item.mask & LVIF_TEXT)
        {
            const CUpDownClient* pClient = reinterpret_cast<CUpDownClient*>(pDispInfo->item.lParam);
            if (pClient != NULL)
                GetItemDisplayText(pClient, pDispInfo->item.iSubItem, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
        }
    }
    *pResult = 0;
}

void CQueueListCtrl::OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLISTVIEW *pNMListView = (NMLISTVIEW *)pNMHDR;
    bool sortAscending;
    if (GetSortItem() != pNMListView->iSubItem)
    {
        switch (pNMListView->iSubItem)
        {
            case 2: // Up Priority
            case 3: // Rating
            case 4: // Score
            case 5: // Ask Count
            case 8: // Banned
            case 9: // Part Count
                sortAscending = false;
                break;
            default:
                sortAscending = true;
                break;
        }
    }
    else
        sortAscending = !GetSortAscending();

    // Sort table
    UpdateSortHistory(pNMListView->iSubItem + (sortAscending ? 0 : 100));
    SetSortArrow(pNMListView->iSubItem, sortAscending);
    SortItems(SortProc, pNMListView->iSubItem + (sortAscending ? 0 : 100));

    *pResult = 0;
}

int CQueueListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    const CUpDownClient *item1 = (CUpDownClient *)lParam1;
    const CUpDownClient *item2 = (CUpDownClient *)lParam2;
    int iColumn = (lParamSort >= 100) ? lParamSort - 100 : lParamSort;
    int iResult = 0;
    switch (iColumn)
    {
        case 0:
            if (item1->GetUserName() && item2->GetUserName())
                iResult = CompareLocaleStringNoCase(item1->GetUserName(), item2->GetUserName());
            else if (item1->GetUserName() == NULL)
                iResult = 1; // place clients with no usernames at bottom
            else if (item2->GetUserName() == NULL)
                iResult = -1; // place clients with no usernames at bottom
            break;

        case 1:
        {
            const CKnownFile *file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
            const CKnownFile *file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
            if (file1 != NULL && file2 != NULL)
            {
//>>> WiZaRd::PowerShare
                iResult = file1->IsPowerShared()-file2->IsPowerShared();
                if (iResult == 0)
//<<< WiZaRd::PowerShare
                    iResult = CompareLocaleStringNoCase(file1->GetFileName(), file2->GetFileName());
            }
            else if (file1 == NULL)
                iResult = 1;
            else
                iResult = -1;
            break;
        }

        case 2:
        {
            const CKnownFile *file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
            const CKnownFile *file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
            if (file1 != NULL && file2 != NULL)
            {
//>>> WiZaRd::PowerShare
                iResult = file1->IsPowerShared()-file2->IsPowerShared();
                if (iResult == 0)
//<<< WiZaRd::PowerShare
                    iResult = (file1->GetUpPriority() == PR_VERYLOW ? -1 : file1->GetUpPriority()) - (file2->GetUpPriority() == PR_VERYLOW ? -1 : file2->GetUpPriority());
            }
            else if (file1 == NULL)
                iResult = 1;
            else
                iResult = -1;
            break;
        }

        case 3:
//>>> WiZaRd::PowerShare
            iResult = item1->GetFriendSlot() - item2->GetFriendSlot();
            if (iResult == 0)
            {
                CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
                CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
                bool bIsPS1 = file1 && file1->IsPowerShared();
                bool bIsPS2 = file2 && file2->IsPowerShared();
                if (bIsPS1 && bIsPS2)
                    iResult = (file1->GetUpPriority() == PR_VERYLOW ? -1 : file1->GetUpPriority()) - (file2->GetUpPriority() == PR_VERYLOW ? -1 : file2->GetUpPriority());
                else
                    iResult = bIsPS1 - bIsPS2;
            }
            if (iResult == 0)
//<<< WiZaRd::PowerShare
                iResult = CompareUnsigned(item1->GetScore(false, false, true), item2->GetScore(false, false, true));
            break;

        case 4:
            iResult = CompareUnsigned(item1->GetScore(false), item2->GetScore(false));
            break;

        case 5:
            iResult = CompareUnsigned(item1->GetAskedCount(), item2->GetAskedCount());
            break;

        case 6:
            iResult = CompareUnsigned(item1->GetLastUpRequest(), item2->GetLastUpRequest());
            break;

        case 7:
            iResult = CompareUnsigned(item1->GetWaitStartTime(), item2->GetWaitStartTime());
            break;

        case 8:
            iResult = item1->IsBanned() - item2->IsBanned();
            break;

        case 9:
            iResult = CompareUnsigned(item1->GetUpPartCount(), item2->GetUpPartCount());
            break;

        case 10:
            //Proper sorting ;)
            iResult = item1->GetClientSoft() - item2->GetClientSoft();
            if (iResult == 0)
                iResult = CompareLocaleStringNoCase(item1->DbgGetFullClientSoftVer(), item2->DbgGetFullClientSoftVer());
            break;

#ifdef _DEBUG
        case 11:
            iResult = (item1->GetUpCompleteSourcesCount() != _UI16_MAX) - (item2->GetUpCompleteSourcesCount() != _UI16_MAX);
            if (iResult == 0)
                iResult = CompareUnsigned(item1->GetUpCompleteSourcesCount(), item2->GetUpCompleteSourcesCount());
            break;
#endif
    }

    if (lParamSort >= 100)
        iResult = -iResult;

    //call secondary sortorder, if this one results in equal
    int dwNextSort;
    if (iResult == 0 && (dwNextSort = theApp.emuledlg->transferwnd->GetQueueList()->GetNextSortOrder(lParamSort)) != -1)
        iResult = SortProc(lParam1, lParam2, dwNextSort);

    return iResult;
}

void CQueueListCtrl::OnNmDblClk(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
    int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
    if (iSel != -1)
    {
        CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
        if (client)
        {
            CClientDetailDialog dialog(client, this);
            dialog.DoModal();
        }
    }
    *pResult = 0;
}

void CQueueListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
    const CUpDownClient* client = (iSel != -1) ? (CUpDownClient*)GetItemData(iSel) : NULL;

    CTitleMenu ClientMenu;
    ClientMenu.CreatePopupMenu();
    ClientMenu.AddMenuTitle(GetResString(IDS_CLIENTS), true);
    ClientMenu.AppendMenu(MF_STRING | (client ? MF_ENABLED : MF_GRAYED), MP_DETAIL, GetResString(IDS_SHOWDETAILS), _T("CLIENTDETAILS"));
    ClientMenu.SetDefaultItem(MP_DETAIL);
//>>> Tux::FriendHandling
//  ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && !client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_ADDFRIEND, GetResString(IDS_ADDFRIEND), _T("ADDFRIEND"));
    if (client && client->IsEd2kClient())
    {
        if (!client->IsFriend())
            ClientMenu.AppendMenu(MF_STRING, MP_ADDFRIEND, GetResString(IDS_ADDFRIEND), _T("ADDFRIEND"));
        else
            ClientMenu.AppendMenu(MF_STRING, MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND), _T("DELETEFRIEND"));
    }
    ClientMenu.AppendMenu(MF_STRING | (client && client->IsFriend() && !client->GetFriendSlot() ? MF_ENABLED : MF_GRAYED), MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT), _T("FRIENDSLOT"));
    ClientMenu.CheckMenuItem(MP_FRIENDSLOT, (client && client->GetFriendSlot()) ? MF_CHECKED : MF_UNCHECKED);
//<<< Tux::FriendHandling
    ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient()) ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG), _T("SENDMESSAGE"));
    ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetViewSharedFilesSupport()) ? MF_ENABLED : MF_GRAYED), MP_SHOWLIST, GetResString(IDS_VIEWFILES), _T("VIEWFILES"));
    if (thePrefs.IsExtControlsEnabled())
        ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsBanned()) ? MF_ENABLED : MF_GRAYED), MP_UNBAN, GetResString(IDS_UNBAN));
    if (Kademlia::CKademlia::IsRunning() && !Kademlia::CKademlia::IsConnected())
        ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetKadPort()!=0 && client->GetKadVersion() > 1) ? MF_ENABLED : MF_GRAYED), MP_BOOT, GetResString(IDS_BOOTSTRAP));
    ClientMenu.AppendMenu(MF_STRING | (GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED), MP_FIND, GetResString(IDS_FIND), _T("Search"));
    GetPopupMenuPos(*this, point);
    ClientMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CQueueListCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
    wParam = LOWORD(wParam);

    switch (wParam)
    {
        case MP_FIND:
            OnFindStart();
            return TRUE;
    }

    int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
    if (iSel != -1)
    {
        CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
        switch (wParam)
        {
            case MP_SHOWLIST:
                client->RequestSharedFileList();
                break;
            case MP_MESSAGE:
                theApp.emuledlg->chatwnd->StartSession(client);
                break;
            case MP_ADDFRIEND:
                if (theApp.friendlist->AddFriend(client))
                    Update(iSel);
                break;
//>>> Tux::FriendHandling
            case MP_REMOVEFRIEND:
                if (client && client->IsFriend())
                {
                    theApp.friendlist->RemoveFriend(client->m_Friend);
                    Update(iSel);
                }
                break;
            case MP_FRIENDSLOT:
                if (client)
                {
                    bool IsAlready;
                    IsAlready = client->GetFriendSlot();
                    theApp.friendlist->RemoveAllFriendSlots();
                    if (!IsAlready)
                        client->SetFriendSlot(true);
                    Update(iSel);
                }
                break;
//<<< Tux::FriendHandling
            case MP_UNBAN:
                if (client->IsBanned())
                {
                    client->UnBan();
                    Update(iSel);
                }
                break;
            case MP_DETAIL:
            case MPG_ALTENTER:
            case IDA_ENTER:
            {
                CClientDetailDialog dialog(client, this);
                dialog.DoModal();
                break;
            }
            case MP_BOOT:
                if (client->GetKadPort() && client->GetKadVersion() > 1)
//>>> WiZaRd::IPv6 [Xanatos]
                    if (!client->GetIPv4().IsNull())
                        Kademlia::CKademlia::Bootstrap(client->GetIPv4().ToIPv4(), client->GetKadPort());
                //Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort());
//<<< WiZaRd::IPv6 [Xanatos]
                break;
        }
    }
    return true;
}

void CQueueListCtrl::AddClient(/*const*/ CUpDownClient *client, bool resetclient)
{
    if (resetclient && client)
    {
        client->SetWaitStartTime();
        client->SetAskedCount(1);
    }

    if (!theApp.emuledlg->IsRunning())
        return;
    if (thePrefs.IsQueueListDisabled())
        return;

    int iItemCount = GetItemCount();
    int iItem = InsertItem(LVIF_TEXT | LVIF_PARAM, iItemCount, LPSTR_TEXTCALLBACK, 0, 0, 0, (LPARAM)client);
    Update(iItem);
    theApp.emuledlg->transferwnd->UpdateListCount(CTransferDlg::wnd2OnQueue, iItemCount + 1);
}

void CQueueListCtrl::RemoveClient(const CUpDownClient *client)
{
    if (!theApp.emuledlg->IsRunning())
        return;

    LVFINDINFO find;
    find.flags = LVFI_PARAM;
    find.lParam = (LPARAM)client;
    int result = FindItem(&find);
    if (result != -1)
    {
        DeleteItem(result);
        theApp.emuledlg->transferwnd->UpdateListCount(CTransferDlg::wnd2OnQueue);
    }
}

void CQueueListCtrl::RefreshClient(const CUpDownClient *client)
{
    if (!theApp.emuledlg->IsRunning())
        return;

    if (theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd || !theApp.emuledlg->transferwnd->GetQueueList()->IsWindowVisible())
        return;

    LVFINDINFO find;
    find.flags = LVFI_PARAM;
    find.lParam = (LPARAM)client;
    int result = FindItem(&find);
    if (result != -1)
        Update(result);
}

void CQueueListCtrl::ShowSelectedUserDetails()
{
    POINT point;
    ::GetCursorPos(&point);
    CPoint p = point;
    ScreenToClient(&p);
    int it = HitTest(p);
    if (it == -1)
        return;

    SetItemState(-1, 0, LVIS_SELECTED);
    SetItemState(it, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    SetSelectionMark(it);   // display selection mark correctly!

    CUpDownClient* client = (CUpDownClient*)GetItemData(GetSelectionMark());
    if (client)
    {
        CClientDetailDialog dialog(client, this);
        dialog.DoModal();
    }
}

void CQueueListCtrl::ShowQueueClients()
{
    DeleteAllItems();
    CUpDownClient *update = theApp.uploadqueue->GetNextClient(NULL);
    while (update)
    {
        AddClient(update, false);
        update = theApp.uploadqueue->GetNextClient(update);
    }
}

// Barry - Refresh the queue every 10 secs
void CALLBACK CQueueListCtrl::QueueUpdateTimer(HWND /*hwnd*/, UINT /*uiMsg*/, UINT /*idEvent*/, DWORD /*dwTime*/)
{
    // NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
    try
    {
        if (!theApp.emuledlg->IsRunning()    // Don't do anything if the app is shutting down - can cause unhandled exceptions
                || !thePrefs.GetUpdateQueueList()
                || theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd
                || !theApp.emuledlg->transferwnd->GetQueueList()->IsWindowVisible())
            return;

        CQueueListCtrl* _this = theApp.emuledlg->transferwnd->GetQueueList();
        for (int i = 0; i < _this->GetItemCount(); ++i)
            _this->Update(i);
    }
    CATCH_DFLT_EXCEPTIONS(_T("CQueueListCtrl::QueueUpdateTimer"))
}