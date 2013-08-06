//this file is part of eMule
//Copyright (C)2002-2005 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <io.h>
#include <share.h>
#include "emule.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "OtherFunctions.h"
#include "Ini2.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "Statistics.h"
#include "MD5Sum.h"
#include "PartFile.h"
#include "ListenSocket.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "SafeFile.h"
#include "emuledlg.h"
#include "StatisticsDlg.h"
#include "Log.h"
#include "MuleToolbarCtrl.h"
#include "VistaDefines.h"

#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4146) // Einem vorzeichenlosen Typ wurde ein unärer Minus-Operator zugewiesen. Das Ergebnis ist weiterhin vorzeichenlos.
#include <crypto51/osrng.h>
#pragma warning(default:4146) // Einem vorzeichenlosen Typ wurde ein unärer Minus-Operator zugewiesen. Das Ergebnis ist weiterhin vorzeichenlos.
#pragma warning(default:4702) // unreachable code
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPreferences thePrefs;

CString CPreferences::m_astrDefaultDirs[EMULE_DIRCOUNT];
bool	CPreferences::m_abDefaultDirsCreated[EMULE_DIRCOUNT] = { false, false, false, false, false, false, false, false, false, false, false, false};
int		CPreferences::m_nCurrentUserDirMode = eDM_Auto;
int		CPreferences::m_iDbgHeap;
CString	CPreferences::strNick;
uint16	CPreferences::minupload;
uint16	CPreferences::maxupload;
uint16	CPreferences::maxdownload;
LPCSTR	CPreferences::m_pszBindAddrA;
CStringA CPreferences::m_strBindAddrA;
LPCWSTR	CPreferences::m_pszBindAddrW;
CStringW CPreferences::m_strBindAddrW;
uint16	CPreferences::port;
uint16	CPreferences::udpport;
UINT	CPreferences::maxconnections;
UINT	CPreferences::maxhalfconnections;
bool	CPreferences::m_bConditionalTCPAccept;
CString	CPreferences::m_strIncomingDir;
CStringArray CPreferences::tempdir;
bool	CPreferences::updatenotify;
bool	CPreferences::mintotray;
bool	CPreferences::autotakeed2klinks;
bool	CPreferences::addnewfilespaused;
UINT	CPreferences::depth3D;
bool	CPreferences::m_bEnableMiniMule;
int		CPreferences::m_iStraightWindowStyles;
bool	CPreferences::m_bUseSystemFontForMainControls;
bool	CPreferences::m_bRTLWindowsLayout;
CString	CPreferences::m_strSkinProfile;
CString	CPreferences::m_strSkinProfileDir;
UINT	CPreferences::maxsourceperfile;
UINT	CPreferences::trafficOMeterInterval;
UINT	CPreferences::statsInterval;
bool	CPreferences::m_bFillGraphs;
uchar	CPreferences::userhash[16];
WINDOWPLACEMENT CPreferences::EmuleWindowPlacement;
int		CPreferences::maxGraphDownloadRate;
int		CPreferences::maxGraphUploadRate;
UINT	CPreferences::maxGraphUploadRateEstimated = 0;
bool	CPreferences::beepOnError;
bool	CPreferences::m_bIconflashOnNewMessage;
bool	CPreferences::confirmExit;
DWORD	CPreferences::m_adwStatsColors[15];
bool	CPreferences::bHasCustomTaskIconColor;
bool	CPreferences::splashscreen;
bool	CPreferences::filterLANIPs;
bool	CPreferences::m_bAllocLocalHostIP;
bool	CPreferences::onlineSig;
uint64	CPreferences::cumDownOverheadTotal;
uint64	CPreferences::cumDownOverheadFileReq;
uint64	CPreferences::cumDownOverheadSrcEx;
uint64	CPreferences::cumDownOverheadKad;
uint64	CPreferences::cumDownOverheadTotalPackets;
uint64	CPreferences::cumDownOverheadFileReqPackets;
uint64	CPreferences::cumDownOverheadSrcExPackets;
uint64	CPreferences::cumDownOverheadKadPackets;
uint64	CPreferences::cumUpOverheadTotal;
uint64	CPreferences::cumUpOverheadFileReq;
uint64	CPreferences::cumUpOverheadSrcEx;
uint64	CPreferences::cumUpOverheadKad;
uint64	CPreferences::cumUpOverheadTotalPackets;
uint64	CPreferences::cumUpOverheadFileReqPackets;
uint64	CPreferences::cumUpOverheadSrcExPackets;
uint64	CPreferences::cumUpOverheadKadPackets;
UINT	CPreferences::cumUpSuccessfulSessions;
UINT	CPreferences::cumUpFailedSessions;
UINT	CPreferences::cumUpAvgTime;
uint64	CPreferences::cumUpData_EDONKEY;
uint64	CPreferences::cumUpData_EDONKEYHYBRID;
uint64	CPreferences::cumUpData_EMULE;
uint64	CPreferences::cumUpData_KMULE;
uint64	CPreferences::cumUpData_MLDONKEY;
uint64	CPreferences::cumUpData_AMULE;
uint64	CPreferences::cumUpData_EMULECOMPAT;
uint64	CPreferences::cumUpData_SHAREAZA;
uint64	CPreferences::sesUpData_EDONKEY;
uint64	CPreferences::sesUpData_EDONKEYHYBRID;
uint64	CPreferences::sesUpData_EMULE;
uint64	CPreferences::sesUpData_KMULE;
uint64	CPreferences::sesUpData_MLDONKEY;
uint64	CPreferences::sesUpData_AMULE;
uint64	CPreferences::sesUpData_EMULECOMPAT;
uint64	CPreferences::sesUpData_SHAREAZA;
uint64	CPreferences::cumUpDataPort_4662;
uint64	CPreferences::cumUpDataPort_OTHER;
uint64	CPreferences::sesUpDataPort_4662;
uint64	CPreferences::sesUpDataPort_OTHER;
uint64	CPreferences::cumUpData_File;
uint64	CPreferences::cumUpData_Partfile;
uint64	CPreferences::sesUpData_File;
uint64	CPreferences::sesUpData_Partfile;
UINT	CPreferences::cumDownCompletedFiles;
UINT	CPreferences::cumDownSuccessfulSessions;
UINT	CPreferences::cumDownFailedSessions;
UINT	CPreferences::cumDownAvgTime;
uint64	CPreferences::cumLostFromCorruption;
uint64	CPreferences::cumSavedFromCompression;
UINT	CPreferences::cumPartsSavedByICH;
UINT	CPreferences::sesDownSuccessfulSessions;
UINT	CPreferences::sesDownFailedSessions;
UINT	CPreferences::sesDownAvgTime;
UINT	CPreferences::sesDownCompletedFiles;
uint64	CPreferences::sesLostFromCorruption;
uint64	CPreferences::sesSavedFromCompression;
UINT	CPreferences::sesPartsSavedByICH;
uint64	CPreferences::cumDownData_EDONKEY;
uint64	CPreferences::cumDownData_EDONKEYHYBRID;
uint64	CPreferences::cumDownData_EMULE;
uint64	CPreferences::cumDownData_KMULE;
uint64	CPreferences::cumDownData_MLDONKEY;
uint64	CPreferences::cumDownData_AMULE;
uint64	CPreferences::cumDownData_EMULECOMPAT;
uint64	CPreferences::cumDownData_SHAREAZA;
uint64	CPreferences::cumDownData_URL;
uint64	CPreferences::sesDownData_EDONKEY;
uint64	CPreferences::sesDownData_EDONKEYHYBRID;
uint64	CPreferences::sesDownData_EMULE;
uint64	CPreferences::sesDownData_KMULE;
uint64	CPreferences::sesDownData_MLDONKEY;
uint64	CPreferences::sesDownData_AMULE;
uint64	CPreferences::sesDownData_EMULECOMPAT;
uint64	CPreferences::sesDownData_SHAREAZA;
uint64	CPreferences::sesDownData_URL;
uint64	CPreferences::cumDownDataPort_4662;
uint64	CPreferences::cumDownDataPort_OTHER;
uint64	CPreferences::sesDownDataPort_4662;
uint64	CPreferences::sesDownDataPort_OTHER;
float	CPreferences::cumConnAvgDownRate;
float	CPreferences::cumConnMaxAvgDownRate;
float	CPreferences::cumConnMaxDownRate;
float	CPreferences::cumConnAvgUpRate;
float	CPreferences::cumConnMaxAvgUpRate;
float	CPreferences::cumConnMaxUpRate;
time_t	CPreferences::cumConnRunTime;
UINT	CPreferences::cumConnAvgConnections;
UINT	CPreferences::cumConnMaxConnLimitReached;
UINT	CPreferences::cumConnPeakConnections;
UINT	CPreferences::cumConnTransferTime;
UINT	CPreferences::cumConnDownloadTime;
UINT	CPreferences::cumConnUploadTime;
UINT	CPreferences::cumSharedMostFilesShared;
uint64	CPreferences::cumSharedLargestShareSize;
uint64	CPreferences::cumSharedLargestAvgFileSize;
uint64	CPreferences::cumSharedLargestFileSize;
time_t	CPreferences::stat_datetimeLastReset;
UINT	CPreferences::statsConnectionsGraphRatio;
UINT	CPreferences::statsSaveInterval;
CString	CPreferences::m_strStatsExpandedTreeItems;
bool	CPreferences::m_bShowVerticalHourMarkers;
uint64	CPreferences::totalDownloadedBytes;
uint64	CPreferences::totalUploadedBytes;
WORD	CPreferences::m_wLanguageID;
bool	CPreferences::transferDoubleclick;
EViewSharedFilesAccess CPreferences::m_iSeeShares;
bool	CPreferences::bringtoforeground;
UINT	CPreferences::splitterbarPosition;
UINT	CPreferences::splitterbarPositionStat;
UINT	CPreferences::splitterbarPositionStat_HL;
UINT	CPreferences::splitterbarPositionStat_HR;
UINT	CPreferences::splitterbarPositionFriend;
UINT	CPreferences::splitterbarPositionShared;
UINT	CPreferences::m_uTransferWnd1;
UINT	CPreferences::m_uTransferWnd2;
UINT	CPreferences::statsMax;
UINT	CPreferences::statsAverageMinutes;
bool	CPreferences::m_bAddTimeStamp;
bool	CPreferences::m_bRemove2bin;
bool	CPreferences::m_bShowCopyEd2kLinkCmd;
bool	CPreferences::m_bpreviewprio;
bool	CPreferences::startMinimized;
bool	CPreferences::m_bAutoStart;
bool	CPreferences::m_bRestoreLastMainWndDlg;
int		CPreferences::m_iLastMainWndDlgID;
UINT	CPreferences::MaxConperFive;
bool	CPreferences::m_bSparsePartFiles;
CString	CPreferences::m_strYourHostname;
bool	CPreferences::m_bEnableVerboseOptions;
bool	CPreferences::m_bVerbose;
bool	CPreferences::m_bFullVerbose;
bool	CPreferences::m_bDebugSourceExchange;
bool	CPreferences::m_bLogBannedClients;
bool	CPreferences::m_bLogRatingDescReceived;
bool	CPreferences::m_bLogAnalyzerEvents; //>>> WiZaRd::ClientAnalyzer
bool	CPreferences::m_bLogSecureIdent;
bool	CPreferences::m_bLogFilteredIPs;
bool	CPreferences::m_bLogFileSaving;
bool	CPreferences::m_bLogA4AF; // ZZ:DownloadManager
bool	CPreferences::m_bLogUlDlEvents;
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
bool	CPreferences::m_bUseDebugDevice = true;
int		CPreferences::m_iDebugClientTCPLevel;
int		CPreferences::m_iDebugClientUDPLevel;
int		CPreferences::m_iDebugClientKadUDPLevel;
int		CPreferences::m_iDebugSearchResultDetailLevel;
#endif
bool	CPreferences::m_bupdatequeuelist;
int		CPreferences::m_istartnextfile;
bool	CPreferences::m_bshowoverhead;
bool	CPreferences::m_bDAP;
bool	CPreferences::m_bUAP;
bool	CPreferences::m_bDisableKnownClientList;
bool	CPreferences::m_bDisableQueueList;
bool	CPreferences::m_bExtControls;
bool	CPreferences::m_bTransflstRemain;
UINT	CPreferences::versioncheckdays;
bool	CPreferences::showRatesInTitle;
CString	CPreferences::m_strTxtEditor;
CString	CPreferences::m_strVideoPlayer;
CString CPreferences::m_strVideoPlayerArgs;
bool	CPreferences::moviePreviewBackup;
int		CPreferences::m_iPreviewSmallBlocks;
bool	CPreferences::m_bPreviewCopiedArchives;
CString CPreferences::m_strExtractFolder;
bool	CPreferences::m_bExtractToIncomingDir;
bool	CPreferences::m_bExtractArchives;
int		CPreferences::m_iInspectAllFileTypes;
bool	CPreferences::m_bPreviewOnIconDblClk;
bool	CPreferences::m_bCheckFileOpen;
bool	CPreferences::indicateratings;
bool	CPreferences::watchclipboard;
bool	CPreferences::m_bFirstStart;
bool	CPreferences::m_bUpdate;
bool	CPreferences::log2disk;
bool	CPreferences::m_bLogAnalyzerToDisk; //>>> WiZaRd::ClientAnalyzer
bool	CPreferences::debug2disk;
int		CPreferences::iMaxLogBuff;
UINT	CPreferences::uMaxLogFileSize;
ELogFileFormat CPreferences::m_iLogFileFormat = Unicode;
bool	CPreferences::dontcompressavi;
bool	CPreferences::msgonlyfriends;
bool	CPreferences::msgsecure;
UINT	CPreferences::filterlevel;
UINT	CPreferences::m_iFileBufferSize;
UINT	CPreferences::m_uFileBufferTimeLimit;
UINT	CPreferences::m_iQueueSize;
int		CPreferences::m_iCommitFiles;
UINT	CPreferences::maxmsgsessions;
UINT	CPreferences::versioncheckLastAutomatic;
CString	CPreferences::messageFilter;
CString	CPreferences::commentFilter;
CString	CPreferences::filenameCleanups;
CString	CPreferences::m_strDateTimeFormat;
CString	CPreferences::m_strDateTimeFormat4Log;
CString	CPreferences::m_strDateTimeFormat4Lists;
LOGFONT CPreferences::m_lfHyperText;
LOGFONT CPreferences::m_lfLogText;
COLORREF CPreferences::m_crLogError = RGB(255, 0, 0);
//>>> WiZaRd::"Proper" Colors
//showing warnings in blue and success in green makes more sense :P
COLORREF CPreferences::m_crLogWarning = RGB(0, 0, 255);
COLORREF CPreferences::m_crLogSuccess = RGB(0, 192, 0);
//COLORREF CPreferences::m_crLogWarning = RGB(128, 0, 128);
//COLORREF CPreferences::m_crLogSuccess = RGB(0, 0, 255);
//<<< WiZaRd::"Proper" Colors
int		CPreferences::m_iExtractMetaData;
bool	CPreferences::m_bAdjustNTFSDaylightFileTime = true;
bool	CPreferences::m_bRearrangeKadSearchKeywords;
ProxySettings CPreferences::proxy;
bool	CPreferences::showCatTabInfos;
bool	CPreferences::resumeSameCat;
bool	CPreferences::dontRecreateGraphs;
bool	CPreferences::autofilenamecleanup;
bool	CPreferences::m_bUseAutocompl;
bool	CPreferences::m_bShowDwlPercentage;
bool	CPreferences::m_bRemoveFinishedDownloads;
UINT	CPreferences::m_iMaxChatHistory;
bool	CPreferences::m_bShowActiveDownloadsBold;
int		CPreferences::m_iSearchMethod;
EToolbarLabelType CPreferences::m_nToolbarLabels;
CString	CPreferences::m_sToolbarBitmap;
CString	CPreferences::m_sToolbarBitmapFolder;
CString	CPreferences::m_sToolbarSettings;
bool	CPreferences::m_bReBarToolbar;
CSize	CPreferences::m_sizToolbarIconSize;
bool	CPreferences::m_bPreviewEnabled;
bool	CPreferences::m_bAutomaticArcPreviewStart;
bool	CPreferences::m_bDynUpEnabled;
int		CPreferences::m_iDynUpPingTolerance;
int		CPreferences::m_iDynUpGoingUpDivider;
int		CPreferences::m_iDynUpGoingDownDivider;
int		CPreferences::m_iDynUpNumberOfPings;
int		CPreferences::m_iDynUpPingToleranceMilliseconds;
bool	CPreferences::m_bDynUpUseMillisecondPingTolerance;
bool    CPreferences::m_bAllocFull;
bool	CPreferences::m_bShowSharedFilesDetails;
bool	CPreferences::m_bShowUpDownIconInTaskbar;
bool	CPreferences::m_bForceSpeedsToKB;
bool	CPreferences::m_bAutoShowLookups;
bool	CPreferences::m_bExtraPreviewWithMenu;

// ZZ:DownloadManager -->
bool    CPreferences::m_bA4AFSaveCpu;
// ZZ:DownloadManager <--
bool    CPreferences::m_bHighresTimer;
bool	CPreferences::m_bResolveSharedShellLinks;
bool	CPreferences::m_bKeepUnavailableFixedSharedDirs;
CStringList CPreferences::shareddir_list;
CList<uint8> CPreferences::shareddir_list_permissions; //>>> WiZaRd::SharePermissions
CStringList CPreferences::addresses_list;
CString CPreferences::m_strFileCommentsFilePath;
Preferences_Ext_Struct* CPreferences::prefsExt;
WORD	CPreferences::m_wWinVer;
CArray<Category_Struct*,Category_Struct*> CPreferences::catMap;
bool	CPreferences::m_bRunAsUser;
bool	CPreferences::m_bPreferRestrictedOverUser;
bool	CPreferences::m_bUseOldTimeRemaining;

bool	CPreferences::m_bOpenPortsOnStartUp;
int		CPreferences::m_byLogLevel;
bool	CPreferences::m_bTrustEveryHash;
bool	CPreferences::m_bRememberCancelledFiles;
bool	CPreferences::m_bRememberDownloadedFiles;
bool	CPreferences::m_bPartiallyPurgeOldKnownFiles;

bool	CPreferences::m_bWinaTransToolbar;
bool	CPreferences::m_bShowDownloadToolbar;

bool	CPreferences::m_bCryptLayerRequested;
bool	CPreferences::m_bCryptLayerRequired;
UINT	CPreferences::m_dwKadUDPKey;
uint8	CPreferences::m_byCryptTCPPaddingLength;

bool	CPreferences::m_bSkipWANIPSetup;
bool	CPreferences::m_bSkipWANPPPSetup;
bool	CPreferences::m_bEnableUPnP;
bool	CPreferences::m_bCloseUPnPOnExit;
bool	CPreferences::m_bIsWinServImplDisabled;
bool	CPreferences::m_bIsMinilibImplDisabled;
int		CPreferences::m_nLastWorkingImpl;

