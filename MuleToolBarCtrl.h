#pragma once

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
