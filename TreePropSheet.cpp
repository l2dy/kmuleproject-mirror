/********************************************************************
*
* Copyright (c) 2002 Sven Wiegand <mail@sven-wiegand.de>
*
* You can use this and modify this in any way you want,
* BUT LEAVE THIS HEADER INTACT.
*
* Redistribution is appreciated.
*
* $Workfile:$
* $Revision:$
* $Modtime:$
* $Author:$
*
* Revision History:
*	$History:$
*
*********************************************************************/
#include "stdafx.h"
#include "emule.h"
#include "TreePropSheet.h"
#include "TreePropSheetPgFrameDef.h"
#include "HighColorTab.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//-------------------------------------------------------------------
// class CTreePropSheet
//-------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CTreePropSheet, CPropertySheet)
    //{{AFX_MSG_MAP(CTreePropSheet)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
    ON_MESSAGE(PSM_ADDPAGE, OnAddPage)
    ON_MESSAGE(PSM_REMOVEPAGE, OnRemovePage)
    ON_MESSAGE(PSM_SETCURSEL, OnSetCurSel)
    ON_MESSAGE(PSM_SETCURSELID, OnSetCurSelId)
    ON_MESSAGE(PSM_ISDIALOGMESSAGE, OnIsDialogMessage)

    ON_NOTIFY(TVN_SELCHANGINGA, s_unPageTreeId, OnPageTreeSelChanging)
    ON_NOTIFY(TVN_SELCHANGINGW, s_unPageTreeId, OnPageTreeSelChanging)
    ON_NOTIFY(TVN_SELCHANGEDA, s_unPageTreeId, OnPageTreeSelChanged)
    ON_NOTIFY(TVN_SELCHANGEDW, s_unPageTreeId, OnPageTreeSelChanged)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CTreePropSheet, CPropertySheet)

const UINT CTreePropSheet::s_unPageTreeId = 0x7EEE;

CTreePropSheet::CTreePropSheet()
    :	CPropertySheet(),
      m_bPageTreeSelChangedActive(FALSE),
      m_bTreeViewMode(TRUE),
      m_bPageCaption(FALSE),
      m_bTreeImages(FALSE),
      m_nPageTreeWidth(150),
      m_pwndPageTree(NULL),
      m_pFrame(NULL)
{}


CTreePropSheet::CTreePropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :	CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
      m_bPageTreeSelChangedActive(FALSE),
      m_bTreeViewMode(TRUE),
      m_bPageCaption(FALSE),
      m_bTreeImages(FALSE),
      m_nPageTreeWidth(150),
      m_pwndPageTree(NULL),
      m_pFrame(NULL)
{
}


CTreePropSheet::CTreePropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    :	CPropertySheet(pszCaption, pParentWnd, iSelectPage),
      m_bPageTreeSelChangedActive(FALSE),
      m_bTreeViewMode(TRUE),
      m_bPageCaption(FALSE),
      m_bTreeImages(FALSE),
      m_nPageTreeWidth(150),
      m_pwndPageTree(NULL),
      m_pFrame(NULL)
{
}


CTreePropSheet::~CTreePropSheet()
{
}


/////////////////////////////////////////////////////////////////////
// Operationen

BOOL CTreePropSheet::SetTreeViewMode(BOOL bTreeViewMode /* = TRUE */, BOOL bPageCaption /* = FALSE */, BOOL bTreeImages /* = FALSE */)
{
    if (IsWindow(m_hWnd))
    {
        // needs to becalled, before the window has been created
        ASSERT(0);
        return FALSE;
    }

    m_bTreeViewMode = bTreeViewMode;
    if (m_bTreeViewMode)
    {
        m_bPageCaption = bPageCaption;
        m_bTreeImages = bTreeImages;
    }

    return TRUE;
}


BOOL CTreePropSheet::SetTreeWidth(int nWidth)
{
    if (IsWindow(m_hWnd))
    {
        // needs to be called, before the window is created.
        ASSERT(0);
        return FALSE;
    }

    m_nPageTreeWidth = nWidth;

    return TRUE;
}


void CTreePropSheet::SetEmptyPageText(LPCTSTR lpszEmptyPageText)
{
    m_strEmptyPageMessage = lpszEmptyPageText;
}