BOOL	CPreferences::m_bIsRunningAeroGlass;
bool	CPreferences::m_bPreventStandby;
bool	CPreferences::m_bStoreSearches;
//>>> WiZaRd::IPFilter-Update
CString	CPreferences::m_strUpdateURLIPFilter;
bool	CPreferences::m_bAutoUpdateIPFilter;
UINT	CPreferences::m_uiIPfilterVersion;
//<<< WiZaRd::IPFilter-Update
//>>> WiZaRd::MediaInfoDLL Update
bool	CPreferences::m_bMediaInfoDllAutoUpdate;
CString	CPreferences::m_strMediaInfoDllUpdateURL;
UINT	CPreferences::m_uiMediaInfoDllVersion;
//<<< WiZaRd::MediaInfoDLL Update
//>>> PreviewIndicator [WiZaRd]
uint8	CPreferences::m_uiPreviewIndicatorMode;
COLORREF CPreferences::m_crPreviewReadyColor; //>>> jerrybg::ColorPreviewReadyFiles [WiZaRd]
//<<< PreviewIndicator [WiZaRd]
//>>> WiZaRd::Advanced Transfer Window Layout [Stulle]
bool	CPreferences::m_bSplitWindow;
//<<< WiZaRd::Advanced Transfer Window Layout [Stulle]
//>>> WiZaRd::SimpleProgress
bool	CPreferences::m_bUseSimpleProgress;
//<<< WiZaRd::SimpleProgress
//>>> WiZaRd::AntiFake
uint8	CPreferences::m_uiSpamFilterMode;
//<<< WiZaRd::AntiFake
//>>> WiZaRd::AutoHL
uint16	CPreferences::m_iAutoHLUpdateTimer;
uint16	CPreferences::m_iMinAutoHL;
uint16	CPreferences::m_iMaxAutoHL;
sint8	CPreferences::m_iUseAutoHL;
uint16  CPreferences::m_iMaxSourcesHL;
//<<< WiZaRd::AutoHL
//>>> WiZaRd::Remove forbidden files
bool	CPreferences::m_bRemoveForbiddenFiles;
// do not share a file with one of these patterns because they are practically worthless
CString	CPreferences::m_strForbiddenFileFilters = L".fb!|.jc!|.antifrag|.dctmp|.bc!|.!ut|.getright|.partial|.partial.sd|.part|.part.met|.part.met.bak|.part.met.backup";
//<<< WiZaRd::Remove forbidden files
//>>> WiZaRd::Drop Blocking Sockets [Xman?]
bool	CPreferences::m_bDropBlockingSession = true;
bool	CPreferences::m_bDropBlockingSockets;
float	CPreferences::m_fMaxBlockRate;
float	CPreferences::m_fMaxBlockRate20;
//<<< WiZaRd::Drop Blocking Sockets [Xman?]
bool	CPreferences::m_bNeedsWineCompatibility; //>>> WiZaRd::Wine Compatibility
//>>> WiZaRd::ModIconDLL Update
bool	CPreferences::m_bModIconDllAutoUpdate;
CString	CPreferences::m_strModIconDllUpdateURL;
//<<< WiZaRd::ModIconDLL Update
//>>> Tux::Spread Priority v3
int		CPreferences::m_iSpreadPrioLimit;
//<<< Tux::Spread Priority v3

CPreferences::CPreferences()
{
#ifdef _DEBUG
    m_iDbgHeap = 1;
#endif
}

CPreferences::~CPreferences()
{
    delete prefsExt;
}

LPCTSTR CPreferences::GetConfigFile()
{
    return theApp.m_pszProfileName;
}

