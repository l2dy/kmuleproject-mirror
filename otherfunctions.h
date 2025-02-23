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
#pragma once

class CAbstractFile;
class CKnownFile;
struct Requested_Block_Struct;
class CUpDownClient;
class CAICHHash;
class CPartFile;
class CSafeMemFile;
class CShareableFile;

enum EFileType
{
    FILETYPE_UNKNOWN,
    FILETYPE_EXECUTABLE,
    ARCHIVE_ZIP,
    ARCHIVE_RAR,
    ARCHIVE_ACE,
    IMAGE_ISO,
    AUDIO_MPEG,
    VIDEO_AVI,
    VIDEO_MPG,
    WM,
    PIC_JPG,
    PIC_PNG,
    PIC_GIF,
    DOCUMENT_PDF
};


#define ROUND(x) (floor((float)x+0.5f))

///////////////////////////////////////////////////////////////////////////////
// Low level str
//
__inline char* nstrdup(const char* todup)
{
    size_t len = strlen(todup) + 1;
    return (char*)memcpy(new char[len], todup, len);
}

TCHAR *stristr(const TCHAR *str1, const TCHAR *str2);
CString GetNextString(const CString& rstr, LPCTSTR pszTokens, int& riStart);
CString GetNextString(const CString& rstr, TCHAR chToken, int& riStart);


///////////////////////////////////////////////////////////////////////////////
// String conversion
//
CString CastItoXBytes(uint16 count, bool isK = false, bool isPerSec = false, UINT decimal = 2);
CString CastItoXBytes(UINT count, bool isK = false, bool isPerSec = false, UINT decimal = 2);
CString CastItoXBytes(uint64 count, bool isK = false, bool isPerSec = false, UINT decimal = 2);
CString CastItoXBytes(float count, bool isK = false, bool isPerSec = false, UINT decimal = 2);
CString CastItoXBytes(double count, bool isK = false, bool isPerSec = false, UINT decimal = 2);
#if defined(_DEBUG) && defined(USE_DEBUG_EMFILESIZE)
CString CastItoXBytes(EMFileSize count, bool isK = false, bool isPerSec = false, UINT decimal = 2);
#endif
CString CastItoIShort(uint16 count, bool isK = false, UINT decimal = 2);
CString CastItoIShort(UINT count, bool isK = false, UINT decimal = 2);
CString CastItoIShort(uint64 count, bool isK = false, UINT decimal = 2);
CString CastItoIShort(float count, bool isK = false, UINT decimal = 2);
CString CastItoIShort(double count, bool isK = false, UINT decimal = 2);
CString CastSecondsToHM(time_t seconds);
CString	CastSecondsToLngHM(time_t seconds);
CString GetFormatedUInt(ULONG ulVal);
CString GetFormatedUInt64(ULONGLONG ullVal);
void SecToTimeLength(unsigned long ulSec, CStringA& rstrTimeLength);
void SecToTimeLength(unsigned long ulSec, CStringW& rstrTimeLength);
bool RegularExpressionMatch(CString regexpr, CString teststring);

///////////////////////////////////////////////////////////////////////////////
// URL conversion
//
CString URLDecode(const CString& sIn, bool bKeepNewLine = false);
CString URLEncode(const CString& sIn);
CString EncodeURLQueryParam(const CString& rstrQuery);
CString MakeStringEscaped(CString in);
CString RemoveAmpersand(const CString& rstr);
CString	StripInvalidFilenameChars(const CString& strText);


///////////////////////////////////////////////////////////////////////////////
// Hex conversion
//
CString EncodeBase32(const unsigned char* buffer, unsigned int bufLen);
CString EncodeBase16(const unsigned char* buffer, unsigned int bufLen);
unsigned int DecodeLengthBase16(unsigned int base16Length);
bool DecodeBase16(const TCHAR *base16Buffer, unsigned int base16BufLen, byte *buffer, unsigned int bufflen);
UINT DecodeBase32(LPCTSTR pszInput, uchar* paucOutput, UINT nBufferLen);
UINT DecodeBase32(LPCTSTR pszInput, CAICHHash& Hash);

