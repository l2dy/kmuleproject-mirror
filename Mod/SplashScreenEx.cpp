//>>> the black hand::SplashScreenEx [shadow2004]
// SplashScreenEx.cpp : implementation file
// by John O'Byrne 01/10/2002
// partially rewritten by WiZaRd (thewizardofdos@gmail.com) for eMuleFuture Mod 28/11/2010

#include "stdafx.h"
#include "SplashScreenEx.h"
#include "Preferences.h"
#include "emule.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if (WINVER < 0x0500)
/* AnimateWindow() Commands */
#define AW_HOR_POSITIVE             0x00000001
#define AW_HOR_NEGATIVE             0x00000002
#define AW_VER_POSITIVE             0x00000004
#define AW_VER_NEGATIVE             0x00000008
#define AW_CENTER                   0x00000010
#define AW_HIDE                     0x00010000
#define AW_ACTIVATE                 0x00020000
#define AW_SLIDE                    0x00040000
#define AW_BLEND                    0x00080000
#endif

#ifndef AW_HIDE
#define AW_HIDE		0x00010000
#endif
#ifndef AW_BLEND
#define AW_BLEND	0x00080000
#endif
#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW   0x00020000
#endif

// CSplashScreenEx

IMPLEMENT_DYNAMIC(CSplashScreenEx, CWnd)
BEGIN_MESSAGE_MAP(CSplashScreenEx, CWnd)
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_MESSAGE(WM_PRINTCLIENT, OnPrintClient)
//	ON_WM_TIMER()
END_MESSAGE_MAP()

CSplashScreenEx::CSplashScreenEx()
{
    m_pWndParent = NULL;
    m_strText = L"";
//	m_hRegion = NULL;
    m_nBitmapWidth = 0;
    m_nBitmapHeight = 0;
    m_nxPos = 0;
    m_nyPos = 0;
    //m_dwTimeout = 2000;
    m_dwStyle = 0;
    m_rcText.SetRect(0, 0, 0, 0);
    m_crTextColor = RGB(0, 0, 0);
    m_uTextFormat = DT_CENTER | DT_VCENTER | DT_WORDBREAK;

    //HMODULE hUser32 = GetModuleHandle(L"USER32.DLL");
    //if (hUser32!=NULL)
    //m_fnAnimateWindow = (FN_ANIMATE_WINDOW)GetProcAddress(hUser32, _TWINAPI("AnimateWindow"));
//	(FARPROC&)m_fnAnimateWindow = GetProcAddress(GetModuleHandle(L"user32"), "AnimateWindow");
    //else
    //	m_fnAnimateWindow = NULL;

    SetTextDefaultFont();
}

CSplashScreenEx::~CSplashScreenEx()
{
}

BOOL CSplashScreenEx::Create(CWnd* pWndParent, LPCTSTR szText, const DWORD /*dwTimeout*/, const DWORD dwStyle)
{
    //ASSERT(pWndParent!=NULL);
    //m_pWndParent = pWndParent;
    if (pWndParent)
        m_pWndParent = pWndParent;
    else
        m_pWndParent = this;

    if (szText != NULL)
        m_strText = szText;
    else
        m_strText = L"";

    //m_dwTimeout = dwTimeout;
    m_dwStyle = dwStyle;

    WNDCLASSEX wcx;
    memset(&wcx, 0, sizeof(WNDCLASSEX));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = AfxWndProc;
    wcx.style = CS_DBLCLKS|CS_SAVEBITS;
//	wcx.cbClsExtra = 0;
//	wcx.cbWndExtra = 0;
    wcx.hInstance = AfxGetInstanceHandle();
//	wcx.hIcon = NULL;
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground=::GetSysColorBrush(COLOR_WINDOW);
//	wcx.lpszMenuName = NULL;
    wcx.lpszClassName = L"SplashScreenExClass";
//	wcx.hIconSm = NULL;

    //Drop support for drop shadows because of
    //ms-help://MS.VSCC.2003/MS.MSDNQTR.2005JUL.1033/winui/winui/windowsuserinterface/windowing/windows/windowreference/windowfunctions/animatewindow.htm
    //namely: "Avoid animating a window that has a drop shadow because it produces visually distracting, jerky animations."
    if (m_dwStyle & CSS_SHADOW)
        wcx.style |= CS_DROPSHADOW;

    ATOM classAtom = RegisterClassEx(&wcx);

    // didn't work? try not using dropshadow (may not be supported)
    if (classAtom == NULL)
    {
        if (m_dwStyle & CSS_SHADOW)
        {
            wcx.style &= ~CS_DROPSHADOW;
            classAtom = RegisterClassEx(&wcx);
            //happens once in a while and is *NOT* lethal!
//			if(classAtom == NULL)
//				ASSERT(0);
        }
        else
            return FALSE;
    }

    if (!CreateEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"SplashScreenExClass", NULL, WS_POPUP, 0, 0, 0, 0, m_pWndParent ? m_pWndParent->m_hWnd : NULL, NULL))
        return FALSE;

    return TRUE;
}

