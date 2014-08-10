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
struct SSearchParams;
class CComboBoxEx2;

class CCustomSearch
{
  public:
    CCustomSearch(const CString& name = L"WEB", const CString& url = L"http://", const CString& types = L"", const CString& lang = L"multi");
    virtual ~CCustomSearch();
    bool operator==(const CCustomSearch& ws);

    void	Remove();
    bool	IsEmpty() const;
    bool	IsSeparator() const;
    CString GetName() const;
    CString GetURL() const;
    UINT	GetLangIndex() const;
    CString GetSearchType(const CString& search) const;
    CString	GetSaveString() const;

  private:
    CString strName;
    CString strURL;
    CString strTypes;
    CString strLang;

  protected:
    CString GetMainURL() const;
};

class CCustomSearches
{
  public:
    CCustomSearches();
    virtual ~CCustomSearches();

    bool	Load(const CString& filename = L"", const bool bUpdate = false);
    bool	Save();
    void	InitSearchList(CComboBoxEx2* list, CImageList* imglist);
    CString CreateQuery(const UINT index, SSearchParams* pPrms = NULL);
    bool	SearchExists(const CString& sName, const CString& sUrl);

    CCustomSearch* GetAt(const UINT index) const;
    void	Add(CCustomSearch* pWeb);
    void	Remove(CCustomSearch* pWeb);
    void	RemoveAll();
    UINT	GetCount() const;
    CString	GetDefaultFile() const;

//	bool    Download(const CString& strURL = L"");

  private:
    bool	m_bDefaultSearch;
    CArray<CCustomSearch*> m_aWebs;
};