DWORD	CTreePropSheet::SetEmptyPageTextFormat(DWORD dwFormat)
{
    DWORD	dwPrevFormat = m_pFrame->GetMsgFormat();
    m_pFrame->SetMsgFormat(dwFormat);
    return dwPrevFormat;
}


BOOL CTreePropSheet::SetTreeDefaultImages(CImageList *pImages)
{
    if (pImages->GetImageCount() != 2)
    {
        ASSERT(0);
        return FALSE;
    }

    if (m_DefaultImages.GetSafeHandle())
        m_DefaultImages.DeleteImageList();
    m_DefaultImages.Create(pImages);

    // update, if necessary
    if (IsWindow(m_hWnd))
        RefillPageTree();

    return TRUE;
}


BOOL CTreePropSheet::SetTreeDefaultImages(UINT unBitmapID, int cx, COLORREF crMask)
{
    if (m_DefaultImages.GetSafeHandle())
        m_DefaultImages.DeleteImageList();
    if (!m_DefaultImages.Create(unBitmapID, cx, 0, crMask))
        return FALSE;

    if (m_DefaultImages.GetImageCount() != 2)
    {
        m_DefaultImages.DeleteImageList();
        return FALSE;
    }

    return TRUE;
}


CTreeCtrl* CTreePropSheet::GetPageTreeControl()
{
    return m_pwndPageTree;
}


/////////////////////////////////////////////////////////////////////
// public helpers

BOOL CTreePropSheet::SetPageIcon(CPropertyPage *pPage, HICON hIcon)
{
    pPage->m_psp.dwFlags|= PSP_USEHICON;
    pPage->m_psp.hIcon = hIcon;
    return TRUE;
}

BOOL CTreePropSheet::SetPageIcon(CPropertyPage *pPage, LPCTSTR pszIconId)
{
    pPage->m_psp.dwFlags|= PSP_USEICONID;
    pPage->m_psp.pszIcon = pszIconId;
    return TRUE;
}

BOOL CTreePropSheet::SetPageIcon(CPropertyPage *pPage, UINT unIconId)
{
    HICON	hIcon = AfxGetApp()->LoadIcon(unIconId);
    if (!hIcon)
        return FALSE;

    return SetPageIcon(pPage, hIcon);
}


BOOL CTreePropSheet::SetPageIcon(CPropertyPage *pPage, CImageList &Images, int nImage)
{
    HICON	hIcon = Images.ExtractIcon(nImage);
    if (!hIcon)
        return FALSE;

    return SetPageIcon(pPage, hIcon);
}


BOOL CTreePropSheet::DestroyPageIcon(CPropertyPage *pPage)
{
    if (!pPage || !(pPage->m_psp.dwFlags&PSP_USEHICON) || !pPage->m_psp.hIcon)
        return FALSE;

    DestroyIcon(pPage->m_psp.hIcon);
    pPage->m_psp.dwFlags&= ~PSP_USEHICON;
    pPage->m_psp.hIcon = NULL;

    return TRUE;
}


/////////////////////////////////////////////////////////////////////
// Overridable implementation helpers

CString CTreePropSheet::GenerateEmptyPageMessage(LPCTSTR lpszEmptyPageMessage, LPCTSTR lpszCaption)
{
    CString	strMsg;
    strMsg.Format(lpszEmptyPageMessage, lpszCaption);
    return strMsg;
}


CTreeCtrl* CTreePropSheet::CreatePageTreeObject()
{
    return new CTreeCtrl;
}


CPropPageFrame* CTreePropSheet::CreatePageFrame()
{
    return new CPropPageFrameDefault;
}


/////////////////////////////////////////////////////////////////////
// Implementation helpers

void CTreePropSheet::MoveChildWindows(int nDx, int nDy)
{
    CWnd* pWnd = GetWindow(GW_CHILD);
    while (pWnd)
    {
        CRect rect;
        pWnd->GetWindowRect(rect);
        ScreenToClient(rect);
        rect.OffsetRect(nDx, nDy);
        pWnd->MoveWindow(rect);

        pWnd = pWnd->GetNextWindow();
    }
}


