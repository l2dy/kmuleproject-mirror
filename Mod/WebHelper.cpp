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
#include "WebHelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// Constructor
CWebHelper::CWebHelper()
{
    m_strUserAgent = DFLT_AGENT_NAME;

    m_hINet = NULL;
    m_hConnection = NULL;
    m_hData = NULL;
}

// Destructor
CWebHelper::~CWebHelper()
{
    //close handles...
    InternetCloseHandle(m_hData);
    InternetCloseHandle(m_hConnection);
    InternetCloseHandle(m_hINet);
}

// Sets the User Agent
void	CWebHelper::SetUserAgent(const CString& strUserAgent)
{
    m_strUserAgent = strUserAgent;
}

// Checks and/or opens the internet connection
void CWebHelper::OpenAndCheckConnection()
{
    if (m_hINet == NULL)
        m_hINet = InternetOpen(m_strUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);

    if (!m_hINet)
        throw GetLastError();

    // check for existing internet connection - this is *NOT* reliable!
    DWORD dwType = 0;
    if (!InternetGetConnectedState(&dwType, 0)
            || dwType == INTERNET_CONNECTION_OFFLINE)
        throw L"No internet connection available!";
}

// Establishes the connection to the given URL
void	CWebHelper::EstablishConnection(const CString& sURL, const CString& sUser, const CString& sPass)
{
    InternetCloseHandle(m_hData);
    InternetCloseHandle(m_hConnection);

    //get current page
    CString mainpage, subpage;
    CrackURL(sURL, mainpage, subpage);

    if (!sUser.IsEmpty() && !sPass.IsEmpty())
        m_hConnection = InternetConnect(m_hINet, mainpage, INTERNET_DEFAULT_HTTP_PORT, sUser, sPass, INTERNET_SERVICE_HTTP, NULL, 0);
    else
        m_hConnection = InternetConnect(m_hINet, mainpage, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, NULL, 0);
    if (!m_hConnection)
        throw GetLastError();

    m_hData = HttpOpenRequest(m_hConnection, L"GET", subpage, 0, 0, 0, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    if (!m_hData)
        throw GetLastError();

    if (!HttpSendRequest(m_hData, 0, 0, 0, 0))
        throw GetLastError();
}

// Downloads a file from the given sURL to path
bool	CWebHelper::LoadToStringFromURL(CString& str, const CString& sURL, const CString& sUser, const CString& sPass)
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
            if (!InternetReadFile(m_hData, pBuffer, dwBytesAvail, &dwBytesRead))
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
    catch (DWORD /*error*/)
    {
        ret = false;
    }
    catch (...)
    {
        ret = false;
    }

    return ret;
}

// Downloads a file from the given sURL to the given path
bool	CWebHelper::LoadToFileFromURL(const CString& path, const CString& sURL, const CString& sUser, const CString& sPass)
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
            if (!InternetReadFile(m_hData, pBuffer, dwBytesAvail, &dwBytesRead))
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
    catch (DWORD /*error*/)
    {
        ret = false;
    }
    catch (...)
    {
        ret = false;
    }

    return ret;
}

// Cracks a given URL into main and sub page parts
void CWebHelper::CrackURL(CString sURL, CString& mainpage, CString& subpage)
{
    sURL.Trim(); // just to be sure

    // search for http:// to get the base URL
    int startpos = sURL.Find(L"http://");
    if (startpos != -1) //if we found it, cap the string
    {
        startpos += 7;
        mainpage = sURL.Mid(startpos);
    }
    else
        mainpage = sURL;

    // search for the first '/' - if none found we already have the URL
    // the subpage then is everything after and including the '/'
    startpos = mainpage.Find(L"/");
    if (startpos == -1)
        return;

    const CString URL = CString(mainpage);

    subpage = URL.Mid(startpos);
    mainpage = URL.Mid(0, startpos);
}