///////////////////////////////////////////////////////////////////////////////
// File/Path string helpers
//
void MakeFoldername(CString &path);
CString RemoveFileExtension(const CString& rstrFilePath);
int CompareDirectories(const CString& rstrDir1, const CString& rstrDir2);
CString StringLimit(CString in, UINT length);
CString CleanupFilename(CString filename, bool bExtension = true);
CString ValidFilename(CString filename);
bool ExpandEnvironmentStrings(CString& rstrStrings);
int CompareLocaleString(LPCTSTR psz1, LPCTSTR psz2);
int CompareLocaleStringNoCase(LPCTSTR psz1, LPCTSTR psz2);
int __cdecl CompareCStringPtrLocaleString(const void* p1, const void* p2);
int __cdecl CompareCStringPtrLocaleStringNoCase(const void* p1, const void* p2);
void Sort(CStringArray& astr, int (__cdecl *pfnCompare)(const void*, const void*) = CompareCStringPtrLocaleStringNoCase);
int __cdecl CompareCStringPtrPtrLocaleString(const void* p1, const void* p2);
int __cdecl CompareCStringPtrPtrLocaleStringNoCase(const void* p1, const void* p2);
void		Sort(CSimpleArray<const CString*>& apstr, int (__cdecl *pfnCompare)(const void*, const void*) = CompareCStringPtrPtrLocaleStringNoCase);
void		StripTrailingCollon(CString& rstr);
bool		IsUnicodeFile(LPCTSTR pszFilePath);
UINT64		GetFreeTempSpace(int tempdirindex);
int			GetPathDriveNumber(CString path);
EFileType	GetFileTypeEx(CShareableFile* kfile, bool checkextention=true, bool checkfileheader=true, bool nocached=false);
CString		GetFileTypeName(EFileType ftype);
int			IsExtensionTypeOf(EFileType type, CString ext);
UINT		LevenshteinDistance(const CString& str1, const CString& str2);
bool		_tmakepathlimit(TCHAR *path, const TCHAR *drive, const TCHAR *dir, const TCHAR *fname, const TCHAR *ext);

///////////////////////////////////////////////////////////////////////////////
// GUI helpers
//
void InstallSkin(LPCTSTR pszSkinPackage);
bool CheckFileOpen(LPCTSTR pszFilePath, LPCTSTR pszFileTitle = NULL);
void ShellOpenFile(CString name);
void ShellOpenFile(CString name, LPCTSTR pszVerb);
bool ShellDeleteFile(LPCTSTR pszFilePath);
CString ShellGetFolderPath(int iCSIDL);
bool SelectDir(HWND hWnd, LPTSTR pszPath, LPCTSTR pszTitle = NULL, LPCTSTR pszDlgTitle = NULL);
BOOL DialogBrowseFile(CString& rstrPath, LPCTSTR pszFilters, LPCTSTR pszDefaultFileName = NULL, DWORD dwFlags = 0,bool openfilestyle=true);
void AddBuddyButton(HWND hwndEdit, HWND hwndButton);
bool InitAttachedBrowseButton(HWND hwndButton, HICON &ricoBrowse);
void GetPopupMenuPos(CListCtrl& lv, CPoint& point);
void GetPopupMenuPos(CTreeCtrl& tv, CPoint& point);
void InitWindowStyles(CWnd* pWnd);
CString GetRateString(UINT rate);
HWND GetComboBoxEditCtrl(CComboBox& cb);
HWND ReplaceRichEditCtrl(CWnd* pwndRE, CWnd* pwndParent, CFont* pFont);
int  FontPointSizeToLogUnits(int nPointSize);
bool CreatePointFont(CFont &rFont, int nPointSize, LPCTSTR lpszFaceName);
bool CreatePointFontIndirect(CFont &rFont, const LOGFONT *lpLogFont);


