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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once

enum eDirectoryMode
{
    eDM_Auto = -1,
    eDM_MultiUser = 0,
    eDM_PublicUser = 1,
    eDM_Executable = 2,
    eDM_Count = 3,
};

enum EViewSharedFilesAccess
{
    vsfaEverybody = 0,
    vsfaFriends = 1,
    vsfaNobody = 2
};

enum EDefaultDirectory
{
    EMULE_CONFIGDIR = 0,
    EMULE_TEMPDIR,
    EMULE_INCOMINGDIR,
    EMULE_LOGDIR,
    EMULE_ADDLANGDIR, // directories with languages installed by the eMule (parent: EMULE_EXPANSIONDIR)
    EMULE_INSTLANGDIR, // directories with languages installed by the user or installer (parent: EMULE_EXECUTEABLEDIR)
    EMULE_SKINDIR,
    EMULE_DATABASEDIR, // the parent directory of the incoming/temp folder
    EMULE_CONFIGBASEDIR, // the parent directory of the config folder
    EMULE_EXECUTEABLEDIR, // assumed to be not writeable (!)
    EMULE_TOOLBARDIR,
    EMULE_EXPANSIONDIR, // this is a base directory accessable for all users for things eMule installs
    EMULE_DIRCOUNT
};


enum EToolbarLabelType;
enum ELogFileFormat;

// DO NOT EDIT VALUES like making a uint16 to UINT, or insert any value. ONLY append new vars
#pragma pack(1)
struct Preferences_Ext_Struct
{
    uint8	version;
    uchar	userhash[16];
    WINDOWPLACEMENT EmuleWindowPlacement;
};
#pragma pack()

// deadlake PROXYSUPPORT
struct ProxySettings
{
    uint16		type;
    uint16		port;
    CStringA	name;
    CStringA	user;
    CStringA	password;
    bool		EnablePassword;
    bool		UseProxy;
};

struct Category_Struct
{
    CString	strIncomingPath;
    CString	strTitle;
    CString	strComment;
    DWORD	color;
    UINT	prio;
    CString autocat;
    CString	regexp;
    int		filter;
    bool	filterNeg;
    bool	care4all;
    bool	ac_regexpeval;
    bool	downloadInAlphabeticalOrder; // ZZ:DownloadManager
};


class CPreferences
{
public:
    static	CString	strNick;
    // ZZ:UploadSpeedSense -->
    static	uint16	minupload;
    // ZZ:UploadSpeedSense <--
    static	uint16	maxupload;
    static	uint16	maxdownload;
    static	LPCSTR	m_pszBindAddrA;
    static	CStringA m_strBindAddrA;
    static	LPCWSTR	m_pszBindAddrW;
    static	CStringW m_strBindAddrW;
    static	uint16	port;
    static	uint16	udpport;
    static	UINT	maxconnections;
    static	UINT	maxhalfconnections;
    static	bool	m_bConditionalTCPAccept;
    static	CString	m_strIncomingDir;
    static	CStringArray	tempdir;
    static	bool	updatenotify;
    static	bool	mintotray;
    static	bool	autotakeed2klinks;	   // Barry
    static	bool	addnewfilespaused;	   // Barry
    static	UINT	depth3D;			   // Barry
    static	bool	m_bEnableMiniMule;
    static	int		m_iStraightWindowStyles;
    static  bool	m_bUseSystemFontForMainControls;
    static	bool	m_bRTLWindowsLayout;
    static	CString	m_strSkinProfile;
    static	CString	m_strSkinProfileDir;
    static	UINT	maxsourceperfile;
    static	UINT	trafficOMeterInterval;
    static	UINT	statsInterval;
    static	bool	m_bFillGraphs;
    static	uchar	userhash[16];
    static	WINDOWPLACEMENT EmuleWindowPlacement;
    static	int		maxGraphDownloadRate;
    static	int		maxGraphUploadRate;
    static	UINT	maxGraphUploadRateEstimated;
    static	bool	beepOnError;
    static	bool	confirmExit;
    static	DWORD	m_adwStatsColors[15];
    static	bool	bHasCustomTaskIconColor;
    static  bool	m_bIconflashOnNewMessage;

    static	bool	splashscreen;
    static	bool	filterLANIPs;
    static	bool	m_bAllocLocalHostIP;
    static	bool	onlineSig;

    // -khaos--+++> Struct Members for Storing Statistics

    // Saved stats for cumulative downline overhead...
    static	uint64	cumDownOverheadTotal;
    static	uint64	cumDownOverheadFileReq;
    static	uint64	cumDownOverheadSrcEx;
    static	uint64	cumDownOverheadKad;
    static	uint64	cumDownOverheadTotalPackets;
    static	uint64	cumDownOverheadFileReqPackets;
    static	uint64	cumDownOverheadSrcExPackets;
    static	uint64	cumDownOverheadKadPackets;

    // Saved stats for cumulative upline overhead...
    static	uint64	cumUpOverheadTotal;
    static	uint64	cumUpOverheadFileReq;
    static	uint64	cumUpOverheadSrcEx;
    static	uint64	cumUpOverheadKad;
    static	uint64	cumUpOverheadTotalPackets;
    static	uint64	cumUpOverheadFileReqPackets;
    static	uint64	cumUpOverheadSrcExPackets;
    static	uint64	cumUpOverheadKadPackets;

    // Saved stats for cumulative upline data...
    static	UINT	cumUpSuccessfulSessions;
    static	UINT	cumUpFailedSessions;
    static	UINT	cumUpAvgTime;
    // Cumulative client breakdown stats for sent bytes...
    static	uint64	cumUpData_EDONKEY;
    static	uint64	cumUpData_EDONKEYHYBRID;
    static	uint64	cumUpData_EMULE;
    static	uint64	cumUpData_KMULE;
    static	uint64	cumUpData_MLDONKEY;
    static	uint64	cumUpData_AMULE;
    static	uint64	cumUpData_EMULECOMPAT;
    static	uint64	cumUpData_SHAREAZA;
    // Session client breakdown stats for sent bytes...
    static	uint64	sesUpData_EDONKEY;
    static	uint64	sesUpData_EDONKEYHYBRID;
    static	uint64	sesUpData_EMULE;
    static	uint64	sesUpData_KMULE;
    static	uint64	sesUpData_MLDONKEY;
    static	uint64	sesUpData_AMULE;
    static	uint64	sesUpData_EMULECOMPAT;
    static	uint64	sesUpData_SHAREAZA;

    // Cumulative port breakdown stats for sent bytes...
    static	uint64	cumUpDataPort_4662;
    static	uint64	cumUpDataPort_OTHER;
    // Session port breakdown stats for sent bytes...
    static	uint64	sesUpDataPort_4662;
    static	uint64	sesUpDataPort_OTHER;

    // Cumulative source breakdown stats for sent bytes...
    static	uint64	cumUpData_File;
    static	uint64	cumUpData_Partfile;
    // Session source breakdown stats for sent bytes...
    static	uint64	sesUpData_File;
    static	uint64	sesUpData_Partfile;

    // Saved stats for cumulative downline data...
    static	UINT	cumDownCompletedFiles;
    static	UINT	cumDownSuccessfulSessions;
    static	UINT	cumDownFailedSessions;
    static	UINT	cumDownAvgTime;

    // Cumulative statistics for saved due to compression/lost due to corruption
    static	uint64	cumLostFromCorruption;
    static	uint64	cumSavedFromCompression;
    static	UINT	cumPartsSavedByICH;

    // Session statistics for download sessions
    static	UINT	sesDownSuccessfulSessions;
    static	UINT	sesDownFailedSessions;
    static	UINT	sesDownAvgTime;
    static	UINT	sesDownCompletedFiles;
    static	uint64	sesLostFromCorruption;
    static	uint64	sesSavedFromCompression;
    static	UINT	sesPartsSavedByICH;

    // Cumulative client breakdown stats for received bytes...
    static	uint64	cumDownData_EDONKEY;
    static	uint64	cumDownData_EDONKEYHYBRID;
    static	uint64	cumDownData_EMULE;
    static	uint64	cumDownData_KMULE;
    static	uint64	cumDownData_MLDONKEY;
    static	uint64	cumDownData_AMULE;
    static	uint64	cumDownData_EMULECOMPAT;
    static	uint64	cumDownData_SHAREAZA;
    static	uint64	cumDownData_URL;
    // Session client breakdown stats for received bytes...
    static	uint64	sesDownData_EDONKEY;
    static	uint64	sesDownData_EDONKEYHYBRID;
    static	uint64	sesDownData_EMULE;
    static	uint64	sesDownData_KMULE;
    static	uint64	sesDownData_MLDONKEY;
    static	uint64	sesDownData_AMULE;
    static	uint64	sesDownData_EMULECOMPAT;
    static	uint64	sesDownData_SHAREAZA;
    static	uint64	sesDownData_URL;

    // Cumulative port breakdown stats for received bytes...
    static	uint64	cumDownDataPort_4662;
    static	uint64	cumDownDataPort_OTHER;
    // Session port breakdown stats for received bytes...
    static	uint64	sesDownDataPort_4662;
    static	uint64	sesDownDataPort_OTHER;

    // Saved stats for cumulative connection data...
    static	float	cumConnAvgDownRate;
    static	float	cumConnMaxAvgDownRate;
    static	float	cumConnMaxDownRate;
    static	float	cumConnAvgUpRate;
    static	float	cumConnMaxAvgUpRate;
    static	float	cumConnMaxUpRate;
    static	time_t	cumConnRunTime;
    static	UINT	cumConnAvgConnections;
    static	UINT	cumConnMaxConnLimitReached;
    static	UINT	cumConnPeakConnections;
    static	UINT	cumConnTransferTime;
    static	UINT	cumConnDownloadTime;
    static	UINT	cumConnUploadTime;

