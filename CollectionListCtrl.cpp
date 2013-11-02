//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
#include "emuleDlg.h"
#include "CollectionListCtrl.h"
#include "OtherFunctions.h"
#include "AbstractFile.h"
#include "MetaDataDlg.h"
#include "HighColorTab.hpp"
#include "ListViewWalkerPropertySheet.h"
#include "UserMsgs.h"
#include "MemDC.h"
//>>> WiZaRd::CollectionEnhancement
#include "TitleMenu.h"
#include "MenuCmds.h"
#include "InputBox.h"
//<<< WiZaRd::CollectionEnhancement

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


//////////////////////////////////////////////////////////////////////////////
// CCollectionFileDetailsSheet

class CCollectionFileDetailsSheet : public CListViewWalkerPropertySheet
{
    DECLARE_DYNAMIC(CCollectionFileDetailsSheet)

public:
    CCollectionFileDetailsSheet(CTypedPtrList<CPtrList, CAbstractFile*>& aFiles, UINT uPshInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
    virtual ~CCollectionFileDetailsSheet();

protected:
    CMetaDataDlg		m_wndMetaData;

    UINT m_uPshInvokePage;
    static LPCTSTR m_pPshStartPage;

    void UpdateTitle();

    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()
    afx_msg void OnDestroy();
    afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
};

LPCTSTR CCollectionFileDetailsSheet::m_pPshStartPage;

IMPLEMENT_DYNAMIC(CCollectionFileDetailsSheet, CListViewWalkerPropertySheet)

BEGIN_MESSAGE_MAP(CCollectionFileDetailsSheet, CListViewWalkerPropertySheet)
    ON_WM_DESTROY()
    ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
END_MESSAGE_MAP()

CCollectionFileDetailsSheet::CCollectionFileDetailsSheet(CTypedPtrList<CPtrList, CAbstractFile*>& aFiles, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
    : CListViewWalkerPropertySheet(pListCtrl)
{
    m_uPshInvokePage = uPshInvokePage;
    POSITION pos = aFiles.GetHeadPosition();
    while (pos)
        m_aItems.Add(aFiles.GetNext(pos));
    m_psh.dwFlags &= ~PSH_HASHELP;

    m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
    m_wndMetaData.m_psp.dwFlags |= PSP_USEICONID;
    m_wndMetaData.m_psp.pszIcon = _T("METADATA");
    if (m_aItems.GetSize() == 1 && thePrefs.IsExtControlsEnabled())
    {
        m_wndMetaData.SetFiles(&m_aItems);
        AddPage(&m_wndMetaData);
    }

    LPCTSTR pPshStartPage = m_pPshStartPage;
    if (m_uPshInvokePage != 0)
        pPshStartPage = MAKEINTRESOURCE(m_uPshInvokePage);
    for (int i = 0; i < m_pages.GetSize(); i++)
    {
        CPropertyPage* pPage = GetPage(i);
        if (pPage->m_psp.pszTemplate == pPshStartPage)
        {
            m_psh.nStartPage = i;
            break;
        }
    }
}

CCollectionFileDetailsSheet::~CCollectionFileDetailsSheet()
{
}

void CCollectionFileDetailsSheet::OnDestroy()
{
    if (m_uPshInvokePage == 0)
        m_pPshStartPage = GetPage(GetActiveIndex())->m_psp.pszTemplate;
    CListViewWalkerPropertySheet::OnDestroy();
}

BOOL CCollectionFileDetailsSheet::OnInitDialog()
{
    EnableStackedTabs(FALSE);
    BOOL bResult = CListViewWalkerPropertySheet::OnInitDialog();
    HighColorTab::UpdateImageList(*this);
    InitWindowStyles(this);
    EnableSaveRestore(_T("CollectionFileDetailsSheet")); // call this after(!) OnInitDialog
    UpdateTitle();
    return bResult;
}

LRESULT CCollectionFileDetailsSheet::OnDataChanged(WPARAM, LPARAM)
{
    UpdateTitle();
    return 1;
}

void CCollectionFileDetailsSheet::UpdateTitle()
{
    if (m_aItems.GetSize() == 1)
        SetWindowText(GetResString(IDS_DETAILS) + _T(": ") + STATIC_DOWNCAST(CAbstractFile, m_aItems[0])->GetFileName());
    else
        SetWindowText(GetResString(IDS_DETAILS));
}



//////////////////////////////////////////////////////////////////////////////
// CCollectionListCtrl

enum ECols
{
    colName = 0,
    colSize,
    colHash,
    colFolder, //>>> WiZaRd::CollectionEnhancement
};

IMPLEMENT_DYNAMIC(CCollectionListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CCollectionListCtrl, CMuleListCtrl)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnClick)	
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetDispInfo)
    ON_NOTIFY_REFLECT(NM_CLICK, OnNmClick)
    ON_WM_CONTEXTMENU() //>>> WiZaRd::CollectionEnhancement
    ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CCollectionListCtrl::CCollectionListCtrl()
    : CListCtrlItemWalk(this)
{
	m_bCheckBoxes = false;
	m_bContextMenu = false;
}

