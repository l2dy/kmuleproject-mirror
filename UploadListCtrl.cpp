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
#include "UploadListCtrl.h"
#include "TransferWnd.h"
#include "TransferDlg.h"
#include "otherfunctions.h"
#include "MenuCmds.h"
#include "ClientDetailDialog.h"
#include "emuledlg.h"
#include "friendlist.h"
#include "MemDC.h"
#include "KnownFile.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "ClientCredits.h"
#include "ChatWnd.h"
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/net/KademliaUDPListener.h"
#include "UploadQueue.h"
#include "ToolTipCtrlX.h"
#include "ListenSocket.h" //>>> WiZaRd::Count block/success send

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CUploadListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CUploadListCtrl, CMuleListCtrl)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnClick)
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetDispInfo)
    ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnNmDblClk)
    ON_WM_CONTEXTMENU()
    ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CUploadListCtrl::CUploadListCtrl()
    : CListCtrlItemWalk(this)
{
    m_tooltip = new CToolTipCtrlX;
    SetGeneralPurposeFind(true);
    SetSkinKey(L"UploadsLv");
}

CUploadListCtrl::~CUploadListCtrl()
{
    delete m_tooltip;
}

void CUploadListCtrl::Init()
{
    SetPrefsKey(_T("UploadListCtrl"));
    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

    CToolTipCtrl* tooltip = GetToolTips();
    if (tooltip)
    {
        m_tooltip->SubclassWindow(tooltip->m_hWnd);
        tooltip->ModifyStyle(0, TTS_NOPREFIX);
        tooltip->SetDelayTime(TTDT_AUTOPOP, 20000);
        tooltip->SetDelayTime(TTDT_INITIAL, 1000);
    }

    InsertColumn(0, GetResString(IDS_QL_USERNAME),	LVCFMT_LEFT,  DFLT_CLIENTNAME_COL_WIDTH);
    InsertColumn(1, GetResString(IDS_FILE),			LVCFMT_LEFT,  DFLT_FILENAME_COL_WIDTH);
    InsertColumn(2, GetResString(IDS_DL_SPEED),		LVCFMT_RIGHT, DFLT_DATARATE_COL_WIDTH);
    InsertColumn(3, GetResString(IDS_DL_TRANSF),	LVCFMT_RIGHT, DFLT_DATARATE_COL_WIDTH);
    InsertColumn(4, GetResString(IDS_WAITED),		LVCFMT_LEFT,   60);
    InsertColumn(5, GetResString(IDS_UPLOADTIME),	LVCFMT_LEFT,   80);
    InsertColumn(6, GetResString(IDS_STATUS),		LVCFMT_LEFT,  100);
    InsertColumn(7, GetResString(IDS_UPSTATUS),		LVCFMT_LEFT,  DFLT_PARTSTATUS_COL_WIDTH);
    InsertColumn(8, GetResString(IDS_CD_CSOFT),		LVCFMT_LEFT,  DFLT_CLIENTSOFT_COL_WIDTH);
	InsertColumn(9, GetResString(IDS_UPSLOTNUMBER), LVCFMT_LEFT,  60);
#ifdef _DEBUG
	InsertColumn(10, L"Blocking Ratio", LVCFMT_LEFT, 60, -1, true); //>>> WiZaRd::Count block/success send
#endif

    SetAllIcons();
    Localize();
    LoadSettings();
    SetSortArrow();
    SortItems(SortProc, GetSortItem() + (GetSortAscending() ? 0 : 100));
}

void CUploadListCtrl::Localize()
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

    strRes = GetResString(IDS_DL_SPEED);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(2, &hdi);

    strRes = GetResString(IDS_DL_TRANSF);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(3, &hdi);

    strRes = GetResString(IDS_WAITED);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(4, &hdi);

    strRes = GetResString(IDS_UPLOADTIME);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(5, &hdi);

    strRes = GetResString(IDS_STATUS);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(6, &hdi);

    strRes = GetResString(IDS_UPSTATUS);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(7, &hdi);

    strRes = GetResString(IDS_CD_CSOFT);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(8, &hdi);

	strRes = GetResString(IDS_UPSLOTNUMBER);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(9, &hdi);