    // Saved records for shared files...
    static	UINT	cumSharedMostFilesShared;
    static	uint64	cumSharedLargestShareSize;
    static	uint64	cumSharedLargestAvgFileSize;
    static	uint64	cumSharedLargestFileSize;

    // Save the date when the statistics were last reset...
    static	time_t	stat_datetimeLastReset;

    // Save new preferences for PPgStats
    static	UINT	statsConnectionsGraphRatio; // This will store the divisor, i.e. for 1:3 it will be 3, for 1:20 it will be 20.
    // Save the expanded branches of the stats tree
    static	CString	m_strStatsExpandedTreeItems;

    static	UINT	statsSaveInterval;
    static  bool	m_bShowVerticalHourMarkers;
    // <-----khaos- End Statistics Members


    // Original Stats Stuff
    static	uint64	totalDownloadedBytes;
    static	uint64	totalUploadedBytes;
    // End Original Stats Stuff
    static	WORD	m_wLanguageID;
    static	bool	transferDoubleclick;
    static	EViewSharedFilesAccess m_iSeeShares;
    static	bool	bringtoforeground;
    static	UINT	splitterbarPosition;

    static	UINT	m_uTransferWnd1;
    static	UINT	m_uTransferWnd2;
    //MORPH START - Added by SiRoB, Splitting Bar [O�]
    static	UINT	splitterbarPositionStat;
    static	UINT	splitterbarPositionStat_HL;
    static	UINT	splitterbarPositionStat_HR;
    static	UINT	splitterbarPositionFriend;
    static	UINT	splitterbarPositionShared;
    //MORPH END - Added by SiRoB, Splitting Bar [O�]
    // -khaos--+++> Changed data type to avoid overflows
    static	UINT	statsMax;
    // <-----khaos-
    static	UINT	statsAverageMinutes;

    static	bool	m_bAddTimeStamp;

    static	bool	m_bRemove2bin;
    static	bool	m_bShowCopyEd2kLinkCmd;
    static	bool	m_bpreviewprio;
    static	bool	startMinimized;
    static	bool	m_bAutoStart;
    static	bool	m_bRestoreLastMainWndDlg;
    static	int		m_iLastMainWndDlgID;
    static	UINT	MaxConperFive;
    static	bool	m_bSparsePartFiles;
    static	CString	m_strYourHostname;
    static	bool	m_bEnableVerboseOptions;
    static	bool	m_bVerbose;
    static	bool	m_bFullVerbose;
    static  int		m_byLogLevel;
    static	bool	m_bDebugSourceExchange; // Sony April 23. 2003, button to keep source exchange msg out of verbose log
    static	bool	m_bLogBannedClients;
    static	bool	m_bLogRatingDescReceived;
    static	bool	m_bLogAnalyzerEvents; //>>> WiZaRd::ClientAnalyzer
    static	bool	m_bLogSecureIdent;
    static	bool	m_bLogFilteredIPs;
    static	bool	m_bLogFileSaving;
    static  bool    m_bLogA4AF; // ZZ:DownloadManager
    static	bool	m_bLogUlDlEvents;
    static	bool	m_bUseDebugDevice;
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    static	int		m_iDebugClientTCPLevel;
    static	int		m_iDebugClientUDPLevel;
    static	int		m_iDebugClientKadUDPLevel;
    static	int		m_iDebugSearchResultDetailLevel;
#endif
    static	bool	m_bupdatequeuelist;
    static	int		m_istartnextfile;
    static	bool	m_bshowoverhead;
    static	bool	m_bDAP;
    static	bool	m_bUAP;
    static	bool	m_bDisableKnownClientList;
    static	bool	m_bDisableQueueList;
    static	bool	m_bExtControls;
    static	bool	m_bTransflstRemain;

    static	UINT	versioncheckdays;
    static	bool	showRatesInTitle;

    static	CString	m_strTxtEditor;
    static	CString	m_strVideoPlayer;
    static	CString	m_strVideoPlayerArgs;
    static	bool	moviePreviewBackup;
    static	int		m_iPreviewSmallBlocks;
    static	bool	m_bPreviewCopiedArchives;

    static  CString m_strExtractFolder;
    static  bool    m_bExtractToIncomingDir;
    static	bool	m_bExtractArchives;

    static	int		m_iInspectAllFileTypes;
    static	bool	m_bPreviewOnIconDblClk;
    static	bool	m_bCheckFileOpen;
    static	bool	indicateratings;
    static	bool	watchclipboard;
    static	bool	m_bFirstStart;
    static	bool	m_bUpdate;

    static	bool	log2disk;
    static	bool	m_bLogAnalyzerToDisk; //>>> WiZaRd::ClientAnalyzer
    static	bool	debug2disk;
    static	int		iMaxLogBuff;
    static	UINT	uMaxLogFileSize;
    static	ELogFileFormat m_iLogFileFormat;
    static	bool	dontcompressavi;
    static	bool	msgonlyfriends;
    static	bool	msgsecure;

    static	UINT	filterlevel;
    static	UINT	m_iFileBufferSize;
    static	UINT	m_iQueueSize;
    static	int		m_iCommitFiles;
    static	UINT	m_uFileBufferTimeLimit;

    static	UINT	maxmsgsessions;
    static	UINT	versioncheckLastAutomatic;
    static	CString	messageFilter;
    static	CString	commentFilter;
    static	CString	filenameCleanups;
    static	CString	m_strDateTimeFormat;
    static	CString	m_strDateTimeFormat4Log;
    static	CString	m_strDateTimeFormat4Lists;
    static	LOGFONT m_lfHyperText;
    static	LOGFONT m_lfLogText;
    static	COLORREF m_crLogError;
    static	COLORREF m_crLogWarning;
    static	COLORREF m_crLogSuccess;
    static	int		m_iExtractMetaData;
    static	bool	m_bAdjustNTFSDaylightFileTime;
    static	bool	m_bRearrangeKadSearchKeywords;
    static  bool    m_bAllocFull;
    static	bool	m_bShowSharedFilesDetails;
    static  bool	m_bShowUpDownIconInTaskbar;
    static	bool	m_bForceSpeedsToKB;
    static	bool	m_bAutoShowLookups;
    static	bool	m_bExtraPreviewWithMenu;

    static	ProxySettings proxy; // deadlake PROXYSUPPORT

    static	bool	showCatTabInfos;
    static	bool	resumeSameCat;
    static	bool	dontRecreateGraphs;
    static	bool	autofilenamecleanup;
    //static	int		allcatType;
    //static	bool	allcatTypeNeg;
    static	bool	m_bUseAutocompl;
    static	bool	m_bShowDwlPercentage;
    static	bool	m_bRemoveFinishedDownloads;
    static	UINT	m_iMaxChatHistory;
    static	bool	m_bShowActiveDownloadsBold;

    static	int		m_iSearchMethod;

    // toolbar
    static	EToolbarLabelType m_nToolbarLabels;
    static	CString	m_sToolbarBitmap;
    static	CString	m_sToolbarBitmapFolder;
    static	CString	m_sToolbarSettings;
    static	bool	m_bReBarToolbar;
    static	CSize	m_sizToolbarIconSize;

    static	bool	m_bWinaTransToolbar;
    static	bool	m_bShowDownloadToolbar;

    //preview
    static	bool	m_bPreviewEnabled;
    static	bool	m_bAutomaticArcPreviewStart;

    // ZZ:UploadSpeedSense -->
    static	bool	m_bDynUpEnabled;
    static	int		m_iDynUpPingTolerance;
    static	int		m_iDynUpGoingUpDivider;
    static	int		m_iDynUpGoingDownDivider;
    static	int		m_iDynUpNumberOfPings;
    static  int		m_iDynUpPingToleranceMilliseconds;
    static  bool	m_bDynUpUseMillisecondPingTolerance;
    // ZZ:UploadSpeedSense <--

    static bool     m_bA4AFSaveCpu; // ZZ:DownloadManager

    static bool     m_bHighresTimer;

    static	bool	m_bResolveSharedShellLinks;
    static	CStringList shareddir_list;
    static  CList<uint8> shareddir_list_permissions; //>>> WiZaRd::SharePermissions
    static	CStringList addresses_list;
    static	bool	m_bKeepUnavailableFixedSharedDirs;

    static	int		m_iDbgHeap;
    static	bool	m_bRunAsUser;
    static	bool	m_bPreferRestrictedOverUser;

    static  bool	m_bUseOldTimeRemaining;

    // Firewall settings
    static bool		m_bOpenPortsOnStartUp;

    //AICH Options
    static bool		m_bTrustEveryHash;

    // files
    static bool		m_bRememberCancelledFiles;
    static bool		m_bRememberDownloadedFiles;
    static bool		m_bPartiallyPurgeOldKnownFiles;

    // encryption / obfuscation / verification
    static bool		m_bCryptLayerRequested;
    static bool		m_bCryptLayerRequired;
    static uint8	m_byCryptTCPPaddingLength;
    static UINT   m_dwKadUDPKey;

    // UPnP
    static bool		m_bSkipWANIPSetup;
    static bool		m_bSkipWANPPPSetup;
    static bool		m_bEnableUPnP;
    static bool		m_bCloseUPnPOnExit;
    static bool		m_bIsWinServImplDisabled;
    static bool		m_bIsMinilibImplDisabled;
    static int		m_nLastWorkingImpl;

    static BOOL		m_bIsRunningAeroGlass;
    static bool		m_bPreventStandby;
    static bool		m_bStoreSearches;



