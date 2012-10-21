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
#include "CustomSearches.h"
#include "emule.h"
#include "emuledlg.h"
#include "SearchDlg.h"
#include "otherfunctions.h"
#include "SearchParams.h"
#include "Preferences.h"
#include "ComboBoxEx2.h"
#include "HttpDownloadDlg.h"
#include "Log.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
// CCustomSearch
CCustomSearch::CCustomSearch(const CString& name, const CString& url, const CString& types, const CString& lang)
{
	strName = name;
	if (!IsSeparator())
	{
		strURL = url;
		strTypes = types;
		strLang = lang;
	}
}

CCustomSearch::~CCustomSearch()
{
}

bool CCustomSearch::operator==(const CCustomSearch& ws)
{
	if (IsSeparator() || ws.IsSeparator())
		return false;
	return (!strName.CompareNoCase(ws.strName) && GetMainURL() == ws.GetMainURL());
}

void CCustomSearch::Remove()
{
	theApp.customSearches->Remove(this);
}

bool CCustomSearch::IsEmpty() const
{
	return ((strName.IsEmpty() || !strName.CompareNoCase(L"WEB")) &&
		(strURL.IsEmpty() || !strURL.CompareNoCase(L"http://")) &&
		strTypes.IsEmpty() && !strLang.CompareNoCase(L"multi"));
}

UINT CCustomSearch::GetLangIndex() const
{
	if (!strLang.CompareNoCase(L"en"))
		return 1;
	return 0;
}

CString CCustomSearch::GetSearchType(const CString& search) const
{
	int pos = 0;
	if (GetResString(IDS_SEARCH_ARC) == search) 
		pos = 1;
	else if (GetResString(IDS_SEARCH_AUDIO) == search) 
		pos = 2;
	else if (GetResString(IDS_SEARCH_CDIMG) == search) 
		pos = 3;
	else if (GetResString(IDS_SEARCH_PICS) == search) 
		pos = 4;
	else if (GetResString(IDS_SEARCH_PRG) == search) 
		pos = 5;
	else if (GetResString(IDS_SEARCH_VIDEO) == search) 
		pos = 6;
	else if (GetResString(IDS_SEARCH_DOC) == search) 
		pos = 7;

	CString tmp[8];
	int tPos = 0;
	for(int i = 0; i < pos+1 && tPos < strTypes.GetLength(); ++i)
		tmp[i] = strTypes.Tokenize(L";", tPos);

	return tmp[pos];
}

bool CCustomSearch::IsSeparator() const
{
	return (!strName.CompareNoCase(L"separator"));
}

CString CCustomSearch::GetName() const
{
	return strName;
}

CString CCustomSearch::GetURL() const
{
	return strURL;
}

CString CCustomSearch::GetMainURL() const
{
	CString url = strURL.Mid(strURL.Find(L"://")+3);
	url.MakeLower();
	int pos = url.ReverseFind(L'/');
	if (pos > 0)
		url = url.Left(pos);
	return url;
}

CString	CCustomSearch::GetSaveString() const
{
	CString ret = L"";

	if (IsSeparator())
		ret = L"<separator>";
	else
	{
		ret = strName + L"," + strURL + L"," + strLang;
		if (!strTypes.IsEmpty())
			ret += L"," + strTypes;
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////
// CCustomSearches
CCustomSearches::CCustomSearches()
{
	m_bDefaultSearch = false;
	Load();

	// create default setup if necessary
	if(!::PathFileExists(GetDefaultFile()) || m_bDefaultSearch)
	{
		Save();
		Load();
	}
}

CCustomSearches::~CCustomSearches()
{
	RemoveAll();
}

CString CCustomSearches::GetDefaultFile() const
{
	return thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + L"customSearches.dat";
}

bool CCustomSearches::Load(const CString& filename, const bool bUpdate)
{
	if (!bUpdate)
		RemoveAll();

	CString strFileName = GetDefaultFile();
	if (!::PathFileExists(strFileName) 
		&& (filename.IsEmpty() || !::PathFileExists(filename)))
		return false;

	if (bUpdate)
	{
		_tremove(strFileName);
		_trename(filename, strFileName);
		return Load();
	}
	if (!filename.IsEmpty())
		strFileName = filename;
		
	CStdioFile file;
	CFileException fexp;
	if (!file.Open(strFileName, CFile::modeRead|CFile::osSequentialScan|CFile::shareDenyWrite|CFile::typeText, &fexp))
	{
		if (fexp.m_cause != CFileException::fileNotFound)
		{
			CString strError = L"";
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, _countof(szError)))
				strError = szError;
			else
				strError = L"unknown error";

			theApp.QueueLogLineEx(LOG_ERROR, GetResString(IDS_ERR_FILEOPEN), strFileName, strError);
			return false;
		}
		return false;
	}

	CString sBuffer, name, url, strTypes, lang;
	while (file.ReadString(sBuffer))
	{
		sBuffer.Replace(L"\n", L"");

		// ignore comments
		if (sBuffer.Left(2) == L"//" || sBuffer.GetLength() < 5)
			continue;	

		int pos = sBuffer.Find(L",");
		if (sBuffer == L"<separator>")
			Add(new CCustomSearch(L"separator"));
		else if (pos > 0 && sBuffer.Find(L"://", pos) > 0)
		{
			pos = 0;
			name = sBuffer.Tokenize(L",", pos);
			url = sBuffer.Tokenize(L",", pos);
			lang = sBuffer.Tokenize(L",", pos);
			if (pos > 0)
				strTypes = sBuffer.Tokenize(L",", pos);
			else
				strTypes = L"";
			if (!name.IsEmpty() && !url.IsEmpty() && !SearchExists(name, url))
				Add(new CCustomSearch(name, url, strTypes, lang));
		}
		else if(sBuffer == L"defaultFileSetup")
			m_bDefaultSearch = true;
	}
	file.Close();
	if (bUpdate)
		_tremove(filename);
	return true;
}