///////////////////////////////////////////////////////////////////////////////
// Resource strings
//
#ifdef USE_STRING_IDS
#define	RESSTRIDTYPE		LPCTSTR
#define	IDS2RESIDTYPE(id)	#id
#define GetResString(id)	_GetResString(#id)
CString _GetResString(RESSTRIDTYPE StringID);
#else//USE_STRING_IDS
#define	RESSTRIDTYPE		UINT
#define	IDS2RESIDTYPE(id)	id
CString GetResString(RESSTRIDTYPE StringID);
#define _GetResString(id)	GetResString(id)
CString GetCurrentResString(RESSTRIDTYPE StringID); //>>> WiZaRd::Honor translators
#endif//!USE_STRING_IDS
void InitThreadLocale();


///////////////////////////////////////////////////////////////////////////////
// Error strings, Debugging, Logging
//
int GetSystemErrorString(DWORD dwError, CString &rstrError);
int GetModuleErrorString(DWORD dwError, CString &rstrError, LPCTSTR pszModule);
int GetErrorMessage(DWORD dwError, CString &rstrErrorMsg, DWORD dwFlags = 0);
CString GetErrorMessage(DWORD dwError, DWORD dwFlags = 0);
LPCTSTR	GetShellExecuteErrMsg(DWORD dwShellExecError);
CString DbgGetHexDump(const uint8* data, UINT size);
void DbgSetThreadName(LPCSTR szThreadName, ...);
void Debug(LPCTSTR pszFmtMsg, ...);
void DebugHexDump(const uint8* data, UINT lenData);
void DebugHexDump(CFile& file);
CString DbgGetFileInfo(const uchar* hash);
CString DbgGetFileStatus(UINT nPartCount, CSafeMemFile* data);
LPCTSTR DbgGetHashTypeString(const uchar* hash);
CString DbgGetClientID(UINT nClientID);
int GetHashType(const uchar* hash);
CString DbgGetDonkeyClientTCPOpcode(UINT opcode);
CString DbgGetMuleClientTCPOpcode(UINT opcode);
CString	DbgGetModTCPOpcode(const UINT opcode); //>>> WiZaRd::ModProt
CString DbgGetClientTCPOpcode(UINT protocol, UINT opcode);
CString DbgGetClientTCPPacket(UINT protocol, UINT opcode, UINT size);
CString DbgGetBlockInfo(const Requested_Block_Struct* block);
CString DbgGetBlockInfo(uint64 StartOffset, uint64 EndOffset);
CString DbgGetBlockFileInfo(const Requested_Block_Struct* block, const CPartFile* partfile);
CString DbgGetFileMetaTagName(UINT uMetaTagID);
CString DbgGetFileMetaTagName(LPCSTR pszMetaTagID);
CString DbgGetSearchOperatorName(UINT uOperator);
void DebugRecv(LPCSTR pszMsg, const CUpDownClient* client, const uchar* packet = NULL, UINT nIP = 0);
void DebugRecv(LPCSTR pszOpcode, UINT ip, uint16 port);
void DebugSend(LPCSTR pszMsg, const CUpDownClient* client, const uchar* packet = NULL);
void DebugSend(LPCSTR pszOpcode, UINT ip, uint16 port);
void DebugSendF(LPCSTR pszOpcode, UINT ip, uint16 port, LPCTSTR pszMsg, ...);
void DebugHttpHeaders(const CStringAArray& astrHeaders);



///////////////////////////////////////////////////////////////////////////////
// Win32 specifics
//
bool Ask4RegFix(bool checkOnly, bool dontAsk = false, bool bAutoTakeCollections = false); // Barry - Allow forced update without prompt
void BackupReg(void); // Barry - Store previous values
void RevertReg(void); // Barry - Restore previous values
bool DoCollectionRegFix(bool checkOnly);
void AddAutoStart();
void RemAutoStart();
ULONGLONG GetModuleVersion(LPCTSTR pszFilePath);
ULONGLONG GetModuleVersion(HMODULE hModule);

int GetMaxWindowsTCPConnections();