    enum Table
    {
        tableDownload,
        tableUpload,
        tableQueue,
        tableSearch,
        tableShared,
        tableClientList,
        tableFilenames,
        tableDownloadClients
    };

    friend class CPreferencesWnd;
    friend class CPPgGeneral;
    friend class CPPgConnection;
    friend class CPPgDirectories;
    friend class CPPgFiles;
    friend class Wizard;
    friend class CPPgTweaks;
    friend class CPPgDisplay;
    friend class CPPgSecurity;
    friend class CPPgDebug;

    CPreferences();
    ~CPreferences();

    static	void	Init();
    static	void	Uninit();

    static	LPCTSTR GetTempDir(int id = 0)
    {
        return (LPCTSTR)tempdir.GetAt((id < tempdir.GetCount()) ? id : 0);
    }
    static	int		GetTempDirCount()
    {
        return tempdir.GetCount();
    }
    static	bool	CanFSHandleLargeFiles(int nForCat);
    static	LPCTSTR GetConfigFile();
    static	const CString& GetFileCommentsFilePath()
    {
        return m_strFileCommentsFilePath;
    }
    static	CString	GetMuleDirectory(EDefaultDirectory eDirectory, bool bCreate = true);
    static	void	SetMuleDirectory(EDefaultDirectory eDirectory, CString strNewDir);
    static	void	ChangeUserDirMode(int nNewMode);

    static	bool	IsTempFile(const CString& rstrDirectory, const CString& rstrName);
    static	bool	IsShareableDirectory(const CString& rstrDirectory);
    static	bool	IsInstallationDirectory(const CString& rstrDir);

    static	bool	Save();
    static	void	SaveCats();

    static	const CString& GetUserNick()
    {
        return strNick;
    }
    static	void	SetUserNick(LPCTSTR pszNick);
    static	int		GetMaxUserNickLength()
    {
        return 50;
    }

    static	LPCSTR	GetBindAddrA()
    {
        return m_pszBindAddrA;
    }
    static	LPCWSTR	GetBindAddrW()
    {
        return m_pszBindAddrW;
    }
    static	uint16	GetPort()
    {
        return port;
    }
    static	uint16	GetUDPPort()
    {
        return udpport;
    }
    static	uchar*	GetUserHash()
    {
        return userhash;
    }
    // ZZ:UploadSpeedSense -->
    static	uint16	GetMinUpload()
    {
        return minupload;
    }
    // ZZ:UploadSpeedSense <--
    static	uint16	GetMaxUpload()
    {
        return maxupload;
    }
    static	bool	UpdateNotify()
    {
        return updatenotify;
    }
    static	bool	GetMinToTray()
    {
        return mintotray;
    }
    static	bool*	GetMinTrayPTR()
    {
        return &mintotray;
    }
    static	UINT	GetTrafficOMeterInterval()
    {
        return trafficOMeterInterval;
    }
    static	void	SetTrafficOMeterInterval(UINT in)
    {
        trafficOMeterInterval=in;
    }
    static	UINT	GetStatsInterval()
    {
        return statsInterval;
    }
    static	void	SetStatsInterval(UINT in)
    {
        statsInterval=in;
    }
    static	bool	GetFillGraphs()
    {
        return m_bFillGraphs;
    }
    static	void	SetFillGraphs(bool bFill)
    {
        m_bFillGraphs = bFill;
    }

    // -khaos--+++> Many, many, many, many methods.
    static	void	SaveStats(int bBackUp = 0);
    static	void	SetRecordStructMembers();
    static	void	SaveCompletedDownloadsStat();
    static	bool	LoadStats(int loadBackUp = 0);
    static	void	ResetCumulativeStatistics();

    static	void	Add2DownCompletedFiles()
    {
        cumDownCompletedFiles++;
    }
    static	void	SetConnMaxAvgDownRate(float in)
    {
        cumConnMaxAvgDownRate = in;
    }
    static	void	SetConnMaxDownRate(float in)
    {
        cumConnMaxDownRate = in;
    }
    static	void	SetConnAvgUpRate(float in)
    {
        cumConnAvgUpRate = in;
    }
    static	void	SetConnMaxAvgUpRate(float in)
    {
        cumConnMaxAvgUpRate = in;
    }
    static	void	SetConnMaxUpRate(float in)
    {
        cumConnMaxUpRate = in;
    }
    static	void	SetConnPeakConnections(int in)
    {
        cumConnPeakConnections = in;
    }
    static	void	SetUpAvgTime(int in)
    {
        cumUpAvgTime = in;
    }
    static	void	Add2DownSAvgTime(int in)
    {
        sesDownAvgTime += in;
    }
    static	void	SetDownCAvgTime(int in)
    {
        cumDownAvgTime = in;
    }
    static	void	Add2ConnTransferTime(int in)
    {
        cumConnTransferTime += in;
    }
    static	void	Add2ConnDownloadTime(int in)
    {
        cumConnDownloadTime += in;
    }
    static	void	Add2ConnUploadTime(int in)
    {
        cumConnUploadTime += in;
    }
    static	void	Add2DownSessionCompletedFiles()
    {
        sesDownCompletedFiles++;
    }
    static	void	Add2SessionTransferData(UINT uClientID, UINT uClientPort, BOOL bFromPF, BOOL bUpDown, UINT bytes, bool sentToFriend = false);
    static	void	Add2DownSuccessfulSessions()
    {
        sesDownSuccessfulSessions++;
        cumDownSuccessfulSessions++;
    }
    static	void	Add2DownFailedSessions()
    {
        sesDownFailedSessions++;
        cumDownFailedSessions++;
    }
    static	void	Add2LostFromCorruption(uint64 in)
    {
        sesLostFromCorruption += in;
    }
    static	void	Add2SavedFromCompression(uint64 in)
    {
        sesSavedFromCompression += in;
    }
    static	void	Add2SessionPartsSavedByICH(int in)
    {
        sesPartsSavedByICH += in;
    }

    // Saved stats for cumulative downline overhead
    static	uint64	GetDownOverheadTotal()
    {
        return cumDownOverheadTotal;
    }
    static	uint64	GetDownOverheadFileReq()
    {
        return cumDownOverheadFileReq;
    }
    static	uint64	GetDownOverheadSrcEx()
    {
        return cumDownOverheadSrcEx;
    }
    static	uint64	GetDownOverheadKad()
    {
        return cumDownOverheadKad;
    }
    static	uint64	GetDownOverheadTotalPackets()
    {
        return cumDownOverheadTotalPackets;
    }
    static	uint64	GetDownOverheadFileReqPackets()
    {
        return cumDownOverheadFileReqPackets;
    }
    static	uint64	GetDownOverheadSrcExPackets()
    {
        return cumDownOverheadSrcExPackets;
    }
    static	uint64	GetDownOverheadKadPackets()
    {
        return cumDownOverheadKadPackets;
    }

    // Saved stats for cumulative upline overhead
    static	uint64	GetUpOverheadTotal()
    {
        return cumUpOverheadTotal;
    }
    static	uint64	GetUpOverheadFileReq()
    {
        return cumUpOverheadFileReq;
    }
    static	uint64	GetUpOverheadSrcEx()
    {
        return cumUpOverheadSrcEx;
    }
    static	uint64	GetUpOverheadKad()
    {
        return cumUpOverheadKad;
    }
    static	uint64	GetUpOverheadTotalPackets()
    {
        return cumUpOverheadTotalPackets;
    }
    static	uint64	GetUpOverheadFileReqPackets()
    {
        return cumUpOverheadFileReqPackets;
    }
    static	uint64	GetUpOverheadSrcExPackets()
    {
        return cumUpOverheadSrcExPackets;
    }
    static	uint64	GetUpOverheadKadPackets()
    {
        return cumUpOverheadKadPackets;
    }

    // Saved stats for cumulative upline data
    static	UINT	GetUpSuccessfulSessions()
    {
        return cumUpSuccessfulSessions;
    }
    static	UINT	GetUpFailedSessions()
    {
        return cumUpFailedSessions;
    }
    static	UINT	GetUpAvgTime()
    {
        return cumUpAvgTime;
    }

    // Saved stats for cumulative downline data
    static	UINT	GetDownCompletedFiles()
    {
        return cumDownCompletedFiles;
    }
    static	UINT	GetDownC_SuccessfulSessions()
    {
        return cumDownSuccessfulSessions;
    }
    static	UINT	GetDownC_FailedSessions()
    {
        return cumDownFailedSessions;
    }
    static	UINT	GetDownC_AvgTime()
    {
        return cumDownAvgTime;
    }

    // Session download stats
    static	UINT	GetDownSessionCompletedFiles()
    {
        return sesDownCompletedFiles;
    }
    static	UINT	GetDownS_SuccessfulSessions()
    {
        return sesDownSuccessfulSessions;
    }
    static	UINT	GetDownS_FailedSessions()
    {
        return sesDownFailedSessions;
    }
    static	UINT	GetDownS_AvgTime()
    {
        return GetDownS_SuccessfulSessions() ? sesDownAvgTime / GetDownS_SuccessfulSessions() : 0;
    }

    // Saved stats for corruption/compression
    static	uint64	GetCumLostFromCorruption()
    {
        return cumLostFromCorruption;
    }
    static	uint64	GetCumSavedFromCompression()
    {
        return cumSavedFromCompression;
    }
    static	uint64	GetSesLostFromCorruption()
    {
        return sesLostFromCorruption;
    }
    static	uint64	GetSesSavedFromCompression()
    {
        return sesSavedFromCompression;
    }
    static	UINT	GetCumPartsSavedByICH()
    {
        return cumPartsSavedByICH;
    }
    static	UINT	GetSesPartsSavedByICH()
    {
        return sesPartsSavedByICH;
    }