void CCollectionListCtrl::Init(CString strNameAdd)
{
    SetPrefsKey(_T("CollectionListCtrl") + strNameAdd);
    ASSERT((GetStyle() & LVS_SINGLESEL) == 0);
	UINT nStyle = LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP;
	if(m_bCheckBoxes)
		nStyle |= LVS_EX_CHECKBOXES;
    SetExtendedStyle(nStyle);

    InsertColumn(colName, GetResString(IDS_DL_FILENAME),	LVCFMT_LEFT,  DFLT_FILENAME_COL_WIDTH);
    InsertColumn(colSize, GetResString(IDS_DL_SIZE),		LVCFMT_RIGHT, DFLT_SIZE_COL_WIDTH);
    InsertColumn(colHash, GetResString(IDS_FILEHASH),		LVCFMT_LEFT,  DFLT_HASH_COL_WIDTH, -1, true);
    InsertColumn(colFolder, GetResString(IDS_FOLDER),		LVCFMT_LEFT,  DFLT_FOLDER_COL_WIDTH); //>>> WiZaRd::CollectionEnhancement

    SetAllIcons();
    Localize();
    LoadSettings();
    SetSortArrow();
    SortItems(SortProc, MAKELONG(GetSortItem(), (GetSortAscending() ? 0 : 1)));
}

void CCollectionListCtrl::Localize()
{
    CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
    HDITEM hdi;
    hdi.mask = HDI_TEXT;

    CString strRes;
    strRes = GetResString(IDS_DL_FILENAME);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(0, &hdi);

    strRes = GetResString(IDS_DL_SIZE);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(1, &hdi);

    strRes = GetResString(IDS_FILEHASH);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(2, &hdi);

//>>> WiZaRd::CollectionEnhancement
    strRes = GetResString(IDS_FOLDER);
    hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
    pHeaderCtrl->SetItem(3, &hdi);
//<<< WiZaRd::CollectionEnhancement
}

void CCollectionListCtrl::OnSysColorChange()
{
    CMuleListCtrl::OnSysColorChange();
    SetAllIcons();
}

void CCollectionListCtrl::SetAllIcons()
{
    ApplyImageList(NULL);
    m_ImageList.DeleteImageList();
    m_ImageList.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);

    // Apply the image list also to the listview control, even if we use our own 'DrawItem'.
    // This is needed to give the listview control a chance to initialize the row height.
    ASSERT((GetStyle() & LVS_SHAREIMAGELISTS) != 0);
    VERIFY(ApplyImageList(m_ImageList) == NULL);
}

void CCollectionListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
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
	const CAbstractFile* pFile = (CAbstractFile*)lpDrawItemStruct->itemData;

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
				GetItemDisplayText(pFile, iColumn, szItem, _countof(szItem));
				switch (iColumn)
				{
					case 0:
					{
						CRect rcDraw(cur_rec);
						int iIconPosY = (rcDraw.Height() > theApp.GetSmallSytemIconSize().cy) ? ((rcDraw.Height() - theApp.GetSmallSytemIconSize().cy) / 2) : 0;

						if(m_bCheckBoxes)
						{
							CRect draw(rcDraw.left, rcDraw.top, rcDraw.left+16, rcDraw.bottom);
							UINT nFlag = DFCS_BUTTONCHECK;
							if(GetCheck(lpDrawItemStruct->itemID))
								nFlag |= DFCS_CHECKED;
							dc->DrawFrameControl(draw, DFC_BUTTON, nFlag);
							rcDraw.left += 16;
						}

						int iImage = theApp.GetFileTypeSystemImageIdx(pFile->GetFileName());
						if (theApp.GetSystemImageList() != NULL)
							::ImageList_Draw(theApp.GetSystemImageList(), iImage, dc->GetSafeHdc(), rcDraw.left, rcDraw.top + iIconPosY, ILD_TRANSPARENT);
						rcDraw.left += theApp.GetSmallSytemIconSize().cx;

						rcDraw.left += sm_iLabelOffset;
						dc->DrawText(szItem, -1, &rcDraw, MLC_DT_TEXT | uDrawTextAlignment);
						break;
					}

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

void CCollectionListCtrl::GetItemDisplayText(const CAbstractFile *pFile, int iSubItem, LPTSTR pszText, int cchTextMax)
{
    if (pszText == NULL || cchTextMax <= 0)
    {
        ASSERT(0);
        return;
    }
    pszText[0] = L'\0';
	CString buffer = L"";
    switch (iSubItem)
    {
		case 0:
			buffer = pFile->GetFileName();				
			break;

		case 1:
			buffer = CastItoXBytes(pFile->GetFileSize());
			break;

		case 2:
			buffer = md4str(pFile->GetFileHash());
			break;

//>>> WiZaRd::CollectionEnhancement
		case 3:
			buffer = pFile->GetDownloadDirectory(true); 
			break;
//<<< WiZaRd::CollectionEnhancement
    }
	_tcsncpy(pszText, buffer, cchTextMax);
    pszText[cchTextMax - 1] = L'\0';
}

void CCollectionListCtrl::OnLvnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
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
            const CAbstractFile* pClient = reinterpret_cast<CAbstractFile*>(pDispInfo->item.lParam);
            if (pClient != NULL)
                GetItemDisplayText(pClient, pDispInfo->item.iSubItem, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
        }
    }
    *pResult = 0;
}

void CCollectionListCtrl::OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLISTVIEW *pNMListView = (NMLISTVIEW *)pNMHDR;

    // Determine ascending based on whether already sorted on this column
    int iSortItem = GetSortItem();
    bool bOldSortAscending = GetSortAscending();
    bool bSortAscending = (iSortItem != pNMListView->iSubItem) ? true : !bOldSortAscending;

    // Item is column clicked
    iSortItem = pNMListView->iSubItem;

    // Sort table
    UpdateSortHistory(MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));
    SetSortArrow(iSortItem, bSortAscending);
    SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));

    *pResult = 0;
}

int CCollectionListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    const CAbstractFile *item1 = (CAbstractFile *)lParam1;
    const CAbstractFile *item2 = (CAbstractFile *)lParam2;
    if (item1 == NULL || item2 == NULL)
        return 0;

    int iResult;
    switch (LOWORD(lParamSort))
    {
		case colName:
			iResult = CompareLocaleStringNoCase(item1->GetFileName(),item2->GetFileName());
			break;

		case colSize:
			iResult = CompareUnsigned64(item1->GetFileSize(), item2->GetFileSize());
			break;

		case colHash:
			iResult = memcmp(item1->GetFileHash(), item2->GetFileHash(), 16);
			break;

//>>> WiZaRd::CollectionEnhancement
		case colFolder:
			iResult = CompareOptLocaleStringNoCaseUndefinedAtBottom(item1->GetDownloadDirectory(true), item2->GetDownloadDirectory(true), true);
			break;
//<<< WiZaRd::CollectionEnhancement

		default:
			return 0;
    }
    if (HIWORD(lParamSort))
        iResult = -iResult;
    return iResult;
}

//>>> WiZaRd::CollectionEnhancement
void CCollectionListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	if(m_bContextMenu)
	{
		UINT flag = MF_STRING;
		if (GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED) == -1)
			flag = MF_GRAYED;

		CTitleMenu popupMenu;
		popupMenu.CreatePopupMenu();
		if(thePrefs.IsExtControlsEnabled())
			popupMenu.AppendMenu(flag, MP_META_DATA, GetResString(IDS_META_DATA));
		popupMenu.AppendMenu(flag, MP_RENAME, GetResString(IDS_FOLDER));
		popupMenu.SetDefaultItem(thePrefs.IsExtControlsEnabled() ? MP_META_DATA : MP_RENAME);

		GetPopupMenuPos(*this, point);
		popupMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
		//VERIFY(popupMenu.DestroyMenu());
	}
}

