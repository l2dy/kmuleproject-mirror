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
#include "stdafx.h"
#include "PPgAbout.h"
#include "emule.h"
#include "PreferencesDlg.h"
#include "HelpIDs.h"
#include "HTRichEditCtrl.h"
#include "./Mod/NetF/DesktopIntegration.h" //>>> WiZaRd::DesktopIntegration [Netfinity]

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgAbout, CPropertyPage)
BEGIN_MESSAGE_MAP(CPPgAbout, CPropertyPage)
    ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_DONATION_TUX, OnBnClickedDonateTux)
	ON_BN_CLICKED(IDC_DONATION_WIZARD, OnBnClickedDonateWiZaRd)
END_MESSAGE_MAP()

CPPgAbout::CPPgAbout()
    : CPropertyPage(CPPgAbout::IDD)
{
	aboutInfo = new CHTRichEditCtrl;
	VERIFY( m_AboutFont.CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Courier New") );
}

CPPgAbout::~CPPgAbout()
{
	m_AboutFont.DeleteObject();
	delete aboutInfo;
}

void CPPgAbout::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
}

CString getCenteredString(const CString str)
{
	CString ret = L"";

#define STR_LIMIT	45 // max 50 chars?
	int nTokenPos = 0;
	CString strToken = str.Tokenize(L" ", nTokenPos);	
	CString line = L"";
	while(!strToken.IsEmpty())
	{
		if(line.GetLength() + strToken.GetLength() < STR_LIMIT)
		{
			if(!line.IsEmpty())
				line.Append(L" ");
			line.Append(strToken);
		}
		else
		{
			// one line complete, buffer and output
			if(!ret.IsEmpty())
				ret.Append(L"\n");
			UINT maxchars = UINT((STR_LIMIT - line.GetLength()) / 2);
			CString buffer(L' ', maxchars);
			ret.AppendFormat(L"%s%s%s", buffer, line, buffer);

			line = L"";
		}
		strToken = str.Tokenize(L" ", nTokenPos);
	}

	// one line complete, buffer and output
	if(!ret.IsEmpty())
		ret.Append(L"\n");
	UINT maxchars = UINT((STR_LIMIT - line.GetLength()) / 2);
	CString buffer(L' ', maxchars);
	ret.AppendFormat(L"%s%s%s", buffer, line, buffer);

	/*if(str.GetLength() < STR_LIMIT)
	{	
		UINT maxchars = UINT((STR_LIMIT - str.GetLength()) / 2);
		CString buffer(L' ', maxchars);
		ret = buffer + str + buffer;
	}*/
#undef STR_LIMIT

	return ret;
}

BOOL CPPgAbout::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    InitWindowStyles(this);

	CRect rect;
	GetDlgItem(IDC_ABOUTINFOBOX)->GetWindowRect(rect);
	GetDlgItem(IDC_ABOUTINFOBOX)->DestroyWindow();
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	if (aboutInfo->Create((WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL), rect, this, IDC_ABOUTINFOBOX))
	{
		aboutInfo->SetProfileSkinKey(L"Log");
		aboutInfo->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		aboutInfo->SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		/*if (theApp.m_fontLog.m_hObject)
			aboutInfo->SetFont(&theApp.m_fontLog);*/
		aboutInfo->SetFont(&m_AboutFont);
		aboutInfo->ApplySkin();
		//aboutInfo->SetTitle(GetResString(IDS_SV_LOG));
		aboutInfo->SetAutoURLDetect(TRUE);
	}
	//AddAnchor(*aboutInfo, TOP_LEFT, BOTTOM_RIGHT);
	aboutInfo->ShowWindow(SW_SHOW);

	// Create credits string - hardcoded! - taken from CreditsThread.cpp!
	CString credits = L"";

	// header
	credits.Format(L"Thanks for using %s!", MOD_VERSION_PLAIN);
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), CFM_BOLD|CFM_ITALIC);
	credits.Format(L"We hope you find it useful!");
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);
	credits.Format(L"We thrive on user feedback so if there's something it does that you don't like, or that doesn't seem to work correctly or as you'd expect visit %s for support.", CString(MOD_HOMEPAGE));
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);
	credits.Format(L"You can also use the forum to request new features from the developers - we're a very friendly bunch and are always keen to hear what people want.");
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);
	credits.Format(L"Lastly, although %s will notify you when an update is available (assuming you're happy for it to do so), you can also keep up-to-date on the next release (and development work in general) by following the forums.", MOD_VERSION_PLAIN);
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);

	// content