void CPreferences::Init()
{
    srand((UINT)time(0)); // we need random numbers sometimes

    prefsExt = new Preferences_Ext_Struct;
    memset(prefsExt, 0, sizeof *prefsExt);

    m_strFileCommentsFilePath = GetMuleDirectory(EMULE_CONFIGDIR) + L"fileinfo.ini";

    ///////////////////////////////////////////////////////////////////////////
    // Move *.log files from application directory into 'log' directory
    //
    CFileFind ff;
    BOOL bFoundFile = ff.FindFile(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"eMule*.log", 0);
    while (bFoundFile)
    {
        bFoundFile = ff.FindNextFile();
        if (ff.IsDots() || ff.IsSystem() || ff.IsDirectory() || ff.IsHidden())
            continue;
        MoveFile(ff.GetFilePath(), GetMuleDirectory(EMULE_LOGDIR) + ff.GetFileName());
    }
    ff.Close();

    ///////////////////////////////////////////////////////////////////////////
    // Move 'downloads.txt/bak' files from application and/or data-base directory
    // into 'config' directory
    //
    if (PathFileExists(GetMuleDirectory(EMULE_DATABASEDIR) + L"downloads.txt"))
        MoveFile(GetMuleDirectory(EMULE_DATABASEDIR) + L"downloads.txt", GetMuleDirectory(EMULE_CONFIGDIR) + L"downloads.txt");
    if (PathFileExists(GetMuleDirectory(EMULE_DATABASEDIR) + L"downloads.bak"))
        MoveFile(GetMuleDirectory(EMULE_DATABASEDIR) + L"downloads.bak", GetMuleDirectory(EMULE_CONFIGDIR) + L"downloads.bak");
    if (PathFileExists(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"downloads.txt"))
        MoveFile(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"downloads.txt", GetMuleDirectory(EMULE_CONFIGDIR) + L"downloads.txt");
    if (PathFileExists(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"downloads.bak"))
        MoveFile(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"downloads.bak", GetMuleDirectory(EMULE_CONFIGDIR) + L"downloads.bak");

    CreateUserHash();

    // load preferences.dat or set standart values
    CString strFullPath;
    strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"preferences.dat";
    FILE* preffile = _tfsopen(strFullPath, L"rb", _SH_DENYWR);

    LoadPreferences();

    if (!preffile)
    {
        SetStandartValues();
    }
    else
    {
        if (fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile) != 1 || ferror(preffile))
            SetStandartValues();

        md4cpy(userhash, prefsExt->userhash);
        EmuleWindowPlacement = prefsExt->EmuleWindowPlacement;

        fclose(preffile);
    }

    // shared directories
    strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"shareddir.dat";
    CStdioFile* sdirfile = new CStdioFile();
    bool bIsUnicodeFile = IsUnicodeFile(strFullPath); // check for BOM
    // open the text file either in ANSI (text) or Unicode (binary), this way we can read old and new files
    // with nearly the same code..
    if (sdirfile->Open(strFullPath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
    {
        try
        {
            if (bIsUnicodeFile)
                sdirfile->Seek(sizeof(WORD), SEEK_CUR); // skip BOM

            CString toadd;
            while (sdirfile->ReadString(toadd))
            {
                toadd.Trim(L" \t\r\n"); // need to trim '\r' in binary mode
                if (toadd.IsEmpty())
                    continue;

                TCHAR szFullPath[MAX_PATH];
                if (PathCanonicalize(szFullPath, toadd))
                    toadd = szFullPath;

                if (!IsShareableDirectory(toadd))
                    continue;

                // Skip non-existing directories from fixed disks only
                int iDrive = PathGetDriveNumber(toadd);
                if (iDrive >= 0 && iDrive <= 25)
                {
                    WCHAR szRootPath[4] = L" :\\";
                    szRootPath[0] = (WCHAR)(L'A' + iDrive);
                    if (GetDriveType(szRootPath) == DRIVE_FIXED && !m_bKeepUnavailableFixedSharedDirs)
                    {
                        if (_taccess(toadd, 0) != 0)
                            continue;
                    }
                }

                if (toadd.Right(1) != L'\\')
                    toadd.Append(L"\\");
                shareddir_list.AddHead(toadd);
                shareddir_list_permissions.AddHead(eSP_Options); //>>> WiZaRd::SharePermissions
            }
            sdirfile->Close();
        }
        catch (CFileException* ex)
        {
            ASSERT(0);
            ex->Delete();
        }
    }
    delete sdirfile;

//>>> WiZaRd::SharePermissions
    _tremove(strFullPath); //delete the old file (if present) because we will use a new file format

    // shared directories
    strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"shareddir_perm.dat";
    sdirfile = new CStdioFile();
    bIsUnicodeFile = IsUnicodeFile(strFullPath); // check for BOM
    // open the text file either in ANSI (text) or Unicode (binary), this way we can read old and new files
    // with nearly the same code..
    if (sdirfile->Open(strFullPath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
    {
        try
        {
            if (bIsUnicodeFile)
                sdirfile->Seek(sizeof(WORD), SEEK_CUR); // skip BOM

            CString toadd;
            while (sdirfile->ReadString(toadd))
            {
                toadd.Trim(L" \t\r\n"); // need to trim '\r' in binary mode
                if (toadd.IsEmpty())
                    continue;

                TCHAR szFullPath[MAX_PATH];
                if (PathCanonicalize(szFullPath, toadd))
                    toadd = szFullPath;

                if (!IsShareableDirectory(toadd))
                {
                    sdirfile->ReadString(toadd); //>>> WiZaRd::SharePermissions - read in and discard the permission setting
                    continue;
                }

                // Skip non-existing directories from fixed disks only
                int iDrive = PathGetDriveNumber(toadd);
                if (iDrive >= 0 && iDrive <= 25)
                {
                    WCHAR szRootPath[4] = L" :\\";
                    szRootPath[0] = (WCHAR)(L'A' + iDrive);
                    if (GetDriveType(szRootPath) == DRIVE_FIXED && !m_bKeepUnavailableFixedSharedDirs)
                    {
                        if (_taccess(toadd, 0) != 0)
                        {
                            sdirfile->ReadString(toadd); //>>> WiZaRd::SharePermissions - read in and discard the permission setting
                            continue;
                        }
                    }
                }

                if (toadd.Right(1) != L'\\')
                    toadd.Append(L"\\");
                shareddir_list.AddHead(toadd);

                sdirfile->ReadString(toadd);
                shareddir_list_permissions.AddHead((uint8)_tstoi(toadd));
            }
            sdirfile->Close();
        }
        catch (CFileException* ex)
        {
            ASSERT(0);
            ex->Delete();
        }
    }
    delete sdirfile;
//<<< WiZaRd::SharePermissions

    userhash[5] = 14;
    userhash[14] = 111;

    // Explicitly inform the user about errors with incoming/temp folders!
    if (!PathFileExists(GetMuleDirectory(EMULE_INCOMINGDIR)) && !::CreateDirectory(GetMuleDirectory(EMULE_INCOMINGDIR),0))
    {
        /*CString strError;
        strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetMuleDirectory(EMULE_INCOMINGDIR), GetErrorMessage(GetLastError()));
        AfxMessageBox(strError, MB_ICONERROR);*/

        m_strIncomingDir = GetDefaultDirectory(EMULE_INCOMINGDIR, true); // will also try to create it if needed
        /*if (!PathFileExists(GetMuleDirectory(EMULE_INCOMINGDIR)))
        {
            strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetMuleDirectory(EMULE_INCOMINGDIR), GetErrorMessage(GetLastError()));
            AfxMessageBox(strError, MB_ICONERROR);
        }*/
    }
    if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0))
    {
        /*CString strError;
        strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
        AfxMessageBox(strError, MB_ICONERROR);*/

        tempdir.SetAt(0, GetDefaultDirectory(EMULE_TEMPDIR, true)); // will also try to create it if needed);
        /*if (!PathFileExists(GetTempDir()))
        {
            strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
            AfxMessageBox(strError, MB_ICONERROR);
        }*/
    }

    // Create 'skins' directory
    if (!PathFileExists(GetMuleDirectory(EMULE_SKINDIR)) && !CreateDirectory(GetMuleDirectory(EMULE_SKINDIR), 0))
        m_strSkinProfileDir = GetDefaultDirectory(EMULE_SKINDIR, true); // will also try to create it if needed

    // Create 'toolbars' directory
    if (!PathFileExists(GetMuleDirectory(EMULE_TOOLBARDIR)) && !CreateDirectory(GetMuleDirectory(EMULE_TOOLBARDIR), 0))
        m_sToolbarBitmapFolder = GetDefaultDirectory(EMULE_TOOLBARDIR, true); // will also try to create it if needed;

    if (isnulmd4(userhash))
        CreateUserHash();
}

void CPreferences::Uninit()
{
    while (!catMap.IsEmpty())
    {
        Category_Struct* delcat = catMap.GetAt(0);
        catMap.RemoveAt(0);
        delete delcat;
    }
}

void CPreferences::SetStandartValues()
{
    CreateUserHash();

    WINDOWPLACEMENT defaultWPM;
    defaultWPM.length = sizeof(WINDOWPLACEMENT);
    defaultWPM.rcNormalPosition.left=10;
    defaultWPM.rcNormalPosition.top=10;
    defaultWPM.rcNormalPosition.right=700;
    defaultWPM.rcNormalPosition.bottom=500;
    defaultWPM.showCmd=0;
    EmuleWindowPlacement=defaultWPM;
    versioncheckLastAutomatic=0;

//	Save();
}

bool CPreferences::IsTempFile(const CString& rstrDirectory, const CString& rstrName)
{
    bool bFound = false;
    for (int i=0; i<tempdir.GetCount() && !bFound; i++)
        if (CompareDirectories(rstrDirectory, GetTempDir(i))==0)
            bFound = true; //ok, found a directory

    if (!bFound) //found nowhere - not a tempfile...
        return false;

    // do not share a file from the temp directory, if it matches one of the following patterns
    CString strNameLower(rstrName);
    strNameLower.MakeLower();
    strNameLower += L"|"; // append an EOS character which we can query for
    static const LPCTSTR _apszNotSharedExts[] =
    {
        L"%u.part" L"%c",
        L"%u.part.met" L"%c",
        L"%u.part.met" PARTMET_BAK_EXT L"%c",
        L"%u.part.met" PARTMET_TMP_EXT L"%c"
    };
    for (int i = 0; i < _countof(_apszNotSharedExts); i++)
    {
        UINT uNum;
        TCHAR iChar;
        // "misuse" the 'scanf' function for a very simple pattern scanning.
        if (_stscanf(strNameLower, _apszNotSharedExts[i], &uNum, &iChar) == 2 && iChar == L'|')
            return true;
    }

    return false;
}

uint16 CPreferences::GetMaxDownload()
{
//>>> WiZaRd::SessionRatio
    //no need to limit that here, it will be limited dynamically!
    return maxdownload;
//  return (uint16)(GetMaxDownloadInBytesPerSec()/1024);
//<<< WiZaRd::SessionRatio
}

uint64 CPreferences::GetMaxDownloadInBytesPerSec(bool dynamic)
{
//>>> WiZaRd::SessionRatio
    uint64 uimaxDownload = maxdownload*1024;
    //WiZaRd: if you CAN not upload, why should you be punished!?
    if (theApp.uploadqueue && theApp.uploadqueue->GetWaitingUserCount() && theApp.uploadqueue->GetUploadQueueLength())
    {
        //Session limit, are we leeching?
        //Don't download more than 3 times our upload (multiFU, partial PS)
        const double uploaded = max(SESSIONMAXTRANS, (double)theStats.sessionSentBytes - (double)theStats.sessionSentBytesToFriend /*- (double)theStats.sessionSentBytesViaPartPS*/); //>>> WiZaRd::MultiFU //>>> WiZaRd::PowerShare
        const double sessionUlDlRatio = (uploaded + 1.0) / (theStats.sessionReceivedBytes - sesDownData_URL + 1.0); //>>> WiZaRd::Exclude HTTP Traffic [Quezl]
        if (sessionUlDlRatio < 0.4) //Activate throttling if close to limit
        {
            //if we cannot upload, yet, then it's not our fault!
            const UINT currUpload = theApp.uploadqueue->GetDatarate();
            const UINT minDownload = max(currUpload, 10240); //we will let them at least 10k DL
            uimaxDownload = (uint64)min(max(minDownload, (9*currUpload*sessionUlDlRatio)), uimaxDownload);
        }
    }
//<<< WiZaRd::SessionRatio

    //dont be a Lam3r :)
    UINT maxup;
    if (dynamic && thePrefs.IsDynUpEnabled() && theApp.uploadqueue->GetWaitingUserCount() != 0 && theApp.uploadqueue->GetDatarate() != 0)
        maxup = theApp.uploadqueue->GetDatarate();
    else
        maxup = GetMaxUpload()*1024;

//>>> WiZaRd::SessionRatio
    uint64 uioffimaxDownload = 0;
    if (maxup < 4*1024)
        uioffimaxDownload = (((maxup < 10*1024) && ((uint64)maxup*3 < maxdownload*1024))? (uint64)maxup*3 : maxdownload*1024);
    else
        uioffimaxDownload = (((maxup < 10*1024) && ((uint64)maxup*4 < maxdownload*1024))? (uint64)maxup*4 : maxdownload*1024);

    //we use the lower one to respect the (IMHO dumb) rules of official client
    //Why is it dumb?
    //Because someone may download at 10000kB and upload 10kB (1:1000) but
    //one may not download with more than 9kB if he can only upload 3kB (1:3)
    //Pretty unfair... and that's why eMule IS already favoring leechers
    //by allowing incredible leeching! *narf*!
    //But we use a sessionratio to limit it at least a *bit*
    return min(uimaxDownload, uioffimaxDownload);
//<<< WiZaRd::SessionRatio
}

// -khaos--+++> A whole bunch of methods!  Keep going until you reach the end tag.
void CPreferences::SaveStats(int bBackUp)
{
    // This function saves all of the new statistics in my addon.  It is also used to
    // save backups for the Reset Stats function, and the Restore Stats function (Which is actually LoadStats)
    // bBackUp = 0: DEFAULT; save to statistics.ini
    // bBackUp = 1: Save to statbkup.ini, which is used to restore after a reset
    // bBackUp = 2: Save to statbkuptmp.ini, which is temporarily created during a restore and then renamed to statbkup.ini

    CString strFullPath(GetMuleDirectory(EMULE_CONFIGDIR));
    if (bBackUp == 1)
        strFullPath += L"statbkup.ini";
    else if (bBackUp == 2)
        strFullPath += L"statbkuptmp.ini";
    else
        strFullPath += L"statistics.ini";

    CIni ini(strFullPath, L"Statistics");

    // Save cumulative statistics to preferences.ini, going in order as they appear in CStatisticsDlg::ShowStatistics.
    // We do NOT SET the values in prefs struct here.

    // Save Cum Down Data
    ini.WriteUInt64(L"TotalDownloadedBytes", theStats.sessionReceivedBytes + GetTotalDownloaded());
    ini.WriteInt(L"DownSuccessfulSessions", cumDownSuccessfulSessions);
    ini.WriteInt(L"DownFailedSessions", cumDownFailedSessions);
    ini.WriteInt(L"DownAvgTime", (GetDownC_AvgTime() + GetDownS_AvgTime()) / 2);
    ini.WriteUInt64(L"LostFromCorruption", cumLostFromCorruption + sesLostFromCorruption);
    ini.WriteUInt64(L"SavedFromCompression", sesSavedFromCompression + cumSavedFromCompression);
    ini.WriteInt(L"PartsSavedByICH", cumPartsSavedByICH + sesPartsSavedByICH);

    ini.WriteUInt64(L"DownData_EDONKEY", GetCumDownData_EDONKEY());
    ini.WriteUInt64(L"DownData_EDONKEYHYBRID", GetCumDownData_EDONKEYHYBRID());
    ini.WriteUInt64(L"DownData_EMULE", GetCumDownData_EMULE());
    ini.WriteUInt64(L"DownData_KMULE", GetCumDownData_KMULE());
    ini.WriteUInt64(L"DownData_MLDONKEY", GetCumDownData_MLDONKEY());
    ini.WriteUInt64(L"DownData_LMULE", GetCumDownData_EMULECOMPAT());
    ini.WriteUInt64(L"DownData_AMULE", GetCumDownData_AMULE());
    ini.WriteUInt64(L"DownData_SHAREAZA", GetCumDownData_SHAREAZA());
    ini.WriteUInt64(L"DownData_URL", GetCumDownData_URL());
    ini.WriteUInt64(L"DownDataPort_4662", GetCumDownDataPort_4662());
    ini.WriteUInt64(L"DownDataPort_OTHER", GetCumDownDataPort_OTHER());

    ini.WriteUInt64(L"DownOverheadTotal",theStats.GetDownDataOverheadFileRequest() +
                    theStats.GetDownDataOverheadSourceExchange() +
                    theStats.GetDownDataOverheadKad() +
                    theStats.GetDownDataOverheadOther() +
                    GetDownOverheadTotal());
    ini.WriteUInt64(L"DownOverheadFileReq", theStats.GetDownDataOverheadFileRequest() + GetDownOverheadFileReq());
    ini.WriteUInt64(L"DownOverheadSrcEx", theStats.GetDownDataOverheadSourceExchange() + GetDownOverheadSrcEx());
    ini.WriteUInt64(L"DownOverheadKad", theStats.GetDownDataOverheadKad() + GetDownOverheadKad());

    ini.WriteUInt64(L"DownOverheadTotalPackets", theStats.GetDownDataOverheadFileRequestPackets() +
                    theStats.GetDownDataOverheadSourceExchangePackets() +
                    theStats.GetDownDataOverheadKadPackets() +
                    theStats.GetDownDataOverheadOtherPackets() +
                    GetDownOverheadTotalPackets());
    ini.WriteUInt64(L"DownOverheadFileReqPackets", theStats.GetDownDataOverheadFileRequestPackets() + GetDownOverheadFileReqPackets());
    ini.WriteUInt64(L"DownOverheadSrcExPackets", theStats.GetDownDataOverheadSourceExchangePackets() + GetDownOverheadSrcExPackets());
    ini.WriteUInt64(L"DownOverheadKadPackets", theStats.GetDownDataOverheadKadPackets() + GetDownOverheadKadPackets());

    // Save Cumulative Upline Statistics
    ini.WriteUInt64(L"TotalUploadedBytes", theStats.sessionSentBytes + GetTotalUploaded());
    ini.WriteInt(L"UpSuccessfulSessions", theApp.uploadqueue->GetSuccessfullUpCount() + GetUpSuccessfulSessions());
    ini.WriteInt(L"UpFailedSessions", theApp.uploadqueue->GetFailedUpCount() + GetUpFailedSessions());
    ini.WriteInt(L"UpAvgTime", (theApp.uploadqueue->GetAverageUpTime() + GetUpAvgTime())/2);
    ini.WriteUInt64(L"UpData_EDONKEY", GetCumUpData_EDONKEY());
    ini.WriteUInt64(L"UpData_EDONKEYHYBRID", GetCumUpData_EDONKEYHYBRID());
    ini.WriteUInt64(L"UpData_EMULE", GetCumUpData_EMULE());
    ini.WriteUInt64(L"UpData_KMULE", GetCumUpData_KMULE());
    ini.WriteUInt64(L"UpData_MLDONKEY", GetCumUpData_MLDONKEY());
    ini.WriteUInt64(L"UpData_LMULE", GetCumUpData_EMULECOMPAT());
    ini.WriteUInt64(L"UpData_AMULE", GetCumUpData_AMULE());
    ini.WriteUInt64(L"UpData_SHAREAZA", GetCumUpData_SHAREAZA());
    ini.WriteUInt64(L"UpDataPort_4662", GetCumUpDataPort_4662());
    ini.WriteUInt64(L"UpDataPort_OTHER", GetCumUpDataPort_OTHER());
    ini.WriteUInt64(L"UpData_File", GetCumUpData_File());
    ini.WriteUInt64(L"UpData_Partfile", GetCumUpData_Partfile());

    ini.WriteUInt64(L"UpOverheadTotal", theStats.GetUpDataOverheadFileRequest() +
                    theStats.GetUpDataOverheadSourceExchange() +
                    theStats.GetUpDataOverheadKad() +
                    theStats.GetUpDataOverheadOther() +
                    GetUpOverheadTotal());
    ini.WriteUInt64(L"UpOverheadFileReq", theStats.GetUpDataOverheadFileRequest() + GetUpOverheadFileReq());
    ini.WriteUInt64(L"UpOverheadSrcEx", theStats.GetUpDataOverheadSourceExchange() + GetUpOverheadSrcEx());
    ini.WriteUInt64(L"UpOverheadKad", theStats.GetUpDataOverheadKad() + GetUpOverheadKad());

    ini.WriteUInt64(L"UpOverheadTotalPackets", theStats.GetUpDataOverheadFileRequestPackets() +
                    theStats.GetUpDataOverheadSourceExchangePackets() +
                    theStats.GetUpDataOverheadKadPackets() +
                    theStats.GetUpDataOverheadOtherPackets() +
                    GetUpOverheadTotalPackets());
    ini.WriteUInt64(L"UpOverheadFileReqPackets", theStats.GetUpDataOverheadFileRequestPackets() + GetUpOverheadFileReqPackets());
    ini.WriteUInt64(L"UpOverheadSrcExPackets", theStats.GetUpDataOverheadSourceExchangePackets() + GetUpOverheadSrcExPackets());
    ini.WriteUInt64(L"UpOverheadKadPackets", theStats.GetUpDataOverheadKadPackets() + GetUpOverheadKadPackets());

    // Save Cumulative Connection Statistics
    float tempRate = 0.0F;

    // Download Rate Average
    tempRate = theStats.GetAvgDownloadRate(AVG_TOTAL);
    ini.WriteFloat(L"ConnAvgDownRate", tempRate);

    // Max Download Rate Average
    if (tempRate > GetConnMaxAvgDownRate())
        SetConnMaxAvgDownRate(tempRate);
    ini.WriteFloat(L"ConnMaxAvgDownRate", GetConnMaxAvgDownRate());

    // Max Download Rate
    tempRate = (float)theApp.downloadqueue->GetDatarate() / 1024;
    if (tempRate > GetConnMaxDownRate())
        SetConnMaxDownRate(tempRate);
    ini.WriteFloat(L"ConnMaxDownRate", GetConnMaxDownRate());

    // Upload Rate Average
    tempRate = theStats.GetAvgUploadRate(AVG_TOTAL);
    ini.WriteFloat(L"ConnAvgUpRate", tempRate);

    // Max Upload Rate Average
    if (tempRate > GetConnMaxAvgUpRate())
        SetConnMaxAvgUpRate(tempRate);
    ini.WriteFloat(L"ConnMaxAvgUpRate", GetConnMaxAvgUpRate());

    // Max Upload Rate
    tempRate = (float)theApp.uploadqueue->GetDatarate() / 1024;
    if (tempRate > GetConnMaxUpRate())
        SetConnMaxUpRate(tempRate);
    ini.WriteFloat(L"ConnMaxUpRate", GetConnMaxUpRate());

    // Overall Run Time
    ini.WriteInt(L"ConnRunTime", (UINT)((GetTickCount() - theStats.starttime)/1000 + GetConnRunTime()));

    // Peak Connections
    if (theApp.listensocket->GetPeakConnections() > cumConnPeakConnections)
        cumConnPeakConnections = theApp.listensocket->GetPeakConnections();
    ini.WriteInt(L"ConnPeakConnections", cumConnPeakConnections);

    // Max Connection Limit Reached
    if (theApp.listensocket->GetMaxConnectionReached() + cumConnMaxConnLimitReached > cumConnMaxConnLimitReached)
        ini.WriteInt(L"ConnMaxConnLimitReached", theApp.listensocket->GetMaxConnectionReached() + cumConnMaxConnLimitReached);

    // Time Stuff...
    ini.WriteInt(L"ConnTransferTime", GetConnTransferTime() + theStats.GetTransferTime());
    ini.WriteInt(L"ConnUploadTime", GetConnUploadTime() + theStats.GetUploadTime());
    ini.WriteInt(L"ConnDownloadTime", GetConnDownloadTime() + theStats.GetDownloadTime());

    // Compare and Save Shared File Records
    if ((UINT)theApp.sharedfiles->GetCount() > cumSharedMostFilesShared)
        cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
    ini.WriteInt(L"SharedMostFilesShared", cumSharedMostFilesShared);

    uint64 bytesLargestFile = 0;
    uint64 allsize = theApp.sharedfiles->GetDatasize(bytesLargestFile);
    if (allsize > cumSharedLargestShareSize)
        cumSharedLargestShareSize = allsize;
    ini.WriteUInt64(L"SharedLargestShareSize", cumSharedLargestShareSize);
    if (bytesLargestFile > cumSharedLargestFileSize)
        cumSharedLargestFileSize = bytesLargestFile;
    ini.WriteUInt64(L"SharedLargestFileSize", cumSharedLargestFileSize);

    if (theApp.sharedfiles->GetCount() != 0)
    {
        uint64 tempint = allsize/theApp.sharedfiles->GetCount();
        if (tempint > cumSharedLargestAvgFileSize)
            cumSharedLargestAvgFileSize = tempint;
    }

    ini.WriteUInt64(L"SharedLargestAvgFileSize", cumSharedLargestAvgFileSize);
    ini.WriteInt(L"statsDateTimeLastReset", stat_datetimeLastReset);

    // If we are saving a back-up or a temporary back-up, return now.
    if (bBackUp != 0)
        return;
}

void CPreferences::SetRecordStructMembers()
{

    // The purpose of this function is to be called from CStatisticsDlg::ShowStatistics()
    // This was easier than making a bunch of functions to interface with the record
    // members of the prefs struct from ShowStatistics.

    // This function is going to compare current values with previously saved records, and if
    // the current values are greater, the corresponding member of prefs will be updated.
    // We will not write to INI here, because this code is going to be called a lot more often
    // than SaveStats()  - Khaos

    CString buffer;

    // Shared Files
    if ((UINT)theApp.sharedfiles->GetCount() > cumSharedMostFilesShared)
        cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
    uint64 bytesLargestFile = 0;
    uint64 allsize=theApp.sharedfiles->GetDatasize(bytesLargestFile);
    if (allsize>cumSharedLargestShareSize) cumSharedLargestShareSize = allsize;
    if (bytesLargestFile>cumSharedLargestFileSize) cumSharedLargestFileSize = bytesLargestFile;
    if (theApp.sharedfiles->GetCount() != 0)
    {
        uint64 tempint = allsize/theApp.sharedfiles->GetCount();
        if (tempint>cumSharedLargestAvgFileSize) cumSharedLargestAvgFileSize = tempint;
    }
} // SetRecordStructMembers()

void CPreferences::SaveCompletedDownloadsStat()
{

    // This function saves the values for the completed
    // download members to INI.  It is called from
    // CPartfile::PerformFileComplete ...   - Khaos

    CIni ini(GetMuleDirectory(EMULE_CONFIGDIR) + L"statistics.ini", L"Statistics");

    ini.WriteInt(L"DownCompletedFiles",			GetDownCompletedFiles());
    ini.WriteInt(L"DownSessionCompletedFiles",	GetDownSessionCompletedFiles());
} // SaveCompletedDownloadsStat()

void CPreferences::Add2SessionTransferData(UINT uClientID, UINT uClientPort, BOOL bFromPF,
        BOOL bUpDown, UINT bytes, bool sentToFriend)
{
    //	This function adds the transferred bytes to the appropriate variables,
    //	as well as to the totals for all clients. - Khaos
    //	PARAMETERS:
    //	uClientID - The identifier for which client software sent or received this data, eg SO_EMULE
    //	uClientPort - The remote port of the client that sent or received this data, eg 4662
    //	bFromPF - Applies only to uploads.  True is from partfile, False is from non-partfile.
    //	bUpDown - True is Up, False is Down
    //	bytes - Number of bytes sent by the client.  Subtract header before calling.

    switch (bUpDown)
    {
    case true:
        //	Upline Data
        switch (uClientID)
        {
            // Update session client breakdown stats for sent bytes...
        case SO_EMULE:
        case SO_OLDEMULE:
            sesUpData_EMULE+=bytes;
            break;
        case SO_EDONKEYHYBRID:
            sesUpData_EDONKEYHYBRID+=bytes;
            break;
        case SO_EDONKEY:
            sesUpData_EDONKEY+=bytes;
            break;
        case SO_MLDONKEY:
            sesUpData_MLDONKEY+=bytes;
            break;
        case SO_AMULE:
            sesUpData_AMULE+=bytes;
            break;
        case SO_SHAREAZA:
            sesUpData_SHAREAZA+=bytes;
            break;
//>>> WiZaRd::ClientAnalyzer
// Spike2 - Enhanced Client Recognition - START
        case SO_HYDRANODE:
        case SO_EMULEPLUS:
        case SO_TRUSTYFILES:
// Spike2 - Enhanced Client Recognition - END
//<<< WiZaRd::ClientAnalyzer
        case SO_CDONKEY:
        case SO_LPHANT:
        case SO_XMULE:
            sesUpData_EMULECOMPAT+=bytes;
            break;
        }

        switch (uClientPort)
        {
            // Update session port breakdown stats for sent bytes...
        case 4662:
            sesUpDataPort_4662+=bytes;
            break;
            //case (UINT)-2:		sesUpDataPort_URL+=bytes;		break;
        default:
            sesUpDataPort_OTHER+=bytes;
            break;
        }

        if (bFromPF)				sesUpData_Partfile+=bytes;
        else						sesUpData_File+=bytes;

        //	Add to our total for sent bytes...
        theApp.UpdateSentBytes(bytes, sentToFriend);

        break;

    case false:
        // Downline Data
        switch (uClientID)
        {
            // Update session client breakdown stats for received bytes...
        case SO_EMULE:
        case SO_OLDEMULE:
            sesDownData_EMULE+=bytes;
            break;
        case SO_EDONKEYHYBRID:
            sesDownData_EDONKEYHYBRID+=bytes;
            break;
        case SO_EDONKEY:
            sesDownData_EDONKEY+=bytes;
            break;
        case SO_MLDONKEY:
            sesDownData_MLDONKEY+=bytes;
            break;
        case SO_AMULE:
            sesDownData_AMULE+=bytes;
            break;
        case SO_SHAREAZA:
            sesDownData_SHAREAZA+=bytes;
            break;
//>>> WiZaRd::ClientAnalyzer
// Spike2 - Enhanced Client Recognition - START
        case SO_HYDRANODE:
        case SO_EMULEPLUS:
        case SO_TRUSTYFILES:
// Spike2 - Enhanced Client Recognition - END
//<<< WiZaRd::ClientAnalyzer
        case SO_CDONKEY:
        case SO_LPHANT:
        case SO_XMULE:
            sesDownData_EMULECOMPAT+=bytes;
            break;
        case SO_URL:
            sesDownData_URL+=bytes;
            break;
        }

        switch (uClientPort)
        {
            // Update session port breakdown stats for received bytes...
            // For now we are only going to break it down by default and non-default.
            // A statistical analysis of all data sent from every single port/domain is
            // beyond the scope of this add-on.
        case 4662:
            sesDownDataPort_4662+=bytes;
            break;
            //case (UINT)-2:		sesDownDataPort_URL+=bytes;		break;
        default:
            sesDownDataPort_OTHER+=bytes;
            break;
        }

        //	Add to our total for received bytes...
        theApp.UpdateReceivedBytes(bytes);
    }
}

// Reset Statistics by Khaos

void CPreferences::ResetCumulativeStatistics()
{

    // Save a backup so that we can undo this action
    SaveStats(1);

    // SET ALL CUMULATIVE STAT VALUES TO 0  :'-(

    totalDownloadedBytes=0;
    totalUploadedBytes=0;
    cumDownOverheadTotal=0;
    cumDownOverheadFileReq=0;
    cumDownOverheadSrcEx=0;
    cumDownOverheadKad=0;
    cumDownOverheadTotalPackets=0;
    cumDownOverheadFileReqPackets=0;
    cumDownOverheadSrcExPackets=0;
    cumDownOverheadKadPackets=0;
    cumUpOverheadTotal=0;
    cumUpOverheadFileReq=0;
    cumUpOverheadSrcEx=0;
    cumUpOverheadKad=0;
    cumUpOverheadTotalPackets=0;
    cumUpOverheadFileReqPackets=0;
    cumUpOverheadSrcExPackets=0;
    cumUpOverheadKadPackets=0;
    cumUpSuccessfulSessions=0;
    cumUpFailedSessions=0;
    cumUpAvgTime=0;
    cumUpData_EDONKEY=0;
    cumUpData_EDONKEYHYBRID=0;
    cumUpData_EMULE=0;
    cumUpData_KMULE = 0;
    cumUpData_MLDONKEY=0;
    cumUpData_AMULE=0;
    cumUpData_EMULECOMPAT=0;
    cumUpData_SHAREAZA=0;
    cumUpDataPort_4662=0;
    cumUpDataPort_OTHER=0;
    cumDownCompletedFiles=0;
    cumDownSuccessfulSessions=0;
    cumDownFailedSessions=0;
    cumDownAvgTime=0;
    cumLostFromCorruption=0;
    cumSavedFromCompression=0;
    cumPartsSavedByICH=0;
    cumDownData_EDONKEY=0;
    cumDownData_EDONKEYHYBRID=0;
    cumDownData_EMULE=0;
    cumDownData_KMULE = 0;
    cumDownData_MLDONKEY=0;
    cumDownData_AMULE=0;
    cumDownData_EMULECOMPAT=0;
    cumDownData_SHAREAZA=0;
    cumDownData_URL=0;
    cumDownDataPort_4662=0;
    cumDownDataPort_OTHER=0;
    cumConnAvgDownRate=0;
    cumConnMaxAvgDownRate=0;
    cumConnMaxDownRate=0;
    cumConnAvgUpRate=0;
    cumConnRunTime=0;
    cumConnAvgConnections=0;
    cumConnMaxConnLimitReached=0;
    cumConnPeakConnections=0;
    cumConnDownloadTime=0;
    cumConnUploadTime=0;
    cumConnTransferTime=0;
    cumConnMaxAvgUpRate=0;
    cumConnMaxUpRate=0;
    cumSharedMostFilesShared=0;
    cumSharedLargestShareSize=0;
    cumSharedLargestAvgFileSize=0;

    // Set the time of last reset...
    time_t timeNow;
    time(&timeNow);
    stat_datetimeLastReset = timeNow;

    // Save the reset stats
    SaveStats();
    theApp.emuledlg->statisticswnd->ShowStatistics(true);
}


// Load Statistics
// This used to be integrated in LoadPreferences, but it has been altered
// so that it can be used to load the backup created when the stats are reset.
// Last Modified: 2-22-03 by Khaos
bool CPreferences::LoadStats(int loadBackUp)
{
    // loadBackUp is 0 by default
    // loadBackUp = 0: Load the stats normally like we used to do in LoadPreferences
    // loadBackUp = 1: Load the stats from statbkup.ini and create a backup of the current stats.  Also, do not initialize session variables.
    CString sINI;
    CFileFind findBackUp;

    switch (loadBackUp)
    {
    case 0:
        // for transition...
        if (PathFileExists(GetMuleDirectory(EMULE_CONFIGDIR) + L"statistics.ini"))
            sINI.Format(L"%sstatistics.ini", GetMuleDirectory(EMULE_CONFIGDIR));
        else
            sINI.Format(L"%spreferences.ini", GetMuleDirectory(EMULE_CONFIGDIR));
        break;
    case 1:
        sINI.Format(L"%sstatbkup.ini", GetMuleDirectory(EMULE_CONFIGDIR));
        if (!findBackUp.FindFile(sINI))
            return false;
        SaveStats(2); // Save our temp backup of current values to statbkuptmp.ini, we will be renaming it at the end of this function.
        break;
    }

    BOOL fileex = PathFileExists(sINI);
    CIni ini(sINI, L"Statistics");

    totalDownloadedBytes			= ini.GetUInt64(L"TotalDownloadedBytes");
    totalUploadedBytes				= ini.GetUInt64(L"TotalUploadedBytes");

    // Load stats for cumulative downline overhead
    cumDownOverheadTotal			= ini.GetUInt64(L"DownOverheadTotal");
    cumDownOverheadFileReq			= ini.GetUInt64(L"DownOverheadFileReq");
    cumDownOverheadSrcEx			= ini.GetUInt64(L"DownOverheadSrcEx");
    cumDownOverheadKad				= ini.GetUInt64(L"DownOverheadKad");
    cumDownOverheadTotalPackets		= ini.GetUInt64(L"DownOverheadTotalPackets");
    cumDownOverheadFileReqPackets	= ini.GetUInt64(L"DownOverheadFileReqPackets");
    cumDownOverheadSrcExPackets		= ini.GetUInt64(L"DownOverheadSrcExPackets");
    cumDownOverheadKadPackets		= ini.GetUInt64(L"DownOverheadKadPackets");

    // Load stats for cumulative upline overhead
    cumUpOverheadTotal				= ini.GetUInt64(L"UpOverHeadTotal");
    cumUpOverheadFileReq			= ini.GetUInt64(L"UpOverheadFileReq");
    cumUpOverheadSrcEx				= ini.GetUInt64(L"UpOverheadSrcEx");
    cumUpOverheadKad				= ini.GetUInt64(L"UpOverheadKad");
    cumUpOverheadTotalPackets		= ini.GetUInt64(L"UpOverHeadTotalPackets");
    cumUpOverheadFileReqPackets		= ini.GetUInt64(L"UpOverheadFileReqPackets");
    cumUpOverheadSrcExPackets		= ini.GetUInt64(L"UpOverheadSrcExPackets");
    cumUpOverheadKadPackets			= ini.GetUInt64(L"UpOverheadKadPackets");

    // Load stats for cumulative upline data
    cumUpSuccessfulSessions			= ini.GetInt(L"UpSuccessfulSessions");
    cumUpFailedSessions				= ini.GetInt(L"UpFailedSessions");
    cumUpAvgTime					= ini.GetInt(L"UpAvgTime");

    // Load cumulative client breakdown stats for sent bytes
    cumUpData_EDONKEY				= ini.GetUInt64(L"UpData_EDONKEY");
    cumUpData_EDONKEYHYBRID			= ini.GetUInt64(L"UpData_EDONKEYHYBRID");
    cumUpData_EMULE					= ini.GetUInt64(L"UpData_EMULE");
    cumUpData_KMULE					= ini.GetUInt64(L"UpData_KMULE");
    cumUpData_MLDONKEY				= ini.GetUInt64(L"UpData_MLDONKEY");
    cumUpData_EMULECOMPAT			= ini.GetUInt64(L"UpData_LMULE");
    cumUpData_AMULE					= ini.GetUInt64(L"UpData_AMULE");
    cumUpData_SHAREAZA				= ini.GetUInt64(L"UpData_SHAREAZA");

    // Load cumulative port breakdown stats for sent bytes
    cumUpDataPort_4662				= ini.GetUInt64(L"UpDataPort_4662");
    cumUpDataPort_OTHER				= ini.GetUInt64(L"UpDataPort_OTHER");

    // Load cumulative source breakdown stats for sent bytes
    cumUpData_File					= ini.GetUInt64(L"UpData_File");
    cumUpData_Partfile				= ini.GetUInt64(L"UpData_Partfile");

    // Load stats for cumulative downline data
    cumDownCompletedFiles			= ini.GetInt(L"DownCompletedFiles");
    cumDownSuccessfulSessions		= ini.GetInt(L"DownSuccessfulSessions");
    cumDownFailedSessions			= ini.GetInt(L"DownFailedSessions");
    cumDownAvgTime					= ini.GetInt(L"DownAvgTime");

    // Cumulative statistics for saved due to compression/lost due to corruption
    cumLostFromCorruption			= ini.GetUInt64(L"LostFromCorruption");
    cumSavedFromCompression			= ini.GetUInt64(L"SavedFromCompression");
    cumPartsSavedByICH				= ini.GetInt(L"PartsSavedByICH");

    // Load cumulative client breakdown stats for received bytes
    cumDownData_EDONKEY				= ini.GetUInt64(L"DownData_EDONKEY");
    cumDownData_EDONKEYHYBRID		= ini.GetUInt64(L"DownData_EDONKEYHYBRID");
    cumDownData_EMULE				= ini.GetUInt64(L"DownData_EMULE");
    cumDownData_KMULE				= ini.GetUInt64(L"DownData_KMULE");
    cumDownData_MLDONKEY			= ini.GetUInt64(L"DownData_MLDONKEY");
    cumDownData_EMULECOMPAT			= ini.GetUInt64(L"DownData_LMULE");
    cumDownData_AMULE				= ini.GetUInt64(L"DownData_AMULE");
    cumDownData_SHAREAZA			= ini.GetUInt64(L"DownData_SHAREAZA");
    cumDownData_URL					= ini.GetUInt64(L"DownData_URL");

    // Load cumulative port breakdown stats for received bytes
    cumDownDataPort_4662			= ini.GetUInt64(L"DownDataPort_4662");
    cumDownDataPort_OTHER			= ini.GetUInt64(L"DownDataPort_OTHER");

    // Load stats for cumulative connection data
    cumConnAvgDownRate				= ini.GetFloat(L"ConnAvgDownRate");
    cumConnMaxAvgDownRate			= ini.GetFloat(L"ConnMaxAvgDownRate");
    cumConnMaxDownRate				= ini.GetFloat(L"ConnMaxDownRate");
    cumConnAvgUpRate				= ini.GetFloat(L"ConnAvgUpRate");
    cumConnMaxAvgUpRate				= ini.GetFloat(L"ConnMaxAvgUpRate");
    cumConnMaxUpRate				= ini.GetFloat(L"ConnMaxUpRate");
    cumConnRunTime					= ini.GetInt(L"ConnRunTime");
    cumConnTransferTime				= ini.GetInt(L"ConnTransferTime");
    cumConnDownloadTime				= ini.GetInt(L"ConnDownloadTime");
    cumConnUploadTime				= ini.GetInt(L"ConnUploadTime");
    cumConnAvgConnections			= ini.GetInt(L"ConnAvgConnections");
    cumConnMaxConnLimitReached		= ini.GetInt(L"ConnMaxConnLimitReached");
    cumConnPeakConnections			= ini.GetInt(L"ConnPeakConnections");

    // Load date/time of last reset
    stat_datetimeLastReset			= ini.GetInt(L"statsDateTimeLastReset");

    // Smart Load For Restores - Don't overwrite records that are greater than the backed up ones
    if (loadBackUp == 1)
    {
        // Load records for shared files
        if ((UINT)ini.GetInt(L"SharedMostFilesShared") > cumSharedMostFilesShared)
            cumSharedMostFilesShared =	ini.GetInt(L"SharedMostFilesShared");

        uint64 temp64 = ini.GetUInt64(L"SharedLargestShareSize");
        if (temp64 > cumSharedLargestShareSize)
            cumSharedLargestShareSize = temp64;

        temp64 = ini.GetUInt64(L"SharedLargestAvgFileSize");
        if (temp64 > cumSharedLargestAvgFileSize)
            cumSharedLargestAvgFileSize = temp64;

        temp64 = ini.GetUInt64(L"SharedLargestFileSize");
        if (temp64 > cumSharedLargestFileSize)
            cumSharedLargestFileSize = temp64;

        // Check to make sure the backup of the values we just overwrote exists.  If so, rename it to the backup file.
        // This allows us to undo a restore, so to speak, just in case we don't like the restored values...
        CString sINIBackUp;
        sINIBackUp.Format(L"%sstatbkuptmp.ini", GetMuleDirectory(EMULE_CONFIGDIR));
        if (findBackUp.FindFile(sINIBackUp))
        {
            ::DeleteFile(sINI);				// Remove the backup that we just restored from
            ::MoveFile(sINIBackUp, sINI);	// Rename our temporary backup to the normal statbkup.ini filename.
        }

        // Since we know this is a restore, now we should call ShowStatistics to update the data items to the new ones we just loaded.
        // Otherwise user is left waiting around for the tick counter to reach the next automatic update (Depending on setting in prefs)
        theApp.emuledlg->statisticswnd->ShowStatistics();
    }
    // Stupid Load -> Just load the values.
    else
    {
        // Load records for shared files
        cumSharedMostFilesShared	= ini.GetInt(L"SharedMostFilesShared");
        cumSharedLargestShareSize	= ini.GetUInt64(L"SharedLargestShareSize");
        cumSharedLargestAvgFileSize = ini.GetUInt64(L"SharedLargestAvgFileSize");
        cumSharedLargestFileSize	= ini.GetUInt64(L"SharedLargestFileSize");

        // Initialize new session statistic variables...
        sesDownCompletedFiles		= 0;

        sesUpData_EDONKEY			= 0;
        sesUpData_EDONKEYHYBRID		= 0;
        sesUpData_EMULE				= 0;
        sesUpData_KMULE				= 0;
        sesUpData_MLDONKEY			= 0;
        sesUpData_AMULE				= 0;
        sesUpData_EMULECOMPAT		= 0;
        sesUpData_SHAREAZA			= 0;
        sesUpDataPort_4662			= 0;
        sesUpDataPort_OTHER			= 0;

        sesDownData_EDONKEY			= 0;
        sesDownData_EDONKEYHYBRID	= 0;
        sesDownData_EMULE			= 0;
        sesDownData_KMULE			= 0;
        sesDownData_MLDONKEY		= 0;
        sesDownData_AMULE			= 0;
        sesDownData_EMULECOMPAT		= 0;
        sesDownData_SHAREAZA		= 0;
        sesDownData_URL				= 0;
        sesDownDataPort_4662		= 0;
        sesDownDataPort_OTHER		= 0;

        sesDownSuccessfulSessions	= 0;
        sesDownFailedSessions		= 0;
        sesPartsSavedByICH			= 0;
    }

    if (!fileex || (stat_datetimeLastReset==0 && totalDownloadedBytes==0 && totalUploadedBytes==0))
    {
        time_t timeNow;
        time(&timeNow);
        stat_datetimeLastReset = timeNow;
    }

    return true;
}

// This formats the UTC long value that is saved for stat_datetimeLastReset
// If this value is 0 (Never reset), then it returns Unknown.
CString CPreferences::GetStatsLastResetStr(bool formatLong)
{
    // formatLong dictates the format of the string returned.
    // For example...
    // true: DateTime format from the .ini
    // false: DateTime format from the .ini for the log
    CString	returnStr;
    if (GetStatsLastResetLng())
    {
        tm *statsReset;
        TCHAR szDateReset[128];
        time_t lastResetDateTime = (time_t) GetStatsLastResetLng();
        statsReset = localtime(&lastResetDateTime);
        if (statsReset)
        {
            _tcsftime(szDateReset, _countof(szDateReset), formatLong ? GetDateTimeFormat() : L"%c", statsReset);
            returnStr = szDateReset;
        }
    }
    if (returnStr.IsEmpty())
        returnStr = GetResString(IDS_UNKNOWN);
    return returnStr;
}

// <-----khaos-

bool CPreferences::Save()
{

    bool error = false;
    CString strFullPath;
    strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"preferences.dat";

    FILE* preffile = _tfsopen(strFullPath, L"wb", _SH_DENYWR);
    prefsExt->version = PREFFILE_VERSION;
    if (preffile)
    {
        prefsExt->version=PREFFILE_VERSION;
        prefsExt->EmuleWindowPlacement=EmuleWindowPlacement;
        md4cpy(prefsExt->userhash, userhash);

        error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile)!=1;
        if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning()))
        {
            fflush(preffile); // flush file stream buffers to disk buffers
            (void)_commit(_fileno(preffile)); // commit disk buffers to disk
        }
        fclose(preffile);
    }
    else
        error = true;

    SavePreferences();
    SaveStats();