BOOL CSplashScreenEx::SetBitmap(const UINT nBitmapID, const COLORREF col)
{
    m_bitmap.DeleteObject();
    if (!m_bitmap.LoadBitmap(nBitmapID))
        return FALSE;

    AdjustBitmap(col);

    return TRUE;
}

BOOL CSplashScreenEx::SetBitmap(LPCTSTR szFileName, const COLORREF col)
{
    m_bitmap.DeleteObject();

    HBITMAP hBmp = theApp.LoadImage(szFileName, L"JPG");
    //HBITMAP hBmp = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), szFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hBmp || !m_bitmap.Attach(hBmp))
        return FALSE;

    AdjustBitmap(col);

    return TRUE;
}

void CSplashScreenEx::AdjustBitmap(const COLORREF col)
{
    BITMAP bm;
    GetObject(m_bitmap.GetSafeHandle(), sizeof(bm), &bm);
    m_nBitmapWidth = bm.bmWidth;
    m_nBitmapHeight = bm.bmHeight;
    m_rcText.SetRect(0, 0, bm.bmWidth, bm.bmHeight);

    if (m_dwStyle & CSS_CENTERSCREEN)
    {
        m_nxPos = (GetSystemMetrics(SM_CXFULLSCREEN)-bm.bmWidth)/2;
        m_nyPos = (GetSystemMetrics(SM_CYFULLSCREEN)-bm.bmHeight)/2;
    }
    else if (m_dwStyle & CSS_CENTERAPP)
    {
        CRect rcParentWindow;
        ASSERT(m_pWndParent != NULL);
        m_pWndParent->GetWindowRect(&rcParentWindow);
        m_nxPos = rcParentWindow.left+(rcParentWindow.right-rcParentWindow.left-bm.bmWidth)/2;
        m_nyPos = rcParentWindow.top+(rcParentWindow.bottom-rcParentWindow.top-bm.bmHeight)/2;
    }

    //Buggy!?
//	if (red != -1 && green != -1 && blue != -1)
    {
        HRGN m_hRegion = CreateRgnFromBitmap((HBITMAP)m_bitmap.GetSafeHandle(), col/*RGB(red, green, blue)*/);
        if (m_hRegion != NULL && SetWindowRgn(m_hRegion, IsWindowVisible())) //FALSE); //was TRUE - WiZaRd - FiX?
            m_dwStyle |= CSS_TRANSPARENCY;
    }
}

void CSplashScreenEx::SetTextFont(LPCTSTR szFont, const int nSize, const int nStyle)
{
    LOGFONT lf;
    m_myFont.DeleteObject();
    m_myFont.CreatePointFont(nSize * 10, szFont);
    m_myFont.GetLogFont(&lf);

    if (nStyle & CSS_TEXT_BOLD)
        lf.lfWeight = FW_BOLD;
    else
        lf.lfWeight = FW_NORMAL;

    if (nStyle & CSS_TEXT_ITALIC)
        lf.lfItalic = TRUE;
    else
        lf.lfItalic = FALSE;

    if (nStyle & CSS_TEXT_UNDERLINE)
        lf.lfUnderline = TRUE;
    else
        lf.lfUnderline = FALSE;

    m_myFont.DeleteObject();
    m_myFont.CreateFontIndirect(&lf);
}

void CSplashScreenEx::SetTextDefaultFont()
{
    CFont* myFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
    LOGFONT lf;
    myFont->GetLogFont(&lf);
    m_myFont.DeleteObject();
    m_myFont.CreateFontIndirect(&lf);
}

void CSplashScreenEx::SetText(LPCTSTR szText)
{
    m_strText = szText;
    if (IsWindowVisible()) //>>> WiZaRd
        RedrawWindow(&m_rcText);
}

void CSplashScreenEx::SetTextColor(COLORREF crTextColor)
{
    m_crTextColor = crTextColor;
    if (IsWindowVisible()) //>>> WiZaRd
        RedrawWindow(&m_rcText);
}