void CTreePropSheet::RefillPageTree()
{
    if (!IsWindow(m_hWnd))
        return;

    m_pwndPageTree->DeleteAllItems();

    CTabCtrl	*pTabCtrl = GetTabControl();
    if (!IsWindow(pTabCtrl->GetSafeHwnd()))
    {
        ASSERT(0);
        return;
    }

    const int	nPageCount = pTabCtrl->GetItemCount();

    // rebuild image list
    if (m_bTreeImages)
    {
        for (int i = m_Images.GetImageCount()-1; i >= 0; --i)
            m_Images.Remove(i);

        // add page images
        CImageList	*pPageImages = pTabCtrl->GetImageList();
        if (pPageImages)
        {
            for (int nImage = 0; nImage < pPageImages->GetImageCount(); ++nImage)
            {
                HICON	hIcon = pPageImages->ExtractIcon(nImage);
                m_Images.Add(hIcon);
                DestroyIcon(hIcon);
            }
        }

        // add default images
        if (m_DefaultImages.GetSafeHandle())
        {
            HICON	hIcon;

            // add default images
            hIcon = m_DefaultImages.ExtractIcon(0);
            if (hIcon)
            {
                m_Images.Add(hIcon);
                DestroyIcon(hIcon);
            }
            hIcon = m_DefaultImages.ExtractIcon(1);
            {
                m_Images.Add(hIcon);
                DestroyIcon(hIcon);
            }
        }
    }

    // insert tree items
    for (int nPage = 0; nPage < nPageCount; ++nPage)
    {
        // Get title and image of the page
        CString	strPagePath;

        TCITEM	ti;
        ZeroMemory(&ti, sizeof(ti));
        ti.mask = TCIF_TEXT|TCIF_IMAGE;
        ti.cchTextMax = MAX_PATH;
        ti.pszText = strPagePath.GetBuffer(ti.cchTextMax);
        ASSERT(ti.pszText);
        if (!ti.pszText)
            return;

        pTabCtrl->GetItem(nPage, &ti);
        ti.pszText[ti.cchTextMax - 1] = L'\0';
        strPagePath.ReleaseBuffer();

        // Create an item in the tree for the page
        HTREEITEM	hItem = CreatePageTreeItem(ti.pszText);
        ASSERT(hItem);
        if (hItem)
        {
            m_pwndPageTree->SetItemData(hItem, nPage);

            // set image
            if (m_bTreeImages)
            {
                int	nImage = ti.iImage;
                if (nImage < 0 || nImage >= m_Images.GetImageCount())
                    nImage = m_DefaultImages.GetSafeHandle()? m_Images.GetImageCount()-1 : -1;

                m_pwndPageTree->SetItemImage(hItem, nImage, nImage);
            }
        }
    }
}


HTREEITEM CTreePropSheet::CreatePageTreeItem(LPCTSTR lpszPath, HTREEITEM hParent /* = TVI_ROOT */)
{
    CString		strPath(lpszPath);
    CString		strTopMostItem(SplitPageTreePath(strPath));

    // Check if an item with the given text does already exist
    HTREEITEM	hItem = NULL;
    HTREEITEM	hChild = m_pwndPageTree->GetChildItem(hParent);
    while (hChild)
    {
        if (m_pwndPageTree->GetItemText(hChild) == strTopMostItem)
        {
            hItem = hChild;
            break;
        }
        hChild = m_pwndPageTree->GetNextItem(hChild, TVGN_NEXT);
    }

    // If item with that text does not already exist, create a new one
    if (!hItem)
    {
        hItem = m_pwndPageTree->InsertItem(strTopMostItem, hParent);
        m_pwndPageTree->SetItemData(hItem, (DWORD_PTR)-1);
        if (!strPath.IsEmpty() && m_bTreeImages && m_DefaultImages.GetSafeHandle())
            // set folder image
            m_pwndPageTree->SetItemImage(hItem, m_Images.GetImageCount()-2, m_Images.GetImageCount()-2);
    }
    if (!hItem)
    {
        ASSERT(0);
        return NULL;
    }

    if (strPath.IsEmpty())
        return hItem;
    else
        return CreatePageTreeItem(strPath, hItem);
}


