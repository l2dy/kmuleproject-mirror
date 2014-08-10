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
class CWebHelper;

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
//>>> WiZaRd::kMule
    UPDATE_EXTRACT,
    UPDATE_DOWNLOADPAGE,
//<<< WiZaRd::kMule
};

class CAutoUpdate
{
  public:
    CAutoUpdate();
    virtual ~CAutoUpdate();

    void	SetUpdateParams(const CString& strUpdateCheckURL, const CString& strCurrentVersion, const CString& strTempPath);
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

  protected:
    CString GetUpdateFile() const;
    CWebHelper* m_pWebHelper;
};