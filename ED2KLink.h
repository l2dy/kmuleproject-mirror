//this file is part of eMule
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
#include "shahashset.h"
class CSafeMemFile;

struct SUnresolvedHostname
{
    SUnresolvedHostname()
    {
        nPort = 0;
    }
    CStringA strHostname;
    uint16 nPort;
    CString strURL;
};


class CED2KLink
{
  public:
    static CED2KLink* CreateLinkFromUrl(const TCHAR* url);
    virtual ~CED2KLink();

    typedef enum { kFile , kNodesList, kSearch, kInvalid } LinkType;

    virtual LinkType GetKind() const						{ AfxDebugBreak(); return kInvalid; }
    virtual void GetLink(CString& /*lnk*/) const			{ AfxDebugBreak(); }
    virtual class CED2KFileLink* GetFileLink()				{ AfxDebugBreak(); return NULL; }
    virtual class CED2KNodesListLink* GetNodesListLink()	{ AfxDebugBreak(); return NULL; }
    virtual class CED2KSearchLink* GetSearchLink()			{ AfxDebugBreak(); return NULL; }
};

class CED2KFileLink : public CED2KLink
{
  public:
    CED2KFileLink(const TCHAR* pszName, const TCHAR* pszSize, const TCHAR* pszHash, const CStringArray& param, const TCHAR* pszSources);
    virtual ~CED2KFileLink();

    virtual LinkType GetKind() const;
    virtual void GetLink(CString& lnk) const;
    virtual CED2KFileLink* GetFileLink();
    virtual CED2KNodesListLink* GetNodesListLink()
    {
        return NULL;
    }
    virtual CED2KSearchLink* GetSearchLink()
    {
        return NULL;
    }

    const TCHAR* GetName() const
    {
        return m_name;
    }
    const uchar* GetHashKey() const
    {
        return m_hash;
    }
    const CAICHHash& GetAICHHash() const
    {
        return m_AICHHash;
    }
    EMFileSize GetSize() const
    {
        return (uint64)_tstoi64(m_size);
    }
    bool HasValidSources() const
    {
        return (SourcesList != NULL);
    }
    bool HasHostnameSources() const
    {
        return (!m_HostnameSourcesList.IsEmpty());
    }
    bool HasValidAICHHash() const
    {
        return m_bAICHHashValid;
    }
//>>> WiZaRd::CollectionEnhancement
    CString GetDownloadDirectory() const
    {
        return m_strDownloadDirectory;
    }
//<<< WiZaRd::CollectionEnhancement

    CSafeMemFile* SourcesList;
    CSafeMemFile* m_hashset;
    CTypedPtrList<CPtrList, SUnresolvedHostname*> m_HostnameSourcesList;

  private:
    CED2KFileLink();
    CED2KFileLink(const CED2KFileLink&);
    CED2KFileLink& operator=(const CED2KFileLink&);

    CString m_name;
    CString m_size;
    uchar	m_hash[16];
    bool	m_bAICHHashValid;
    CAICHHash m_AICHHash;
    CString m_strDownloadDirectory; //>>> WiZaRd::CollectionEnhancement
};

class CED2KNodesListLink : public CED2KLink
{
  public:
    CED2KNodesListLink(const TCHAR* pszAddress);
    virtual ~CED2KNodesListLink();

    virtual LinkType GetKind() const;
    virtual void GetLink(CString& lnk) const;
    virtual CED2KFileLink* GetFileLink();
    virtual CED2KNodesListLink* GetNodesListLink();
    virtual CED2KSearchLink* GetSearchLink()
    {
        return NULL;
    }

    const TCHAR* GetAddress() const
    {
        return m_address;
    }

  private:
    CString m_address;
};

class CED2KSearchLink : public CED2KLink
{
  public:
    CED2KSearchLink(const TCHAR* pszSearchTerm);
    virtual ~CED2KSearchLink();

    virtual LinkType GetKind() const;
    virtual void GetLink(CString& lnk) const;
    virtual CED2KFileLink* GetFileLink();
    virtual CED2KNodesListLink* GetNodesListLink();
    virtual CED2KSearchLink* GetSearchLink();

    CString GetSearchTerm() const
    {
        return m_strSearchTerm;
    }

  private:
    CString m_strSearchTerm;
};
