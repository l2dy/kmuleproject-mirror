#pragma once

struct SSearchParams;
class CSearchResultsWnd;
class CSearchParamsWnd;
class CSearchFile;
class CClosableTabCtrl;


///////////////////////////////////////////////////////////////////////////////
// CSearchDlg frame

class CSearchDlg : public CFrameWnd
{
    DECLARE_DYNCREATE(CSearchDlg)

  public:
    CSearchDlg();           // protected constructor used by dynamic creation
    virtual ~CSearchDlg();
    CSearchResultsWnd* m_pwndResults;

    BOOL Create(CWnd* pParent);

    void Localize();
    void CreateMenus();

    void RemoveResult(const CSearchFile* toremove);

    bool DoNewKadSearch(SSearchParams* pParams);
    void CancelKadSearch(UINT uSearchID);
    void ProcessEd2kSearchLinkRequest(CString strSearchTerm);

    void DownloadSelected();
    void DownloadSelected(bool paused);

    bool CanDeleteSearch(UINT nSearchID) const;
    bool CanDeleteAllSearches() const;
    void DeleteSearch(UINT nSearchID);
    void DeleteAllSearches();

    bool CreateNewTab(SSearchParams* pParams, bool bActiveIcon = true);
    SSearchParams* GetSearchParamsBySearchID(UINT nSearchID);
    void ShowSearchSelector(bool visible);
    CClosableTabCtrl& GetSearchSelector();

    int GetSelectedCat();
    void UpdateCatTabs();
    void SaveAllSettings();
    BOOL SaveSearchStrings();
    void ResetHistory();

    void SetToolTipsDelay(UINT uDelay);
    void DeleteAllSearchListCtrlItems();

    BOOL IsSearchParamsWndVisible() const;
    void OpenParametersWnd();
    void DockParametersWnd();

    void UpdateSearch(CSearchFile* pSearchFile);

  protected:
    CSearchParamsWnd* m_pwndParams;

    virtual BOOL PreTranslateMessage(MSG* pMsg);

    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnClose();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

//>>> WiZaRd:CustomSearches
  public:
    void UpdateSearchList();
//<<< WiZaRd:CustomSearches
};