#ifdef _DEBUG
//>>> WiZaRd::Count block/success send
	strRes = L"Blocking Ratio";
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(10, &hdi);
//<<< WiZaRd::Count block/success send
#endif
}

void CUploadListCtrl::OnSysColorChange()
{
    CMuleListCtrl::OnSysColorChange();
    SetAllIcons();
}

void CUploadListCtrl::SetAllIcons()
{
    ApplyImageList(NULL);
    m_ImageList.DeleteImageList();
    m_ImageList.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
    m_ImageList.Add(CTempIconLoader(_T("ClientEDonkey")));
    m_ImageList.Add(CTempIconLoader(_T("ClientCompatible")));
    m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyPlus")));
    m_ImageList.Add(CTempIconLoader(_T("ClientCompatiblePlus")));
    m_ImageList.Add(CTempIconLoader(_T("Friend")));
    m_ImageList.Add(CTempIconLoader(_T("ClientMLDonkey")));
    m_ImageList.Add(CTempIconLoader(_T("ClientMLDonkeyPlus")));
    m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyHybrid")));
    m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyHybridPlus")));
    m_ImageList.Add(CTempIconLoader(_T("ClientShareaza")));
    m_ImageList.Add(CTempIconLoader(_T("ClientShareazaPlus")));
    m_ImageList.Add(CTempIconLoader(_T("ClientAMule")));
    m_ImageList.Add(CTempIconLoader(_T("ClientAMulePlus")));
    m_ImageList.Add(CTempIconLoader(_T("ClientLPhant")));
    m_ImageList.Add(CTempIconLoader(_T("ClientLPhantPlus")));
    badguy = m_ImageList.Add(CTempIconLoader(L"BADGUY")); //>>> WiZaRd::ClientAnalyzer
    m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
    m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("OverlayObfu"))), 2);
    m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("OverlaySecureObfu"))), 3);
    // Apply the image list also to the listview control, even if we use our own 'DrawItem'.
    // This is needed to give the listview control a chance to initialize the row height.
    ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) != 0 );
    VERIFY( ApplyImageList(m_ImageList) == NULL );
}

void CUploadListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
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
//>>> WiZaRd::ZZUL Upload [ZZ]
	if(client->IsScheduledForRemoval()) 
	{
		if(client->GetSlotNumber() > theApp.uploadqueue->GetActiveUploadsCount()) 
			dc.SetTextColor(RGB(255, 170, 170));
		else
			dc.SetTextColor(RGB(255, 50, 50));
	}
	else 
//<<< WiZaRd::ZZUL Upload [ZZ]
//>>> WiZaRd::Count block/success send
	//count a client as "full" if he is either transferring (above trickle speed) or blocking too much
//	if(client->GetSlotNumber() > theApp.uploadqueue->GetActiveUploadsCount()) 
	if(client->GetDatarate() < 1024
	   || (client->socket && client->socket->GetOverallBlockingRatio() > 95	//95% of all send were blocked - thePrefs.GetMaxBlockRate()
	   && client->socket->GetBlockingRatio() > 96))		//96% the last 20 seconds - thePrefs.GetMaxBlockRate20()
//<<< WiZaRd::Count block/success send
        dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

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
                    if(badguy != -1 && client->IsBadGuy())
                    {
                        int iIconPosY = (cur_rec.Height() > 16) ? ((cur_rec.Height() - 16) / 2) : 1;
                        POINT point = { cur_rec.left, cur_rec.top + iIconPosY };
                        m_ImageList.Draw(dc, badguy, point, ILD_NORMAL);
                        cur_rec.left += 17;
                        plusminus += 17;
                    }
