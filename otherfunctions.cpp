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
#include <sys/stat.h>
#include <share.h>
#include <io.h>
#include <fcntl.h>
#if _MSC_VER>=1400
#pragma warning(disable:6385 6011)
#endif
#include <atlrx.h>
#if _MSC_VER>=1400
#pragma warning(default:6385 6011)
#endif
#include "emule.h"
#include "OtherFunctions.h"
#include "DownloadQueue.h"
#include "Preferences.h"
#include "PartFile.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "UpDownClient.h"
#include "Opcodes.h"
#include "WebServices.h"
#include <shlobj.h>
#include "emuledlg.h"
#include "MenuCmds.h"
#include "ZipFile.h"
#include "RarFile.h"
#include <atlbase.h>
#include "StringConversion.h"
#include "shahashset.h"
#include "collection.h"
#include "SafeFile.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "Log.h"
#include "CxImage/xImage.h"
#include "NetworkInfoDlg.h"
#include "HttpDownloadDlg.h"
#include "kademlia/routing/RoutingZone.h"
#include <IPHlpApi.h>
#include "./Mod/NetF/DesktopIntegration.h" //>>> WiZaRd::DesktopIntegration [Netfinity]
#ifdef INFO_WND
#include "./Mod/InfoWnd.h" //>>> WiZaRd::InfoWnd
#endif
#include <netioapi.h> //>>> WiZaRd::Find Best Interface IP [netfinity]
#include "MD5Sum.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern bool GetDRM(LPCTSTR pszFilePath);

CWebServices theWebServices;

// Base chars for encode an decode functions
static byte base16Chars[17] = "0123456789ABCDEF";
static byte base32Chars[33] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
#define BASE16_LOOKUP_MAX 23
static byte base16Lookup[BASE16_LOOKUP_MAX][2] =
{
    { '0', 0x0 },
    { '1', 0x1 },
    { '2', 0x2 },
    { '3', 0x3 },
    { '4', 0x4 },
    { '5', 0x5 },
    { '6', 0x6 },
    { '7', 0x7 },
    { '8', 0x8 },
    { '9', 0x9 },
    { ':', 0x9 },
    { ';', 0x9 },
    { '<', 0x9 },
    { '=', 0x9 },
    { '>', 0x9 },
    { '?', 0x9 },
    { '@', 0x9 },
    { 'A', 0xA },
    { 'B', 0xB },
    { 'C', 0xC },
    { 'D', 0xD },
    { 'E', 0xE },
    { 'F', 0xF }
};

CString CastItoXBytes(uint16 count, bool isK, bool isPerSec, UINT decimal)
{
    return CastItoXBytes((double)count, isK, isPerSec, decimal);
}

CString CastItoXBytes(UINT count, bool isK, bool isPerSec, UINT decimal)
{
    return CastItoXBytes((double)count, isK, isPerSec, decimal);
}

CString CastItoXBytes(uint64 count, bool isK, bool isPerSec, UINT decimal)
{
    return CastItoXBytes((double)count, isK, isPerSec, decimal);
}

#if defined(_DEBUG) && defined(USE_DEBUG_EMFILESIZE)
CString CastItoXBytes(EMFileSize count, bool isK, bool isPerSec, UINT decimal)
{
    return CastItoXBytes((double)count, isK, isPerSec, decimal);
}
#endif

CString CastItoXBytes(float count, bool isK, bool isPerSec, UINT decimal)
{
    return CastItoXBytes((double)count, isK, isPerSec, decimal);
}

CString CastItoXBytes(double count, bool isK, bool isPerSec, UINT decimal)
{
    if (count <= 0.0)
    {
        if (isPerSec)
        {
            if (thePrefs.GetForceSpeedsToKB())
                return L"0 " + GetResString(IDS_KBYTESPERSEC);
            else
                return L"0 " + GetResString(IDS_BYTESPERSEC);
        }
        else
            return L"0 " + GetResString(IDS_BYTES);
    }
    else if (isK)
    {
        if (count >  1.7E+300)
            count =  1.7E+300;
        else
            count *= 1024.0;
    }
    CString buffer;
    if (isPerSec)
    {
        if (thePrefs.GetForceSpeedsToKB())
            buffer.Format(L"%.*f %s", decimal, count/1024.0, GetResString(IDS_KBYTESPERSEC));
        else if (count < 1024.0)
            buffer.Format(L"%.0f %s", count, GetResString(IDS_BYTESPERSEC));
        else if (count < 1024000.0)
            buffer.Format(L"%.*f %s", decimal, count/1024.0, GetResString(IDS_KBYTESPERSEC));
        else if (count < 1048576000.0)
            buffer.Format(L"%.*f %s", decimal, count/1048576.0, GetResString(IDS_MBYTESPERSEC));
        else if (count < 1073741824000.0)
            buffer.Format(L"%.*f %s", decimal, count/1073741824.0, GetResString(IDS_GBYTESPERSEC));
        else
            buffer.Format(L"%.*f %s", decimal, count/1099511627776.0, GetResString(IDS_TBYTESPERSEC));
    }
    else
    {
        if (count < 1024.0)
            buffer.Format(L"%.0f %s", count, GetResString(IDS_BYTES));
        else if (count < 1024000.0)
            buffer.Format(L"%.*f %s", decimal, count/1024.0, GetResString(IDS_KBYTES));
        else if (count < 1048576000.0)
            buffer.Format(L"%.*f %s", decimal, count/1048576.0, GetResString(IDS_MBYTES));
        else if (count < 1073741824000.0)
            buffer.Format(L"%.*f %s", decimal, count/1073741824.0, GetResString(IDS_GBYTES));
        else
            buffer.Format(L"%.*f %s", decimal, count/1099511627776.0, GetResString(IDS_TBYTES));
    }
    return buffer;
}

CString CastItoIShort(uint16 count, bool isK, UINT decimal)
{
    return CastItoIShort((double)count, isK, decimal);
}

CString CastItoIShort(UINT count, bool isK, UINT decimal)
{
    return CastItoIShort((double)count, isK, decimal);
}

CString CastItoIShort(uint64 count, bool isK, UINT decimal)
{
    return CastItoIShort((double)count, isK, decimal);
}

CString CastItoIShort(float count, bool isK, UINT decimal)
{
    return CastItoIShort((double)count, isK, decimal);
}

CString CastItoIShort(double count, bool isK, UINT decimal)
{
    if (count <= 0.0)
    {
        return L"0";
    }
    else if (isK)
    {
        if (count >  1.7E+300)
            count =  1.7E+300;
        else
            count *= 1000.0;
    }
    CString output;
    if (count < 1000.0)
        output.Format(L"%.0f", count);
    else if (count < 1000000.0)
        output.Format(L"%.*f %s", decimal, count/1000.0, GetResString(IDS_KILO));
    else if (count < 1000000000.0)
        output.Format(L"%.*f %s", decimal, count/1000000.0, GetResString(IDS_MEGA));
    else if (count < 1000000000000.0)
        output.Format(L"%.*f %s", decimal, count/1000000000.0, GetResString(IDS_GIGA));
    else if (count < 1000000000000000.0)
        output.Format(L"%.*f %s", decimal, count/1000000000000.0, GetResString(IDS_TERRA));
    return output;
}

CString CastSecondsToHM(time_t tSeconds)
{
    if (tSeconds == -1)	// invalid or unknown time value
        return L"?";

    CString buffer;
    UINT count = tSeconds;
    if (count < 60)
        buffer.Format(L"%u %s", count, GetResString(IDS_SECS));
    else if (count < 3600)
        buffer.Format(L"%u:%02u %s", count/60, count - (count/60)*60, GetResString(IDS_MINS));
    else if (count < 86400)
        buffer.Format(L"%u:%02u %s", count/3600, (count - (count/3600)*3600)/60, GetResString(IDS_HOURS));
    else
    {
        UINT cntDays = count/86400;
        UINT cntHrs = (count - cntDays*86400)/3600;
        buffer.Format(L"%u %s %u %s", cntDays, GetResString(IDS_DAYS), cntHrs, GetResString(IDS_HOURS));
    }
    return buffer;
}

CString CastSecondsToLngHM(time_t tSeconds)
{
    if (tSeconds == -1) // invalid or unknown time value
        return L"?";

    CString buffer;
    UINT count = tSeconds;
    if (count < 60)
        buffer.Format(L"%u %s", count, GetResString(IDS_LONGSECS));
    else if (count < 3600)
        buffer.Format(L"%u:%02u %s", count/60, count - (count/60)*60, GetResString(IDS_LONGMINS));
    else if (count < 86400)
        buffer.Format(L"%u:%02u %s", count/3600, (count - (count/3600)*3600)/60, GetResString(IDS_LONGHRS));
    else
    {
        UINT cntDays = count/86400;
        UINT cntHrs = (count - cntDays*86400)/3600;
        if (cntHrs)
            buffer.Format(L"%u %s %u:%02u %s", cntDays, GetResString(IDS_DAYS2), cntHrs, (count - (cntDays*86400) - (cntHrs*3600))/60, GetResString(IDS_LONGHRS));
        else
            buffer.Format(L"%u %s %u %s", cntDays, GetResString(IDS_DAYS2), (count - (cntDays*86400) - (cntHrs*3600))/60, GetResString(IDS_LONGMINS));
    }
    return buffer;
}

bool CheckFileOpen(LPCTSTR pszFilePath, LPCTSTR pszFileTitle)
{
    if (thePrefs.GetCheckFileOpen() && !PathIsURL(pszFilePath) && GetDRM(pszFilePath))
    {
        CString strWarning;
        strWarning.Format(GetResString(IDS_FILE_WARNING_DRM), pszFileTitle != NULL ? pszFileTitle : pszFilePath);
        int iAnswer = AfxMessageBox(strWarning, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
        if (iAnswer != IDYES)
            return false;
    }
    return true;
}

void ShellOpenFile(CString name)
{
    ShellOpenFile(name, L"open");
}

void ShellOpenFile(CString name, LPCTSTR pszVerb)
{
    if ((pszVerb == NULL || pszVerb[0] == L'\0' || _tcsicmp(pszVerb, L"open")) && !CheckFileOpen(name, NULL))
        return;
    _ShellExecute(NULL, pszVerb, name, NULL, NULL, SW_SHOW);
}

bool ShellDeleteFile(LPCTSTR pszFilePath)
{
    if (!PathFileExists(pszFilePath))
        return true;
    if (!thePrefs.GetRemoveToBin())
        return (DeleteFile(pszFilePath) != FALSE);

    TCHAR todel[MAX_PATH + 1];
    memset(todel, 0, sizeof todel);
    _tcsncpy(todel, pszFilePath, _countof(todel) - 2);

    SHFILEOPSTRUCT fp = {0};
    fp.wFunc = FO_DELETE;
    fp.hwnd = theApp.emuledlg->m_hWnd;
    fp.pFrom = todel;
    fp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NORECURSION;
    bool bResult = false;
    __try
    {
        // Win98/MSLU: This seems to crash always in UNICOWS.DLL !?
        bResult = (SHFileOperation(&fp) == 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        bResult = (DeleteFile(pszFilePath) != FALSE);
    }
    return bResult;
}

CString ShellGetFolderPath(int iCSIDL)
{
    CString strFolderPath;

    // Try the Unicode version from "shell32" *and* examine the function result - just the presence of that
    // function does not mean that it returns the requested path.
    //
    // Win98: 'SHGetFolderPathW' is available in 'shell32.dll', but it does not support all of the CSIDL values.
    HRESULT(WINAPI *pfnSHGetFolderPathW)(HWND, int, HANDLE, DWORD, LPWSTR);
    (FARPROC&)pfnSHGetFolderPathW = GetProcAddress(GetModuleHandle(L"shell32"), "SHGetFolderPathW");
    if (pfnSHGetFolderPathW)
    {
        WCHAR wszPath[MAX_PATH];
        if ((*pfnSHGetFolderPathW)(NULL, iCSIDL, NULL, SHGFP_TYPE_CURRENT, wszPath) == S_OK)
            strFolderPath = wszPath;
    }

    if (strFolderPath.IsEmpty())
    {
        HMODULE hLibShFolder = LoadLibrary(L"shfolder.dll");
        if (hLibShFolder)
        {
            HRESULT(WINAPI *pfnSHGetFolderPathW)(HWND, int, HANDLE, DWORD, LPWSTR);
            (FARPROC&)pfnSHGetFolderPathW = GetProcAddress(hLibShFolder, "SHGetFolderPathW");
            if (pfnSHGetFolderPathW)
            {
                WCHAR wszPath[MAX_PATH];
                if ((*pfnSHGetFolderPathW)(NULL, iCSIDL, NULL, SHGFP_TYPE_CURRENT, wszPath) == S_OK)
                    strFolderPath = wszPath;
            }

            if (strFolderPath.IsEmpty())
            {
                HRESULT(WINAPI *pfnSHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPSTR);
                (FARPROC&)pfnSHGetFolderPathA = GetProcAddress(hLibShFolder, "SHGetFolderPathA");
                if (pfnSHGetFolderPathA)
                {
                    CHAR aszPath[MAX_PATH];
                    if ((*pfnSHGetFolderPathA)(NULL, iCSIDL, NULL, SHGFP_TYPE_CURRENT, aszPath) == S_OK)
                        strFolderPath = CString(aszPath);
                }
            }
            FreeLibrary(hLibShFolder);
        }
    }

    return strFolderPath;
}

namespace
{
bool IsHexDigit(int c)
{
    switch (c)
    {
        case '0':
            return true;
        case '1':
            return true;
        case '2':
            return true;
        case '3':
            return true;
        case '4':
            return true;
        case '5':
            return true;
        case '6':
            return true;
        case '7':
            return true;
        case '8':
            return true;
        case '9':
            return true;
        case 'A':
            return true;
        case 'B':
            return true;
        case 'C':
            return true;
        case 'D':
            return true;
        case 'E':
            return true;
        case 'F':
            return true;
        case 'a':
            return true;
        case 'b':
            return true;
        case 'c':
            return true;
        case 'd':
            return true;
        case 'e':
            return true;
        case 'f':
            return true;
        default:
            return false;
    }
}
}

CString URLDecode(const CString& inStr, bool bKeepNewLine)
{
    // decode escape sequences
    CString res;
    for (int x = 0; x < inStr.GetLength(); x++)
    {
        if (inStr.GetAt(x) == _T('%') && x + 2 < inStr.GetLength() && IsHexDigit(inStr.GetAt(x+1)) && IsHexDigit(inStr.GetAt(x+2)))
        {
            TCHAR hexstr[3];
            _tcsncpy(hexstr, inStr.Mid(x+1, 2), 2);
            hexstr[2] = L'\0';
            x += 2;

            // Convert the hex to ASCII
            TCHAR ch = (TCHAR)_tcstoul(hexstr, NULL, 16);
            if (ch > '\x1F' || (bKeepNewLine && ch == '\x0A')) // filter control chars
                res.AppendChar(ch);
        }
        else
        {
            res.AppendChar(inStr.GetAt(x));
        }
    }
    return res;
}

CString URLEncode(const CString& sInT)
{
    CStringA sIn(sInT);
    LPCSTR pInBuf = sIn;

    CString sOut;
    LPTSTR pOutBuf = sOut.GetBuffer(sIn.GetLength() * 3);
    if (pOutBuf)
    {
        // do encoding
        while (*pInBuf)
        {
            if (_ismbcalnum((BYTE)*pInBuf))
                *pOutBuf++ = (BYTE)*pInBuf;
            else
            {
                *pOutBuf++ = _T('%');
                *pOutBuf++ = toHex((BYTE)*pInBuf >> 4);
                *pOutBuf++ = toHex((BYTE)*pInBuf % 16);
            }
            pInBuf++;
        }
        *pOutBuf = L'\0';
        sOut.ReleaseBuffer();
    }
    return sOut;
}

CString EncodeURLQueryParam(const CString& sInT)
{
    CStringA sIn(sInT);
    LPCSTR pInBuf = sIn;

    // query         = *uric
    // uric          = reserved | unreserved | escaped
    // reserved      = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" | "$" | ","
    // unreserved    = alphanum | mark
    // mark          = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"
    //
    // See also: http://www.w3.org/MarkUp/html-spec/html-spec_8.html

    CString sOut;
    LPTSTR pOutBuf = sOut.GetBuffer(sIn.GetLength() * 3);
    if (pOutBuf)
    {
        // do encoding
        while (*pInBuf)
        {
            if (_ismbcalnum((BYTE)*pInBuf))
                *pOutBuf++ = (BYTE)*pInBuf;
            else if (_ismbcspace((BYTE)*pInBuf))
                *pOutBuf++ = _T('+');
            else
            {
                *pOutBuf++ = _T('%');
                *pOutBuf++ = toHex((BYTE)*pInBuf >> 4);
                *pOutBuf++ = toHex((BYTE)*pInBuf % 16);
            }
            pInBuf++;
        }
        *pOutBuf = L'\0';
        sOut.ReleaseBuffer();
    }
    return sOut;
}

CString MakeStringEscaped(CString in)
{
    in.Replace(L"&", L"&&");
    return in;
}

CString RemoveAmpersand(const CString& rstr)
{
    CString str(rstr);
    str.Remove(_T('&'));
    return str;
}

bool Ask4RegFix(bool checkOnly, bool dontAsk, bool bAutoTakeCollections)
{
    // Barry - Make backup first
    if (!checkOnly)
        BackupReg();

    bool bGlobalSet = false;
    CRegKey regkey;
    LONG result;
    TCHAR modbuffer[MAX_PATH];
    DWORD dwModPathLen = ::GetModuleFileName(NULL, modbuffer, _countof(modbuffer));
    if (dwModPathLen == 0 || dwModPathLen == _countof(modbuffer))
        return false;
    CString strCanonFileName = modbuffer;
    strCanonFileName.Replace(L"%", L"%%");
    CString regbuffer;
    regbuffer.Format(L"\"%s\" \"%%1\"", strCanonFileName);

    // first check if the registry keys are already set (either by installer in HKLM or by user in HKCU)
    result = regkey.Open(HKEY_CLASSES_ROOT, L"ed2k\\shell\\open\\command", KEY_READ);
    if (result == ERROR_SUCCESS)
    {
        TCHAR rbuffer[MAX_PATH + 100];
        ULONG maxsize = _countof(rbuffer);
        regkey.QueryStringValue(NULL, rbuffer, &maxsize);
        rbuffer[_countof(rbuffer) - 1] = L'\0';
        if (maxsize != 0 && _tcsicmp(rbuffer, regbuffer) == 0)
            bGlobalSet = true; // yup, globally we have an entry for this mule
        regkey.Close();
    }

    if (!bGlobalSet)
    {
        // we actually need to change the registry and write an entry for HKCU
        if (checkOnly)
            return true;
        HKEY hkeyCR = thePrefs.GetWindowsVersion() < _WINVER_2K_ ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        if (regkey.Create(hkeyCR, L"Software\\Classes\\ed2k\\shell\\open\\command") == ERROR_SUCCESS)
        {
            if (dontAsk || (AfxMessageBox(GetResString(IDS_ASSIGNED2K), MB_ICONQUESTION|MB_YESNO) == IDYES))
            {
                VERIFY(regkey.SetStringValue(NULL, regbuffer) == ERROR_SUCCESS);

                VERIFY(regkey.Create(hkeyCR, L"Software\\Classes\\ed2k\\DefaultIcon") == ERROR_SUCCESS);
                VERIFY(regkey.SetStringValue(NULL, modbuffer) == ERROR_SUCCESS);

                VERIFY(regkey.Create(hkeyCR, L"Software\\Classes\\ed2k") == ERROR_SUCCESS);
                VERIFY(regkey.SetStringValue(NULL, L"URL: ed2k Protocol") == ERROR_SUCCESS);
                VERIFY(regkey.SetStringValue(L"URL Protocol", L"") == ERROR_SUCCESS);

                VERIFY(regkey.Open(hkeyCR, L"Software\\Classes\\ed2k\\shell\\open") == ERROR_SUCCESS);
                regkey.RecurseDeleteKey(L"ddexec");
                regkey.RecurseDeleteKey(L"ddeexec");
            }
            regkey.Close();
        }
        else
            ASSERT(0);
    }
    else if (checkOnly)
        return bAutoTakeCollections && DoCollectionRegFix(true);
    else if (bAutoTakeCollections)
        DoCollectionRegFix(false);

    return false;
}

void BackupReg(void)
{
    // TODO: This function needs to be changed in at least 2 regards
    //	1)	It must follow the rules: reading from HKCR and writing into HKCU. What we are currently doing
    //		is not consistent with the other registry function which are dealing with the same keys.
    //
    //	2)	It behavious quite(!) differently under Win98 due to an obvious change in the Windows API.
    //		WinXP: Reading an non existant value returns 'key not found' error.
    //		Win98: Reading an non existant value returns an empty string (which gets saved and restored by our code).
    //		This means that saving/restoring existant registry keys works completely different in Win98/XP.
    //		Actually it works correctly under Win98 and is broken in WinXP+. Though, did someone notice it all ?

    HKEY hkeyCR = thePrefs.GetWindowsVersion() < _WINVER_2K_ ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    // Look for pre-existing old ed2k links
    CRegKey regkey;
    if (regkey.Create(hkeyCR, L"Software\\Classes\\ed2k\\shell\\open\\command") == ERROR_SUCCESS)
    {
        TCHAR rbuffer[MAX_PATH + 100];
        ULONG maxsize = _countof(rbuffer);
        if (regkey.QueryStringValue(L"OldDefault", rbuffer, &maxsize) != ERROR_SUCCESS || maxsize == 0)
        {
            maxsize = _countof(rbuffer);
            if (regkey.QueryStringValue(NULL, rbuffer, &maxsize) == ERROR_SUCCESS)
                VERIFY(regkey.SetStringValue(L"OldDefault", rbuffer) == ERROR_SUCCESS);

            VERIFY(regkey.Create(hkeyCR, L"Software\\Classes\\ed2k\\DefaultIcon") == ERROR_SUCCESS);
            maxsize = _countof(rbuffer);
            if (regkey.QueryStringValue(NULL, rbuffer, &maxsize) == ERROR_SUCCESS)
                VERIFY(regkey.SetStringValue(L"OldIcon", rbuffer) == ERROR_SUCCESS);
        }
        regkey.Close();
    }
    else
        ASSERT(0);
}

// Barry - Restore previous values
void RevertReg(void)
{
    HKEY hkeyCR = thePrefs.GetWindowsVersion() < _WINVER_2K_ ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    // restore previous ed2k links before being assigned to emule
    CRegKey regkey;
    if (regkey.Create(hkeyCR, L"Software\\Classes\\ed2k\\shell\\open\\command") == ERROR_SUCCESS)
    {
        TCHAR rbuffer[MAX_PATH + 100];
        ULONG maxsize = _countof(rbuffer);
        if (regkey.QueryStringValue(L"OldDefault", rbuffer, &maxsize) == ERROR_SUCCESS)
        {
            VERIFY(regkey.SetStringValue(NULL, rbuffer) == ERROR_SUCCESS);
            VERIFY(regkey.DeleteValue(L"OldDefault") == ERROR_SUCCESS);

            VERIFY(regkey.Create(hkeyCR, L"Software\\Classes\\ed2k\\DefaultIcon") == ERROR_SUCCESS);
            maxsize = _countof(rbuffer);
            if (regkey.QueryStringValue(L"OldIcon", rbuffer, &maxsize) == ERROR_SUCCESS)
            {
                VERIFY(regkey.SetStringValue(NULL, rbuffer) == ERROR_SUCCESS);
                VERIFY(regkey.DeleteValue(L"OldIcon") == ERROR_SUCCESS);
            }
        }
        regkey.Close();
    }
    else
        ASSERT(0);
}

int GetMaxWindowsTCPConnections()
{
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx((OSVERSIONINFO*)&osvi))
    {
        //if OSVERSIONINFOEX doesn't work, try OSVERSIONINFO
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (!GetVersionEx((OSVERSIONINFO*)&osvi))
            return -1;  //shouldn't ever happen
    }

    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) // Windows NT product family
        return -1;  //no limits

    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)  // Windows 95 product family
    {

        if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)   //old school 95
        {
            HKEY hKey;
            DWORD dwValue;
            DWORD dwLength = sizeof(dwValue);
            LONG lResult;

            RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\VxD\\MSTCP",
                         0, KEY_QUERY_VALUE, &hKey);
            lResult = RegQueryValueEx(hKey, TEXT("MaxConnections"), NULL, NULL,
                                      (LPBYTE)&dwValue, &dwLength);
            RegCloseKey(hKey);

            if (lResult != ERROR_SUCCESS || lResult < 1)
                return 100;  //the default for 95 is 100

            return dwValue;

        }
        else     //98 or ME
        {
            HKEY hKey;
            TCHAR szValue[32];
            DWORD dwLength = sizeof(szValue);
            LONG lResult;

            RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\VxD\\MSTCP",
                         0, KEY_QUERY_VALUE, &hKey);
            lResult = RegQueryValueEx(hKey, TEXT("MaxConnections"), NULL, NULL,
                                      (LPBYTE)szValue, &dwLength);
            RegCloseKey(hKey);

            LONG lMaxConnections;
            if (lResult != ERROR_SUCCESS || (lMaxConnections = _tstoi(szValue)) < 1)
                return 100;  //the default for 98/ME is 100

            return lMaxConnections;
        }
    }

    return -1;  //give the user the benefit of the doubt, most use NT+ anyway
}

