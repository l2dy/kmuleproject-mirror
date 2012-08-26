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
#include "stdafx.h"
#include "emule.h"
#include "MuleStatusBarCtrl.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "StatisticsDlg.h"
#include "ChatWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CMuleStatusBarCtrl

IMPLEMENT_DYNAMIC(CMuleStatusBarCtrl, CStatusBarCtrl)

BEGIN_MESSAGE_MAP(CMuleStatusBarCtrl, CStatusBarCtrl)
    ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

CMuleStatusBarCtrl::CMuleStatusBarCtrl()
{
}

CMuleStatusBarCtrl::~CMuleStatusBarCtrl()
{
}

void CMuleStatusBarCtrl::Init(void)
{
}

void CMuleStatusBarCtrl::OnLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
    int iPane = GetPaneAtPosition(point);
    switch (iPane)
    {
    case SBarLog:
        if(thePrefs.GetLog2Disk())
            ShellOpenFile(thePrefs.GetMuleDirectory(EMULE_LOGDIR) + L"eMule.log");
        if(thePrefs.GetDebug2Disk())
            ShellOpenFile(thePrefs.GetMuleDirectory(EMULE_LOGDIR) + L"eMule_Verbose.log");
        break;

    case SBarUsers:
        ShowNetworkInfo();
        break;

    case SBarUpDown:
        theApp.emuledlg->SetActiveDialog(theApp.emuledlg->statisticswnd);
        break;

    case SBarConnected:
        ShowNetworkInfo();
        break;

    case SBarChatMsg:
        theApp.emuledlg->SetActiveDialog(theApp.emuledlg->chatwnd);
        break;
    }
}

int CMuleStatusBarCtrl::GetPaneAtPosition(CPoint& point) const
{
    CRect rect;
    int nParts = GetParts(0, NULL);
    for (int i = 0; i < nParts; i++)
    {
        GetRect(i, rect);
        if (rect.PtInRect(point))
            return i;
    }
    return -1;
}