//>>> WiZaRd::SharePermissions
    strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"shareddir_perm.dat";
//<<< WiZaRd::SharePermissions
    CStdioFile sdirfile;
    if (sdirfile.Open(strFullPath, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite | CFile::typeBinary))
    {
        try
        {
            // write Unicode byte-order mark 0xFEFF
            WORD wBOM = 0xFEFF;
            sdirfile.Write(&wBOM, sizeof(wBOM));

//>>> WiZaRd::SharePermissions
            for (POSITION pos = shareddir_list.GetHeadPosition(), pos2 = shareddir_list_permissions.GetHeadPosition(); pos != 0;)
//<<< WiZaRd::SharePermissions
            {
                sdirfile.WriteString(shareddir_list.GetNext(pos));
                sdirfile.Write(L"\r\n", sizeof(TCHAR)*2);
//>>> WiZaRd::SharePermissions
                CString tmp = L"";
                if (pos2 != NULL)
                    tmp.Format(L"%u", shareddir_list_permissions.GetNext(pos2));
                else
                    tmp = L"0";
                sdirfile.WriteString(tmp);
                sdirfile.Write(L"\r\n", sizeof(TCHAR)*2);
//<<< WiZaRd::SharePermissions
            }
            if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning()))
            {
                sdirfile.Flush(); // flush file stream buffers to disk buffers
                if (_commit(_fileno(sdirfile.m_pStream)) != 0) // commit disk buffers to disk
                    AfxThrowFileException(CFileException::hardIO, GetLastError(), sdirfile.GetFileName());
            }
            sdirfile.Close();
        }
        catch (CFileException* error)
        {
            TCHAR buffer[MAX_CFEXP_ERRORMSG];
            error->GetErrorMessage(buffer,_countof(buffer));
            if (thePrefs.GetVerbose())
                AddDebugLogLine(true, L"Failed to save %s - %s", strFullPath, buffer);
            error->Delete();
        }
    }
    else
        error = true;

    ::CreateDirectory(GetMuleDirectory(EMULE_INCOMINGDIR), 0);
    ::CreateDirectory(GetTempDir(), 0);
    return error;
}

void CPreferences::CreateUserHash()
{
    CryptoPP::AutoSeededRandomPool rng;
    rng.GenerateBlock(userhash, 16);
    // mark as emule client. that will be need in later version
    userhash[5] = 14;
    userhash[14] = 111;
}

int CPreferences::GetRecommendedMaxConnections()
{
    int iRealMax = ::GetMaxWindowsTCPConnections();
    if (iRealMax == -1 || iRealMax > 520)
        return 500;

    if (iRealMax < 20)
        return iRealMax;

    if (iRealMax <= 256)
        return iRealMax - 10;

    return iRealMax - 20;
}