void CSplashScreenEx::SetTextRect()
{
    SetTextRect(CRect(0, m_nBitmapHeight-40, m_nBitmapWidth, m_nBitmapHeight));
}

void CSplashScreenEx::SetTextRect(const CRect& rcText)
{
    m_rcText = rcText;
    if (IsWindowVisible()) //>>> WiZaRd
        RedrawWindow();
}

void CSplashScreenEx::SetTextFormat(const UINT uTextFormat)
{
    m_uTextFormat = uTextFormat;
}

void CSplashScreenEx::Show()
{
    SetWindowPos(&CWnd::wndNoTopMost, m_nxPos, m_nyPos, m_nBitmapWidth, m_nBitmapHeight, SWP_HIDEWINDOW | SWP_NOOWNERZORDER | SWP_NOREDRAW /*| SWP_NOZORDER*/);
    //SetWindowPos(NULL, m_nxPos, m_nyPos, m_nBitmapWidth, m_nBitmapHeight, SWP_NOOWNERZORDER | SWP_NOZORDER);

    BOOL (WINAPI *pfnAnimateWindow)(HWND hWnd, DWORD dwTime, DWORD dwFlags);
    (FARPROC&)pfnAnimateWindow = GetProcAddress(GetModuleHandle(L"user32"), "AnimateWindow");
    if (!thePrefs.IsRunningAeroGlassTheme() && (m_dwStyle & CSS_FADEIN) && pfnAnimateWindow != NULL)
        pfnAnimateWindow(m_hWnd, 500, AW_BLEND);
    else
    {
        ShowWindow(SW_SHOW);
        if (IsWindowVisible()) //>>> WiZaRd
            RedrawWindow();
    }

    //if (m_dwTimeout!=0)
    //	SetTimer(0,m_dwTimeout,NULL);
}

void CSplashScreenEx::Hide()
{
    BOOL (WINAPI *pfnAnimateWindow)(HWND hWnd, DWORD dwTime, DWORD dwFlags);
    (FARPROC&)pfnAnimateWindow = GetProcAddress(GetModuleHandle(L"user32"), "AnimateWindow");
    //obviously there is not an issue if you HIDE the element
    if (/*!thePrefs.IsRunningAeroGlassTheme() &&*/ (m_dwStyle & CSS_FADEOUT) && pfnAnimateWindow != NULL)
        pfnAnimateWindow(m_hWnd, 200, AW_HIDE | AW_BLEND);
    else
        ShowWindow(SW_HIDE);

    DestroyWindow();
}