#define _WINVER_95_		0x0400	// 4.0
#define _WINVER_NT4_	0x0401	// 4.1 (baked version)
#define _WINVER_98_		0x040A	// 4.10
#define _WINVER_ME_		0x045A	// 4.90
#define _WINVER_2K_		0x0500	// 5.0
#define _WINVER_XP_		0x0501	// 5.1
#define _WINVER_2003_	0x0502	// 5.2
#define _WINVER_VISTA_	0x0600	// 6.0
#define _WINVER_7_		0x0601	// 6.1
#define	_WINVER_S2008_	0x0601	// 6.1
#define _WINVER_8_		0x0602	// 6.2

WORD		DetectWinVersion();
int			IsRunningXPSP2();
int			IsRunningXPSP2OrHigher();
uint64		GetFreeDiskSpaceX(LPCTSTR pDirectory);
ULONGLONG	GetDiskFileSize(LPCTSTR pszFilePath);
int			GetAppImageListColorFlag();
int			GetDesktopColorDepth();
bool		IsFileOnFATVolume(LPCTSTR pszFilePath);
void		ClearVolumeInfoCache(int iDrive = -1);
bool		AddIconGrayscaledToImageList(CImageList& rList, HICON hIcon);


///////////////////////////////////////////////////////////////////////////////
// MD4 helpers
//

__inline BYTE toHex(const BYTE &x)
{
    return x > 9 ? x + 55: x + 48;
}

// md4cmp -- replacement for memcmp(hash1,hash2,16)
// Like 'memcmp' this function returns 0, if hash1==hash2, and !0, if hash1!=hash2.
// NOTE: Do *NOT* use that function for determining if hash1<hash2 or hash1>hash2.
__inline int md4cmp(const void* hash1, const void* hash2)
{
    return !(((UINT*)hash1)[0] == ((UINT*)hash2)[0] &&
             ((UINT*)hash1)[1] == ((UINT*)hash2)[1] &&
             ((UINT*)hash1)[2] == ((UINT*)hash2)[2] &&
             ((UINT*)hash1)[3] == ((UINT*)hash2)[3]);
}

__inline bool isnulmd4(const void* hash)
{
    return (((UINT*)hash)[0] == 0 &&
            ((UINT*)hash)[1] == 0 &&
            ((UINT*)hash)[2] == 0 &&
            ((UINT*)hash)[3] == 0);
}

// md4clr -- replacement for memset(hash,0,16)
__inline void md4clr(const void* hash)
{
    ((UINT*)hash)[0] = ((UINT*)hash)[1] = ((UINT*)hash)[2] = ((UINT*)hash)[3] = 0;
}

// md4cpy -- replacement for memcpy(dst,src,16)
__inline void md4cpy(void* dst, const void* src)
{
    ((UINT*)dst)[0] = ((UINT*)src)[0];
    ((UINT*)dst)[1] = ((UINT*)src)[1];
    ((UINT*)dst)[2] = ((UINT*)src)[2];
    ((UINT*)dst)[3] = ((UINT*)src)[3];
}

#define	MAX_HASHSTR_SIZE (16*2+1)
CString md4str(const uchar* hash);
CStringA md4strA(const uchar* hash);
void md4str(const uchar* hash, TCHAR* pszHash);
void md4strA(const uchar* hash, CHAR* pszHash);
bool strmd4(const char* pszHash, uchar* hash);
bool strmd4(const CString& rstr, uchar* hash);


///////////////////////////////////////////////////////////////////////////////
// Compare helpers
//
__inline int CompareUnsigned(UINT uSize1, UINT uSize2)
{
    if (uSize1 < uSize2)
        return -1;
    if (uSize1 > uSize2)
        return 1;
    return 0;
}

__inline int CompareUnsignedUndefinedAtBottom(UINT uSize1, UINT uSize2, bool bSortAscending)
{
    if (uSize1 == 0 && uSize2 == 0)
        return 0;
    if (uSize1 == 0)
        return bSortAscending ? 1 : -1;
    if (uSize2 == 0)
        return bSortAscending ? -1 : 1;
    return CompareUnsigned(uSize1, uSize2);
}

__inline int CompareUnsigned64(uint64 uSize1, uint64 uSize2)
{
    if (uSize1 < uSize2)
        return -1;
    if (uSize1 > uSize2)
        return 1;
    return 0;
}