void CPreferences::SavePreferences()
{
    CString buffer;

    CIni ini(GetConfigFile(), MOD_VERSION_PLAIN);
    //---
    ini.WriteString(L"AppVersion", theApp.m_strCurVersionLong);
    //---

#ifdef _DEBUG
    ini.WriteInt(L"DebugHeap", m_iDbgHeap);
#endif

    ini.WriteStringUTF8(L"Nick", strNick);
    ini.WriteString(L"IncomingDir", m_strIncomingDir);

    ini.WriteString(L"TempDir", tempdir.GetAt(0));

    CString tempdirs;
    for (int i=1; i<tempdir.GetCount(); i++)
    {
        tempdirs.Append(tempdir.GetAt(i));
        if (i+1<tempdir.GetCount())
            tempdirs.Append(L"|");
    }
    ini.WriteString(L"TempDirs", tempdirs);

    ini.WriteInt(L"MinUpload", minupload);
    ini.WriteInt(L"MaxUpload",maxupload);
    ini.WriteInt(L"MaxDownload",maxdownload);
    ini.WriteInt(L"MaxConnections",maxconnections);
    ini.WriteInt(L"MaxHalfConnections",maxhalfconnections);
    ini.WriteBool(L"ConditionalTCPAccept", m_bConditionalTCPAccept);
    ini.WriteInt(L"Port",port);
    ini.WriteInt(L"UDPPort",udpport);
    ini.WriteInt(L"MaxSourcesPerFile",maxsourceperfile);
    ini.WriteWORD(L"Language",m_wLanguageID);
    ini.WriteInt(L"SeeShare",m_iSeeShares);
    ini.WriteInt(L"StatGraphsInterval",trafficOMeterInterval);
    ini.WriteInt(L"StatsInterval",statsInterval);
    ini.WriteBool(L"StatsFillGraphs",m_bFillGraphs);
    ini.WriteInt(L"DownloadCapacity",maxGraphDownloadRate);
    ini.WriteInt(L"UploadCapacityNew",maxGraphUploadRate);
    ini.WriteInt(L"SplitterbarPosition",splitterbarPosition);
    ini.WriteInt(L"SplitterbarPositionStat",splitterbarPositionStat+1);
    ini.WriteInt(L"SplitterbarPositionStat_HL",splitterbarPositionStat_HL+1);
    ini.WriteInt(L"SplitterbarPositionStat_HR",splitterbarPositionStat_HR+1);
    ini.WriteInt(L"SplitterbarPositionFriend",splitterbarPositionFriend);
    ini.WriteInt(L"SplitterbarPositionShared",splitterbarPositionShared);
//>>> WiZaRd::Advanced Transfer Window Layout [Stulle]
//    ini.WriteInt(L"TransferWnd1",m_uTransferWnd1);
//    ini.WriteInt(L"TransferWnd2",m_uTransferWnd2);
//<<< WiZaRd::Advanced Transfer Window Layout [Stulle]
    ini.WriteInt(L"VariousStatisticsMaxValue",statsMax);
    ini.WriteInt(L"StatsAverageMinutes",statsAverageMinutes);
    ini.WriteInt(L"MaxConnectionsPerFiveSeconds",MaxConperFive);
    ini.WriteInt(L"Check4NewVersionDelay",versioncheckdays);

    ini.WriteBool(L"UpdateNotifyTestClient",updatenotify);
    if (IsRunningAeroGlassTheme())
        ini.WriteBool(L"MinToTray_Aero",mintotray);
    else
        ini.WriteBool(L"MinToTray",mintotray);
    ini.WriteBool(L"PreventStandby", m_bPreventStandby);
    ini.WriteBool(L"StoreSearches", m_bStoreSearches);
    ini.WriteBool(L"Splashscreen",splashscreen);
    ini.WriteBool(L"BringToFront",bringtoforeground);
    ini.WriteBool(L"TransferDoubleClick",transferDoubleclick);
    ini.WriteBool(L"ConfirmExit",confirmExit);
    ini.WriteBool(L"FilterBadIPs",filterLANIPs);
    ini.WriteBool(L"OnlineSignature",onlineSig);
    ini.WriteBool(L"StartupMinimized",startMinimized);
    ini.WriteBool(L"AutoStart",m_bAutoStart);
    ini.WriteInt(L"LastMainWndDlgID",m_iLastMainWndDlgID);
    ini.WriteBool(L"ShowRatesOnTitle",showRatesInTitle);
    ini.WriteBool(L"IndicateRatings",indicateratings);
    ini.WriteBool(L"WatchClipboard4ED2kFilelinks",watchclipboard);
    ini.WriteInt(L"SearchMethod",m_iSearchMethod);
    ini.WriteBool(L"SparsePartFiles",m_bSparsePartFiles);
    ini.WriteBool(L"ResolveSharedShellLinks",m_bResolveSharedShellLinks);
    ini.WriteString(L"YourHostname",m_strYourHostname);
    ini.WriteBool(L"CheckFileOpen",m_bCheckFileOpen);

    // Barry - New properties...
    ini.WriteBool(L"AutoTakeED2KLinks", autotakeed2klinks);
    ini.WriteBool(L"AddNewFilesPaused", addnewfilespaused);
    ini.WriteInt(L"3DDepth", depth3D);
    ini.WriteBool(L"MiniMule", m_bEnableMiniMule);

    ini.WriteString(L"TxtEditor",m_strTxtEditor);
    ini.WriteString(L"VideoPlayer",m_strVideoPlayer);
    ini.WriteString(L"VideoPlayerArgs",m_strVideoPlayerArgs);
    ini.WriteString(L"ExtractFolder",m_strExtractFolder);
    ini.WriteBool(L"ExtractToIncomingDir",m_bExtractToIncomingDir);
    ini.WriteBool(L"ExtractArchives",m_bExtractArchives);
    ini.WriteString(L"MessageFilter",messageFilter);
    ini.WriteString(L"CommentFilter",commentFilter);
    ini.WriteString(L"DateTimeFormat",GetDateTimeFormat());
    ini.WriteString(L"DateTimeFormat4Log",GetDateTimeFormat4Log());
    ini.WriteString(L"FilenameCleanups",filenameCleanups);
    ini.WriteInt(L"ExtractMetaData",m_iExtractMetaData);

    ini.DeleteKey(L"IRCAddTimestamp"); // delete old setting
    ini.WriteBool(L"AddTimestamp", m_bAddTimeStamp);

    ini.WriteBool(L"Verbose", m_bVerbose);
    ini.WriteBool(L"DebugSourceExchange", m_bDebugSourceExchange);	// do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogBannedClients", m_bLogBannedClients);			// do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogRatingDescReceived", m_bLogRatingDescReceived);// do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogAnalyzerEvents", m_bLogAnalyzerEvents); //>>> WiZaRd::ClientAnalyzer
    ini.WriteBool(L"LogSecureIdent", m_bLogSecureIdent);				// do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogFilteredIPs", m_bLogFilteredIPs);				// do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogFileSaving", m_bLogFileSaving);				// do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogA4AF", m_bLogA4AF);                           // do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogUlDlEvents", m_bLogUlDlEvents);
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    // following options are for debugging or when using an external debug device viewer only.
    ini.WriteInt(L"DebugClientTCP",m_iDebugClientTCPLevel);
    ini.WriteInt(L"DebugClientUDP",m_iDebugClientUDPLevel);
    ini.WriteInt(L"DebugClientKadUDP",m_iDebugClientKadUDPLevel);
#endif
    ini.WriteBool(L"PreviewPrio", m_bpreviewprio);
    ini.WriteBool(L"ShowOverhead", m_bshowoverhead);
    ini.WriteBool(L"VideoPreviewBackupped", moviePreviewBackup);
    ini.WriteInt(L"StartNextFile", m_istartnextfile);

    ini.DeleteKey(L"FileBufferSizePref"); // delete old 'file buff size' setting
    ini.WriteInt(L"FileBufferSize", m_iFileBufferSize);

    ini.DeleteKey(L"QueueSizePref"); // delete old 'queue size' setting
    ini.WriteInt(L"QueueSize", m_iQueueSize);

    ini.WriteInt(L"CommitFiles", m_iCommitFiles);
    ini.WriteBool(L"DAPPref", m_bDAP);
    ini.WriteBool(L"UAPPref", m_bUAP);
    ini.WriteBool(L"DisableKnownClientList",m_bDisableKnownClientList);
    ini.WriteBool(L"DisableQueueList",m_bDisableQueueList);
    ini.WriteBool(L"SaveLogToDisk",log2disk);
    ini.WriteBool(L"SaveAnalyzerLogToDisk", m_bLogAnalyzerToDisk); //>>> WiZaRd::ClientAnalyzer
    ini.WriteBool(L"SaveDebugToDisk",debug2disk);
    ini.WriteBool(L"MessagesFromFriendsOnly",msgonlyfriends);
    ini.WriteBool(L"ShowInfoOnCatTabs",showCatTabInfos);
    ini.WriteBool(L"AutoFilenameCleanup",autofilenamecleanup);
    ini.WriteBool(L"ShowExtControls",m_bExtControls);
    ini.WriteBool(L"UseAutocompletion",m_bUseAutocompl);
    ini.WriteBool(L"AutoClearCompleted",m_bRemoveFinishedDownloads);
    ini.WriteBool(L"TransflstRemainOrder",m_bTransflstRemain);
    ini.WriteBool(L"UseSimpleTimeRemainingcomputation",m_bUseOldTimeRemaining);
    ini.WriteBool(L"AllocateFullFile",m_bAllocFull);
    ini.WriteBool(L"ShowSharedFilesDetails", m_bShowSharedFilesDetails);
    ini.WriteBool(L"AutoShowLookups", m_bAutoShowLookups);

    ini.WriteInt(L"VersionCheckLastAutomatic", versioncheckLastAutomatic);
    ini.WriteInt(L"FilterLevel",filterlevel);

    ini.WriteBool(L"ShowDwlPercentage",m_bShowDwlPercentage);
    ini.WriteBool(L"RemoveFilesToBin",m_bRemove2bin);
    //ini.WriteBool(L"ShowCopyEd2kLinkCmd",m_bShowCopyEd2kLinkCmd);
    ini.WriteBool(L"AutoArchivePreviewStart", m_bAutomaticArcPreviewStart);

    // Toolbar
    ini.WriteString(L"ToolbarSetting", m_sToolbarSettings);
    ini.WriteString(L"ToolbarBitmap", m_sToolbarBitmap);
    ini.WriteString(L"ToolbarBitmapFolder", m_sToolbarBitmapFolder);
    ini.WriteInt(L"ToolbarLabels", m_nToolbarLabels);
    ini.WriteInt(L"ToolbarIconSize", m_sizToolbarIconSize.cx);
    ini.WriteString(L"SkinProfile", m_strSkinProfile);
    ini.WriteString(L"SkinProfileDir", m_strSkinProfileDir);

    ini.WriteBinary(L"HyperTextFont", (LPBYTE)&m_lfHyperText, sizeof m_lfHyperText);
    ini.WriteBinary(L"LogTextFont", (LPBYTE)&m_lfLogText, sizeof m_lfLogText);

    // ZZ:UploadSpeedSense -->
    ini.WriteBool(L"USSEnabled", m_bDynUpEnabled);
    ini.WriteBool(L"USSUseMillisecondPingTolerance", m_bDynUpUseMillisecondPingTolerance);
    ini.WriteInt(L"USSPingTolerance", m_iDynUpPingTolerance);
    ini.WriteInt(L"USSPingToleranceMilliseconds", m_iDynUpPingToleranceMilliseconds); // EastShare - Add by TAHO, USS limit
    ini.WriteInt(L"USSGoingUpDivider", m_iDynUpGoingUpDivider);
    ini.WriteInt(L"USSGoingDownDivider", m_iDynUpGoingDownDivider);
    ini.WriteInt(L"USSNumberOfPings", m_iDynUpNumberOfPings);
    // ZZ:UploadSpeedSense <--

    ini.WriteBool(L"A4AFSaveCpu", m_bA4AFSaveCpu); // ZZ:DownloadManager
    ini.WriteBool(L"HighresTimer", m_bHighresTimer);
    ini.WriteBool(L"RunAsUnprivilegedUser", m_bRunAsUser);
    ini.WriteBool(L"OpenPortsOnStartUp", m_bOpenPortsOnStartUp);
    ini.WriteInt(L"DebugLogLevel", m_byLogLevel);
    ini.WriteInt(L"WinXPSP2OrHigher", IsRunningXPSP2OrHigher());
    ini.WriteBool(L"RememberCancelledFiles", m_bRememberCancelledFiles);
    ini.WriteBool(L"RememberDownloadedFiles", m_bRememberDownloadedFiles);

    ini.WriteBool(L"WinaTransToolbar", m_bWinaTransToolbar);
    ini.WriteBool(L"ShowDownloadToolbar", m_bShowDownloadToolbar);

    ini.WriteBool(L"CryptLayerRequested", m_bCryptLayerRequested);
    ini.WriteBool(L"CryptLayerRequired", m_bCryptLayerRequired);
    ini.WriteInt(L"KadUDPKey", m_dwKadUDPKey);

    ///////////////////////////////////////////////////////////////////////////
    // Section: "Proxy"
    //
    ini.WriteBool(L"ProxyEnablePassword",proxy.EnablePassword,L"Proxy");
    ini.WriteBool(L"ProxyEnableProxy",proxy.UseProxy,L"Proxy");
    ini.WriteString(L"ProxyName",CStringW(proxy.name),L"Proxy");
    ini.WriteString(L"ProxyPassword",CStringW(proxy.password),L"Proxy");
    ini.WriteString(L"ProxyUser",CStringW(proxy.user),L"Proxy");
    ini.WriteInt(L"ProxyPort",proxy.port,L"Proxy");
    ini.WriteInt(L"ProxyType",proxy.type,L"Proxy");


    ///////////////////////////////////////////////////////////////////////////
    // Section: "Statistics"
    //
    ini.WriteInt(L"statsConnectionsGraphRatio", statsConnectionsGraphRatio,L"Statistics");
    ini.WriteString(L"statsExpandedTreeItems", m_strStatsExpandedTreeItems);
    CString buffer2;
    for (int i=0; i<15; i++)
    {
        buffer.Format(L"0x%06x",GetStatsColor(i));
        buffer2.Format(L"StatColor%i",i);
        ini.WriteString(buffer2,buffer,L"Statistics");
    }
    ini.WriteBool(L"HasCustomTaskIconColor", bHasCustomTaskIconColor, L"Statistics");

    ///////////////////////////////////////////////////////////////////////////
    // Section: "UPnP"
    //
    ini.WriteBool(L"EnableUPnP", m_bEnableUPnP, L"UPnP");
    ini.WriteBool(L"SkipWANIPSetup", m_bSkipWANIPSetup);
    ini.WriteBool(L"SkipWANPPPSetup", m_bSkipWANPPPSetup);
    ini.WriteBool(L"CloseUPnPOnExit", m_bCloseUPnPOnExit);
    ini.WriteInt(L"LastWorkingImplementation", m_nLastWorkingImpl);

    SavekMulePrefs();
}

//>>> WiZaRd::Own Prefs
void CPreferences::SavekMulePrefs()
{
    CString path = L"";
    path.Format(L"%s%s.ini", GetMuleDirectory(EMULE_CONFIGDIR), MOD_INI_FILE);
    CIni ini(path, MOD_VERSION_PLAIN);

//>>> WiZaRd::IPFilter-Update
    ini.WriteInt(L"IPFilterVersion", m_uiIPfilterVersion);
    ini.WriteBool(L"AutoUpdateIPFilter", m_bAutoUpdateIPFilter);
    ini.WriteString(L"UpdateURLIPFilter", m_strUpdateURLIPFilter);
//<<< WiZaRd::IPFilter-Update
//>>> WiZaRd::MediaInfoDLL Update
    ini.WriteBool(L"AutoUpdateMediaInfoDll", m_bMediaInfoDllAutoUpdate);
    ini.WriteString(L"UpdateURLMediaInfoDll", m_strMediaInfoDllUpdateURL);
    ini.WriteInt(L"MediaInfoDllVersion", m_uiMediaInfoDllVersion);
//<<< WiZaRd::MediaInfoDLL Update
//>>> PreviewIndicator [WiZaRd]
    ini.WriteInt(L"PreviewIndicatorMode", m_uiPreviewIndicatorMode);
    ini.WriteColRef(L"PreviewReadyColor", m_crPreviewReadyColor); //>>> jerrybg::ColorPreviewReadyFiles [WiZaRd]
//<<< PreviewIndicator [WiZaRd]
//>>> WiZaRd::Advanced Transfer Window Layout [Stulle]
    ini.WriteInt(L"TransferWnd1", m_uTransferWnd1);
    ini.WriteInt(L"TransferWnd2", m_uTransferWnd2);
    ini.WriteBool(L"SplitWindow", m_bSplitWindow);
//<<< WiZaRd::Advanced Transfer Window Layout [Stulle]
    ini.WriteBool(L"UseSimpleProgress", m_bUseSimpleProgress); //>>> WiZaRd::SimpleProgress
    ini.WriteInt(L"SpamFilterMode", m_uiSpamFilterMode); //>>> WiZaRd::AntiFake
//>>> WiZaRd::AutoHL
    ini.WriteInt(L"AutoHLUpdate", m_iAutoHLUpdateTimer);
    ini.WriteInt(L"UseAutoHL", m_iUseAutoHL);
    ini.WriteInt(L"MinAutoHL", m_iMinAutoHL);
    ini.WriteInt(L"MaxAutoHL", m_iMaxAutoHL);
    ini.WriteInt(L"MaxSourcesHL", m_iMaxSourcesHL);
//<<< WiZaRd::AutoHL
//>>> WiZaRd::Remove forbidden files
    ini.WriteBool(L"RemoveForbiddenFiles", m_bRemoveForbiddenFiles);
//<<< WiZaRd::Remove forbidden files
//>>> WiZaRd::Drop Blocking Sockets [Xman?]
    ini.WriteBool(L"DropBlockingSockets", m_bDropBlockingSockets);
    ini.WriteFloat(L"SocketBlockRate", m_fMaxBlockRate);
    ini.WriteFloat(L"SocketBlockRate20", m_fMaxBlockRate20);
//<<< WiZaRd::Drop Blocking Sockets [Xman?]
//>>> WiZaRd::ModIconDLL Update
    ini.WriteBool(L"AutoUpdateModIconDll", m_bModIconDllAutoUpdate);
    ini.WriteString(L"UpdateURLModIconDll", m_strModIconDllUpdateURL);
//<<< WiZaRd::ModIconDLL Update
    ini.WriteInt(L"SpreadPrioLimit",m_iSpreadPrioLimit); //>>> Tux::Spread Priority v3
}
//<<< WiZaRd::Own Prefs

void CPreferences::ResetStatsColor(int index)
{
    switch (index)
    {
    case  0:
        m_adwStatsColors[ 0]=RGB(0,  0, 0);
        break;
    case  1:
        m_adwStatsColors[ 1]=RGB(255,255,255);
        break;
    case  2:
        m_adwStatsColors[ 2]=RGB(128,255,128);
        break;
    case  3:
        m_adwStatsColors[ 3]=RGB(0,210,  0);
        break;
    case  4:
        m_adwStatsColors[ 4]=RGB(0,128,  0);
        break;
    case  5:
        m_adwStatsColors[ 5]=RGB(255,128,128);
        break;
    case  6:
        m_adwStatsColors[ 6]=RGB(200,  0,  0);
        break;
    case  7:
        m_adwStatsColors[ 7]=RGB(140,  0,  0);
        break;
    case  8:
        m_adwStatsColors[ 8]=RGB(150,150,255);
        break;
    case  9:
        m_adwStatsColors[ 9]=RGB(192,  0,192);
        break;
    case 10:
        m_adwStatsColors[10]=RGB(255,255,128);
        break;
    case 11:
        m_adwStatsColors[11]=RGB(0,  0,  0);
        bHasCustomTaskIconColor = false;
        break;
    case 12:
        m_adwStatsColors[12]=RGB(255,255,255);
        break;
    case 13:
        m_adwStatsColors[13]=RGB(255,255,255);
        break;
    case 14:
        m_adwStatsColors[14]=RGB(255,190,190);
        break;
    }
}

void CPreferences::GetAllStatsColors(int iCount, LPDWORD pdwColors)
{
    memset(pdwColors, 0, sizeof(*pdwColors) * iCount);
    memcpy(pdwColors, m_adwStatsColors, sizeof(*pdwColors) * min(_countof(m_adwStatsColors), iCount));
}

bool CPreferences::SetAllStatsColors(int iCount, const DWORD* pdwColors)
{
    bool bModified = false;
    int iMin = min(_countof(m_adwStatsColors), iCount);
    for (int i = 0; i < iMin; i++)
    {
        if (m_adwStatsColors[i] != pdwColors[i])
        {
            m_adwStatsColors[i] = pdwColors[i];
            bModified = true;
            if (i == 11)
                bHasCustomTaskIconColor = true;
        }
    }
    return bModified;
}

void CPreferences::IniCopy(CString si, CString di)
{
    CIni ini(GetConfigFile(), MOD_VERSION_PLAIN);
    CString s = ini.GetString(si);
    // Do NOT write empty settings, this will mess up reading of default settings in case
    // there were no settings (fresh emule install) at all available!
    if (!s.IsEmpty())
    {
        ini.SetSection(L"ListControlSetup");
        ini.WriteString(di,s);
    }
}