CString CTreePropSheet::SplitPageTreePath(CString &strRest)
{
    int	nSeperatorPos = 0;
    for (;;)
    {
        nSeperatorPos = strRest.Find(_T("::"), nSeperatorPos);
        if (nSeperatorPos == -1)
        {
            CString	strItem(strRest);
            strRest.Empty();
            return strItem;
        }
        else if (nSeperatorPos>0)
        {
            // if there is an odd number of backslashes infront of the
            // seperator, than do not interpret it as separator
            int	nBackslashCount = 0;
            for (int nPos = nSeperatorPos-1; nPos >= 0 && strRest[nPos]==_T('\\'); --nPos, ++nBackslashCount);
            if (nBackslashCount%2 == 0)
                break;
            else
                ++nSeperatorPos;
        }
    }

    CString	strItem(strRest.Left(nSeperatorPos));
    strItem.Replace(_T("\\::"), _T("::"));
    strItem.Replace(_T("\\\\"), L"\\");
    strRest = strRest.Mid(nSeperatorPos+2);
    return strItem;
}


BOOL CTreePropSheet::KillActiveCurrentPage()
{
    HWND	hCurrentPage = PropSheet_GetCurrentPageHwnd(m_hWnd);
    if (!IsWindow(hCurrentPage))
    {
        ASSERT(0);
        return TRUE;
    }

    // Check if the current page is really active (if page is invisible
    // an virtual empty page is the active one.
    if (!::IsWindowVisible(hCurrentPage))
        return TRUE;

    // Try to deactivate current page
    PSHNOTIFY	pshn;
    pshn.hdr.code = PSN_KILLACTIVE;
    pshn.hdr.hwndFrom = m_hWnd;
    pshn.hdr.idFrom = GetDlgCtrlID();
    pshn.lParam = 0;
    if (::SendMessage(hCurrentPage, WM_NOTIFY, pshn.hdr.idFrom, (LPARAM)&pshn))
        // current page does not allow page change
        return FALSE;

    // Hide the page
    ::ShowWindow(hCurrentPage, SW_HIDE);

    return TRUE;
}


HTREEITEM CTreePropSheet::GetPageTreeItem(int nPage, HTREEITEM hRoot /* = TVI_ROOT */)
{
    // Special handling for root case
    if (hRoot == TVI_ROOT)
        hRoot = m_pwndPageTree->GetNextItem(NULL, TVGN_ROOT);

    // Check parameters
    if (nPage < 0 || nPage >= GetPageCount())
    {
//        ASSERT(0);
        return NULL;
    }

    if (hRoot == NULL)
    {
        ASSERT(0);
        return NULL;
    }

    // we are performing a simple linear search here, because we are
    // expecting only little data
    HTREEITEM	hItem = hRoot;
    while (hItem)
    {
        if ((signed)m_pwndPageTree->GetItemData(hItem) == nPage)
            return hItem;
        if (m_pwndPageTree->ItemHasChildren(hItem))
        {
            HTREEITEM	hResult = GetPageTreeItem(nPage, m_pwndPageTree->GetNextItem(hItem, TVGN_CHILD));
            if (hResult)
                return hResult;
        }

        hItem = m_pwndPageTree->GetNextItem(hItem, TVGN_NEXT);
    }

    // we've found nothing, if we arrive here
    return hItem;
}


BOOL CTreePropSheet::SelectPageTreeItem(int nPage)
{
    HTREEITEM	hItem = GetPageTreeItem(nPage);
    if (!hItem)
        return FALSE;

    return m_pwndPageTree->SelectItem(hItem);
}


BOOL CTreePropSheet::SelectCurrentPageTreeItem()
{
    CTabCtrl	*pTab = GetTabControl();
    if (!IsWindow(pTab->GetSafeHwnd()))
        return FALSE;

    return SelectPageTreeItem(pTab->GetCurSel());
}


