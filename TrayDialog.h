#pragma once
#include "DialogMinTrayBtn.h"
#include "ResizableLib\ResizableDialog.h"

#define	IDT_SINGLE_CLICK	100

class CTrayDialog : public CDialogMinTrayBtn<CResizableDialog>
{
  protected:
    typedef CDialogMinTrayBtn<CResizableDialog> CTrayDialogBase;

  public:
    CTrayDialog(UINT uIDD, CWnd* pParent = NULL);   // standard constructor

    void TraySetMinimizeToTray(bool* pbMinimizeToTray);
    BOOL TraySetMenu(const UINT nResourceID);
    BOOL TraySetMenu(HMENU hMenu);
    BOOL TraySetMenu(LPCTSTR lpszMenuName);
    BOOL TrayUpdate();
    BOOL TrayShow();
    BOOL TrayHide();
    void TraySetToolTip(LPCTSTR lpszToolTip);
    void TraySetIcon(HICON hIcon, const bool bDelete = false);
//	void TraySetIcon(const UINT nResourceID, const bool bDelete= false);
//	void TraySetIcon(LPCTSTR lpszResourceName, const bool bDelete = false);
    bool TrayIsVisible() const;

    virtual void TrayMinimizeToTrayChange();
    virtual void RestoreWindow();
    virtual void OnTrayLButtonDown(CPoint pt);
    virtual void OnTrayLButtonUp(CPoint pt);
    virtual void OnTrayLButtonDblClk(CPoint pt);
    virtual void OnTrayRButtonUp(CPoint pt);
    virtual void OnTrayRButtonDblClk(CPoint pt);
    virtual void OnTrayMouseMove(CPoint pt);

  protected:
    bool* m_pbMinimizeToTray;
    bool m_bCurIconDelete;
    HICON m_hPrevIconDelete;
    bool m_bLButtonDblClk;
    bool m_bLButtonDown;
    bool			m_bTrayIconVisible;
    NOTIFYICONDATA m_nidIconData;
    CMenu m_mnuTrayMenu;
    UINT m_nDefaultMenuItem;
    UINT m_uSingleClickTimer;

    void KillSingleClickTimer();

    DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTaskBarCreated(WPARAM wParam, LPARAM lParam);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnTimer(UINT nIDEvent);
};