WORD DetectWinVersion()
{
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx((OSVERSIONINFO*)&osvi))
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (!GetVersionEx((OSVERSIONINFO*)&osvi))
            return FALSE;
    }

    switch (osvi.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_NT:
            if (osvi.dwMajorVersion <= 4)
                return _WINVER_NT4_;
            if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
                return _WINVER_2K_;
            if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
                return _WINVER_XP_;
            if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
                return _WINVER_2003_;
            if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
                return _WINVER_VISTA_;
            if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
                return _WINVER_7_;
            if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
                return _WINVER_8_;
            return _WINVER_7_; // never return Win95 if we get the info about a NT system

        case VER_PLATFORM_WIN32_WINDOWS:
            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
                return _WINVER_95_;
            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
                return _WINVER_98_;
            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
                return _WINVER_ME_;
            break;
    }

    return _WINVER_95_;		// there should'nt be anything lower than this
}

int IsRunningXPSP2()
{
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx((OSVERSIONINFO*)&osvi))
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (!GetVersionEx((OSVERSIONINFO*)&osvi))
            return 0;
    }

    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
    {
        if (osvi.wServicePackMajor >= 2)
            return 1;
    }
    return 0;
}

int IsRunningXPSP2OrHigher()
{
    switch (thePrefs.GetWindowsVersion())
    {
        case _WINVER_95_:
        case _WINVER_98_:
        case _WINVER_NT4_:
        case _WINVER_2K_:
        case _WINVER_ME_:
            return 0;
            break;
        case _WINVER_XP_:
            return IsRunningXPSP2();
            break;
        default:
            return 1;
    }
}

uint64 GetFreeDiskSpaceX(LPCTSTR pDirectory)
{
    extern bool g_bUnicoWS;
    static BOOL _bInitialized = FALSE;
    static BOOL (WINAPI *_pfnGetDiskFreeSpaceEx)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER) = NULL;
    static BOOL (WINAPI *_pfnGetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER) = NULL;

    if (!_bInitialized)
    {
        _bInitialized = TRUE;
        if (g_bUnicoWS)
            (FARPROC&)_pfnGetDiskFreeSpaceExA = GetProcAddress(GetModuleHandle(L"kernel32"), "GetDiskFreeSpaceExA");
        else
            (FARPROC&)_pfnGetDiskFreeSpaceEx = GetProcAddress(GetModuleHandle(L"kernel32"), _TWINAPI("GetDiskFreeSpaceEx"));
    }

    if (_pfnGetDiskFreeSpaceEx)
    {
        ULARGE_INTEGER nFreeDiskSpace;
        ULARGE_INTEGER dummy;
        if ((*_pfnGetDiskFreeSpaceEx)(pDirectory, &nFreeDiskSpace, &dummy, &dummy)==TRUE)
            return nFreeDiskSpace.QuadPart;
        else
            return 0;
    }
    else if (_pfnGetDiskFreeSpaceExA)
    {
        ULARGE_INTEGER nFreeDiskSpace;
        ULARGE_INTEGER dummy;
        if ((*_pfnGetDiskFreeSpaceExA)(CT2CA(pDirectory), &nFreeDiskSpace, &dummy, &dummy)==TRUE)
            return nFreeDiskSpace.QuadPart;
        else
            return 0;
    }
    else
    {
        TCHAR cDrive[MAX_PATH];
        const TCHAR *p = _tcschr(pDirectory, _T('\\'));
        if (p)
        {
            size_t uChars = p - pDirectory;
            if (uChars >= _countof(cDrive))
                return 0;
            memcpy(cDrive, pDirectory, uChars * sizeof(TCHAR));
            cDrive[uChars] = L'\0';
        }
        else
        {
            if (_tcslen(pDirectory) >= _countof(cDrive))
                return 0;
            _tcscpy(cDrive, pDirectory);
        }
        DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwDummy;
        if (GetDiskFreeSpace(cDrive, &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwDummy))
            return (ULONGLONG)dwFreeClusters * (ULONGLONG)dwSectPerClust * (ULONGLONG)dwBytesPerSect;
        else
            return 0;
    }
}

CString GetRateString(UINT rate)
{
    switch (rate)
    {
        case 0:
            return GetResString(IDS_CMT_NOTRATED);
        case 1:
            return GetResString(IDS_CMT_FAKE);
        case 2:
            return GetResString(IDS_CMT_POOR);
        case 3:
            return GetResString(IDS_CMT_FAIR);
        case 4:
            return GetResString(IDS_CMT_GOOD);
        case 5:
            return GetResString(IDS_CMT_EXCELLENT);
    }
    return GetResString(IDS_CMT_NOTRATED);
}

// Returns a BASE32 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Lenght of buffer array
//
// [Return]
//   CString object with BASE32 encoded byte array
CString EncodeBase32(const unsigned char* buffer, unsigned int bufLen)
{
    CString Base32Buff;

    unsigned int i, index;
    unsigned char word;

    for (i = 0, index = 0; i < bufLen;)
    {

        // Is the current word going to span a byte boundary?
        if (index > 3)
        {
            word = (BYTE)(buffer[i] & (0xFF >> index));
            index = (index + 5) % 8;
            word <<= index;
            if (i < bufLen - 1)
                word |= buffer[i + 1] >> (8 - index);

            i++;
        }
        else
        {
            word = (BYTE)((buffer[i] >> (8 - (index + 5))) & 0x1F);
            index = (index + 5) % 8;
            if (index == 0)
                i++;
        }

        Base32Buff += (char) base32Chars[word];
    }

    return Base32Buff;
}

// Returns a BASE16 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Lenght of buffer array
//
// [Return]
//   CString object with BASE16 encoded byte array
CString EncodeBase16(const unsigned char* buffer, unsigned int bufLen)
{
    CString Base16Buff;

    for (unsigned int i = 0; i < bufLen; i++)
    {
        Base16Buff += base16Chars[buffer[i] >> 4];
        Base16Buff += base16Chars[buffer[i] & 0xf];
    }

    return Base16Buff;
}

// Decodes a BASE16 string into a byte array
//
// [In]
//   base16Buffer: String containing BASE16
//   base16BufLen: Lenght BASE16 coded string's length
//
// [Out]
//   buffer: byte array containing decoded string
bool DecodeBase16(const TCHAR *base16Buffer, unsigned int base16BufLen, byte *buffer, unsigned int bufflen)
{
    unsigned int uDecodeLengthBase16 = DecodeLengthBase16(base16BufLen);
    if (uDecodeLengthBase16 > bufflen)
        return false;
    memset(buffer, 0, uDecodeLengthBase16);

    for (unsigned int i = 0; i < base16BufLen; i++)
    {
        int lookup = _totupper((_TUCHAR)base16Buffer[i]) - _T('0');

        // Check to make sure that the given word falls inside a valid range
        byte word = 0;

        if (lookup < 0 || lookup >= BASE16_LOOKUP_MAX)
            word = 0xFF;
        else
            word = base16Lookup[lookup][1];

        if (i % 2 == 0)
        {
            buffer[i/2] = word << 4;
        }
        else
        {
            buffer[(i-1)/2] |= word;
        }
    }
    return true;
}

// Calculates length to decode from BASE16
//
// [In]
//   base16Length: Actual length of BASE16 string
//
// [Return]
//   New length of byte array decoded
unsigned int DecodeLengthBase16(unsigned int base16Length)
{
    return base16Length / 2U;
}

UINT DecodeBase32(LPCTSTR pszInput, uchar* paucOutput, UINT nBufferLen)
{
    if (pszInput == NULL)
        return false;
    UINT nDecodeLen = (_tcslen(pszInput)*5)/8;
    if ((_tcslen(pszInput)*5) % 8 > 0)
        nDecodeLen++;
    UINT nInputLen = _tcslen(pszInput);
    if (paucOutput == NULL || nBufferLen == 0)
        return nDecodeLen;
    if (nDecodeLen > nBufferLen || paucOutput == NULL)
        return 0;

    DWORD nBits	= 0;
    int nCount	= 0;

    for (int nChars = nInputLen ; nChars-- ; pszInput++)
    {
        if (*pszInput >= 'A' && *pszInput <= 'Z')
            nBits |= (*pszInput - 'A');
        else if (*pszInput >= 'a' && *pszInput <= 'z')
            nBits |= (*pszInput - 'a');
        else if (*pszInput >= '2' && *pszInput <= '7')
            nBits |= (*pszInput - '2' + 26);
        else
            return 0;

        nCount += 5;

        if (nCount >= 8)
        {
            *paucOutput++ = (BYTE)(nBits >> (nCount - 8));
            nCount -= 8;
        }

        nBits <<= 5;
    }

    return nDecodeLen;
}

UINT DecodeBase32(LPCTSTR pszInput, CAICHHash& Hash)
{
    return DecodeBase32(pszInput, Hash.GetRawHash(), Hash.GetHashSize());
}

CWebServices::CWebServices()
{
    m_tDefServicesFileLastModified = 0;
}

CString CWebServices::GetDefaultServicesFile() const
{
    return thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + L"webservices.dat";
}

void CWebServices::RemoveAllServices()
{
    m_aServices.RemoveAll();
    m_tDefServicesFileLastModified = 0;
}

int CWebServices::ReadAllServices()
{
    RemoveAllServices();

    CString strFilePath = GetDefaultServicesFile();
    FILE* readFile = _tfsopen(strFilePath, L"r", _SH_DENYWR);
    if (readFile != NULL)
    {
        CString name, url, sbuffer;
        while (!feof(readFile))
        {
            TCHAR buffer[1024];
            if (_fgetts(buffer, _countof(buffer), readFile) == NULL)
                break;
            sbuffer = buffer;

            // ignore comments & too short lines
            if (sbuffer.GetAt(0) == _T('#') || sbuffer.GetAt(0) == _T('/') || sbuffer.GetLength() < 5)
                continue;

            int iPos = sbuffer.Find(_T(','));
            if (iPos > 0)
            {
                CString strUrlTemplate = sbuffer.Right(sbuffer.GetLength() - iPos - 1).Trim();
                if (!strUrlTemplate.IsEmpty())
                {
                    bool bFileMacros = false;
                    static const LPCTSTR _apszMacros[] =
                    {
                        L"#hashid",
                        L"#filesize",
                        L"#filename",
                        L"#name",
                        L"#cleanfilename",
                        L"#cleanname"
                    };
                    for (int i = 0; i < _countof(_apszMacros); i++)
                    {
                        if (strUrlTemplate.Find(_apszMacros[i]) != -1)
                        {
                            bFileMacros = true;
                            break;
                        }
                    }

                    SEd2kLinkService svc;
                    svc.uMenuID = MP_WEBURL + m_aServices.GetCount();
                    svc.strMenuLabel = sbuffer.Left(iPos).Trim();
                    svc.strUrl = strUrlTemplate;
                    svc.bFileMacros = bFileMacros;
                    m_aServices.Add(svc);
                }
            }
        }
        fclose(readFile);

        struct _stat32i64 fileinfo;
        if(_tstat32i64(strFilePath, &fileinfo) == 0)
            m_tDefServicesFileLastModified = fileinfo.st_mtime;
    }

    return m_aServices.GetCount();
}

int CWebServices::GetAllMenuEntries(CTitleMenu* pMenu, DWORD dwFlags)
{
    if (m_aServices.GetCount() == 0)
        ReadAllServices();
    else
    {
		struct _stat32i64 fileinfo;
        if (_tstat32i64(GetDefaultServicesFile(), &fileinfo) == 0 && fileinfo.st_mtime > m_tDefServicesFileLastModified)
            ReadAllServices();
    }

    int iMenuEntries = 0;
    for (int i = 0; i < m_aServices.GetCount(); i++)
    {
        const SEd2kLinkService& rSvc = m_aServices.GetAt(i);
        if ((dwFlags & WEBSVC_GEN_URLS) && rSvc.bFileMacros)
            continue;
        if ((dwFlags & WEBSVC_FILE_URLS) && !rSvc.bFileMacros)
            continue;
        if (pMenu->AppendMenu(MF_STRING, MP_WEBURL + i, rSvc.strMenuLabel, L"WEB"))
            iMenuEntries++;
    }
    return iMenuEntries;
}

bool CWebServices::RunURL(const CAbstractFile* file, UINT uMenuID)
{
    for (int i = 0; i < m_aServices.GetCount(); i++)
    {
        const SEd2kLinkService& rSvc = m_aServices.GetAt(i);
        if (rSvc.uMenuID == uMenuID)
        {
            CString strUrlTemplate = rSvc.strUrl;
            if (file != NULL)
            {
                // Convert hash to hexadecimal text and add it to the URL
                strUrlTemplate.Replace(L"#hashid", md4str(file->GetFileHash()));

                // Add file size to the URL
                CString temp;
                temp.Format(L"%I64u", file->GetFileSize());
                strUrlTemplate.Replace(L"#filesize", temp);

                // add complete filename to the url
                strUrlTemplate.Replace(L"#filename", EncodeURLQueryParam(file->GetFileName()));

                // add basename to the url
                CString strBaseName = file->GetFileName();
                PathRemoveExtension(strBaseName.GetBuffer(strBaseName.GetLength()));
                strBaseName.ReleaseBuffer();
                strUrlTemplate.Replace(L"#name", EncodeURLQueryParam(strBaseName));

                // add cleaned up complete filename to the url
                strUrlTemplate.Replace(L"#cleanfilename", EncodeURLQueryParam(CleanupFilename(file->GetFileName())));

                // add cleaned up basename to the url
                strUrlTemplate.Replace(L"#cleanname", EncodeURLQueryParam(CleanupFilename(strBaseName, false)));
            }

            // Open URL
            TRACE(L"Starting URL: %s\n", strUrlTemplate);
            return (int)_ShellExecute(NULL, NULL, strUrlTemplate, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT) > 32;
        }
    }
    return false;
}

void CWebServices::Edit()
{
    _ShellExecute(NULL, L"open", thePrefs.GetTxtEditor(), L"\"" + thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + L"webservices.dat\"", NULL, SW_SHOW);
}

typedef struct
{
    LPCTSTR	pszInitialDir;
    LPCTSTR	pszDlgTitle;
} BROWSEINIT, *LPBROWSEINIT;

extern "C" int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        // Set initial directory
        if (((LPBROWSEINIT)lpData)->pszInitialDir != NULL)
            SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)((LPBROWSEINIT)lpData)->pszInitialDir);

        // Set dialog's window title
        if (((LPBROWSEINIT)lpData)->pszDlgTitle != NULL)
            SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)((LPBROWSEINIT)lpData)->pszDlgTitle);
    }

    return 0;
}

bool SelectDir(HWND hWnd, LPTSTR pszPath, LPCTSTR pszTitle, LPCTSTR pszDlgTitle)
{
    bool bResult = false;
    CoInitialize(0);
    LPMALLOC pShlMalloc;
    if (SHGetMalloc(&pShlMalloc) == NOERROR)
    {
        BROWSEINFO BrsInfo = {0};
        BrsInfo.hwndOwner = hWnd;
        BrsInfo.lpszTitle = (pszTitle != NULL) ? pszTitle : pszDlgTitle;
        BrsInfo.ulFlags = BIF_VALIDATE | BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_SHAREABLE | BIF_DONTGOBELOWDOMAIN;

        BROWSEINIT BrsInit = {0};
        if (pszPath != NULL || pszTitle != NULL || pszDlgTitle != NULL)
        {
            // Need the 'BrowseCallbackProc' to set those strings
            BrsInfo.lpfn = BrowseCallbackProc;
            BrsInfo.lParam = (LPARAM)&BrsInit;
            BrsInit.pszDlgTitle = (pszDlgTitle != NULL) ? pszDlgTitle : NULL/*pszTitle*/;
            BrsInit.pszInitialDir = pszPath;
        }

        LPITEMIDLIST pidlBrowse;
        if ((pidlBrowse = SHBrowseForFolder(&BrsInfo)) != NULL)
        {
            if (SHGetPathFromIDList(pidlBrowse, pszPath))
                bResult = true;
            pShlMalloc->Free(pidlBrowse);
        }
        pShlMalloc->Release();
    }
    CoUninitialize();
    return bResult;
}

void MakeFoldername(CString &rstrPath)
{
    if (!rstrPath.IsEmpty()) // don't canonicalize an empty path, we would get a "\"
    {
        CString strNewPath;
        LPTSTR pszNewPath = strNewPath.GetBuffer(MAX_PATH);
        PathCanonicalize(pszNewPath, rstrPath);
        PathRemoveBackslash(pszNewPath);
        strNewPath.ReleaseBuffer();
        rstrPath = strNewPath;
    }
}

CString StringLimit(CString in, UINT length)
{
    if ((UINT)in.GetLength() <= length || length < 10)
        return in;
    return (in.Left(length-8) + L"..." + in.Right(8));
}

BOOL DialogBrowseFile(CString& rstrPath, LPCTSTR pszFilters, LPCTSTR pszDefaultFileName, DWORD dwFlags,bool openfilestyle)
{
    CFileDialog myFileDialog(openfilestyle,NULL,pszDefaultFileName,
                             dwFlags | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, pszFilters, NULL,
                             0/*automatically use explorer style open dialog on systems which support it*/);
    if (myFileDialog.DoModal() != IDOK)
        return FALSE;
    rstrPath = myFileDialog.GetPathName();
    return TRUE;
}

void md4str(const uchar* hash, TCHAR* pszHash)
{
    static const TCHAR _acHexDigits[] = L"0123456789ABCDEF";
    for (int i = 0; i < 16; i++)
    {
        *pszHash++ = _acHexDigits[hash[i] >> 4];
        *pszHash++ = _acHexDigits[hash[i] & 0xf];
    }
    *pszHash = L'\0';
}

CString md4str(const uchar* hash)
{
    TCHAR szHash[MAX_HASHSTR_SIZE];
    md4str(hash, szHash);
    return CString(szHash);
}

void md4strA(const uchar* hash, CHAR* pszHash)
{
    static const CHAR _acHexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++)
    {
        *pszHash++ = _acHexDigits[hash[i] >> 4];
        *pszHash++ = _acHexDigits[hash[i] & 0xf];
    }
    *pszHash = '\0';
}

CStringA md4strA(const uchar* hash)
{
    CHAR szHash[MAX_HASHSTR_SIZE];
    md4strA(hash, szHash);
    return CStringA(szHash);
}

bool strmd4(const char* pszHash, uchar* hash)
{
    memset(hash, 0, 16);
    for (int i = 0; i < 16; i++)
    {
        char byte[3];
        byte[0] = pszHash[i*2+0];
        byte[1] = pszHash[i*2+1];
        byte[2] = '\0';

        UINT b;
        if (sscanf(byte, "%x", &b) != 1)
            return false;
        hash[i] = (char)b;
    }
    return true;
}

bool strmd4(const CString& rstr, uchar* hash)
{
    memset(hash, 0, 16);
    if (rstr.GetLength() != 16*2)
        return false;
    for (int i = 0; i < 16; i++)
    {
        char byte[3];
        byte[0] = (char)rstr[i*2+0];
        byte[1] = (char)rstr[i*2+1];
        byte[2] = '\0';

        UINT b;
        if (sscanf(byte, "%x", &b) != 1)
            return false;
        hash[i] = (char)b;
    }
    return true;
}

void StripTrailingCollon(CString& rstr)
{
    if (!rstr.IsEmpty())
    {
        if (rstr[rstr.GetLength() - 1] == _T(':'))
            rstr = rstr.Left(rstr.GetLength() - 1);
    }
}

CString ValidFilename(CString filename)
{
    filename = URLDecode(filename);

    if (filename.GetLength() > 100)
        filename = filename.Left(100);

    // remove invalid filename characters
    filename.Remove(_T('\\'));
    filename.Remove(_T('\"'));
    filename.Remove(_T('/'));
    filename.Remove(_T(':'));
    filename.Remove(_T('*'));
    filename.Remove(_T('?'));
    filename.Remove(_T('<'));
    filename.Remove(_T('>'));
    filename.Remove(_T('|'));

    filename.Trim();
    return filename;
}

CString CleanupFilename(CString filename, bool bExtension)
{
    filename = URLDecode(filename);
    filename.MakeLower();

    //remove substrings, defined in the preferences (.ini)
    CString strlink = thePrefs.GetFilenameCleanups();
    strlink.MakeLower();

    int curPos = 0;
    CString resToken = strlink.Tokenize(L"|", curPos);
    while (!resToken.IsEmpty())
    {
        filename.Replace(resToken, L"");
        resToken = strlink.Tokenize(L"|", curPos);
    }

    // Replace "." with space - except the last one (extension-dot)
    int extpos = bExtension ? filename.ReverseFind(_T('.')) : filename.GetLength();
    if (extpos > 0)
    {
        for (int i = 0; i < extpos; i++)
        {
            if (filename.GetAt(i) != _T('.'))
                continue;
            if (i > 0 && i < filename.GetLength()-1 && _istdigit((_TUCHAR)filename.GetAt(i-1)) && _istdigit((_TUCHAR)filename.GetAt(i+1)))
                continue;
            filename.SetAt(i, _T(' '));
        }
    }

    // replace space-holders with spaces
    filename.Replace(_T('_'), _T(' '));
    filename.Replace(_T('+'), _T(' '));
    filename.Replace(_T('='), _T(' '));

    // remove invalid filename characters
    filename.Remove(_T('\\'));
    filename.Remove(_T('\"'));
    filename.Remove(_T('/'));
    filename.Remove(_T(':'));
    filename.Remove(_T('*'));
    filename.Remove(_T('?'));
    filename.Remove(_T('<'));
    filename.Remove(_T('>'));
    filename.Remove(_T('|'));

    // remove [AD]
    CString tempStr;
    int pos1 = -1;
    for (;;)
    {
        pos1 = filename.Find(_T('['), pos1+1);
        if (pos1 == -1)
            break;
        int pos2 = filename.Find(_T(']'), pos1);
        if (pos1 > -1 && pos2 > pos1)
        {
            if (pos2 - pos1 > 1)
            {
                tempStr = filename.Mid(pos1+1, pos2-pos1-1);
                int numcount = 0;
                for (int i = 0; i < tempStr.GetLength(); i++)
                {
                    if (_istdigit((_TUCHAR)tempStr.GetAt(i)))
                        numcount++;
                }
                if (numcount > tempStr.GetLength()/2)
                    continue;
            }
            filename = filename.Left(pos1) + filename.Right(filename.GetLength()-pos2-1);
            pos1--;
        }
        else
            break;
    }

    // Make leading Caps
    if (filename.GetLength() > 1)
    {
        tempStr = filename.GetAt(0);
        tempStr.MakeUpper();
        filename.SetAt(0, tempStr.GetAt(0));

        int topos = filename.ReverseFind(_T('.')) - 1;
        if (topos < 0)
            topos = filename.GetLength() - 1;

        for (int ix = 0; ix < topos; ix++)
        {
            if (!_istalpha((_TUCHAR)filename.GetAt(ix)))
            {
                if ((ix < filename.GetLength()-2 && _istdigit((_TUCHAR)filename.GetAt(ix+2))) ||
                        filename.GetAt(ix)==_T('\'')
                   )
                    continue;

                tempStr = filename.GetAt(ix+1);
                tempStr.MakeUpper();
                filename.SetAt(ix+1, tempStr.GetAt(0));
            }
        }
    }

    // additional formatting
    filename.Replace(L"()", L"");
    filename.Replace(L"  ", L" ");
    filename.Replace(L" .", L".");
    filename.Replace(L"( ", L"(");
    filename.Replace(L" )", L")");
    filename.Replace(L"()", L"");
    filename.Replace(L"{ ", L"{");
    filename.Replace(L" }", L"}");
    filename.Replace(L"{}", L"");

    filename.Trim();
    return filename;
}