    // Cumulative client breakdown stats for sent bytes
    static	uint64	GetUpTotalClientData()
    {
        return   GetCumUpData_EDONKEY()
                 + GetCumUpData_EDONKEYHYBRID()
                 + GetCumUpData_EMULE()
                 + GetCumUpData_KMULE()
                 + GetCumUpData_MLDONKEY()
                 + GetCumUpData_AMULE()
                 + GetCumUpData_EMULECOMPAT()
                 + GetCumUpData_SHAREAZA();
    }
    static	uint64	GetCumUpData_EDONKEY()
    {
        return (cumUpData_EDONKEY +		sesUpData_EDONKEY);
    }
    static	uint64	GetCumUpData_EDONKEYHYBRID()
    {
        return (cumUpData_EDONKEYHYBRID +	sesUpData_EDONKEYHYBRID);
    }
    static	uint64	GetCumUpData_EMULE()
    {
        return (cumUpData_EMULE +			sesUpData_EMULE);
    }
    static	uint64	GetCumUpData_KMULE()
    {
        return (cumUpData_KMULE +			sesUpData_KMULE);
    }
    static	uint64	GetCumUpData_MLDONKEY()
    {
        return (cumUpData_MLDONKEY +		sesUpData_MLDONKEY);
    }
    static	uint64	GetCumUpData_AMULE()
    {
        return (cumUpData_AMULE +			sesUpData_AMULE);
    }
    static	uint64	GetCumUpData_EMULECOMPAT()
    {
        return (cumUpData_EMULECOMPAT +	sesUpData_EMULECOMPAT);
    }
    static	uint64	GetCumUpData_SHAREAZA()
    {
        return (cumUpData_SHAREAZA +		sesUpData_SHAREAZA);
    }

    // Session client breakdown stats for sent bytes
    static	uint64	GetUpSessionClientData()
    {
        return   sesUpData_EDONKEY
                 + sesUpData_EDONKEYHYBRID
                 + sesUpData_EMULE
                 + sesUpData_KMULE
                 + sesUpData_MLDONKEY
                 + sesUpData_AMULE
                 + sesUpData_EMULECOMPAT
                 + sesUpData_SHAREAZA;
    }
    static	uint64	GetUpData_EDONKEY()
    {
        return sesUpData_EDONKEY;
    }
    static	uint64	GetUpData_EDONKEYHYBRID()
    {
        return sesUpData_EDONKEYHYBRID;
    }
    static	uint64	GetUpData_EMULE()
    {
        return sesUpData_EMULE;
    }
    static	uint64	GetUpData_KMULE()
    {
        return sesUpData_KMULE;
    }
    static	uint64	GetUpData_MLDONKEY()
    {
        return sesUpData_MLDONKEY;
    }
    static	uint64	GetUpData_AMULE()
    {
        return sesUpData_AMULE;
    }
    static	uint64	GetUpData_EMULECOMPAT()
    {
        return sesUpData_EMULECOMPAT;
    }
    static	uint64	GetUpData_SHAREAZA()
    {
        return sesUpData_SHAREAZA;
    }

    // Cumulative port breakdown stats for sent bytes...
    static	uint64	GetUpTotalPortData()
    {
        return   GetCumUpDataPort_4662()
                 + GetCumUpDataPort_OTHER();
    }
    static	uint64	GetCumUpDataPort_4662()
    {
        return (cumUpDataPort_4662 +		sesUpDataPort_4662);
    }
    static	uint64	GetCumUpDataPort_OTHER()
    {
        return (cumUpDataPort_OTHER +		sesUpDataPort_OTHER);
    }

    // Session port breakdown stats for sent bytes...
    static	uint64	GetUpSessionPortData()
    {
        return   sesUpDataPort_4662
                 + sesUpDataPort_OTHER;
    }
    static	uint64	GetUpDataPort_4662()
    {
        return sesUpDataPort_4662;
    }
    static	uint64	GetUpDataPort_OTHER()
    {
        return sesUpDataPort_OTHER;
    }

    // Cumulative DS breakdown stats for sent bytes...
    static	uint64	GetUpTotalDataFile()
    {
        return (GetCumUpData_File() +		GetCumUpData_Partfile());
    }
    static	uint64	GetCumUpData_File()
    {
        return (cumUpData_File +			sesUpData_File);
    }
    static	uint64	GetCumUpData_Partfile()
    {
        return (cumUpData_Partfile +		sesUpData_Partfile);
    }
    // Session DS breakdown stats for sent bytes...
    static	uint64	GetUpSessionDataFile()
    {
        return (sesUpData_File +			sesUpData_Partfile);
    }
    static	uint64	GetUpData_File()
    {
        return sesUpData_File;
    }
    static	uint64	GetUpData_Partfile()
    {
        return sesUpData_Partfile;
    }

    // Cumulative client breakdown stats for received bytes
    static	uint64	GetDownTotalClientData()
    {
        return   GetCumDownData_EDONKEY()
                 + GetCumDownData_EDONKEYHYBRID()
                 + GetCumDownData_EMULE()
                 + GetCumDownData_KMULE()
                 + GetCumDownData_MLDONKEY()
                 + GetCumDownData_AMULE()
                 + GetCumDownData_EMULECOMPAT()
                 + GetCumDownData_SHAREAZA()
                 + GetCumDownData_URL();
    }
    static	uint64	GetCumDownData_EDONKEY()
    {
        return (cumDownData_EDONKEY +			sesDownData_EDONKEY);
    }
    static	uint64	GetCumDownData_EDONKEYHYBRID()
    {
        return (cumDownData_EDONKEYHYBRID +	sesDownData_EDONKEYHYBRID);
    }
    static	uint64	GetCumDownData_EMULE()
    {
        return (cumDownData_EMULE +			sesDownData_EMULE);
    }
    static	uint64	GetCumDownData_KMULE()
    {
        return (cumDownData_KMULE +			sesDownData_KMULE);
    }
    static	uint64	GetCumDownData_MLDONKEY()
    {
        return (cumDownData_MLDONKEY +			sesDownData_MLDONKEY);
    }
    static	uint64	GetCumDownData_AMULE()
    {
        return (cumDownData_AMULE +			sesDownData_AMULE);
    }
    static	uint64	GetCumDownData_EMULECOMPAT()
    {
        return (cumDownData_EMULECOMPAT +		sesDownData_EMULECOMPAT);
    }
    static	uint64	GetCumDownData_SHAREAZA()
    {
        return (cumDownData_SHAREAZA +			sesDownData_SHAREAZA);
    }
    static	uint64	GetCumDownData_URL()
    {
        return (cumDownData_URL +				sesDownData_URL);
    }

    // Session client breakdown stats for received bytes
    static	uint64	GetDownSessionClientData()
    {
        return   sesDownData_EDONKEY
                 + sesDownData_EDONKEYHYBRID
                 + sesDownData_EMULE
                 + sesDownData_KMULE
                 + sesDownData_MLDONKEY
                 + sesDownData_AMULE
                 + sesDownData_EMULECOMPAT
                 + sesDownData_SHAREAZA
                 + sesDownData_URL;
    }
    static	uint64	GetDownData_EDONKEY()
    {
        return sesDownData_EDONKEY;
    }
    static	uint64	GetDownData_EDONKEYHYBRID()
    {
        return sesDownData_EDONKEYHYBRID;
    }
    static	uint64	GetDownData_EMULE()
    {
        return sesDownData_EMULE;
    }
    static	uint64	GetDownData_KMULE()
    {
        return sesDownData_KMULE;
    }
    static	uint64	GetDownData_MLDONKEY()
    {
        return sesDownData_MLDONKEY;
    }
    static	uint64	GetDownData_AMULE()
    {
        return sesDownData_AMULE;
    }
    static	uint64	GetDownData_EMULECOMPAT()
    {
        return sesDownData_EMULECOMPAT;
    }
    static	uint64	GetDownData_SHAREAZA()
    {
        return sesDownData_SHAREAZA;
    }
    static	uint64	GetDownData_URL()
    {
        return sesDownData_URL;
    }

    // Cumulative port breakdown stats for received bytes...
    static	uint64	GetDownTotalPortData()
    {
        return   GetCumDownDataPort_4662()
                 + GetCumDownDataPort_OTHER();
    }
    static	uint64	GetCumDownDataPort_4662()
    {
        return cumDownDataPort_4662		+ sesDownDataPort_4662;
    }
    static	uint64	GetCumDownDataPort_OTHER()
    {
        return cumDownDataPort_OTHER		+ sesDownDataPort_OTHER;
    }

    // Session port breakdown stats for received bytes...
    static	uint64	GetDownSessionDataPort()
    {
        return   sesDownDataPort_4662
                 + sesDownDataPort_OTHER;
    }
    static	uint64	GetDownDataPort_4662()
    {
        return sesDownDataPort_4662;
    }
    static	uint64	GetDownDataPort_OTHER()
    {
        return sesDownDataPort_OTHER;
    }

    // Saved stats for cumulative connection data
    static	float	GetConnAvgDownRate()
    {
        return cumConnAvgDownRate;
    }
    static	float	GetConnMaxAvgDownRate()
    {
        return cumConnMaxAvgDownRate;
    }
    static	float	GetConnMaxDownRate()
    {
        return cumConnMaxDownRate;
    }
    static	float	GetConnAvgUpRate()
    {
        return cumConnAvgUpRate;
    }
    static	float	GetConnMaxAvgUpRate()
    {
        return cumConnMaxAvgUpRate;
    }
    static	float	GetConnMaxUpRate()
    {
        return cumConnMaxUpRate;
    }
    static	time_t	GetConnRunTime()
    {
        return cumConnRunTime;
    }
    static	UINT	GetConnAvgConnections()
    {
        return cumConnAvgConnections;
    }
    static	UINT	GetConnMaxConnLimitReached()
    {
        return cumConnMaxConnLimitReached;
    }
    static	UINT	GetConnPeakConnections()
    {
        return cumConnPeakConnections;
    }
    static	UINT	GetConnTransferTime()
    {
        return cumConnTransferTime;
    }
    static	UINT	GetConnDownloadTime()
    {
        return cumConnDownloadTime;
    }
    static	UINT	GetConnUploadTime()
    {
        return cumConnUploadTime;
    }