bool CCustomSearches::Save()
{
	CString strFileName = GetDefaultFile();
	CStdioFile file;
	CFileException fexp;
	if (!file.Open(strFileName, CFile::modeWrite|CFile::modeCreate|CFile::osSequentialScan|CFile::shareDenyWrite|CFile::typeText, &fexp))
	{
		if (fexp.m_cause != CFileException::fileNotFound)
		{
			CString strError = L"";
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, _countof(szError)))
				strError = szError;
			else
				strError = L"unknown error";

			theApp.QueueLogLineEx(LOG_ERROR, GetResString(IDS_ERR_FILEOPEN), strFileName, strError);
			return false;
		}
		return false;
	}

	file.WriteString(L"///////////////////////////////////////////////////////\n");
	file.WriteString(L"//\n");
	file.WriteString(L"// -= WiZaRds CustomSearch File =-\n");
	file.WriteString(L"// This feature is based on Hebmule Websearches by Avi3k\n");
	file.WriteString(L"//\n");
	file.WriteString(L"// CustomSearch File Format:\n");
	file.WriteString(L"//\t<separator> - creates a separator\n");
	file.WriteString(L"//\tname,url,lang,type1,type2,... - creates a search entry\n");
	file.WriteString(L"//\n");
	file.WriteString(L"//\tfor lang you can use 1 (English) or 0 (default)\n");	
	file.WriteString(L"//\n");
	file.WriteString(L"//\tURL can be customised with the following terms:\n");
	file.WriteString(L"//\n");
	file.WriteString(L"//\t[type] - the selected file type\n");
	file.WriteString(L"//\t[minsize] - the selected minimum file size\n");
	file.WriteString(L"//\t[maxsize] - the selected maximum file size\n");
	file.WriteString(L"//\t[available] - the selected availability\n");
	file.WriteString(L"//\t[extension] - the selected file extension\n");
	file.WriteString(L"//\t[completesrc] - the selected complete source count\n");
	file.WriteString(L"//\t[codec] - the selected file codec\n");
	file.WriteString(L"//\t[bitrate] - the selected bitrate\n");
	file.WriteString(L"//\t[length] - the selected length\n");
	file.WriteString(L"//\t[title] - the selected title\n");
	file.WriteString(L"//\t[album] - the selected album\n");
	file.WriteString(L"//\t[artist] - the selected artist\n");
	file.WriteString(L"//\n");
	file.WriteString(L"// Remove the following line when customising searches\n");
	file.WriteString(L"// to prevent the client from changing the file automatically!\n");
	file.WriteString(L"defaultFileSetup\n\n");
	
 	CCustomSearch* tmp = new CCustomSearch(L"Zoozle", L"http://emule.zoozle.org/search.php?q=[query]&client=kMule");
 	file.WriteString(tmp->GetSaveString() + L"\n");
 	delete tmp;
	tmp = new CCustomSearch(L"eMule Content DB", L"http://contentdb.emule-project.net/search.php?s=[query]&client=kMule");
	file.WriteString(tmp->GetSaveString() + L"\n");
	delete tmp;
	for (UINT i = 0; i < GetCount(); ++i)
	{		
		if(m_aWebs[i])
		{
			CString ret = L"";
			ret = m_aWebs[i]->GetSaveString();
			file.WriteString(ret + L"\n");
		}
	}
	file.Close();
	return true;
}

void CCustomSearches::InitSearchList(CComboBoxEx2* list, CImageList* imglist)
{
	list->ResetContent();
	list->SetImageList(imglist);
	VERIFY( list->AddItem(GetResString(IDS_KADEMLIA) + L" " + GetResString(IDS_NETWORK), 0) == SearchTypeKademlia );
	VERIFY( list->AddItem(L"--------------------", 1) == (SearchTypeWeb+0) );
	for (UINT i = 0; i < GetCount(); ++i)
	{
		CCustomSearch* pWeb = m_aWebs[i];
		if (pWeb->IsSeparator())
			VERIFY( (UINT)list->AddItem(L"--------------------", 1) == (SearchTypeWeb+i+1) );
		else
			VERIFY( (UINT)list->AddItem(pWeb->GetName(), pWeb->GetLangIndex()+2) == (SearchTypeWeb+i+1) );
	}
	list->SetCurSel(thePrefs.GetSearchMethod() == CB_ERR ? SearchTypeKademlia : thePrefs.GetSearchMethod());
}