__inline int CompareFloat(float uSize1, float uSize2)
{
    if (uSize1 < uSize2)
        return -1;
    if (uSize1 > uSize2)
        return 1;
    return 0;
}

__inline int CompareDouble(double uSize1, double uSize2)
{
    if (uSize1 < uSize2)
        return -1;
    if (uSize1 > uSize2)
        return 1;
    return 0;
}

__inline int CompareOptLocaleStringNoCase(LPCTSTR psz1, LPCTSTR psz2)
{
    if (psz1 && psz2)
        return CompareLocaleStringNoCase(psz1, psz2);
    if (psz1)
        return -1;
    if (psz2)
        return 1;
    return 0;
}

__inline int CompareOptLocaleStringNoCaseUndefinedAtBottom(const CString &str1, const CString &str2, bool bSortAscending)
{
    if (str1.IsEmpty() && str2.IsEmpty())
        return 0;
    if (str1.IsEmpty())
        return bSortAscending ? 1 : -1;
    if (str2.IsEmpty())
        return bSortAscending ? -1 : 1;
    return CompareOptLocaleStringNoCase(str1, str2);
}


///////////////////////////////////////////////////////////////////////////////
// ED2K File Type
//
enum EED2KFileType
{
    ED2KFT_ANY				= 0,
    ED2KFT_AUDIO			= 1,	// ED2K protocol value (eserver 17.6+)
    ED2KFT_VIDEO			= 2,	// ED2K protocol value (eserver 17.6+)
    ED2KFT_IMAGE			= 3,	// ED2K protocol value (eserver 17.6+)
    ED2KFT_PROGRAM			= 4,	// ED2K protocol value (eserver 17.6+)
    ED2KFT_DOCUMENT			= 5,	// ED2K protocol value (eserver 17.6+)
    ED2KFT_ARCHIVE			= 6,	// ED2K protocol value (eserver 17.6+)
    ED2KFT_CDIMAGE			= 7,	// ED2K protocol value (eserver 17.6+)
    ED2KFT_EMULECOLLECTION	= 8
};

CString GetFileTypeByName(LPCTSTR pszFileName);
CString GetFileTypeDisplayStrFromED2KFileType(LPCTSTR pszED2KFileType);
LPCSTR GetED2KFileTypeSearchTerm(EED2KFileType iFileID);
EED2KFileType GetED2KFileTypeSearchID(EED2KFileType iFileID);
EED2KFileType GetED2KFileTypeID(LPCTSTR pszFileName);
bool gotostring(CFile &file, const uchar *find, LONGLONG plen);

///////////////////////////////////////////////////////////////////////////////
// IP/UserID
//
void TriggerPortTest(uint16 tcp, uint16 udp);
bool IsGoodIP(UINT nIP, bool forceCheck = false);
bool IsGoodIPPort(UINT nIP, uint16 nPort);
bool IsLANIP(UINT nIP);
#ifdef NAT_TRAVERSAL
uint8 GetMyConnectOptions(const bool bEncryption, const bool bCallback, const bool bNATTraversal); //>>> WiZaRd::NatTraversal [Xanatos]
#else
uint8 GetMyConnectOptions(const bool bEncryption, const bool bCallback);
#endif
//No longer need seperate lowID checks as we now know the servers just give *.*.*.0 users a lowID
__inline bool IsLowID(UINT id)
{
    return (id < 16777216);
}
CString ipstr(UINT nIP);
CString ipstr(UINT nIP, uint16 nPort);
CString ipstr(LPCTSTR pszAddress, uint16 nPort);
CStringA ipstrA(UINT nIP);
void ipstrA(CHAR* pszAddress, int iMaxAddress, UINT nIP);
__inline CString ipstr(in_addr nIP)
{
    return ipstr(*(UINT*)&nIP);
}
__inline CStringA ipstrA(in_addr nIP)
{
    return ipstrA(*(UINT*)&nIP);
}


///////////////////////////////////////////////////////////////////////////////
// Date/Time
//
time_t safe_mktime(struct tm* ptm);
bool AdjustNTFSDaylightFileTime(UINT& ruFileDate, LPCTSTR pszFilePath);