    // Saved records for shared files
    static	UINT	GetSharedMostFilesShared()
    {
        return cumSharedMostFilesShared;
    }
    static	uint64	GetSharedLargestShareSize()
    {
        return cumSharedLargestShareSize;
    }
    static	uint64	GetSharedLargestAvgFileSize()
    {
        return cumSharedLargestAvgFileSize;
    }
    static	uint64	GetSharedLargestFileSize()
    {
        return cumSharedLargestFileSize;
    }

    // Get the long date/time when the stats were last reset
    static	time_t GetStatsLastResetLng()
    {
        return stat_datetimeLastReset;
    }
    static	CString GetStatsLastResetStr(bool formatLong = true);
    static	UINT	GetStatsSaveInterval()
    {
        return statsSaveInterval;
    }

    // Get and Set our new preferences
    static	void	SetStatsMax(UINT in)
    {
        statsMax = in;
    }
    static	void	SetStatsConnectionsGraphRatio(UINT in)
    {
        statsConnectionsGraphRatio = in;
    }
    static	UINT	GetStatsConnectionsGraphRatio()
    {
        return statsConnectionsGraphRatio;
    }
    static	void	SetExpandedTreeItems(CString in)
    {
        m_strStatsExpandedTreeItems = in;
    }
    static	const CString &GetExpandedTreeItems()
    {
        return m_strStatsExpandedTreeItems;
    }

    static	uint64	GetTotalDownloaded()
    {
        return totalDownloadedBytes;
    }
    static	uint64	GetTotalUploaded()
    {
        return totalUploadedBytes;
    }

    static	bool	IsErrorBeepEnabled()
    {
        return beepOnError;
    }
    static	bool	IsConfirmExitEnabled()
    {
        return confirmExit;
    }
    static	void	SetConfirmExit(bool bVal)
    {
        confirmExit = bVal;
    }
    static	bool	UseSplashScreen()
    {
        return splashscreen;
    }
    static	bool	FilterLANIPs()
    {
        return filterLANIPs;
    }
    static	bool	GetAllowLocalHostIP()
    {
        return m_bAllocLocalHostIP;
    }
    static	bool	IsOnlineSignatureEnabled()
    {
        return onlineSig;
    }
    static	int		GetMaxGraphUploadRate(bool bEstimateIfUnlimited);
    static	int		GetMaxGraphDownloadRate()
    {
        return maxGraphDownloadRate;
    }
    static	void	SetMaxGraphUploadRate(int in);
    static	void	SetMaxGraphDownloadRate(int in)
    {
        maxGraphDownloadRate=(in)?in:96;
    }

    static	uint16	GetMaxDownload();
    static	uint64	GetMaxDownloadInBytesPerSec(bool dynamic = false);
    static	UINT	GetMaxConnections()
    {
        return maxconnections;
    }
    static	UINT	GetMaxHalfConnections()
    {
        return maxhalfconnections;
    }
    static	UINT	GetMaxSourcePerFileDefault()
    {
        return maxsourceperfile;
    }
    static	bool	GetConditionalTCPAccept()
    {
        return m_bConditionalTCPAccept;
    }

    static	WORD	GetLanguageID();
    static	void	SetLanguageID(WORD lid);
    static	void	GetLanguages(CWordArray& aLanguageIDs);
    static	void	SetLanguage();
    static	bool	IsLanguageSupported(LANGID lidSelected, bool bUpdateBefore);
    static	CString GetLangDLLNameByID(LANGID lidSelected);
    static	void	InitThreadLocale();
    static	void	SetRtlLocale(LCID lcid);
    static	CString GetHtmlCharset();

    static	bool	IsDoubleClickEnabled()
    {
        return transferDoubleclick;
    }
    static	EViewSharedFilesAccess CanSeeShares(void)
    {
        return m_iSeeShares;
    }
    static	bool	IsBringToFront()
    {
        return bringtoforeground;
    }

    static	UINT	GetSplitterbarPosition()
    {
        return splitterbarPosition;
    }
    static	void	SetSplitterbarPosition(UINT pos)
    {
        splitterbarPosition=pos;
    }
    static	UINT	GetTransferWnd1()
    {
        return m_uTransferWnd1;
    }
    static	void	SetTransferWnd1(UINT uWnd1)
    {
        m_uTransferWnd1 = uWnd1;
    }
    static	UINT	GetTransferWnd2()
    {
        return m_uTransferWnd2;
    }
    static	void	SetTransferWnd2(UINT uWnd2)
    {
        m_uTransferWnd2 = uWnd2;
    }
    //MORPH START - Added by SiRoB, Splitting Bar [O�]
    static	UINT	GetSplitterbarPositionStat()
    {
        return splitterbarPositionStat;
    }
    static	void	SetSplitterbarPositionStat(UINT pos)
    {
        splitterbarPositionStat=pos;
    }
    static	UINT	GetSplitterbarPositionStat_HL()
    {
        return splitterbarPositionStat_HL;
    }
    static	void	SetSplitterbarPositionStat_HL(UINT pos)
    {
        splitterbarPositionStat_HL=pos;
    }
    static	UINT	GetSplitterbarPositionStat_HR()
    {
        return splitterbarPositionStat_HR;
    }
    static	void	SetSplitterbarPositionStat_HR(UINT pos)
    {
        splitterbarPositionStat_HR=pos;
    }
    static	UINT	GetSplitterbarPositionFriend()
    {
        return splitterbarPositionFriend;
    }
    static	void	SetSplitterbarPositionFriend(UINT pos)
    {
        splitterbarPositionFriend=pos;
    }
    static	UINT	GetSplitterbarPositionShared()
    {
        return splitterbarPositionShared;
    }
    static	void	SetSplitterbarPositionShared(UINT pos)
    {
        splitterbarPositionShared=pos;
    }
    //MORPH END   - Added by SiRoB, Splitting Bar [O�]
    // -khaos--+++> Changed datatype to avoid overflows
    static	UINT	GetStatsMax()
    {
        return statsMax;
    }
    // <-----khaos-
    static	bool	UseFlatBar()
    {
        return (depth3D==0);
    }
    static	int		GetStraightWindowStyles()
    {
        return m_iStraightWindowStyles;
    }
    static  bool	GetUseSystemFontForMainControls()
    {
        return m_bUseSystemFontForMainControls;
    }

    static	const CString& GetSkinProfile()
    {
        return m_strSkinProfile;
    }
    static	void	SetSkinProfile(LPCTSTR pszProfile)
    {
        m_strSkinProfile = pszProfile;
    }

    static	UINT	GetStatsAverageMinutes()
    {
        return statsAverageMinutes;
    }
    static	void	SetStatsAverageMinutes(UINT in)
    {
        statsAverageMinutes=in;
    }

    static	bool	GetEnableMiniMule()
    {
        return m_bEnableMiniMule;
    }
    static	bool	GetRTLWindowsLayout()
    {
        return m_bRTLWindowsLayout;
    }

    static	bool	GetAddTimeStamp()
    {
        return m_bAddTimeStamp;
    }

    static	WORD	GetWindowsVersion();
    static  bool	IsRunningAeroGlassTheme();
    static	bool	GetStartMinimized()
    {
        return startMinimized;
    }
    static	void	SetStartMinimized(bool instartMinimized)
    {
        startMinimized = instartMinimized;
    }
    static	bool	GetAutoStart()
    {
        return m_bAutoStart;
    }
    static	void	SetAutoStart(bool val)
    {
        m_bAutoStart = val;
    }

    static	bool	GetRestoreLastMainWndDlg()
    {
        return m_bRestoreLastMainWndDlg;
    }
    static	int		GetLastMainWndDlgID()
    {
        return m_iLastMainWndDlgID;
    }
    static	void	SetLastMainWndDlgID(int iID)
    {
        m_iLastMainWndDlgID = iID;
    }

    static	bool	GetPreviewPrio()
    {
        return m_bpreviewprio;
    }
    static	void	SetPreviewPrio(bool in)
    {
        m_bpreviewprio=in;
    }
    static	bool	GetUpdateQueueList()
    {
        return m_bupdatequeuelist;
    }
    static	int		StartNextFile()
    {
        return m_istartnextfile;
    }
    static	bool	ShowOverhead()
    {
        return m_bshowoverhead;
    }
    static	void	SetNewAutoUp(bool m_bInUAP)
    {
        m_bUAP = m_bInUAP;
    }
    static	bool	GetNewAutoUp()
    {
        return m_bUAP;
    }
    static	void	SetNewAutoDown(bool m_bInDAP)
    {
        m_bDAP = m_bInDAP;
    }
    static	bool	GetNewAutoDown()
    {
        return m_bDAP;
    }
    static	bool	IsKnownClientListDisabled()
    {
        return m_bDisableKnownClientList;
    }
    static	bool	IsQueueListDisabled()
    {
        return m_bDisableQueueList;
    }
    static	bool	IsFirstStart()
    {
        return m_bFirstStart;
    }

    static bool		IsUpdate()
    {
        return m_bUpdate;
    }

