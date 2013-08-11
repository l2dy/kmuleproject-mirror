//this file is part of kMule
//Copyright (C)2012 WiZaRd ( strEmail.Format("%s@%s", "thewizardofdos", "gmail.com") / http://www.emulefuture.de )
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
#include "resourcemod.h"
#include "HyperTextCtrl.h"

class CHTRichEditCtrl;
class CPPgAbout : public CPropertyPage
{
    DECLARE_DYNAMIC(CPPgAbout)

public:
    CPPgAbout();
    virtual ~CPPgAbout();

// Dialog Data
    enum { IDD = IDD_PPG_ABOUT };

    void Localize();

private:
	CHTRichEditCtrl* aboutInfo;
	CFont	m_AboutFont;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    virtual BOOL OnSetActive();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnSettingsChange()				{SetModified();}
	afx_msg void OnBnClickedDonateTux();
	afx_msg void OnBnClickedDonateWiZaRd();
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};
