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

//////////////////////////////////////////////////////////////////////////
// Helper function
//
// Cracks a given URL into main and sub page parts
void	CrackURL(CString sURL, CString& mainpage, CString& subpage)
{
    sURL.Trim(); // just to be sure

    // search for http:// to get the base URL
    int startpos = sURL.Find(L"http://");
    if(startpos != -1) //if we found it, cap the string
    {
        startpos += 7;
        mainpage = sURL.Mid(startpos);
    }
    else
        mainpage = sURL;

    // search for the first '/' - if none found we already have the URL
    // the subpage then is everything after and including the '/'
    startpos = mainpage.Find(L"/");
    if(startpos == -1)
        return;

    const CString URL = CString(mainpage);

    subpage = URL.Mid(startpos);
    mainpage = URL.Mid(0, startpos);
}
//
//////////////////////////////////////////////////////////////////////////


CAutoUpdate::CAutoUpdate()
{
    m_strUpdateCheckURL = L"";
    m_strCurrentVersion = L"";
    m_strTempPath = L"";
    m_strTempFile = L"";
    m_strUpdateURL = L"";
    m_uiUpdateApplication = UPDATE_NONE;
    m_bDeleteAfterUpdate = true;
    m_strUserAgent = DFLT_AGENT_NAME;

    m_hINet = NULL;
    m_hConnection = NULL;
    m_hData = NULL;
}

CAutoUpdate::~CAutoUpdate()
{
    //close handles...
    InternetCloseHandle(m_hData);
    InternetCloseHandle(m_hConnection);
    InternetCloseHandle(m_hINet);
}

void	CAutoUpdate::SetUpdateParams(const CString& strUpdateCheckURL, const CString& strCurrentVersion, const CString& strTempPath)
{
    m_strUpdateCheckURL = strUpdateCheckURL;
    if(m_strUpdateCheckURL.Right(1) != L"/")
        m_strUpdateCheckURL += L"/";
    m_strCurrentVersion = strCurrentVersion;
    m_strTempPath = strTempPath;
    if(m_strTempPath.Right(1) != L"/")
        m_strTempPath += L"/";
}

void	CAutoUpdate::SetUserAgent(const CString& strUserAgent)
{
    m_strUserAgent = strUserAgent;
}

void	CAutoUpdate::CheckForUpdates()
{
    const uint8 updateMode = IsUpdateAvail();
    if(updateMode != AUTOUPDATE_NONE)
    {
        CString strMessage = L"";
        //strMessage.Format(L"An update is available!\nDo you want to download and install %s, now?\n\nNOTE: a restart is required!", m_strTempFile);
        strMessage.Format(GetResString(IDS_UPDATE_AVAIL_QUESTION), m_strTempFile); //>>> WiZaRd::kMule
        if(updateMode == AUTOUPDATE_FORCE || AfxMessageBox(strMessage, MB_YESNO|MB_DEFBUTTON1|MB_ICONQUESTION) == IDYES)
        {
            const CString strUpdateFile = GetUpdateFile();
            _tremove(strUpdateFile);
            if(LoadToFileFromURL(strUpdateFile, m_strUpdateURL))
            {
                ASSERT( ::PathFileExists(strUpdateFile) ); // downloaded successfully?

                // finally, restart your application here!
//>>> WiZaRd::kMule
				if(AfxMessageBox(GetResString(IDS_RESTART_NOW_QUESTION), MB_YESNO|MB_DEFBUTTON1|MB_ICONQUESTION) == IDYES)
				{
					theApp.m_app_state = APP_STATE_SHUTTINGDOWN; // prevent ask for exit
					SendMessage(theApp.emuledlg->m_hWnd, WM_CLOSE, 0, 0);
				}
//<<< WiZaRd::kMule
            }
        }
        else
            m_uiUpdateApplication = AUTOUPDATE_NONE;
    }
}