    static	const CString& GetTxtEditor()
    {
        return m_strTxtEditor;
    }
    static	const CString& GetVideoPlayer()
    {
        return m_strVideoPlayer;
    }
    static	const CString& GetVideoPlayerArgs()
    {
        return m_strVideoPlayerArgs;
    }

    static	UINT	GetFileBufferSize()
    {
        return m_iFileBufferSize;
    }
    static	UINT	GetFileBufferTimeLimit()
    {
        return m_uFileBufferTimeLimit;
    }
    static	UINT	GetQueueSize()
    {
        return m_iQueueSize;
    }
    static	int		GetCommitFiles()
    {
        return m_iCommitFiles;
    }
    static	bool	GetShowCopyEd2kLinkCmd()
    {
        return m_bShowCopyEd2kLinkCmd;
    }

    // Barry
    static	UINT	Get3DDepth()
    {
        return depth3D;
    }
    static	bool	AutoTakeED2KLinks()
    {
        return autotakeed2klinks;
    }
    static	bool	AddNewFilesPaused()
    {
        return addnewfilespaused;
    }

    static	bool	TransferlistRemainSortStyle()
    {
        return m_bTransflstRemain;
    }
    static	void	TransferlistRemainSortStyle(bool in)
    {
        m_bTransflstRemain=in;
    }

    static	DWORD	GetStatsColor(int index)
    {
        return m_adwStatsColors[index];
    }
    static	void	SetStatsColor(int index, DWORD value)
    {
        m_adwStatsColors[index] = value;
    }
    static	int		GetNumStatsColors()
    {
        return ARRSIZE(m_adwStatsColors);
    }
    static	void	GetAllStatsColors(int iCount, LPDWORD pdwColors);
    static	bool	SetAllStatsColors(int iCount, const DWORD* pdwColors);
    static	void	ResetStatsColor(int index);
    static	bool	HasCustomTaskIconColor()
    {
        return bHasCustomTaskIconColor;
    }

    static	void	SetMaxConsPerFive(UINT in)
    {
        MaxConperFive=in;
    }
    static	LPLOGFONT GetHyperTextLogFont()
    {
        return &m_lfHyperText;
    }
    static	void	SetHyperTextFont(LPLOGFONT plf)
    {
        m_lfHyperText = *plf;
    }
    static	LPLOGFONT GetLogFont()
    {
        return &m_lfLogText;
    }
    static	void	SetLogFont(LPLOGFONT plf)
    {
        m_lfLogText = *plf;
    }
    static	COLORREF GetLogErrorColor()
    {
        return m_crLogError;
    }
    static	COLORREF GetLogWarningColor()
    {
        return m_crLogWarning;
    }
    static	COLORREF GetLogSuccessColor()
    {
        return m_crLogSuccess;
    }

    static	UINT	GetMaxConperFive()
    {
        return MaxConperFive;
    }
    static	UINT	GetDefaultMaxConperFive();

    static	bool	IsMoviePreviewBackup()
    {
        return moviePreviewBackup;
    }
    static	int		GetPreviewSmallBlocks()
    {
        return m_iPreviewSmallBlocks;
    }
    static	bool	GetPreviewCopiedArchives()
    {
        return m_bPreviewCopiedArchives;
    }
    static	int		GetInspectAllFileTypes()
    {
        return m_iInspectAllFileTypes;
    }
    static	int		GetExtractMetaData()
    {
        return m_iExtractMetaData;
    }
    static	bool	GetAdjustNTFSDaylightFileTime()
    {
        return m_bAdjustNTFSDaylightFileTime;
    }
    static	bool	GetRearrangeKadSearchKeywords()
    {
        return m_bRearrangeKadSearchKeywords;
    }

    static	const CString& GetYourHostname()
    {
        return m_strYourHostname;
    }
    static	void	SetYourHostname(LPCTSTR pszHostname)
    {
        m_strYourHostname = pszHostname;
    }
    static	bool	GetSparsePartFiles();
    static	void	SetSparsePartFiles(bool bEnable)
    {
        m_bSparsePartFiles = bEnable;
    }
    static	bool	GetResolveSharedShellLinks()
    {
        return m_bResolveSharedShellLinks;
    }
    static  bool	IsShowUpDownIconInTaskbar()
    {
        return m_bShowUpDownIconInTaskbar;
    }

    static	void	SetMaxUpload(UINT in);
    static	void	SetMaxDownload(UINT in);

    static	WINDOWPLACEMENT GetEmuleWindowPlacement()
    {
        return EmuleWindowPlacement;
    }
    static	void	SetWindowLayout(WINDOWPLACEMENT in)
    {
        EmuleWindowPlacement=in;
    }

    static	UINT	GetUpdateDays()
    {
        return versioncheckdays;
    }
    static	UINT	GetLastVC()
    {
        return versioncheckLastAutomatic;
    }
    static	void	UpdateLastVC();
    static	int		GetIPFilterLevel()
    {
        return filterlevel;
    }
    static	const CString& GetMessageFilter()
    {
        return messageFilter;
    }
    static	const CString& GetCommentFilter()
    {
        return commentFilter;
    }
    static	void	SetCommentFilter(const CString& strFilter)
    {
        commentFilter = strFilter;
    }
    static	const CString& GetFilenameCleanups()
    {
        return filenameCleanups;
    }

    static	bool	ShowRatesOnTitle()
    {
        return showRatesInTitle;
    }
    static	void	LoadCats();
    static	const CString& GetDateTimeFormat()
    {
        return m_strDateTimeFormat;
    }
    static	const CString& GetDateTimeFormat4Log()
    {
        return m_strDateTimeFormat4Log;
    }
    static	const CString& GetDateTimeFormat4Lists()
    {
        return m_strDateTimeFormat4Lists;
    }

    // Download Categories (Ornis)
    static	int		AddCat(Category_Struct* cat)
    {
        catMap.Add(cat);
        return catMap.GetCount()-1;
    }
    static	bool	MoveCat(UINT from, UINT to);
    static	void	RemoveCat(int index);
    static	int		GetCatCount()
    {
        return catMap.GetCount();
    }
    static  bool	SetCatFilter(int index, int filter);
    static  int		GetCatFilter(int index);
    static	bool	GetCatFilterNeg(int index);
    static	void	SetCatFilterNeg(int index, bool val);
    static	Category_Struct* GetCategory(int index)
    {
        if (index>=0 && index<catMap.GetCount()) return catMap.GetAt(index);
        else return NULL;
    }
    static	const CString &GetCatPath(int index)
    {
        return catMap.GetAt(index)->strIncomingPath;
    }
    static	DWORD	GetCatColor(int index, int nDefault = COLOR_BTNTEXT);

    static	bool	GetPreviewOnIconDblClk()
    {
        return m_bPreviewOnIconDblClk;
    }
    static	bool	GetCheckFileOpen()
    {
        return m_bCheckFileOpen;
    }
    static	bool	ShowRatingIndicator()
    {
        return indicateratings;
    }
    static	bool	WatchClipboard4ED2KLinks()
    {
        return watchclipboard;
    }
    static	bool	GetRemoveToBin()
    {
        return m_bRemove2bin;
    }

    static	bool	GetLog2Disk()
    {
        return log2disk;
    }
//>>> WiZaRd::ClientAnalyzer
    static	bool	GetLogAnalyzerToDisk()
    {
        return m_bLogAnalyzerToDisk;
    }
//<<< WiZaRd::ClientAnalyzer
    static	bool	GetDebug2Disk()
    {
        return m_bVerbose && debug2disk;
    }
    static	int		GetMaxLogBuff()
    {
        return iMaxLogBuff;
    }
    static	UINT	GetMaxLogFileSize()
    {
        return uMaxLogFileSize;
    }
    static	ELogFileFormat GetLogFileFormat()
    {
        return m_iLogFileFormat;
    }

    static	void	SetMaxSourcesPerFile(UINT in)
    {
        maxsourceperfile=in;
    }
    static	void	SetMaxConnections(UINT in)
    {
        maxconnections =in;
    }
    static	void	SetMaxHalfConnections(UINT in)
    {
        maxhalfconnections =in;
    }
    static	bool	GetDontCompressAvi()
    {
        return dontcompressavi;
    }

    static	bool	MsgOnlyFriends()
    {
        return msgonlyfriends;
    }
    static	bool	MsgOnlySecure()
    {
        return msgsecure;
    }
    static	UINT	GetMsgSessionsMax()
    {
        return maxmsgsessions;
    }

    // deadlake PROXYSUPPORT
    static	const ProxySettings& GetProxySettings()
    {
        return proxy;
    }
    static	void	SetProxySettings(const ProxySettings& proxysettings)
    {
        proxy = proxysettings;
    }

    static	bool	ShowCatTabInfos()
    {
        return showCatTabInfos;
    }
    static	void	ShowCatTabInfos(bool in)
    {
        showCatTabInfos=in;
    }

    static	bool	AutoFilenameCleanup()
    {
        return autofilenamecleanup;
    }
    static	void	AutoFilenameCleanup(bool in)
    {
        autofilenamecleanup=in;
    }
    static	void	SetFilenameCleanups(CString in)
    {
        filenameCleanups=in;
    }

    static	bool	GetResumeSameCat()
    {
        return resumeSameCat;
    }
    static	bool	IsGraphRecreateDisabled()
    {
        return dontRecreateGraphs;
    }
    static	bool	IsExtControlsEnabled()
    {
        return m_bExtControls;
    }
    static	void	SetExtControls(bool in)
    {
        m_bExtControls=in;
    }
    static	bool	GetRemoveFinishedDownloads()
    {
        return m_bRemoveFinishedDownloads;
    }