///////////////////////////////////////////////////////////////////////////////
// Random Numbers
//
uint16 GetRandomUInt16();
UINT GetRandomUInt32();

///////////////////////////////////////////////////////////////////////////////
// RC4 Encryption
//
struct RC4_Key_Struct
{
    uint8 abyState[256];
    uint8 byX;
    uint8 byY;
};

RC4_Key_Struct* RC4CreateKey(const uchar* pachKeyData, UINT nLen, RC4_Key_Struct* key = NULL, bool bSkipDiscard = false);
void RC4Crypt(const uchar* pachIn, uchar* pachOut, UINT nLen, RC4_Key_Struct* key);

//>>> WiZaRd::Functions from removed files
void	InitLocalIP();
UINT	GetLocalIP();
CString	GetLocalIPStr();
void	ResetLocalIP();
CComBSTR	GetLocalIPBSTR();
void	ShowNetworkInfo();

namespace Kademlia
{
class CContact;
};
bool	UpdateNodesDatFromURL(CString strURL);
bool	ContactAdd(Kademlia::CContact* pContact);
void	ContactRem(Kademlia::CContact* pContact);
UINT	GetKadContactCount();
void	SearchAdd();
void	SearchRem();
UINT	GetKadSearchCount();
//<<< WiZaRd::Functions from removed files
//>>> WiZaRd::Additional Functions
CString	GetExtension(const CString& path);
CString GetRegVLCDir();
struct _MIB_UDPTABLE;
typedef struct _MIB_UDPTABLE MIB_UDPTABLE, *PMIB_UDPTABLE;
struct _MIB_TCPTABLE;
typedef struct _MIB_TCPTABLE MIB_TCPTABLE, *PMIB_TCPTABLE;
PMIB_TCPTABLE GetTCPTable();
PMIB_UDPTABLE GetUDPTable();
bool	IsPortInUse(const uint16 port);
bool	IsTCPPortInUse(const uint16 port);
bool	IsUDPPortInUse(const uint16 port);
uint16	GetRandomTCPPort();
uint16	GetRandomUDPPort();
bool	GetDiskSpaceInfo(LPCTSTR pDirectory, uint64& freespace, uint64& totalspace);
bool	CheckURL(CString& strURL);
//>>> WiZaRd::Ratio Indicator
double	GetRatioDouble(const uint64& ul, const uint64& dl);
int		GetRatioSmileyIndex(const double& dRatio);
int		GetRatioSmileyIndex(const uint64& ul, const uint64& dl);
//<<< WiZaRd::Ratio Indicator
CString	GetFileNameFromURL(const CString& strURL);
uint64	GetFileSizeOnDisk(const CString& strFilePath);
bool	RunningWine(); //>>> WiZaRd::Wine Compatibility
void	FillClientIconImageList(CImageList& imageList);
int		GetClientImageIndex(const bool bFriend, const UINT nClientVersion, const bool bPlus, bool bExt);
HCURSOR		CreateHandCursor();
//<<< WiZaRd::Additional Functions
uint8	CalcPrioFromSrcAverage(const UINT srcs, const float avg); //>>> WiZaRd::Improved Auto Prio
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
void DebugRecv(LPCSTR pszMsg, const CUpDownClient* client, const uchar* packet, const CAddress& IP);
void DebugRecv(LPCSTR pszOpcode, const CAddress& IP, uint16 port);
void DebugSend(LPCSTR pszOpcode, const CAddress& IP, uint16 port);
CString ipstr(const CAddress& IP);
//<<< WiZaRd::IPv6 [Xanatos]
#endif
ULONG GetBestInterfaceIP(const ULONG dest_addr); //>>> WiZaRd::Find Best Interface IP [netfinity]
bool IsDirectoryWriteable(LPCTSTR pszDirectory); //>>> WiZaRd
void CreateBetaFile();
void DeleteBetaFile();
void GetPartStartAndEnd(const UINT uPartNumber, const uint64 partsize, const EMFileSize filesize, uint64& uStart, uint64& uEnd);