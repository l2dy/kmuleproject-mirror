//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include <share.h>
#include <fcntl.h>
#include <io.h>
#include "emule.h"
#include "IPFilter.h"
#include "OtherFunctions.h"
#include "StringConversion.h"
#include "Preferences.h"
#include "emuledlg.h"
#include "Log.h"
#include "Statistics.h"
//>>> WiZaRd::IPFilter-Update
#include "HttpDownloadDlg.h"
#include "./Mod/extractfile.h"
//<<< WiZaRd::IPFilter-Update
#include "./Mod/loadStatus.h" //>>> WiZaRd::LoadStatus [X-Ray]

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	DFLT_FILTER_LEVEL	100 // default filter level if non specified

CIPFilter::CIPFilter()
{
    m_pLastHit = NULL;
    m_bModified = false;
    LoadFromDefaultFile(false);
}

CIPFilter::~CIPFilter()
{
    if (m_bModified)
    {
        try
        {
            SaveToDefaultFile();
        }
        catch (CString)
        {
        }
    }
    RemoveAllIPFilters();
}

static int __cdecl CmpSIPFilterByStartAddr(const void* p1, const void* p2)
{
    const SIPFilter* rng1 = *(SIPFilter**)p1;
    const SIPFilter* rng2 = *(SIPFilter**)p2;
    return CompareUnsigned(rng1->start, rng2->start);
}

CString CIPFilter::GetDefaultFilePath() const
{
    return thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + DFLT_IPFILTER_FILENAME;
}

int CIPFilter::LoadFromDefaultFile(bool bShowResponse)
{
    RemoveAllIPFilters();
    return AddFromFile(GetDefaultFilePath(), bShowResponse);
}