struct SED2KFileType
{
    LPCTSTR pszExt;
    EED2KFileType iFileType;
} g_aED2KFileTypes[] =
{
    { L".aac",   ED2KFT_AUDIO },     // Advanced Audio Coding File
    { L".ac3",   ED2KFT_AUDIO },     // Audio Codec 3 File
    { L".aif",   ED2KFT_AUDIO },     // Audio Interchange File Format
    { L".aifc",  ED2KFT_AUDIO },     // Audio Interchange File Format
    { L".aiff",  ED2KFT_AUDIO },     // Audio Interchange File Format
    { L".alac",  ED2KFT_AUDIO },     // Apple Lossless Audio Codec File
    { L".amr",   ED2KFT_AUDIO },     // Adaptive Multi-Rate Codec File
    { L".ape",   ED2KFT_AUDIO },     // Monkey's Audio Lossless Audio File
    { L".au",    ED2KFT_AUDIO },     // Audio File (Sun, Unix)
    { L".aud",   ED2KFT_AUDIO },     // General Audio File
    { L".audio", ED2KFT_AUDIO },     // General Audio File
    { L".cda",   ED2KFT_AUDIO },     // CD Audio Track
    { L".dmf",   ED2KFT_AUDIO },     // Delusion Digital Music File
    { L".dsm",   ED2KFT_AUDIO },     // Digital Sound Module
    { L".dts",   ED2KFT_AUDIO },     // DTS Encoded Audio File
    { L".far",   ED2KFT_AUDIO },     // Farandole Composer Module
    { L".flac",  ED2KFT_AUDIO },     // Free Lossless Audio Codec File
    { L".it",    ED2KFT_AUDIO },     // Impulse Tracker Module
    { L".m1a",   ED2KFT_AUDIO },     // MPEG-1 Audio File
    { L".m2a",   ED2KFT_AUDIO },     // MPEG-2 Audio File
    { L".m4a",   ED2KFT_AUDIO },     // MPEG-4 Audio File
    { L".mdl",   ED2KFT_AUDIO },     // DigiTrakker Module
    { L".med",   ED2KFT_AUDIO },     // Amiga MED Sound File
    { L".mid",   ED2KFT_AUDIO },     // MIDI File
    { L".midi",  ED2KFT_AUDIO },     // MIDI File
    { L".mka",   ED2KFT_AUDIO },     // Matroska Audio File
    { L".mod",   ED2KFT_AUDIO },     // Amiga Music Module File
    { L".mp1",   ED2KFT_AUDIO },     // MPEG-1 Audio File
    { L".mp2",   ED2KFT_AUDIO },     // MPEG-2 Audio File
    { L".mp3",   ED2KFT_AUDIO },     // MPEG-3 Audio File
    { L".mpa",   ED2KFT_AUDIO },     // MPEG Audio File
    { L".mpc",   ED2KFT_AUDIO },     // Musepack Compressed Audio File
    { L".mtm",   ED2KFT_AUDIO },     // MultiTracker Module
    { L".ogg",   ED2KFT_AUDIO },     // Ogg Vorbis Compressed Audio File
    { L".opus",  ED2KFT_AUDIO },     // Opus Audio File
    { L".psm",   ED2KFT_AUDIO },     // Protracker Studio Module
    { L".ptm",   ED2KFT_AUDIO },     // PolyTracker Module
    { L".ra",    ED2KFT_AUDIO },     // Real Audio File
    { L".rmi",   ED2KFT_AUDIO },     // MIDI File
    { L".s3m",   ED2KFT_AUDIO },     // Scream Tracker 3 Module
    { L".shn",   ED2KFT_AUDIO },     // Shorten Audio File
    { L".snd",   ED2KFT_AUDIO },     // Audio File (Sun, Unix)
    { L".stm",   ED2KFT_AUDIO },     // Scream Tracker 2 Module
    { L".tak",   ED2KFT_AUDIO },     // Tom's Audio-Kompressor File
    { L".umx",   ED2KFT_AUDIO },     // Unreal Music Package
    { L".wav",   ED2KFT_AUDIO },     // WAVE Audio File
    { L".w64",   ED2KFT_AUDIO },     // Sonic Foundry Video Editor Wave64 File
    { L".wma",   ED2KFT_AUDIO },     // Windows Media Audio File
    { L".wv",    ED2KFT_AUDIO },     // WavPack Audio File
    { L".xm",    ED2KFT_AUDIO },     // Fasttracker 2 Extended Module

    { L".3g2",   ED2KFT_VIDEO },     // 3GPP Multimedia File
    { L".3gp",   ED2KFT_VIDEO },     // 3GPP Multimedia File
    { L".3gp2",  ED2KFT_VIDEO },     // 3GPP Multimedia File
    { L".3gpp",  ED2KFT_VIDEO },     // 3GPP Multimedia File
    { L".amv",   ED2KFT_VIDEO },     // Anime Music Video File
    { L".asf",   ED2KFT_VIDEO },     // Advanced Systems Format File
    { L".avi",   ED2KFT_VIDEO },     // Audio Video Interleave File
    { L".bik",   ED2KFT_VIDEO },     // BINK Video File
    { L".divx",  ED2KFT_VIDEO },     // DivX-Encoded Movie File
    { L".dvr-ms",ED2KFT_VIDEO },     // Microsoft Digital Video Recording
    { L".flc",   ED2KFT_VIDEO },     // FLIC Video File
    { L".fli",   ED2KFT_VIDEO },     // FLIC Video File
    { L".flic",  ED2KFT_VIDEO },     // FLIC Video File
    { L".flv",   ED2KFT_VIDEO },     // Flash Video File
    { L".hdmov", ED2KFT_VIDEO },     // High-Definition QuickTime Movie
    { L".ifo",   ED2KFT_VIDEO },     // DVD-Video Disc Information File
    { L".m1v",   ED2KFT_VIDEO },     // MPEG-1 Video File
    { L".m2t",   ED2KFT_VIDEO },     // MPEG-2 Video Transport Stream
    { L".m2ts",  ED2KFT_VIDEO },     // MPEG-2 Video Transport Stream
    { L".m2v",   ED2KFT_VIDEO },     // MPEG-2 Video File
    { L".m4b",   ED2KFT_VIDEO },     // MPEG-4 Video File
    { L".m4v",   ED2KFT_VIDEO },     // MPEG-4 Video File
    { L".mkv",   ED2KFT_VIDEO },     // Matroska Video File
    { L".mov",   ED2KFT_VIDEO },     // QuickTime Movie File
    { L".movie", ED2KFT_VIDEO },     // QuickTime Movie File
    { L".mp1v",  ED2KFT_VIDEO },     // MPEG-1 Video File
    { L".mp2v",  ED2KFT_VIDEO },     // MPEG-2 Video File
    { L".mp4",   ED2KFT_VIDEO },     // MPEG-4 Video File
    { L".mpe",   ED2KFT_VIDEO },     // MPEG Video File
    { L".mpeg",  ED2KFT_VIDEO },     // MPEG Video File
    { L".mpg",   ED2KFT_VIDEO },     // MPEG Video File
    { L".mpv",   ED2KFT_VIDEO },     // MPEG Video File
    { L".mpv1",  ED2KFT_VIDEO },     // MPEG-1 Video File
    { L".mpv2",  ED2KFT_VIDEO },     // MPEG-2 Video File
    { L".ogm",   ED2KFT_VIDEO },     // Ogg Media File
    { L".ogv",   ED2KFT_VIDEO },     // Ogg Video File
    { L".pva",   ED2KFT_VIDEO },     // MPEG Video File
    { L".qt",    ED2KFT_VIDEO },     // QuickTime Movie
    { L".ram",   ED2KFT_VIDEO },     // Real Audio Media
    { L".ratdvd",ED2KFT_VIDEO },     // RatDVD Disk Image
    { L".rm",    ED2KFT_VIDEO },     // Real Media File
    { L".rmm",   ED2KFT_VIDEO },     // Real Media File
    { L".rmvb",  ED2KFT_VIDEO },     // Real Video Variable Bit Rate File
    { L".rv",    ED2KFT_VIDEO },     // Real Video File
    { L".smil",  ED2KFT_VIDEO },     // SMIL Presentation File
    { L".smk",   ED2KFT_VIDEO },     // Smacker Compressed Movie File
    { L".swf",   ED2KFT_VIDEO },     // Macromedia Flash Movie
    { L".tp",    ED2KFT_VIDEO },     // Video Transport Stream File
    { L".ts",    ED2KFT_VIDEO },     // Video Transport Stream File
    { L".vid",   ED2KFT_VIDEO },     // General Video File
    { L".video", ED2KFT_VIDEO },     // General Video File
    { L".vob",   ED2KFT_VIDEO },     // DVD Video Object File
    { L".vp6",   ED2KFT_VIDEO },     // TrueMotion VP6 Video File
    { L".wm",    ED2KFT_VIDEO },     // Windows Media Video File
    { L".wmv",   ED2KFT_VIDEO },     // Windows Media Video File
    { L".xvid",  ED2KFT_VIDEO },     // Xvid-Encoded Video File

    { L".bmp",   ED2KFT_IMAGE },     // Bitmap Image File
    { L".emf",   ED2KFT_IMAGE },     // Enhanced Windows Metafile
    { L".gif",   ED2KFT_IMAGE },     // Graphical Interchange Format File
    { L".ico",   ED2KFT_IMAGE },     // Icon File
    { L".jfif",  ED2KFT_IMAGE },     // JPEG File Interchange Format
    { L".jpe",   ED2KFT_IMAGE },     // JPEG Image File
    { L".jpeg",  ED2KFT_IMAGE },     // JPEG Image File
    { L".jpg",   ED2KFT_IMAGE },     // JPEG Image File
    { L".pct",   ED2KFT_IMAGE },     // PICT Picture File
    { L".pcx",   ED2KFT_IMAGE },     // Paintbrush Bitmap Image File
    { L".pic",   ED2KFT_IMAGE },     // PICT Picture File
    { L".pict",  ED2KFT_IMAGE },     // PICT Picture File
    { L".png",   ED2KFT_IMAGE },     // Portable Network Graphic
    { L".psd",   ED2KFT_IMAGE },     // Photoshop Document
    { L".psp",   ED2KFT_IMAGE },     // Paint Shop Pro Image File
    { L".pspimage", ED2KFT_IMAGE },  // Paint Shop Pro Image File
    { L".tga",   ED2KFT_IMAGE },     // Targa Graphic
    { L".tif",   ED2KFT_IMAGE },     // Tagged Image File
    { L".tiff",  ED2KFT_IMAGE },     // Tagged Image File
    { L".wmf",   ED2KFT_IMAGE },     // Windows Metafile
    { L".wmp",   ED2KFT_IMAGE },     // Windows Media Photo File
    { L".xbm",   ED2KFT_IMAGE },     // X Bitmap File
    { L".xif",   ED2KFT_IMAGE },     // ScanSoft Pagis Extended Image Format File
    { L".xpm",   ED2KFT_IMAGE },     // X Pixmap File

    { L".7z",    ED2KFT_ARCHIVE },   // 7-Zip Compressed File
    { L".ace",   ED2KFT_ARCHIVE },   // WinAce Compressed File
    { L".alz",   ED2KFT_ARCHIVE },   // ALZip Archive
    { L".arc",   ED2KFT_ARCHIVE },   // Compressed File Archive
    { L".arj",   ED2KFT_ARCHIVE },   // ARJ Compressed File Archive
    { L".bz2",   ED2KFT_ARCHIVE },   // Bzip Compressed File
    { L".cab",   ED2KFT_ARCHIVE },   // Cabinet File
    { L".gz",    ED2KFT_ARCHIVE },   // Gnu Zipped File
    { L".hqx",   ED2KFT_ARCHIVE },	// BinHex 4.0 Encoded File
    { L".jar",   ED2KFT_ARCHIVE },   // Java Archive File
    { L".lha",   ED2KFT_ARCHIVE },   // LHARC Compressed Archive
    { L".lzh",   ED2KFT_ARCHIVE },   // LZH Compressed File
    { L".msi",   ED2KFT_ARCHIVE },   // Microsoft Installer File
    { L".pak",   ED2KFT_ARCHIVE },   // PAK (Packed) File
    { L".par",   ED2KFT_ARCHIVE },   // Parchive Index File
    { L".par2",  ED2KFT_ARCHIVE },   // Parchive 2 Index File
    { L".rar",   ED2KFT_ARCHIVE },   // WinRAR Compressed Archive
    { L".sit",   ED2KFT_ARCHIVE },   // Stuffit Archive
    { L".sitx",  ED2KFT_ARCHIVE },   // Stuffit X Archive
    { L".sqx",   ED2KFT_ARCHIVE },   // Squeez Archive
    { L".tar",   ED2KFT_ARCHIVE },   // Consolidated Unix File Archive
    { L".tbz2",  ED2KFT_ARCHIVE },   // Tar BZip 2 Compressed File
    { L".tgz",   ED2KFT_ARCHIVE },   // Gzipped Tar File
    { L".uha",   ED2KFT_ARCHIVE },   // uharc compression
    { L".xpi",   ED2KFT_ARCHIVE },   // Mozilla Installer Package
    { L".z",     ED2KFT_ARCHIVE },   // Unix Compressed File
    { L".zip",   ED2KFT_ARCHIVE },   // Zipped File
    { L".zipx",  ED2KFT_ARCHIVE },   // WinZip Archive File

    { L".apk",   ED2KFT_PROGRAM },  // Android package
    { L".app",   ED2KFT_PROGRAM },  // Mac OS X Application File
    { L".bat",   ED2KFT_PROGRAM },	// Batch File
    { L".cmd",   ED2KFT_PROGRAM },	// Command File
    { L".com",   ED2KFT_PROGRAM },	// COM File
    { L".exe",   ED2KFT_PROGRAM },	// Executable File
    { L".hta",   ED2KFT_PROGRAM },	// HTML Application
    { L".js",    ED2KFT_PROGRAM },	// Java Script
    { L".jse",   ED2KFT_PROGRAM },	// Encoded  Java Script
    { L".msc",   ED2KFT_PROGRAM },	// Microsoft Common Console File
    { L".vbe",   ED2KFT_PROGRAM },	// Encoded Visual Basic Script File
    { L".vbs",   ED2KFT_PROGRAM },	// Visual Basic Script File
    { L".wsf",   ED2KFT_PROGRAM },	// Windows Script File
    { L".wsh",   ED2KFT_PROGRAM },	// Windows Scripting Host File

    { L".bin",   ED2KFT_CDIMAGE },   // CD Image
    { L".bwa",   ED2KFT_CDIMAGE },   // BlindWrite Disk Information File
    { L".bwi",   ED2KFT_CDIMAGE },   // BlindWrite CD/DVD Disc Image
    { L".bws",   ED2KFT_CDIMAGE },   // BlindWrite Sub Code File
    { L".bwt",   ED2KFT_CDIMAGE },   // BlindWrite 4 Disk Image
    { L".ccd",   ED2KFT_CDIMAGE },   // CloneCD Disk Image
    { L".cue",   ED2KFT_CDIMAGE },   // Cue Sheet File
    { L".dmg",   ED2KFT_CDIMAGE },   // Mac OS X Disk Image
    { L".img",   ED2KFT_CDIMAGE },   // Disk Image Data File
    { L".iso",   ED2KFT_CDIMAGE },   // Disc Image File
    { L".mdf",   ED2KFT_CDIMAGE },   // Media Disc Image File
    { L".mds",   ED2KFT_CDIMAGE },   // Media Descriptor File
    { L".nrg",   ED2KFT_CDIMAGE },   // Nero CD/DVD Image File
    { L".sub",   ED2KFT_CDIMAGE },   // Subtitle File
    { L".toast", ED2KFT_CDIMAGE },   // Toast Disc Image
	    
    { L".css",   ED2KFT_DOCUMENT },  // Cascading Style Sheet
    { L".diz",   ED2KFT_DOCUMENT },  // Description in Zip File
    { L".doc",   ED2KFT_DOCUMENT },  // Document File
    { L".docx",  ED2KFT_DOCUMENT },  // Document File
    { L".dot",   ED2KFT_DOCUMENT },  // Document Template File
    { L".dotx",  ED2KFT_DOCUMENT },  // Document Template File
    { L".hlp",   ED2KFT_DOCUMENT },  // Help File
    { L".nfo",   ED2KFT_DOCUMENT },  // Warez Information File
    { L".odb",   ED2KFT_DOCUMENT },  // Open Document File
    { L".odf",   ED2KFT_DOCUMENT },  // Open Document File
    { L".odp",   ED2KFT_DOCUMENT },  // Open Document File
    { L".ods",   ED2KFT_DOCUMENT },  // Open Document File
    { L".odt",   ED2KFT_DOCUMENT },  // Open Document File    
    { L".pmd",   ED2KFT_DOCUMENT },  // PageMaker Publication
    { L".pmv",   ED2KFT_DOCUMENT },  // PlanMaker Spreadsheet
    { L".pps",   ED2KFT_DOCUMENT },  // PowerPoint Slide Show
    { L".ppt",   ED2KFT_DOCUMENT },  // PowerPoint Presentation
    { L".pptx",  ED2KFT_DOCUMENT },  // PowerPoint Presentation    
    { L".qpw",   ED2KFT_DOCUMENT },  // Quattro Pro Spreadsheet
    { L".rtf",   ED2KFT_DOCUMENT },  // Rich Text Format File
    { L".text",  ED2KFT_DOCUMENT },  // General Text File
    { L".tmd",   ED2KFT_DOCUMENT },  // TextMaker Document    
    { L".wpd",   ED2KFT_DOCUMENT },  // WordPerfect Document
    { L".wri",   ED2KFT_DOCUMENT },  // Windows Write Document
    { L".xls",   ED2KFT_DOCUMENT },  // Microsoft Excel Spreadsheet
    { L".xlsx",  ED2KFT_DOCUMENT },  // Microsoft Excel Spreadsheet
    { L".xlt",   ED2KFT_DOCUMENT },  // Microsoft Excel Template File
    { L".xltx",  ED2KFT_DOCUMENT },  // Microsoft Excel Template File
    
	// taken from http://en.wikipedia.org/wiki/Comparison_of_e-book_formats
	{ L".aeh",   ED2KFT_DOCUMENT },  // Archos Reader
	{ L".lrf",   ED2KFT_DOCUMENT },  // Sony Media
	{ L".lrx",   ED2KFT_DOCUMENT },  // Sony Media
	{ L".cbr",   ED2KFT_ARCHIVE },   // Comic Book RAR Archive
	{ L".cbz",   ED2KFT_ARCHIVE },   // Comic Book ZIP Archive
	{ L".cb7",   ED2KFT_ARCHIVE },   // Comic Book 7z Archive
	{ L".cbt",   ED2KFT_ARCHIVE },   // Comic Book TAR Archive
	{ L".cba",   ED2KFT_ARCHIVE },   // Comic Book ACE Archive
	{ L".chm",   ED2KFT_DOCUMENT },  // Compiled HTML Help File
	{ L".djvu",  ED2KFT_DOCUMENT },  // DjVu
	{ L".epub",  ED2KFT_DOCUMENT },  // IDPF/EPUB
	{ L".pdb",   ED2KFT_DOCUMENT },	 // Palm Media/Plucker
	{ L".fb2",   ED2KFT_DOCUMENT },	 // FictionBook
	{ L".xeb",   ED2KFT_DOCUMENT },	 // Apabi Reader
	{ L".ceb",   ED2KFT_DOCUMENT },	 // Apabi Reader
	{ L".htm",   ED2KFT_DOCUMENT },  // HTML File
	{ L".html",  ED2KFT_DOCUMENT },  // HTML File
	{ L".ibooks",  ED2KFT_DOCUMENT },  // iBook
	{ L".azw",   ED2KFT_DOCUMENT },  // Kindle
	{ L".kf8",   ED2KFT_DOCUMENT },  // Kindle
	{ L".lit",   ED2KFT_DOCUMENT },  // Microsoft Reader
	{ L".prc",   ED2KFT_DOCUMENT },  // Mobipocket
	{ L".mobi",  ED2KFT_DOCUMENT },  // Mobipocket
	{ L".pkg",   ED2KFT_DOCUMENT },  // Newton eBook
	{ L".opf",   ED2KFT_DOCUMENT },  // Open eBook
	{ L".pdf",   ED2KFT_DOCUMENT },  // Portable Document Format File
	{ L".txt",   ED2KFT_DOCUMENT },  // Text File
	{ L".ps",    ED2KFT_DOCUMENT },  // PostScript File
	{ L".pdg",   ED2KFT_DOCUMENT },  // SSReader
	{ L".tebr",  ED2KFT_DOCUMENT },  // TEBR
	{ L".xml",   ED2KFT_DOCUMENT },  // XML File
	{ L".tr2",   ED2KFT_DOCUMENT },  // TomeRaider
	{ L".tr3",   ED2KFT_DOCUMENT },  // TomeRaider
	{ L".xps",   ED2KFT_DOCUMENT },  // OpenXPS
	{ L".oxps",  ED2KFT_DOCUMENT },  // OpenXPS

    { L".emulecollection", ED2KFT_EMULECOLLECTION }
};

int __cdecl CompareE2DKFileType(const void* p1, const void* p2)
{
    return _tcscmp(((const SED2KFileType*)p1)->pszExt, ((const SED2KFileType*)p2)->pszExt);
}

EED2KFileType GetED2KFileTypeID(LPCTSTR pszFileName)
{
    LPCTSTR pszExt = _tcsrchr(pszFileName, _T('.'));
    if (pszExt == NULL)
        return ED2KFT_ANY;
    CString strExt(pszExt);
    strExt.MakeLower();

    SED2KFileType ft;
    ft.pszExt = strExt;
    ft.iFileType = ED2KFT_ANY;
    const SED2KFileType* pFound = (SED2KFileType*)bsearch(&ft, g_aED2KFileTypes, _countof(g_aED2KFileTypes), sizeof g_aED2KFileTypes[0], CompareE2DKFileType);
    if (pFound != NULL)
        return pFound->iFileType;
    return ED2KFT_ANY;
}

// Retuns the ed2k file type string ID which is to be used for publishing+searching
LPCSTR GetED2KFileTypeSearchTerm(EED2KFileType iFileID)
{
    if (iFileID == ED2KFT_AUDIO)			return ED2KFTSTR_AUDIO;
    if (iFileID == ED2KFT_VIDEO)			return ED2KFTSTR_VIDEO;
    if (iFileID == ED2KFT_IMAGE)			return ED2KFTSTR_IMAGE;
    if (iFileID == ED2KFT_PROGRAM)			return ED2KFTSTR_PROGRAM;
    if (iFileID == ED2KFT_DOCUMENT)			return ED2KFTSTR_DOCUMENT;
    // NOTE: Archives and CD-Images are published+searched with file type "Pro"
    // NOTE: If this gets changed, the function 'GetED2KFileTypeSearchID' also needs to get updated!
    if (iFileID == ED2KFT_ARCHIVE)			return ED2KFTSTR_PROGRAM;
    if (iFileID == ED2KFT_CDIMAGE)			return ED2KFTSTR_PROGRAM;
    if (iFileID == ED2KFT_EMULECOLLECTION)	return ED2KFTSTR_EMULECOLLECTION;
    return NULL;
}

// Retuns the ed2k file type integer ID which is to be used for publishing+searching
EED2KFileType GetED2KFileTypeSearchID(EED2KFileType iFileID)
{
    if (iFileID == ED2KFT_AUDIO)			return ED2KFT_AUDIO;
    if (iFileID == ED2KFT_VIDEO)			return ED2KFT_VIDEO;
    if (iFileID == ED2KFT_IMAGE)			return ED2KFT_IMAGE;
    if (iFileID == ED2KFT_PROGRAM)			return ED2KFT_PROGRAM;
    if (iFileID == ED2KFT_DOCUMENT)			return ED2KFT_DOCUMENT;
    // NOTE: Archives and CD-Images are published+searched with file type "Pro"
    // NOTE: If this gets changed, the function 'GetED2KFileTypeSearchTerm' also needs to get updated!
    if (iFileID == ED2KFT_ARCHIVE)			return ED2KFT_PROGRAM;
    if (iFileID == ED2KFT_CDIMAGE)			return ED2KFT_PROGRAM;
    return ED2KFT_ANY;
}

// Returns a file type which is used eMule internally only, examining the extension of the given filename
CString GetFileTypeByName(LPCTSTR pszFileName)
{
    EED2KFileType iFileType = GetED2KFileTypeID(pszFileName);
    switch (iFileType)
    {
        case ED2KFT_AUDIO:
            return _T(ED2KFTSTR_AUDIO);
        case ED2KFT_VIDEO:
            return _T(ED2KFTSTR_VIDEO);
        case ED2KFT_IMAGE:
            return _T(ED2KFTSTR_IMAGE);
        case ED2KFT_DOCUMENT:
            return _T(ED2KFTSTR_DOCUMENT);
        case ED2KFT_PROGRAM:
            return _T(ED2KFTSTR_PROGRAM);
        case ED2KFT_ARCHIVE:
            return _T(ED2KFTSTR_ARCHIVE);
        case ED2KFT_CDIMAGE:
            return _T(ED2KFTSTR_CDIMAGE);
        case ED2KFT_EMULECOLLECTION:
            return _T(ED2KFTSTR_EMULECOLLECTION);
        default:
            return L"";
    }
}

// Returns a file type which is used eMule internally only (GUI)
CString GetFileTypeDisplayStrFromED2KFileType(LPCTSTR pszED2KFileType)
{
    ASSERT(pszED2KFileType != NULL);
    if (pszED2KFileType != NULL)
    {
        if (_tcscmp(pszED2KFileType, _T(ED2KFTSTR_AUDIO)) == 0)			return GetResString(IDS_SEARCH_AUDIO);
        else if (_tcscmp(pszED2KFileType, _T(ED2KFTSTR_VIDEO)) == 0)    return GetResString(IDS_SEARCH_VIDEO);
        else if (_tcscmp(pszED2KFileType, _T(ED2KFTSTR_IMAGE)) == 0)    return GetResString(IDS_SEARCH_PICS);
        else if (_tcscmp(pszED2KFileType, _T(ED2KFTSTR_DOCUMENT)) == 0)	return GetResString(IDS_SEARCH_DOC);
        else if (_tcscmp(pszED2KFileType, _T(ED2KFTSTR_PROGRAM)) == 0)  return GetResString(IDS_SEARCH_PRG);
        else if (_tcscmp(pszED2KFileType, _T(ED2KFTSTR_ARCHIVE)) == 0)	return GetResString(IDS_SEARCH_ARC);
        else if (_tcscmp(pszED2KFileType, _T(ED2KFTSTR_CDIMAGE)) == 0)  return GetResString(IDS_SEARCH_CDIMG);
        else if (_tcscmp(pszED2KFileType, _T(ED2KFTSTR_EMULECOLLECTION)) == 0)	return GetResString(IDS_SEARCH_EMULECOLLECTION);
    }
    return L"";
}