CString CCustomSearches::CreateQuery(const UINT index, SSearchParams* pPrms)
{
	CCustomSearch* tmp = GetAt(index);

	if (!tmp || (tmp && tmp->IsSeparator()) || !pPrms)
		return L"";

	CString query = tmp->GetURL();
	CString buffer = URLEncode(pPrms->strExpression);
	buffer.Replace(L"%20", L"+");
	query.Replace(L"[query]", buffer);

	buffer.Format(L"%s", pPrms->strFileType);
	query.Replace(L"[type]", tmp->GetSearchType(buffer));
	query.Replace(L"[minsize]", (pPrms->ullMinSize > (uint64)0) ? pPrms->strMinSize : L"");
	query.Replace(L"[maxsize]", (pPrms->ullMaxSize > (uint64)0) ? pPrms->strMaxSize : L"");
	buffer.Format(L"%i", pPrms->uAvailability);
	query.Replace(L"[available]", (pPrms->uAvailability > 0) ? buffer : L"");
	query.Replace(L"[extension]", pPrms->strExtension);
	buffer.Format(L"%i", pPrms->uComplete);
	query.Replace(L"[completesrc]", (pPrms->uComplete > 0) ? buffer : L"");
	query.Replace(L"[codec]", pPrms->strCodec);
	buffer.Format(L"%i", pPrms->ulMinBitrate);
	query.Replace(L"[bitrate]", (pPrms->ulMinBitrate > 0) ? buffer : L"");
	buffer.Format(L"%i", pPrms->ulMinLength);
	query.Replace(L"[length]", (pPrms->ulMinLength > 0) ? buffer : L"");
	query.Replace(L"[title]", pPrms->strTitle);
	query.Replace(L"[album]", pPrms->strAlbum);
	query.Replace(L"[artist]", pPrms->strArtist);

	return query;
}

bool CCustomSearches::SearchExists(const CString& strName, const CString& strURL)
{
	CCustomSearch ws(strName, strURL);
	for (UINT i = 0; i < GetCount(); ++i)
		if (ws == *m_aWebs[i])
			return true;
	return false;
}

CCustomSearch* CCustomSearches::GetAt(const UINT index) const
{
	if (index < GetCount())
		return m_aWebs[index];
	return NULL;
}

void CCustomSearches::Add(CCustomSearch* pWeb)
{
	if (pWeb)
		m_aWebs.Add(pWeb);
}

void CCustomSearches::Remove(CCustomSearch* pWeb)
{
	for (UINT i = 0; i < GetCount(); ++i)
	{
		if (pWeb == m_aWebs[i])
		{
			delete m_aWebs[i]; //WiZaRd - memleak fix
			m_aWebs.RemoveAt(i);
			break;
		}
	}
}

void CCustomSearches::RemoveAll()
{
	for (UINT i = 0; i < GetCount(); ++i)
		delete m_aWebs[i];
	m_aWebs.RemoveAll();
}

UINT CCustomSearches::GetCount() const
{
	return (UINT)m_aWebs.GetCount();
}

/*
bool CCustomSearches::Download(const CString& strURL)
{	
	CString sURL = strURL;
	if(sURL.IsEmpty())
		sURL = GetDefaultFile();

	CString szTempFilePath = L"";
	_tmakepathlimit(szTempFilePath.GetBuffer(MAX_PATH), NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), GetFileNameFromURL(sURL) + L".new", NULL);
	szTempFilePath.ReleaseBuffer();

	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_strTitle.Format(GetResString(IDS_HTTPDOWNLOAD_FILESTATUS), GetResString(IDS_FILE), strURL);
	dlgDownload.m_sURLToDownload = strURL;
	dlgDownload.m_sFileToDownloadInto = szTempFilePath;
	if (dlgDownload.DoModal() != IDOK || GetDiskFileSize(szTempFilePath) == 0)
	{
		_tremove(szTempFilePath);
		LogError(NULL, L"Download from %s failed!", strURL);
		return false;
	}

	CString msg = L"Download complete!\nDo you want to replace the existing file?\nSelect \"no\" to append the downloaded file to the existing one!";
	if(AfxMessageBox(msg, MB_YESNO | MB_ICONWARNING) == IDYES)
		theApp.customSearches->Load(szTempFilePath, true);
	else
		theApp.customSearches->Load(szTempFilePath, false);
	_tremove(szTempFilePath);
	if(theApp.emuledlg && theApp.emuledlg->searchwnd && theApp.emuledlg->IsRunning())
		theApp.emuledlg->searchwnd->UpdateSearchList();
	return true;
}
*/