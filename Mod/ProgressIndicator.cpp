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
#include "ProgressIndicator.h"
#include "emule.h"
#include "emuleDlg.h"
#include "MuleStatusBarCtrl.h"
#include "otherfunctions.h"
#include "Log.h"

CProgressIndicator::CProgressIndicator()
{
    m_strFilePath = L"";
    m_uLastProgress = 0;
    m_uProgress = 0;
    m_dProgress = 0;
}

void CProgressIndicator::Initialise(const CString& strFilePath, const CString& strProgressString, const CString& strFinishString)
{
    m_strFilePath = strFilePath;
    m_strProgressString = strProgressString;
    m_strFinishString = strFinishString;

    if (theApp.emuledlg && theApp.emuledlg->IsRunning())
    {
        CString strBuffer = L"";
        strBuffer.Format(m_strProgressString, m_dProgress, m_strFilePath);
        theApp.emuledlg->statusbar->SetText(strBuffer, SBarLog, 0);
    }
}

CProgressIndicator::~CProgressIndicator()
{
}

void CProgressIndicator::UpdateProgress(const double& progress)
{
    m_dProgress = progress;
    m_uProgress = (UINT)m_dProgress;

    if (m_uProgress != m_uLastProgress)
    {
        m_uLastProgress = m_uProgress;
        if (theApp.emuledlg && theApp.emuledlg->IsRunning())
        {
            CString strBuffer = L"";
            if (m_uProgress < 100)
            {
                strBuffer.Format(m_strProgressString, m_dProgress, m_strFilePath);
                theApp.emuledlg->statusbar->SetText(strBuffer, SBarLog, 0);
            }
            else
            {
                strBuffer = m_strFinishString + L" " + m_strFilePath;
                theApp.emuledlg->statusbar->SetText(strBuffer, SBarLog, 0);
                AddLogLine(false, strBuffer);
            }
        }
    }
}