//<<< WiZaRd::ClientAnalyzer
                    int iImage;
                    if (client->IsFriend())
                        iImage = 4;
                    else if (client->GetClientSoft() == SO_EDONKEYHYBRID)
                    {
                        if (client->credits->GetScoreRatio(client->GetIP()) > 1)
                            iImage = 8;
                        else
                            iImage = 7;
                    }
                    else if (client->GetClientSoft() == SO_MLDONKEY)
                    {
                        if (client->credits->GetScoreRatio(client->GetIP()) > 1)
                            iImage = 6;
                        else
                            iImage = 5;
                    }
                    else if (client->GetClientSoft() == SO_SHAREAZA)
                    {
                        if (client->credits->GetScoreRatio(client->GetIP()) > 1)
                            iImage = 10;
                        else
                            iImage = 9;
                    }
                    else if (client->GetClientSoft() == SO_AMULE)
                    {
                        if (client->credits->GetScoreRatio(client->GetIP()) > 1)
                            iImage = 12;
                        else
                            iImage = 11;
                    }
                    else if (client->GetClientSoft() == SO_LPHANT)
                    {
                        if (client->credits->GetScoreRatio(client->GetIP()) > 1)
                            iImage = 14;
                        else
                            iImage = 13;
                    }
                    else if (client->ExtProtocolAvailable())
                    {
                        if (client->credits->GetScoreRatio(client->GetIP()) > 1)
                            iImage = 3;
                        else
                            iImage = 1;
                    }
                    else
                    {
                        if (client->credits->GetScoreRatio(client->GetIP()) > 1)
                            iImage = 2;
                        else
                            iImage = 0;
                    }

                    UINT nOverlayImage = 0;
                    if ((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED))
                        nOverlayImage |= 1;
                    if (client->IsObfuscatedConnectionEstablished())
                        nOverlayImage |= 2;
                    int iIconPosY = (cur_rec.Height() > 16) ? ((cur_rec.Height() - 16) / 2) : 1;
                    POINT point = { cur_rec.left, cur_rec.top + iIconPosY };
                    m_ImageList.Draw(dc, iImage, point, ILD_NORMAL | INDEXTOOVERLAYMASK(nOverlayImage));

                    cur_rec.left += 16 + sm_iLabelOffset;
                    dc.DrawText(szItem, -1, &cur_rec, MLC_DT_TEXT | uDrawTextAlignment);
                    cur_rec.left -= 16;
                    cur_rec.right -= sm_iSubItemInset;
                    cur_rec.left -= plusminus; //>>> WiZaRd::ClientAnalyzer
                    break;
                }

                case 7:
                    cur_rec.bottom--;
                    cur_rec.top++;
                    client->DrawUpStatusBar(dc, &cur_rec, false, thePrefs.UseFlatBar());
                    cur_rec.bottom++;
                    cur_rec.top--;
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

void CUploadListCtrl::GetItemDisplayText(const CUpDownClient *client, int iSubItem, LPTSTR pszText, int cchTextMax)
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
        _tcsncpy(pszText, file != NULL ? file->GetFileName() : _T(""), cchTextMax);
        break;
    }

    case 2:
        _tcsncpy(pszText, CastItoXBytes(client->GetDatarate(), false, true), cchTextMax);
        break;

    case 3:
        // NOTE: If you change (add/remove) anything which is displayed here, update also the sorting part..
        if (thePrefs.m_bExtControls)
            _sntprintf(pszText, cchTextMax, _T("%s (%s)"), CastItoXBytes(client->GetSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionPayloadUp(), false, false));
        else
            _tcsncpy(pszText, CastItoXBytes(client->GetSessionUp(), false, false), cchTextMax);
        break;

    case 4:
        if (client->HasLowID())
//>>> WiZaRd::Fix for LowID slots only on connection [VQB]
			if(client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)
				_sntprintf(pszText, cchTextMax, L"%s (%s %s)", CastSecondsToHM((client->GetWaitTime())/1000), GetResString(IDS_IDLOW), CastSecondsToHM((::GetTickCount()-client->GetUpStartTimeDelay()-client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)/1000));
			else
//<<< WiZaRd::Fix for LowID slots only on connection [VQB]
				_sntprintf(pszText, cchTextMax, _T("%s (%s)"), CastSecondsToHM(client->GetWaitTime() / 1000), GetResString(IDS_IDLOW));
        else
            _tcsncpy(pszText, CastSecondsToHM(client->GetWaitTime() / 1000), cchTextMax);
        break;

    case 5:
        _tcsncpy(pszText, CastSecondsToHM(client->GetUpStartTimeDelay() / 1000), cchTextMax);
        break;

    case 6:
        _tcsncpy(pszText, client->GetUploadStateDisplayString(), cchTextMax);
        break;

    case 7:
        _tcsncpy(pszText, GetResString(IDS_UPSTATUS), cchTextMax);
        break;

    case 8:
        _tcsncpy(pszText, client->DbgGetFullClientSoftVer(), cchTextMax);
        break;

	case 9:
		_sntprintf(pszText, cchTextMax, L"%u", client->GetSlotNumber());
		break;

