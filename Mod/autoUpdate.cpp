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
#include "autoUpdate.h"
#include "WebHelper.h"
//>>> WiZaRd::kMule
#include "emule.h"
#include "emuleDlg.h"
#include "otherfunctions.h"
#include "Preferences.h"
#include "Log.h"
#include "extractfile.h"
//<<< WiZaRd::kMule

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CAutoUpdate::CAutoUpdate()
{
    m_strUpdateCheckURL = L"";
    m_strCurrentVersion = L"";
    m_strTempPath = L"";
    m_strTempFile = L"";
    m_strUpdateURL = L"";
    m_uiUpdateApplication = UPDATE_NONE;
    m_bDeleteAfterUpdate = true;

    m_pWebHelper = new CWebHelper();
}

CAutoUpdate::~CAutoUpdate()
{
    delete m_pWebHelper;
}

void	CAutoUpdate::SetUpdateParams(const CString& strUpdateCheckURL, const CString& strCurrentVersion, const CString& strTempPath)
{
    m_strUpdateCheckURL = strUpdateCheckURL;
    if (m_strUpdateCheckURL.Right(1) != L"/")
        m_strUpdateCheckURL += L"/";
    m_strCurrentVersion = strCurrentVersion;
    m_strTempPath = strTempPath;
    if (m_strTempPath.Right(1) != L"/")
        m_strTempPath += L"/";
}