HRGN CSplashScreenEx::CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color)
{
    if (!hBmp)
        return NULL;

    CDC* pDC = GetDC();
    //is it necessary to be != NULL?
//	if (!pDC)
//		return NULL;

    BITMAP bm;
    GetObject(hBmp, sizeof(BITMAP), &bm);	// get bitmap attributes

    CDC dcBmp;
    dcBmp.CreateCompatibleDC(pDC);	//Creates a memory device context for the bitmap
    /*HGDIOBJ hOldBmp =*/
    dcBmp.SelectObject(hBmp);			//selects the bitmap in the device context

    const DWORD RDHDR = sizeof(RGNDATAHEADER);
    const DWORD MAXBUF = 40;		// size of one block in RECTs
    // (i.e. MAXBUF*sizeof(RECT) in bytes)
    DWORD	cBlocks = 0;			// number of allocated blocks

    // allocate memory for region data
    RGNDATAHEADER* pRgnData = (RGNDATAHEADER*)new BYTE[ RDHDR + ++cBlocks * MAXBUF * sizeof(RECT) ];
    memset(pRgnData, 0, RDHDR + cBlocks * MAXBUF * sizeof(RECT));

    // fill it by default
    pRgnData->dwSize	= RDHDR;
    pRgnData->iType		= RDH_RECTANGLES;
    pRgnData->nCount	= 0;

    LPRECT	pRects;
//	int		i, j;					// current position in mask image
    int		first = 0;				// left position of current scan line
    // where mask was found
    bool	wasfirst = false;		// set when if mask was found in current scan line
    bool	ismask; 				// set when current color is mask color

    for (int i = 0; i < bm.bmHeight; ++i)
    {
        for (int j = 0; j < bm.bmWidth; ++j)
        {
            // get color
            ismask = dcBmp.GetPixel(j, bm.bmHeight-i-1) != color;
            // place part of scan line as RECT region if transparent color found after mask color or
            // mask color found at the end of mask image
            if (wasfirst && ((ismask && (j==(bm.bmWidth-1))) || (ismask ^ (j<bm.bmWidth))))
            {
                // get offset to RECT array if RGNDATA buffer
                pRects = (LPRECT)((LPBYTE)pRgnData + RDHDR);
                // save current RECT
                pRects[pRgnData->nCount++] = CRect(first, bm.bmHeight-i-1, j+(j==(bm.bmWidth-1)), bm.bmHeight-i);
                // if buffer full reallocate it
                if (pRgnData->nCount >= cBlocks * MAXBUF)
                {
                    LPBYTE pRgnDataNew = new BYTE[ RDHDR + ++cBlocks * MAXBUF * sizeof(RECT)];
                    memcpy(pRgnDataNew, pRgnData, RDHDR + (cBlocks - 1) * MAXBUF * sizeof(RECT));
                    delete[] pRgnData; //>>> WiZaRd::Memleak FiX
                    pRgnData = (RGNDATAHEADER*)pRgnDataNew;
                }
                wasfirst = false;
            }
            else if (!wasfirst && ismask)		// set wasfirst when mask is found
            {
                first = j;
                wasfirst = true;
            }
        }
    }

//	dcBmp.SelectObject(hOldBmp);
//	dcBmp.DeleteDC();	//release the bitmap
    ::ReleaseDC(NULL, dcBmp);
    // create region
    //  Under WinNT the ExtCreateRegion returns NULL (by Fable@aramszu.net)
    //	HRGN hRgn = ExtCreateRegion(NULL, RDHDR + pRgnData->nCount * sizeof(RECT), (LPRGNDATA)pRgnData);
    /* ExtCreateRegion replacement */
    HRGN hRgn = CreateRectRgn(0, 0, 0, 0);
    {
        ASSERT(hRgn!=NULL);
        pRects = (LPRECT)((LPBYTE)pRgnData + RDHDR);
        for (int i = 0; i < (int)pRgnData->nCount; ++i)
        {
            HRGN hr = CreateRectRgn(pRects[i].left, pRects[i].top, pRects[i].right, pRects[i].bottom);
            VERIFY(CombineRgn(hRgn, hRgn, hr, RGN_OR) != ERROR);
            if (hr)
                DeleteObject(hr);
        }
        ASSERT(hRgn!=NULL);
    }
    /* ExtCreateRegion replacement */

    delete[] pRgnData; //>>> WiZaRd::Memleak FiX
    ReleaseDC(pDC);
    return hRgn;
}

void CSplashScreenEx::DrawWindow(CDC* pDC)
{
    // Blit Background
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    CBitmap* pOldBitmap = memDC.SelectObject(&m_bitmap);
    pDC->BitBlt(0, 0, m_nBitmapWidth, m_nBitmapHeight, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBitmap);

    // Draw Text
    CFont* pOldFont = pDC->SelectObject(&m_myFont);
    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(m_crTextColor);

    pDC->DrawText(m_strText, -1, m_rcText, m_uTextFormat);

    pDC->SelectObject(pOldFont);
}

// CSplashScreenEx message handlers

BOOL CSplashScreenEx::OnEraseBkgnd(CDC* /*pDC*/)
{
    //WiZaRd - FALSE will be flicker-free?
    //return TRUE;
    return FALSE; //possibly solves transparency issues
}

void CSplashScreenEx::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    DrawWindow(&dc);
}

LRESULT CSplashScreenEx::OnPrintClient(WPARAM wParam, LPARAM /*lParam*/)
{
    CDC* pDC = CDC::FromHandle((HDC)wParam);
    DrawWindow(pDC);
    return 1;
}

/*
BOOL CSplashScreenEx::PreTranslateMessage(MSG* pMsg)
{
	// If a key is pressed, Hide the Splash Screen and destroy it
	if (m_dwStyle & CSS_HIDEONCLICK)
	{
		if (pMsg->message == WM_KEYDOWN ||
			pMsg->message == WM_SYSKEYDOWN ||
			pMsg->message == WM_LBUTTONDOWN ||
			pMsg->message == WM_RBUTTONDOWN ||
			pMsg->message == WM_MBUTTONDOWN ||
			pMsg->message == WM_NCLBUTTONDOWN ||
			pMsg->message == WM_NCRBUTTONDOWN ||
			pMsg->message == WM_NCMBUTTONDOWN)
		{
			Hide();
			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}
*/

/*void CSplashScreenEx::OnTimer(UINT nIDEvent)
{
	KillTimer(0);
	Hide();
}*/

void CSplashScreenEx::PostNcDestroy()
{
    CWnd::PostNcDestroy();
    delete this;
}
//<<< the black hand::SplashScreenEx [shadow2004]