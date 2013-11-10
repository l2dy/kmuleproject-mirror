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

#pragma once
#include "MuleListCtrl.h"
#include "ListCtrlItemWalk.h"

class CAbstractFile;

class CCollectionListCtrl : public CMuleListCtrl, public CListCtrlItemWalk
{
    DECLARE_DYNAMIC(CCollectionListCtrl)

public:
    CCollectionListCtrl();

    void Init(CString strNameAdd);
    void Localize();

    void AddFileToList(CAbstractFile* pAbstractFile);
    void RemoveFileFromList(CAbstractFile* pAbstractFile);
    void SetContextMenu(const bool b)	{m_bContextMenu = b;}
    void SetCheckboxes(const bool b)	{m_bCheckBoxes = b;}
    void CheckAll();
    void UncheckAll();

protected:
    bool	m_bCheckBoxes;
    bool	m_bContextMenu;
    CImageList m_ImageList;

    void SetAllIcons();
    void GetItemDisplayText(const CAbstractFile *pFile, int iSubItem, LPTSTR pszText, int cchTextMax);
    static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam); //>>> WiZaRd::CollectionEnhancement
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point); //>>> WiZaRd::CollectionEnhancement
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnNmClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnListModified(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSysColorChange();
};