// 	credits.Format(L"%s Build%s Team", MOD_VERSION_PLAIN, GetModVersionNumber());
// 	aboutInfo->AddLine(L"\n" + getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), CFM_BOLD);
// 	credits.Format(L"For %s", CString(MOD_HOMEPAGE));
// 	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);

	credits.Format(L"Modders");
	aboutInfo->AddLine(L"\n" + getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), CFM_BOLD);
	credits.Format(L"tuxman");
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);
	credits.Format(L"WiZaRd");
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);

// 	credits.Format(L"\nGrafix by\n");
// 	aboutInfo->AddLine(credits, -1, false, RGB(0, 0, 0), RGB(255, 255, 255), CFM_BOLD);
// 	credits.Format(L"???\n");
// 	aboutInfo->AddLine(credits, -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);

// 	credits.Format(L"\nTranslated by\n");
// 	aboutInfo->AddLine(credits, -1, false, RGB(0, 0, 0), RGB(255, 255, 255), CFM_BOLD);
// 	credits.Format(L"Some language: ???\n");
// 	aboutInfo->AddLine(credits, -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);

// 	credits.Format(L"\nTested by\n");
// 	aboutInfo->AddLine(credits, -1, false, RGB(0, 0, 0), RGB(255, 255, 255), CFM_BOLD);
// 	credits.Format(L"some tester\n");
// 	aboutInfo->AddLine(credits, -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);

	credits.Format(L"%s is based on eMule", MOD_VERSION_PLAIN);
	aboutInfo->AddLine(L"\n\n" + getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), CFM_BOLD);

	// footer
	credits.Format(L"\n_____________________________________________\n");	
	aboutInfo->AddLine(credits, -1, false, RGB(0, 0, 0), RGB(255, 255, 255), CFM_BOLD);
	credits.Format(L"Copyright (C) 2012-2013 tuxman/WiZaRd");
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);
	credits.Format(L"%s is licensed under the GPL", MOD_VERSION_PLAIN);
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);
	credits.Format(L"Source code is available (via SVN) from SourceForge (Project \"kMuleProject\")");
	aboutInfo->AddLine(getCenteredString(credits) + L"\n", -1, false, RGB(0, 0, 0), RGB(255, 255, 255), 0);
//<<< WiZaRd::ModCredits

    Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgAbout::OnApply()
{
    SetModified(FALSE);
    return CPropertyPage::OnApply();
}

BOOL CPPgAbout::OnSetActive()
{
    return __super::OnSetActive();
}

void CPPgAbout::Localize(void)
{
    if (m_hWnd)
    {
        SetWindowText(GetResString(IDS_ABOUTBOX));
		GetDlgItem(IDC_DONATION_FRM)->SetWindowText(GetResString(IDS_DONATION_LABEL));
		GetDlgItem(IDC_DONATION_DESC)->SetWindowText(GetResString(IDS_DONATION_DESC));
		GetDlgItem(IDC_DONATION_TUX)->SetWindowText(GetResString(IDS_DONATION_TUX));
		GetDlgItem(IDC_DONATION_WIZARD)->SetWindowText(GetResString(IDS_DONATION_WIZARD));
    }
}


void CPPgAbout::OnHelp()
{
    theApp.ShowHelp(eMule_FAQ_Preferences_About);
}

BOOL CPPgAbout::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (wParam == ID_HELP)
    {
        OnHelp();
        return TRUE;
    }
    return __super::OnCommand(wParam, lParam);
}

BOOL CPPgAbout::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    OnHelp();
    return TRUE;
}

void CPPgAbout::OnBnClickedDonateTux()
{
	CString url = L"http://www.amazon.de/registry/wishlist/N16BDCERSOZK?tag=kMule-21";
	_ShellExecute(NULL, NULL, url, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
}

void CPPgAbout::OnBnClickedDonateWiZaRd()
{
	CString url = L"http://www.amazon.de/registry/wishlist/FNE7OMIHR5AC?tag=kMule-21";
	_ShellExecute(NULL, NULL, url, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
}