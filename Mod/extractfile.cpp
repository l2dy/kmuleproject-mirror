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
#include "extractfile.h"
#include "Log.h"
#include "RarFile.h"
#include "ZipFile.h"
#include "GZipFile.h"
#include <share.h>
#include "Preferences.h"
#include "emule.h"
#include "otherfunctions.h" //needed for inside MediaInfo.h
#include "MediaInfo.h"
#include "HttpDownloadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

bool	DownloadFromURLToFile(const CString& strURL, const CString& strFile)
{
    CHttpDownloadDlg dlgDownload;
    dlgDownload.m_strTitle.Format(L"Downloading %s from %s...", GetResString(IDS_FILE), strURL);
    dlgDownload.m_sURLToDownload = strURL;
    dlgDownload.m_sFileToDownloadInto = strFile;
    if (dlgDownload.DoModal() != IDOK)
    {
        _tremove(strFile);
        LogError(NULL, _T("Download from %s failed!"), strURL);
        return false;
    }
    return true;
}

void	DoCleanUp(const CString& strBasicFileName, const CString& strOrigFile, const CString& strTempUnzipFilePath, const CString& strTempFilePath, const bool bDelFile)
{
    if (_tremove(strOrigFile) != 0 && errno != ENOENT)
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"*** Error: Failed to remove default %s file \"%s\" - %s", strBasicFileName, strOrigFile, _tcserror(errno));
    if (_trename(strTempUnzipFilePath, strOrigFile) != 0)
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"*** Error: Failed to rename uncompressed %s file \"%s\" to default file \"%s\" - %s", strBasicFileName, strTempUnzipFilePath, strOrigFile, _tcserror(errno));
    if (bDelFile)
    {
        if (strTempFilePath.CompareNoCase(strOrigFile) != 0 //don't delete if orig and new file were equal
                && _tremove(strTempFilePath) != 0 && errno != ENOENT)
            theApp.QueueDebugLogLineEx(LOG_ERROR, L"*** Error: Failed to remove temporary %s file \"%s\" - %s", strBasicFileName, strTempFilePath, _tcserror(errno));
    }
}

//////////////////////////////////////////////////////////////////////////
// Extract Wrapper
//
// strTempFilePath:		the path of the file from which you want to extract
// strOrigFilePath:		the path of the file which you want to replace
// strBasicFileName:	the basic filename to use in logs
// strFileToExtract:	the file to search and extract from the archive
//-- Partially taken from official codepart in PPgSecurity.cpp
//////////////////////////////////////////////////////////////////////////
CString GetUnpackPath(const CString& strFileToExtract)
{
    CString strTempUnzipFilePath = L"";
    _tmakepathlimit(strTempUnzipFilePath.GetBuffer(_MAX_PATH), NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGBASEDIR), strFileToExtract, L".unpack"); //Beta49: EMULE_CONFIGBASEDIR has no write permissions!
    strTempUnzipFilePath.ReleaseBuffer();
    _tremove(strTempUnzipFilePath); //just to be sure

    return strTempUnzipFilePath;
}

bool Extract(const CString& strTempFilePath, const CString& strOrigFile, const CString& strBasicFileName, const bool bDelAfterExtract, UINT numFiles, ...)
{
    CStringList strFilesToExtract;

    va_list args;
    va_start(args, numFiles);
    while (numFiles-- > 0)
    {
        LPCTSTR pszFileToExtract = va_arg(args, LPCTSTR);
        strFilesToExtract.AddTail(pszFileToExtract);
    }
    va_end(args);

    return Extract(strTempFilePath, strOrigFile, strBasicFileName, strFilesToExtract, bDelAfterExtract);
}