    static	UINT	GetMaxChatHistoryLines()
    {
        return m_iMaxChatHistory;
    }
    static	bool	GetUseAutocompletion()
    {
        return m_bUseAutocompl;
    }
    static	bool	GetUseDwlPercentage()
    {
        return m_bShowDwlPercentage;
    }
    static	void	SetUseDwlPercentage(bool in)
    {
        m_bShowDwlPercentage=in;
    }
    static	bool	GetShowActiveDownloadsBold()
    {
        return m_bShowActiveDownloadsBold;
    }
    static	bool	GetShowSharedFilesDetails()
    {
        return m_bShowSharedFilesDetails;
    }
    static	void	SetShowSharedFilesDetails(bool bIn)
    {
        m_bShowSharedFilesDetails = bIn;
    }
    static	bool	GetAutoShowLookups()
    {
        return m_bAutoShowLookups;
    }
    static	void	SetAutoShowLookups(bool bIn)
    {
        m_bAutoShowLookups = bIn;
    }
    static	bool	GetForceSpeedsToKB()
    {
        return m_bForceSpeedsToKB;
    }
    static	bool	GetExtraPreviewWithMenu()
    {
        return m_bExtraPreviewWithMenu;
    }

    //Toolbar
    static	const CString& GetToolbarSettings()
    {
        return m_sToolbarSettings;
    }
    static	void	SetToolbarSettings(const CString& in)
    {
        m_sToolbarSettings = in;
    }
    static	const CString& GetToolbarBitmapSettings()
    {
        return m_sToolbarBitmap;
    }
    static	void	SetToolbarBitmapSettings(const CString& path)
    {
        m_sToolbarBitmap = path;
    }
    static	EToolbarLabelType GetToolbarLabelSettings()
    {
        return m_nToolbarLabels;
    }
    static	void	SetToolbarLabelSettings(EToolbarLabelType eLabelType)
    {
        m_nToolbarLabels = eLabelType;
    }
    static	bool	GetReBarToolbar()
    {
        return m_bReBarToolbar;
    }
    static	bool	GetUseReBarToolbar();
    static	CSize	GetToolbarIconSize()
    {
        return m_sizToolbarIconSize;
    }
    static	void	SetToolbarIconSize(CSize siz)
    {
        m_sizToolbarIconSize = siz;
    }

    static	bool	IsTransToolbarEnabled()
    {
        return m_bWinaTransToolbar;
    }
    static	bool	IsDownloadToolbarEnabled()
    {
        return m_bShowDownloadToolbar;
    }
    static	void	SetDownloadToolbar(bool bShow)
    {
        m_bShowDownloadToolbar = bShow;
    }

    static	int		GetSearchMethod()
    {
        return m_iSearchMethod;
    }
    static	void	SetSearchMethod(int iMethod)
    {
        m_iSearchMethod = iMethod;
    }

    // ZZ:UploadSpeedSense -->
    static	bool	IsDynUpEnabled();
    static	void	SetDynUpEnabled(bool newValue)
    {
        m_bDynUpEnabled = newValue;
    }
    static	int		GetDynUpPingTolerance()
    {
        return m_iDynUpPingTolerance;
    }
    static	int		GetDynUpGoingUpDivider()
    {
        return m_iDynUpGoingUpDivider;
    }
    static	int		GetDynUpGoingDownDivider()
    {
        return m_iDynUpGoingDownDivider;
    }
    static	int		GetDynUpNumberOfPings()
    {
        return m_iDynUpNumberOfPings;
    }
    static  bool	IsDynUpUseMillisecondPingTolerance()
    {
        return m_bDynUpUseMillisecondPingTolerance;   // EastShare - Added by TAHO, USS limit
    }
    static  int		GetDynUpPingToleranceMilliseconds()
    {
        return m_iDynUpPingToleranceMilliseconds;   // EastShare - Added by TAHO, USS limit
    }
    static  void	SetDynUpPingToleranceMilliseconds(int in)
    {
        m_iDynUpPingToleranceMilliseconds = in;
    }
    // ZZ:UploadSpeedSense <--

    static bool     GetA4AFSaveCpu()
    {
        return m_bA4AFSaveCpu;   // ZZ:DownloadManager
    }

    static bool     GetHighresTimer()
    {
        return m_bHighresTimer;
    }

    static	bool	UseSimpleTimeRemainingComputation()
    {
        return m_bUseOldTimeRemaining;
    }

    static	bool	IsRunAsUserEnabled();
    static	bool	IsPreferingRestrictedOverUser()
    {
        return m_bPreferRestrictedOverUser;
    }

    // Verbose log options
    static	bool	GetEnableVerboseOptions()
    {
        return m_bEnableVerboseOptions;
    }
    static	bool	GetVerbose()
    {
        return m_bVerbose;
    }
    static	bool	GetFullVerbose()
    {
        return m_bVerbose && m_bFullVerbose;
    }
    static	bool	GetDebugSourceExchange()
    {
        return m_bVerbose && m_bDebugSourceExchange;
    }
    static	bool	GetLogBannedClients()
    {
        return m_bVerbose && m_bLogBannedClients;
    }
    static	bool	GetLogRatingDescReceived()
    {
        return m_bVerbose && m_bLogRatingDescReceived;
    }
//>>> WiZaRd::ClientAnalyzer
    static	bool	GetLogAnalyzerEvents()
    {
        return m_bLogAnalyzerEvents;
    }
//<<< WiZaRd::ClientAnalyzer
    static	bool	GetLogSecureIdent()
    {
        return m_bVerbose && m_bLogSecureIdent;
    }
    static	bool	GetLogFilteredIPs()
    {
        return m_bVerbose && m_bLogFilteredIPs;
    }
    static	bool	GetLogFileSaving()
    {
        return m_bVerbose && m_bLogFileSaving;
    }
    static	bool	GetLogA4AF()
    {
        return m_bVerbose && m_bLogA4AF;   // ZZ:DownloadManager
    }
    static	bool	GetLogUlDlEvents()
    {
        return m_bVerbose && m_bLogUlDlEvents;
    }
    static	bool	GetLogKadSecurityEvents()
    {
        return m_bVerbose && true;
    }
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    static	bool	GetUseDebugDevice()
    {
        return m_bUseDebugDevice;
    }
    static	int		GetDebugClientTCPLevel()
    {
        return m_iDebugClientTCPLevel;
    }
    static	int		GetDebugClientUDPLevel()
    {
        return m_iDebugClientUDPLevel;
    }
    static	int		GetDebugClientKadUDPLevel()
    {
        return m_iDebugClientKadUDPLevel;
    }
    static	int		GetDebugSearchResultDetailLevel()
    {
        return m_iDebugSearchResultDetailLevel;
    }
#else
    // for normal release builds these options are all turned off
    inline static bool  GetUseDebugDevice()
    {
        return false;
    }
    inline static int   GetDebugClientTCPLevel()
    {
        return 0;
    }
    inline static int   GetDebugClientUDPLevel()
    {
        return 0;
    }
    inline static int   GetDebugClientKadUDPLevel()
    {
        return 0;
    }
    inline static int   GetDebugSearchResultDetailLevel()
    {
        return 0;
    }
#endif
    static	int		GetVerboseLogPriority()
    {
        return	m_byLogLevel;
    }

    // Firewall settings
    static  bool	IsOpenPortsOnStartupEnabled()
    {
        return m_bOpenPortsOnStartUp;
    }

    //AICH Hash
    static	bool	IsTrustingEveryHash()
    {
        return m_bTrustEveryHash;   // this is a debug option
    }

    static	bool	IsRememberingDownloadedFiles()
    {
        return m_bRememberDownloadedFiles;
    }
    static	bool	IsRememberingCancelledFiles()
    {
        return m_bRememberCancelledFiles;
    }
    static  bool	DoPartiallyPurgeOldKnownFiles()
    {
        return m_bPartiallyPurgeOldKnownFiles;
    }
    static	void	SetRememberDownloadedFiles(bool nv)
    {
        m_bRememberDownloadedFiles = nv;
    }
    static	void	SetRememberCancelledFiles(bool nv)
    {
        m_bRememberCancelledFiles = nv;
    }

    static  bool	DoFlashOnNewMessage()
    {
        return m_bIconflashOnNewMessage;
    }
    static  void	IniCopy(CString si, CString di);

    static	void	EstimateMaxUploadCap(UINT nCurrentUpload);
    static  bool	GetAllocCompleteMode()
    {
        return m_bAllocFull;
    }
    static  void	SetAllocCompleteMode(bool in)
    {
        m_bAllocFull=in;
    }

    // encryption
    static bool		IsClientCryptLayerRequested()
    {
        return m_bCryptLayerRequested;
    }
    static bool		IsClientCryptLayerRequired()
    {
        return IsClientCryptLayerRequested() && m_bCryptLayerRequired;
    }
    static bool		IsClientCryptLayerRequiredStrict()
    {
        return false;   // not even incoming test connections will be answered
    }
    static UINT	GetKadUDPKey()
    {
        return m_dwKadUDPKey;
    }
    static uint8	GetCryptTCPPaddingLength()
    {
        return m_byCryptTCPPaddingLength;
    }

    // UPnP
    static bool		GetSkipWANIPSetup()
    {
        return m_bSkipWANIPSetup;
    }
    static bool		GetSkipWANPPPSetup()
    {
        return m_bSkipWANPPPSetup;
    }
    static bool		IsUPnPEnabled()
    {
        return m_bEnableUPnP;
    }
    static void		SetSkipWANIPSetup(bool nv)
    {
        m_bSkipWANIPSetup = nv;
    }
    static void		SetSkipWANPPPSetup(bool nv)
    {
        m_bSkipWANPPPSetup = nv;
    }
    static bool		CloseUPnPOnExit()
    {
        return m_bCloseUPnPOnExit;
    }
    static bool		IsWinServUPnPImplDisabled()
    {
        return m_bIsWinServImplDisabled;
    }
    static bool		IsMinilibUPnPImplDisabled()
    {
        return m_bIsMinilibImplDisabled;
    }
    static int		GetLastWorkingUPnPImpl()
    {
        return m_nLastWorkingImpl;
    }
    static void		SetLastWorkingUPnPImpl(int val)
    {
        m_nLastWorkingImpl = val;
    }