void CPreferences::LoadPreferences()
{
    CIni ini(GetConfigFile(), MOD_VERSION_PLAIN);
    ini.SetSection(MOD_VERSION_PLAIN);

    CString strCurrVersion, strPrefsVersion;

    strCurrVersion = theApp.m_strCurVersionLong;
    strPrefsVersion = ini.GetString(L"AppVersion");

    m_bFirstStart = false;
    m_bUpdate = false;

    if (strPrefsVersion.IsEmpty())
        m_bFirstStart = true;
    // Update/Downgrade?
    else if (strPrefsVersion != theApp.m_strCurVersionLong)
        m_bUpdate = true;

#ifdef _DEBUG
    m_iDbgHeap = ini.GetInt(L"DebugHeap", 1);
#else
    m_iDbgHeap = 0;
#endif

    updatenotify=ini.GetBool(L"UpdateNotifyTestClient",true);

    SetUserNick(ini.GetStringUTF8(L"Nick", DEFAULT_NICK));
    if (strNick.IsEmpty())
        SetUserNick(DEFAULT_NICK);

    m_strIncomingDir = ini.GetString(L"IncomingDir", L"");
    if (m_strIncomingDir.IsEmpty()) // We want GetDefaultDirectory to also create the folder, so we have to know if we use the default or not
        m_strIncomingDir = GetDefaultDirectory(EMULE_INCOMINGDIR, true);
    MakeFoldername(m_strIncomingDir);

    // load tempdir(s) setting
    CString tempdirs;
    tempdirs = ini.GetString(L"TempDir", L"");
    if (tempdirs.IsEmpty()) // We want GetDefaultDirectory to also create the folder, so we have to know if we use the default or not
        tempdirs = GetDefaultDirectory(EMULE_TEMPDIR, true);
    tempdirs += L"|" + ini.GetString(L"TempDirs");

    int curPos=0;
    bool doubled;
    CString atmp=tempdirs.Tokenize(L"|", curPos);
    while (!atmp.IsEmpty())
    {
        atmp.Trim();
        if (!atmp.IsEmpty())
        {
            MakeFoldername(atmp);
            doubled=false;
            for (int i=0; i<tempdir.GetCount(); i++)	// avoid double tempdirs
                if (atmp.CompareNoCase(GetTempDir(i))==0)
                {
                    doubled=true;
                    break;
                }
            if (!doubled)
            {
                if (PathFileExists(atmp)==FALSE)
                {
                    CreateDirectory(atmp,NULL);
                    if (PathFileExists(atmp)==TRUE || tempdir.GetCount()==0)
                        tempdir.Add(atmp);
                }
                else
                    tempdir.Add(atmp);
            }
        }
        atmp = tempdirs.Tokenize(L"|", curPos);
    }

    maxGraphDownloadRate=ini.GetInt(L"DownloadCapacity",96);
    if (maxGraphDownloadRate==0)
        maxGraphDownloadRate=96;

    maxGraphUploadRate = ini.GetInt(L"UploadCapacityNew",-1);
    if (maxGraphUploadRate == 0)
        maxGraphUploadRate = UNLIMITED;
    else if (maxGraphUploadRate == -1)
    {
        // converting value from prior versions
        int nOldUploadCapacity = ini.GetInt(L"UploadCapacity", 16);
        if (nOldUploadCapacity == 16 && ini.GetInt(L"MaxUpload",12) == 12)
        {
            // either this is a complete new install, or the prior version used the default value
            // in both cases, set the new default values to unlimited
            maxGraphUploadRate = UNLIMITED;
            ini.WriteInt(L"MaxUpload",UNLIMITED, MOD_VERSION_PLAIN);
        }
        else
            maxGraphUploadRate = nOldUploadCapacity; // use old custoum value
    }

    minupload=(uint16)ini.GetInt(L"MinUpload", 1);
    if (minupload < 1)
        minupload = 1;
    maxupload=(uint16)ini.GetInt(L"MaxUpload",UNLIMITED);
    if (maxupload > maxGraphUploadRate && maxupload != UNLIMITED)
        maxupload = (uint16)(maxGraphUploadRate * .8);

    maxdownload=(uint16)ini.GetInt(L"MaxDownload", UNLIMITED);
    if (maxdownload > maxGraphDownloadRate && maxdownload != UNLIMITED)
        maxdownload = (uint16)(maxGraphDownloadRate * .8);
    maxconnections=ini.GetInt(L"MaxConnections",GetRecommendedMaxConnections());
    maxhalfconnections=ini.GetInt(L"MaxHalfConnections",9);
    m_bConditionalTCPAccept = ini.GetBool(L"ConditionalTCPAccept", false);

    // reset max halfopen to a default if OS changed to SP2 (or higher) or away
    int dwSP2OrHigher = ini.GetInt(L"WinXPSP2OrHigher", -1);
    int dwCurSP2OrHigher = IsRunningXPSP2OrHigher();
    if (dwSP2OrHigher != dwCurSP2OrHigher)
    {
        if (dwCurSP2OrHigher == 0)
            maxhalfconnections = 50;
        else if (dwCurSP2OrHigher == 1)
            maxhalfconnections = 9;
    }

    m_strBindAddrW = ini.GetString(L"BindAddr");
    m_strBindAddrW.Trim();
    m_pszBindAddrW = m_strBindAddrW.IsEmpty() ? NULL : (LPCWSTR)m_strBindAddrW;
    m_strBindAddrA = m_strBindAddrW;
    m_pszBindAddrA = m_strBindAddrA.IsEmpty() ? NULL : (LPCSTR)m_strBindAddrA;

    // WiZaRd:
    // It seems the "port in use" check is a bit buggy once in a while... that's why we will not check and set a different port
    // if either the user did an update installation (i.e. it worked in the past) or has disabled UPnP (i.e. it knows what its doing)
    int iPort = ini.GetInt(L"Port", 0);
    if (iPort == INT_MAX || iPort == 0 || (!m_bUpdate && !m_bEnableUPnP && IsTCPPortInUse((uint16)iPort)))
        port = GetRandomTCPPort();
    else
        port = (uint16)iPort;

    iPort = ini.GetInt(L"UDPPort", INT_MAX/*invalid port value*/);
    if (iPort == INT_MAX || iPort == 0 || (!m_bUpdate && !m_bEnableUPnP && IsUDPPortInUse((uint16)iPort))) // don't allow to disable UDP!
        udpport = GetRandomUDPPort();
    else
        udpport = (uint16)iPort;

    maxsourceperfile=ini.GetInt(L"MaxSourcesPerFile",400);
    m_wLanguageID=ini.GetWORD(L"Language",0);
    m_iSeeShares=(EViewSharedFilesAccess)ini.GetInt(L"SeeShare",vsfaNobody);
    trafficOMeterInterval=ini.GetInt(L"StatGraphsInterval",3);
    statsInterval=ini.GetInt(L"statsInterval",5);
    m_bFillGraphs=ini.GetBool(L"StatsFillGraphs");
    dontcompressavi=ini.GetBool(L"DontCompressAvi",false);

    splitterbarPosition=ini.GetInt(L"SplitterbarPosition",75);
    if (splitterbarPosition < 9)
        splitterbarPosition = 9;
    else if (splitterbarPosition > 93)
        splitterbarPosition = 93;
    splitterbarPositionStat=ini.GetInt(L"SplitterbarPositionStat",30);
    splitterbarPositionStat_HL=ini.GetInt(L"SplitterbarPositionStat_HL",66);
    splitterbarPositionStat_HR=ini.GetInt(L"SplitterbarPositionStat_HR",33);
    if (splitterbarPositionStat_HR+1>=splitterbarPositionStat_HL)
    {
        splitterbarPositionStat_HL = 66;
        splitterbarPositionStat_HR = 33;
    }
    splitterbarPositionFriend=ini.GetInt(L"SplitterbarPositionFriend",170);
    splitterbarPositionShared=ini.GetInt(L"SplitterbarPositionShared",179);

//>>> WiZaRd::Advanced Transfer Window Layout [Stulle]
//    m_uTransferWnd1 = ini.GetInt(L"TransferWnd1",0);
//    m_uTransferWnd2 = ini.GetInt(L"TransferWnd2",1);
//<<< WiZaRd::Advanced Transfer Window Layout [Stulle]

    statsMax=ini.GetInt(L"VariousStatisticsMaxValue",100);
    statsAverageMinutes=ini.GetInt(L"StatsAverageMinutes",5);
    MaxConperFive=ini.GetInt(L"MaxConnectionsPerFiveSeconds",GetDefaultMaxConperFive());

    // since the minimize to tray button is not working under Aero (at least not at this point),
    // we enable map the minimize to tray on the minimize button by default if Aero is running
    if (IsRunningAeroGlassTheme())
        mintotray=ini.GetBool(L"MinToTray_Aero", true);
    else
        mintotray=ini.GetBool(L"MinToTray", true);

    m_bPreventStandby = ini.GetBool(L"PreventStandby", false);
    m_bStoreSearches = ini.GetBool(L"StoreSearches", true);
    splashscreen=ini.GetBool(L"Splashscreen",true);
    bringtoforeground=ini.GetBool(L"BringToFront",true);
    transferDoubleclick=ini.GetBool(L"TransferDoubleClick",true);
    beepOnError=ini.GetBool(L"BeepOnError",true);
    confirmExit=ini.GetBool(L"ConfirmExit",true);
    filterLANIPs=ini.GetBool(L"FilterBadIPs",true);
    m_bAllocLocalHostIP=ini.GetBool(L"AllowLocalHostIP",false);
    showRatesInTitle=ini.GetBool(L"ShowRatesOnTitle",true);
    m_bIconflashOnNewMessage=ini.GetBool(L"IconflashOnNewMessage",true);

    onlineSig=ini.GetBool(L"OnlineSignature",false);
    startMinimized=ini.GetBool(L"StartupMinimized",false);
    m_bAutoStart=ini.GetBool(L"AutoStart",false);
    m_bRestoreLastMainWndDlg=ini.GetBool(L"RestoreLastMainWndDlg",false);
    m_iLastMainWndDlgID=ini.GetInt(L"LastMainWndDlgID",0);

    m_bTransflstRemain =ini.GetBool(L"TransflstRemainOrder",false);
    filterlevel=ini.GetInt(L"FilterLevel",127);
    m_bSparsePartFiles=ini.GetBool(L"SparsePartFiles",false);  // http://support.microsoft.com/kb/967351
    m_bResolveSharedShellLinks=ini.GetBool(L"ResolveSharedShellLinks",false);
    m_bKeepUnavailableFixedSharedDirs = ini.GetBool(L"KeepUnavailableFixedSharedDirs", false);
    m_strYourHostname=ini.GetString(L"YourHostname", L"");

    // Barry - New properties...
    autotakeed2klinks = ini.GetBool(L"AutoTakeED2KLinks",true);
    addnewfilespaused = ini.GetBool(L"AddNewFilesPaused",false);
    depth3D = ini.GetInt(L"3DDepth", 5);
    m_bEnableMiniMule = ini.GetBool(L"MiniMule", true);

    m_strDateTimeFormat = ini.GetString(L"DateTimeFormat", L"%A, %c");
    m_strDateTimeFormat4Log = ini.GetString(L"DateTimeFormat4Log", L"%c");
    m_strDateTimeFormat4Lists = ini.GetString(L"DateTimeFormat4Lists", L"%c");

    m_bAddTimeStamp = ini.GetBool(L"IRCAddTimestamp", true);
    m_bAddTimeStamp = ini.GetBool(L"AddTimestamp", m_bAddTimeStamp);

    log2disk = ini.GetBool(L"SaveLogToDisk", false);
    m_bLogAnalyzerToDisk = ini.GetBool(L"SaveAnalyzerLogToDisk", false); //>>> WiZaRd::ClientAnalyzer
    uMaxLogFileSize = ini.GetInt(L"MaxLogFileSize", 1024*1024);
    iMaxLogBuff = ini.GetInt(L"MaxLogBuff",64) * 1024;
    m_iLogFileFormat = (ELogFileFormat)ini.GetInt(L"LogFileFormat", Unicode);
    m_bEnableVerboseOptions=ini.GetBool(L"VerboseOptions", true);
    m_bLogAnalyzerEvents = ini.GetBool(L"LogAnalyzerEvents", true); //>>> WiZaRd::ClientAnalyzer
    if (m_bEnableVerboseOptions)
    {
#ifdef _DEBUG
        m_bVerbose = ini.GetBool(L"Verbose", true);
#else
        m_bVerbose=ini.GetBool(L"Verbose",false);
#endif
        m_bFullVerbose=ini.GetBool(L"FullVerbose",false);
        debug2disk=ini.GetBool(L"SaveDebugToDisk", false);
        m_bDebugSourceExchange=ini.GetBool(L"DebugSourceExchange",false);
        m_bLogBannedClients=ini.GetBool(L"LogBannedClients", true);
        m_bLogRatingDescReceived=ini.GetBool(L"LogRatingDescReceived",true);
        m_bLogSecureIdent=ini.GetBool(L"LogSecureIdent",true);
        m_bLogFilteredIPs=ini.GetBool(L"LogFilteredIPs",true);
        m_bLogFileSaving=ini.GetBool(L"LogFileSaving",false);
        m_bLogA4AF=ini.GetBool(L"LogA4AF",false); // ZZ:DownloadManager
        m_bLogUlDlEvents=ini.GetBool(L"LogUlDlEvents",true);
    }

#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    // following options are for debugging or when using an external debug device viewer only.
    m_iDebugClientTCPLevel = ini.GetInt(L"DebugClientTCP", 0);
    m_iDebugClientUDPLevel = ini.GetInt(L"DebugClientUDP", 0);
    m_iDebugClientKadUDPLevel = ini.GetInt(L"DebugClientKadUDP", 0);
    m_iDebugSearchResultDetailLevel = ini.GetInt(L"DebugSearchResultDetailLevel", 0);
    /*
    #else
        // for normal release builds ensure that those options are all turned off
        m_iDebugClientTCPLevel = 0;
        m_iDebugClientUDPLevel = 0;
        m_iDebugClientKadUDPLevel = 0;
        m_iDebugSearchResultDetailLevel = 0;
    */
#endif

    m_bpreviewprio=ini.GetBool(L"PreviewPrio",false);
    m_bupdatequeuelist=ini.GetBool(L"UpdateQueueListPref",false);
    m_istartnextfile=ini.GetInt(L"StartNextFile",0);
    m_bshowoverhead=ini.GetBool(L"ShowOverhead",false);
    moviePreviewBackup=ini.GetBool(L"VideoPreviewBackupped",false);
    m_iPreviewSmallBlocks=ini.GetInt(L"PreviewSmallBlocks", 0);
    m_bPreviewCopiedArchives=ini.GetBool(L"PreviewCopiedArchives", true);
    m_iInspectAllFileTypes=ini.GetInt(L"InspectAllFileTypes", 0);
    m_bAllocFull=ini.GetBool(L"AllocateFullFile",0);
    m_bAutomaticArcPreviewStart=ini.GetBool(L"AutoArchivePreviewStart", true);
    m_bShowSharedFilesDetails = ini.GetBool(L"ShowSharedFilesDetails", true);
    m_bAutoShowLookups = ini.GetBool(L"AutoShowLookups", true);
    m_bShowUpDownIconInTaskbar = ini.GetBool(L"ShowUpDownIconInTaskbar", true);
    m_bForceSpeedsToKB = ini.GetBool(L"ForceSpeedsToKB", false);
    m_bExtraPreviewWithMenu = ini.GetBool(L"ExtraPreviewWithMenu", false);

    // read file buffer size (with backward compatibility)
    m_iFileBufferSize=ini.GetInt(L"FileBufferSizePref",0); // old setting
    if (m_iFileBufferSize == 0)
        m_iFileBufferSize = 10*1024*1024;
    else
        m_iFileBufferSize = ((m_iFileBufferSize*15000 + 512)/1024)*1024;
    m_iFileBufferSize=ini.GetInt(L"FileBufferSize",m_iFileBufferSize);
//>>> WiZaRd::IntelliFlush
    //was 1min. - changed to 15min. default because with IntelliFlush, we basically don't care
    m_uFileBufferTimeLimit = SEC2MS(ini.GetInt(L"FileBufferTimeLimit", 915));
//<<< WiZaRd::IntelliFlush

    // read queue size (with backward compatibility)
    m_iQueueSize=ini.GetInt(L"QueueSizePref",0); // old setting
    if (m_iQueueSize == 0)
        m_iQueueSize = 50*100;
    else
        m_iQueueSize = m_iQueueSize*100;
    m_iQueueSize=ini.GetInt(L"QueueSize",m_iQueueSize);

    m_iCommitFiles=ini.GetInt(L"CommitFiles", 2); // 1 = "commit" on application shut down; 2 = "commit" on each file saveing
    versioncheckdays=ini.GetInt(L"Check4NewVersionDelay",5);
    m_bDAP=ini.GetBool(L"DAPPref",true);
    m_bUAP=ini.GetBool(L"UAPPref",true);
    m_bPreviewOnIconDblClk=ini.GetBool(L"PreviewOnIconDblClk",false);
    m_bCheckFileOpen=ini.GetBool(L"CheckFileOpen",true);
    indicateratings=ini.GetBool(L"IndicateRatings",true);
    watchclipboard=ini.GetBool(L"WatchClipboard4ED2kFilelinks",false);
    m_iSearchMethod=ini.GetInt(L"SearchMethod",0);

    showCatTabInfos=ini.GetBool(L"ShowInfoOnCatTabs",false);
//	resumeSameCat=ini.GetBool(L"ResumeNextFromSameCat",false);
    dontRecreateGraphs =ini.GetBool(L"DontRecreateStatGraphsOnResize",false);
    m_bExtControls =ini.GetBool(L"ShowExtControls",false);

    versioncheckLastAutomatic=ini.GetInt(L"VersionCheckLastAutomatic",0);
    m_bDisableKnownClientList=ini.GetBool(L"DisableKnownClientList",false);
    m_bDisableQueueList=ini.GetBool(L"DisableQueueList",false);
    msgonlyfriends=ini.GetBool(L"MessagesFromFriendsOnly",false);
    msgsecure=ini.GetBool(L"MessageFromValidSourcesOnly",true);
    autofilenamecleanup=ini.GetBool(L"AutoFilenameCleanup",false);
    m_bUseAutocompl=ini.GetBool(L"UseAutocompletion",true);
    m_bShowDwlPercentage=ini.GetBool(L"ShowDwlPercentage",true);
    m_bRemove2bin=ini.GetBool(L"RemoveFilesToBin",true);
    m_bShowCopyEd2kLinkCmd=ini.GetBool(L"ShowCopyEd2kLinkCmd",false);

    m_iMaxChatHistory=ini.GetInt(L"MaxChatHistoryLines",100);
    if (m_iMaxChatHistory < 1)
        m_iMaxChatHistory = 100;
    maxmsgsessions=ini.GetInt(L"MaxMessageSessions",50);
    m_bShowActiveDownloadsBold = ini.GetBool(L"ShowActiveDownloadsBold", false);

    m_strTxtEditor = ini.GetString(L"TxtEditor", L"notepad.exe");
    m_strVideoPlayer = ini.GetString(L"VideoPlayer", L"");
    m_strVideoPlayerArgs = ini.GetString(L"VideoPlayerArgs",L"");

    m_strExtractFolder = ini.GetString(L"ExtractFolder",L"");
    m_bExtractToIncomingDir = ini.GetBool(L"ExtractToIncomingDir",true);
    m_bExtractArchives = ini.GetBool(L"ExtractArchives",true);

//>>> WiZaRd::Promote VLC
    if (m_strVideoPlayer.IsEmpty() || !::PathFileExists(m_strVideoPlayer))
    {
        m_strVideoPlayer = GetRegVLCDir();
        if (m_strVideoPlayer.IsEmpty())
            m_strVideoPlayer = L"http://www.videolan.org";
        else
            moviePreviewBackup = false; // using VLC - no backup necessary
    }
//<<< WiZaRd::Promote VLC

    messageFilter=ini.GetStringLong(L"MessageFilter",L"fastest download speed|fastest eMule");
    commentFilter = ini.GetStringLong(L"CommentFilter",L"http://|https://|ftp://|www.|ftp.");
    commentFilter.MakeLower();
    filenameCleanups=ini.GetStringLong(L"FilenameCleanups",L"http|www.|.com|.de|.org|.net|shared|powered|sponsored|sharelive|filedonkey|");
    m_iExtractMetaData = ini.GetInt(L"ExtractMetaData", 1); // 0=disable, 1=mp3, 2=MediaDet
    if (m_iExtractMetaData > 1)
        m_iExtractMetaData = 1;
    m_bAdjustNTFSDaylightFileTime=ini.GetBool(L"AdjustNTFSDaylightFileTime", true);
    m_bRearrangeKadSearchKeywords = ini.GetBool(L"RearrangeKadSearchKeywords", true);

    m_bRemoveFinishedDownloads=ini.GetBool(L"AutoClearCompleted",false);
    m_bUseOldTimeRemaining= ini.GetBool(L"UseSimpleTimeRemainingcomputation",false);

    // Toolbar
    m_sToolbarSettings = ini.GetString(L"ToolbarSetting", strDefaultToolbar);
    m_sToolbarBitmap = ini.GetString(L"ToolbarBitmap", L"");
    m_sToolbarBitmapFolder = ini.GetString(L"ToolbarBitmapFolder", L"");
    if (m_sToolbarBitmapFolder.IsEmpty()) // We want GetDefaultDirectory to also create the folder, so we have to know if we use the default or not
        m_sToolbarBitmapFolder = GetDefaultDirectory(EMULE_TOOLBARDIR, true);
    m_nToolbarLabels = (EToolbarLabelType)ini.GetInt(L"ToolbarLabels", CMuleToolbarCtrl::GetDefaultLabelType());
    m_bReBarToolbar = ini.GetBool(L"ReBarToolbar", 1);
    m_sizToolbarIconSize.cx = m_sizToolbarIconSize.cy = ini.GetInt(L"ToolbarIconSize", 32);
    m_iStraightWindowStyles=ini.GetInt(L"StraightWindowStyles",0);
    m_bUseSystemFontForMainControls=ini.GetBool(L"UseSystemFontForMainControls",0);
    m_bRTLWindowsLayout = ini.GetBool(L"RTLWindowsLayout");
    m_strSkinProfile = ini.GetString(L"SkinProfile", L"");
    m_strSkinProfileDir = ini.GetString(L"SkinProfileDir", L"");
    if (m_strSkinProfileDir.IsEmpty()) // We want GetDefaultDirectory to also create the folder, so we have to know if we use the default or not
        m_strSkinProfileDir = GetDefaultDirectory(EMULE_SKINDIR, true);


    LPBYTE pData = NULL;
    UINT uSize = sizeof m_lfHyperText;
    if (ini.GetBinary(L"HyperTextFont", &pData, &uSize) && uSize == sizeof m_lfHyperText)
        memcpy(&m_lfHyperText, pData, sizeof m_lfHyperText);
    else
        memset(&m_lfHyperText, 0, sizeof m_lfHyperText);
    delete[] pData;

    pData = NULL;
    uSize = sizeof m_lfLogText;
    if (ini.GetBinary(L"LogTextFont", &pData, &uSize) && uSize == sizeof m_lfLogText)
        memcpy(&m_lfLogText, pData, sizeof m_lfLogText);
    else
        memset(&m_lfLogText, 0, sizeof m_lfLogText);
    delete[] pData;

    m_crLogError = ini.GetColRef(L"LogErrorColor", m_crLogError);
    m_crLogWarning = ini.GetColRef(L"LogWarningColor", m_crLogWarning);
    m_crLogSuccess = ini.GetColRef(L"LogSuccessColor", m_crLogSuccess);

    if (statsAverageMinutes < 1)
        statsAverageMinutes = 5;

    // ZZ:UploadSpeedSense -->
    m_bDynUpEnabled = ini.GetBool(L"USSEnabled", true);
    m_bDynUpUseMillisecondPingTolerance = ini.GetBool(L"USSUseMillisecondPingTolerance", false);
    m_iDynUpPingTolerance = ini.GetInt(L"USSPingTolerance", 500);
    m_iDynUpPingToleranceMilliseconds = ini.GetInt(L"USSPingToleranceMilliseconds", 200);
    m_iDynUpGoingUpDivider = ini.GetInt(L"USSGoingUpDivider", 1000);
    m_iDynUpGoingDownDivider = ini.GetInt(L"USSGoingDownDivider", 1000);
    m_iDynUpNumberOfPings = ini.GetInt(L"USSNumberOfPings", 1);
    // ZZ:UploadSpeedSense <--

    m_bA4AFSaveCpu = ini.GetBool(L"A4AFSaveCpu", false); // ZZ:DownloadManager
    m_bHighresTimer = ini.GetBool(L"HighresTimer", true);
    m_bRunAsUser = ini.GetBool(L"RunAsUnprivilegedUser", false);
    m_bPreferRestrictedOverUser = ini.GetBool(L"PreferRestrictedOverUser", false);
    m_bOpenPortsOnStartUp = ini.GetBool(L"OpenPortsOnStartUp", false);
    m_byLogLevel = ini.GetInt(L"DebugLogLevel", DLP_VERYLOW);
    m_bTrustEveryHash = ini.GetBool(L"AICHTrustEveryHash", false);
    m_bRememberCancelledFiles = ini.GetBool(L"RememberCancelledFiles", true);
    m_bRememberDownloadedFiles = ini.GetBool(L"RememberDownloadedFiles", true);
    m_bPartiallyPurgeOldKnownFiles = ini.GetBool(L"PartiallyPurgeOldKnownFiles", true);

    m_bWinaTransToolbar = ini.GetBool(L"WinaTransToolbar", true);
    m_bShowDownloadToolbar = ini.GetBool(L"ShowDownloadToolbar", false);

    m_bCryptLayerRequested = ini.GetBool(L"CryptLayerRequested", true);
    m_bCryptLayerRequired = ini.GetBool(L"CryptLayerRequired", false);
    m_dwKadUDPKey = ini.GetInt(L"KadUDPKey", GetRandomUInt32());

    UINT nTmp = ini.GetInt(L"CryptTCPPaddingLength", 128);
    m_byCryptTCPPaddingLength = (uint8)min(nTmp, 254);

    ///////////////////////////////////////////////////////////////////////////
    // Section: "Proxy"
    //
    proxy.EnablePassword = ini.GetBool(L"ProxyEnablePassword",false,L"Proxy");
    proxy.UseProxy = ini.GetBool(L"ProxyEnableProxy",false,L"Proxy");
    proxy.name = CStringA(ini.GetString(L"ProxyName", L"", L"Proxy"));
    proxy.user = CStringA(ini.GetString(L"ProxyUser", L"", L"Proxy"));
    proxy.password = CStringA(ini.GetString(L"ProxyPassword", L"", L"Proxy"));
    proxy.port = (uint16)ini.GetInt(L"ProxyPort",1080,L"Proxy");
    proxy.type = (uint16)ini.GetInt(L"ProxyType",PROXYTYPE_SOCKS5,L"Proxy");


    ///////////////////////////////////////////////////////////////////////////
    // Section: "Statistics"
    //
    statsSaveInterval = ini.GetInt(L"SaveInterval", 60, L"Statistics");
    statsConnectionsGraphRatio = ini.GetInt(L"statsConnectionsGraphRatio", 3, L"Statistics");
    m_strStatsExpandedTreeItems = ini.GetString(L"statsExpandedTreeItems",L"111000000100000110000010000011110000010010",L"Statistics");
    CString buffer2;
    for (int i = 0; i < _countof(m_adwStatsColors); i++)
    {
        buffer2.Format(L"StatColor%i", i);
        m_adwStatsColors[i] = 0;
        if (_stscanf(ini.GetString(buffer2, L"", L"Statistics"), L"%i", &m_adwStatsColors[i]) != 1)
            ResetStatsColor(i);
    }
    bHasCustomTaskIconColor = ini.GetBool(L"HasCustomTaskIconColor",false, L"Statistics");
    m_bShowVerticalHourMarkers = ini.GetBool(L"ShowVerticalHourMarkers", true, L"Statistics");

    // -khaos--+++> Load Stats
    // I changed this to a separate function because it is now also used
    // to load the stats backup and to load stats from preferences.ini.old.
    LoadStats();
    // <-----khaos-

    ///////////////////////////////////////////////////////////////////////////
    // Section: "UPnP"
    //
    m_bEnableUPnP = ini.GetBool(L"EnableUPnP", true, L"UPnP");
    m_bSkipWANIPSetup = ini.GetBool(L"SkipWANIPSetup", false);
    m_bSkipWANPPPSetup = ini.GetBool(L"SkipWANPPPSetup", false);
    m_bCloseUPnPOnExit = ini.GetBool(L"CloseUPnPOnExit", true);
    m_nLastWorkingImpl = ini.GetInt(L"LastWorkingImplementation", 1 /*MiniUPnPLib*/);
    m_bIsMinilibImplDisabled = ini.GetBool(L"DisableMiniUPNPLibImpl", false);
    m_bIsWinServImplDisabled = ini.GetBool(L"DisableWinServImpl", false);

    LoadkMulePrefs(); //>>> WiZaRd::Own Prefs

    LoadCats();
    SetLanguage();
}

