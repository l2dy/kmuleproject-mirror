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
#include "CollectionListCtrl.h"
#include "OtherFunctions.h"
#include "AbstractFile.h"
#include "MetaDataDlg.h"
#include "HighColorTab.hpp"
#include "ListViewWalkerPropertySheet.h"
#include "UserMsgs.h"
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
    ON_NOTIFY_REFLECT(NM_RCLICK, OnNmRClick)
END_MESSAGE_MAP()

CCollectionListCtrl::CCollectionListCtrl()
    : CListCtrlItemWalk(this)
{
}

CCollectionListCtrl::~CCollectionListCtrl()
{
}

void CCollectionListCtrl::Init(CString strNameAdd)
{
    SetPrefsKey(_T("CollectionListCtrl") + strNameAdd);

    ASSERT(GetStyle() & LVS_SHAREIMAGELISTS);
    SendMessage(LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)theApp.GetSystemImageList());

    ASSERT((GetStyle() & LVS_SINGLESEL) == 0);
    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

    InsertColumn(colName, GetResString(IDS_DL_FILENAME),	LVCFMT_LEFT,  DFLT_FILENAME_COL_WIDTH);
    InsertColumn(colSize, GetResString(IDS_DL_SIZE),		LVCFMT_RIGHT, DFLT_SIZE_COL_WIDTH);
    InsertColumn(colHash, GetResString(IDS_FILEHASH),		LVCFMT_LEFT,  DFLT_HASH_COL_WIDTH);
    InsertColumn(colFolder, GetResString(IDS_FOLDER),		LVCFMT_LEFT,  DFLT_FOLDER_COL_WIDTH); //>>> WiZaRd::CollectionEnhancement

    LoadSettings();
    SetSortArrow();
    SortItems(SortProc, MAKELONG(GetSortItem(), (GetSortAscending() ? 0 : 1)));
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

void CCollectionListCtrl::OnNmRClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
//>>> WiZaRd::CollectionEnhancement
    UINT flag = MF_STRING;
    if (GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED) == -1)
        flag = MF_GRAYED;

    POINT point;
    ::GetCursorPos(&point);

    CTitleMenu popupMenu;
    popupMenu.CreatePopupMenu();
    popupMenu.AppendMenu(flag, MP_META_DATA, GetResString(IDS_META_DATA));
    popupMenu.AppendMenu(flag, MP_RENAME, GetResString(IDS_FOLDER));
    popupMenu.SetDefaultItem(MP_META_DATA);

    popupMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
    VERIFY(popupMenu.DestroyMenu());

    *pResult = 0;
//<<< WiZaRd::CollectionEnhancement
}

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

//>>> WiZaRd::CollectionEnhancement
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
            if (!abstractFileList.IsEmpty())
            {
                CCollectionFileDetailsSheet dialog(abstractFileList, 0, this);
                dialog.DoModal();
            }
            return TRUE;
        }
        case MP_RENAME:
        {
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
        }
    }
    return __super::OnCommand(wParam, lParam);
}
//<<< WiZaRd::CollectionEnhancement