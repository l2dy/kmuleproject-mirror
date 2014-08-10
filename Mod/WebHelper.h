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

class CWebHelper
{
  public:
    CWebHelper();
    virtual ~CWebHelper();

    void	SetUserAgent(const CString& strUserAgent);

    bool	LoadToStringFromURL(CString& str, const CString& sURL, const CString& sUser = L"", const CString& sPass = L"");
    bool	LoadToFileFromURL(const CString& path, const CString& sURL, const CString& sUser = L"", const CString& sPass = L"");
    static void	CrackURL(CString sURL, CString& mainpage, CString& subpage);

  private:
    CString m_strUserAgent;

    HINTERNET m_hINet;
    HINTERNET m_hConnection;
    HINTERNET m_hData;

    void	OpenAndCheckConnection();
    void	EstablishConnection(const CString& sURL, const CString& sUser = L"", const CString& sPass = L"");
};