void CTreePropSheet::UpdateCaption()
{
    HWND			hPage = PropSheet_GetCurrentPageHwnd(GetSafeHwnd());
    BOOL			bRealPage = IsWindow(hPage) && ::IsWindowVisible(hPage);
    HTREEITEM	hItem = m_pwndPageTree->GetSelectedItem();
    if (!hItem)
        return;
    CString		strCaption = m_pwndPageTree->GetItemText(hItem);

    // if empty page, then update empty page message
    if (!bRealPage)
        m_pFrame->SetMsgText(GenerateEmptyPageMessage(m_strEmptyPageMessage, strCaption));

    // if no captions are displayed, cancel here
    if (!m_pFrame->GetShowCaption())
        return;

    // get tab control, to the the images from
    CTabCtrl	*pTabCtrl = GetTabControl();
    if (!IsWindow(pTabCtrl->GetSafeHwnd()))
    {
        ASSERT(0);
        return;
    }

    if (m_bTreeImages)
    {
        // get image from tree
        int	nImage;
        m_pwndPageTree->GetItemImage(hItem, nImage, nImage);
        HICON	hIcon = m_Images.ExtractIcon(nImage);
        m_pFrame->SetCaption(strCaption, hIcon);
        if (hIcon)
            DestroyIcon(hIcon);
    }
    else if (bRealPage)
    {
        // get image from hidden (original) tab provided by the original
        // implementation
        CImageList	*pImages = pTabCtrl->GetImageList();
        if (pImages)
        {
            TCITEM	ti;
            ZeroMemory(&ti, sizeof(ti));
            ti.mask = TCIF_IMAGE;

            HICON	hIcon = NULL;
            if (pTabCtrl->GetItem((int)m_pwndPageTree->GetItemData(hItem), &ti))
                hIcon = pImages->ExtractIcon(ti.iImage);

            m_pFrame->SetCaption(strCaption, hIcon);
            if (hIcon)
                DestroyIcon(hIcon);
        }
        else
            m_pFrame->SetCaption(strCaption);
    }
    else
        m_pFrame->SetCaption(strCaption);
}


void CTreePropSheet::ActivatePreviousPage()
{
    if (!IsWindow(m_hWnd))
        return;

    if (!IsWindow(m_pwndPageTree->GetSafeHwnd()))
    {
        // normal tab property sheet. Simply use page index
        int	nPageIndex = GetActiveIndex();
        if (nPageIndex<0 || nPageIndex>=GetPageCount())
            return;

        int	nPrevIndex = (nPageIndex==0)? GetPageCount()-1 : nPageIndex-1;
        SetActivePage(nPrevIndex);
    }
    else
    {
        // property sheet with page tree.
        // we need a more sophisticated handling here, than simply using
        // the page index, because we won't skip empty pages.
        // so we have to walk the page tree
        HTREEITEM	hItem = m_pwndPageTree->GetSelectedItem();
        ASSERT(hItem);
        if (!hItem)
            return;

        HTREEITEM	hPrevItem = NULL;
        if ((hPrevItem = m_pwndPageTree->GetPrevSiblingItem(hItem)) != NULL)
        {
            while (m_pwndPageTree->ItemHasChildren(hPrevItem))
            {
                hPrevItem = m_pwndPageTree->GetChildItem(hPrevItem);
                while (m_pwndPageTree->GetNextSiblingItem(hPrevItem))
                    hPrevItem = m_pwndPageTree->GetNextSiblingItem(hPrevItem);
            }
        }
        else
            hPrevItem=m_pwndPageTree->GetParentItem(hItem);

        if (!hPrevItem)
        {
            // no prev item, so cycle to the last item
            hPrevItem = m_pwndPageTree->GetRootItem();

            for (;;)
            {
                while (m_pwndPageTree->GetNextSiblingItem(hPrevItem))
                    hPrevItem = m_pwndPageTree->GetNextSiblingItem(hPrevItem);

                if (m_pwndPageTree->ItemHasChildren(hPrevItem))
                    hPrevItem = m_pwndPageTree->GetChildItem(hPrevItem);
                else
                    break;
            }
        }

        if (hPrevItem)
            m_pwndPageTree->SelectItem(hPrevItem);
    }
}