class CED2KFileTypes
{
  public:
    CED2KFileTypes()
    {
        qsort(g_aED2KFileTypes, _countof(g_aED2KFileTypes), sizeof g_aED2KFileTypes[0], CompareE2DKFileType);
#ifdef _DEBUG
        // check for duplicate entries
        LPCTSTR pszLast = g_aED2KFileTypes[0].pszExt;
        for (int i = 1; i < _countof(g_aED2KFileTypes); i++)
        {
            ASSERT(_tcscmp(pszLast, g_aED2KFileTypes[i].pszExt) != 0);
            pszLast = g_aED2KFileTypes[i].pszExt;
        }
#endif
    }
};
CED2KFileTypes theED2KFileTypes; // get the list sorted *before* any code is accessing it

const BYTE *FindPattern(const BYTE *pucBuff, int iBuffSize, const BYTE *pucPattern, int iPatternSize)
{
    int iSearchRange = iBuffSize - iPatternSize;
    while (iSearchRange-- >= 0)
    {
        if (memcmp(pucBuff, pucPattern, iPatternSize) == 0)
            return pucBuff;
        pucBuff++;
    }
    return NULL;
}

TCHAR *stristr(const TCHAR *str1, const TCHAR *str2)
{
    const TCHAR *cp = str1;
    const TCHAR *s1;
    const TCHAR *s2;

    if (!*str2)
        return (TCHAR *)str1;

    while (*cp)
    {
        s1 = cp;
        s2 = str2;

        while (*s1 && *s2 && _totlower((_TUCHAR)*s1) == _totlower((_TUCHAR)*s2))
            s1++, s2++;

        if (!*s2)
            return (TCHAR *)cp;

        cp++;
    }

    return NULL;
}

CString GetNextString(const CString& rstr, LPCTSTR pszTokens, int& riStart)
{
    CString strResult;
    if (pszTokens != NULL && riStart != -1)
    {
        int iToken = rstr.Find(pszTokens, riStart);
        if (iToken != -1)
        {
            int iLen = iToken - riStart;
            if (iLen >= 0)
            {
                strResult = rstr.Mid(riStart, iLen);
                riStart += iLen + 1;
            }
        }
        else
        {
            strResult = rstr.Mid(riStart);
            riStart = -1;
        }
    }
    return strResult;
}

CString GetNextString(const CString& rstr, TCHAR chToken, int& riStart)
{
    CString strResult;
    if (chToken != L'\0' && riStart != -1)
    {
        int iToken = rstr.Find(chToken, riStart);
        if (iToken != -1)
        {
            int iLen = iToken - riStart;
            if (iLen >= 0)
            {
                strResult = rstr.Mid(riStart, iLen);
                riStart += iLen + 1;
            }
        }
        else
        {
            strResult = rstr.Mid(riStart);
            riStart = -1;
        }
    }
    return strResult;
}

int GetSystemErrorString(DWORD dwError, CString &rstrError)
{
    // FormatMessage language flags:
    //
    // - MFC uses: MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT)
    //				SUBLANG_SYS_DEFAULT = 0x02 (system default)
    //
    // - SDK uses: MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
    //				SUBLANG_DEFAULT		= 0x01 (user default)
    //
    //
    // Found in "winnt.h"
    // ------------------
    //  Language IDs.
    //
    //  The following two combinations of primary language ID and
    //  sublanguage ID have special semantics:
    //
    //    Primary Language ID   Sublanguage ID      Result
    //    -------------------   ---------------     ------------------------
    //    LANG_NEUTRAL          SUBLANG_NEUTRAL     Language neutral
    //    LANG_NEUTRAL          SUBLANG_DEFAULT     User default language
    //    LANG_NEUTRAL          SUBLANG_SYS_DEFAULT System default language
    //
    // *** SDK notes also:
    // If you pass in zero, 'FormatMessage' looks for a message for LANGIDs in
    // the following order:
    //
    //	1) Language neutral
    //	2) Thread LANGID, based on the thread's locale value
    //  3) User default LANGID, based on the user's default locale value
    //	4) System default LANGID, based on the system default locale value
    //	5) US English
    LPTSTR pszSysMsg = NULL;
    DWORD dwLength = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                                   (LPTSTR)&pszSysMsg, 0, NULL);
    if (pszSysMsg != NULL && dwLength != 0)
    {
        if (dwLength >= 2 && pszSysMsg[dwLength - 2] == _T('\r'))
            pszSysMsg[dwLength - 2] = L'\0';
        rstrError = pszSysMsg;
        rstrError.Replace(L"\r\n", L" "); // some messages contain CRLF within the message!?
    }
    else
    {
        rstrError.Empty();
    }

    if (pszSysMsg)
        LocalFree(pszSysMsg);

    return rstrError.GetLength();
}

int GetModuleErrorString(DWORD dwError, CString &rstrError, LPCTSTR pszModule)
{
    LPTSTR pszSysMsg = NULL;
    DWORD dwLength = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   GetModuleHandle(pszModule), dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                                   (LPTSTR)&pszSysMsg, 0, NULL);
    if (pszSysMsg != NULL && dwLength != 0)
    {
        if (dwLength >= 2 && pszSysMsg[dwLength - 2] == _T('\r'))
            pszSysMsg[dwLength - 2] = L'\0';
        rstrError = pszSysMsg;
        rstrError.Replace(L"\r\n", L" "); // some messages contain CRLF within the message!?
    }
    else
    {
        rstrError.Empty();
    }

    if (pszSysMsg)
        LocalFree(pszSysMsg);

    return rstrError.GetLength();
}

int GetErrorMessage(DWORD dwError, CString &rstrErrorMsg, DWORD dwFlags)
{
    int iMsgLen = GetSystemErrorString(dwError, rstrErrorMsg);
    if (iMsgLen == 0)
    {
        if ((long)dwError >= 0)
            rstrErrorMsg.Format(L"Error %u", dwError);
        else
            rstrErrorMsg.Format(L"Error 0x%08x", dwError);
    }
    else if (dwFlags & 1)
    {
        CString strFullErrorMsg;
        if ((long)dwError >= 0)
            strFullErrorMsg.Format(L"Error %u: %s", dwError, rstrErrorMsg);
        else
            strFullErrorMsg.Format(L"Error 0x%08x: %s", dwError, rstrErrorMsg);
        rstrErrorMsg = strFullErrorMsg;
    }

    return rstrErrorMsg.GetLength();
}

CString GetErrorMessage(DWORD dwError, DWORD dwFlags)
{
    CString strError;
    GetErrorMessage(dwError, strError, dwFlags);
    return strError;
}

LPCTSTR GetShellExecuteErrMsg(DWORD dwShellExecError)
{
    /* The error codes returned from 'ShellExecute' consist of the 'old' WinExec
     * error codes and some additional error codes.
     *
     * Error					Code							WinExec
     * ----------------------------------------------------------------------------
     * 0						0	"Out of memory"				Yes
     * ERROR_FILE_NOT_FOUND		2	(== SE_ERR_FNF)				Yes
     * SE_ERR_FNF				2	(== ERROR_FILE_NOT_FOUND)	Yes
     * ERROR_PATH_NOT_FOUND		3	(== SE_ERR_PNF)				Yes
     * SE_ERR_PNF				3	(== ERROR_PATH_NOT_FOUND)	Yes
     * SE_ERR_ACCESSDENIED		5								Yes
     * SE_ERR_OOM				8								Yes
     * ERROR_BAD_FORMAT			11								Yes
     * SE_ERR_SHARE				26								No
     * SE_ERR_ASSOCINCOMPLETE	27								No
     * SE_ERR_DDETIMEOUT		28								No
     * SE_ERR_DDEFAIL			29								No
     * SE_ERR_DDEBUSY			30								No
     * SE_ERR_NOASSOC			31								No
     * SE_ERR_DLLNOTFOUND		32								No
     *
     * Using "FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, <shell32.dll>" does *not* work!
     */
    static const struct _tagERRMSG
    {
        UINT	uId;
        LPCTSTR	pszMsg;
    } s_aszWinExecErrMsg[] =
    {
        {
            0,
            L"The operating system is out of memory or resources."
        },
        {
            1,
            L"Call to MS-DOS Int 21 Function 4B00h was invalid."
        },
        {/* 2*/SE_ERR_FNF,
            L"The specified file was not found."
        },
        {/* 3*/SE_ERR_PNF,
            L"The specified path was not found."
        },
        {
            4,
            L"Too many files were open."
        },
        {/* 5*/SE_ERR_ACCESSDENIED,
            L"The operating system denied access to the specified file."
        },
        {
            6,
            L"Library requires separate data segments for each task."
        },
        {
            7,
            L"There were MS-DOS Memory block problems."
        },
        {/* 8*/SE_ERR_OOM,
            L"There was insufficient memory to start the application."
        },
        {
            9,
            L"There were MS-DOS Memory block problems."
        },
        {
            10,
            L"Windows version was incorrect."
        },
        {
            11,
            L"Executable file was invalid. Either it was not a Windows application or there was an error in the .EXE image."
        },
        {
            12,
            L"Application was designed for a different operating system."
        },
        {
            13,
            L"Application was designed for MS-DOS 4.0."
        },
        {
            14,
            L"Type of executable file was unknown."
        },
        {
            15,
            L"Attempt was made to load a real-mode application (developed for an earlier version of Windows)."
        },
        {
            16,
            L"Attempt was made to load a second instance of an executable file containing multiple data segments that were not marked read-only."
        },
        {
            17,
            L"Attempt was made in large-frame EMS mode to load a second instance of an application that links to certain nonshareable DLLs that are already in use."
        },
        {
            18,
            L"Attempt was made in real mode to load an application marked for use in protected mode only."
        },
        {
            19,
            L"Attempt was made to load a compressed executable file. The file must be decompressed before it can be loaded."
        },
        {
            20,
            L"Dynamic-link library (DLL) file was invalid. One of the DLLs required to run this application was corrupt."
        },
        {
            21,
            L"Application requires Microsoft Windows 32-bit extensions."
        },
        /*22-31	 RESERVED FOR FUTURE USE. NOT RETURNED BY VERSION 3.0.*/
        {
            24,
            L"Command line too long."
        }, // 30.03.99 []: Seen under WinNT 4.0/Win98 for a very long command line!

        /*non-WinExec error codes*/
        {/*26*/SE_ERR_SHARE,
            L"A sharing violation occurred."
        },
        {/*27*/SE_ERR_ASSOCINCOMPLETE,
            L"The file name association is incomplete or invalid."
        },
        {/*28*/SE_ERR_DDETIMEOUT,
            L"The DDE transaction could not be completed because the request timed out."
        },
        {/*29*/SE_ERR_DDEFAIL,
            L"The DDE transaction failed."
        },
        {/*30*/SE_ERR_DDEBUSY,
            L"The DDE transaction could not be completed because other DDE transactions were being processed."
        },
        {/*31*/SE_ERR_NOASSOC,
            L"There is no application associated with the given file name extension."
        },
        {/*32*/SE_ERR_DLLNOTFOUND,
            L"The specified dynamic-link library was not found."
        }
    };

    if (dwShellExecError > 32)
        return L"";	// No error

    // Search error message
    for (UINT i = 0; i < _countof(s_aszWinExecErrMsg); i++)
    {
        if (s_aszWinExecErrMsg[i].uId == dwShellExecError)
            return s_aszWinExecErrMsg[i].pszMsg;
    }

    return L"Unknown error.";
}

int GetDesktopColorDepth()
{
    HDC hdcScreen = ::GetDC(HWND_DESKTOP);
    int iColorBits = GetDeviceCaps(hdcScreen, BITSPIXEL) * GetDeviceCaps(hdcScreen, PLANES);
    ::ReleaseDC(HWND_DESKTOP, hdcScreen);
    return iColorBits;
}

int GetAppImageListColorFlag()
{
    int iColorBits = GetDesktopColorDepth();
    int iIlcFlag;
    if (iColorBits >= 32)
        iIlcFlag = ILC_COLOR32;
    else if (iColorBits >= 24)
        iIlcFlag = ILC_COLOR24;
    else if (iColorBits >= 16)
        iIlcFlag = ILC_COLOR16;
    else if (iColorBits >= 8)
        iIlcFlag = ILC_COLOR8;
    else if (iColorBits >= 4)
        iIlcFlag = ILC_COLOR4;
    else
        iIlcFlag = ILC_COLOR;
    return iIlcFlag;
}

CString DbgGetHexDump(const uint8* data, UINT size)
{
    CString buffer;
    buffer.Format(L"Size=%u", size);
    buffer += L", Data=[";
    UINT i = 0;
    for (; i < size && i < 50; i++)
    {
        if (i > 0)
            buffer += L" ";
        TCHAR temp[3];
        _stprintf(temp, L"%02x", data[i]);
        buffer += temp;
    }
    buffer += (i == size) ? L"]" : L"..]";
    return buffer;
}

void DbgSetThreadName(LPCSTR szThreadName, ...)
{
#ifdef DEBUG

#ifndef MS_VC_EXCEPTION
#define MS_VC_EXCEPTION 0x406d1388

    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType;		// must be 0x1000
        LPCSTR szName;		// pointer to name (in same addr space)
        DWORD dwThreadID;	// thread ID (-1 caller thread)
        DWORD dwFlags;		// reserved for future use, must be zero
    } THREADNAME_INFO;
#endif

    __try
    {
        va_list args;
        va_start(args, szThreadName);
        int lenBuf = 0;
        char *buffer = NULL;
        int lenResult;
        do // the VS debugger truncates the string to 31 characters anyway!
        {
            lenBuf += 128;
            delete[] buffer;
            buffer = new char[lenBuf];
            lenResult = _vsnprintf(buffer, lenBuf, szThreadName, args);
        }
        while (lenResult == -1);
        va_end(args);
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = buffer;
        info.dwThreadID = (DWORD)-1;
        info.dwFlags = 0;
        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (DWORD *)&info);
        }
        __except (EXCEPTION_CONTINUE_EXECUTION) { }
        delete [] buffer;
    }
    __except (EXCEPTION_CONTINUE_EXECUTION) {}
#else
    UNREFERENCED_PARAMETER(szThreadName);
#endif
}

CString RemoveFileExtension(const CString& rstrFilePath)
{
    int iDot = rstrFilePath.ReverseFind(_T('.'));
    if (iDot == -1)
        return rstrFilePath;
    return rstrFilePath.Mid(0, iDot);
}

int CompareDirectories(const CString& rstrDir1, const CString& rstrDir2)
{
    // use case insensitive compare as a starter
    if (rstrDir1.CompareNoCase(rstrDir2)==0)
        return 0;

    // if one of the paths ends with a '\' the paths may still be equal from the file system's POV
    CString strDir1(rstrDir1);
    CString strDir2(rstrDir2);
    PathRemoveBackslash(strDir1.GetBuffer());	// remove any available backslash
    strDir1.ReleaseBuffer();
    PathRemoveBackslash(strDir2.GetBuffer());	// remove any available backslash
    strDir2.ReleaseBuffer();
    // remove backslash from root drives like "C:\" - PathRemoveBackslash wonn't do this
    if (strDir1.GetLength() == 3 && strDir1.GetAt(2) == '\\')
        strDir1.Truncate(2);
    if (strDir2.GetLength() == 3 && strDir2.GetAt(2) == '\\')
        strDir2.Truncate(2);
    return strDir1.CompareNoCase(strDir2);		// compare again
}

bool IsGoodIP(UINT nIP, bool forceCheck)
{
    // always filter following IP's
    // -------------------------------------------
    // 0.0.0.0							invalid
    // 127.0.0.0 - 127.255.255.255		Loopback
    // 224.0.0.0 - 239.255.255.255		Multicast
    // 240.0.0.0 - 255.255.255.255		Reserved for Future Use
    // 255.255.255.255					invalid

    if (nIP==0 || (uint8)nIP==127 || (uint8)nIP>=224)
    {
#ifdef _DEBUG
        if (nIP==0x0100007F && thePrefs.GetAllowLocalHostIP())
            return true;
#endif
        return false;
    }

    if (!thePrefs.FilterLANIPs() && !forceCheck/*ZZ:UploadSpeedSense*/)
        return true;
    else
        return !IsLANIP(nIP);
}

bool IsLANIP(UINT nIP)
{
    // LAN IP's
    // -------------------------------------------
    //	0.*								"This" Network
    //	10.0.0.0 - 10.255.255.255		Class A
    //	172.16.0.0 - 172.31.255.255		Class B
    //	192.168.0.0 - 192.168.255.255	Class C

    uint8 nFirst = (uint8)nIP;
    uint8 nSecond = (uint8)(nIP >> 8);

    if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
        return true;

    if (nFirst==172 && nSecond>=16 && nSecond<=31)
        return true;

    if (nFirst==0 || nFirst==10)
        return true;

    return false;
}

bool IsGoodIPPort(UINT nIP, uint16 nPort)
{
    return IsGoodIP(nIP) && nPort!=0;
}

CString GetFormatedUInt(ULONG ulVal)
{
    TCHAR szVal[12];
    _ultot(ulVal, szVal, 10);

    static NUMBERFMT nf;
    if (nf.Grouping == 0)
    {
        nf.NumDigits = 0;
        nf.LeadingZero = 0;
        nf.Grouping = 3;
        // we are hardcoding the following two format chars by intention because the C-RTL also has the decimal sep hardcoded to '.'
        nf.lpDecimalSep = L".";
        nf.lpThousandSep = L",";
        nf.NegativeOrder = 0;
    }
    CString strVal;
    const int iBuffSize = _countof(szVal)*2;
    int iResult = GetNumberFormat(LOCALE_SYSTEM_DEFAULT, 0, szVal, &nf, strVal.GetBuffer(iBuffSize), iBuffSize);
    strVal.ReleaseBuffer();
    if (iResult == 0)
        strVal = szVal;
    return strVal;
}

CString GetFormatedUInt64(ULONGLONG ullVal)
{
    TCHAR szVal[24];
    _ui64tot(ullVal, szVal, 10);

    static NUMBERFMT nf;
    if (nf.Grouping == 0)
    {
        nf.NumDigits = 0;
        nf.LeadingZero = 0;
        nf.Grouping = 3;
        // we are hardcoding the following two format chars by intention because the C-RTL also has the decimal sep hardcoded to '.'
        nf.lpDecimalSep = L".";
        nf.lpThousandSep = L",";
        nf.NegativeOrder = 0;
    }
    CString strVal;
    const int iBuffSize = _countof(szVal)*2;
    int iResult = GetNumberFormat(LOCALE_SYSTEM_DEFAULT, 0, szVal, &nf, strVal.GetBuffer(iBuffSize), iBuffSize);
    strVal.ReleaseBuffer();
    if (iResult == 0)
        strVal = szVal;
    return strVal;
}

void Debug(LPCTSTR pszFmtMsg, ...)
{
    va_list pArgs;
    va_start(pArgs, pszFmtMsg);
    CString strBuff;
#ifdef _DEBUG
    time_t tNow = time(NULL);
    int iTimeLen = _tcsftime(strBuff.GetBuffer(40), 40, L"%H:%M:%S ", localtime(&tNow));
    strBuff.ReleaseBuffer(iTimeLen);
#endif
    strBuff.AppendFormatV(pszFmtMsg, pArgs);

    // get around a bug in the debug device which is not capable of dumping long strings
    int i = 0;
    while (i < strBuff.GetLength())
    {
        OutputDebugString(strBuff.Mid(i, 1024));
        i += 1024;
    }
    va_end(pArgs);
}

void DebugHexDump(const uint8* data, UINT lenData)
{
    int lenLine = 16;
    UINT pos = 0;
    byte c = 0;
    while (pos < lenData)
    {
        CStringA line;
        CStringA single;
        line.Format("%08X ", pos);
        lenLine = min((lenData - pos), 16);
        for (int i=0; i<lenLine; i++)
        {
            single.Format(" %02X", data[pos+i]);
            line += single;
            if (i == 7)
                line += ' ';
        }
        line += CStringA(' ', 60 - line.GetLength());
        for (int i=0; i<lenLine; i++)
        {
            c = data[pos + i];
            single.Format("%c", (((c > 31) && (c < 127)) ? (_TUCHAR)c : '.'));
            line += single;
        }
        Debug(L"%hs\n", line);
        pos += lenLine;
    }
}

void DebugHexDump(CFile& file)
{
    int iSize = (int)(file.GetLength() - file.GetPosition());
    if (iSize > 0)
    {
        uint8* data = NULL;
        try
        {
            data = new uint8[iSize];
            file.Read(data, iSize);
            DebugHexDump(data, iSize);
        }
        catch (CFileException* e)
        {
            TRACE(L"*** DebugHexDump(CFile&); CFileException\n");
            e->Delete();
        }
        catch (CMemoryException* e)
        {
            TRACE(L"*** DebugHexDump(CFile&); CMemoryException\n");
            e->Delete();
        }
        delete[] data;
    }
}

LPCTSTR DbgGetFileNameFromID(const uchar* hash)
{
    CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(hash);
    if (reqfile != NULL)
        return reqfile->GetFileName();

    CPartFile* partfile = theApp.downloadqueue->GetFileByID(hash);
    if (partfile != NULL)
        return partfile->GetFileName();

    CKnownFile* knownfile = theApp.knownfiles->FindKnownFileByID(hash);
    if (knownfile != NULL)
        return knownfile->GetFileName();

    return NULL;
}

CString DbgGetFileInfo(const uchar* hash)
{
    if (hash == NULL)
        return CString();

    CString strInfo(L"File=");
    LPCTSTR pszName = DbgGetFileNameFromID(hash);
    if (pszName != NULL)
        strInfo += pszName;
    else
        strInfo += md4str(hash);
    return strInfo;
}

CString DbgGetFileStatus(UINT nPartCount, CSafeMemFile* data)
{
    CString strFileStatus;
    if (nPartCount == 0)
        strFileStatus = L"Complete";
    else
    {
        CString strPartStatus;
        UINT nAvailableParts = 0;
        UINT nPart = 0;
        while (nPart < nPartCount)
        {
            uint8 ucPartMask;
            try
            {
                ucPartMask = data->ReadUInt8();
            }
            catch (CFileException* ex)
            {
                ex->Delete();
                strPartStatus = L"*PacketException*";
                break;
            }
            for (int i = 0; i < 8; i++)
            {
                bool bPartAvailable = (((ucPartMask >> i) & 1) != 0);
                if (bPartAvailable)
                    nAvailableParts++;
                strPartStatus += bPartAvailable ? _T('#') : _T('.');
                nPart++;
                if (nPart == nPartCount)
                    break;
            }
        }
        strFileStatus.Format(L"Parts=%u  Avail=%u  %s", nPartCount, nAvailableParts, strPartStatus);
    }
    return strFileStatus;
}

CString DbgGetBlockInfo(const Requested_Block_Struct* block)
{
    return DbgGetBlockInfo(block->StartOffset, block->EndOffset);
}

CString DbgGetBlockInfo(uint64 StartOffset, uint64 EndOffset)
{
    CString strInfo;
    strInfo.Format(L"%I64u-%I64u (%I64u bytes)", StartOffset, EndOffset, EndOffset - StartOffset + 1);

    strInfo.AppendFormat(L", Part %I64u", StartOffset/PARTSIZE);
    if (StartOffset/PARTSIZE != EndOffset/PARTSIZE)
        strInfo.AppendFormat(L"-%I64u(**)", EndOffset/PARTSIZE);

    strInfo.AppendFormat(L", Block %I64u", StartOffset/EMBLOCKSIZE);
    if (StartOffset/EMBLOCKSIZE != EndOffset/EMBLOCKSIZE)
    {
        strInfo.AppendFormat(L"-%I64u", EndOffset/EMBLOCKSIZE);
        if (EndOffset/EMBLOCKSIZE - StartOffset/EMBLOCKSIZE > 1)
            strInfo += L"(**)";
    }

    return strInfo;
}

CString DbgGetBlockFileInfo(const Requested_Block_Struct* block, const CPartFile* partfile)
{
    CString strInfo(DbgGetBlockInfo(block));
    strInfo += L"; ";
    strInfo += DbgGetFileInfo(partfile ? partfile->GetFileHash() : NULL);
    return strInfo;
}

int GetHashType(const uchar* hash)
{
    if (hash[5] == 13 && hash[14] == 110)
        return SO_OLDEMULE;
    else if (hash[5] == 14 && hash[14] == 111)
        return SO_EMULE;
    else if (hash[5] == 'M' && hash[14] == 'L')
        return SO_MLDONKEY;
    else
        return SO_UNKNOWN;
}

LPCTSTR DbgGetHashTypeString(const uchar* hash)
{
    int iHashType = GetHashType(hash);
    if (iHashType == SO_EMULE)
        return L"eMule";
    if (iHashType == SO_MLDONKEY)
        return L"MLdonkey";
    if (iHashType == SO_OLDEMULE)
        return L"Old eMule";
    ASSERT(iHashType == SO_UNKNOWN);
    return L"Unknown";
}