bool Extract(const CString& strTempFilePath, const CString& strOrigFile, const CString& strBasicFileName, const CStringList& strFilesToExtract, const bool bDelAfterExtract)
{
    bool bHaveNewFile = false;
    CString strError = L"";

    bool bIsArchiveFile = false;
    bool bUncompressed = false;
    CZIPFile zip;
    if (zip.Open(strTempFilePath))
    {
        bIsArchiveFile = true;

        for (POSITION pos = strFilesToExtract.GetHeadPosition(); pos && !bHaveNewFile;)
        {
            CString strFileToExtract = strFilesToExtract.GetNext(pos);

            CZIPFile::File* zfile = zip.GetFile(strFileToExtract);
            if (zfile)
            {
                CString strTempUnzipFilePath = GetUnpackPath(strFileToExtract);

                if (zfile->Extract(strTempUnzipFilePath))
                {
                    zip.Close();
                    zfile = NULL;
                    bUncompressed = true;
                    bHaveNewFile = true;

                    DoCleanUp(strBasicFileName, strOrigFile, strTempUnzipFilePath, strTempFilePath, bDelAfterExtract);
                }
                else
                    strError.Format(L"Failed to extract %s file from ZIP file \"%s\".", strBasicFileName, strTempFilePath);
            }
        }
        if (!bHaveNewFile)
        {
            //do we need localization?
            strError.Format(L"Downloaded %s file \"%s\" is a ZIP file with unexpected content.", strBasicFileName, strTempFilePath);
        }

        zip.Close();
    }
    else
    {
        CString strMimeType = L"";
        GetMimeType(strTempFilePath, strMimeType);
        if (strMimeType.CompareNoCase(L"application/x-rar-compressed") == 0)
        {
            CRARFile rar;
            if (rar.Open(strTempFilePath))
            {
                bIsArchiveFile = true;

                CString strFile;
                while (rar.GetNextFile(strFile))
                {
                    for (POSITION pos = strFilesToExtract.GetHeadPosition(); pos && !bHaveNewFile;)
                    {
                        CString strFileToExtract = strFilesToExtract.GetNext(pos);
                        if (strFile.CompareNoCase(strFileToExtract) == 0)
                        {
                            CString strTempUnzipFilePath = GetUnpackPath(strFileToExtract);

                            if (rar.Extract(strTempUnzipFilePath))
                            {
                                rar.Close();
                                bUncompressed = true;
                                bHaveNewFile = true;

                                DoCleanUp(strBasicFileName, strOrigFile, strTempUnzipFilePath, strTempFilePath, bDelAfterExtract);
                            }
                            else
                                strError.Format(L"Failed to extract %s file from RAR file \"%s\".", strBasicFileName, strTempFilePath);
                        }
                    }
                }
                if (!bHaveNewFile)
                    strError.Format(L"Downloaded %s file \"%s\" is a RAR file with unexpected content.", strBasicFileName, strTempFilePath);
                rar.Close();
            }
            else
            {
                strError.Format(L"Failed to open file \"%s\".\r\n\r\nInvalid file format?\r\n\r\nDownload latest version of UNRAR.DLL from http://www.rarlab.com and copy UNRAR.DLL into eMule installation folder.", strTempFilePath);
            }
        }
        else
        {
            CGZIPFile gz;
            if (gz.Open(strTempFilePath))
            {
                bIsArchiveFile = true;

                CString strTempUnzipFilePath = GetUnpackPath(strFilesToExtract.GetHead());

                // add filename and extension of uncompressed file to temporary file
                CString strUncompressedFileName = gz.GetUncompressedFileName();
                if (!strUncompressedFileName.IsEmpty())
                {
                    strTempUnzipFilePath += L'.';
                    strTempUnzipFilePath += strUncompressedFileName;
                }
                _tremove(strTempUnzipFilePath); //just to be sure

                if (gz.Extract(strTempUnzipFilePath))
                {
                    gz.Close();
                    bUncompressed = true;
                    bHaveNewFile = true;

                    DoCleanUp(strBasicFileName, strOrigFile, strTempUnzipFilePath, strTempFilePath, bDelAfterExtract);
                }
                else
                {
                    //do we need localization?
                    strError.Format(L"Failed to extract %s file from downloaded GZIP file \"%s\".", strBasicFileName, strTempFilePath);
                }
            }
            gz.Close();
        }
    }
    if (!strError.IsEmpty())
        //AfxMessageBox(strError, MB_ICONERROR);
        theApp.QueueLogLineEx(LOG_ERROR, strError);

    if (!bIsArchiveFile && !bUncompressed)
    {
        // Check first lines of downloaded file for potential HTML content (e.g. 404 error pages)
        bool bValidFile = true;
        FILE* fp = _tfsopen(strTempFilePath, L"rb", _SH_DENYWR);
        if (fp)
        {
            char szBuff[16384];
            int iRead = fread(szBuff, 1, _countof(szBuff)-1, fp);
            if (iRead <= 0)
                bValidFile = false;
            else
            {
                szBuff[iRead-1] = '\0';

                const char* pc = szBuff;
                while (*pc == ' ' || *pc == '\t' || *pc == '\r' || *pc == '\n')
                    ++pc;
                if (_strnicmp(pc, "<html", 5) == 0
                        || _strnicmp(pc, "<xml", 4) == 0
                        || _strnicmp(pc, "<!doc", 5) == 0)
                {
                    bValidFile = false;
                }
            }
            fclose(fp);
        }

        if (bValidFile)
        {
            DoCleanUp(strBasicFileName, strOrigFile, strTempFilePath, strTempFilePath, bDelAfterExtract);
            bHaveNewFile = true;
        }
        else
        {
            //do we need localization?
            strError.Format(L"%s download failed", strBasicFileName);
            //AfxMessageBox(strError, MB_ICONERROR);
            theApp.QueueLogLineEx(LOG_ERROR, strError);
        }
    }
    return bHaveNewFile;
}