void CTreePropSheet::ActivateNextPage()
{
    if (!IsWindow(m_hWnd))
        return;

    if (!IsWindow(m_pwndPageTree->GetSafeHwnd()))
    {
        // normal tab property sheet. Simply use page index
        int	nPageIndex = GetActiveIndex();
        if (nPageIndex<0 || nPageIndex>=GetPageCount())
            return;

        int	nNextIndex = (nPageIndex==GetPageCount()-1)? 0 : nPageIndex+1;
        SetActivePage(nNextIndex);
    }
    else
    {
        // property sheet with page tree.
        // we need a more sophisticated handling here, than simply using
        // the page index, because we won't skip empty pages.
        // so we have to walk the page tree
        HTREEITEM	hItem = m_pwndPageTree->GetSelectedItem();
        ASSERT(hItem);
        if (!hItem)
            return;

        HTREEITEM	hNextItem = NULL;
        if ((hNextItem = m_pwndPageTree->GetChildItem(hItem)) != NULL)
            ;
        else if ((hNextItem = m_pwndPageTree->GetNextSiblingItem(hItem)) != NULL)
            ;
        else if (m_pwndPageTree->GetParentItem(hItem))
        {
            while (!hNextItem)
            {
                hItem = m_pwndPageTree->GetParentItem(hItem);
                if (!hItem)
                    break;

                hNextItem	= m_pwndPageTree->GetNextSiblingItem(hItem);
            }
        }

        if (!hNextItem)
            // no next item -- so cycle to the first item
            hNextItem = m_pwndPageTree->GetRootItem();

        if (hNextItem)
            m_pwndPageTree->SelectItem(hNextItem);
    }
}


/////////////////////////////////////////////////////////////////////
// Overridings