CString DbgGetClientID(UINT nClientID)
{
    CString strClientID;
    if (IsLowID(nClientID))
        strClientID.Format(L"LowID=%u", nClientID);
    else
        strClientID = ipstr(nClientID);
    return strClientID;
}

#define _STRVAL(o)	{_T(#o), o}

CString DbgGetDonkeyClientTCPOpcode(UINT opcode)
{
    static const struct
    {
        LPCTSTR pszOpcode;
        UINT uOpcode;
    } _aOpcodes[] =
    {
        _STRVAL(OP_HELLO),
        _STRVAL(OP_SENDINGPART),
        _STRVAL(OP_REQUESTPARTS),
        _STRVAL(OP_FILEREQANSNOFIL),
        _STRVAL(OP_END_OF_DOWNLOAD),
        _STRVAL(OP_ASKSHAREDFILES),
        _STRVAL(OP_ASKSHAREDFILESANSWER),
        _STRVAL(OP_HELLOANSWER),
        _STRVAL(OP_CHANGE_CLIENT_ID),
        _STRVAL(OP_MESSAGE),
        _STRVAL(OP_SETREQFILEID),
        _STRVAL(OP_FILESTATUS),
        _STRVAL(OP_HASHSETREQUEST),
        _STRVAL(OP_HASHSETANSWER),
        _STRVAL(OP_STARTUPLOADREQ),
        _STRVAL(OP_ACCEPTUPLOADREQ),
        _STRVAL(OP_CANCELTRANSFER),
        _STRVAL(OP_OUTOFPARTREQS),
        _STRVAL(OP_REQUESTFILENAME),
        _STRVAL(OP_REQFILENAMEANSWER),
        _STRVAL(OP_CHANGE_SLOT),
        _STRVAL(OP_QUEUERANK),
        _STRVAL(OP_ASKSHAREDDIRS),
        _STRVAL(OP_ASKSHAREDFILESDIR),
        _STRVAL(OP_ASKSHAREDDIRSANS),
        _STRVAL(OP_ASKSHAREDFILESDIRANS),
        _STRVAL(OP_ASKSHAREDDENIEDANS)
    };

    for (int i = 0; i < _countof(_aOpcodes); i++)
    {
        if (_aOpcodes[i].uOpcode == opcode)
            return _aOpcodes[i].pszOpcode;
    }
    CString strOpcode;
    strOpcode.Format(L"0x%02x", opcode);
    return strOpcode;
}

CString DbgGetMuleClientTCPOpcode(UINT opcode)
{
    static const struct
    {
        LPCTSTR pszOpcode;
        UINT uOpcode;
    } _aOpcodes[] =
    {
        _STRVAL(OP_EMULEINFO),
        _STRVAL(OP_EMULEINFOANSWER),
        _STRVAL(OP_COMPRESSEDPART),
        _STRVAL(OP_QUEUERANKING),
        _STRVAL(OP_FILEDESC),
        _STRVAL(OP_REQUESTSOURCES),
        _STRVAL(OP_ANSWERSOURCES),
        _STRVAL(OP_REQUESTSOURCES2),
        _STRVAL(OP_ANSWERSOURCES2),
        _STRVAL(OP_PUBLICKEY),
        _STRVAL(OP_SIGNATURE),
        _STRVAL(OP_SECIDENTSTATE),
        _STRVAL(OP_REQUESTPREVIEW),
        _STRVAL(OP_PREVIEWANSWER),
        _STRVAL(OP_MULTIPACKET),
        _STRVAL(OP_MULTIPACKETANSWER),
        _STRVAL(OP_PEERCACHE_QUERY),
        _STRVAL(OP_PEERCACHE_ANSWER),
        _STRVAL(OP_PEERCACHE_ACK),
        _STRVAL(OP_PUBLICIP_ANSWER),
        _STRVAL(OP_PUBLICIP_REQ),
        _STRVAL(OP_PORTTEST),
        _STRVAL(OP_CALLBACK),
        _STRVAL(OP_BUDDYPING),
        _STRVAL(OP_BUDDYPONG),
        _STRVAL(OP_REASKCALLBACKTCP),
        _STRVAL(OP_AICHANSWER),
        _STRVAL(OP_AICHREQUEST),
        _STRVAL(OP_AICHFILEHASHANS),
        _STRVAL(OP_AICHFILEHASHREQ),
        _STRVAL(OP_COMPRESSEDPART_I64),
        _STRVAL(OP_SENDINGPART_I64),
        _STRVAL(OP_REQUESTPARTS_I64),
        _STRVAL(OP_MULTIPACKET_EXT),
        _STRVAL(OP_CHATCAPTCHAREQ),
        _STRVAL(OP_CHATCAPTCHARES),
        _STRVAL(OP_FWCHECKUDPREQ),
        _STRVAL(OP_KAD_FWTCPCHECK_ACK)
    };

    for (int i = 0; i < _countof(_aOpcodes); i++)
    {
        if (_aOpcodes[i].uOpcode == opcode)
            return _aOpcodes[i].pszOpcode;
    }
    CString strOpcode;
    strOpcode.Format(L"0x%02x", opcode);
    return strOpcode;
}

//>>> WiZaRd::ModProt
CString		DbgGetModTCPOpcode(const UINT opcode)
{
    static const struct
    {
        LPCTSTR pszOpcode;
        UINT uOpcode;
    } _aOpcodes[] =
    {
        _STRVAL(OP_MODINFOPACKET)
    };

    for (int i = 0; i < _countof(_aOpcodes); ++i)
    {
        if (_aOpcodes[i].uOpcode == opcode)
            return _aOpcodes[i].pszOpcode;
    }
    CString strOpcode;
    strOpcode.Format(L"0x%02x", opcode);
    return strOpcode;
}
//<<< WiZaRd::ModProt
#undef _STRVAL

CString DbgGetClientTCPPacket(UINT protocol, UINT opcode, UINT size)
{
    CString str;
    if (protocol == OP_EDONKEYPROT)
        str.Format(L"protocol=eDonkey  opcode=%s  size=%u", DbgGetDonkeyClientTCPOpcode(opcode), size);
    else if (protocol == OP_PACKEDPROT)
        str.Format(L"protocol=Packed  opcode=%s  size=%u", DbgGetMuleClientTCPOpcode(opcode), size);
    else if (protocol == OP_EMULEPROT)
        str.Format(L"protocol=eMule  opcode=%s  size=%u", DbgGetMuleClientTCPOpcode(opcode), size);
//>>> WiZaRd::ModProt
    else if (protocol == OP_MODPROT_PACKED)
        str.Format(L"protocol=Packed ModProt opcode=%s size=%u", DbgGetModTCPOpcode(opcode), size);
    else if (protocol == OP_MODPROT)
        str.Format(L"protocol=ModProt opcode=%s size=%u", DbgGetModTCPOpcode(opcode), size);
//<<< WiZaRd::ModProt
    else
        str.Format(L"protocol=0x%02x  opcode=0x%02x  size=%u", protocol, opcode, size);
    return str;
}

CString DbgGetClientTCPOpcode(UINT protocol, UINT opcode)
{
    CString str;
    if (protocol == OP_EDONKEYPROT)
        str.Format(L"%s", DbgGetDonkeyClientTCPOpcode(opcode));
    else if (protocol == OP_PACKEDPROT)
        str.Format(L"%s", DbgGetMuleClientTCPOpcode(opcode));
    else if (protocol == OP_EMULEPROT)
        str.Format(L"%s", DbgGetMuleClientTCPOpcode(opcode));
//>>> WiZaRd::ModProt
    else if (protocol == OP_MODPROT_PACKED)
        str.Format(L"protocol=Packed ModProt opcode=%s", DbgGetModTCPOpcode(opcode));
    else if (protocol == OP_MODPROT)
        str.Format(L"protocol=ModProt opcode=%s", DbgGetModTCPOpcode(opcode));
//<<< WiZaRd::ModProt
    else
        str.Format(L"protocol=0x%02x  opcode=0x%02x", protocol, opcode);
    return str;
}

void DebugRecv(LPCSTR pszMsg, const CUpDownClient* client, const uchar* packet, UINT nIP)
{
    // 111.222.333.444 = 15 chars
    if (client)
    {
        if (client != NULL && packet != NULL)
            Debug(L"<<< %-24hs from %s; %s\n", pszMsg, client->DbgGetClientInfo(true), DbgGetFileInfo(packet));
        else if (client != NULL && packet == NULL)
            Debug(L"<<< %-24hs from %s\n", pszMsg, client->DbgGetClientInfo(true));
        else if (client == NULL && packet != NULL)
            Debug(L"<<< %-24hs; %s\n", pszMsg, DbgGetFileInfo(packet));
        else
            Debug(L"<<< %-24hs\n", pszMsg);
    }
    else
    {
        if (nIP != 0 && packet != NULL)
            Debug(L"<<< %-24hs from %-15s; %s\n", pszMsg, ipstr(nIP), DbgGetFileInfo(packet));
        else if (nIP != 0 && packet == NULL)
            Debug(L"<<< %-24hs from %-15s\n", pszMsg, ipstr(nIP));
        else if (nIP == 0 && packet != NULL)
            Debug(L"<<< %-24hs; %s\n", pszMsg, DbgGetFileInfo(packet));
        else
            Debug(L"<<< %-24hs\n", pszMsg);
    }
}

void DebugSend(LPCSTR pszMsg, const CUpDownClient* client, const uchar* packet)
{
    if (client != NULL && packet != NULL)
        Debug(L">>> %-20hs to   %s; %s\n", pszMsg, client->DbgGetClientInfo(true), DbgGetFileInfo(packet));
    else if (client != NULL && packet == NULL)
        Debug(L">>> %-20hs to   %s\n", pszMsg, client->DbgGetClientInfo(true));
    else if (client == NULL && packet != NULL)
        Debug(L">>> %-20hs; %s\n", pszMsg, DbgGetFileInfo(packet));
    else
        Debug(L">>> %-20hs\n", pszMsg);
}

void DebugSend(LPCSTR pszOpcode, UINT ip, uint16 port)
{
    TCHAR szIPPort[22];
    _stprintf(szIPPort, L"%s:%u", ipstr(ntohl(ip)), port);
    Debug(L">>> %-20hs to   %-21s\n", pszOpcode, szIPPort);
}

void DebugSendF(LPCSTR pszOpcode, UINT ip, uint16 port, LPCTSTR pszMsg, ...)
{
    va_list args;
    va_start(args, pszMsg);
    TCHAR szIPPort[22];
    _stprintf(szIPPort, L"%s:%u", ipstr(ntohl(ip)), port);
    CString str;
    str.Format(L">>> %-20hs to   %-21s; ", pszOpcode, szIPPort);
    str.AppendFormatV(pszMsg, args);
    va_end(args);
    Debug(L"%s\n", str);
}

void DebugRecv(LPCSTR pszOpcode, UINT ip, uint16 port)
{
    TCHAR szIPPort[22];
    _stprintf(szIPPort, L"%s:%u", ipstr(ntohl(ip)), port);
    Debug(L"%-24hs from %-21s\n", pszOpcode, szIPPort);
}

void DebugHttpHeaders(const CStringAArray& astrHeaders)
{
    for (int i = 0; i < astrHeaders.GetCount(); i++)
    {
        const CStringA& rstrHdr = astrHeaders.GetAt(i);
        Debug(L"<%hs\n", rstrHdr);
    }
}