BOOL CCollectionListCtrl::OnCommand(WPARAM wParam,LPARAM lParam)
{
    int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
    if (iSel != -1)
    {
        CTypedPtrList<CPtrList, CAbstractFile*> abstractFileList;
        POSITION pos = GetFirstSelectedItemPosition();
        while (pos != NULL)
        {
            int index = GetNextSelectedItem(pos);
            if (index >= 0)
                abstractFileList.AddTail((CAbstractFile*)GetItemData(index));
        }
        switch (wParam)
        {
			case MP_META_DATA:
			{
				ASSERT(m_bContextMenu && thePrefs.IsExtControlsEnabled());
				if (!abstractFileList.IsEmpty())
				{
					CCollectionFileDetailsSheet dialog(abstractFileList, 0, this);
					dialog.DoModal();
				}
				return TRUE;
			}

			case MP_RENAME:
			{
				ASSERT(m_bContextMenu);
				CString strDirectory = L"";
				if (!abstractFileList.IsEmpty())
					strDirectory = abstractFileList.GetHead()->GetDownloadDirectory();
				InputBox inputbox;
				CString title = GetResString(IDS_FOLDER);
				inputbox.SetLabels(title, GetResString(IDS_FOLDER), strDirectory);
				inputbox.SetEditFilenameMode();
				inputbox.DoModal();
				strDirectory = inputbox.GetInput();
				if (!inputbox.WasCancelled())
				{
					LVFINDINFO find;
					find.flags = LVFI_PARAM;
					int iItem = -1;
					for (POSITION pos = abstractFileList.GetHeadPosition(); pos;)
					{
						CAbstractFile* pAbstractFile = abstractFileList.GetNext(pos);
						pAbstractFile->SetDownloadDirectory(strDirectory);

						// update the displayed file
						find.lParam = (LPARAM)pAbstractFile;
						iItem = FindItem(&find);
						if (iItem != -1)
							SetItemText(iItem, colFolder, pAbstractFile->GetDownloadDirectory(true));
					}
				}
				return TRUE;
			}

			case VK_SPACE:
			{
				if(m_bCheckBoxes)
				{
					POSITION pos = GetFirstSelectedItemPosition();
					while (pos != NULL)
					{
						int index = GetNextSelectedItem(pos);
						if (index >= 0)
							SetCheck(index, !GetCheck(index));
					}
					return TRUE;
				}
			}
        }
    }
    return __super::OnCommand(wParam, lParam);
}
//<<< WiZaRd::CollectionEnhancement

void CCollectionListCtrl::AddFileToList(CAbstractFile* pAbstractFile)
{
    LVFINDINFO find;
    find.flags = LVFI_PARAM;
    find.lParam = (LPARAM)pAbstractFile;
    int iItem = FindItem(&find);
    if (iItem != -1)
    {
        ASSERT(0);
        return;
    }

    int iImage = theApp.GetFileTypeSystemImageIdx(pAbstractFile->GetFileName());
    iItem = InsertItem(LVIF_TEXT | LVIF_PARAM | (iImage > 0 ? LVIF_IMAGE : 0), GetItemCount(), NULL, 0, 0, iImage, (LPARAM)pAbstractFile);
    if (iItem != -1)
    {
        SetItemText(iItem,colName,pAbstractFile->GetFileName());
        SetItemText(iItem,colSize,CastItoXBytes(pAbstractFile->GetFileSize()));
        SetItemText(iItem,colHash,::md4str(pAbstractFile->GetFileHash()));
        SetItemText(iItem, colFolder, pAbstractFile->GetDownloadDirectory(true)); //>>> WiZaRd::CollectionEnhancement
    }
}

void CCollectionListCtrl::RemoveFileFromList(CAbstractFile* pAbstractFile)
{
    LVFINDINFO find;
    find.flags = LVFI_PARAM;
    find.lParam = (LPARAM)pAbstractFile;
    int iItem = FindItem(&find);
    if (iItem != -1)
        DeleteItem(iItem);
    else
        ASSERT(0);
}

void CCollectionListCtrl::OnNmClick(NMHDR *pNMHDR, LRESULT * /*pResult*/)
{
	POINT pt;
	::GetCursorPos(&pt);
	ScreenToClient(&pt);
	if (pt.x < 16)
	{
		NMLISTVIEW *pNMListView = (NMLISTVIEW *)pNMHDR;
		BOOL bChecked = GetCheck(pNMListView->iItem);
		SetCheck(pNMListView->iItem, !bChecked);
	}
}

void CCollectionListCtrl::CheckAll()
{
	ASSERT(m_bCheckBoxes);
	for(int i = 0; i < GetItemCount(); ++i)
		SetCheck(i, TRUE);
}

void CCollectionListCtrl::UncheckAll()
{
	ASSERT(m_bCheckBoxes);
	for(int i = 0; i < GetItemCount(); ++i)
		SetCheck(i, FALSE);
}