BOOL CTreePropSheet::OnInitDialog()
{
    if (m_bTreeViewMode)
    {

        // be sure, there are no stacked tabs, because otherwise the
        // page caption will be to large in tree view mode
        EnableStackedTabs(FALSE);

        // Initialize image list.
        if (m_DefaultImages.GetSafeHandle())
        {
            IMAGEINFO	ii;
            m_DefaultImages.GetImageInfo(0, &ii);
            m_Images.Create(ii.rcImage.right - ii.rcImage.left, ii.rcImage.bottom - ii.rcImage.top, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
        }
        else
            m_Images.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
    }

    // perform default implementation
    BOOL bResult = CPropertySheet::OnInitDialog();

    if (!m_bTreeViewMode)
        // stop here, if we would like to use tabs
        return bResult;

    // Get tab control...
    CTabCtrl* pTab = GetTabControl();
    if (!IsWindow(pTab->GetSafeHwnd()))
    {
        ASSERT(0);
        return bResult;
    }

    HighColorTab::UpdateImageList(*this);

    // ... and hide it
    pTab->ShowWindow(SW_HIDE);
    pTab->EnableWindow(FALSE);

    // Place another (empty) tab ctrl, to get a frame instead
    CRect rectFrame;
    pTab->GetWindowRect(rectFrame);
    ScreenToClient(rectFrame);

    m_pFrame = CreatePageFrame();
    if (!m_pFrame)
    {
        ASSERT(0);
        AfxThrowMemoryException();
    }
    m_pFrame->Create(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, rectFrame, this, 0xFFFF);
    m_pFrame->ShowCaption(m_bPageCaption);

    // Lets make place for the tree ctrl
    const int nTreeWidth = m_nPageTreeWidth;
    const int nTreeSpace = 5;

    CRect rectSheet;
    GetWindowRect(rectSheet);
    rectSheet.right += nTreeWidth;
    SetWindowPos(NULL, 0, 0, rectSheet.Width(), rectSheet.Height(), SWP_NOZORDER | SWP_NOMOVE);
    CenterWindow();

    MoveChildWindows(nTreeWidth, 0);

    // Lets calculate the rectangle for the tree ctrl
    CRect	rectTree(rectFrame);
    rectTree.right = rectTree.left + nTreeWidth - nTreeSpace;

    // calculate caption height
    CTabCtrl	wndTabCtrl;
    wndTabCtrl.Create(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, rectFrame, this, 0x1234);
    wndTabCtrl.InsertItem(0, L"");
    CRect	rectFrameCaption;
    wndTabCtrl.GetItemRect(0, rectFrameCaption);
    wndTabCtrl.DestroyWindow();
    m_pFrame->SetCaptionHeight(rectFrameCaption.Height());

    // if no caption should be displayed, make the window smaller in
    // height
    if (!m_bPageCaption)
    {
        // make frame smaller
        m_pFrame->GetWnd()->GetWindowRect(rectFrame);
        ScreenToClient(rectFrame);
        rectFrame.top+= rectFrameCaption.Height();
        m_pFrame->GetWnd()->MoveWindow(rectFrame);

        // move all child windows up
        MoveChildWindows(0, -rectFrameCaption.Height());

        // modify rectangle for the tree ctrl
        rectTree.bottom-= rectFrameCaption.Height();

        // make us smaller
        CRect	rect;
        GetWindowRect(rect);
        rect.top+= rectFrameCaption.Height()/2;
        rect.bottom-= rectFrameCaption.Height()-rectFrameCaption.Height()/2;
        if (GetParent())
            GetParent()->ScreenToClient(rect);
        MoveWindow(rect);
        // Need to center window again to reflect the missing caption bar (noticeable on 640x480 resolutions)
        CenterWindow();
    }

    // finally create the tree control
    //const DWORD	dwTreeStyle = TVS_SHOWSELALWAYS/*|TVS_TRACKSELECT*/|TVS_HASLINES/*|TVS_LINESATROOT*/|TVS_HASBUTTONS;
    // As long as we don't use sub pages we apply the 'TVS_FULLROWSELECT' style for a little more user convinience.
    const DWORD	dwTreeStyle = TVS_SHOWSELALWAYS | TVS_FULLROWSELECT;
    m_pwndPageTree = CreatePageTreeObject();
    if (!m_pwndPageTree)
    {
        ASSERT(0);
        AfxThrowMemoryException();
    }

    // MFC7-support here (Thanks to Rainer Wollgarten)
#if _MFC_VER >= 0x0700
    {
        // Using 'CTreeCtrl::CreateEx' (and it's indeed a good idea to call this one), results in
        // flawed window styles (border is missing) when running under WinXP themed.. ???
        //m_pwndPageTree->CreateEx(
        //	WS_EX_CLIENTEDGE|WS_EX_NOPARENTNOTIFY,
        //	WS_TABSTOP|WS_CHILD|WS_VISIBLE|dwTreeStyle,
        //	rectTree, this, s_unPageTreeId);

        // Feel free to explain to me why we need to call CWnd::CreateEx to get the proper window style
        // for the tree view control when running under WinXP. Look at CTreeCtrl::CreateEx and CWnd::CreateEx to
        // see the (minor) difference. However, this could create problems in future MFC versions..
        m_pwndPageTree->CWnd::CreateEx(
            WS_EX_CLIENTEDGE|WS_EX_NOPARENTNOTIFY,
            WC_TREEVIEW, _T("PageTree"),
            WS_TABSTOP|WS_CHILD|WS_VISIBLE|dwTreeStyle,
            rectTree, this, s_unPageTreeId);
    }
#else
    {
        m_pwndPageTree->CreateEx(
            WS_EX_CLIENTEDGE|WS_EX_NOPARENTNOTIFY,
            _T("SysTreeView32"), _T("PageTree"),
            WS_TABSTOP|WS_CHILD|WS_VISIBLE|dwTreeStyle,
            rectTree, this, s_unPageTreeId);
    }
#endif

    // This treeview control was created dynamically, thus it does not derive the font
    // settings from the parent dialog. Need to set the font explicitly so that it fits
    // to the font which is used for the property pages.
    m_pwndPageTree->SendMessage(WM_SETFONT, (WPARAM)AfxGetMainWnd()->GetFont()->m_hObject, TRUE);

    // Win98: Explicitly set to Unicode to receive Unicode notifications.
    m_pwndPageTree->SendMessage(CCM_SETUNICODEFORMAT, TRUE);

    m_pwndPageTree->SetItemHeight(m_pwndPageTree->GetItemHeight() + 6);

    if (m_bTreeImages)
    {
        m_pwndPageTree->SetImageList(&m_Images, TVSIL_NORMAL);
        m_pwndPageTree->SetImageList(&m_Images, TVSIL_STATE);
    }

    // Fill the tree ctrl
    RefillPageTree();

    // Select item for the current page
    if (pTab->GetCurSel() > -1)
        SelectPageTreeItem(pTab->GetCurSel());

    return bResult;
}


void CTreePropSheet::OnDestroy()
{
    CPropertySheet::OnDestroy();

    if (m_Images.GetSafeHandle())
        m_Images.DeleteImageList();

    if (m_pwndPageTree)
    {
        VERIFY(m_pwndPageTree->DestroyWindow());
        delete m_pwndPageTree;
        m_pwndPageTree = NULL;
    }

    if (m_pFrame)
    {
        VERIFY(m_pFrame->GetWnd()->DestroyWindow());
        delete m_pFrame;
        m_pFrame = NULL;
    }
}


LRESULT CTreePropSheet::OnAddPage(WPARAM wParam, LPARAM lParam)
{
    LRESULT	lResult = DefWindowProc(PSM_ADDPAGE, wParam, lParam);
    if (!m_bTreeViewMode)
        return lResult;

    RefillPageTree();
    SelectCurrentPageTreeItem();

    return lResult;
}


LRESULT CTreePropSheet::OnRemovePage(WPARAM wParam, LPARAM lParam)
{
    LRESULT	lResult = DefWindowProc(PSM_REMOVEPAGE, wParam, lParam);
    if (!m_bTreeViewMode)
        return lResult;

    RefillPageTree();
    SelectCurrentPageTreeItem();

    return lResult;
}


LRESULT CTreePropSheet::OnSetCurSel(WPARAM wParam, LPARAM lParam)
{
    LRESULT	lResult = DefWindowProc(PSM_SETCURSEL, wParam, lParam);
    if (!m_bTreeViewMode)
        return lResult;

    SelectCurrentPageTreeItem();
    UpdateCaption();
    return lResult;
}


LRESULT CTreePropSheet::OnSetCurSelId(WPARAM wParam, LPARAM lParam)
{
    LRESULT	lResult = DefWindowProc(PSM_SETCURSEL, wParam, lParam);
    if (!m_bTreeViewMode)
        return lResult;

    SelectCurrentPageTreeItem();
    UpdateCaption();
    return lResult;
}


void CTreePropSheet::OnPageTreeSelChanging(NMHDR *pNotifyStruct, LRESULT *plResult)
{
    *plResult = 0;
    if (m_bPageTreeSelChangedActive)
        return;
    else
        m_bPageTreeSelChangedActive = TRUE;

    NMTREEVIEW	*pTvn = reinterpret_cast<NMTREEVIEW*>(pNotifyStruct);
    int					nPage = m_pwndPageTree->GetItemData(pTvn->itemNew.hItem);
    BOOL				bResult;
    if (nPage<0 || (unsigned)nPage>=m_pwndPageTree->GetCount())
        bResult = KillActiveCurrentPage();
    else
        bResult = SetActivePage(nPage);

    if (!bResult)
        // prevent selection to change
        *plResult = TRUE;

    // Set focus to tree ctrl (I guess that's what the user expects)
    m_pwndPageTree->SetFocus();

    m_bPageTreeSelChangedActive = FALSE;

    return;
}


void CTreePropSheet::OnPageTreeSelChanged(NMHDR* /*pNotifyStruct*/, LRESULT* plResult)
{
    *plResult = 0;

    UpdateCaption();

    return;
}


LRESULT CTreePropSheet::OnIsDialogMessage(WPARAM wParam, LPARAM lParam)
{
    MSG	*pMsg = reinterpret_cast<MSG*>(lParam);
    if (pMsg->message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000))
    {
        // Handle default Windows Common Controls short cuts
        if (pMsg->wParam == VK_TAB)
        {
            if (GetKeyState(VK_SHIFT) & 0x8000)
                ActivatePreviousPage();			// Ctrl+Shift+Tab
            else
                ActivateNextPage();				// Ctrl+Tab
            return TRUE;
        }
        else if (pMsg->wParam == VK_PRIOR)
        {
            ActivatePreviousPage();				// Ctrl+PageUp
            return TRUE;
        }
        else if (pMsg->wParam == VK_NEXT)
        {
            ActivateNextPage();					// Ctrl+PageDown
            return TRUE;
        }
    }

    return CPropertySheet::DefWindowProc(PSM_ISDIALOGMESSAGE, wParam, lParam);
}
