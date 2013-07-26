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
#pragma once

#if defined _DEBUG  && defined INFO_WND
const CString strDefaultToolbar = L"0001020304990506079908";
#else
const CString strDefaultToolbar = L"000102030499050607";
#endif

#define IDC_TOOLBAR			16127
#define IDC_TOOLBARBUTTON	16129

#define	TBBTN_TRANSFERS	(IDC_TOOLBARBUTTON	+ 0)
#define	TBBTN_SEARCH	(TBBTN_TRANSFERS	+ 1)
#define	TBBTN_SHARED	(TBBTN_SEARCH		+ 1)
#define	TBBTN_MESSAGES	(TBBTN_SHARED		+ 1)
#define	TBBTN_STATS		(TBBTN_MESSAGES		+ 1)
#define	TBBTN_OPTIONS	(TBBTN_STATS		+ 1)
#define	TBBTN_TOOLS		(TBBTN_OPTIONS		+ 1)
#ifdef INFO_WND
//>>> WiZaRd::InfoWnd
#define TBBTN_INFO		(TBBTN_TOOLS		+ 1)
#define	TBBTN_HELP		(TBBTN_INFO			+ 1)

#define TBBTN_COUNT		(9)
//<<< WiZaRd::InfoWnd
#else
#define	TBBTN_HELP		(TBBTN_TOOLS		+ 1)

#define TBBTN_COUNT		(8)
#endif

#define	MULE_TOOLBAR_BAND_NR	0

enum EToolbarLabelType
{
    NoLabels	= 0,
    LabelsBelow = 1,
    LabelsRight = 2
};

class CMuleToolbarCtrl : public CToolBarCtrl
{
    DECLARE_DYNAMIC(CMuleToolbarCtrl)

public:
    CMuleToolbarCtrl();
    virtual ~CMuleToolbarCtrl();

    void Init();
    void Localize();
    void Refresh();
    void SaveCurHeight();
    void UpdateBackground();
    void PressMuleButton(int nID);
    BOOL GetMaxSize(LPSIZE pSize) const;

    static int GetDefaultLabelType()
    {
        return (int)LabelsBelow;
    }

protected:
    CSize		m_sizBtnBmp;
    int			m_iPreviousHeight;
    int			m_iLastPressedButton;
    int			m_buttoncount;
    TBBUTTON	TBButtons[TBBTN_COUNT];
    TCHAR		TBStrings[TBBTN_COUNT][200];
    CStringArray m_astrToolbarPaths;
    EToolbarLabelType m_eLabelType;
    CStringArray m_astrSkinPaths;
    CBitmap		m_bmpBack;

    void ChangeToolbarBitmap(const CString& rstrPath, bool bRefresh);
    void ChangeTextLabelStyle(EToolbarLabelType eLabelType, bool bRefresh, bool bForceUpdateButtons = false);
    void UpdateIdealSize();
    void SetAllButtonsStrings();
    void SetAllButtonsWidth();
    void ForceRecalcLayout();

#ifdef _DEBUG
    void Dump();
#endif

    void AutoSize();
    virtual	BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnNmRClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSysColorChange();
    afx_msg void OnTbnEndAdjust(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTbnGetButtonInfo(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTbnInitCustomize(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTbnQueryDelete(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTbnQueryInsert(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTbnReset(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTbnToolbarChange(NMHDR *pNMHDR, LRESULT *pResult);
};