void	CAutoUpdate::CheckForUpdates()
{
    const uint8 updateMode = IsUpdateAvail();
    if (updateMode != AUTOUPDATE_NONE)
    {
        CString strMessage = L"";
        //strMessage.Format(L"An update is available!\nDo you want to download and install %s, now?\n\nNOTE: a restart is required!", m_strTempFile);
        strMessage.Format(GetResString(IDS_UPDATE_AVAIL_QUESTION), m_strTempFile); //>>> WiZaRd::kMule
        if (updateMode == AUTOUPDATE_FORCE || AfxMessageBox(strMessage, MB_YESNO|MB_DEFBUTTON1|MB_ICONQUESTION) == IDYES)
        {
            if (m_uiUpdateApplication == UPDATE_DOWNLOADPAGE)
            {
                ShellExecute(NULL, NULL, m_strUpdateURL, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
            }
            else
            {
                const CString strUpdateFile = GetUpdateFile();
                _tremove(strUpdateFile);
                if (m_pWebHelper->LoadToFileFromURL(strUpdateFile, m_strUpdateURL))
                {
                    ASSERT(::PathFileExists(strUpdateFile));   // downloaded successfully?

                    // finally, restart your application here!
//>>> WiZaRd::kMule
                    if (AfxMessageBox(GetResString(IDS_RESTART_NOW_QUESTION), MB_YESNO|MB_DEFBUTTON1|MB_ICONQUESTION) == IDYES)
                    {
                        theApp.m_app_state = APP_STATE_SHUTTINGDOWN; // prevent ask for exit
                        SendMessage(theApp.emuledlg->m_hWnd, WM_CLOSE, 0, 0);
                    }
//<<< WiZaRd::kMule
                }
            }
        }
        else
            m_uiUpdateApplication = UPDATE_NONE;
    }
}

bool	CAutoUpdate::ApplyUpdate() const
{
    switch (m_uiUpdateApplication)
    {
    case UPDATE_NONE:
    case UPDATE_DOWNLOADPAGE:
    default:
        return false;

    case UPDATE_EXECUTE:
    {
        const CString strUpdateFile = GetUpdateFile();
        if (::PathFileExists(strUpdateFile))
        {
            ShellExecute(NULL, L"open", strUpdateFile, NULL, NULL, SW_SHOWDEFAULT);
            if (m_bDeleteAfterUpdate)
                _tremove(strUpdateFile);
            return true;
        }
        return false;
    }

    case UPDATE_REPLACE:
    case UPDATE_EXTRACT: //>>> WiZaRd::kMule
    {
        TCHAR tchBuffer[MAX_PATH];
        ::GetModuleFileName(NULL, tchBuffer, _countof(tchBuffer));
        tchBuffer[_countof(tchBuffer) - 1] = L'\0';
        CString strExePath = tchBuffer;

        CString strBak = L"";
        strBak.Format(L"%s_%s.bak", strExePath, MOD_VERSION_BUILD);
        _tremove(strBak);
        const CString strUpdateFile = GetUpdateFile();

        // create a backup in case something goes wrong
        if (_trename(strExePath, strBak) == 0)
        {
            if (m_uiUpdateApplication == UPDATE_REPLACE)
            {
                if ((m_bDeleteAfterUpdate && ::MoveFileEx(strUpdateFile, strExePath, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH))
                        || ::CopyFile(strUpdateFile, strExePath, TRUE))
                    return true; //update done ;)
            }
//>>> WiZaRd::kMule
            else
            {
                if (Extract(strUpdateFile, strExePath, L"kMule.exe", m_bDeleteAfterUpdate, 1, L"kMule.exe"))
                    return true;
            }
//<<< WiZaRd::kMule
        }

        //update failed!?
        ASSERT(0);
        //restore the old file
        _tremove(strExePath);
        _trename(strBak, strExePath);
        return false;
    }
    }
}

uint8	CAutoUpdate::IsUpdateAvail()
{
    uint8 ret = AUTOUPDATE_NONE;

    CString strUpdateCheckURL = L"";
    strUpdateCheckURL.Format(L"%s?version=%s", m_strUpdateCheckURL, m_strCurrentVersion);
    CString response = L"";
    if (m_pWebHelper->LoadToStringFromURL(response, strUpdateCheckURL) && !response.IsEmpty())
    {
        thePrefs.UpdateLastVC();

        int curPos = 0;
        CString strTok = response.Tokenize(L"\r\n", curPos);
        int count = 0;
        while (!strTok.IsEmpty())
        {
            strTok.Trim();
            switch (count)
            {
            case 0:
            {
                m_strUpdateURL = strTok;
                int pos = m_strUpdateURL.ReverseFind(L'/');
                if (pos != -1)
                    m_strTempFile = m_strUpdateURL.Mid(pos+1);
                else
                    m_strTempFile = L"update.exe";
                break;
            }

            case 1:
                ret = (uint8)_tstoi(strTok);
                break;

            case 2:
                m_uiUpdateApplication = (uint8)_tstoi(strTok);
                break;

            case 3:
                m_bDeleteAfterUpdate = _tstoi(strTok) != 0;
                break;
            }

            strTok = response.Tokenize(L"\r\n", curPos);
            ++count;
        }

//>>> WiZaRd::kMule
        if (ret == AUTOUPDATE_NONE)
            AddLogLine(true,GetResString(IDS_NONEWERVERSION));
        else
        {
#ifndef _BETA
            Log(LOG_SUCCESS|LOG_STATUSBAR, GetResString(IDS_NEWVERSIONAVL));
            theApp.emuledlg->ShowNotifier(GetResString(IDS_NEWVERSIONAVL), TBN_NEWVERSION);
#else
            Log(LOG_SUCCESS|LOG_STATUSBAR, GetResString(IDS_NEWVERSIONAVLBETA));
            theApp.emuledlg->ShowNotifier(GetResString(IDS_NEWVERSIONAVL), TBN_NEWVERSION);
            if (AfxMessageBox(GetResString(IDS_NEWVERSIONAVLBETA) + GetResString(IDS_VISITVERSIONCHECK), MB_OK) == IDOK)
            {
                CString theUrl = thePrefs.GetVersionCheckBaseURL() + L"/beta";
                ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
            }
#endif
        }
//<<< WiZaRd::kMule
    }
//>>> WiZaRd::kMule
    else
        AddLogLine(true, GetResString(IDS_NEWVERSIONFAILED));
//<<< WiZaRd::kMule

    return ret;
}

CString CAutoUpdate::GetUpdateFile() const
{
    return m_strTempPath + m_strTempFile;
}