#ifdef _DEBUG
//>>> WiZaRd::Count block/success send
	case 10: 
	{
		CString sBuffer = L"";
		if(client->socket)
			sBuffer.Format(L"%.2f%% (%.2f%%)", client->socket->GetBlockingRatio(), client->socket->GetOverallBlockingRatio());
		else
			sBuffer = L"-";
		_tcsncpy(pszText, sBuffer, cchTextMax);
		break;
	}
//<<< WiZaRd::Count block/success send
#endif
    }
    pszText[cchTextMax - 1] = L'\0';
}

void CUploadListCtrl::OnLvnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
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

void CUploadListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
    if (pGetInfoTip->iSubItem == 0)
    {
        LVHITTESTINFO hti = {0};
        ::GetCursorPos(&hti.pt);
        ScreenToClient(&hti.pt);
        if (SubItemHitTest(&hti) == -1 || hti.iItem != pGetInfoTip->iItem || hti.iSubItem != 0)
        {
            // don't show the default label tip for the main item, if the mouse is not over the main item
            if ((pGetInfoTip->dwFlags & LVGIT_UNFOLDED) == 0 && pGetInfoTip->cchTextMax > 0 && pGetInfoTip->pszText[0] != L'\0')
                pGetInfoTip->pszText[0] = L'\0';
            return;
        }

        const CUpDownClient* client = (CUpDownClient*)GetItemData(pGetInfoTip->iItem);
        if (client && pGetInfoTip->pszText && pGetInfoTip->cchTextMax > 0)
        {
            CString strInfo;
            strInfo.Format(GetResString(IDS_USERINFO), client->GetUserName());
            const CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
            if (file)
            {
                strInfo += GetResString(IDS_SF_REQUESTED) + _T(' ') + file->GetFileName() + _T('\n');
                strInfo.AppendFormat(GetResString(IDS_FILESTATS_SESSION) + GetResString(IDS_FILESTATS_TOTAL),
                                     file->statistic.GetAccepts(), file->statistic.GetRequests(), CastItoXBytes(file->statistic.GetTransferred(), false, false),
                                     file->statistic.GetAllTimeAccepts(), file->statistic.GetAllTimeRequests(), CastItoXBytes(file->statistic.GetAllTimeTransferred(), false, false));
            }
            else
            {
                strInfo += GetResString(IDS_REQ_UNKNOWNFILE);
            }
            strInfo += TOOLTIP_AUTOFORMAT_SUFFIX_CH;
            _tcsncpy(pGetInfoTip->pszText, strInfo, pGetInfoTip->cchTextMax);
            pGetInfoTip->pszText[pGetInfoTip->cchTextMax-1] = L'\0';
        }
    }
    *pResult = 0;
}

