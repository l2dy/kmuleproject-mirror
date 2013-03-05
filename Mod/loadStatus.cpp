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
#include "loadStatus.h"
#include "emule.h"
#include "emuleDlg.h"
#include "MuleStatusBarCtrl.h"

CLoadStatus::CLoadStatus()
{
	m_uLineCount = 0;
	m_uLastPercent = 0;
	m_uPercent =  0;
	m_dPercent = 0;
	m_strLoadString = L"";
	m_bLoadMode = true;
}

CLoadStatus::~CLoadStatus()
{
}

void	CLoadStatus::SetLoadMode(const bool bLoading)
{
	m_bLoadMode = bLoading;
}

void	CLoadStatus::SetLoadString(const CString& strLoadString)
{
	m_strLoadString = strLoadString;
}

void	CLoadStatus::SetLineCount(const UINT iCount)
{
	m_uLineCount = iCount;
}

void	CLoadStatus::ReadLineCount(LPCTSTR pszFilePath)
{
	CStdioFile countFile;
	if (countFile.Open(pszFilePath, CFile::modeRead))
	{
		CString strBuffer = L"";
		while (countFile.ReadString(strBuffer))
			++m_uLineCount;
		countFile.Close();
	}
}

void	CLoadStatus::UpdateCount(const UINT iLine)
{	
	if (m_uLineCount)
	{		
		m_dPercent = (double)m_uLineCount != 0.0 ? (double)(iLine)/(double)m_uLineCount*100.0 : 0.0;
		m_uPercent = (UINT)m_dPercent;
		if (m_uPercent != m_uLastPercent)
		{
			CString strPercent = L"";
			strPercent.Format(L"%s %s: %.2f%%...", m_bLoadMode ? L"Loading" : L"Unloading", m_strLoadString, m_dPercent);
			theApp.SetSplashText(strPercent);
			if (theApp.emuledlg && theApp.emuledlg->statusbar->m_hWnd)
				theApp.emuledlg->statusbar->SetText(strPercent, SBarLog, 0);
			m_uLastPercent = m_uPercent;
		}
	}
}

void	CLoadStatus::Complete()
{
	UpdateCount(m_uLineCount);
}