//>>> WiZaRd::Own Prefs
void CPreferences::LoadkMulePrefs()
{
    CString path;
    path.Format(L"%s%s.ini", GetMuleDirectory(EMULE_CONFIGDIR), MOD_INI_FILE);
    CIni ini(path, MOD_VERSION_PLAIN);

//>>> WiZaRd::IPFilter-Update
    m_uiIPfilterVersion = ini.GetInt(L"IPFilterVersion", 0);
    m_bAutoUpdateIPFilter = ini.GetBool(L"AutoUPdateIPFilter", true);
    m_strUpdateURLIPFilter = ini.GetString(L"UpdateURLIPFilter", MOD_IPPFILTER_URL);
//<<< WiZaRd::IPFilter-Update
//>>> WiZaRd::MediaInfoDLL Update
    m_bMediaInfoDllAutoUpdate = ini.GetBool(L"AutoUpdateMediaInfoDll", true);
    m_strMediaInfoDllUpdateURL = ini.GetString(L"UpdateURLMediaInfoDll", MOD_MEDIAINFO_URL);
    m_uiMediaInfoDllVersion = (UINT)ini.GetInt(L"MediaInfoDllVersion", 0);
//<<< WiZaRd::MediaInfoDLL Update
//>>> PreviewIndicator [WiZaRd]
    m_uiPreviewIndicatorMode = (uint8)ini.GetInt(L"PreviewIndicatorMode", ePIM_Icon);
    m_crPreviewReadyColor = ini.GetColRef(L"PreviewReadyColor", RGB(140, 225, 110)); //>>> jerrybg::ColorPreviewReadyFiles [WiZaRd]
//<<< PreviewIndicator [WiZaRd]
//>>> WiZaRd::Advanced Transfer Window Layout [Stulle]
    m_uTransferWnd1 = ini.GetInt(L"TransferWnd1", 1);
    m_uTransferWnd2 = ini.GetInt(L"TransferWnd2", 1);
    m_bSplitWindow = ini.GetBool(L"SplitWindow", false);
//<<< WiZaRd::Advanced Transfer Window Layout [Stulle]
    m_bUseSimpleProgress = ini.GetBool(L"UseSimpleProgress", true); //>>> WiZaRd::SimpleProgress
    m_uiSpamFilterMode = (uint8)ini.GetInt(L"SpamFilterMode", eSFM_AutoSort); //>>> WiZaRd::AntiFake
//>>> WiZaRd::AutoHL
    m_iAutoHLUpdateTimer = (uint16)ini.GetInt(L"AutoHLUpdate", 60);
    MINMAX(m_iAutoHLUpdateTimer, 10, 600);
    m_iUseAutoHL = (sint8)ini.GetInt(L"UseAutoHL", 1);
    m_iMinAutoHL = (uint16)ini.GetInt(L"MinAutoHL", 25);
    m_iMaxAutoHL = (uint16)ini.GetInt(L"MaxAutoHL", _UI16_MAX);
    m_iMaxAutoHL = max(m_iMinAutoHL, m_iMaxAutoHL);
    m_iMaxSourcesHL = (uint16)ini.GetInt(L"MaxSourcesHL", _UI16_MAX);
//<<< WiZaRd::AutoHL
//>>> WiZaRd::Remove forbidden files
    m_bRemoveForbiddenFiles = ini.GetBool(L"RemoveForbiddenFiles", true);
//<<< WiZaRd::Remove forbidden files
//>>> WiZaRd::Drop Blocking Sockets [Xman?]
    m_bDropBlockingSockets = ini.GetBool(L"DropBlockingSockets", true);
    m_fMaxBlockRate = ini.GetFloat(L"SocketBlockRate", 96.0f);
    m_fMaxBlockRate20 = ini.GetFloat(L"SocketBlockRate20", 98.0f);
//<<< WiZaRd::Drop Blocking Sockets [Xman?]
//>>> WiZaRd::Wine Compatibility
    m_bNeedsWineCompatibility = ini.GetBool(L"WineCompatibility", RunningWine());
//<<< WiZaRd::Wine Compatibility
//>>> WiZaRd::ModIconDLL Update
    m_bModIconDllAutoUpdate = ini.GetBool(L"AutoUpdateModIconDll", true);
    m_strModIconDllUpdateURL = ini.GetString(L"UpdateURLModIconDll", MOD_MODICON_URL);
//<<< WiZaRd::ModIconDLL Update
    m_iSpreadPrioLimit = ini.GetInt(L"SpreadPrioLimit",5); //>>> Tux::Spread Priority v3
}
//<<< WiZaRd::Own Prefs

WORD CPreferences::GetWindowsVersion()
{
    static bool bWinVerAlreadyDetected = false;
    if (!bWinVerAlreadyDetected)
    {
        bWinVerAlreadyDetected = true;
        m_wWinVer = DetectWinVersion();
    }
    return m_wWinVer;
}

UINT CPreferences::GetDefaultMaxConperFive()
{
    switch (GetWindowsVersion())
    {
    case _WINVER_98_:
        return 5;
    case _WINVER_95_:
    case _WINVER_ME_:
        return MAXCON5WIN9X;
    case _WINVER_2K_:
    case _WINVER_XP_:
        return MAXCONPER5SEC;
    default:
        return MAXCONPER5SEC;
    }
}

//////////////////////////////////////////////////////////
// category implementations
//////////////////////////////////////////////////////////

void CPreferences::SaveCats()
{
    CString strCatIniFilePath;
    strCatIniFilePath.Format(L"%sCategory.ini", GetMuleDirectory(EMULE_CONFIGDIR));
    (void)_tremove(strCatIniFilePath);
    CIni ini(strCatIniFilePath);
    ini.WriteInt(L"Count", catMap.GetCount() - 1, L"General");
    for (int i = 0; i < catMap.GetCount(); i++)
    {
        CString strSection;
        strSection.Format(L"Cat#%i", i);
        ini.SetSection(strSection);

        ini.WriteStringUTF8(L"Title", catMap.GetAt(i)->strTitle);
        ini.WriteStringUTF8(L"Incoming", catMap.GetAt(i)->strIncomingPath);
        ini.WriteStringUTF8(L"Comment", catMap.GetAt(i)->strComment);
        ini.WriteStringUTF8(L"RegularExpression", catMap.GetAt(i)->regexp);
        ini.WriteInt(L"Color", catMap.GetAt(i)->color);
        ini.WriteInt(L"a4afPriority", catMap.GetAt(i)->prio); // ZZ:DownloadManager
        ini.WriteStringUTF8(L"AutoCat", catMap.GetAt(i)->autocat);
        ini.WriteInt(L"Filter", catMap.GetAt(i)->filter);
        ini.WriteBool(L"FilterNegator", catMap.GetAt(i)->filterNeg);
        ini.WriteBool(L"AutoCatAsRegularExpression", catMap.GetAt(i)->ac_regexpeval);
        ini.WriteBool(L"downloadInAlphabeticalOrder", catMap.GetAt(i)->downloadInAlphabeticalOrder!=FALSE);
        ini.WriteBool(L"Care4All", catMap.GetAt(i)->care4all);
    }
}

void CPreferences::LoadCats()
{
    CString strCatIniFilePath;
    strCatIniFilePath.Format(L"%sCategory.ini", GetMuleDirectory(EMULE_CONFIGDIR));
    CIni ini(strCatIniFilePath);
    int iNumCategories = ini.GetInt(L"Count", 0, L"General");
    for (int i = 0; i <= iNumCategories; i++)
    {
        CString strSection;
        strSection.Format(L"Cat#%i", i);
        ini.SetSection(strSection);

        Category_Struct* newcat = new Category_Struct;
        newcat->filter = 0;
        newcat->strTitle = ini.GetStringUTF8(L"Title");
        if (i != 0) // All category
        {
            newcat->strIncomingPath = ini.GetStringUTF8(L"Incoming");
            MakeFoldername(newcat->strIncomingPath);
            if (!IsShareableDirectory(newcat->strIncomingPath)
                    || (!PathFileExists(newcat->strIncomingPath) && !::CreateDirectory(newcat->strIncomingPath, 0)))
            {
                newcat->strIncomingPath = GetMuleDirectory(EMULE_INCOMINGDIR);
                MakeFoldername(newcat->strIncomingPath);
            }
        }
        else
            newcat->strIncomingPath.Empty();
        newcat->strComment = ini.GetStringUTF8(L"Comment");
        newcat->prio = ini.GetInt(L"a4afPriority", PR_NORMAL); // ZZ:DownloadManager
        newcat->filter = ini.GetInt(L"Filter", 0);
        newcat->filterNeg = ini.GetBool(L"FilterNegator", FALSE);
        newcat->ac_regexpeval = ini.GetBool(L"AutoCatAsRegularExpression", FALSE);
        newcat->care4all = ini.GetBool(L"Care4All", FALSE);
        newcat->regexp = ini.GetStringUTF8(L"RegularExpression");
        newcat->autocat = ini.GetStringUTF8(L"Autocat");
        newcat->downloadInAlphabeticalOrder = ini.GetBool(L"downloadInAlphabeticalOrder", FALSE); // ZZ:DownloadManager
        newcat->color = ini.GetInt(L"Color", (DWORD)-1);
        AddCat(newcat);
    }
}

void CPreferences::RemoveCat(int index)
{
    if (index >= 0 && index < catMap.GetCount())
    {
        Category_Struct* delcat = catMap.GetAt(index);
        catMap.RemoveAt(index);
        delete delcat;
    }
}

bool CPreferences::SetCatFilter(int index, int filter)
{
    if (index >= 0 && index < catMap.GetCount())
    {
        catMap.GetAt(index)->filter = filter;
        return true;
    }
    return false;
}

int CPreferences::GetCatFilter(int index)
{
    if (index >= 0 && index < catMap.GetCount())
        return catMap.GetAt(index)->filter;
    return 0;
}

bool CPreferences::GetCatFilterNeg(int index)
{
    if (index >= 0 && index < catMap.GetCount())
        return catMap.GetAt(index)->filterNeg;
    return false;
}

void CPreferences::SetCatFilterNeg(int index, bool val)
{
    if (index >= 0 && index < catMap.GetCount())
        catMap.GetAt(index)->filterNeg = val;
}

bool CPreferences::MoveCat(UINT from, UINT to)
{
    if (from >= (UINT)catMap.GetCount() || to >= (UINT)catMap.GetCount() + 1 || from == to)
        return false;

    Category_Struct* tomove = catMap.GetAt(from);
    if (from < to)
    {
        catMap.RemoveAt(from);
        catMap.InsertAt(to - 1, tomove);
    }
    else
    {
        catMap.InsertAt(to, tomove);
        catMap.RemoveAt(from + 1);
    }
    SaveCats();
    return true;
}


DWORD CPreferences::GetCatColor(int index, int nDefault)
{
    if (index>=0 && index<catMap.GetCount())
    {
        DWORD c=catMap.GetAt(index)->color;
        if (c!=(DWORD)-1)
            return catMap.GetAt(index)->color;
    }

    return GetSysColor(nDefault);
}


///////////////////////////////////////////////////////

bool CPreferences::IsInstallationDirectory(const CString& rstrDir)
{
    CString strFullPath;
    if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
        strFullPath.ReleaseBuffer();
    else
        strFullPath = rstrDir;

    // skip sharing of several special eMule folders
    if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_EXECUTEABLEDIR)))
        return true;
    if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_CONFIGDIR)))
        return true;
    if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_INSTLANGDIR)))
        return true;
    if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_LOGDIR)))
        return true;

    return false;
}

bool CPreferences::IsShareableDirectory(const CString& rstrDir)
{
    if (IsInstallationDirectory(rstrDir))
        return false;

    CString strFullPath;
    if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
        strFullPath.ReleaseBuffer();
    else
        strFullPath = rstrDir;

    // skip sharing of several special eMule folders
    for (int i=0; i<GetTempDirCount(); i++)
        if (!CompareDirectories(strFullPath, GetTempDir(i)))			// ".\eMule\temp"
            return false;

    return true;
}

void CPreferences::UpdateLastVC()
{
    struct tm tmTemp;
    versioncheckLastAutomatic = safe_mktime(CTime::GetCurrentTime().GetLocalTm(&tmTemp));
}

void CPreferences::SetMaxUpload(UINT in)
{
    uint16 oldMaxUpload = (uint16)in;
    maxupload = (oldMaxUpload) ? oldMaxUpload : (uint16)UNLIMITED;
}

void CPreferences::SetMaxDownload(UINT in)
{
    uint16 oldMaxDownload = (uint16)in;
    maxdownload = (oldMaxDownload) ? oldMaxDownload : (uint16)UNLIMITED;
}

void CPreferences::SetUserNick(LPCTSTR pszNick)
{
    strNick = pszNick;
}