void CUploadListCtrl::OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLISTVIEW *pNMListView = (NMLISTVIEW *)pNMHDR;
    bool sortAscending;
    if (GetSortItem() != pNMListView->iSubItem)
    {
        switch (pNMListView->iSubItem)
        {
        case 2: // Datarate
        case 3: // Session Up
        case 4: // Wait Time
        case 7: // Part Count
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

int CUploadListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
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
            iResult = CompareLocaleStringNoCase(file1->GetFileName(), file2->GetFileName());
        else if (file1 == NULL)
            iResult = 1;
        else
            iResult = -1;
        break;
    }

    case 2:
        iResult = CompareUnsigned(item1->GetDatarate(), item2->GetDatarate());
        break;

    case 3:
        iResult = CompareUnsigned(item1->GetSessionUp(), item2->GetSessionUp());
        if (iResult == 0 && thePrefs.m_bExtControls)
            iResult = CompareUnsigned(item1->GetQueueSessionPayloadUp(), item2->GetQueueSessionPayloadUp());
        break;

    case 4:
        iResult = CompareUnsigned(item1->GetWaitTime(), item2->GetWaitTime());
        break;

    case 5:
        iResult = CompareUnsigned(item1->GetUpStartTimeDelay() ,item2->GetUpStartTimeDelay());
        break;

    case 6:
        iResult = item1->GetUploadState() - item2->GetUploadState();
        break;

    case 7:
        iResult = CompareUnsigned(item1->GetUpPartCount(), item2->GetUpPartCount());
        break;

    case 8:
        //Proper sorting ;)
        iResult = item1->GetClientSoft() - item2->GetClientSoft();
        if(iResult == 0)
            iResult = CompareLocaleStringNoCase(item1->DbgGetFullClientSoftVer(), item2->DbgGetFullClientSoftVer());
        break;

	case 9:
		iResult = CompareUnsigned(item1->GetSlotNumber(), item2->GetSlotNumber());
		break;

#ifdef _DEBUG
//>>> WiZaRd::Count block/success send
	case 10:
		if(item1->socket && item2->socket)
		{
			iResult = CompareFloat(item1->socket->GetBlockingRatio(), item2->socket->GetBlockingRatio());
			if(iResult == 0)
				iResult = CompareFloat(item1->socket->GetOverallBlockingRatio(), item2->socket->GetOverallBlockingRatio());
		}
		else if(item1->socket)
			iResult = -1;
		else if(item2->socket)
			iResult = 1;
		break;
//<<< WiZaRd::Count block/success send
#endif
    }

    if (lParamSort >= 100)
        iResult = -iResult;

    //call secondary sortorder, if this one results in equal
    int dwNextSort;
    if (iResult == 0 && (dwNextSort = theApp.emuledlg->transferwnd->m_pwndTransfer->uploadlistctrl.GetNextSortOrder(lParamSort)) != -1)
        iResult = SortProc(lParam1, lParam2, dwNextSort);

    return iResult;
}

void CUploadListCtrl::OnNmDblClk(NMHDR* /*pNMHDR*/, LRESULT* pResult)
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

void CUploadListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
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
    ClientMenu.AppendMenu(MF_STRING | (client && client->IsFriend() && !client->GetFriendSlot()) ? MF_ENABLED : MF_GRAYED, MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT), _T("FRIENDSLOT"));
    ClientMenu.CheckMenuItem(MP_FRIENDSLOT, (client && client->GetFriendSlot()) ? MF_CHECKED : MF_UNCHECKED);
//<<< Tux::FriendHandling
    ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient()) ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG), _T("SENDMESSAGE"));
    ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetViewSharedFilesSupport()) ? MF_ENABLED : MF_GRAYED), MP_SHOWLIST, GetResString(IDS_VIEWFILES), _T("VIEWFILES"));
    if (Kademlia::CKademlia::IsRunning() && !Kademlia::CKademlia::IsConnected())
        ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetKadPort()!=0 && client->GetKadVersion() > 1) ? MF_ENABLED : MF_GRAYED), MP_BOOT, GetResString(IDS_BOOTSTRAP));
    ClientMenu.AppendMenu(MF_STRING | (GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED), MP_FIND, GetResString(IDS_FIND), _T("Search"));
    GetPopupMenuPos(*this, point);
    ClientMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CUploadListCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
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
                Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort());
            break;
        }
    }
    return true;
}

void CUploadListCtrl::AddClient(const CUpDownClient *client)
{
    if (!theApp.emuledlg->IsRunning())
        return;

    int iItemCount = GetItemCount();
    int iItem = InsertItem(LVIF_TEXT | LVIF_PARAM, iItemCount, LPSTR_TEXTCALLBACK, 0, 0, 0, (LPARAM)client);
    Update(iItem);
    theApp.emuledlg->transferwnd->m_pwndTransfer->UpdateListCount(CTransferWnd::wnd2Uploading, iItemCount + 1);
}

void CUploadListCtrl::RemoveClient(const CUpDownClient *client)
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
        theApp.emuledlg->transferwnd->m_pwndTransfer->UpdateListCount(CTransferWnd::wnd2Uploading);
    }
}

void CUploadListCtrl::RefreshClient(const CUpDownClient *client)
{
    if (!theApp.emuledlg->IsRunning())
        return;

    if (theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd || !theApp.emuledlg->transferwnd->m_pwndTransfer->uploadlistctrl.IsWindowVisible())
        return;

    LVFINDINFO find;
    find.flags = LVFI_PARAM;
    find.lParam = (LPARAM)client;
    int result = FindItem(&find);
    if (result != -1)
        Update(result);
}

void CUploadListCtrl::ShowSelectedUserDetails()
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