ULONGLONG GetDiskFileSize(LPCTSTR pszFilePath)
{
    static BOOL _bInitialized = FALSE;
    static DWORD (WINAPI *_pfnGetCompressedFileSize)(LPCTSTR, LPDWORD) = NULL;

    if (!_bInitialized)
    {
        _bInitialized = TRUE;
        (FARPROC&)_pfnGetCompressedFileSize = GetProcAddress(GetModuleHandle(L"kernel32"), _TWINAPI("GetCompressedFileSize"));
    }

    // If the file is not compressed nor sparse, 'GetCompressedFileSize' returns the 'normal' file size.
    if (_pfnGetCompressedFileSize)
    {
        ULONGLONG ullCompFileSize;
        ((LPDWORD)&ullCompFileSize)[0] = (*_pfnGetCompressedFileSize)(pszFilePath, &((LPDWORD)&ullCompFileSize)[1]);
        if (((LPDWORD)&ullCompFileSize)[0] != INVALID_FILE_SIZE || GetLastError() == NO_ERROR)
            return ullCompFileSize;
    }

    // If 'GetCompressedFileSize' failed or is not available, use the default function
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(pszFilePath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;
    FindClose(hFind);

    return (ULONGLONG)fd.nFileSizeHigh << 32 | (ULONGLONG)fd.nFileSizeLow;
}

// Listview helper function
void GetPopupMenuPos(CListCtrl& lv, CPoint& point)
{
    // If the context menu was not opened using the right mouse button,
    // but the keyboard (Shift+F10), get a useful position for the context menu.
    if (point.x == -1 && point.y == -1)
    {
        int iIdxItem = lv.GetNextItem(-1, LVNI_SELECTED | LVNI_FOCUSED);
        if (iIdxItem != -1)
        {
            CRect rc;
            if (lv.GetItemRect(iIdxItem, &rc, LVIR_BOUNDS))
            {
                point.x = rc.left + lv.GetColumnWidth(0) / 2;
                point.y = rc.top + rc.Height() / 2;
                lv.ClientToScreen(&point);
            }
        }
        else
        {
            point.x = 16;
            point.y = 32;
            lv.ClientToScreen(&point);
        }
    }
}

void GetPopupMenuPos(CTreeCtrl& tv, CPoint& point)
{
    // If the context menu was not opened using the right mouse button,
    // but the keyboard (Shift+F10), get a useful position for the context menu.
    if (point.x == -1 && point.y == -1)
    {
        HTREEITEM hSel = tv.GetNextItem(TVI_ROOT, TVGN_CARET);
        if (hSel)
        {
            CRect rcItem;
            if (tv.GetItemRect(hSel, &rcItem, TRUE))
            {
                point.x = rcItem.left;
                point.y = rcItem.top;
                tv.ClientToScreen(&point);
            }
        }
        else
        {
            point.x = 16;
            point.y = 32;
            tv.ClientToScreen(&point);
        }
    }
}

time_t safe_mktime(struct tm* ptm)
{
    if (ptm == NULL)
        return -1;
    return mktime(ptm);
}

CString StripInvalidFilenameChars(const CString& strText)
{
    LPCTSTR pszSource = strText;
    CString strDest;

    while (*pszSource != L'\0')
    {
        if (!(((_TUCHAR)*pszSource >= 0 && (_TUCHAR)*pszSource <= 31) ||
                // lots of invalid chars for filenames in windows :=)
                *pszSource == _T('\"') || *pszSource == _T('*') || *pszSource == _T('<')  || *pszSource == _T('>') ||
                *pszSource == _T('?')  || *pszSource == _T('|') || *pszSource == _T('\\') || *pszSource == _T('/') ||
                *pszSource == _T(':')))
        {
            strDest += *pszSource;
        }
        pszSource++;
    }

    static const LPCTSTR apszReservedFilenames[] =
    {
        L"NUL", L"CON", L"PRN", L"AUX", L"CLOCK$",
        L"COM1",L"COM2",L"COM3",L"COM4",L"COM5",L"COM6",L"COM7",L"COM8",L"COM9",
        L"LPT1",L"LPT2",L"LPT3",L"LPT4",L"LPT5",L"LPT6",L"LPT7",L"LPT8",L"LPT9"
    };
    for (int i = 0; i < _countof(apszReservedFilenames); i++)
    {
        int nPrefixLen = _tcslen(apszReservedFilenames[i]);
        if (_tcsnicmp(strDest, apszReservedFilenames[i], nPrefixLen) == 0)
        {
            if (strDest.GetLength() == nPrefixLen)
            {
                // Filename is a reserved file name:
                // Append an underscore character
                strDest += L"_";
                break;
            }
            else if (strDest[nPrefixLen] == _T('.'))
            {
                // Filename starts with a reserved file name followed by a '.' character:
                // Replace that ',' character with an '_' character.
                LPTSTR pszDest = strDest.GetBuffer(strDest.GetLength());
                pszDest[nPrefixLen] = _T('_');
                strDest.ReleaseBuffer(strDest.GetLength());
                break;
            }
        }
    }

    return strDest;
}

bool operator==(const CCKey& k1,const CCKey& k2)
{
    return !md4cmp(k1.m_key, k2.m_key);
}

bool operator==(const CSKey& k1,const CSKey& k2)
{
    return !md4cmp(k1.m_key, k2.m_key);
}

CString ipstr(UINT nIP)
{
    // following gives the same string as 'inet_ntoa(*(in_addr*)&nIP)' but is not restricted to ASCII strings
    const BYTE* pucIP = (BYTE*)&nIP;
    CString strIP;
    strIP.ReleaseBuffer(_stprintf(strIP.GetBuffer(3+1+3+1+3+1+3), L"%u.%u.%u.%u", pucIP[0], pucIP[1], pucIP[2], pucIP[3]));
    return strIP;
}

CString ipstr(UINT nIP, uint16 nPort)
{
    // following gives the same string as 'inet_ntoa(*(in_addr*)&nIP)' but is not restricted to ASCII strings
    const BYTE* pucIP = (BYTE*)&nIP;
    CString strIP;
    strIP.ReleaseBuffer(_stprintf(strIP.GetBuffer(3+1+3+1+3+1+3+1+5), L"%u.%u.%u.%u:%u", pucIP[0], pucIP[1], pucIP[2], pucIP[3], nPort));
    return strIP;
}

CString ipstr(LPCTSTR pszAddress, uint16 nPort)
{
    CString strIPPort;
    strIPPort.Format(L"%s:%u", pszAddress, nPort);
    return strIPPort;
}

CStringA ipstrA(UINT nIP)
{
    const BYTE* pucIP = (BYTE*)&nIP;
    CStringA strIP;
    strIP.ReleaseBuffer(sprintf(strIP.GetBuffer(3+1+3+1+3+1+3), "%u.%u.%u.%u", pucIP[0], pucIP[1], pucIP[2], pucIP[3]));
    return strIP;
}

void ipstrA(CHAR* pszAddress, int iMaxAddress, UINT nIP)
{
    const BYTE* pucIP = (BYTE*)&nIP;
    _snprintf(pszAddress, iMaxAddress, "%u.%u.%u.%u", pucIP[0], pucIP[1], pucIP[2], pucIP[3]);
}

bool IsDaylightSavingTimeActive(LONG& rlDaylightBias)
{
    TIME_ZONE_INFORMATION tzi;
    if (GetTimeZoneInformation(&tzi) != TIME_ZONE_ID_DAYLIGHT)
        return false;
    rlDaylightBias = tzi.DaylightBias;
    return true;
}

class CVolumeInfo
{
  public:
    static bool IsNTFSVolume(LPCTSTR pszVolume)
    {
        return (_tcscmp(GetVolumeInfo(pszVolume), L"NTFS") == 0);
    }

    static bool IsFATVolume(LPCTSTR pszVolume)
    {
        return (_tcsnicmp(GetVolumeInfo(pszVolume), L"FAT", 3) == 0);
    }

    static const CString& GetVolumeInfo(LPCTSTR pszVolume)
    {
        CString strVolumeId(pszVolume);
        ASSERT(strVolumeId.GetLength() > 0 && strVolumeId[strVolumeId.GetLength() - 1] == _T('\\'));
        strVolumeId.MakeLower();
        CMapStringToString::CPair *pPair = m_mapVolumeInfo.PLookup(strVolumeId);
        if (pPair != NULL)
            return pPair->value;

        // 'GetVolumeInformation' may cause a noticable delay - depending on the type of volume
        // which is queried. As we are using that function for almost every file (for compensating
        // the NTFS file time issues), we need to cash this information.
        //
        // The cash gets cleared when the user manually hits the 'Reload' button in the 'Shared
        // Files' window and when Windows broadcasts a message about that a volume was mounted/unmounted.
        //
        DWORD dwMaximumComponentLength = 0;
        DWORD dwFileSystemFlags = 0;
        TCHAR szFileSystemNameBuffer[MAX_PATH + 1];
        if (!GetVolumeInformation(pszVolume, NULL, 0, NULL, &dwMaximumComponentLength, &dwFileSystemFlags, szFileSystemNameBuffer, _countof(szFileSystemNameBuffer)))
            szFileSystemNameBuffer[0] = L'\0';
        return m_mapVolumeInfo[strVolumeId] = szFileSystemNameBuffer;
    }

    static void ClearCache(int iDrive)
    {
        if (iDrive == -1)
            m_mapVolumeInfo.RemoveAll();
        else
        {
            TCHAR szRoot[MAX_PATH];
            szRoot[0] = L'\0';
            PathBuildRoot(szRoot, iDrive);
            if (szRoot[0] != L'\0')
            {
                CString strVolumeId(szRoot);
                ASSERT(strVolumeId.GetLength() > 0 && strVolumeId[strVolumeId.GetLength() - 1] == _T('\\'));
                strVolumeId.MakeLower();
                m_mapVolumeInfo.RemoveKey(strVolumeId);
            }
        }
    }

  protected:
    static CMapStringToString m_mapVolumeInfo;
};

static CVolumeInfo g_VolumeInfo;
CMapStringToString CVolumeInfo::m_mapVolumeInfo;

bool IsNTFSVolume(LPCTSTR pszVolume)
{
    return g_VolumeInfo.IsNTFSVolume(pszVolume);
}

bool IsFATVolume(LPCTSTR pszVolume)
{
    return g_VolumeInfo.IsFATVolume(pszVolume);
}

void ClearVolumeInfoCache(int iDrive)
{
    g_VolumeInfo.ClearCache(iDrive);
}

bool IsFileOnNTFSVolume(LPCTSTR pszFilePath)
{
    CString strRootPath(pszFilePath);
    BOOL bResult = PathStripToRoot(strRootPath.GetBuffer());
    strRootPath.ReleaseBuffer();
    if (!bResult)
        return false;
    // Need to add a trailing backslash in case of a network share
    if (!strRootPath.IsEmpty() && strRootPath[strRootPath.GetLength() - 1] != _T('\\'))
    {
        PathAddBackslash(strRootPath.GetBuffer(strRootPath.GetLength() + 1));
        strRootPath.ReleaseBuffer();
    }
    return IsNTFSVolume(strRootPath);
}

bool IsFileOnFATVolume(LPCTSTR pszFilePath)
{
    CString strRootPath(pszFilePath);
    BOOL bResult = PathStripToRoot(strRootPath.GetBuffer());
    strRootPath.ReleaseBuffer();
    if (!bResult)
        return false;
    // Need to add a trailing backslash in case of a network share
    if (!strRootPath.IsEmpty() && strRootPath[strRootPath.GetLength() - 1] != _T('\\'))
    {
        PathAddBackslash(strRootPath.GetBuffer(strRootPath.GetLength() + 1));
        strRootPath.ReleaseBuffer();
    }
    return IsFATVolume(strRootPath);
}

bool IsAutoDaylightTimeSetActive()
{
    CRegKey key;
    if (key.Open(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation", KEY_READ) == ERROR_SUCCESS)
    {
        DWORD dwDisableAutoDaylightTimeSet = 0;
        if (key.QueryDWORDValue(L"DisableAutoDaylightTimeSet", dwDisableAutoDaylightTimeSet) == ERROR_SUCCESS)
        {
            if (dwDisableAutoDaylightTimeSet)
                return false;
        }
    }
    return true; // default to 'Automatically adjust clock for daylight saving changes'
}

bool AdjustNTFSDaylightFileTime(UINT& ruFileDate, LPCTSTR pszFilePath)
{
    if (!thePrefs.GetAdjustNTFSDaylightFileTime())
        return false;
    if (ruFileDate == 0 || ruFileDate == -1)
        return false;

    // See also KB 129574
    LONG lDaylightBias = 0;
    if (IsDaylightSavingTimeActive(lDaylightBias))
    {
        if (IsAutoDaylightTimeSetActive())
        {
            if (IsFileOnNTFSVolume(pszFilePath))
            {
                ruFileDate += lDaylightBias*60;
                return true;
            }
        }
        else
        {
            // If 'Automatically adjust clock for daylight saving changes' is disabled and
            // if the file's date is within DST period, we get again a wrong file date.
            //
            // If 'Automatically adjust clock for daylight saving changes' is disabled,
            // Windows always reports 'Active DST'(!!) with a bias of '0' although there is no
            // DST specified. This means also, that there is no chance to determine if a date is
            // within any DST.

            // Following code might be correct, but because we don't have a DST and because we don't have a bias,
            // the code won't do anything useful.
            /*struct tm* ptm = localtime((time_t*)&ruFileDate);
            bool bFileDateInDST = (ptm && ptm->tm_isdst == 1);
            if (bFileDateInDST)
            {
            	ruFileDate += lDaylightBias*60;
            	return true;
            }*/
        }
    }

    return false;
}

bool ExpandEnvironmentStrings(CString& rstrStrings)
{
    DWORD dwSize = ExpandEnvironmentStrings(rstrStrings, NULL, 0);
    if (dwSize == 0)
        return false;

    CString strExpanded;
    DWORD dwCount = ExpandEnvironmentStrings(rstrStrings, strExpanded.GetBuffer(dwSize-1), dwSize);
    if (dwCount == 0 || dwCount != dwSize)
    {
        ASSERT(0);
        return false;
    }
    strExpanded.ReleaseBuffer(dwCount-1);
    rstrStrings = strExpanded;
    return true;
}

uint16 GetRandomUInt16()
{
#if RAND_MAX == 0x7fff
    // get 2 random numbers
    UINT uRand0 = rand();
    UINT uRand1 = rand();

    // NOTE: if that assert fires, you have most likely called that function *without* calling 'srand' first.
    // NOTE: each spawned thread HAS to call 'srand' for itself to get real random numbers.
    ASSERT(!(uRand0 == 41 && uRand1 == 18467));

    return (uint16)(uRand0 | ((uRand1 >= RAND_MAX/2) ? 0x8000 : 0x0000));
#else
#error "Implement it!"
#endif
}

UINT GetRandomUInt32()
{
#if RAND_MAX == 0x7fff
    //return ((UINT)GetRandomUInt16() << 16) | (UINT)GetRandomUInt16();
    // this would give the receiver the following information:
    //	random number N
    //	random number N+1 is below or greater/equal than 0x8000
    //	random number N+2
    //	random number N+3 is below or greater/equal than 0x8000

    UINT uRand0 = GetRandomUInt16();
    srand(GetTickCount() | uRand0);
    UINT uRand1 = GetRandomUInt16();
    return (uRand0 << 16) | uRand1;
#else
#error "Implement it!"
#endif
}

HWND ReplaceRichEditCtrl(CWnd* pwndRE, CWnd* pwndParent, CFont* pFont)
{
    HWND hwndNewRE = NULL;

    ASSERT(pwndRE);
    if (pwndRE)
    {
        CHAR szClassName[MAX_PATH];
        if (GetClassNameA(*pwndRE, szClassName, _countof(szClassName)) && __ascii_stricmp(szClassName, "RichEdit20W")==0)
            return NULL;

        CRect rcWnd;
        pwndRE->GetWindowRect(&rcWnd);

        DWORD dwStyle = pwndRE->GetStyle();
        dwStyle |= WS_VSCROLL | WS_HSCROLL;
        DWORD dwExStyle = pwndRE->GetExStyle();

        CString strText;
        pwndRE->GetWindowText(strText);

        UINT uCtrlID = GetWindowLong(*pwndRE, GWL_ID);

        pwndRE->DestroyWindow();
        pwndRE = NULL;

        pwndParent->ScreenToClient(&rcWnd);
        hwndNewRE = CreateWindowEx(dwExStyle, RICHEDIT_CLASS, strText, dwStyle, rcWnd.left, rcWnd.top, rcWnd.Width(), rcWnd.Height(), pwndParent->m_hWnd, (HMENU)uCtrlID, NULL, NULL);
        if (hwndNewRE && pFont && pFont->m_hObject)
            ::SendMessage(hwndNewRE, WM_SETFONT, (WPARAM)pFont->m_hObject, 0);
    }
    return hwndNewRE;
}

void InstallSkin(LPCTSTR pszSkinPackage)
{
    if (thePrefs.GetMuleDirectory(EMULE_SKINDIR).IsEmpty() || _taccess(thePrefs.GetMuleDirectory(EMULE_SKINDIR), 0) != 0)
    {
        AfxMessageBox(GetResString(IDS_INSTALL_SKIN_NODIR), MB_ICONERROR);
        return;
    }

    static const TCHAR _szSkinSuffix[] = L"." EMULSKIN_BASEEXT L".ini";

    TCHAR szExt[_MAX_EXT];
    _tsplitpath(pszSkinPackage, NULL, NULL, NULL, szExt);
    _tcslwr(szExt);

    if (_tcscmp(szExt, L".zip") == 0)
    {
        CZIPFile zip;
        if (zip.Open(pszSkinPackage))
        {
            // Search the "*.eMuleSkin.ini" file..
            CZIPFile::File* zfIniFile = NULL;
            for (int i = 0; i < zip.GetCount(); i++)
            {
                CZIPFile::File* zf = zip.GetFile(i);
                if (zf && zf->m_sName.Right(_countof(_szSkinSuffix)-1).CompareNoCase(_szSkinSuffix) == 0)
                {
                    zfIniFile = zf;
                    break;
                }
            }

            if (zfIniFile)
            {
                for (int i = 0; i < zip.GetCount(); i++)
                {
                    CZIPFile::File* zf = zip.GetFile(i);
                    if (zf)
                    {
                        if (zf->m_sName.IsEmpty())
                            continue;
                        if (zf->m_sName[0] == _T('\\') || zf->m_sName[0] == _T('/'))
                            continue;
                        if (zf->m_sName.Find(_T(':')) != -1)
                            continue;
                        if (zf->m_sName.Find(L"..\\") != -1 || zf->m_sName.Find(L"../") != -1)
                            continue;
                        if (zf->m_sName[zf->m_sName.GetLength()-1] == _T('/'))
                        {
                            CString strDstDirPath;
                            PathCanonicalize(strDstDirPath.GetBuffer(MAX_PATH), thePrefs.GetMuleDirectory(EMULE_SKINDIR) + _T('\\') + zf->m_sName.Left(zf->m_sName.GetLength()-1));
                            strDstDirPath.ReleaseBuffer();
                            if (!CreateDirectory(strDstDirPath, NULL))
                            {
                                DWORD dwError = GetLastError();
                                CString strError;
                                strError.Format(GetResString(IDS_INSTALL_SKIN_DIR_ERROR), strDstDirPath, GetErrorMessage(dwError));
                                AfxMessageBox(strError, MB_ICONERROR);
                                break;
                            }
                        }
                        else
                        {
                            CString strDstFilePath;
                            PathCanonicalize(strDstFilePath.GetBuffer(MAX_PATH), thePrefs.GetMuleDirectory(EMULE_SKINDIR) + _T('\\') + zf->m_sName);
                            strDstFilePath.ReleaseBuffer();
                            SetLastError(0);
                            if (!zf->Extract(strDstFilePath))
                            {
                                DWORD dwError = GetLastError();
                                CString strError;
                                strError.Format(GetResString(IDS_INSTALL_SKIN_FILE_ERROR), zf->m_sName, strDstFilePath, GetErrorMessage(dwError));
                                AfxMessageBox(strError, MB_ICONERROR);
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                AfxMessageBox(GetResString(IDS_INSTALL_SKIN_PKG_ERROR), MB_ICONERROR);
            }
            zip.Close();
        }
        else
        {
            AfxMessageBox(GetResString(IDS_INSTALL_SKIN_PKG_ERROR), MB_ICONERROR);
        }
    }
    else if (_tcscmp(szExt, L".rar") == 0)
    {
        CRARFile rar;
        if (rar.Open(pszSkinPackage))
        {
            bool bError = false;
            bool bFoundSkinINIFile = false;
            CString strFileName;
            while (rar.GetNextFile(strFileName))
            {
                if (strFileName.IsEmpty())
                {
                    rar.Skip();
                    continue;
                }
                if (strFileName[0] == _T('\\') || strFileName[0] == _T('/'))
                {
                    rar.Skip();
                    continue;
                }
                if (strFileName.Find(_T(':')) != -1)
                {
                    rar.Skip();
                    continue;
                }
                if (strFileName.Find(L"..\\") != -1 || strFileName.Find(L"../") != -1)
                {
                    rar.Skip();
                    continue;
                }

                if (!bFoundSkinINIFile && strFileName.Right(_countof(_szSkinSuffix)-1).CompareNoCase(_szSkinSuffix) == 0)
                    bFoundSkinINIFile = true;

                // No need to care about possible available sub-directories. UnRAR.DLL cares about that automatically.
                CString strDstFilePath;
                PathCanonicalize(strDstFilePath.GetBuffer(MAX_PATH), thePrefs.GetMuleDirectory(EMULE_SKINDIR) + _T('\\') + strFileName);
                strDstFilePath.ReleaseBuffer();
                SetLastError(0);
                if (!rar.Extract(strDstFilePath))
                {
                    DWORD dwError = GetLastError();
                    CString strError;
                    strError.Format(GetResString(IDS_INSTALL_SKIN_FILE_ERROR), strFileName, strDstFilePath, GetErrorMessage(dwError));
                    AfxMessageBox(strError, MB_ICONERROR);
                    bError = true;
                    break;
                }
            }

            if (!bError && !bFoundSkinINIFile)
                AfxMessageBox(GetResString(IDS_INSTALL_SKIN_PKG_ERROR), MB_ICONERROR);

            rar.Close();
        }
        else
        {
            CString strError;
            strError.Format(L"%s\r\n\r\nDownload latest version of UNRAR.DLL from http://www.rarlab.com and copy UNRAR.DLL into eMule installation folder.", GetResString(IDS_INSTALL_SKIN_PKG_ERROR));
            AfxMessageBox(strError, MB_ICONERROR);
        }
    }
}

void TriggerPortTest(uint16 tcp, uint16 udp)
{
    CString m_sTestURL;

    // do not alter the connection test, this is a manual test only. If you want to change the behavior, use your server!
    m_sTestURL.Format(PORTTESTURL, tcp, udp , thePrefs.GetLanguageID());

    // the portcheck will need to do an obfuscated callback too if obfuscation is requested, so we have to provide our userhash so it can create the key
    if (thePrefs.IsClientCryptLayerRequested())
        m_sTestURL += L"&obfuscated_test=" + md4str(thePrefs.GetUserHash());

    ShellOpenFile(m_sTestURL);
}

int CompareLocaleStringNoCase(LPCTSTR psz1, LPCTSTR psz2)
{
    // SDK says: The 'CompareString' function is optimized to run at the highest speed when 'dwCmpFlags' is set to 0
    // or NORM_IGNORECASE, and 'cchCount1' and 'cchCount2' have the value -1.
    int iResult = CompareString(GetThreadLocale(), NORM_IGNORECASE, psz1, -1, psz2, -1);
    if (iResult == 0)
        return 0;
    return iResult - 2;
}

int CompareLocaleString(LPCTSTR psz1, LPCTSTR psz2)
{
    // SDK says: The 'CompareString' function is optimized to run at the highest speed when 'dwCmpFlags' is set to 0
    // or NORM_IGNORECASE, and 'cchCount1' and 'cchCount2' have the value -1.
    int iResult = CompareString(GetThreadLocale(), 0, psz1, -1, psz2, -1);
    if (iResult == 0)
        return 0;
    return iResult - 2;
}

int __cdecl CompareCStringPtrLocaleStringNoCase(const void* p1, const void* p2)
{
    const CString* pstr1 = (const CString*)p1;
    const CString* pstr2 = (const CString*)p2;
    return CompareLocaleStringNoCase(*pstr1, *pstr2);
}

int __cdecl CompareCStringPtrLocaleString(const void* p1, const void* p2)
{
    const CString* pstr1 = (const CString*)p1;
    const CString* pstr2 = (const CString*)p2;
    return CompareLocaleString(*pstr1, *pstr2);
}

void Sort(CStringArray& astr, int (__cdecl *pfnCompare)(const void*, const void*))
{
    qsort(astr.GetData(), astr.GetCount(), sizeof(CString*), pfnCompare);
}

int __cdecl CompareCStringPtrPtrLocaleStringNoCase(const void* p1, const void* p2)
{
    const CString* pstr1 = *(const CString**)p1;
    const CString* pstr2 = *(const CString**)p2;
    return CompareLocaleStringNoCase(*pstr1, *pstr2);
}

int __cdecl CompareCStringPtrPtrLocaleString(const void* p1, const void* p2)
{
    const CString* pstr1 = *(const CString**)p1;
    const CString* pstr2 = *(const CString**)p2;
    return CompareLocaleString(*pstr1, *pstr2);
}

void Sort(CSimpleArray<const CString*>& apstr, int (__cdecl *pfnCompare)(const void*, const void*))
{
    qsort(apstr.GetData(), apstr.GetSize(), sizeof(CString*), pfnCompare);
}

void AddAutoStart()
{
#ifndef _DEBUG
    RemAutoStart();
    CString strKeyName;
    strKeyName = L"eMuleAutoStart";
    TCHAR sExeFilePath[MAX_PATH];
    DWORD dwModPathLen = ::GetModuleFileName(NULL, sExeFilePath, _countof(sExeFilePath));
    if (dwModPathLen == 0 || dwModPathLen == _countof(sExeFilePath))
        return;
    CString sFullExeCommand;
    sFullExeCommand.Format(L"%s -AutoStart", sExeFilePath);
    CRegKey mKey;
    mKey.Create(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                REG_NONE,REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,	NULL,NULL);
    if (mKey != NULL)
    {
        mKey.SetStringValue(strKeyName, sFullExeCommand);
        mKey.Close();
    }
#endif
}

void RemAutoStart()
{
    CString strKeyName;
    strKeyName = L"eMuleAutoStart";
    CRegKey mKey;
    mKey.Create(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                REG_NONE,REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,	NULL,NULL);
    if (mKey != NULL)
    {
        mKey.DeleteValue(strKeyName);
        mKey.Close();
    }
}

int FontPointSizeToLogUnits(int nPointSize)
{
    HDC hDC = ::GetDC(HWND_DESKTOP);
    if (hDC)
    {
        POINT pt;
#if 0
        // This is the same math which is performed by "CFont::CreatePointFont",
        // which is flawed because it does not perform any rounding. But without
        // performing the correct rounding one can not get the correct LOGFONT-height
        // for an 8pt font!
        //
        // PointSize	Result
        // -------------------
        // 8*10			10.666 -> 10 (cut down and thus wrong result)
        pt.y = GetDeviceCaps(hDC, LOGPIXELSY) * nPointSize;
        pt.y /= 720;
#else
        // This math accounts for proper rounding and thus we will get the correct results.
        //
        // PointSize	Result
        // -------------------
        // 8*10			10.666 -> 11 (rounded up and thus correct result)
        pt.y = MulDiv(GetDeviceCaps(hDC, LOGPIXELSY), nPointSize, 720);
#endif
        pt.x = 0;
        DPtoLP(hDC, &pt, 1);
        POINT ptOrg = { 0, 0 };
        DPtoLP(hDC, &ptOrg, 1);
        nPointSize = -abs(pt.y - ptOrg.y);
        ReleaseDC(HWND_DESKTOP, hDC);
    }
    return nPointSize;
}

bool CreatePointFontIndirect(CFont &rFont, const LOGFONT *lpLogFont)
{
    LOGFONT logFont = *lpLogFont;
    logFont.lfHeight = FontPointSizeToLogUnits(logFont.lfHeight);
    return rFont.CreateFontIndirect(&logFont) != FALSE;
}

bool CreatePointFont(CFont &rFont, int nPointSize, LPCTSTR lpszFaceName)
{
    LOGFONT logFont = {0};
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfHeight = nPointSize;
    lstrcpyn(logFont.lfFaceName, lpszFaceName, _countof(logFont.lfFaceName));
    return CreatePointFontIndirect(rFont, &logFont);
}

bool IsUnicodeFile(LPCTSTR pszFilePath)
{
    bool bResult = false;
    FILE* fp = _tfsopen(pszFilePath, L"rb", _SH_DENYWR);
    if (fp != NULL)
    {
        WORD wBOM = 0;
        bResult = (fread(&wBOM, sizeof(wBOM), 1, fp) == 1 && wBOM == 0xFEFF);
        fclose(fp);
    }
    return bResult;
}

bool RegularExpressionMatch(CString regexpr, CString teststring)
{

    CAtlRegExp<> reFN;

    REParseError status = reFN.Parse(regexpr);
    if (REPARSE_ERROR_OK != status)
    {
        // Unexpected error.
        return false;
    }

    CAtlREMatchContext<> mcUrl;
    if (!reFN.Match(
                teststring,
                &mcUrl))
    {
        // Unexpected error.
        return false;
    }
    else
    {
        return true;
    }
}

ULONGLONG GetModuleVersion(LPCTSTR pszFilePath)
{
    ULONGLONG ullVersion = 0;
    DWORD dwUnused;
    DWORD dwVerInfSize = GetFileVersionInfoSize(const_cast<LPTSTR>(pszFilePath), &dwUnused);
    if (dwVerInfSize != 0)
    {
        LPBYTE pucVerInf = (LPBYTE)calloc(dwVerInfSize, 1);
        if (pucVerInf)
        {
            if (GetFileVersionInfo(const_cast<LPTSTR>(pszFilePath), 0, dwVerInfSize, pucVerInf))
            {
                VS_FIXEDFILEINFO* pFileInf = NULL;
                UINT uLen = 0;
                if (VerQueryValue(pucVerInf, L"\\", (LPVOID*)&pFileInf, &uLen) && pFileInf && uLen)
                {
                    ullVersion = MAKEDLLVERULL(HIWORD(pFileInf->dwFileVersionMS), LOWORD(pFileInf->dwFileVersionMS),
                                               HIWORD(pFileInf->dwFileVersionLS), LOWORD(pFileInf->dwFileVersionLS));
                }
            }
            free(pucVerInf);
        }
    }
    return ullVersion;
}

ULONGLONG GetModuleVersion(HMODULE hModule)
{
    TCHAR szFilePath[MAX_PATH];
    DWORD dwModPathLen = GetModuleFileName(hModule, szFilePath, _countof(szFilePath));
    if (dwModPathLen == 0 || dwModPathLen == _countof(szFilePath))
        return 0;
    return GetModuleVersion(szFilePath);
}

int GetPathDriveNumber(CString path)
{
    if (path.GetLength()<3 || path.GetAt(1)!=_T(':') || path.GetAt(2)!=_T('\\'))
        return -1;

    return path.MakeLower().GetAt(0) - 97;
}

UINT64 GetFreeTempSpace(int tempdirindex)
{
    ASSERT(tempdirindex<thePrefs.tempdir.GetCount() && tempdirindex>=-1);
    if (tempdirindex>=thePrefs.tempdir.GetCount() || tempdirindex<-1)
        return 0;

    if (tempdirindex>=0)
        return GetFreeDiskSpaceX(thePrefs.GetTempDir(tempdirindex));

    bool toadd;
    CArray<int> hist;
    UINT64 sum=0;
    for (int  i=0; i<thePrefs.tempdir.GetCount(); i++)
    {
        int pdn=GetPathDriveNumber(thePrefs.GetTempDir(i));
        toadd=true;

        if (pdn>=0)
            for (int i=0; i<hist.GetCount(); i++)
            {
                if (hist.GetAt(i)==pdn)
                {
                    toadd=false;
                    break;
                }
            }
        if (!toadd)
            continue;

        sum+=GetFreeDiskSpaceX(thePrefs.GetTempDir(i));
        hist.Add(pdn);
    }

    return sum;
}

bool DoCollectionRegFix(bool checkOnly)
{
    bool bGlobalSet = false;
    CRegKey regkey;
    LONG result;
    TCHAR modbuffer[MAX_PATH];
    DWORD dwModPathLen = ::GetModuleFileName(NULL, modbuffer, _countof(modbuffer));
    if (dwModPathLen == 0 || dwModPathLen == _countof(modbuffer))
        return false;
    CString strCanonFileName = modbuffer;
    strCanonFileName.Replace(L"%", L"%%");
    CString regbuffer;
    regbuffer.Format(L"\"%s\" \"%%1\"", strCanonFileName);

    // first check if the registry keys are already set (either by installer in HKLM or by user in HKCU)
    result = regkey.Open(HKEY_CLASSES_ROOT, L"eMule\\shell\\open\\command", KEY_READ);
    if (result == ERROR_SUCCESS)
    {
        TCHAR rbuffer[MAX_PATH + 100];
        ULONG maxsize = _countof(rbuffer);
        regkey.QueryStringValue(NULL, rbuffer, &maxsize);
        rbuffer[_countof(rbuffer) - 1] = L'\0';
        if (maxsize != 0 && _tcsicmp(rbuffer, regbuffer) == 0)
            bGlobalSet = true; // yup, globally we have an entry for this mule
        regkey.Close();
    }

    if (!bGlobalSet)
    {
        // we actually need to change the registry and write an entry for HKCU
        if (checkOnly)
            return true;
        HKEY hkeyCR = thePrefs.GetWindowsVersion() < _WINVER_2K_ ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        if (regkey.Create(hkeyCR, L"Software\\Classes\\eMule\\shell\\open\\command") == ERROR_SUCCESS)
        {
            VERIFY(regkey.SetStringValue(NULL, regbuffer) == ERROR_SUCCESS);

            VERIFY(regkey.Create(hkeyCR, L"Software\\Classes\\eMule\\DefaultIcon") == ERROR_SUCCESS);
            VERIFY(regkey.SetStringValue(NULL, CString(modbuffer) + L",1") == ERROR_SUCCESS);

            VERIFY(regkey.Create(hkeyCR, L"Software\\Classes\\eMule") == ERROR_SUCCESS);
            VERIFY(regkey.SetStringValue(NULL, L"eMule Collection File") == ERROR_SUCCESS);

            VERIFY(regkey.Open(hkeyCR, L"Software\\Classes\\eMule\\shell\\open") == ERROR_SUCCESS);
            regkey.RecurseDeleteKey(L"ddexec");
            regkey.RecurseDeleteKey(L"ddeexec");

            VERIFY(regkey.Create(hkeyCR, L"Software\\Classes\\" COLLECTION_FILEEXTENSION) == ERROR_SUCCESS);
            VERIFY(regkey.SetStringValue(NULL, L"eMule") == ERROR_SUCCESS);
            VERIFY(regkey.Close() == ERROR_SUCCESS);
        }
        else
            ASSERT(0);
    }
    return false;
}

bool gotostring(CFile &file, const uchar *find, LONGLONG plen)
{
    bool found = false;
    LONGLONG i=0;
    LONGLONG j=0;
    //LONGLONG plen = strlen(find);
    LONGLONG len = file.GetLength() - file.GetPosition();
    uchar temp;

    while (!found && i < len)
    {
        file.Read(&temp,1);
        if (temp == find[j])
            j++;
        else if (temp == find[0])
            j=1;
        else
            j=0;
        if (j==plen)
            return true;
        i++;
    }
    return false;
}

static void swap_byte(uint8* a, uint8* b)
{
    uint8 bySwap;
    bySwap = *a;
    *a = *b;
    *b = bySwap;
}

RC4_Key_Struct* RC4CreateKey(const uchar* pachKeyData, UINT nLen, RC4_Key_Struct* key, bool bSkipDiscard)
{
    uint8 index1;
    uint8 index2;
    uint8* pabyState;

    if (key == NULL)
        key = new RC4_Key_Struct;

    pabyState= &key->abyState[0];
    for (int i = 0; i < 256; i++)
        pabyState[i] = (uint8)i;

    key->byX = 0;
    key->byY = 0;
    index1 = 0;
    index2 = 0;
    for (int i = 0; i < 256; i++)
    {
        index2 = (pachKeyData[index1] + pabyState[i] + index2);
        swap_byte(&pabyState[i], &pabyState[index2]);
        index1 = (uint8)((index1 + 1) % nLen);
    }
    if (!bSkipDiscard)
        RC4Crypt(NULL, NULL, 1024, key);
    return key;
}

void RC4Crypt(const uchar* pachIn, uchar* pachOut, UINT nLen, RC4_Key_Struct* key)
{
    ASSERT(key != NULL && nLen > 0);
    if (key == NULL)
        return;

    uint8 byX = key->byX;;
    uint8 byY = key->byY;
    uint8* pabyState = &key->abyState[0];;
    uint8 byXorIndex;

    for (UINT i = 0; i < nLen; i++)
    {
        byX = (byX + 1);
        byY = (pabyState[byX] + byY);
        swap_byte(&pabyState[byX], &pabyState[byY]);
        byXorIndex = (pabyState[byX] + pabyState[byY]);

        if (pachIn != NULL)
            pachOut[i] = pachIn[i] ^ pabyState[byXorIndex];
    }
    key->byX = byX;
    key->byY = byY;
}


struct SFileExts
{
    EFileType ftype;
    LPCTSTR label;
    LPCTSTR extlist;
};

static const SFileExts s_fileexts[] =
{
    {ARCHIVE_ZIP,			L"ZIP",			L"|ZIP|JAR|CBZ|" },
    {ARCHIVE_RAR,			L"RAR",			L"|RAR|CBR|" },
    {ARCHIVE_ACE,			L"ACE",			L"|ACE|" },
    {AUDIO_MPEG,			L"MPEG Audio",	L"|MP2|MP3|" },
    {IMAGE_ISO,				L"ISO/NRG",		L"|ISO|NRG|" },
    {VIDEO_MPG,				L"MPEG Video",	L"|MPG|MPEG|" },
    {VIDEO_AVI,				L"AVI",			L"|AVI|DIVX|" },
    {WM,					L"Microsoft Media Audio/Video", L"|ASF|WMV|WMA|" },
    {PIC_JPG,				L"JPEG",			L"|JPG|JPEG|" },
    {PIC_PNG,				L"PNG",			L"|PNG|" },
    {PIC_GIF,				L"GIF",			L"|GIF|" },
    {DOCUMENT_PDF,			L"PDF",			L"|PDF|" },
    {FILETYPE_EXECUTABLE,	L"WIN/DOS EXE",	L"|EXE|COM|DLL|SYS|CPL|FON|OCX|SCR|VBX|" },
    {FILETYPE_UNKNOWN,		L"",	 			L"" }
};

#define HEADERCHECKSIZE 16

const unsigned char FILEHEADER_AVI_ID[]	= { 0x52, 0x49, 0x46, 0x46 };
const unsigned char FILEHEADER_RAR[]	= { 0x52, 0x61, 0x72, 0x21 };
const unsigned char FILEHEADER_ZIP[]	= { 0x50, 0x4b, 0x03, 0x04 };
const unsigned char FILEHEADER_ACE_ID[]	= { 0x2A, 0x2A, 0x41, 0x43, 0x45, 0x2A, 0x2A };
const unsigned char FILEHEADER_MP3_ID[]	= { 0x49, 0x44, 0x33, 0x03 };
const unsigned char FILEHEADER_MP3_ID2[]= { 0xFE, 0xFB };
const unsigned char FILEHEADER_MPG_ID[]	= { 0x00, 0x00, 0x01, 0xba };
const unsigned char FILEHEADER_ISO_ID[]	= { 0x01, 0x43, 0x44, 0x30, 0x30, 0x31 };
const unsigned char FILEHEADER_WM_ID[]	= { 0x30, 0x26, 0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c };
const unsigned char FILEHEADER_PNG_ID[]	= { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
const unsigned char FILEHEADER_JPG_ID[]	= { 0xff, 0xd8, 0xff };
const unsigned char FILEHEADER_GIF_ID[]	= { 0x47, 0x49, 0x46, 0x38 };
const unsigned char FILEHEADER_PDF_ID[]	= { 0x25, 0x50, 0x44, 0x46 };
const unsigned char FILEHEADER_EXECUTABLE_ID[]= {0x4d, 0x5a };

bool GetDRM(LPCTSTR pszFilePath)
{
    int fd = _topen(pszFilePath, O_RDONLY | O_BINARY);
    if (fd != -1)
    {
        BYTE aucBuff[16384];
        int iRead = _read(fd, aucBuff, sizeof aucBuff);
        _close(fd);
        fd = -1;

        if (iRead > sizeof(FILEHEADER_WM_ID) && memcmp(aucBuff, FILEHEADER_WM_ID, sizeof(FILEHEADER_WM_ID)) == 0)
        {
            iRead -= sizeof(FILEHEADER_WM_ID);
            if (iRead > 0)
            {
                static const WCHAR s_wszWrmHdr[] = L"<WRMHEADER";
                if (FindPattern(aucBuff + sizeof(FILEHEADER_WM_ID), iRead, (const BYTE *)s_wszWrmHdr, sizeof(s_wszWrmHdr) - sizeof(s_wszWrmHdr[0])))
                    return true;
            }
        }
    }
    return false;
}

EFileType GetFileTypeEx(CShareableFile* kfile, bool checkextention, bool checkfileheader, bool nocached)
{
    if (!nocached && kfile->GetVerifiedFileType() != FILETYPE_UNKNOWN)
        return kfile->GetVerifiedFileType();

    // check file header first
    EFileType res = FILETYPE_UNKNOWN;
    CPartFile* pfile = (CPartFile*)kfile;

    bool test4iso = (!kfile->IsPartFile()
                     || ((uint64)pfile->GetFileSize() > 0x8000 + HEADERCHECKSIZE && pfile->IsComplete(0x8000, 0x8000 + HEADERCHECKSIZE, true)));
    if (checkfileheader && (!kfile->IsPartFile() || pfile->IsComplete(0, HEADERCHECKSIZE, true) || test4iso))
    {
        unsigned char *headerbuf = headerbuf = (unsigned char *)calloc(HEADERCHECKSIZE, 1);
        if (headerbuf == NULL)
            return FILETYPE_UNKNOWN;

        try
        {
            CFile inFile;
            if (inFile.Open(kfile->GetFilePath(), CFile::modeRead | CFile::shareDenyNone))
            {
                if (!kfile->IsPartFile() || pfile->IsComplete(0, HEADERCHECKSIZE, true))
                {
                    int read = inFile.Read(headerbuf, HEADERCHECKSIZE);
                    if (read == HEADERCHECKSIZE)
                    {
                        if (memcmp(headerbuf, FILEHEADER_ZIP, sizeof(FILEHEADER_ZIP)) == 0)
                            res = ARCHIVE_ZIP;
                        else if (memcmp(headerbuf, FILEHEADER_RAR, sizeof(FILEHEADER_RAR)) == 0)
                            res = ARCHIVE_RAR;
                        else if (memcmp(headerbuf + 7, FILEHEADER_ACE_ID, sizeof(FILEHEADER_ACE_ID)) == 0)
                            res = ARCHIVE_ACE;
                        else if (memcmp(headerbuf, FILEHEADER_WM_ID, sizeof(FILEHEADER_WM_ID)) == 0)
                            res = WM;
                        else if (memcmp(headerbuf, FILEHEADER_AVI_ID, sizeof(FILEHEADER_AVI_ID)) == 0
                                 && strncmp((const char *)headerbuf + 8, "AVI", 3) == 0)
                            res = VIDEO_AVI;
                        else if (memcmp(headerbuf, FILEHEADER_MP3_ID, sizeof(FILEHEADER_MP3_ID)) == 0
                                 || memcmp(headerbuf, FILEHEADER_MP3_ID2, sizeof(FILEHEADER_MP3_ID2)) == 0)
                            res = AUDIO_MPEG;
                        else if (memcmp(headerbuf, FILEHEADER_MPG_ID, sizeof(FILEHEADER_MPG_ID)) == 0)
                            res = VIDEO_MPG;
                        else if (memcmp(headerbuf, FILEHEADER_PDF_ID, sizeof(FILEHEADER_PDF_ID)) == 0)
                            res = DOCUMENT_PDF;
                        else if (memcmp(headerbuf, FILEHEADER_PNG_ID, sizeof(FILEHEADER_PNG_ID)) == 0)
                            res = PIC_PNG;
                        else if (memcmp(headerbuf, FILEHEADER_JPG_ID, sizeof(FILEHEADER_JPG_ID)) == 0
                                 &&	(headerbuf[3] == 0xe1 || headerbuf[3] == 0xe0))
                            res = PIC_JPG;
                        else if (memcmp(headerbuf, FILEHEADER_GIF_ID, sizeof(FILEHEADER_GIF_ID)) == 0
                                 &&	headerbuf[5] == 0x61 && (headerbuf[4] == 0x37 || headerbuf[4] == 0x39))
                            res = PIC_GIF;
                        else if (memcmp(headerbuf, FILEHEADER_EXECUTABLE_ID, sizeof(FILEHEADER_EXECUTABLE_ID)) == 0)
                        {
                            res = FILETYPE_EXECUTABLE;

                            // This could be a self extracting RAR archive. If the ED2K name of the file
                            // has the RAR extension we treat this 'EXE' file like an 'unverified' RAR file.
                            LPCTSTR pszExt = PathFindExtension(kfile->GetFileName());
                            if (pszExt != NULL && _tcsicmp(pszExt, L".rar") == 0)
                                res = FILETYPE_UNKNOWN;
                        }
                        else if ((headerbuf[0] & 0xFF) == 0xFF && (headerbuf[1] & 0xE0) == 0xE0)
                            res = AUDIO_MPEG;
                    }
                }
                if (res == FILETYPE_UNKNOWN && test4iso)
                {
                    inFile.Seek(0x8000, CFile::begin);
                    int read = inFile.Read(headerbuf, HEADERCHECKSIZE);
                    if (read == HEADERCHECKSIZE)
                    {
                        if (memcmp(headerbuf, FILEHEADER_ISO_ID, sizeof(FILEHEADER_ISO_ID)) == 0)
                            res = IMAGE_ISO;
                    }
                }
                inFile.Close();
            }
        }
        catch (...)
        {
            ASSERT(0);
        }
        free(headerbuf);

        if (res != FILETYPE_UNKNOWN)
        {
            kfile->SetVerifiedFileType(res);
            return res;
        }
    }

    if (!checkextention)
        return res;

    CString extLC;
    int posD = kfile->GetFileName().ReverseFind(_T('.'));
    if (posD >= 0)
        extLC = kfile->GetFileName().Mid(posD + 1).MakeUpper();

    const SFileExts *ext = s_fileexts;
    while (ext->ftype != FILETYPE_UNKNOWN)
    {
        CString testext = ext->extlist;
        if (testext.Find(_T('|') + extLC + _T('|')) != -1)
            return ext->ftype;
        ext++;
    }

    // rar multivolume old naming
    if (extLC.GetLength() == 3
            && extLC.GetAt(0) == _T('R')
            && _istdigit((_TUCHAR)extLC.GetAt(1))
            && _istdigit((_TUCHAR)extLC.GetAt(2)))
        return ARCHIVE_RAR;

    return FILETYPE_UNKNOWN;
}

CString GetFileTypeName(EFileType ftype)
{
    const SFileExts *ext = s_fileexts;
    while (ext->ftype != FILETYPE_UNKNOWN)
    {
        if (ftype == ext->ftype)
            return ext->label;
        ext++;
    }
    return L"?";
}

int IsExtensionTypeOf(EFileType ftype, CString fext)
{
    fext = _T('|') + fext + _T('|');

    const SFileExts *ext = s_fileexts;
    while (ext->ftype != FILETYPE_UNKNOWN)
    {
        CString testext = ext->extlist;
        if (ftype == ext->ftype && testext.Find(fext) != -1)	// type matches acceptable extention
            return 1;

        if (ftype != ext->ftype && testext.Find(fext) != -1)	// not just unknown ext, but from a different type!
            return -1;
        ext++;
    }

    return 0;
}

UINT LevenshteinDistance(const CString& str1, const CString& str2)
{
    UINT n1 = str1.GetLength();
    UINT n2 = str2.GetLength();

    UINT* p = new UINT[n2+1];
    UINT* q = new UINT[n2+1];
    UINT* r;

    p[0] = 0;
    for (UINT j = 1; j <= n2; ++j)
        p[j] = p[j-1] + 1;

    for (UINT i = 1; i <= n1; ++i)
    {
        q[0] = p[0] + 1;
        for (UINT j = 1; j <= n2; ++j)
        {
            UINT d_del = p[j] + 1;
            UINT d_ins = q[j-1] + 1;
            UINT d_sub = p[j-1] + (str1.GetAt(i-1) == str2.GetAt(j-1) ? 0 : 1);
            q[j] = min(min(d_del, d_ins), d_sub);
        }
        r = p;
        p = q;
        q = r;
    }

    UINT tmp = p[n2];
    delete[] p;
    delete[] q;

    return tmp;
}

// Wrapper for _tmakepath which ensures that the outputbuffer does not exceed MAX_PATH
// using a smaller buffer  without checking the sizes prior calling this function is not safe
// If the resulting path would be bigger than MAX_PATH-1, it will be empty and return false (similar to PathCombine)
bool _tmakepathlimit(TCHAR *path, const TCHAR *drive, const TCHAR *dir, const TCHAR *fname, const TCHAR *ext)
{
    if (path == NULL)
    {
        ASSERT(0);
        return false;
    }

    UINT nSize = 64; // the function should actually only add 4 (+1 nullbyte) bytes max extra
    if (drive != NULL)
        nSize += _tcsclen(drive);
    if (dir != NULL)
        nSize += _tcsclen(dir);
    if (fname != NULL)
        nSize += _tcsclen(fname);
    if (ext != NULL)
        nSize += _tcsclen(ext);

    TCHAR* tchBuffer = new TCHAR[nSize];
    _tmakepath(tchBuffer, drive, dir, fname, ext);

    if (_tcslen(tchBuffer) >= MAX_PATH)
    {
        path[0] = L'\0';
        ASSERT(0);
        delete[] tchBuffer;
        return false;
    }
    else
    {
        _tcscpy(path, tchBuffer);
        delete[] tchBuffer;
        return true;
    }
}

#ifdef NAT_TRAVERSAL
uint8 GetMyConnectOptions(const bool bEncryption, const bool bCallback, const bool bNATTraversal) //>>> WiZaRd::NatTraversal [Xanatos]
#else
uint8 GetMyConnectOptions(const bool bEncryption, const bool bCallback)
#endif
{
    // Connect options Tag
    // 4 Reserved (!)
    // 1 Direct Callback
    // 1 CryptLayer Required
    // 1 CryptLayer Requested
    // 1 CryptLayer Supported
    const uint8 uSupportsCryptLayer	= (bEncryption) ? 1 : 0;
    const uint8 uRequestsCryptLayer	= (bEncryption && thePrefs.IsClientCryptLayerRequested()) ? 1 : 0;
    const uint8 uRequiresCryptLayer	= (bEncryption && thePrefs.IsClientCryptLayerRequired()) ? 1 : 0;
    // direct callback is only possible if connected to kad, tcp firewalled and verified UDP open (for example on a full cone NAT)
    const uint8 uDirectUDPCallback	= (bCallback && theApp.IsFirewalled() && Kademlia::CKademlia::IsRunning() && !Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) && Kademlia::CUDPFirewallTester::IsVerified()) ? 1 : 0;

#ifdef NAT_TRAVERSAL
    const uint8 uSupportsNatTraversal	= bNATTraversal ? 1 : 0; //>>> WiZaRd::NatTraversal [Xanatos]
#else
    const uint8 uSupportsNatTraversal	= 0;
#endif
    const uint8 byCryptOptions = (uSupportsNatTraversal << 7) | (uDirectUDPCallback << 3) | (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0);
    //const uint8 byCryptOptions = (uDirectUDPCallback << 3) | (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0);
    return byCryptOptions;
}

bool AddIconGrayscaledToImageList(CImageList& rList, HICON hIcon)
{
    // Use to create grayscaled alpha using icons on WinXP and lower
    // Only works with edited CxImage lib, not 6.0 standard
    bool bResult = false;
    ICONINFO iinfo;
    if (GetIconInfo(hIcon, &iinfo))
    {
        CxImage cxGray;
        if (cxGray.CreateFromHBITMAP(iinfo.hbmColor))
        {
            cxGray.GrayScale();
            HBITMAP hGrayBmp = cxGray.MakeBitmap(NULL, true);
            bResult = rList.Add(CBitmap::FromHandle(hGrayBmp), CBitmap::FromHandle(iinfo.hbmMask)) != (-1);
            DeleteObject(hGrayBmp);
        }
        DeleteObject(iinfo.hbmColor);
        DeleteObject(iinfo.hbmMask);
    }
    return bResult;
}

//>>> WiZaRd::Functions from removed files
static CString	m_sLocalIP = L"";
static UINT		m_uLocalIP = 0;
/////////////////////////////////////////////////////////////////////////////////
// Initializes m_localIP variable, for future access to GetLocalIP()
/////////////////////////////////////////////////////////////////////////////////
void InitLocalIP()
{
    static UINT uLastLocalIP = 0;
    // Using 'gethostname/gethostbyname' does not solve the problem when we have more than
    // one IP address. Using 'gethostname/gethostbyname' even seems to return the last IP
    // address which we got. e.g. if we already got an IP from our ISP,
    // 'gethostname/gethostbyname' will returned that (primary) IP, but if we add another
    // IP by opening a VPN connection, 'gethostname' will still return the same hostname,
    // but 'gethostbyname' will return the 2nd IP.
    // To weaken that problem at least for users which are binding eMule to a certain IP,
    // we use the explicitly specified bind address as our local IP address.
    if (thePrefs.GetBindAddrA() != NULL)
    {
        unsigned long ulBindAddr = inet_addr(thePrefs.GetBindAddrA());
        if (ulBindAddr != INADDR_ANY && ulBindAddr != INADDR_NONE)
        {
            m_uLocalIP = ulBindAddr;
            m_sLocalIP = ipstr(m_uLocalIP);
            if (uLastLocalIP != m_uLocalIP)
            {
                uLastLocalIP = m_uLocalIP;
                theApp.QueueLogLineEx(LOG_SUCCESS, L"Detected local IP: %u (%s)", m_uLocalIP, m_sLocalIP);
            }
            return;
        }
    }

    // Don't use 'gethostbyname(NULL)'. The winsock DLL may be replaced by a DLL from a third party
    // which is not fully compatible to the original winsock DLL. ppl reported crash with SCORSOCK.DLL
    // when using 'gethostbyname(NULL)'.
    try
    {
        char szHost[256];
        if (gethostname(szHost, sizeof szHost) == 0)
        {
            hostent* pHostEnt = gethostbyname(szHost);
            if (pHostEnt != NULL && pHostEnt->h_length == 4 && pHostEnt->h_addr_list[0] != NULL)
            {
                struct in_addr addr;
                memcpy(&addr, pHostEnt->h_addr_list[0], sizeof(struct in_addr));

                m_sLocalIP = inet_ntoa(addr);
                m_uLocalIP = addr.S_un.S_addr;
                if (uLastLocalIP != m_uLocalIP)
                {
                    uLastLocalIP = m_uLocalIP;
                    theApp.QueueLogLineEx(LOG_SUCCESS, L"Detected local IP: %u (%s)", m_uLocalIP, m_sLocalIP);
                }
            }
            else
                throw L"Failed to get a valid local ip";
        }
        else
            throw L"Failed to get local host name";
    }
    catch (CString error)
    {
        m_sLocalIP = L"";
        m_uLocalIP = 0;
        theApp.QueueLogLineEx(LOG_ERROR, L"InitLocalIP: %s");
    }
    catch (...)
    {
        m_sLocalIP = L"";
        m_uLocalIP = 0;
        theApp.QueueLogLineEx(LOG_ERROR, L"InitLocalIP: unknown exception in %hs", __FUNCTION__);
    }
}

/////////////////////////////////////////////////////////////////////////////////
// Returns the Local IP
/////////////////////////////////////////////////////////////////////////////////
UINT GetLocalIP()
{
    if (m_uLocalIP == 0)
        InitLocalIP();

    return m_uLocalIP;
}

/////////////////////////////////////////////////////////////////////////////////
// Forces re-detection
/////////////////////////////////////////////////////////////////////////////////
void ResetLocalIP()
{
    m_uLocalIP = 0;
}

/////////////////////////////////////////////////////////////////////////////////
// Returns a CString with the local IP in format xxx.xxx.xxx.xxx
/////////////////////////////////////////////////////////////////////////////////
CString GetLocalIPStr()
{
    if (m_sLocalIP.IsEmpty())
        InitLocalIP();

    return m_sLocalIP;
}

/////////////////////////////////////////////////////////////////////////////////
// Returns a CComBSTR with the local IP in format xxx.xxx.xxx.xxx
/////////////////////////////////////////////////////////////////////////////////
CComBSTR GetLocalIPBSTR()
{
    if (m_sLocalIP.IsEmpty())
        InitLocalIP();
    return CComBSTR(m_sLocalIP);
}

void ShowNetworkInfo()
{
    CNetworkInfoDlg dlg;
    dlg.DoModal();
}

bool UpdateNodesDatFromURL(CString strURL)
{
    bool success = false;
    CString strTempFilename;
    strTempFilename.Format(L"%stemp-%d-nodes.dat", thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), ::GetTickCount());

    // try to download nodes.dat
    Log(GetResString(IDS_DOWNLOADING_NODESDAT_FROM), strURL);
    CHttpDownloadDlg dlgDownload;
    dlgDownload.m_strTitle = GetResString(IDS_DOWNLOADING_NODESDAT);
    dlgDownload.m_sURLToDownload = strURL;
    dlgDownload.m_sFileToDownloadInto = strTempFilename;
    if (dlgDownload.DoModal() != IDOK)
        LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FAILEDDOWNLOADNODES), strURL);
    else
    {
        if (!Kademlia::CKademlia::IsRunning())
        {
            Kademlia::CKademlia::Start();
            theApp.emuledlg->ShowConnectionState();
        }
        success = Kademlia::CKademlia::GetRoutingZone()->ReadFile(strTempFilename) != 0;
        (void)_tremove(strTempFilename);
    }
    return success;
}

CList<Kademlia::CContact*> m_lContacts;
UINT GetKadContactCount()
{
    return m_lContacts.GetCount();
}

bool ContactAdd(Kademlia::CContact* pContact)
{
    if (m_lContacts.Find(pContact))
        return false;

    m_lContacts.AddTail(pContact);
#ifdef INFO_WND
    theApp.emuledlg->infoWnd->UpdateMyInfo(); //>>> WiZaRd::InfoWnd
#endif
    return true;
}

void ContactRem(Kademlia::CContact* pContact)
{
    POSITION pos = m_lContacts.Find(pContact);
    if (pos != NULL)
    {
        m_lContacts.RemoveAt(pos);
#ifdef INFO_WND
        theApp.emuledlg->infoWnd->UpdateMyInfo(); //>>> WiZaRd::InfoWnd
#endif
    }
}

UINT nKadSearchCount = 0;
UINT GetKadSearchCount()
{
    return nKadSearchCount;
}

void SearchAdd()
{
    ++nKadSearchCount;
#ifdef INFO_WND
    theApp.emuledlg->infoWnd->UpdateMyInfo(); //>>> WiZaRd::InfoWnd
#endif
}

void SearchRem()
{
    if (nKadSearchCount > 0)
    {
        --nKadSearchCount;
#ifdef INFO_WND
        theApp.emuledlg->infoWnd->UpdateMyInfo(); //>>> WiZaRd::InfoWnd
#endif
    }
}
//<<< WiZaRd::Functions from removed files
//>>> WiZaRd::Additional Functions
CString	GetExtension(const CString& path)
{
    CString ext = path;
    ext.MakeLower();
    int pos = ext.ReverseFind(L'.');
    if (pos == -1)
        return false; //could not get file type...
    return ext.Mid(pos);
}

// Get Registry VLC dir
CString GetRegVLCDir()
{
    CRegKey mKey;
    CString buffer = L"Applications\\vlc.exe\\shell\\open\\command";
    if (mKey.Open(HKEY_CLASSES_ROOT, buffer, KEY_READ) != ERROR_SUCCESS)
        return L"";

    DWORD iVersionLen = MAX_PATH;
    TCHAR strPath[MAX_PATH];
    if (mKey.QueryStringValue(L"", strPath, &iVersionLen) != ERROR_SUCCESS)
    {
        mKey.Close(); //close old keyhandle
        return L"";
    }

    mKey.Close(); //close old keyhandle

    //construct our string...
    CString ret = strPath;
    int pos = ret.ReverseFind(L'.');
    if (pos == -1
            || ret.GetLength() < pos+3)
        return L"";
    CString path = ret.Mid(1, pos+3); //theres a " on pos 1
    if (!::PathFileExists(path))
        return L"";
    return path;
}

PMIB_TCPTABLE GetTCPTable()
{
    // Get table of currently used TCP ports.
    PMIB_TCPTABLE pTCPTab = NULL;
    // Couple of crash dmp files are showing that we may crash somewhere in 'iphlpapi.dll' when doing the 2nd call
    __try
    {
        HMODULE hIpHlpDll = LoadLibrary(L"iphlpapi.dll");
        if (hIpHlpDll)
        {
            DWORD (WINAPI *pfnGetTcpTable)(PMIB_TCPTABLE, PDWORD, BOOL);
            (FARPROC&)pfnGetTcpTable = GetProcAddress(hIpHlpDll, "GetTcpTable");
            if (pfnGetTcpTable)
            {
                DWORD dwSize = 0;
                if ((*pfnGetTcpTable)(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
                {
                    // The nr. of TCP entries could change (increase) between
                    // the two function calls, allocate some more memory.
                    dwSize += sizeof(pTCPTab->table[0]) * 50;
                    pTCPTab = (PMIB_TCPTABLE)malloc(dwSize);
                    if (pTCPTab)
                    {
                        if ((*pfnGetTcpTable)(pTCPTab, &dwSize, TRUE) != ERROR_SUCCESS)
                        {
                            free(pTCPTab);
                            pTCPTab = NULL;
                        }
                    }
                }
            }
            FreeLibrary(hIpHlpDll);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        free(pTCPTab);
        pTCPTab = NULL;
    }

    return pTCPTab;
}

PMIB_UDPTABLE GetUDPTable()
{
    // Get table of currently used UDP ports.
    PMIB_UDPTABLE pUDPTab = NULL;
    // Couple of crash dmp files are showing that we may crash somewhere in 'iphlpapi.dll' when doing the 2nd call
    __try
    {
        HMODULE hIpHlpDll = LoadLibrary(L"iphlpapi.dll");
        if (hIpHlpDll)
        {
            DWORD (WINAPI *pfnGetUdpTable)(PMIB_UDPTABLE, PDWORD, BOOL);
            (FARPROC&)pfnGetUdpTable = GetProcAddress(hIpHlpDll, "GetUdpTable");
            if (pfnGetUdpTable)
            {
                DWORD dwSize = 0;
                if ((*pfnGetUdpTable)(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
                {
                    // The nr. of UDP entries could change (increase) between
                    // the two function calls, allocate some more memory.
                    dwSize += sizeof(pUDPTab->table[0]) * 50;
                    pUDPTab = (PMIB_UDPTABLE)malloc(dwSize);
                    if (pUDPTab)
                    {
                        if ((*pfnGetUdpTable)(pUDPTab, &dwSize, TRUE) != ERROR_SUCCESS)
                        {
                            free(pUDPTab);
                            pUDPTab = NULL;
                        }
                    }
                }
            }
            FreeLibrary(hIpHlpDll);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        free(pUDPTab);
        pUDPTab = NULL;
    }

    return pUDPTab;
}

bool IsPortInUse(const uint16 port)
{
    return IsTCPPortInUse(port) || IsUDPPortInUse(port);
}

bool PortStateInUse(const DWORD dwState)
{
    bool inUse = false;

    switch (dwState)
    {
        case MIB_TCP_STATE_CLOSED:
        case MIB_TCP_STATE_FIN_WAIT1:
        case MIB_TCP_STATE_FIN_WAIT2:
        case MIB_TCP_STATE_CLOSE_WAIT:
        case MIB_TCP_STATE_CLOSING:
        case MIB_TCP_STATE_LAST_ACK:
        case MIB_TCP_STATE_TIME_WAIT:
        case MIB_TCP_STATE_DELETE_TCB:
            break;

        case MIB_TCP_STATE_SYN_SENT:
        case MIB_TCP_STATE_SYN_RCVD:
        case MIB_TCP_STATE_ESTAB:
        case MIB_TCP_STATE_LISTEN:
            inUse = true;
            break;
    }
    return inUse;
}

CString DebugPortState(const DWORD dwState)
{
    CString strState = L"";

    switch (dwState)
    {
        case MIB_TCP_STATE_CLOSED:
            strState = L"CLOSED";
            break;

        case MIB_TCP_STATE_LISTEN:
            strState = L"LISTEN";
            break;

        case MIB_TCP_STATE_SYN_SENT:
            strState = L"SYN_SENT";
            break;

        case MIB_TCP_STATE_SYN_RCVD:
            strState = L"SYN_RCVD";
            break;

        case MIB_TCP_STATE_ESTAB:
            strState = L"ESTAB";
            break;

        case MIB_TCP_STATE_FIN_WAIT1:
            strState = L"FIN_WAIT1";
            break;

        case MIB_TCP_STATE_FIN_WAIT2:
            strState = L"FIN_WAIT2";
            break;

        case MIB_TCP_STATE_CLOSE_WAIT:
            strState = L"CLOSE_WAIT";
            break;

        case MIB_TCP_STATE_CLOSING:
            strState = L"CLOSING";
            break;

        case MIB_TCP_STATE_LAST_ACK:
            strState = L"LAST_ACK";
            break;

        case MIB_TCP_STATE_TIME_WAIT:
            strState = L"TIME_WAIT";
            break;

        case MIB_TCP_STATE_DELETE_TCB:
            strState = L"DELETE_TCB";
            break;

        default:
            strState.Format(L"Error: unknown state %u", dwState);
            break;
    }
    return strState;
}

bool IsTCPPortInUse(const uint16 port)
{
    // Get table of currently used TCP ports.
    PMIB_TCPTABLE pTCPTab = GetTCPTable();

    // The port is by default assumed to be available. If we got a table of currently
    // used TCP ports, we verify that this port is currently not used in any way.
    bool bPortIsFree = true;
    if (pTCPTab)
    {
        uint16 nPortBE = htons(port);
        for (UINT e = 0; e < pTCPTab->dwNumEntries && bPortIsFree; ++e)
        {
            // If there is a TCP entry in the table (regardless of its state), the port
            // is treated as not available.
            if (pTCPTab->table[e].dwLocalPort == nPortBE)
            {
                if (PortStateInUse(pTCPTab->table[e].dwState))
                {
                    bPortIsFree = false;
                    theApp.QueueDebugLogLineEx(LOG_WARNING, L"* TCP Port %u (%u) already in use (state %s)", port, nPortBE, DebugPortState(pTCPTab->table[e].dwState));
                }
                else
                    theApp.QueueDebugLogLineEx(LOG_WARNING, L"* TCP Port %u (%u) already in use (state %s)", port, nPortBE, DebugPortState(pTCPTab->table[e].dwState));
            }
        }
    }

    free(pTCPTab);
    return !bPortIsFree;
}

bool IsUDPPortInUse(const uint16 port)
{
    // Get table of currently used UDP ports.
    PMIB_UDPTABLE pUDPTab = GetUDPTable();

    // The port is by default assumed to be available. If we got a table of currently
    // used UDP ports, we verify that this port is currently not used in any way.
    bool bPortIsFree = true;
    if (pUDPTab)
    {
        uint16 nPortBE = htons(port);
        for (UINT e = 0; e < pUDPTab->dwNumEntries && bPortIsFree; ++e)
        {
            if (pUDPTab->table[e].dwLocalPort == nPortBE)
            {
                bPortIsFree = false;
                theApp.QueueDebugLogLineEx(LOG_WARNING, L"* UDP Port %u (%u) already in use", port, nPortBE);
            }
        }
    }

    free(pUDPTab);
    return !bPortIsFree;
}

uint16 GetRandomTCPPort()
{
    // Get table of currently used TCP ports.
    PMIB_TCPTABLE pTCPTab = GetTCPTable();

    const UINT uValidPortRange = 61000;
    int iMaxTests = uValidPortRange; // just in case, avoid endless loop
    uint16 nPort;
    bool bPortIsFree;
    do
    {
        // Get random port
        nPort = 4096 + (GetRandomUInt16() % uValidPortRange);

        // The port is by default assumed to be available. If we got a table of currently
        // used TCP ports, we verify that this port is currently not used in any way.
        bPortIsFree = true;
        if (pTCPTab)
        {
            uint16 nPortBE = htons(nPort);
            for (UINT e = 0; e < pTCPTab->dwNumEntries; ++e)
            {
                // If there is a TCP entry in the table (regardless of its state), the port
                // is treated as not available.
                if (pTCPTab->table[e].dwLocalPort == nPortBE)
                {
                    bPortIsFree = false;
                    break;
                }
            }
        }
    }
    while (!bPortIsFree && --iMaxTests > 0);
    free(pTCPTab);
    return nPort;
}

uint16 GetRandomUDPPort()
{
    // Get table of currently used UDP ports.
    PMIB_UDPTABLE pUDPTab = GetUDPTable();

    const UINT uValidPortRange = 61000;
    int iMaxTests = uValidPortRange; // just in case, avoid endless loop
    uint16 nPort;
    bool bPortIsFree;
    do
    {
        // Get random port
        nPort = 4096 + (GetRandomUInt16() % uValidPortRange);

        // The port is by default assumed to be available. If we got a table of currently
        // used UDP ports, we verify that this port is currently not used in any way.
        bPortIsFree = true;
        if (pUDPTab)
        {
            uint16 nPortBE = htons(nPort);
            for (UINT e = 0; e < pUDPTab->dwNumEntries; ++e)
            {
                if (pUDPTab->table[e].dwLocalPort == nPortBE)
                {
                    bPortIsFree = false;
                    break;
                }
            }
        }
    }
    while (!bPortIsFree && --iMaxTests > 0);
    free(pUDPTab);
    return nPort;
}

bool	GetDiskSpaceInfo(LPCTSTR pDirectory, uint64& freespace, uint64& totalspace)
{
    freespace = totalspace = 0;

    extern bool g_bUnicoWS;
    static BOOL _bInitialized = FALSE;
    static BOOL (WINAPI *_pfnGetDiskFreeSpaceEx)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER) = NULL;
    static BOOL (WINAPI *_pfnGetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER) = NULL;

    if (!_bInitialized)
    {
        _bInitialized = TRUE;
        if (g_bUnicoWS)
            (FARPROC&)_pfnGetDiskFreeSpaceExA = GetProcAddress(GetModuleHandle(L"kernel32"), "GetDiskFreeSpaceExA");
        else
            (FARPROC&)_pfnGetDiskFreeSpaceEx = GetProcAddress(GetModuleHandle(L"kernel32"), _TWINAPI("GetDiskFreeSpaceEx"));
    }

    if (_pfnGetDiskFreeSpaceEx)
    {
        ULARGE_INTEGER nFreeDiskSpace;
        ULARGE_INTEGER nTotalDiskSpace;
        ULARGE_INTEGER dummy;
        if ((*_pfnGetDiskFreeSpaceEx)(pDirectory, &nFreeDiskSpace, &nTotalDiskSpace, &dummy)==TRUE)
        {
            freespace = nFreeDiskSpace.QuadPart;
            totalspace = nTotalDiskSpace.QuadPart;
            return true;
        }
        return false;
    }

    if (_pfnGetDiskFreeSpaceExA)
    {
        ULARGE_INTEGER nFreeDiskSpace;
        ULARGE_INTEGER nTotalDiskSpace;
        ULARGE_INTEGER dummy;
        if ((*_pfnGetDiskFreeSpaceExA)(CT2CA(pDirectory), &nFreeDiskSpace, &nTotalDiskSpace, &dummy)==TRUE)
        {
            freespace = nFreeDiskSpace.QuadPart;
            totalspace = nTotalDiskSpace.QuadPart;
            return true;
        }
        return false;
    }

    TCHAR cDrive[MAX_PATH];
    const TCHAR *p = _tcschr(pDirectory, _T('\\'));
    if (p)
    {
        size_t uChars = p - pDirectory;
        if (uChars >= _countof(cDrive))
            return false;
        memcpy(cDrive, pDirectory, uChars * sizeof(TCHAR));
        cDrive[uChars] = L'\0';
    }
    else
    {
        if (_tcslen(pDirectory) >= _countof(cDrive))
            return false;
        _tcscpy(cDrive, pDirectory);
    }
    DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
    if (GetDiskFreeSpace(cDrive, &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters))
    {
        freespace = (ULONGLONG)dwFreeClusters * (ULONGLONG)dwSectPerClust * (ULONGLONG)dwBytesPerSect;
        totalspace = (ULONGLONG)dwTotalClusters * (ULONGLONG)dwSectPerClust * (ULONGLONG)dwBytesPerSect;
    }
    return false;
}

bool CheckURL(CString& strURL)
{
    strURL.Trim();
    if (strURL.IsEmpty() || (strURL.Find(L"://") == -1))
        return false;
    return true;
}

//>>> WiZaRd::Ratio Indicator
double	GetRatioDouble(const uint64& ul, const uint64& dl)
{
    double ret = DBL_MAX; // upload only? best value :)

    if (dl != 0)
        ret = (double)ul / dl;

    return ret;
}

int	GetRatioSmileyIndex(const double& dRatio)
{
    int index = dRatio > INT_MAX ? INT_MAX : int(dRatio);
    if (index < 0)
        index = 0;
    if (index > ratioSmileyCount)
        index = ratioSmileyCount;

    return index;
}

int GetRatioSmileyIndex(const uint64& ul, const uint64& dl)
{
    return GetRatioSmileyIndex(GetRatioDouble(ul, dl) * ratioSmileyCount);

}
//<<< WiZaRd::Ratio Indicator

CString	GetFileNameFromURL(const CString& strURL)
{
    CString strRet = L"";

    const int iSlash = strURL.ReverseFind(L'/');
    const int iRedir = strURL.ReverseFind(L'='); //e.g. for http://ip-filter.emulefuture.de/download.php?file=ipfilter.dat
    const int iUse = max(iSlash, iRedir);
    if (iUse == -1)
        strRet = strURL;
    else
    {
        const int iDot = strURL.Find(L'.', iUse+1);
        if (iDot != -1)
            strRet = strURL.Mid(iUse+1);
    }
    if (strRet.IsEmpty())
        return L"tmp.txt";
    return strRet;
}

uint64	GetFileSizeOnDisk(const CString& strFilePath)
{
    uint64 ret = (uint64)0;

    FILE* file = _tfsopen(strFilePath, L"rb", _SH_DENYNO); // can not use _SH_DENYWR because we may access a completing part file
    if (file)
    {
        // get filesize
        __int64 llFileSize = _filelengthi64(_fileno(file));
        if (llFileSize != -1i64)
            ret = (uint64)llFileSize;
        fclose(file);
    }

    return ret;
}

//>>> WiZaRd::Wine Compatibility
bool RunningWine()
{
    static int runningWine = -1;

    if (runningWine == -1)
    {
        runningWine = 0;
        if (GetProcAddress(GetModuleHandle(L"kernel32"), _TWINAPI("wine_get_unix_file_name")) != NULL)
        {
            runningWine = 1;
            theApp.QueueLogLineEx(LOG_WARNING, L"Detected WINE via kernel32.dll");
        }
        else if (GetProcAddress(GetModuleHandle(L"ntdll"), _TWINAPI("wine_get_unix_file_name")) != NULL)
        {
            runningWine = 1;
            theApp.QueueLogLineEx(LOG_WARNING, L"Detected WINE via ntdll.dll");
        }
        else
        {
            CRegKey key;
            if (key.Open(HKEY_CURRENT_USER, L"Software\\Wine", KEY_READ) == ERROR_SUCCESS)
            {
                runningWine = 1;
                theApp.QueueLogLineEx(LOG_WARNING, L"Detected WINE via registry");
            }
        }
    }

    return runningWine != 0;
}
//<<< WiZaRd::Wine Compatibility

void	FillClientIconImageList(CImageList& imageList)
{
    imageList.Add(CTempIconLoader(L"Friend"));						// 0
    imageList.Add(CTempIconLoader(L"ClientEDonkey"));				// 1
    imageList.Add(CTempIconLoader(L"ClientEDonkeyPlus"));			// 2
    imageList.Add(CTempIconLoader(L"ClientCompatible"));			// 3
    imageList.Add(CTempIconLoader(L"ClientCompatiblePlus"));		// 4
    imageList.Add(CTempIconLoader(L"ClientMLDonkey"));				// 5
    imageList.Add(CTempIconLoader(L"ClientMLDonkeyPlus"));			// 6
    imageList.Add(CTempIconLoader(L"ClientEDonkeyHybrid"));			// 7
    imageList.Add(CTempIconLoader(L"ClientEDonkeyHybridPlus"));		// 8
    imageList.Add(CTempIconLoader(L"ClientShareaza"));				// 9
    imageList.Add(CTempIconLoader(L"ClientShareazaPlus"));			//10
    imageList.Add(CTempIconLoader(L"ClientAMule"));					//11
    imageList.Add(CTempIconLoader(L"ClientAMulePlus"));				//12
    imageList.Add(CTempIconLoader(L"ClientLPhant"));				//13
    imageList.Add(CTempIconLoader(L"ClientLPhantPlus"));			//14
    imageList.Add(CTempIconLoader(L"ClientkMule"));					//15
    imageList.Add(CTempIconLoader(L"ClientkMulePlus"));				//16
    imageList.Add(CTempIconLoader(L"Server"));						//17
    imageList.Add(CTempIconLoader(L"ANALYZER0"));					//18
    imageList.Add(CTempIconLoader(L"ANALYZER1"));					//19
    imageList.Add(CTempIconLoader(L"ANALYZER2"));					//20
    imageList.Add(CTempIconLoader(L"ANALYZER3"));					//21
    imageList.Add(CTempIconLoader(L"ANALYZER4"));					//22
}

int	GetClientImageIndex(const bool bFriend, const UINT nClientVersion, const bool bPlus, const bool bExt)
{
    int index = 0;

    if (!bFriend)
    {
        if (nClientVersion == SO_URL)
            index = 17;
        else
        {
            if (nClientVersion == SO_KMULE)
                index = 15;
            else if (nClientVersion == SO_EDONKEYHYBRID)
                index = 7;
            else if (nClientVersion == SO_MLDONKEY)
                index = 5;
            else if (nClientVersion == SO_SHAREAZA)
                index = 9;
            else if (nClientVersion == SO_AMULE)
                index = 11;
            else if (nClientVersion == SO_LPHANT)
                index = 13;
            else if (bExt)
                index = 1;
            else
                index = 3;

            if (bPlus)
                index += 1;
        }
    }

    return index;
}

//NOTE: this retrieves a copy of the HAND cursor - it has to be destroyed in ANY case!
HCURSOR		CreateHandCursor()
{
    HCURSOR hLinkCursor = NULL;
    HCURSOR hHandCursor = NULL;

#ifdef IDC_HAND
    hHandCursor = ::LoadCursor(NULL, IDC_HAND); // Load Windows' hand cursor
    if (hHandCursor != NULL)                    // if not available, load it from winhlp32.exe
        hLinkCursor = CopyCursor(hHandCursor);
    else
#endif //IDC_HAND	
    {
        // This retrieves cursor #106 from winhlp32.exe, which is a hand pointer
        CString strWinDir;
        (void)GetWindowsDirectory(strWinDir.GetBuffer(MAX_PATH), MAX_PATH);
        strWinDir.ReleaseBuffer();
        strWinDir += L"\\winhlp32.exe";

        HMODULE hModule = LoadLibrary(strWinDir);
        if (hModule)
        {
            hHandCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
            if (hHandCursor != NULL)
                hLinkCursor = CopyCursor(hHandCursor);
            FreeLibrary(hModule);
        }
    }

    if (hLinkCursor == NULL)
        hLinkCursor = CopyCursor(::LoadCursor(NULL, IDC_ARROW));

    return hLinkCursor;
}
//<<< WiZaRd::Additional Functions
//>>> WiZaRd::Improved Auto Prio
uint8	CalcPrioFromSrcAverage(const UINT srcs, const float avg)
{
    uint8 prio = PR_NORMAL;			// default

    if (srcs < avg*0.50f)				//50% less
        prio = PR_VERYHIGH;
    else if (srcs < avg*0.75f)		//25% less
        prio = PR_HIGH;
    else if (srcs > avg*1.50f)		//50% more
        prio = PR_VERYLOW;
    else if (srcs > avg*1.25f)		//25% more
        prio = PR_LOW;

    return prio;
}
//<<< WiZaRd::Improved Auto Prio
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
void DebugRecv(LPCSTR pszMsg, const CUpDownClient* client, const uchar* packet, const CAddress& IP)
{
    // 111.222.333.444 = 15 chars
    if (client)
    {
        if (client != NULL && packet != NULL)
            Debug(L"%-24hs from %s; %s\n", pszMsg, client->DbgGetClientInfo(true), DbgGetFileInfo(packet));
        else if (client != NULL && packet == NULL)
            Debug(L"%-24hs from %s\n", pszMsg, client->DbgGetClientInfo(true));
        else if (client == NULL && packet != NULL)
            Debug(L"%-24hs; %s\n", pszMsg, DbgGetFileInfo(packet));
        else
            Debug(L"%-24hs\n", pszMsg);
    }
    else
    {
        if (!IP.IsNull() && packet != NULL)
            Debug(L"%-24hs from %-15s; %s\n", pszMsg, ipstr(IP), DbgGetFileInfo(packet));
        else if (!IP.IsNull() && packet == NULL)
            Debug(L"%-24hs from %-15s\n", pszMsg, ipstr(IP));
        else if (IP.IsNull() && packet != NULL)
            Debug(L"%-24hs; %s\n", pszMsg, DbgGetFileInfo(packet));
        else
            Debug(L"%-24hs\n", pszMsg);
    }
}

void DebugRecv(LPCSTR pszOpcode, const CAddress& IP, uint16 port)
{
    TCHAR szIPPort[22];
    _stprintf(szIPPort, L"%s:%u", ipstr(IP), port);
    Debug(L"%-24hs from %-21s\n", pszOpcode, szIPPort);
}

void DebugSend(LPCSTR pszOpcode, const CAddress& IP, uint16 port)
{
    TCHAR szIPPort[22];
    _stprintf(szIPPort, L"%s:%u", ipstr(IP), port);
    Debug(L">>> %-20hs to   %-21s\n", pszOpcode, szIPPort);
}

CString ipstr(const CAddress& IP)
{
    return IP.ToStringW().c_str();
}
//<<< WiZaRd::IPv6 [Xanatos]
#endif

//>>> WiZaRd::Find Best Interface IP [netfinity]
ULONG GetBestInterfaceIP(const ULONG dest_addr)
{
    ULONG bestInterfaceIP = INADDR_NONE;

    HINSTANCE m_hIPHlp = ::LoadLibrary(L"Iphlpapi.dll");
    if (m_hIPHlp != NULL)
    {
        typedef DWORD (WINAPI *t_GetBestInterface)(
            IPAddr dwDestAddr,
            PDWORD pdwBestIfIndex);
        typedef DWORD (WINAPI *t_GetBestRoute2)(
            NET_LUID *InterfaceLuid,
            NET_IFINDEX InterfaceIndex,
            const SOCKADDR_INET *SourceAddress,
            const SOCKADDR_INET *DestinationAddress,
            ULONG AddressSortOptions,
            PMIB_IPFORWARD_ROW2 BestRoute,
            SOCKADDR_INET *BestSourceAddress);

        t_GetBestInterface s_pGetBestInterface = (t_GetBestInterface) ::GetProcAddress(m_hIPHlp, "GetBestInterface");
        t_GetBestRoute2 s_pGetBestRoute2 = (t_GetBestRoute2) ::GetProcAddress(m_hIPHlp, "GetBestRoute2");

        SOCKADDR_INET		saBestInterfaceAddress = {0};
        SOCKADDR_INET		saDestinationAddress = {0};
        MIB_IPFORWARD_ROW2	rowBestRoute = {0};
        DWORD	dwBestIfIndex;
        DWORD	dwError;

        saDestinationAddress.Ipv4.sin_family = AF_INET;
        saDestinationAddress.Ipv4.sin_addr.S_un.S_addr = dest_addr;

        if (s_pGetBestInterface != nullptr && s_pGetBestRoute2 != nullptr && dest_addr != INADDR_NONE)
        {
            dwError = s_pGetBestInterface(dest_addr, &dwBestIfIndex);
            if (dwError == NO_ERROR)
            {
                dwError = s_pGetBestRoute2(nullptr, dwBestIfIndex, nullptr, &saDestinationAddress, 0, &rowBestRoute, &saBestInterfaceAddress);
                if (dwError == NO_ERROR)
                    bestInterfaceIP = saBestInterfaceAddress.Ipv4.sin_addr.S_un.S_addr;
                else
                    DebugLogError(_T(__FUNCTION__) L": Failed to retrieve best route source ip address to dest %s: %s", ipstr(dest_addr), GetErrorMessage(dwError, 1));
            }
            else
                DebugLogError(_T(__FUNCTION__) L": Failed to retrieve best interface index for destination %s: %s", ipstr(dest_addr), GetErrorMessage(dwError, 1));
        }
    }

    return bestInterfaceIP;
}
//<<< WiZaRd::Find Best Interface IP [netfinity]
//>>> WiZaRd
// Unfortunately, Win2K and higher don't allow to read the "read-only" attribute of directories anymore.
// This is a workaround for that issue.
bool IsDirectoryWriteable(LPCTSTR pszDirectory)
{
    bool writeable = false;

    if (PathFileExists(pszDirectory) || ::CreateDirectory(pszDirectory, 0))
    {
        const CString strFilename = pszDirectory + (CString)L"test.file";
        HANDLE hFile = CreateFile(strFilename,
                                  GENERIC_WRITE,				// open for writing
                                  NULL,						// don't share
                                  NULL,						// no security
                                  CREATE_ALWAYS,				// always create
                                  FILE_ATTRIBUTE_TEMPORARY,	// just a temporary file
                                  NULL);						// no attr. template

        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            ::DeleteFile(strFilename);

            writeable = true;
        }
    }

    return writeable;
}
//<<< WiZaRd

CString GetBetaFileName()
{
    CString strBetaFileName = L"";

    strBetaFileName.Format(L"eMule%u.%u%c.%u Beta Testfile ", CemuleApp::m_nVersionMjr,
                           CemuleApp::m_nVersionMin, L'a' + CemuleApp::m_nVersionUpd, CemuleApp::m_nVersionBld);
    MD5Sum md5(strBetaFileName);
    strBetaFileName.Append(md5.GetHash().Left(6) + L".txt");

    return strBetaFileName;
}

void CreateBetaFile()
{
#ifdef _BETA
    // In Betaversion we create a testfile which is published in order to make testing easier
    // by allowing to easily find files which are published and shared by "new" nodes
    CString strBetaFileName = GetBetaFileName();
    CString tempDir = thePrefs.GetMuleDirectory(EMULE_INCOMINGDIR);
    if (tempDir.Right(1) != L"\\")
        tempDir.Append(L"\\");

    // always create it - this will ensure that the file content is correct!
    CStdioFile f;
    if (!f.Open(tempDir + strBetaFileName, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite))
        ASSERT(0);
    else
    {
        try
        {
            // do not translate the content!
            f.WriteString(strBetaFileName + '\n'); // guarantees a different hash on different versions
            f.WriteString(L"This file is automatically created by eMule Beta versions to help the developers testing and debugging new the new features. eMule will delete this file when exiting, otherwise you can remove this file at any time.\nThanks for beta testing eMule :)");
            f.Close();
        }
        catch (CFileException* ex)
        {
            ASSERT(0);
            ex->Delete();
        }
    }
#endif
}

void DeleteBetaFile()
{
    // On Beta builds we created a testfile, delete it when closing eMule
    CString strBetaFileName = GetBetaFileName();
    CString tempDir = thePrefs.GetMuleDirectory(EMULE_INCOMINGDIR);
    if (tempDir.Right(1) != L"\\")
        tempDir += L"\\";

    ::DeleteFile(tempDir + strBetaFileName);
}

void GetPartStartAndEnd(const UINT uPartNumber, const uint64 partsize, const EMFileSize emFileSize, uint64& uStart, uint64& uEnd)
{
    uStart = partsize*(uint64)uPartNumber;
    uEnd = partsize*(uint64)(uPartNumber+1);
    if (uEnd > emFileSize)
        uEnd = emFileSize;
}