bool	CAutoUpdate::ApplyUpdate() const
{
    switch(m_uiUpdateApplication)
    {
		case UPDATE_NONE:
		default:
			return false;

		case UPDATE_EXECUTE:
		{
			const CString strUpdateFile = GetUpdateFile();
			if(::PathFileExists(strUpdateFile))
			{
				ShellExecute(NULL, L"open", strUpdateFile, NULL, NULL, SW_SHOWDEFAULT);
				if(m_bDeleteAfterUpdate)
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
			if(_trename(strExePath, strBak) == 0)
			{
				if(m_uiUpdateApplication == UPDATE_REPLACE)
				{
					if((m_bDeleteAfterUpdate && ::MoveFileEx(strUpdateFile, strExePath, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH))
							|| ::CopyFile(strUpdateFile, strExePath, TRUE))
						return true; //update done ;)
				}
//>>> WiZaRd::kMule
				else
				{
					if(Extract(strUpdateFile, strExePath, L"kMule.exe", m_bDeleteAfterUpdate, 1, L"kMule.exe"))
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
    if(LoadToStringFromURL(response, strUpdateCheckURL) && !response.IsEmpty())
    {
        thePrefs.UpdateLastVC();

        int curPos = 0;
        CString strTok = response.Tokenize(L"\r\n", curPos);
        int count = 0;
        while(!strTok.IsEmpty())
        {
            strTok.Trim();
            switch(count)
            {
				case 0:
				{
					m_strUpdateURL = strTok;
					int pos = m_strUpdateURL.ReverseFind(L'/');
					if(pos != -1)
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
        if(ret == AUTOUPDATE_NONE)
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

void CAutoUpdate::OpenAndCheckConnection()
{
    if(m_hINet == NULL)
        m_hINet = InternetOpen(m_strUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);

    if(!m_hINet)
        throw GetLastError();

    // check for existing internet connection - this is *NOT* reliable!
    DWORD dwType = 0;
    if (!InternetGetConnectedState(&dwType, 0)
            || dwType == INTERNET_CONNECTION_OFFLINE)
        throw L"No internet connection available!";
}

void	CAutoUpdate::EstablishConnection(const CString& sURL, const CString& sUser, const CString& sPass)
{
    InternetCloseHandle(m_hData);
    InternetCloseHandle(m_hConnection);

    //get current page
    CString mainpage, subpage;
    CrackURL(sURL, mainpage, subpage);

    if(!sUser.IsEmpty() && !sPass.IsEmpty())
        m_hConnection = InternetConnect(m_hINet, mainpage, INTERNET_DEFAULT_HTTP_PORT, sUser, sPass, INTERNET_SERVICE_HTTP, NULL, 0);
    else
        m_hConnection = InternetConnect(m_hINet, mainpage, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, NULL, 0);
    if(!m_hConnection)
        throw GetLastError();

    m_hData = HttpOpenRequest(m_hConnection, L"GET", subpage, 0, 0, 0, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    if(!m_hData)
        throw GetLastError();

    if(!HttpSendRequest(m_hData, 0, 0, 0, 0))
        throw GetLastError();
}

CString CAutoUpdate::GetUpdateFile() const
{
    return m_strTempPath + m_strTempFile;
}

// Downloads a file from the given sURL to path
bool	CAutoUpdate::LoadToStringFromURL(CString& str, const CString& sURL, const CString& sUser, const CString& sPass)
{
    bool ret = true;

    try
    {
        OpenAndCheckConnection();
        EstablishConnection(sURL, sUser, sPass);

        DWORD dwBytesAvail = 0;
        while (InternetQueryDataAvailable(m_hData, &dwBytesAvail, 0, 0))
        {
            BYTE* pBuffer = new BYTE[dwBytesAvail+1];
            memset(pBuffer, 0, dwBytesAvail+1);
            DWORD dwBytesRead = 0;
            if(!InternetReadFile(m_hData, pBuffer, dwBytesAvail, &dwBytesRead))
            {
                delete[] pBuffer;
                throw GetLastError();
            }
            if (dwBytesRead == 0)
            {
                delete[] pBuffer;
                break;	//eof
            }

            pBuffer[dwBytesRead] = '\0';
            str.AppendFormat(L"%hs", pBuffer);
            delete[] pBuffer;
        }
    }
    catch(DWORD /*error*/)
    {
        ret = false;
    }
    catch(...)
    {
        ret = false;
    }

    return ret;
}

bool	CAutoUpdate::LoadToFileFromURL(const CString& path, const CString& sURL, const CString& sUser, const CString& sPass)
{
    bool ret = true;

    try
    {
        OpenAndCheckConnection();
        EstablishConnection(sURL, sUser, sPass);

        CFileException fexp;
        CStdioFile file;
        if (!file.Open(path, CFile::modeWrite|CFile::modeCreate|CFile::osSequentialScan|CFile::shareDenyWrite|CFile::typeBinary, &fexp))
            throw GetLastError();
        setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

        DWORD dwBytesAvail = 0;
        while (InternetQueryDataAvailable(m_hData, &dwBytesAvail, 0, 0))
        {
            BYTE* pBuffer = new BYTE[dwBytesAvail+1];
            memset(pBuffer, 0, dwBytesAvail+1);
            DWORD dwBytesRead = 0;
            if(!InternetReadFile(m_hData, pBuffer, dwBytesAvail, &dwBytesRead))
            {
                delete[] pBuffer;
                throw GetLastError();
            }
            if (dwBytesRead == 0)
            {
                delete[] pBuffer;
                break;	//eof
            }

            file.Write((void*)pBuffer, dwBytesRead);
            delete[] pBuffer;
        }

        file.Close();
    }
    catch(DWORD /*error*/)
    {
        ret = false;
    }
    catch(...)
    {
        ret = false;
    }

    return ret;
}