int CIPFilter::AddFromFile(LPCTSTR pszFilePath, bool bShowResponse)
{
    DWORD dwStart = GetTickCount();

//>>> WiZaRd::LoadStatus [X-Ray]
    CLoadStatus loadStatus;
    loadStatus.SetLoadMode(true);
    loadStatus.SetLoadString(L"IP-Filter");
    loadStatus.ReadLineCount(pszFilePath);
//<<< WiZaRd::LoadStatus [X-Ray]

    FILE* readFile = _tfsopen(pszFilePath, _T("r"), _SH_DENYWR);
    if (readFile != NULL)
    {
        enum EIPFilterFileType
        {
            Unknown = 0,
            FilterDat = 1,		// ipfilter.dat/ip.prefix format
            PeerGuardian = 2,	// PeerGuardian text format
            PeerGuardian2 = 3	// PeerGuardian binary format
        } eFileType = Unknown;

        setvbuf(readFile, NULL, _IOFBF, 32768);

        TCHAR szNam[_MAX_FNAME];
        TCHAR szExt[_MAX_EXT];
        _tsplitpath(pszFilePath, NULL, NULL, szNam, szExt);
        if (_tcsicmp(szExt, _T(".p2p")) == 0 || (_tcsicmp(szNam, _T("guarding.p2p")) == 0 && _tcsicmp(szExt, _T(".txt")) == 0))
            eFileType = PeerGuardian;
        else if (_tcsicmp(szExt, _T(".prefix")) == 0)
            eFileType = FilterDat;
        else
        {
            VERIFY(_setmode(_fileno(readFile), _O_BINARY) != -1);
            static const BYTE _aucP2Bheader[] = "\xFF\xFF\xFF\xFFP2B";
            BYTE aucHeader[sizeof _aucP2Bheader - 1];
            if (fread(aucHeader, sizeof aucHeader, 1, readFile) == 1)
            {
                if (memcmp(aucHeader, _aucP2Bheader, sizeof _aucP2Bheader - 1)==0)
                    eFileType = PeerGuardian2;
                else
                {
                    (void)fseek(readFile, 0, SEEK_SET);
                    VERIFY(_setmode(_fileno(readFile), _O_TEXT) != -1);   // ugly!
                }
            }
        }

        int iFoundRanges = 0;
        int iLine = 0;
        if (eFileType == PeerGuardian2)
        {
            // Version 1: strings are ISO-8859-1 encoded
            // Version 2: strings are UTF-8 encoded
            // Version 3: PeerBlock cache.p2b format
            uint8 nVersion;
            if (fread(&nVersion, sizeof nVersion, 1, readFile)==1 && (nVersion == 1 || nVersion == 2 || nVersion == 3)) //>>> WiZaRd::Support Peerblock [ElKarro]
            {
                if (nVersion == 1 || nVersion == 2) //>>> WiZaRd::Support Peerblock [ElKarro]
                {
                    while (!feof(readFile))
                    {
                        CHAR szName[256];
                        int iLen = 0;
                        for (;;) // read until NUL or EOF
                        {
                            int iChar = getc(readFile);
                            if (iChar == EOF)
                                break;
                            if (iLen < sizeof szName - 1)
                                szName[iLen++] = (CHAR)iChar;
                            if (iChar == '\0')
                                break;
                        }
                        szName[iLen] = '\0';

                        UINT uStart;
                        if (fread(&uStart, sizeof uStart, 1, readFile) != 1)
                            break;
                        uStart = ntohl(uStart);

                        UINT uEnd;
                        if (fread(&uEnd, sizeof uEnd, 1, readFile) != 1)
                            break;
                        uEnd = ntohl(uEnd);

                        iLine++;
                        // (nVersion == 2) ? OptUtf8ToStr(szName, iLen) :
                        AddIPRange(uStart, uEnd, DFLT_FILTER_LEVEL, CStringA(szName, iLen));
                        iFoundRanges++;

                        loadStatus.UpdateCount(iLine); //>>> WiZaRd::LoadStatus [X-Ray]
                    }
                }
//>>> WiZaRd::Support Peerblock [ElKarro]
                else
                {
                    UINT namecount;
                    if (fread(&namecount, sizeof namecount, 1, readFile) == 1)
                    {
                        namecount = ntohl(namecount);
                        long *nameIdx = new long [namecount + 1];

                        if (nameIdx)
                        {
                            UINT maxStrLen = 0;
                            UINT strLen = 0;

                            nameIdx[0] = 0x0C;

                            for (UINT i = 0; i < namecount; ++i)
                            {
                                for (strLen = 0; ; ++strLen)
                                {
                                    CHAR ch;
                                    if (fread(&ch, sizeof ch, 1, readFile) == 1)
                                    {
                                        if (ch == '\0')
                                            break;
                                    }
                                    else
                                    {
                                        // unexpected EOF
                                        strLen = 0;
                                        break;
                                    }
                                }

                                if (strLen)
                                {
                                    if (maxStrLen < strLen)
                                        maxStrLen = strLen;

                                    nameIdx[i+1] = nameIdx[i] + strLen + 1;
                                }
                                else
                                    break;
                            }

                            if (strLen)
                            {
                                UINT rangecount;
                                if (fread(&rangecount, sizeof rangecount, 1, readFile) == 1)
                                {
                                    long rangeOfs = ftell(readFile);

                                    if (rangeOfs >= 0x0C)
                                    {
                                        char *szName = new char [maxStrLen];

                                        if (szName)
                                        {
                                            rangecount = ntohl(rangecount);
                                            for (UINT i = 0; i < rangecount; ++i)
                                            {
                                                struct
                                                {
                                                    UINT name;
                                                    UINT start;
                                                    UINT end;
                                                } theRange;

                                                if (fread(&theRange, sizeof theRange, 1, readFile) == 1)
                                                {
                                                    UINT name = ntohl(theRange.name);

                                                    if (name < namecount)
                                                    {
                                                        if (fseek(readFile, nameIdx[name], SEEK_SET) == 0)
                                                        {
                                                            UINT iLen = nameIdx[name+1]-nameIdx[name] - 1;
                                                            if (fread(szName, sizeof(char), iLen, readFile) == iLen)
                                                            {
                                                                ++iLine;
                                                                loadStatus.UpdateCount(iLine); //>>> WiZaRd::LoadStatus [X-Ray]
                                                                AddIPRange(ntohl(theRange.start), ntohl(theRange.end), DFLT_FILTER_LEVEL, CStringA(szName, iLen));
                                                                ++iFoundRanges;

                                                                rangeOfs += sizeof theRange;

                                                                if (fseek(readFile, rangeOfs, SEEK_SET))
                                                                    break;
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                            delete [] szName;
                                        }
                                    }
                                }
                            }
                        }

                        delete [] nameIdx;
                    }
                }
//<<< WiZaRd::Support Peerblock [ElKarro]
            }
        }
        else
        {
            CStringA sbuffer;
            CHAR szBuffer[1024];
            while (fgets(szBuffer, _countof(szBuffer), readFile) != NULL)
            {
                iLine++;

                loadStatus.UpdateCount(iLine); //>>> WiZaRd::LoadStatus [X-Ray]

                sbuffer = szBuffer;

                // ignore comments & too short lines
                if (sbuffer.GetAt(0) == '#' || sbuffer.GetAt(0) == '/' || sbuffer.GetLength() < 5)
                {
                    sbuffer.Trim(" \t\r\n");
                    DEBUG_ONLY((!sbuffer.IsEmpty()) ? TRACE(L"IP filter: ignored line %u\n", iLine) : 0);
                    continue;
                }

                if (eFileType == Unknown)
                {
                    // looks like html
                    if (sbuffer.Find('>') > -1 && sbuffer.Find('<') > -1)
                        sbuffer.Delete(0, sbuffer.ReverseFind('>') + 1);

                    // check for <IP> - <IP> at start of line
                    UINT u1, u2, u3, u4, u5, u6, u7, u8;
                    if (sscanf(sbuffer, "%u.%u.%u.%u - %u.%u.%u.%u", &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8) == 8)
                    {
                        eFileType = FilterDat;
                    }
                    else
                    {
                        // check for <description> ':' <IP> '-' <IP>
                        int iColon = sbuffer.Find(':');
                        if (iColon > -1)
                        {
                            CStringA strIPRange = sbuffer.Mid(iColon + 1);
                            UINT u1, u2, u3, u4, u5, u6, u7, u8;
                            if (sscanf(strIPRange, "%u.%u.%u.%u - %u.%u.%u.%u", &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8) == 8)
                            {
                                eFileType = PeerGuardian;
                            }
                        }
                    }
                }

                bool bValid = false;
                UINT start = 0;
                UINT end = 0;
                UINT level = 0;
                CStringA desc;
                if (eFileType == FilterDat)
                    bValid = ParseFilterLine1(sbuffer, start, end, level, desc);
                else if (eFileType == PeerGuardian)
                    bValid = ParseFilterLine2(sbuffer, start, end, level, desc);

                // add a filter
                if (bValid)
                {
                    AddIPRange(start, end, level, desc);
                    iFoundRanges++;
                }
                else
                {
                    sbuffer.Trim(" \t\r\n");
                    DEBUG_ONLY((!sbuffer.IsEmpty()) ? TRACE(L"IP filter: ignored line %u\n", iLine) : 0);
                }
            }
        }
        fclose(readFile);

        // sort the IP filter list by IP range start addresses
        qsort(m_iplist.GetData(), m_iplist.GetCount(), sizeof(m_iplist[0]), CmpSIPFilterByStartAddr);

        // merge overlapping and adjacent filter ranges
        int iDuplicate = 0;
        int iMerged = 0;
        if (m_iplist.GetCount() >= 2)
        {
            // On large IP-filter lists there is a noticeable performance problem when merging the list.
            // The 'CIPFilterArray::RemoveAt' call is way too expensive to get called during the merging,
            // thus we use temporary helper arrays to copy only the entries into the final list which
            // are not get deleted.

            // Reserve a byte array (its used as a boolean array actually) as large as the current
            // IP-filter list, so we can set a 'to delete' flag for each entry in the current IP-filter list.
            char* pcToDelete = new char[m_iplist.GetCount()];
            memset(pcToDelete, 0, m_iplist.GetCount());
            int iNumToDelete = 0;

            SIPFilter* pPrv = m_iplist[0];
            int i = 1;
            while (i < m_iplist.GetCount())
            {
                SIPFilter* pCur = m_iplist[i];
                if (pCur->start >= pPrv->start && pCur->start <= pPrv->end	    // overlapping
                        || pCur->start == pPrv->end+1 && pCur->level == pPrv->level) // adjacent
                {
                    if (pCur->start != pPrv->start || pCur->end != pPrv->end) // don't merge identical entries
                    {
                        //TODO: not yet handled, overlapping entries with different 'level'
                        if (pCur->end > pPrv->end)
                            pPrv->end = pCur->end;
                        //pPrv->desc += _T("; ") + pCur->desc; // this may create a very very long description string...
                        iMerged++;
                    }
                    else
                    {
                        // if we have identical entries, use the lowest 'level'
                        if (pCur->level < pPrv->level)
                            pPrv->level = pCur->level;
                        iDuplicate++;
                    }
                    delete pCur;
                    //m_iplist.RemoveAt(i);	// way too expensive (read above)
                    pcToDelete[i] = 1;		// mark this entry as 'to delete'
                    iNumToDelete++;
                    i++;
                    continue;
                }
                pPrv = pCur;
                i++;
            }

            // Create new IP-filter list which contains only the entries from the original IP-filter list
            // which are not to be deleted.
            if (iNumToDelete > 0)
            {
                CIPFilterArray newList;
                newList.SetSize(m_iplist.GetCount() - iNumToDelete);
                int iNewListIndex = 0;
                for (int i = 0; i < m_iplist.GetCount(); i++)
                {
                    if (!pcToDelete[i])
                        newList[iNewListIndex++] = m_iplist[i];
                }
                ASSERT(iNewListIndex == newList.GetSize());

                // Replace current list with new list. Dump, but still fast enough (only 1 memcpy)
                m_iplist.RemoveAll();
                m_iplist.Append(newList);
                newList.RemoveAll();
                m_bModified = true;
            }
            delete[] pcToDelete;
        }

        loadStatus.Complete(); //>>> WiZaRd::LoadStatus [X-Ray]
        if (thePrefs.GetVerbose())
        {
            DWORD dwEnd = GetTickCount();
            AddDebugLogLine(false, _T("Loaded IP filters from \"%s\""), pszFilePath);
            AddDebugLogLine(false, _T("Parsed lines/entries:%u  Found IP ranges:%u  Duplicate:%u  Merged:%u  Time:%s"), iLine, iFoundRanges, iDuplicate, iMerged, CastSecondsToHM((dwEnd-dwStart+500)/1000));
        }
        AddLogLine(bShowResponse, GetResString(IDS_IPFILTERLOADED), m_iplist.GetCount());
    }
    return m_iplist.GetCount();
}

void CIPFilter::SaveToDefaultFile()
{
    CString strFilePath = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + DFLT_IPFILTER_FILENAME;
    FILE* fp = _tfsopen(strFilePath, _T("wt"), _SH_DENYWR);
    if (fp != NULL)
    {
        for (int i = 0; i < m_iplist.GetCount(); i++)
        {
            const SIPFilter* flt = m_iplist[i];

            CHAR szStart[16];
            ipstrA(szStart, _countof(szStart), htonl(flt->start));

            CHAR szEnd[16];
            ipstrA(szEnd, _countof(szEnd), htonl(flt->end));

            if (fprintf(fp, "%-15s - %-15s , %3u , %s\n", szStart, szEnd, flt->level, flt->desc) == 0 || ferror(fp))
            {
                fclose(fp);
                CString strError;
                strError.Format(_T("Failed to save IP filter to file \"%s\" - %s"), strFilePath, _tcserror(errno));
                throw strError;
            }
        }
        fclose(fp);
        m_bModified = false;
    }
    else
    {
        CString strError;
        strError.Format(_T("Failed to save IP filter to file \"%s\" - %s"), strFilePath, _tcserror(errno));
        throw strError;
    }
}

bool CIPFilter::ParseFilterLine1(const CStringA& sbuffer, UINT& ip1, UINT& ip2, UINT& level, CStringA& desc) const
{
    UINT u1, u2, u3, u4, u5, u6, u7, u8, uLevel = DFLT_FILTER_LEVEL;
    int iDescStart = 0;
    int iItems = sscanf(sbuffer, "%u.%u.%u.%u - %u.%u.%u.%u , %u , %n", &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8, &uLevel, &iDescStart);
    if (iItems < 8)
        return false;

    ((BYTE*)&ip1)[0] = (BYTE)u4;
    ((BYTE*)&ip1)[1] = (BYTE)u3;
    ((BYTE*)&ip1)[2] = (BYTE)u2;
    ((BYTE*)&ip1)[3] = (BYTE)u1;

    ((BYTE*)&ip2)[0] = (BYTE)u8;
    ((BYTE*)&ip2)[1] = (BYTE)u7;
    ((BYTE*)&ip2)[2] = (BYTE)u6;
    ((BYTE*)&ip2)[3] = (BYTE)u5;

    if (iItems == 8)
    {
        level = DFLT_FILTER_LEVEL;	// set default level
        return true;
    }

    level = uLevel;

    if (iDescStart > 0)
    {
        LPCSTR pszDescStart = (LPCSTR)sbuffer + iDescStart;
        int iDescLen = sbuffer.GetLength() - iDescStart;
        if (iDescLen > 0)
        {
            if (*(pszDescStart + iDescLen - 1) == '\n')
                --iDescLen;
        }
        memcpy(desc.GetBuffer(iDescLen), pszDescStart, iDescLen * sizeof(pszDescStart[0]));
        desc.ReleaseBuffer(iDescLen);
    }

    return true;
}

bool CIPFilter::ParseFilterLine2(const CStringA& sbuffer, UINT& ip1, UINT& ip2, UINT& level, CStringA& desc) const
{
    int iPos = sbuffer.ReverseFind(':');
    if (iPos < 0)
        return false;

    desc = sbuffer.Left(iPos);
    desc.Replace("PGIPDB", "");
    desc.Trim();

    CStringA strIPRange = sbuffer.Mid(iPos + 1, sbuffer.GetLength() - iPos);
    UINT u1, u2, u3, u4, u5, u6, u7, u8;
    if (sscanf(strIPRange, "%u.%u.%u.%u - %u.%u.%u.%u", &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8) != 8)
        return false;

    ((BYTE*)&ip1)[0] = (BYTE)u4;
    ((BYTE*)&ip1)[1] = (BYTE)u3;
    ((BYTE*)&ip1)[2] = (BYTE)u2;
    ((BYTE*)&ip1)[3] = (BYTE)u1;

    ((BYTE*)&ip2)[0] = (BYTE)u8;
    ((BYTE*)&ip2)[1] = (BYTE)u7;
    ((BYTE*)&ip2)[2] = (BYTE)u6;
    ((BYTE*)&ip2)[3] = (BYTE)u5;

    level = DFLT_FILTER_LEVEL;

    return true;
}

void CIPFilter::RemoveAllIPFilters()
{
//>>> WiZaRd::LoadStatus [X-Ray]
    CLoadStatus loadStatus;
    loadStatus.SetLoadMode(false);
    loadStatus.SetLoadString(L"IP-Filter");
    loadStatus.SetLineCount(m_iplist.GetCount());
//<<< WiZaRd::LoadStatus [X-Ray]
    for (int i = 0; i < m_iplist.GetCount(); i++)
    {
        delete m_iplist[i];
        loadStatus.UpdateCount(i); //>>> WiZaRd::LoadStatus [X-Ray]
    }

    loadStatus.Complete(); //>>> WiZaRd::LoadStatus [X-Ray]
    m_iplist.RemoveAll();
    m_pLastHit = NULL;
}

bool CIPFilter::IsFiltered(UINT ip) /*const*/
{
    return IsFiltered(ip, thePrefs.GetIPFilterLevel());
}

static int __cdecl CmpSIPFilterByAddr(const void* pvKey, const void* pvElement)
{
    UINT ip = *(UINT*)pvKey;
    const SIPFilter* pIPFilter = *(SIPFilter**)pvElement;

    if (ip < pIPFilter->start)
        return -1;
    if (ip > pIPFilter->end)
        return 1;
    return 0;
}

bool CIPFilter::IsFiltered(UINT ip, UINT level) /*const*/
{
    if (m_iplist.GetCount() == 0 || ip == 0)
        return false;

    ip = htonl(ip);

    // to speed things up we use a binary search
    //	*)	the IP filter list must be sorted by IP range start addresses
    //	*)	the IP filter list is not allowed to contain overlapping IP ranges (see also the IP range merging code when
    //		loading the list)
    //	*)	the filter 'level' is ignored during the binary search and is evaluated only for the found element
    //
    // TODO: this can still be improved even more:
    //	*)	use a pre assembled list of IP ranges which contains only the IP ranges for the currently used filter level
    //	*)	use a dumb plain array for storing the IP range structures. this will give more cach hits when processing
    //		the list. but(!) this would require to also use a dumb SIPFilter structure (don't use data items with ctors).
    //		otherwise the creation of the array would be rather slow.
    SIPFilter** ppFound = (SIPFilter**)bsearch(&ip, m_iplist.GetData(), m_iplist.GetCount(), sizeof(m_iplist[0]), CmpSIPFilterByAddr);
    if (ppFound && (*ppFound)->level < level)
    {
        (*ppFound)->hits++;
        m_pLastHit = *ppFound;
        ++theStats.filteredclients;
        return true;
    }

    return false;
}

CString CIPFilter::GetLastHit() const
{
    return m_pLastHit ? CString(m_pLastHit->desc) : _T("Not available");
}

const CIPFilterArray& CIPFilter::GetIPFilter() const
{
    return m_iplist;
}

bool CIPFilter::RemoveIPFilter(const SIPFilter* pFilter)
{
    for (int i = 0; i < m_iplist.GetCount(); i++)
    {
        if (m_iplist[i] == pFilter)
        {
            delete m_iplist[i];
            m_iplist.RemoveAt(i);
            return true;
        }
    }
    return false;
}

//>>> WiZaRd::IPFilter-Update
void CIPFilter::UpdateIPFilterURL(const CString& m_sURL)
{
    CString strURL = L"";
    if (m_sURL.IsEmpty())
        strURL = thePrefs.GetUpdateURLIPFilter();
    else
        strURL = m_sURL;

    strURL.TrimRight(L".txt");
    strURL.TrimRight(L".dat");
    strURL.TrimRight(L".zip");
    strURL.Append(L".txt");

    TCHAR szTempFilePath[_MAX_PATH];
    _tmakepathlimit(szTempFilePath, NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), DFLT_IPFILTER_FILENAME, L"tmp");

    CHttpDownloadDlg dlgDownload;
    dlgDownload.m_strTitle = GetResString(IDS_DWL_IPFILTERFILE) + L" (Version)";
    dlgDownload.m_sURLToDownload = strURL;
    dlgDownload.m_sFileToDownloadInto = szTempFilePath;

    UINT uVersion = 0;
    bool bReadVersion = false;
    if (dlgDownload.DoModal() == IDOK)
    {
        CStdioFile IPFilterDotTxtFile;
        if (IPFilterDotTxtFile.Open(szTempFilePath, CFile::modeRead | CFile::shareDenyWrite))
        {
            CString strVersion;
            IPFilterDotTxtFile.ReadString(strVersion);

            strVersion.Trim();
            uVersion = (UINT)_tstoi(strVersion);
            bReadVersion = true;

            IPFilterDotTxtFile.Close();
        }
        _tremove(szTempFilePath);

        // no need to update? exit now!
        if (bReadVersion && thePrefs.GetIPfilterVersion() >= uVersion && ::PathFileExists(GetDefaultFilePath()) && GetDiskFileSize(GetDefaultFilePath()) >= 10240)
            return;
    }
    else
    {
        _tremove(szTempFilePath);
        CString strError = GetResString(IDS_DWLIPFILTERFAILED) + L" (Version)";
        if (!dlgDownload.GetError().IsEmpty())
            strError.AppendFormat(L" (%s)", dlgDownload.GetError());
        AddLogLine(true, strError);
    }

    CString IPFilterURL = L"";
    if (m_sURL.IsEmpty())
        IPFilterURL = thePrefs.GetUpdateURLIPFilter();
    else
        IPFilterURL = m_sURL;

    _tmakepathlimit(szTempFilePath, NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), DFLT_IPFILTER_FILENAME, L"tmp");

    dlgDownload.m_strTitle = GetResString(IDS_DWL_IPFILTERFILE);
    dlgDownload.m_sURLToDownload = IPFilterURL;
    dlgDownload.m_sFileToDownloadInto = szTempFilePath;
    if (dlgDownload.DoModal() != IDOK || GetDiskFileSize(szTempFilePath) < 10240)
    {
        _tremove(szTempFilePath);
        CString strError = GetResString(IDS_DWLIPFILTERFAILED);
        if (!dlgDownload.GetError().IsEmpty())
            strError.AppendFormat(L" (%s)", dlgDownload.GetError());
        AddLogLine(true, strError);
        return;
    }

    if (!Extract(szTempFilePath, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + DFLT_IPFILTER_FILENAME, DFLT_IPFILTER_FILENAME, true, 2, DFLT_IPFILTER_FILENAME, L"guarding.p2p"))
        return;

    LoadFromDefaultFile();

    thePrefs.SetIpfilterVersion(uVersion);
    thePrefs.Save();
}
//<<< WiZaRd::IPFilter-Update