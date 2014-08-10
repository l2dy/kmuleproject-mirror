//>>> the black hand::SplashScreenEx [shadow2004]
// SplashScreenEx.cpp : header file
// by John O'Byrne 01/10/2002
// partially rewritten by WiZaRd (thewizardofdos@gmail.com) for eMuleFuture Mod 28/11/2010

#pragma once

#define CSS_FADEIN		0x0001
#define CSS_FADEOUT		0x0002
#define CSS_FADE		CSS_FADEIN | CSS_FADEOUT
#define CSS_SHADOW		0x0004
#define CSS_CENTERSCREEN	0x0008
#define CSS_CENTERAPP		0x0010
#define CSS_HIDEONCLICK		0x0020
#define CSS_TRANSPARENCY	0x0040

#define CSS_TEXT_NORMAL		0x0000
#define CSS_TEXT_BOLD		0x0001
#define CSS_TEXT_ITALIC		0x0002
#define CSS_TEXT_UNDERLINE	0x0004

typedef BOOL (WINAPI* FN_ANIMATE_WINDOW)(HWND,DWORD,DWORD);

// CSplashScreenEx

class CSplashScreenEx : public CWnd
{
    DECLARE_DYNAMIC(CSplashScreenEx)

  public:
    CSplashScreenEx();
    virtual ~CSplashScreenEx();

    BOOL Create(CWnd* pWndParent, LPCTSTR szText = NULL, const DWORD /*dwTimeout*/ = 2000, const DWORD dwStyle = CSS_FADE | CSS_CENTERSCREEN | CSS_SHADOW);
    BOOL SetBitmap(const UINT nBitmapID, const COLORREF col);
    BOOL SetBitmap(LPCTSTR szFileName, const COLORREF col);

    void Show();
    void Hide();

    void SetText(LPCTSTR szText);
    void SetTextFont(LPCTSTR szFont, const int nSize, const int nStyle);
    void SetTextDefaultFont();
    void SetTextColor(COLORREF crTextColor);
    void SetTextRect();
    void SetTextRect(const CRect& rcText);
    void SetTextFormat(const UINT uTextFormat);

  private:
    void AdjustBitmap(const COLORREF col);

  protected:
    //FN_ANIMATE_WINDOW m_fnAnimateWindow;
    CWnd*	m_pWndParent;
    CBitmap m_bitmap;
    CFont	m_myFont;
//	CRgn*	m_hRegion;

    DWORD	m_dwStyle;
    //DWORD m_dwTimeout;
    CString m_strText;
    CRect	m_rcText;
    UINT	m_uTextFormat;
    COLORREF m_crTextColor;

    int m_nBitmapWidth;
    int m_nBitmapHeight;
    int m_nxPos;
    int m_nyPos;
    HRGN CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color);
    void DrawWindow(CDC* pDC);

  protected:
    DECLARE_MESSAGE_MAP()
  public:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    //afx_msg void OnTimer(UINT nIDEvent);
    LRESULT OnPrintClient(WPARAM wParam, LPARAM lParam);
  protected:
    //virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void PostNcDestroy();
};
//<<< the black hand::SplashScreenEx [shadow2004]