    static bool		IsStoringSearchesEnabled()
    {
        return m_bStoreSearches;
    }
    static bool		GetPreventStandby()
    {
        return m_bPreventStandby;
    }

protected:
    static	CString m_strFileCommentsFilePath;
    static	Preferences_Ext_Struct* prefsExt;
    static	WORD m_wWinVer;
    static	CArray<Category_Struct*,Category_Struct*> catMap;
    static	CString	m_astrDefaultDirs[EMULE_DIRCOUNT];
    static	bool	m_abDefaultDirsCreated[EMULE_DIRCOUNT];
    static	int		m_nCurrentUserDirMode; // Only for PPgTweaks

    static void	CreateUserHash();
    static void	SetStandartValues();
    static int	GetRecommendedMaxConnections();
    static void LoadPreferences();
    static void SavePreferences();
//>>> WiZaRd::Own Prefs
    static void	LoadkMulePrefs();
    static void	SavekMulePrefs();
//<<< WiZaRd::Own Prefs
    static CString	GetDefaultDirectory(EDefaultDirectory eDirectory, bool bCreate = true);

//>>> WiZaRd::IntelliFlush
public:
    static	void	SetFileBufferSize(const UINT& i)
    {
        m_iFileBufferSize = i;
    }
//<<< WiZaRd::IntelliFlush
//>>> WiZaRd::IPFilter-Update
private:
    static	bool	m_bAutoUpdateIPFilter;
    static	UINT	m_uiIPfilterVersion;
    static	CString	m_strUpdateURLIPFilter;
public:
    static	CString	GetUpdateURLIPFilter()
    {
        return m_strUpdateURLIPFilter;
    }
    static	UINT	GetIPfilterVersion()
    {
        return m_uiIPfilterVersion;
    }
    static	void	SetIpfilterVersion(const UINT i)
    {
        m_uiIPfilterVersion = i;
    }
    static	bool	IsAutoUpdateIPFilterEnabled()
    {
        return m_bAutoUpdateIPFilter;
    }
    static	void	SetAutoUpdateIPFilter(const bool b)
    {
        m_bAutoUpdateIPFilter = b;
    }
//<<< WiZaRd::IPFilter-Update
//>>> WiZaRd::MediaInfoDLL Update
private:
    static	bool	m_bMediaInfoDllAutoUpdate;
    static	UINT	m_uiMediaInfoDllVersion;

public:
    static	CString	m_strMediaInfoDllUpdateURL;

    static	bool	IsAutoUpdateMediaInfoDllEnabled()
    {
        return m_bMediaInfoDllAutoUpdate;
    }
    static	void	SetAutoUpdateMediaInfoDll(const bool b)
    {
        m_bMediaInfoDllAutoUpdate = b;
    }

    static  CString GetMediaInfoDllUpdateURL()
    {
        return m_strMediaInfoDllUpdateURL;
    }

    static	UINT	GetMediaInfoDllVersion()
    {
        return m_uiMediaInfoDllVersion;
    }
    static	void	SetMediaInfoDllVersion(const UINT i)
    {
        m_uiMediaInfoDllVersion = i;
    }
//<<< WiZaRd::MediaInfoDLL Update
//>>> PreviewIndicator [WiZaRd]
private:
    static	uint8		m_uiPreviewIndicatorMode;
    static	COLORREF m_crPreviewReadyColor; //>>> jerrybg::ColorPreviewReadyFiles [WiZaRd]
public:
    static	uint8		GetPreviewIndicatorMode()
    {
        return m_uiPreviewIndicatorMode;
    }
//>>> jerrybg::ColorPreviewReadyFiles [WiZaRd]
    static	COLORREF GetPreviewReadyColor()
    {
        return m_crPreviewReadyColor;
    }
//<<< jerrybg::ColorPreviewReadyFiles [WiZaRd]
//<<< PreviewIndicator [WiZaRd]
//>>> WiZaRd::Advanced Transfer Window Layout [Stulle]
private:
    static bool		m_bSplitWindow;
public:
    static	bool	GetSplitWindow()
    {
        return m_bSplitWindow;
    }
    static	void	SetSplitWindow(const bool b)
    {
        m_bSplitWindow = b;
    }
//<<< WiZaRd::Advanced Transfer Window Layout [Stulle]
//>>> WiZaRd::SimpleProgress
private:
    static	bool	m_bUseSimpleProgress;
public:
    static	bool	GetUseSimpleProgress()
    {
        return m_bUseSimpleProgress;
    }
//<<< WiZaRd::SimpleProgress
//>>> WiZaRd::AntiFake
private:
    static	uint8	m_uiSpamFilterMode;
public:
    static	uint8	GetSpamFilterMode()
    {
        return m_uiSpamFilterMode;
    }
//<<< WiZaRd::AntiFake
//>>> WiZaRd::AutoHL
private:
    static  uint16	m_iMinAutoHL;
    static	uint16	m_iMaxAutoHL;
    static	uint16	m_iMaxSourcesHL;
    static	sint8	m_iUseAutoHL;
    static	uint16	m_iAutoHLUpdateTimer;
public:
    static	uint16	GetAutoHLUpdateTimer()
    {
        return m_iAutoHLUpdateTimer;
    }
    static	void	SetAutoHLUpdateTimer(const uint16 i)
    {
        m_iAutoHLUpdateTimer = i;
    }
    static  uint16	GetMinAutoHL()
    {
        return m_iMinAutoHL;
    }
    static  void	SetMinAutoHL(const uint16 i)
    {
        m_iMinAutoHL = i;
    }
    static	uint16	GetMaxAutoHL()
    {
        return m_iMaxAutoHL;
    }
    static	void	SetMaxAutoHL(const uint16 i)
    {
        m_iMaxAutoHL = i;
    }
    static  uint16  GetMaxSourcesHL()
    {
        return m_iMaxSourcesHL;
    }
    static	void	SetMaxSourcesHL(const uint16 i)
    {
        m_iMaxSourcesHL = i;
    }
    static	sint8	IsUseAutoHL()
    {
        return m_iUseAutoHL;
    }
    static	void	SetUseAutoHL(const sint8 i)
    {
        m_iUseAutoHL = i;
    }
//<<< WiZaRd::AutoHL
//>>> WiZaRd::Remove forbidden files
private:
    static	bool	m_bRemoveForbiddenFiles;
    static	CString	m_strForbiddenFileFilters;
public:
    static	bool	IsForbiddenFile(const CString& rstrName);
    static	bool	RemoveForbiddenFiles()
    {
        return m_bRemoveForbiddenFiles;
    }
    static	void	SetRemoveForbiddenFiles(const bool b)
    {
        m_bRemoveForbiddenFiles = b;
    }
    static	CString	GetForbiddenFileFilters()
    {
        return m_strForbiddenFileFilters;
    }
    static	void	SetForbiddenFileFilters(const CString& str)
    {
        m_strForbiddenFileFilters = str;
    }
//<<< WiZaRd::Remove forbidden files
//>>> WiZaRd::Drop Blocking Sockets [Xman?]
private:
    static	bool	m_bDropBlockingSession;
    static	bool	m_bDropBlockingSockets;
    static	float	m_fMaxBlockRate;
    static	float	m_fMaxBlockRate20;
public:
    static	bool	DropBlockingSockets()
    {
        return m_bDropBlockingSession && m_bDropBlockingSockets;
    }
    static	void	SetDropBlockingSockets(const bool b)
    {
        m_bDropBlockingSockets = b;
    }
    static	float	GetMaxBlockRate()
    {
        return m_fMaxBlockRate;
    }
    static	void	SetMaxBlockRate(const float f)
    {
        m_fMaxBlockRate = f;
    }
    static	float	GetMaxBlockRate20()
    {
        return m_fMaxBlockRate20;
    }
    static	void	SetMaxBlockRate20(const float f)
    {
        m_fMaxBlockRate20 = f;
    }
//<<< WiZaRd::Drop Blocking Sockets [Xman?]
//>>> WiZaRd::Wine Compatibility
private:
    static bool		m_bNeedsWineCompatibility;
public:
    static bool		WeNeedWineCompatibility()
    {
        return m_bNeedsWineCompatibility;
    }
//<<< WiZaRd::Wine Compatibility
//>>> WiZaRd::ModIconDLL Update
private:
    static	bool	m_bModIconDllAutoUpdate;
public:
    static	CString	m_strModIconDllUpdateURL;

    static	bool	IsAutoUpdateModIconDllEnabled()
    {
        return m_bModIconDllAutoUpdate;
    }
    static	void	SetAutoUpdateModIconDll(const bool b)
    {
        m_bModIconDllAutoUpdate = b;
    }

    static  CString GetModIconDllUpdateURL()
    {
        return m_strModIconDllUpdateURL;
    }
//<<< WiZaRd::ModIconDLL Update
//>>> WiZaRd
public:
    static	int		GetCurrentUserDirMode()
    {
        return m_nCurrentUserDirMode;
    }
    static	void	SetCurrentUserDirMode(const int i)
    {
        m_nCurrentUserDirMode = i;
    }
//<<< WiZaRd
};

extern CPreferences thePrefs;