bool CPreferences::IsRunAsUserEnabled()
{
    return (GetWindowsVersion() == _WINVER_XP_ || GetWindowsVersion() == _WINVER_2K_ || GetWindowsVersion() == _WINVER_2003_)
           && m_bRunAsUser
           && m_nCurrentUserDirMode == eDM_Executable;
}

bool CPreferences::GetUseReBarToolbar()
{
    return GetReBarToolbar() && theApp.m_ullComCtrlVer >= MAKEDLLVERULL(5,8,0,0);
}

int	CPreferences::GetMaxGraphUploadRate(bool bEstimateIfUnlimited)
{
    if (maxGraphUploadRate != UNLIMITED || !bEstimateIfUnlimited)
    {
        return maxGraphUploadRate;
    }
    else
    {
        if (maxGraphUploadRateEstimated != 0)
        {
            return maxGraphUploadRateEstimated +4;
        }
        else
            return 16;
    }
}

void CPreferences::EstimateMaxUploadCap(UINT nCurrentUpload)
{
    if (maxGraphUploadRateEstimated+1 < nCurrentUpload)
    {
        maxGraphUploadRateEstimated = nCurrentUpload;
        if (maxGraphUploadRate == UNLIMITED && theApp.emuledlg && theApp.emuledlg->statisticswnd)
            theApp.emuledlg->statisticswnd->SetARange(false, thePrefs.GetMaxGraphUploadRate(true));
    }
}

void CPreferences::SetMaxGraphUploadRate(int in)
{
    maxGraphUploadRate	=(in) ? in : UNLIMITED;
}

bool CPreferences::IsDynUpEnabled()
{
    return m_bDynUpEnabled || maxGraphUploadRate == UNLIMITED;
}

bool CPreferences::CanFSHandleLargeFiles(int nForCat)
{
    bool bResult = false;
    for (int i = 0; i != tempdir.GetCount(); i++)
    {
        if (!IsFileOnFATVolume(tempdir.GetAt(i)))
        {
            bResult = true;
            break;
        }
    }
    return bResult && !IsFileOnFATVolume((nForCat > 0) ? GetCatPath(nForCat) : GetMuleDirectory(EMULE_INCOMINGDIR));
}

// General behavior:
//
// WinVer < Vista
// Default: ApplicationDir if preference.ini exists there. If not: user specific dirs if preferences.ini exits there. If not: again ApplicationDir
// Default overwritten by Registry value (see below)
// Fallback: ApplicationDir
//
// WinVer >= Vista:
// Default: User specific Dir if preferences.ini exists there. If not: All users dir, if preferences.ini exists there. If not user specific dirs again
// Default overwritten by Registry value (see below)
// Fallback: ApplicationDir
CString CPreferences::GetDefaultDirectory(EDefaultDirectory eDirectory, bool bCreate)
{
    if (m_astrDefaultDirs[0].IsEmpty())  // already have all directories fetched and stored?
    {
        // Get out exectuable starting directory which was our default till Vista
        TCHAR tchBuffer[MAX_PATH];
        ::GetModuleFileName(NULL, tchBuffer, _countof(tchBuffer));
        tchBuffer[_countof(tchBuffer) - 1] = L'\0';
        LPTSTR pszFileName = _tcsrchr(tchBuffer, L'\\') + 1;
        *pszFileName = L'\0';
        m_astrDefaultDirs[EMULE_EXECUTEABLEDIR] = tchBuffer;

        // set our results to old default / fallback values
        // those 3 dirs are the base for all others
        CString strSelectedDataBaseDirectory = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR];
        CString strSelectedConfigBaseDirectory = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR];
        CString strSelectedExpansionBaseDirectory = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR];
        m_nCurrentUserDirMode = eDM_Executable; // To let us know which "mode" we are using in case we want to switch per options

        // check if preferences.ini exists already in our default / fallback dir
        CFileFind ff;
        bool bConfigAvailableExecuteable = ff.FindFile(strSelectedConfigBaseDirectory + CONFIGFOLDER + _T("preferences.ini"), 0) != 0;
        ff.Close();

        // check if our registry setting is present which forces the single or multiuser directories
        // and lets us ignore other defaults
        // 0 = Multiuser, 1 = Publicuser, 2 = ExecuteableDir. (on Winver < Vista 1 has the same effect as 2)
        DWORD nRegistrySetting = (DWORD)eDM_Auto;
        CRegKey rkEMuleRegKey;
        if (rkEMuleRegKey.Open(HKEY_CURRENT_USER, _T("Software\\eMule"), KEY_READ) == ERROR_SUCCESS)
        {
            rkEMuleRegKey.QueryDWORDValue(_T("UsePublicUserDirectories"), nRegistrySetting);
            rkEMuleRegKey.Close();
        }
        if (nRegistrySetting != eDM_Auto && nRegistrySetting != eDM_MultiUser && nRegistrySetting != eDM_PublicUser && nRegistrySetting != eDM_Executable)
            nRegistrySetting = (DWORD)eDM_Auto;

        // Do we need to get SystemFolders or do we use our old Default anyway? (Executable Dir)
        if (nRegistrySetting == eDM_MultiUser
                || (nRegistrySetting == eDM_Auto && GetWindowsVersion() >= _WINVER_VISTA_)
                || (nRegistrySetting == eDM_Auto && (!bConfigAvailableExecuteable || GetWindowsVersion() >= _WINVER_VISTA_)))
        {
            HMODULE hShell32 = LoadLibrary(_T("shell32.dll"));
            if (hShell32)
            {
                if (GetWindowsVersion() >= _WINVER_VISTA_)
                {
                    PWSTR pszLocalAppData = NULL;
                    PWSTR pszPersonalDownloads = NULL;
                    PWSTR pszPublicDownloads = NULL;
                    PWSTR pszProgrammData = NULL;

                    // function not available on < WinVista
                    HRESULT(WINAPI *pfnSHGetKnownFolderPath)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR*);
                    (FARPROC&)pfnSHGetKnownFolderPath = GetProcAddress(hShell32, "SHGetKnownFolderPath");

                    if (pfnSHGetKnownFolderPath != NULL
                            && (*pfnSHGetKnownFolderPath)(FOLDERID_LocalAppData, 0, NULL, &pszLocalAppData) == S_OK
                            && (*pfnSHGetKnownFolderPath)(FOLDERID_Downloads, 0, NULL, &pszPersonalDownloads) == S_OK
                            && (*pfnSHGetKnownFolderPath)(FOLDERID_PublicDownloads, 0, NULL, &pszPublicDownloads) == S_OK
                            && (*pfnSHGetKnownFolderPath)(FOLDERID_ProgramData, 0, NULL, &pszProgrammData) == S_OK)
                    {
                        if (_tcsclen(pszLocalAppData) < MAX_PATH - 30 && _tcsclen(pszPersonalDownloads) < MAX_PATH - 40
                                && _tcsclen(pszProgrammData) < MAX_PATH - 30 && _tcsclen(pszPublicDownloads) < MAX_PATH - 40)
                        {
                            CString strLocalAppData  = pszLocalAppData;
                            CString strPersonalDownloads = pszPersonalDownloads;
                            CString strPublicDownloads = pszPublicDownloads;
                            CString strProgrammData = pszProgrammData;
                            if (strLocalAppData.Right(1) != L"\\")
                                strLocalAppData += L"\\";
                            if (strPersonalDownloads.Right(1) != L"\\")
                                strPersonalDownloads += L"\\";
                            if (strPublicDownloads.Right(1) != L"\\")
                                strPublicDownloads += L"\\";
                            if (strProgrammData.Right(1) != L"\\")
                                strProgrammData += L"\\";

                            if (nRegistrySetting == eDM_Auto)
                            {
                                // no registry default, check if we find a preferences.ini to use
                                bool bRes =  ff.FindFile(strLocalAppData + _T("eMule\\") + CONFIGFOLDER + _T("preferences.ini"), 0) != 0;
                                ff.Close();
                                if (bRes)
                                    m_nCurrentUserDirMode = eDM_MultiUser;
                                else
                                {
                                    bRes =  ff.FindFile(strProgrammData + _T("eMule\\") + CONFIGFOLDER + _T("preferences.ini"), 0) != 0;
                                    ff.Close();
                                    if (bRes)
                                        m_nCurrentUserDirMode = eDM_PublicUser;
                                    else if (bConfigAvailableExecuteable)
                                        m_nCurrentUserDirMode = eDM_Executable;
                                    else
                                        m_nCurrentUserDirMode = eDM_MultiUser; // no preferences.ini found, use the default
                                }
                            }
                            else
                                m_nCurrentUserDirMode = nRegistrySetting;

                            if (m_nCurrentUserDirMode == eDM_MultiUser)
                            {
                                // multiuser
                                strSelectedDataBaseDirectory = strPersonalDownloads + _T("eMule\\");
                                strSelectedConfigBaseDirectory = strLocalAppData + _T("eMule\\");
                                strSelectedExpansionBaseDirectory = strProgrammData + _T("eMule\\");
                            }
                            else if (m_nCurrentUserDirMode == eDM_PublicUser)
                            {
                                // public user
                                strSelectedDataBaseDirectory = strPublicDownloads + _T("eMule\\");
                                strSelectedConfigBaseDirectory = strProgrammData + _T("eMule\\");
                                strSelectedExpansionBaseDirectory = strProgrammData + _T("eMule\\");
                            }
                            else if (m_nCurrentUserDirMode == eDM_Executable)
                            {
                                // program directory
                            }
                            else
                                ASSERT(0);
                        }
                        else
                            ASSERT(0);
                    }

                    CoTaskMemFree(pszLocalAppData);
                    CoTaskMemFree(pszPersonalDownloads);
                    CoTaskMemFree(pszPublicDownloads);
                    CoTaskMemFree(pszProgrammData);
                }
                else   // GetWindowsVersion() >= _WINVER_VISTA_
                {
                    CString strAppData = ShellGetFolderPath(CSIDL_APPDATA);
                    CString strPersonal = ShellGetFolderPath(CSIDL_PERSONAL);
                    if (!strAppData.IsEmpty() && !strPersonal.IsEmpty())
                    {
                        if (strAppData.GetLength() < MAX_PATH - 30 && strPersonal.GetLength() < MAX_PATH - 40)
                        {
                            if (strPersonal.Right(1) != L"\\")
                                strPersonal += L"\\";
                            if (strAppData.Right(1) != L"\\")
                                strAppData += L"\\";
                            if (nRegistrySetting == eDM_MultiUser)
                            {
                                // registry setting overwrites, use these folders
                                strSelectedDataBaseDirectory = strPersonal + _T("eMule Downloads\\");
                                strSelectedConfigBaseDirectory = strAppData + _T("eMule\\");
                                m_nCurrentUserDirMode = eDM_MultiUser;
                                // strSelectedExpansionBaseDirectory stays default
                            }
                            else if (nRegistrySetting == eDM_Auto && !bConfigAvailableExecuteable)
                            {
                                if (ff.FindFile(strAppData + _T("eMule\\") + CONFIGFOLDER + _T("preferences.ini"), 0))
                                {
                                    // preferences.ini found, so we use this as default
                                    strSelectedDataBaseDirectory = strPersonal + _T("eMule Downloads\\");
                                    strSelectedConfigBaseDirectory = strAppData + _T("eMule\\");
                                    m_nCurrentUserDirMode = eDM_MultiUser;
                                }
                                ff.Close();
                            }
                            else
                                ASSERT(0);
                        }
                        else
                            ASSERT(0);
                    }
                }
                FreeLibrary(hShell32);
            }
            else
            {
                DebugLogError(_T("Unable to load shell32.dll to retrieve the systemfolder locations, using fallbacks"));
                ASSERT(0);
            }
        }

        // the use of ending backslashes is inconsistent, would need a rework throughout the code to fix this
        m_astrDefaultDirs[EMULE_CONFIGDIR] = strSelectedConfigBaseDirectory + CONFIGFOLDER;
        m_astrDefaultDirs[EMULE_TEMPDIR] = strSelectedDataBaseDirectory + _T("Temp");
        m_astrDefaultDirs[EMULE_INCOMINGDIR] = strSelectedDataBaseDirectory + _T("Incoming");
        m_astrDefaultDirs[EMULE_LOGDIR] = strSelectedConfigBaseDirectory + _T("logs\\");
        m_astrDefaultDirs[EMULE_ADDLANGDIR] = strSelectedExpansionBaseDirectory + _T("lang\\");
        m_astrDefaultDirs[EMULE_INSTLANGDIR] = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR] + _T("lang\\");
        m_astrDefaultDirs[EMULE_SKINDIR] = strSelectedExpansionBaseDirectory + _T("skins");
        m_astrDefaultDirs[EMULE_DATABASEDIR] = strSelectedDataBaseDirectory; // has ending backslashes
        m_astrDefaultDirs[EMULE_CONFIGBASEDIR] = strSelectedConfigBaseDirectory; // has ending backslashes
        //                EMULE_EXECUTEABLEDIR
        m_astrDefaultDirs[EMULE_TOOLBARDIR] = strSelectedExpansionBaseDirectory + _T("skins");
        m_astrDefaultDirs[EMULE_EXPANSIONDIR] = strSelectedExpansionBaseDirectory; // has ending backslashes

#ifdef _DEBUG
        theApp.QueueDebugLogLineEx(LOG_INFO, L"Directory mode: %u", m_nCurrentUserDirMode);
        CString strDebug = L"";;
        for (UINT i = 0; i < EMULE_DIRCOUNT; ++i)
            theApp.QueueDebugLogLineEx(LOG_INFO, L"Dir %u: %s", i, m_astrDefaultDirs[i]);
#endif
    }
    if (bCreate && !m_abDefaultDirsCreated[eDirectory])
    {
        switch (eDirectory)  // create the underlying directory first - be sure to adjust this if changing default directories
        {
        case EMULE_CONFIGDIR:
        case EMULE_LOGDIR:
            ::CreateDirectory(m_astrDefaultDirs[EMULE_CONFIGBASEDIR], NULL);
            break;
        case EMULE_TEMPDIR:
        case EMULE_INCOMINGDIR:
            ::CreateDirectory(m_astrDefaultDirs[EMULE_DATABASEDIR], NULL);
            break;
        case EMULE_ADDLANGDIR:
        case EMULE_SKINDIR:
        case EMULE_TOOLBARDIR:
            ::CreateDirectory(m_astrDefaultDirs[EMULE_EXPANSIONDIR], NULL);
            break;
        }
        ::CreateDirectory(m_astrDefaultDirs[eDirectory], NULL);
        m_abDefaultDirsCreated[eDirectory] = true;
    }
    return m_astrDefaultDirs[eDirectory];
}

CString	CPreferences::GetMuleDirectory(EDefaultDirectory eDirectory, bool bCreate)
{
    switch (eDirectory)
    {
    case EMULE_INCOMINGDIR:
        return m_strIncomingDir;
    case EMULE_TEMPDIR:
        ASSERT(0); // use GetTempDir() instead! This function can only return the first tempdirectory
        return GetTempDir(0);
    case EMULE_SKINDIR:
        return m_strSkinProfileDir;
    case EMULE_TOOLBARDIR:
        return m_sToolbarBitmapFolder;
    default:
        return GetDefaultDirectory(eDirectory, bCreate);
    }
}

void CPreferences::SetMuleDirectory(EDefaultDirectory eDirectory, CString strNewDir)
{
    switch (eDirectory)
    {
    case EMULE_INCOMINGDIR:
        m_strIncomingDir = strNewDir;
        break;
    case EMULE_SKINDIR:
        m_strSkinProfileDir = strNewDir;
        break;
    case EMULE_TOOLBARDIR:
        m_sToolbarBitmapFolder = strNewDir;
        break;
    default:
        ASSERT(0);
    }
}

void CPreferences::ChangeUserDirMode(int nNewMode)
{
    if (m_nCurrentUserDirMode == nNewMode)
        return;
    if (nNewMode == eDM_PublicUser && GetWindowsVersion() < _WINVER_VISTA_)
    {
        ASSERT(0);
        return;
    }
    // check if our registry setting is present which forces the single or multiuser directories
    // and lets us ignore other defaults
    // 0 = Multiuser, 1 = Publicuser, 2 = ExecuteableDir.
    CRegKey rkEMuleRegKey;
    if (rkEMuleRegKey.Create(HKEY_CURRENT_USER, _T("Software\\eMule")) == ERROR_SUCCESS)
    {
        if (rkEMuleRegKey.SetDWORDValue(_T("UsePublicUserDirectories"), nNewMode) != ERROR_SUCCESS)
            DebugLogError(_T("Failed to write registry key to switch UserDirMode"));
        else
            m_nCurrentUserDirMode = nNewMode;
        rkEMuleRegKey.Close();
    }
}

bool CPreferences::GetSparsePartFiles()
{
    // Vistas Sparse File implemenation seems to be buggy as far as i can see
    // If a sparsefile exceeds a given limit of write io operations in a certain order (or i.e. end to beginning)
    // in its lifetime, it will at some point throw out a FILE_SYSTEM_LIMITATION error and deny any writing
    // to this file.
    // It was suggested that Vista might limits the dataruns, which would lead to such a behavior, but wouldn't
    // make much sense for a sparse file implementation nevertheless.
    // Due to the fact that eMule wirtes a lot small blocks into sparse files and flushs them every 6 seconds,
    // this problem pops up sooner or later for all big files. I don't see any way to walk arround this for now
    // Update: This problem seems to be fixed on Win7, possibly on earlier Vista ServicePacks too
    //		   In any case, we allow sparse files for vesions earlier and later than Vista
    return m_bSparsePartFiles && (GetWindowsVersion() != _WINVER_VISTA_);
}

bool CPreferences::IsRunningAeroGlassTheme()
{
    // This is important for all functions which need to draw in the NC-Area (glass style)
    // Aero by default does not allow this, any drawing will not be visible. This can be turned off,
    // but Vista will not deliver the Glass style then as background when calling the default draw function
    // in other words, its draw all or nothing yourself - eMule chooses currently nothing
    static bool bAeroAlreadyDetected = false;
    if (!bAeroAlreadyDetected)
    {
        bAeroAlreadyDetected = true;
        m_bIsRunningAeroGlass = FALSE;
        if (GetWindowsVersion() >= _WINVER_VISTA_)
        {
            HMODULE hDWMAPI = LoadLibrary(_T("dwmapi.dll"));
            if (hDWMAPI)
            {
                HRESULT(WINAPI *pfnDwmIsCompositionEnabled)(BOOL*);
                (FARPROC&)pfnDwmIsCompositionEnabled = GetProcAddress(hDWMAPI, "DwmIsCompositionEnabled");
                if (pfnDwmIsCompositionEnabled != NULL)
                    pfnDwmIsCompositionEnabled(&m_bIsRunningAeroGlass);
                FreeLibrary(hDWMAPI);
            }
        }
    }
    return m_bIsRunningAeroGlass == TRUE ? true : false;
}

//>>> Remove forbidden files
bool CPreferences::IsForbiddenFile(const CString& rstrName)
{
    if (thePrefs.RemoveForbiddenFiles())
    {
        int curPos = 0;
        CString strFilter = m_strForbiddenFileFilters.Tokenize(L"|", curPos);
        while (!strFilter.IsEmpty())
        {
            strFilter.Trim();

            int iLen = strFilter.GetLength();
            if (rstrName.GetLength() >= iLen && rstrName.Right(iLen).CompareNoCase(strFilter) == 0)
                return true;

            strFilter = m_strForbiddenFileFilters.Tokenize(L"|", curPos);
        }
    }

    return false;
}
//<<< Remove forbidden files CString CPreferences::m_strExtractFolder;