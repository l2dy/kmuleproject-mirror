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
#include <WinInet.h>

#define DFLT_AGENT_NAME			L"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.0; SLCC1)"

typedef unsigned char		uint8;
enum eUpdateMode
{
    AUTOUPDATE_NONE = 0,
    AUTOUPDATE_AVAIL,
    AUTOUPDATE_FORCE
};

enum eUpdateApplication
{
    UPDATE_NONE = 0,
    UPDATE_EXECUTE,
    UPDATE_REPLACE,
	UPDATE_EXTRACT,	//>>> WiZaRd::kMule
};

class CAutoUpdate
{
public:
    CAutoUpdate();
    virtual ~CAutoUpdate();

    void	SetUpdateParams(const CString& strUpdateCheckURL, const CString& strCurrentVersion, const CString& strTempPath);
    void	SetUserAgent(const CString& strUserAgent);
    void	CheckForUpdates();
    bool	ApplyUpdate() const;

private:
    uint8	IsUpdateAvail();

    CString m_strUpdateCheckURL;
    CString m_strCurrentVersion;
    CString m_strTempPath;
    CString m_strTempFile;
    CString m_strUpdateURL;
    uint8	m_uiUpdateApplication;
    bool	m_bDeleteAfterUpdate;
    CString m_strUserAgent;

protected:
    CString GetUpdateFile() const;
    bool	LoadToStringFromURL(CString& str, const CString& sURL, const CString& sUser = L"", const CString& sPass = L"");
    bool	LoadToFileFromURL(const CString& path, const CString& sURL, const CString& sUser = L"", const CString& sPass = L"");
    void	OpenAndCheckConnection();
    void	EstablishConnection(const CString& sURL, const CString& sUser = L"", const CString& sPass = L"");

    HINTERNET m_hINet;
    HINTERNET m_hConnection;
    HINTERNET m_hData;
};