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
#ifdef _DEBUG
#include "DebugHelpers.h"
#endif
#include "emule.h"
#include "UpDownClient.h"
#include "FriendList.h"
#include "Clientlist.h"
#include "OtherFunctions.h"
#include "PartFile.h"
#include "ListenSocket.h"
#include "Friend.h"
#include <zlib/zlib.h>
#include "Packets.h"
#include "Opcodes.h"
#include "SafeFile.h"
#include "Preferences.h"
#include "ClientCredits.h"
#include "IPFilter.h"
#include "Statistics.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "SearchFile.h"
#include "SearchList.h"
#include "SharedFileList.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Search.h"
#include "Kademlia/Kademlia/SearchManager.h"
#include "Kademlia/Kademlia/UDPFirewallTester.h"
#include "Kademlia/routing/RoutingZone.h"
#include "Kademlia/Utils/UInt128.h"
#include "Kademlia/Net/KademliaUDPListener.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "ChatWnd.h"
#include "CxImage/xImage.h"
#include "PreviewDlg.h"
#include "Exceptions.h"
#include "ClientUDPSocket.h"
#include "shahashset.h"
#include "Log.h"
#include "CaptchaGenerator.h"
//>>> WiZaRd::ClientAnalyzer
#include "./Mod/ClientAnalyzer.h"
#include "Version.h"
//<<< WiZaRd::ClientAnalyzer
#include "./Mod/ModIconMapping.h" //>>> WiZaRd::ModIconMapper

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define URLINDICATOR	L"http:|www.|.de |.net |.com |.org |.to |.tk |.cc |.fr |ftp:|ed2k:|https:|ftp.|.info|.biz|.uk|.eu|.es|.tv|.cn|.tw|.ws|.nu|.jp"

IMPLEMENT_DYNAMIC(CClientException, CException)
IMPLEMENT_DYNAMIC(CUpDownClient, CObject)

CUpDownClient::CUpDownClient(CClientReqSocket* sender)
{
    socket = sender;
    reqfile = NULL;
    Init();
}

CUpDownClient::CUpDownClient(CPartFile* in_reqfile, uint16 in_port, UINT in_userid,UINT in_serverip, uint16 in_serverport, bool ed2kID)
{
    //Converting to the HybridID system.. The ED2K system didn't take into account of IP address ending in 0..
    //All IP addresses ending in 0 were assumed to be a lowID because of the calculations.
    socket = NULL;
    reqfile = in_reqfile;
    Init();
    m_nUserPort = in_port;
    //If this is a ED2K source, check if it's a lowID.. If not, convert it to a HyrbidID.
    //Else, it's already in hybrid form.
    if (ed2kID && !IsLowID(in_userid))
        m_nUserIDHybrid = ntohl(in_userid);
    else
        m_nUserIDHybrid = in_userid;

    //If highID and ED2K source, incoming ID and IP are equal..
    //If highID and Kad source, incoming IP needs ntohl for the IP
    if (!HasLowID() && ed2kID)
        m_nConnectIP = in_userid;
    else if (!HasLowID())
        m_nConnectIP = ntohl(in_userid);
    m_dwServerIP = in_serverip;
    m_nServerPort = in_serverport;
}

void CUpDownClient::Init()
{
	m_iModIconIndex = MODMAP_NONE; //>>> WiZaRd::ModIconMapper
//>>> WiZaRd::ZZUL Upload [ZZ]
    m_nCurQueueSessionUp = 0;
    m_nCurSessionPayloadUp = 0;
    m_addedPayloadQueueSession = 0;
    m_bScheduledForRemoval = false;
    m_bScheduledForRemovalWillKeepWaitingTimeIntact = false;
//<<< WiZaRd::ZZUL Upload [ZZ]
    m_uiFailedUDPPacketsInARow = 0; //>>> WiZaRd::Detect UDP problem clients
    m_abyUpPartStatusHidden = NULL; //>>> WiZaRd::Intelligent SOTN
//>>> WiZaRd::ClientAnalyzer
    m_fAICHHashRequested = FALSE; //>>> Security
    m_fSourceExchangeRequested = FALSE; //>>> Security
    m_uiRealVersion = 0;
    pAntiLeechData = NULL;
//<<< WiZaRd::ClientAnalyzer
//>>> WiZaRd::ICS [enkeyDEV]
    m_abyIncPartStatus = NULL;
    m_incompletepartVer = 0;
//<<< WiZaRd::ICS [enkeyDEV]
    m_abySeenPartStatus = NULL; //>>> WiZaRd::AntiHideOS [netfinity]
    m_nChatstate = MS_NONE;
    m_nKadState = KS_NONE;
    m_nChatCaptchaState = CA_NONE;
    m_nUploadState = US_NONE;
    m_nDownloadState = DS_NONE;
    m_SecureIdentState = IS_UNAVAILABLE;
    m_nConnectingState = CCS_NONE;

    credits = NULL;
    m_nSumForAvgUpDataRate = 0;
//>>> WiZaRd::Fix for LowID slots only on connection [VQB]
    //m_bAddNextConnect = false;
    m_dwWouldHaveGottenUploadSlotIfNotLowIdTick = 0;
//<<< WiZaRd::Fix for LowID slots only on connection [VQB]
    m_cShowDR = 0;
    m_nUDPPort = 0;
    m_nKadPort = 0;
    m_nTransferredUp = 0;
    m_cAsked = 0;
    m_cDownAsked = 0;
    m_nUpDatarate = 0;
    m_pszUsername = 0;
    m_nUserIDHybrid = 0;
    m_dwServerIP = 0;
    m_nServerPort = 0;
    m_iFileListRequested = 0;
    m_dwLastUpRequest = 0;
    m_bEmuleProtocol = false;
    m_bCompleteSource = false;
    m_bFriendSlot = false;
    m_bCommentDirty = false;
    m_bReaskPending = false;
    m_bUDPPending = false;
    m_byEmuleVersion = 0;
    m_nUserPort = 0;
    m_nPartCount = 0;
    m_nUpPartCount = 0;
    m_abyPartStatus = 0;
    m_abyUpPartStatus = 0;
    m_dwUploadTime = 0;
    m_nTransferredDown = 0;
    m_nDownDatarate = 0;
    m_nDownDataRateMS = 0;
    m_dwLastBlockReceived = 0;
    m_byDataCompVer = 0;
    m_byUDPVer = 0;
    m_bySourceExchange1Ver = 0;
    m_byAcceptCommentVer = 0;
    m_byExtendedRequestsVer = 0;
    m_nRemoteQueueRank = 0;
    m_dwLastSourceRequest = 0;
    m_dwLastSourceAnswer = 0;
    m_dwLastAskedForSources = 0;
    m_byCompatibleClient = 0;
    m_nSourceFrom = SF_SERVER;
    m_bIsHybrid = false;
    m_bIsML=false;
    m_Friend = NULL;
    m_uFileRating=0;
    (void)m_strFileComment;
    m_fMessageFiltered = 0;
    m_fIsSpammer = 0;
    m_cMessagesReceived = 0;
    m_cMessagesSent = 0;
    m_nCurSessionUp = 0;
    m_nCurSessionDown = 0;
    m_nCurSessionPayloadDown = 0;
    m_nSumForAvgDownDataRate = 0;
    m_clientSoft=SO_UNKNOWN;
    m_bRemoteQueueFull = false;
    md4clr(m_achUserHash);
    SetBuddyID(NULL);
    m_nBuddyIP = 0;
    m_nBuddyPort = 0;
    if (socket)
    {
        SOCKADDR_IN sockAddr = {0};
        int nSockAddrLen = sizeof(sockAddr);
        socket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
        SetIP(sockAddr.sin_addr.S_un.S_addr);
    }
    else
    {
        SetIP(0);
    }
    m_fHashsetRequestingAICH = 0;
    m_fHashsetRequestingMD4 = 0;
    m_fSharedDirectories = 0;
    m_fSentCancelTransfer = 0;
    m_nClientVersion = 0;
    m_lastRefreshedDLDisplay = 0;
    m_dwDownStartTime = 0;
    m_nLastBlockOffset = (uint64)-1;
    m_bUnicodeSupport = false;
    m_dwLastSignatureIP = 0;
    m_bySupportSecIdent = 0;
    m_byInfopacketsReceived = IP_NONE;
    m_lastPartAsked = (uint16)-1;
    m_nUpCompleteSourcesCount= 0;
    m_fSupportsPreview = 0;
    m_fPreviewReqPending = 0;
    m_fPreviewAnsPending = 0;
    m_bTransferredDownMini = false;
    m_addedPayloadQueueSession = 0;
    m_nCurQueueSessionPayloadUp = 0; // PENDING: Is this necessary? ResetSessionUp()...
    m_lastRefreshedULDisplay = ::GetTickCount();
//>>> WiZaRd::More GPLEvilDoers
    m_bGPLEvilDoerNick = false;
    m_bGPLEvilDoerMod = false;
//<<< WiZaRd::More GPLEvilDoers
    m_bHelloAnswerPending = false;
    m_fNoViewSharedFiles = 0;
    m_bMultiPacket = 0;
    md4clr(requpfileid);
    m_nTotalUDPPackets = 0;
    m_nFailedUDPPackets = 0;
    m_nUrlStartPos = (uint64)-1;
    m_fNeedOurPublicIP = 0;
    m_random_update_wait = (UINT)(rand()/(RAND_MAX/1000));
    m_bSourceExchangeSwapped = false; // ZZ:DownloadManager
    m_dwLastTriedToConnect = ::GetTickCount()-20*60*1000; // ZZ:DownloadManager
    m_fQueueRankPending = 0;
    m_fUnaskQueueRankRecv = 0;
    m_fFailedFileIdReqs = 0;
    m_slotNumber = 0;
    lastSwapForSourceExchangeTick = 0;
    m_pReqFileAICHHash = NULL;
    m_fSupportsAICH = 0;
    m_fAICHRequested = 0;
    m_byKadVersion = 0;
    SetLastBuddyPingPongTime();
    m_fSentOutOfPartReqs = 0;
    m_fSupportsLargeFiles = 0;
    m_fExtMultiPacket = 0;
    m_fRequestsCryptLayer = 0;
    m_fSupportsCryptLayer = 0;
    m_fRequiresCryptLayer = 0;
    m_fSupportsSourceEx2 = 0;
    m_fSupportsCaptcha = 0;
    m_fDirectUDPCallback = 0;
    m_cCaptchasSent = 0;
    m_fSupportsFileIdent = 0;
}

CUpDownClient::~CUpDownClient()
{
//>>> WiZaRd::ICS [enkeyDEV]
    delete[] m_abyIncPartStatus;
    m_abyIncPartStatus = NULL;
//<<< WiZaRd::ICS [enkeyDEV]
//>>> WiZaRd::AntiHideOS [netfinity]
    delete[] m_abySeenPartStatus;
    m_abySeenPartStatus = NULL;
//<<< WiZaRd::AntiHideOS [netfinity]
//>>> WiZaRd::Intelligent SOTN
    delete[] m_abyUpPartStatusHidden;
    m_abyUpPartStatusHidden = NULL;
//<<< WiZaRd::Intelligent SOTN
//>>> WiZaRd::ClientAnalyzer
    if (pAntiLeechData)
        theApp.antileechlist->ResetParent(this);
//<<< WiZaRd::ClientAnalyzer

    if (IsAICHReqPending())
    {
        m_fAICHRequested = FALSE;
        CAICHRecoveryHashSet::ClientAICHRequestFailed(this);
    }

    if (GetFriend() != NULL)
    {
        if (GetFriend()->IsTryingToConnect())
            GetFriend()->UpdateFriendConnectionState(FCR_DELETED);
        m_Friend->SetLinkedClient(NULL);
    }
    ASSERT(m_nConnectingState == CCS_NONE || !theApp.emuledlg->IsRunning());
    theApp.clientlist->RemoveClient(this, L"Destructing client object");

    if (socket)
    {
        socket->client = 0;
        socket->Safe_Delete();
    }

    free(m_pszUsername);

//>>> WiZaRd::ICS [enkeyDEV]
    delete[] m_abyIncPartStatus;
    m_abyIncPartStatus = NULL;
//<<< WiZaRd::ICS [enkeyDEV]

//>>> WiZaRd::AntiHideOS [netfinity]
    delete[] m_abySeenPartStatus;
    m_abySeenPartStatus = NULL;
//<<< WiZaRd::AntiHideOS [netfinity]

    delete[] m_abyPartStatus;
    m_abyPartStatus = NULL;

    delete[] m_abyUpPartStatus;
    m_abyUpPartStatus = NULL;

    ClearUploadBlockRequests();

    for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition(); pos != 0;)
        delete m_DownloadBlocks_list.GetNext(pos);

    for (POSITION pos = m_RequestedFiles_list.GetHeadPosition(); pos != 0;)
        delete m_RequestedFiles_list.GetNext(pos);

    for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != 0;)
    {
        Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
        delete pending->block;
        // Not always allocated
        if (pending->zStream)
        {
            inflateEnd(pending->zStream);
            delete pending->zStream;
        }
        delete pending;
    }

    for (POSITION pos = m_WaitingPackets_list.GetHeadPosition(); pos != 0;)
        delete m_WaitingPackets_list.GetNext(pos);

    DEBUG_ONLY(theApp.listensocket->Debug_ClientDeleted(this));
    SetUploadFileID(NULL);

    m_fileReaskTimes.RemoveAll(); // ZZ:DownloadManager (one resk timestamp for each file)

    delete m_pReqFileAICHHash;
}

void CUpDownClient::ClearHelloProperties()
{
    m_uiRealVersion = 0; //>>> WiZaRd::ClientAnalyzer
    m_nUDPPort = 0;
    m_byUDPVer = 0;
    m_byDataCompVer = 0;
    m_byEmuleVersion = 0;
    m_bySourceExchange1Ver = 0;
    m_byAcceptCommentVer = 0;
    m_byExtendedRequestsVer = 0;
    m_byCompatibleClient = 0;
    m_nKadPort = 0;
    m_bySupportSecIdent = 0;
    m_fSupportsPreview = 0;
    m_nClientVersion = 0;
    m_fSharedDirectories = 0;
    m_bMultiPacket = 0;
    m_byKadVersion = 0;
    m_fSupportsLargeFiles = 0;
    m_fExtMultiPacket = 0;
    m_fRequestsCryptLayer = 0;
    m_fSupportsCryptLayer = 0;
    m_fRequiresCryptLayer = 0;
    m_fSupportsSourceEx2 = 0;
    m_fSupportsCaptcha = 0;
    m_fDirectUDPCallback = 0;
    m_fSupportsFileIdent = 0;
    m_strModVersion.Empty(); //>>> WiZaRd::Missing code?
    m_incompletepartVer = 0; //>>> WiZaRd::ICS [enkeyDEV]
}

bool CUpDownClient::ProcessHelloPacket(const uchar* pachPacket, UINT nSize)
{
    CSafeMemFile data(pachPacket, nSize);
    data.ReadUInt8(); // read size of userhash
    // reset all client properties; a client may not send a particular emule tag any longer
    ClearHelloProperties();
    return ProcessHelloTypePacket(&data);
}

bool CUpDownClient::ProcessHelloAnswer(const uchar* pachPacket, UINT nSize)
{
    CSafeMemFile data(pachPacket, nSize);
    bool bIsMule = ProcessHelloTypePacket(&data);
    m_bHelloAnswerPending = false;
    return bIsMule;
}

bool CUpDownClient::ProcessHelloTypePacket(CSafeMemFile* data)
{
    bool bDbgInfo = thePrefs.GetUseDebugDevice();
    m_strHelloInfo.Empty();
    // clear hello properties which can be changed _only_ on receiving OP_Hello/OP_HelloAnswer
    m_bIsHybrid = false;
    m_bIsML = false;
    m_fNoViewSharedFiles = 0;
    m_bUnicodeSupport = false;

    data->ReadHash16(m_achUserHash);
    if (bDbgInfo)
        m_strHelloInfo.AppendFormat(L"Hash=%s (%s)", md4str(m_achUserHash), DbgGetHashTypeString(m_achUserHash));
    m_nUserIDHybrid = data->ReadUInt32();
    if (bDbgInfo)
        m_strHelloInfo.AppendFormat(L"  UserID=%u (%s)", m_nUserIDHybrid, ipstr(m_nUserIDHybrid));
    uint16 nUserPort = data->ReadUInt16(); // hmm clientport is sent twice - why?
    if (bDbgInfo)
        m_strHelloInfo.AppendFormat(L"  Port=%u", nUserPort);

    DWORD dwEmuleTags = 0;
    bool bPrTag = false;
    UINT tagcount = data->ReadUInt32();
    if (bDbgInfo)
        m_strHelloInfo.AppendFormat(L"  Tags=%u", tagcount);

//>>> WiZaRd::ClientAnalyzer
    bool bNick = false;
    bool bMod = false;
//>>> zz_fly::Bad Shareaza detection
    bool bWasUDPPortSent = false;
    bool bIsBadShareaza = false;
//<<< zz_fly::Bad Shareaza detection
//<<< WiZaRd::ClientAnalyzer
    for (UINT i = 0; i < tagcount; i++)
    {
        CTag temptag(data, true);
        switch (temptag.GetNameID())
        {
        case CT_NAME:
            if (temptag.IsStr())
            {
                free(m_pszUsername);
                m_pszUsername = _tcsdup(temptag.GetStr());
                if (bDbgInfo)
                {
                    if (m_pszUsername)  //filter username for bad chars
                    {
                        TCHAR* psz = m_pszUsername;
                        while (*psz != L'\0')
                        {
                            if (*psz == L'\n' || *psz == L'\r')
                                *psz = L' ';
                            psz++;
                        }
                    }
                    m_strHelloInfo.AppendFormat(L"\n  Name='%s'", m_pszUsername);
                }
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            bNick = true; //>>> WiZaRd::ClientAnalyzer
            break;

        case CT_VERSION:
            if (temptag.IsInt())
            {
                if (bDbgInfo)
                    m_strHelloInfo.AppendFormat(L"\n  Version=%u", temptag.GetInt());
                m_nClientVersion = temptag.GetInt();
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case CT_PORT:
            if (temptag.IsInt())
            {
                if (bDbgInfo)
                    m_strHelloInfo.AppendFormat(L"\n  Port=%u", temptag.GetInt());
                nUserPort = (uint16)temptag.GetInt();
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case CT_MOD_VERSION:
            if (temptag.IsStr())
                m_strModVersion = temptag.GetStr();
            else if (temptag.IsInt())
                m_strModVersion.Format(L"ModID=%u", temptag.GetInt());
            else
                m_strModVersion = L"ModID=<Unknown>";
            if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ModID=%s", m_strModVersion);
            bMod = true; //>>> WiZaRd::ClientAnalyzer
            break;

        case CT_EMULE_UDPPORTS:
            // 16 KAD Port
            // 16 UDP Port
            if (temptag.IsInt())
            {
                m_nKadPort = (uint16)(temptag.GetInt() >> 16);
                m_nUDPPort = (uint16)temptag.GetInt();
                if (bDbgInfo)
                    m_strHelloInfo.AppendFormat(L"\n  KadPort=%u  UDPPort=%u", m_nKadPort, m_nUDPPort);
                dwEmuleTags |= 1;
                bWasUDPPortSent = true; //>>> WiZaRd::ClientAnalyzer //>>> zz_fly::Bad Shareaza detection
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case CT_EMULE_BUDDYUDP:
            // 16 --Reserved for future use--
            // 16 BUDDY Port
            if (temptag.IsInt())
            {
                m_nBuddyPort = (uint16)temptag.GetInt();
                if (bDbgInfo)
                    m_strHelloInfo.AppendFormat(L"\n  BuddyPort=%u", m_nBuddyPort);
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case CT_EMULE_BUDDYIP:
            // 32 BUDDY IP
            if (temptag.IsInt())
            {
                m_nBuddyIP = temptag.GetInt();
                if (bDbgInfo)
                    m_strHelloInfo.AppendFormat(L"\n  BuddyIP=%s", ipstr(m_nBuddyIP));
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case CT_EMULE_MISCOPTIONS1:
            //  3 AICH Version (0 = not supported)
            //  1 Unicode
            //  4 UDP version
            //  4 Data compression version
            //  4 Secure Ident
            //  4 Source Exchange - deprecated
            //  4 Ext. Requests
            //  4 Comments
            //	1 PeerChache supported
            //	1 No 'View Shared Files' supported
            //	1 MultiPacket - deprecated with FileIdentifiers/MultipacketExt2
            //  1 Preview
            if (temptag.IsInt())
            {
                m_fSupportsAICH			= (temptag.GetInt() >> 29) & 0x07;
                m_bUnicodeSupport		= (temptag.GetInt() >> 28) & 0x01;
                m_byUDPVer				= (uint8)((temptag.GetInt() >> 24) & 0x0f);
                m_byDataCompVer			= (uint8)((temptag.GetInt() >> 20) & 0x0f);
                m_bySupportSecIdent		= (uint8)((temptag.GetInt() >> 16) & 0x0f);
                m_bySourceExchange1Ver	= (uint8)((temptag.GetInt() >> 12) & 0x0f);
                m_byExtendedRequestsVer	= (uint8)((temptag.GetInt() >>  8) & 0x0f);
                m_byAcceptCommentVer	= (uint8)((temptag.GetInt() >>  4) & 0x0f);
                UINT m_fPeerCache		= (temptag.GetInt() >>  3) & 0x01;
                m_fNoViewSharedFiles	= (temptag.GetInt() >>  2) & 0x01;
                m_bMultiPacket			= (temptag.GetInt() >>  1) & 0x01;
                m_fSupportsPreview		= (temptag.GetInt() >>  0) & 0x01;
                dwEmuleTags |= 2;
                if (bDbgInfo)
                {
                    m_strHelloInfo.AppendFormat(L"\n  PeerCache=%u  UDPVer=%u  DataComp=%u  SecIdent=%u  SrcExchg=%u"
                                                L"  ExtReq=%u  Commnt=%u  Preview=%u  NoViewFiles=%u  Unicode=%u",
                                                m_fPeerCache, m_byUDPVer, m_byDataCompVer, m_bySupportSecIdent, m_bySourceExchange1Ver,
                                                m_byExtendedRequestsVer, m_byAcceptCommentVer, m_fSupportsPreview, m_fNoViewSharedFiles, m_bUnicodeSupport);
                }
//>>> WiZaRd::ClientAnalyzer
//>>> zz_fly::Bad Shareaza detection
                if (!bWasUDPPortSent)
                    bIsBadShareaza = true;
//<<< zz_fly::Bad Shareaza detection
//<<< WiZaRd::ClientAnalyzer
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case CT_EMULE_MISCOPTIONS2:
            //	18 Reserved
            //   1 Supports new FileIdentifiers/MultipacketExt2
            //   1 Direct UDP Callback supported and available
            //	 1 Supports ChatCaptchas
            //	 1 Supports SourceExachnge2 Packets, ignores SX1 Packet Version
            //	 1 Requires CryptLayer
            //	 1 Requests CryptLayer
            //	 1 Supports CryptLayer
            //	 1 Reserved (ModBit)
            //   1 Ext Multipacket (Hash+Size instead of Hash) - deprecated with FileIdentifiers/MultipacketExt2
            //   1 Large Files (includes support for 64bit tags)
            //   4 Kad Version - will go up to version 15 only (may need to add another field at some point in the future)
            if (temptag.IsInt())
            {
                m_fSupportsFileIdent	= (temptag.GetInt() >>  13) & 0x01;
                m_fDirectUDPCallback	= (temptag.GetInt() >>  12) & 0x01;
                m_fSupportsCaptcha	    = (temptag.GetInt() >>  11) & 0x01;
                m_fSupportsSourceEx2	= (temptag.GetInt() >>  10) & 0x01;
                m_fRequiresCryptLayer	= (temptag.GetInt() >>  9) & 0x01;
                m_fRequestsCryptLayer	= (temptag.GetInt() >>  8) & 0x01;
                m_fSupportsCryptLayer	= (temptag.GetInt() >>  7) & 0x01;
                // reserved 1
                m_fExtMultiPacket		= (temptag.GetInt() >>  5) & 0x01;
                m_fSupportsLargeFiles   = (temptag.GetInt() >>  4) & 0x01;
                m_byKadVersion			= (uint8)((temptag.GetInt() >>  0) & 0x0f);
                dwEmuleTags |= 8;
                if (bDbgInfo)
                    m_strHelloInfo.AppendFormat(L"\n  KadVersion=%u, LargeFiles=%u ExtMultiPacket=%u CryptLayerSupport=%u CryptLayerRequest=%u CryptLayerRequires=%u SupportsSourceEx2=%u SupportsCaptcha=%u DirectUDPCallback=%u", m_byKadVersion, m_fSupportsLargeFiles, m_fExtMultiPacket, m_fSupportsCryptLayer, m_fRequestsCryptLayer, m_fRequiresCryptLayer, m_fSupportsSourceEx2, m_fSupportsCaptcha, m_fDirectUDPCallback);
                m_fRequestsCryptLayer &= m_fSupportsCryptLayer;
                m_fRequiresCryptLayer &= m_fRequestsCryptLayer;
//>>> WiZaRd::ClientAnalyzer
//>>> zz_fly::Bad Shareaza detection
                if (!bWasUDPPortSent)
                    bIsBadShareaza = true;
//<<< zz_fly::Bad Shareaza detection
//<<< WiZaRd::ClientAnalyzer
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case CT_EMULE_VERSION:
            //  8 Compatible Client ID
            //  7 Mjr Version (Doesn't really matter..)
            //  7 Min Version (Only need 0-99)
            //  3 Upd Version (Only need 0-5)
            //  7 Bld Version (Only need 0-99) -- currently not used
            if (temptag.IsInt())
            {
                m_byCompatibleClient = (uint8)((temptag.GetInt() >> 24));
                m_uiRealVersion = temptag.GetInt(); //>>> WiZaRd::ClientAnalyzer
                m_nClientVersion = temptag.GetInt() & 0x00ffffff;
                m_byEmuleVersion = 0x99;
                m_fSharedDirectories = 1;
                dwEmuleTags |= 4;
                if (bDbgInfo)
                    m_strHelloInfo.AppendFormat(L"\n  ClientVer=%u.%u.%u.%u  Comptbl=%u", (m_nClientVersion >> 17) & 0x7f, (m_nClientVersion >> 10) & 0x7f, (m_nClientVersion >> 7) & 0x07, m_nClientVersion & 0x7f, m_byCompatibleClient);
            }
            else if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

//>>> WiZaRd::ICS [enkeyDEV]
        case ET_INCOMPLETEPARTS:
            if (temptag.IsInt())
            {
                m_incompletepartVer = (uint8)temptag.GetInt();
                if (bDbgInfo)
                    m_strHelloInfo.AppendFormat(L"\n  IncVer=%u", m_incompletepartVer);
            }
            break;
            //flow over
//<<< WiZaRd::ICS [enkeyDEV]

        default:
            // Since eDonkeyHybrid 1.3 is no longer sending the additional Int32 at the end of the Hello packet,
            // we use the "pr=1" tag to determine them.
            if (temptag.GetName() && temptag.GetName()[0]=='p' && temptag.GetName()[1]=='r')
            {
                bPrTag = true;
            }
            if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***UnkTag=%s", temptag.GetFullInfo());
        }
    }
    m_nUserPort = nUserPort;
    m_dwServerIP = data->ReadUInt32();
    m_nServerPort = data->ReadUInt16();
    if (bDbgInfo)
        m_strHelloInfo.AppendFormat(L"\n  Server=%s:%u", ipstr(m_dwServerIP), m_nServerPort);

    // Check for additional data in Hello packet to determine client's software version.
    //
    // *) eDonkeyHybrid 0.40 - 1.2 sends an additional Int32. (Since 1.3 they don't send it any longer.)
    // *) MLdonkey sends an additional Int32
    //
    if (data->GetLength() - data->GetPosition() == sizeof(UINT))
    {
        UINT test = data->ReadUInt32();
        if (test == 'KDLM')
        {
            m_bIsML = true;
            if (bDbgInfo)
                m_strHelloInfo += L"\n  ***AddData: \"MLDK\"";
        }
        else
        {
            m_bIsHybrid = true;
            if (bDbgInfo)
                m_strHelloInfo.AppendFormat(L"\n  ***AddData: UINT=%u (0x%08x)", test, test);
        }
    }
    else if (bDbgInfo && data->GetPosition() < data->GetLength())
    {
        UINT uAddHelloDataSize = (UINT)(data->GetLength() - data->GetPosition());
        if (uAddHelloDataSize == sizeof(UINT))
        {
            DWORD dwAddHelloInt32 = data->ReadUInt32();
            m_strHelloInfo.AppendFormat(L"\n  ***AddData: UINT=%u (0x%08x)", dwAddHelloInt32, dwAddHelloInt32);
        }
        else if (uAddHelloDataSize == sizeof(UINT)+sizeof(uint16))
        {
            DWORD dwAddHelloInt32 = data->ReadUInt32();
            WORD w = data->ReadUInt16();
            m_strHelloInfo.AppendFormat(L"\n  ***AddData: UINT=%u (0x%08x),  uint16=%u (0x%04x)", dwAddHelloInt32, dwAddHelloInt32, w, w);
        }
        else
            m_strHelloInfo.AppendFormat(L"\n  ***AddData: %u bytes", uAddHelloDataSize);
    }

    SOCKADDR_IN sockAddr = {0};
    int nSockAddrLen = sizeof(sockAddr);
    socket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
    SetIP(sockAddr.sin_addr.S_un.S_addr);

    //(a)If this is a highID user, store the ID in the Hybrid format.
    //(b)Some older clients will not send a ID, these client are HighID users that are not connected to a server.
    //(c)Kad users with a *.*.*.0 IPs will look like a lowID user they are actually a highID user.. They can be detected easily
    //because they will send a ID that is the same as their IP..
    if (!HasLowID() || m_nUserIDHybrid == 0 || m_nUserIDHybrid == m_dwUserIP)
        m_nUserIDHybrid = ntohl(m_dwUserIP);

    CClientCredits* pFoundCredits = theApp.clientcredits->GetCredit(m_achUserHash);
    if (credits == NULL)
    {
        credits = pFoundCredits;
        if (!theApp.clientlist->ComparePriorUserhash(m_dwUserIP, m_nUserPort, pFoundCredits))
        {
            if (thePrefs.GetLogBannedClients())
                AddDebugLogLine(false, L"Clients: %s (%s), Banreason: Userhash changed (Found in TrackedClientsList)", GetUserName(), ipstr(GetConnectIP()));
            Ban();
        }
    }
    else if (credits != pFoundCredits)
    {
        // userhash change ok, however two hours "waittime" before it can be used
        credits = pFoundCredits;
        if (thePrefs.GetLogBannedClients())
            AddDebugLogLine(false, L"Clients: %s (%s), Banreason: Userhash changed", GetUserName(), ipstr(GetConnectIP()));
        Ban();
    }


    if (GetFriend() != NULL && GetFriend()->HasUserhash() && md4cmp(GetFriend()->m_abyUserhash, m_achUserHash) != 0)
    {
        // this isnt our friend anymore and it will be removed/replaced, tell our friendobject about it
        if (GetFriend()->IsTryingToConnect())
            GetFriend()->UpdateFriendConnectionState(FCR_USERHASHFAILED); // this will remove our linked friend
        else
            GetFriend()->SetLinkedClient(NULL);
    }
    // do not replace friendobjects which have no userhash, but the fitting ip with another friend object with the
    // fitting userhash (both objects would fit to this instance), as this could lead to unwanted results
    if (GetFriend() == NULL || GetFriend()->HasUserhash() || GetFriend()->m_dwLastUsedIP != GetConnectIP()
            || GetFriend()->m_nLastUsedPort != GetUserPort())
    {
        if ((m_Friend = theApp.friendlist->SearchFriend(m_achUserHash, m_nConnectIP, m_nUserPort)) != NULL)
        {
            // Link the friend to that client
            m_Friend->SetLinkedClient(this);
        }
        else
        {
            // avoid that an unwanted client instance keeps a friend slot
            SetFriendSlot(false);
        }
    }
    else
    {
        // however, copy over our userhash in this case
        md4cpy(GetFriend()->m_abyUserhash, m_achUserHash);
    }

    m_byInfopacketsReceived |= IP_EDONKEYPROTPACK;
    // check if at least CT_EMULEVERSION was received, all other tags are optional
    bool bIsMule = (dwEmuleTags & 0x04) == 0x04;
    if (bIsMule)
    {
        m_bEmuleProtocol = true;
        m_byInfopacketsReceived |= IP_EMULEPROTPACK;
    }
    else if (bPrTag)
    {
        m_bIsHybrid = true;
    }

    InitClientSoftwareVersion();

//>>> WiZaRd::ClientAnalyzer
    //UserHash changes are already taken care of - so we concentrate on the *real* deal :)
//	pAntiLeechData = theApp.antileechlist->GetData(m_achUserHash);
    theApp.antileechlist->SetParent(this);
    ASSERT(pAntiLeechData != NULL); //GetData() *should* always return a valid struct... if not, somethings really wrong!
    ASSERT(pAntiLeechData->GetParent() == this); //make sure the cross-pointers are valid!
    if (theApp.antileechlist->IsCorruptPartSender(m_achUserHash)) //store corrupt part sender info
    {
        theApp.clientlist->AddTrackClient(this);
        Ban(L"Identified as sender of corrupt data");
    }
    if (pAntiLeechData)
    {
//		pAntiLeechData->SetParent(this);
        //be careful! Call this AFTER InitClientSoftwareVersion() - otherwise it won't work correctly!
        if (bNick)
            pAntiLeechData->Check4NickThief();
        m_bIsBadClient = false; //>>> WiZaRd::BadClientFlag
        if (bMod)
        {
            pAntiLeechData->Check4ModThief();
//>>> WiZaRd::BadClientFlag
            if (//just in case... keep those checks
                _tcscmp(m_strModVersion, L"Xtreme") == 0
                || _tcscmp(m_strModVersion, L"DreaMule ") == 0		//>>> Spike2: DreaMule is based on Xtreme
//				|| _tcscmp(m_strModVersion, L"ScarAngel") == 0		//>>> Spike2: ...and "Mulle's" ScarAngel, too !
//				|| _tcscmp(m_strModVersion, L"Mephisto v") == 0		//>>> WiZaRd: ...same goes for Mephisto...
            )
                m_bIsBadClient = true;
//<<< WiZaRd::BadClientFlag
        }
        pAntiLeechData->Check4ModFaker(bIsBadShareaza);
    }
    CheckForGPLEvilDoer(bNick, bMod); //>>> WiZaRd::More GPLEvilDoers	
//<<< WiZaRd::ClientAnalyzer
	CheckModIconIndex(); //>>> WiZaRd::ModIconMapper

    if (m_bIsHybrid)
        m_fSharedDirectories = 1;

    if (thePrefs.GetVerbose() && GetServerIP() == INADDR_NONE)
        AddDebugLogLine(false, L"Received invalid server IP %s from %s", ipstr(GetServerIP()), DbgGetClientInfo());

    return bIsMule;
}

void CUpDownClient::SendHelloPacket()
{
    if (socket == NULL)
    {
        ASSERT(0);
        return;
    }

    CSafeMemFile data(128);
    data.WriteUInt8(16); // size of userhash
    SendHelloTypePacket(&data);
    Packet* packet = new Packet(&data);
    packet->opcode = OP_HELLO;
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__Hello", this);
    theStats.AddUpDataOverheadOther(packet->size);
    SendPacket(packet, true);

    m_bHelloAnswerPending = true;
    return;
}

void CUpDownClient::SendMuleInfoPacket(bool bAnswer)
{
    if (socket == NULL)
    {
        ASSERT(0);
        return;
    }

    CSafeMemFile data(128);
    data.WriteUInt8((uint8)theApp.m_uCurVersionShort);
    data.WriteUInt8(EMULE_PROTOCOL);
    const bool bSend = !m_pszUsername || !m_strModVersion.IsEmpty(); //>>> WiZaRd::Easy ModVersion
    const bool bSendICS = !m_pszUsername || GetIncompletePartVersion(); //>>> WiZaRd::ICS [enkeyDEV]
    data.WriteUInt32(7+bSend+bSendICS); // nr. of tags //>>> WiZaRd::Easy ModVersion //>>> WiZaRd::ICS [enkeyDEV]
    CTag tag(ET_COMPRESSION,1);
    tag.WriteTagToFile(&data);
    CTag tag2(ET_UDPVER,4);
    tag2.WriteTagToFile(&data);
    CTag tag3(ET_UDPPORT,thePrefs.GetUDPPort());
    tag3.WriteTagToFile(&data);
    CTag tag4(ET_SOURCEEXCHANGE,3);
    tag4.WriteTagToFile(&data);
    CTag tag5(ET_COMMENTS,1);
    tag5.WriteTagToFile(&data);
    CTag tag6(ET_EXTENDEDREQUEST,2);
    tag6.WriteTagToFile(&data);

    UINT dwTagValue = (theApp.clientcredits->CryptoAvailable() ? 3 : 0);
    if (thePrefs.CanSeeShares() != vsfaNobody) // set 'Preview supported' only if 'View Shared Files' allowed
        dwTagValue |= 128;
    CTag tag7(ET_FEATURES, dwTagValue);
    tag7.WriteTagToFile(&data);

//>>> WiZaRd::Easy ModVersion
    if (bSend)
    {
        CTag tagModVersion(ET_MOD_VERSION, MOD_VERSION);
        tagModVersion.WriteTagToFile(&data);
    }
//<<< WiZaRd::Easy ModVersion
//>>> WiZaRd::ICS [enkeyDEV]
    if (bSendICS)
    {
        //we support *only* the "real-deal" v1 - no neo mule fuss
        //though we keep it compatible so we might be able to update it in future
        CTag tagICS(ET_INCOMPLETEPARTS, 1);
        tagICS.WriteTagToFile(&data);
    }
//<<< WiZaRd::ICS [enkeyDEV]

    Packet* packet = new Packet(&data,OP_EMULEPROT);
    if (!bAnswer)
        packet->opcode = OP_EMULEINFO;
    else
        packet->opcode = OP_EMULEINFOANSWER;
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend(!bAnswer ? "OP__EmuleInfo" : "OP__EmuleInfoAnswer", this);
    theStats.AddUpDataOverheadOther(packet->size);
    SendPacket(packet, true);
}

void CUpDownClient::ProcessMuleInfoPacket(const uchar* pachPacket, UINT nSize)
{
    bool bDbgInfo = thePrefs.GetUseDebugDevice();
    m_strMuleInfo.Empty();
    CSafeMemFile data(pachPacket, nSize);
    m_byCompatibleClient = 0;
    m_byEmuleVersion = data.ReadUInt8();
    if (bDbgInfo)
        m_strMuleInfo.AppendFormat(L"EmuleVer=0x%x", (UINT)m_byEmuleVersion);
    if (m_byEmuleVersion == 0x2B)
        m_byEmuleVersion = 0x22;
    uint8 protversion = data.ReadUInt8();
    if (bDbgInfo)
        m_strMuleInfo.AppendFormat(L"  ProtVer=%u", (UINT)protversion);

    m_incompletepartVer = 0;	//>>> WiZaRd::ICS [enkeyDEV]

    //implicitly supported options by older clients
    if (protversion == EMULE_PROTOCOL)
    {
        //in the future do not use version to guess about new features

        if (m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x22)
            m_byUDPVer = 1;

        if (m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x21)
            m_bySourceExchange1Ver = 1;

        if (m_byEmuleVersion == 0x24)
            m_byAcceptCommentVer = 1;

        // Shared directories are requested from eMule 0.28+ because eMule 0.27 has a bug in
        // the OP_ASKSHAREDFILESDIR handler, which does not return the shared files for a
        // directory which has a trailing backslash.
        if (m_byEmuleVersion >= 0x28 && !m_bIsML) // MLdonkey currently does not support shared directories
            m_fSharedDirectories = 1;

    }
    else
    {
        return;
    }

    UINT tagcount = data.ReadUInt32();
    if (bDbgInfo)
        m_strMuleInfo.AppendFormat(L"  Tags=%u", (UINT)tagcount);

    bool bMod = false; //>>> WiZaRd::ClientAnalyzer
    for (UINT i = 0; i < tagcount; i++)
    {
        CTag temptag(&data, false);
        switch (temptag.GetNameID())
        {
        case ET_COMPRESSION:
            // Bits 31- 8: 0 - reserved
            // Bits  7- 0: data compression version
            if (temptag.IsInt())
            {
                m_byDataCompVer = (uint8)temptag.GetInt();
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  Compr=%u", (UINT)temptag.GetInt());
            }
            else if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case ET_UDPPORT:
            // Bits 31-16: 0 - reserved
            // Bits 15- 0: UDP port
            if (temptag.IsInt())
            {
                m_nUDPPort = (uint16)temptag.GetInt();
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  UDPPort=%u", (UINT)temptag.GetInt());
            }
            else if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case ET_UDPVER:
            // Bits 31- 8: 0 - reserved
            // Bits  7- 0: UDP protocol version
            if (temptag.IsInt())
            {
                m_byUDPVer = (uint8)temptag.GetInt();
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  UDPVer=%u", (UINT)temptag.GetInt());
            }
            else if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case ET_SOURCEEXCHANGE:
            // Bits 31- 8: 0 - reserved
            // Bits  7- 0: source exchange protocol version
            if (temptag.IsInt())
            {
                m_bySourceExchange1Ver = (uint8)temptag.GetInt();
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  SrcExch=%u", (UINT)temptag.GetInt());
            }
            else if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case ET_COMMENTS:
            // Bits 31- 8: 0 - reserved
            // Bits  7- 0: comments version
            if (temptag.IsInt())
            {
                m_byAcceptCommentVer = (uint8)temptag.GetInt();
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  Commnts=%u", (UINT)temptag.GetInt());
            }
            else if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case ET_EXTENDEDREQUEST:
            // Bits 31- 8: 0 - reserved
            // Bits  7- 0: extended requests version
            if (temptag.IsInt())
            {
                m_byExtendedRequestsVer = (uint8)temptag.GetInt();
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  ExtReq=%u", (UINT)temptag.GetInt());
            }
            else if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case ET_COMPATIBLECLIENT:
            // Bits 31- 8: 0 - reserved
            // Bits  7- 0: compatible client ID
            if (temptag.IsInt())
            {
                m_byCompatibleClient = (uint8)temptag.GetInt();
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  Comptbl=%u", (UINT)temptag.GetInt());
            }
            else if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case ET_FEATURES:
            // Bits 31- 8: 0 - reserved
            // Bit	    7: Preview
            // Bit   6- 0: secure identification
            if (temptag.IsInt())
            {
                m_bySupportSecIdent = (uint8)((temptag.GetInt()) & 3);
                m_fSupportsPreview  = (temptag.GetInt() >> 7) & 1;
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  SecIdent=%u  Preview=%u", m_bySupportSecIdent, m_fSupportsPreview);
            }
            else if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
            break;

        case ET_MOD_VERSION:
            if (temptag.IsStr())
                m_strModVersion = temptag.GetStr();
            else if (temptag.IsInt())
                m_strModVersion.Format(L"ModID=%u", temptag.GetInt());
            else
                m_strModVersion = L"ModID=<Unknwon>";
            if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ModID=%s", m_strModVersion);
            bMod = true; //>>> WiZaRd::ClientAnalyzer
            break;

//>>> WiZaRd::ICS [enkeyDEV]
        case ET_INCOMPLETEPARTS:
            if (temptag.IsInt())
            {
                m_incompletepartVer = (uint8)temptag.GetInt();
                if (bDbgInfo)
                    m_strMuleInfo.AppendFormat(L"\n  IncVer=%u", m_incompletepartVer);
            }
            break;
            //flow over
//<<< WiZaRd::ICS [enkeyDEV]

        default:
            if (bDbgInfo)
                m_strMuleInfo.AppendFormat(L"\n  ***UnkTag=%s", temptag.GetFullInfo());
        }
    }
    if (m_byDataCompVer == 0)
    {
        m_bySourceExchange1Ver = 0;
        m_byExtendedRequestsVer = 0;
        m_byAcceptCommentVer = 0;
        m_nUDPPort = 0;
        m_incompletepartVer = 0;	//>>> WiZaRd::ICS [enkeyDEV] //hmmm why is this reset?
    }
    if (bDbgInfo && data.GetPosition() < data.GetLength())
    {
        m_strMuleInfo.AppendFormat(L"\n  ***AddData: %u bytes", data.GetLength() - data.GetPosition());
    }

    m_bEmuleProtocol = true;
    m_byInfopacketsReceived |= IP_EMULEPROTPACK;
    InitClientSoftwareVersion();

//>>> WiZaRd::ClientAnalyzer
    //be careful! Call this AFTER InitClientSoftwareVersion() - otherwise it won't work correctly!
    m_bIsBadClient = false; //>>> WiZaRd::BadClientFlag
    if (bMod)
    {
        if (pAntiLeechData)
            pAntiLeechData->Check4ModThief();
//>>> WiZaRd::BadClientFlag
#define XTREME_ADDON L" �" + m_strModVersion + L"�"
        //to find all Xtreme based clients it's easier to search for their "anti-modthief"
        CString nick(m_pszUsername);
        if (nick.Find(XTREME_ADDON)
#undef XTREME_ADDON

                //just in case... keep those checks
                || _tcscmp(m_strModVersion, L"Xtreme") == 0
                || _tcscmp(m_strModVersion, L"DreaMule ") == 0		//>>> Spike2: DreaMule is based on Xtreme
//			|| _tcscmp(m_strModVersion, L"ScarAngel") == 0		//>>> Spike2: ...and "Mulle's" ScarAngel, too !
//			|| _tcscmp(m_strModVersion, L"Mephisto v") == 0		//>>> WiZaRd: ...same goes for Mephisto...
           )
            m_bIsBadClient = true;
//<<< WiZaRd::BadClientFlag
//could also be called here to perform the basic checks for proper modstring schemes though we should not receive that packet anymore, anyways!?
//		pAntiLeechData->Check4ModFaker();
//>>> WiZaRd::More GPLEvilDoers
        CheckForGPLEvilDoer(false, true);
//<<< WiZaRd::More GPLEvilDoers		
    }
//<<< WiZaRd::ClientAnalyzer
	CheckModIconIndex(); //>>> WiZaRd::ModIconMapper

    if (thePrefs.GetVerbose() && GetServerIP() == INADDR_NONE)
        AddDebugLogLine(false, L"Received invalid server IP %s from %s", ipstr(GetServerIP()), DbgGetClientInfo());
}

void CUpDownClient::SendHelloAnswer()
{
    if (socket == NULL)
    {
        ASSERT(0);
        return;
    }

    CSafeMemFile data(128);
    SendHelloTypePacket(&data);
    Packet* packet = new Packet(&data);
    packet->opcode = OP_HELLOANSWER;
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__HelloAnswer", this);
    theStats.AddUpDataOverheadOther(packet->size);

    socket->SendPacket(packet, true, true, 0, false);

    m_bHelloAnswerPending = false;
}

void CUpDownClient::SendHelloTypePacket(CSafeMemFile* data)
{
    data->WriteHash16(thePrefs.GetUserHash());
    UINT clientid;
    clientid = theApp.GetID();

    data->WriteUInt32(clientid);
    data->WriteUInt16(thePrefs.GetPort());

    UINT tagcount = 6;

    if (theApp.clientlist->GetBuddy() && theApp.IsFirewalled())
        tagcount += 2;

//>>> WiZaRd::Easy ModVersion
    const bool bSend = !m_pszUsername || !m_strModVersion.IsEmpty();
    if (bSend)
        ++tagcount;
//<<< WiZaRd::Easy ModVersion
//>>> WiZaRd::ICS [enkeyDev]
    const bool bSendICS = !m_pszUsername || GetIncompletePartVersion();
    if (bSendICS)
        ++tagcount;
//<<< WiZaRd::ICS [enkeyDev]

    data->WriteUInt32(tagcount);

    // eD2K Name

    // TODO implement multi language website which informs users of the effects of bad mods
//>>> WiZaRd::ClientAnalyzer
    if (m_bGPLEvilDoerNick || m_bGPLEvilDoerMod)
    {
        CTag tagName(CT_NAME, L"Please use a GPL-conform version of eMule");
        tagName.WriteTagToFile(data, utf8strRaw);
    }
    //WiZaRd:
    //according to a report by jerryBG there are some mods out there that punish nickname changes and thus a CA client might run into
    //problems while chatting with such a bad mod because the antinickthiefnick changes (a bit) on every connect/hello.
    //Thus, as a workaround, we will keep the set nickname from our preferences during every chat session to avoid getting
    //banned/punished in an unfair way. Though that solution isn't perfect it's just a simple workaround until the mods fix/remove
    //their code.
    //Just ONE simple reason why such a punishment doesn't make sense/is unfair:
    //EVERY client might change his name as many times as he wants. That's possible with EVERY eMule based client which is sending the
    //CT_NAME tag! Though it MIGHT not happen in reality that someone changes his nickname a dozen times in a short period of time it is
    //still POSSIBLE and LEGAL and thus ANY action against such a client is (IMHO) a rule violation.
    else if (GetChatState() != MS_NONE)
    {
        CTag tagName(CT_NAME, thePrefs.GetUserNick());
        tagName.WriteTagToFile(data, utf8strRaw);
    }
    else
    {
        CTag tagName(CT_NAME, CAntiLeechData::GetAntiNickThiefNick());
        tagName.WriteTagToFile(data, utf8strRaw);
    }
//<<< WiZaRd::ClientAnalyzer

    // eD2K Version
    CTag tagVersion(CT_VERSION,EDONKEYVERSION);
    tagVersion.WriteTagToFile(data);

    // eMule UDP Ports
    UINT kadUDPPort = 0;
    if (Kademlia::CKademlia::IsConnected())
    {
        if (Kademlia::CKademlia::GetPrefs()->GetExternalKadPort() != 0
                && Kademlia::CKademlia::GetPrefs()->GetUseExternKadPort()
                && Kademlia::CUDPFirewallTester::IsVerified())
        {
            kadUDPPort = Kademlia::CKademlia::GetPrefs()->GetExternalKadPort();
        }
        else
            kadUDPPort = Kademlia::CKademlia::GetPrefs()->GetInternKadPort();
    }
    CTag tagUdpPorts(CT_EMULE_UDPPORTS,
                     ((UINT)kadUDPPort			   << 16) |
                     ((UINT)thePrefs.GetUDPPort() <<  0)
                    );
    tagUdpPorts.WriteTagToFile(data);

    if (theApp.clientlist->GetBuddy() && theApp.IsFirewalled())
    {
        CTag tagBuddyIP(CT_EMULE_BUDDYIP, theApp.clientlist->GetBuddy()->GetIP());
        tagBuddyIP.WriteTagToFile(data);

        CTag tagBuddyPort(CT_EMULE_BUDDYUDP,
//					( RESERVED												)
                          ((UINT)theApp.clientlist->GetBuddy()->GetUDPPort())
                         );
        tagBuddyPort.WriteTagToFile(data);
    }

    // eMule Misc. Options #1
    const UINT uUdpVer				= 4;
    const UINT uDataCompVer			= 1;
    const UINT uSupportSecIdent		= theApp.clientcredits->CryptoAvailable() ? 3 : 0;
    // ***
    // deprecated - will be set back to 3 with the next release (to allow the new version to spread first),
    // due to a bug in earlier eMule version. Use SupportsSourceEx2 and new opcodes instead
    const UINT uSourceExchange1Ver	= 4;
    // ***
    const UINT uExtendedRequestsVer	= 2;
    const UINT uAcceptCommentVer	= 1;
    const UINT uNoViewSharedFiles	= (thePrefs.CanSeeShares() == vsfaNobody) ? 1 : 0; // for backward compatibility this has to be a 'negative' flag
    const UINT uMultiPacket			= 1;
    const UINT uSupportPreview		= (thePrefs.CanSeeShares() != vsfaNobody) ? 1 : 0; // set 'Preview supported' only if 'View Shared Files' allowed
    const UINT uPeerCache			= 1;
    const UINT uUnicodeSupport		= 1;
    const UINT nAICHVer				= 1;
    CTag tagMisOptions1(CT_EMULE_MISCOPTIONS1,
                        (nAICHVer				<< 29) |
                        (uUnicodeSupport		<< 28) |
                        (uUdpVer				<< 24) |
                        (uDataCompVer			<< 20) |
                        (uSupportSecIdent		<< 16) |
                        (uSourceExchange1Ver	<< 12) |
                        (uExtendedRequestsVer	<<  8) |
                        (uAcceptCommentVer		<<  4) |
                        (uPeerCache				<<  3) |
                        (uNoViewSharedFiles		<<  2) |
                        (uMultiPacket			<<  1) |
                        (uSupportPreview		<<  0)
                       );
    tagMisOptions1.WriteTagToFile(data);

    // eMule Misc. Options #2
    const UINT uKadVersion			= KADEMLIA_VERSION;
    const UINT uSupportLargeFiles	= 1;
    const UINT uExtMultiPacket		= 1;
    const UINT uReserved			= 0; // mod bit
    const UINT uSupportsCryptLayer	= 1;
    const UINT uRequestsCryptLayer	= thePrefs.IsClientCryptLayerRequested() ? 1 : 0;
    const UINT uRequiresCryptLayer	= thePrefs.IsClientCryptLayerRequired() ? 1 : 0;
    const UINT uSupportsSourceEx2	= 1;
    const UINT uSupportsCaptcha		= 1;
    // direct callback is only possible if connected to kad, tcp firewalled and verified UDP open (for example on a full cone NAT)
    const UINT uDirectUDPCallback	= (Kademlia::CKademlia::IsRunning() && Kademlia::CKademlia::IsFirewalled()
                                       && !Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) && Kademlia::CUDPFirewallTester::IsVerified()) ? 1 : 0;
    const UINT uFileIdentifiers		= 1;

    CTag tagMisOptions2(CT_EMULE_MISCOPTIONS2,
//				(RESERVED				     )
                        (uFileIdentifiers		<< 13) |
                        (uDirectUDPCallback		<< 12) |
                        (uSupportsCaptcha		<< 11) |
                        (uSupportsSourceEx2		<< 10) |
                        (uRequiresCryptLayer	<<  9) |
                        (uRequestsCryptLayer	<<  8) |
                        (uSupportsCryptLayer	<<  7) |
                        (uReserved				<<  6) |
                        (uExtMultiPacket		<<  5) |
                        (uSupportLargeFiles		<<  4) |
                        (uKadVersion			<<  0)
                       );
    tagMisOptions2.WriteTagToFile(data);

    // eMule Version
    CTag tagMuleVersion(CT_EMULE_VERSION,
                        //(uCompatibleClientID		<< 24) |
                        (CemuleApp::m_nVersionMjr	<< 17) |
                        (CemuleApp::m_nVersionMin	<< 10) |
                        (CemuleApp::m_nVersionUpd	<<  7)
                        | (DBG_VERSION_BUILD) //>>> WiZaRd::ClientAnalyzer
//				(RESERVED			     )
                       );
    tagMuleVersion.WriteTagToFile(data);

//>>> WiZaRd::Easy ModVersion
    if (bSend)
    {
        CTag tagModVersion(CT_MOD_VERSION, MOD_VERSION);
        tagModVersion.WriteTagToFile(data);
    }
//<<< WiZaRd::Easy ModVersion
//>>> WiZaRd::ICS [enkeyDev]
    if (bSendICS)
    {
        //we support *only* the "real-deal" v1 - no neo mule fuss
        //though we keep it compatible so we might be able to update it in future
        CTag tagICS(ET_INCOMPLETEPARTS, 1);
        tagICS.WriteTagToFile(data);
    }
//<<< WiZaRd::ICS [enkeyDev]

    data->WriteUInt32(0);
    data->WriteUInt16(0);
//	data->WriteUInt32(dwIP); //The Hybrid added some bits here, what ARE THEY FOR?
}

void CUpDownClient::ProcessMuleCommentPacket(const uchar* pachPacket, UINT nSize)
{
    if (reqfile && reqfile->IsPartFile())
    {
        CSafeMemFile data(pachPacket, nSize);
        uint8 uRating = data.ReadUInt8();
        if (thePrefs.GetLogRatingDescReceived() && uRating > 0)
            AddDebugLogLine(false, GetResString(IDS_RATINGRECV), m_strClientFilename, uRating);
        CString strComment;
        UINT uLength = data.ReadUInt32();
        if (uLength > 0)
        {
            // we have to increase the raw max. allowed file comment len because of possible UTF8 encoding.
            if (uLength > MAXFILECOMMENTLEN*4)
                uLength = MAXFILECOMMENTLEN*4;
            strComment = data.ReadString(GetUnicodeSupport()!=utf8strNone, uLength);

            if (strComment.GetLength() > MAXFILECOMMENTLEN) // enforce the max len on the comment
                strComment = strComment.Left(MAXFILECOMMENTLEN);

            if (thePrefs.GetLogRatingDescReceived() && !strComment.IsEmpty())
                AddDebugLogLine(false, GetResString(IDS_DESCRIPTIONRECV), m_strClientFilename, strComment);

            // test if comment is filtered
            if (!thePrefs.GetCommentFilter().IsEmpty())
            {
                CString strCommentLower(strComment);
                strCommentLower.MakeLower();

                int iPos = 0;
                CString strFilter(thePrefs.GetCommentFilter().Tokenize(L"|", iPos));
                while (!strFilter.IsEmpty())
                {
                    // comment filters are already in lowercase, compare with temp. lowercased received comment
                    if (strCommentLower.Find(strFilter) >= 0)
                    {
                        strComment.Empty();
                        uRating = 0;
                        SetSpammer(true);
                        break;
                    }
                    strFilter = thePrefs.GetCommentFilter().Tokenize(L"|", iPos);
                }
            }
        }
        if (!strComment.IsEmpty() || uRating > 0)
        {
            m_strFileComment = strComment;
            m_uFileRating = uRating;
            reqfile->UpdateFileRatingCommentAvail();
//>>> WiZaRd::ClientAnalyzer
            if (pAntiLeechData)
                pAntiLeechData->DecSpam();
//<<< WiZaRd::ClientAnalyzer
        }
    }
}

bool CUpDownClient::Disconnected(LPCTSTR pszReason, bool bFromSocket)
{
    ASSERT(theApp.clientlist->IsValidClient(this));

    // TODO LOGREMOVE
    if (m_nConnectingState == CCS_DIRECTCALLBACK)
        DebugLog(L"Direct Callback failed - %s", DbgGetClientInfo());

    if (GetKadState() == KS_QUEUED_FWCHECK_UDP || GetKadState() == KS_CONNECTING_FWCHECK_UDP)
        Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, true, ntohl(GetConnectIP()), 0); // inform the tester that this test was cancelled
    else if (GetKadState() == KS_FWCHECK_UDP)
        Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, false, ntohl(GetConnectIP()), 0); // inform the tester that this test has failed
    else if (GetKadState() == KS_CONNECTED_BUDDY)
        DebugLogWarning(L"Buddy client disconnected - %s, %s", pszReason, DbgGetClientInfo());
    //If this is a KAD client object, just delete it!
    SetKadState(KS_NONE);

    if (GetUploadState() == US_UPLOADING || GetUploadState() == US_CONNECTING)
    {
        // sets US_NONE
        theApp.uploadqueue->RemoveFromUploadQueue(this, CString(L"CUpDownClient::Disconnected: ") + pszReason);
    }

    // 28-Jun-2004 [bc]: re-applied this patch which was in 0.30b-0.30e. it does not seem to solve the bug but
    // it does not hurt either...
    if (m_BlockRequests_queue.GetCount() > 0 || m_DoneBlocks_list.GetCount())
    {
        // Although this should not happen, it seems(?) to happens sometimes. The problem we may run into here is as follows:
        //
        // 1.) If we do not clear the block send requests for that client, we will send those blocks next time the client
        // gets an upload slot. But because we are starting to send any available block send requests right _before_ the
        // remote client had a chance to prepare to deal with them, the first sent blocks will get dropped by the client.
        // Worst thing here is, because the blocks are zipped and can therefore only be uncompressed when the first block
        // was received, all of those sent blocks will create a lot of uncompress errors at the remote client.
        //
        // 2.) The remote client may have already received those blocks from some other client when it gets the next
        // upload slot.
        DebugLogWarning(L"Disconnected client with non empty block send queue; %s reqs: %s doneblocks: %s", DbgGetClientInfo(), m_BlockRequests_queue.GetCount() > 0 ? L"true" : L"false", m_DoneBlocks_list.GetCount() ? L"true" : L"false");
        ClearUploadBlockRequests();
    }

    if (GetDownloadState() == DS_DOWNLOADING)
    {
        ASSERT(m_nConnectingState == CCS_NONE);
        SetDownloadState(DS_ONQUEUE, CString(L"Disconnected: ") + pszReason);
    }
    else
    {
        // ensure that all possible block requests are removed from the partfile
        ClearDownloadBlockRequests();
        if (GetDownloadState() == DS_CONNECTED)  // successfully connected, but probably didn't responsed to our filerequest
        {
            theApp.clientlist->m_globDeadSourceList.AddDeadSource(this);
            theApp.downloadqueue->RemoveSource(this);
        }
    }

    // we had still an AICH request pending, handle it
    if (IsAICHReqPending())
    {
        m_fAICHRequested = FALSE;
        CAICHRecoveryHashSet::ClientAICHRequestFailed(this);
    }

    // The remote client does not have to answer with OP_HASHSETANSWER *immediatly*
    // after we've sent OP_HASHSETREQUEST. It may occure that a (buggy) remote client
    // is sending use another OP_FILESTATUS which would let us change to DL-state to DS_ONQUEUE.
    if (m_fHashsetRequestingMD4 && (reqfile != NULL))
        reqfile->m_bMD4HashsetNeeded = true;
    if (m_fHashsetRequestingAICH && (reqfile != NULL))
        reqfile->SetAICHHashSetNeeded(true);

    if (m_iFileListRequested)
    {
        LogWarning(LOG_STATUSBAR, GetResString(IDS_SHAREDFILES_FAILED), GetUserName());
        m_iFileListRequested = 0;
    }

    if (m_Friend)
        theApp.friendlist->RefreshFriend(m_Friend);

    ASSERT(theApp.clientlist->IsValidClient(this));

    //check if this client is needed in any way, if not delete it
    bool bDelete = true;
    switch (m_nUploadState)
    {
    case US_ONUPLOADQUEUE:
        bDelete = false;
        break;
    }
    switch (m_nDownloadState)
    {
    case DS_ONQUEUE:
    case DS_TOOMANYCONNS:
    case DS_NONEEDEDPARTS:
    case DS_LOWTOLOWIP:
        bDelete = false;
    }

    // Dead Soure Handling
    //
    // If we failed to connect to that client, it is supposed to be 'dead'. Add the IP
    // to the 'dead sources' lists so we don't waste resources and bandwidth to connect
    // to that client again within the next hour.
    //
    // But, if we were just connecting to a proxy and failed to do so, that client IP
    // is supposed to be valid until the proxy itself tells us that the IP can not be
    // connected to (e.g. 504 Bad Gateway)
    //
    if ((m_nConnectingState != CCS_NONE && !(socket && socket->GetProxyConnectFailed()))
            || m_nDownloadState == DS_ERROR)
    {
        if (m_nDownloadState != DS_NONE) // Unable to connect = Remove any downloadstate
            theApp.downloadqueue->RemoveSource(this);
        theApp.clientlist->m_globDeadSourceList.AddDeadSource(this);
        bDelete = true;
    }

    // We keep chat partners in any case
    if (GetChatState() != MS_NONE)
    {
        bDelete = false;
        if (GetFriend() != NULL && GetFriend()->IsTryingToConnect())
            GetFriend()->UpdateFriendConnectionState(FCR_DISCONNECTED); // for friends any connectionupdate is handled in the friend class
        else
            theApp.emuledlg->chatwnd->chatselector.ConnectingResult(this,false); // other clients update directly
    }

    // Delete Socket
    if (!bFromSocket && socket)
    {
        ASSERT(theApp.listensocket->IsValidSocket(socket));
        socket->Safe_Delete();
    }
    socket = NULL;

    theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(this);

    // finally, remove the client from the timeouttimer and reset the connecting state
    m_nConnectingState = CCS_NONE;
    theApp.clientlist->RemoveConnectingClient(this);

    if (bDelete)
    {
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            Debug(L"--- Deleted client            %s; Reason=%s\n", DbgGetClientInfo(true), pszReason);
        return true;
    }
    else
    {
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            Debug(L"--- Disconnected client       %s; Reason=%s\n", DbgGetClientInfo(true), pszReason);
        m_fHashsetRequestingMD4 = 0;
        m_fHashsetRequestingAICH = 0;
        SetSentCancelTransfer(0);
        m_bHelloAnswerPending = false;
        m_fQueueRankPending = 0;
        m_fFailedFileIdReqs = 0;
        m_fUnaskQueueRankRecv = 0;
        m_fSentOutOfPartReqs = 0;
        return false;
    }
}

//Returned bool is not if the TryToConnect is successful or not..
//false means the client was deleted!
//true means the client was not deleted!
bool CUpDownClient::TryToConnect(bool bIgnoreMaxCon, bool bNoCallbacks, CRuntimeClass* pClassSocket)
{
    // There are 7 possible ways how we are going to connect in this function, sorted by priority:
    // 1) Already Connected/Connecting
    //		We are already connected or try to connect right now. Abort, no additional Disconnect() call will be done
    // 2) Immediate Fail
    //		Some precheck or precondition failed, or no other way is available, so we do not try to connect at all
    //		but fail right away, possibly deleting the client as it becomes useless
    // 3) Normal Outgoing TCP Connection
    //		Applies to all HighIDs/Open clients: We do a straight forward connection try to the TCP port of the client
    // 4) Direct Callback Connections
    //		Applies to TCP firewalled - UDP open clients: We sent a UDP packet to the client, requesting him to connect
    //		to us. This is pretty easy too and ressourcewise nearly on the same level as 3)
    // (* 5) Waiting/Abort
    //		This check is done outside this function.
    //		We want to connect for some download related thing (for example reasking), but the client has a LowID and
    //		is on our uploadqueue. So we are smart and safing ressources by just waiting untill he reasks us, so we don't
    //		have to do the ressource intensive options 6 or 7. *)
    // 6) Server Callback
    //		This client is firewalled, but connected to our server. We sent the server a callback request to forward to
    //		the client and hope for the best
    // 7) Kad Callback
    //		This client is firewalled, but has a Kad buddy. We sent the buddy a callback request to forward to the client
    //		and hope for the best

    if (GetKadState() == KS_QUEUED_FWCHECK)
        SetKadState(KS_CONNECTING_FWCHECK);
    else if (GetKadState() == KS_QUEUED_FWCHECK_UDP)
        SetKadState(KS_CONNECTING_FWCHECK_UDP);

    ////////////////////////////////////////////////////////////
    // Check for 1) Already Connected/Connecting
    if (m_nConnectingState != CCS_NONE)
    {
        DebugLog(L"TryToConnect: Already Connecting (%s)", DbgGetClientInfo());// TODO LogRemove
        return true;
    }
    else if (socket != NULL)
    {
        if (socket->IsConnected())
        {
            if (CheckHandshakeFinished())
            {
                DEBUG_ONLY(DebugLog(L"TryToConnect: Already Connected (%s)", DbgGetClientInfo()));  // TODO LogRemove
                ConnectionEstablished();
            }
            else
                DebugLogWarning(L"TryToConnect found connected socket, but without Handshake finished - %s", DbgGetClientInfo());
            return true;
        }
        else
            socket->Safe_Delete();
    }
    m_nConnectingState = CCS_PRECONDITIONS; // We now officially try to connect :)

    ////////////////////////////////////////////////////////////
    // Check for 2) Immediate Fail

    if (!bIgnoreMaxCon && theApp.listensocket->TooManySockets())
    {
        // This is a sanitize check and counts as a "hard failure", so this check should be also done before calling
        // TryToConnect if a special handling, like waiting till there are enough connection available should be fone
        DebugLogWarning(L"TryToConnect: Too many connections sanitize check (%s)", DbgGetClientInfo());
        if (Disconnected(L"Too many connections"))
        {
            delete this;
            return false;
        }
        return true;
    }
    // do not try to connect to source which are incompatible with our encryption setting (one requires it, and the other one doesn't supports it)
    if ((thePrefs.IsClientCryptLayerRequired() && !SupportsCryptLayer()))
    {
        DEBUG_ONLY(AddDebugLogLine(DLP_DEFAULT, false, L"Rejected outgoing connection because CryptLayer-Setting (Obfuscation) was incompatible %s", DbgGetClientInfo()));
        if (Disconnected(L"CryptLayer-Settings (Obfuscation) incompatible"))
        {
            delete this;
            return false;
        }
        else
            return true;
    }

    UINT uClientIP = (GetIP() != 0) ? GetIP() : GetConnectIP();
    if (uClientIP == 0 && !HasLowID())
        uClientIP = ntohl(m_nUserIDHybrid);
    if (uClientIP)
    {
        // although we filter all received IPs (server sources, source exchange) and all incomming connection attempts,
        // we do have to filter outgoing connection attempts here too, because we may have updated the ip filter list
        if (theApp.ipfilter->IsFiltered(uClientIP))
        {
            if (thePrefs.GetLogFilteredIPs())
                AddDebugLogLine(true, GetResString(IDS_IPFILTERED), ipstr(uClientIP), theApp.ipfilter->GetLastHit());
            if (Disconnected(L"IPFilter"))
            {
                delete this;
                return false;
            }
            return true;
        }

        // for safety: check again whether that IP is banned
        if (theApp.clientlist->IsBannedClient(uClientIP))
        {
            if (thePrefs.GetLogBannedClients())
                AddDebugLogLine(false, L"Refused to connect to banned client %s", DbgGetClientInfo());
            if (Disconnected(L"Banned IP"))
            {
                delete this;
                return false;
            }
            return true;
        }
    }

    if (HasLowID())
    {
        ASSERT(pClassSocket == NULL);
        if (!theApp.CanDoCallback()) // lowid2lowid check used for the whole function, don't remove
        {
            // We cannot reach this client, so we hard fail to connect, if this client should be kept,
            // for example because we might want to wait a bit and hope we get a highid, this check has
            // to be done before calling this function
            if (Disconnected(L"LowID->LowID"))
            {
                delete this;
                return false;
            }
            return true;
        }

        // are callbacks disallowed?
        if (bNoCallbacks)
        {
            DebugLogError(L"TryToConnect: Would like to do callback on a no-callback client, %s", DbgGetClientInfo());
            if (Disconnected(L"LowID: No Callback Option allowed"))
            {
                delete this;
                return false;
            }
            return true;
        }

        // Is any callback available?
        if (!((SupportsDirectUDPCallback() && thePrefs.GetUDPPort() != 0 && GetConnectIP() != 0)  // Direct Callback
                || (HasValidBuddyID() && Kademlia::CKademlia::IsConnected()) // Kad Callback
             ))
        {
            // Nope
            if (Disconnected(L"LowID: No Callback Option available"))
            {
                delete this;
                return false;
            }
            return true;
        }
    }

    // Prechecks finished, now for the real connecting
    ////////////////////////////////////////////////////

    theApp.clientlist->AddConnectingClient(this); // Starts and checks for the timeout, ensures following Disconnect() or ConnectionEstablished() call

    ////////////////////////////////////////////////////////////
    // 3) Normal Outgoing TCP Connection
    if (!HasLowID())
    {
        m_nConnectingState = CCS_DIRECTTCP;
        if (pClassSocket == NULL)
            pClassSocket = RUNTIME_CLASS(CClientReqSocket);
        socket = static_cast<CClientReqSocket*>(pClassSocket->CreateObject());
        socket->SetClient(this);
        if (!socket->Create())
        {
            socket->Safe_Delete();
            // we let the timeout handle the cleanup in this case
            DebugLogError(L"TryToConnect: Failed to create socket for outgoing connection, %s", DbgGetClientInfo());
        }
        else
            Connect();
        return true;
    }
    ////////////////////////////////////////////////////////////
    // 4) Direct Callback Connections
    else if (SupportsDirectUDPCallback() && thePrefs.GetUDPPort() != 0 && GetConnectIP() != 0)
    {
        m_nConnectingState = CCS_DIRECTCALLBACK;
        // TODO LOGREMOVE
        DebugLog(L"Direct Callback on port %u to client %s (%s) ", GetKadPort(), DbgGetClientInfo(), md4str(GetUserHash()));
        CSafeMemFile data;
        data.WriteUInt16(thePrefs.GetPort()); // needs to know our port
        data.WriteHash16(thePrefs.GetUserHash()); // and userhash
        // our connection settings
        data.WriteUInt8(GetMyConnectOptions(true, false));
        if (thePrefs.GetDebugClientUDPLevel() > 0)
            DebugSend("OP_DIRECTCALLBACKREQ", this);
        Packet* packet = new Packet(&data, OP_EMULEPROT);
        packet->opcode = OP_DIRECTCALLBACKREQ;
        theStats.AddUpDataOverheadOther(packet->size);
        theApp.clientudp->SendPacket(packet, GetConnectIP(), GetKadPort(), ShouldReceiveCryptUDPPackets(), GetUserHash(), false, 0);
        return true;
    }
    ////////////////////////////////////////////////////////////
    // 6) Server Callback + 7) Kad Callback
    if (GetDownloadState() == DS_CONNECTING)
        SetDownloadState(DS_WAITCALLBACK);

    if (GetUploadState() == US_CONNECTING)
    {
        ASSERT(0); // we should never try to connect in this case, but wait for the LowID to connect to us
        DebugLogError(L"LowID and US_CONNECTING (%s)", DbgGetClientInfo());
    }

    if (HasValidBuddyID() && Kademlia::CKademlia::IsConnected())
    {
        m_nConnectingState = CCS_KADCALLBACK;
        if (GetBuddyIP() && GetBuddyPort())
        {
            CSafeMemFile bio(34);
            bio.WriteUInt128(&Kademlia::CUInt128(GetBuddyID()));
            bio.WriteUInt128(&Kademlia::CUInt128(reqfile->GetFileHash()));
            bio.WriteUInt16(thePrefs.GetPort());
            if (thePrefs.GetDebugClientKadUDPLevel() > 0 || thePrefs.GetDebugClientUDPLevel() > 0)
                DebugSend("KadCallbackReq", this);
            Packet* packet = new Packet(&bio, OP_KADEMLIAHEADER);
            packet->opcode = KADEMLIA_CALLBACK_REQ;
            theStats.AddUpDataOverheadKad(packet->size);
            // FIXME: We dont know which kadversion the buddy has, so we need to send unencrypted
            theApp.clientudp->SendPacket(packet, GetBuddyIP(), GetBuddyPort(), false, NULL, true, 0);
            SetDownloadState(DS_WAITCALLBACKKAD);
        }
        else
        {
            // I don't think we should ever have a buddy without its IP (anymore), but nevertheless let the functionality in
            //Create search to find buddy.
            Kademlia::CSearch *findSource = new Kademlia::CSearch;
            findSource->SetSearchTypes(Kademlia::CSearch::FINDSOURCE);
            findSource->SetTargetID(Kademlia::CUInt128(GetBuddyID()));
            findSource->AddFileID(Kademlia::CUInt128(reqfile->GetFileHash()));
            if (Kademlia::CKademlia::GetPrefs()->GetTotalSource() > 0 || Kademlia::CSearchManager::AlreadySearchingFor(Kademlia::CUInt128(GetBuddyID())))
            {
                //There are too many source lookups already or we are already searching this key.
                // bad luck, as lookups aren't supposed to happen anyway, we just let it fail, if we want
                // to actually really use lookups (so buddies without known IPs), this should be reworked
                // for example by adding a queuesystem for queries
                DebugLogWarning(L"TryToConnect: Buddy without knonw IP, Lookup crrently impossible");
                return true;
            }
            if (Kademlia::CSearchManager::StartSearch(findSource))
            {
                //Started lookup..
                SetDownloadState(DS_WAITCALLBACKKAD);
            }
            else
            {
                //This should never happen..
                ASSERT(0);
            }
        }
        return true;
    }
    else
    {
        ASSERT(0);
        DebugLogError(L"TryToConnect: Bug: No Callback available despite prechecks");
        return true;
    }
}

void CUpDownClient::Connect()
{
    // enable or disable crypting based on our and the remote clients preference
    if (HasValidHash() && SupportsCryptLayer() && (RequestsCryptLayer() || thePrefs.IsClientCryptLayerRequested()))
    {
        //DebugLog(L"Enabling CryptLayer on outgoing connection to client %s", DbgGetClientInfo()); // to be removed later
        socket->SetConnectionEncryption(true, GetUserHash(), false);
    }
    else
        socket->SetConnectionEncryption(false, NULL, false);

    //Try to always tell the socket to WaitForOnConnect before you call Connect.
    socket->WaitForOnConnect();
    SOCKADDR_IN sockAddr = {0};
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(GetUserPort());
    sockAddr.sin_addr.S_un.S_addr = GetConnectIP();
    socket->Connect((SOCKADDR*)&sockAddr, sizeof sockAddr);
    SendHelloPacket();
}

void CUpDownClient::ConnectionEstablished()
{
    // ok we have a connection, lets see if we want anything from this client

    // was this a direct callback?
    if (m_nConnectingState == CCS_DIRECTCALLBACK) // TODO LOGREMOVE
        DebugLog(L"Direct Callback succeeded, connection established - %s", DbgGetClientInfo());

    // remove the connecting timer and state
    //if (m_nConnectingState == CCS_NONE) // TODO LOGREMOVE
    //	DEBUG_ONLY( DebugLog(L"ConnectionEstablished with CCS_NONE (incoming, thats fine)")) );
    m_nConnectingState = CCS_NONE;
    theApp.clientlist->RemoveConnectingClient(this);

    // check if we should use this client to retrieve our public IP
    // TODO - WiZaRd: why do we rely on the m_fPeerCache flag!?
    //PC was introduced with eMule v0.43a, so we check for the version, now
    if (theApp.GetPublicIP() == 0 && theApp.IsConnected()
            && GetClientSoft() == SO_EMULE && m_nClientVersion >= MAKE_CLIENT_VERSION(0, 43, 0))
        SendPublicIPRequest();

    switch (GetKadState())
    {
    case KS_CONNECTING_FWCHECK:
        SetKadState(KS_CONNECTED_FWCHECK);
        break;
    case KS_CONNECTING_BUDDY:
    case KS_INCOMING_BUDDY:
        DEBUG_ONLY(DebugLog(L"Set KS_CONNECTED_BUDDY for client %s", DbgGetClientInfo()));
        SetKadState(KS_CONNECTED_BUDDY);
        break;
    case KS_CONNECTING_FWCHECK_UDP:
        SetKadState(KS_FWCHECK_UDP);
        DEBUG_ONLY(DebugLog(L"Set KS_FWCHECK_UDP for client %s", DbgGetClientInfo()));
        SendFirewallCheckUDPRequest();
        break;
    }

    if (GetChatState() == MS_CONNECTING || GetChatState() == MS_CHATTING)
    {
        if (GetFriend() != NULL && GetFriend()->IsTryingToConnect())
        {
            GetFriend()->UpdateFriendConnectionState(FCR_ESTABLISHED); // for friends any connectionupdate is handled in the friend class
            if (credits != NULL && credits->GetCurrentIdentState(GetConnectIP()) == IS_IDFAILED)
                GetFriend()->UpdateFriendConnectionState(FCR_SECUREIDENTFAILED);
        }
        else
            theApp.emuledlg->chatwnd->chatselector.ConnectingResult(this, true); // other clients update directly
    }

    switch (GetDownloadState())
    {
    case DS_CONNECTING:
    case DS_WAITCALLBACK:
    case DS_WAITCALLBACKKAD:
        m_bReaskPending = false;
        SetDownloadState(DS_CONNECTED);
        SendFileRequest();
        break;
    }

    if (m_bReaskPending)
    {
        m_bReaskPending = false;
        if (GetDownloadState() != DS_NONE && GetDownloadState() != DS_DOWNLOADING)
        {
            SetDownloadState(DS_CONNECTED);
            SendFileRequest();
        }
    }

    switch (GetUploadState())
    {
    case US_CONNECTING:
        if (theApp.uploadqueue->IsDownloading(this))
        {
            SetUploadState(US_UPLOADING);
            if (thePrefs.GetDebugClientTCPLevel() > 0)
                DebugSend("OP__AcceptUploadReq", this);
            Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
            theStats.AddUpDataOverheadFileRequest(packet->size);
            SendPacket(packet,true);
        }
    }

    if (m_iFileListRequested == 1)
    {
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend(m_fSharedDirectories ? "OP__AskSharedDirs" : "OP__AskSharedFiles", this);
        Packet* packet = new Packet(m_fSharedDirectories ? OP_ASKSHAREDDIRS : OP_ASKSHAREDFILES,0);
        theStats.AddUpDataOverheadOther(packet->size);
        SendPacket(packet,true);
    }

    while (!m_WaitingPackets_list.IsEmpty())
    {
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("Buffered Packet", this);
        SendPacket(m_WaitingPackets_list.RemoveHead(), true);
    }

}

void CUpDownClient::InitClientSoftwareVersion()
{
	CString szSoftware = L"";
    if (m_pszUsername == NULL)
    {
        m_clientSoft = SO_UNKNOWN;
    }
	else
	{
		int iHashType = GetHashType();
		if (m_bEmuleProtocol || iHashType == SO_EMULE)
		{
			CString pszSoftware;
			switch (m_byCompatibleClient)
			{
			case SO_CDONKEY:
				m_clientSoft = SO_CDONKEY;
				pszSoftware = L"cDonkey";
				break;
			case SO_XMULE:
				m_clientSoft = SO_XMULE;
				pszSoftware = L"xMule";
				break;
			case SO_AMULE:
				m_clientSoft = SO_AMULE;
				pszSoftware = L"aMule";
				break;
			case SO_SHAREAZA:
//>>> WiZaRd::ClientAnalyzer
// Spike2 - Enhanced Client Recognition - START
				// removed "case 40"... this is now integrated here
			case SO_SHAREAZA2:
			case SO_SHAREAZA3:
			case SO_SHAREAZA4:
// Spike2 - Enhanced Client Recognition - END
//<<< WiZaRd::ClientAnalyzer
				m_clientSoft = SO_SHAREAZA;
				pszSoftware = L"Shareaza";
				break;
			case SO_LPHANT:
				m_clientSoft = SO_LPHANT;
				pszSoftware = L"lphant";
				break;
//>>> WiZaRd::ClientAnalyzer
// Spike2 - Enhanced Client Recognition - START
			case SO_EMULEPLUS:
				m_clientSoft = SO_EMULEPLUS;
				pszSoftware = L"eMule Plus";
				break;
			case SO_HYDRANODE:
				m_clientSoft = SO_HYDRANODE;
				pszSoftware = L"Hydranode";
				break;
			case SO_TRUSTYFILES:
				m_clientSoft = SO_TRUSTYFILES;
				pszSoftware = L"TrustyFiles";
				break;
			case SO_EASYMULE2:
				m_clientSoft = SO_EASYMULE2;
				pszSoftware = L"EasyMule2";
				break;
			case SO_NEOLOADER:
				m_clientSoft = SO_NEOLOADER;
				pszSoftware = L"NeoLoader";
				break;
//>>> WiZaRd::kMule Version Ident
			// not used, yet...
			case SO_KMULE:
				m_clientSoft = SO_KMULE;
				pszSoftware = L"kMule";
				break;
//<<< WiZaRd::kMule Version Ident
// Spike2 - Enhanced Client Recognition - END
//<<< WiZaRd::ClientAnalyzer
			default:
				if (m_bIsML 
					|| m_byCompatibleClient == SO_MLDONKEY 
//>>> WiZaRd::ClientAnalyzer // Spike2 - Enhanced Client Recognition
					|| m_byCompatibleClient == SO_MLDONKEY2 
					|| m_byCompatibleClient == SO_MLDONKEY3) 
//<<< WiZaRd::ClientAnalyzer // Spike2 - Enhanced Client Recognition
				{
					m_clientSoft = SO_MLDONKEY;
					pszSoftware = L"MLdonkey";
				}
				else if (m_bIsHybrid 
//>>> WiZaRd::ClientAnalyzer // Spike2 - Enhanced Client Recognition
					|| m_byCompatibleClient == SO_EDONKEYHYBRID) 
//<<< WiZaRd::ClientAnalyzer // Spike2 - Enhanced Client Recognition
				{
					m_clientSoft = SO_EDONKEYHYBRID;
					pszSoftware = L"eDonkeyHybrid";
				}
				else if (m_byCompatibleClient != 0)
				{
//>>> WiZaRd::ClientAnalyzer
// Spike2 - Enhanced Client Recognition - START
//WiZaRd: this is highly unreliable and thus I removed it
 					// Recognize other Shareazas - just to be sure :)
 					/*if (StrStrI(m_pszUsername, L"shareaza"))
 					{
 						m_clientSoft = SO_SHAREAZA;
 						pszSoftware = L"Shareaza";
 					}
 					// Recognize all eMulePlus - just to be sure !
					else*/ if (StrStr(m_strModVersion, L"Plus 1"))
					{
						m_clientSoft = SO_EMULEPLUS;
						pszSoftware = L"eMule Plus";
					}
					else
// Spike2 - Enhanced Client Recognition - END
//<<< WiZaRd::ClientAnalyzer
					{
						m_clientSoft = SO_XMULE; // means: 'eMule Compatible'
						pszSoftware = L"eMule Compat";
					}
				}
				else
				{
					m_clientSoft = SO_EMULE;
					pszSoftware = L"eMule";
				}
				break;
			}

			if (m_byEmuleVersion == 0)
			{
				m_nClientVersion = MAKE_CLIENT_VERSION(0, 0, 0);
				szSoftware = pszSoftware;
			}
			else if (m_byEmuleVersion != 0x99)
			{
				UINT nClientMinVersion = (m_byEmuleVersion >> 4)*10 + (m_byEmuleVersion & 0x0f);
				m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
				szSoftware.Format(L"%s v0.%u", pszSoftware, nClientMinVersion);
			}
			else
			{
				UINT nClientMajVersion = (m_nClientVersion >> 17) & 0x7f;
				UINT nClientMinVersion = (m_nClientVersion >> 10) & 0x7f;
				UINT nClientUpVersion  = (m_nClientVersion >>  7) & 0x07;
//>>> WiZaRd::kMule Version Ident
				// Some hack solution to accept the current kMule string scheme while we aren't identifying as SO_KMULE straight away
				if (m_clientSoft == SO_EMULE && _tcscmp(m_strModVersion, L"kMule Build") == 0)
				{
					CString tmp = m_strModVersion.Mid(11);
					m_clientSoft = SO_KMULE;
					m_strModVersion = L"";
					nClientMajVersion = (UINT)_tstoi(tmp.Mid(0, 4));
					nClientMinVersion = (UINT)_tstoi(tmp.Mid(4, 2));
					nClientUpVersion = (UINT)_tstoi(tmp.Mid(6, 2));
				}
//<<< WiZaRd::kMule Version Ident
				m_nClientVersion = MAKE_CLIENT_VERSION(nClientMajVersion, nClientMinVersion, nClientUpVersion);
				if (m_clientSoft == SO_EMULE)
					szSoftware.Format(L"%s v%u.%u%c", pszSoftware, nClientMajVersion, nClientMinVersion, L'a' + nClientUpVersion);
//>>> WiZaRd::ClientAnalyzer
// Spike2 - Enhanced Client Recognition - START
				else if (m_clientSoft == SO_EMULEPLUS)
				{
					szSoftware.Format(L"%s v%u", pszSoftware, nClientMajVersion);
					if(nClientMinVersion != 0)
						szSoftware.AppendFormat(L".%u", nClientMinVersion);
					if(nClientUpVersion != 0)
						szSoftware.AppendFormat(L"%c", L'a' + nClientUpVersion - 1);
				}
				else if (m_clientSoft == SO_NEOLOADER)
				{
					if (nClientMinVersion < 10)
						szSoftware.Format(L"%s v%u.0%u", pszSoftware, nClientMajVersion, nClientMinVersion);
					else
						szSoftware.Format(L"%s v%u.%u", pszSoftware, nClientMajVersion, nClientMinVersion);

					if (nClientUpVersion != 0)
						szSoftware.AppendFormat(L"%c", L'a' + nClientUpVersion - 1);
				}
// Spike2 - Enhanced Client Recognition - END
//>>> WiZaRd::kMule Version Ident
				// not used, yet...
				else if(m_clientSoft == SO_KMULE)
				{
					// we (will) use	nClientMajVersion (4 digits = year)
					//					nClientMinVersion (2 digits = month)
					//					nClientUpVersion (2 digits = day)
					szSoftware.Format(L"%s Build%u", nClientMajVersion);
					if(nClientMinVersion < 10)
						szSoftware.AppendFormat(L"0%u", nClientMinVersion);
					else
						szSoftware.AppendFormat(L"%u", nClientMinVersion);
					if(nClientUpVersion < 10)
						szSoftware.AppendFormat(L"0%u", nClientUpVersion);
					else
						szSoftware.AppendFormat(L"%u", nClientUpVersion);
				}
//<<< WiZaRd::kMule Version Ident
//<<< WiZaRd::ClientAnalyzer
				else if (m_clientSoft == SO_AMULE || nClientUpVersion != 0)
					szSoftware.Format(L"%s v%u.%u.%u", pszSoftware, nClientMajVersion, nClientMinVersion, nClientUpVersion);
				else if (m_clientSoft == SO_LPHANT)
				{
					if (nClientMinVersion < 10)
						szSoftware.Format(L"%s v%u.0%u", pszSoftware, (nClientMajVersion-1), nClientMinVersion);
					else
						szSoftware.Format(L"%s v%u.%u", pszSoftware, (nClientMajVersion-1), nClientMinVersion);
				}
				else
					szSoftware.Format(L"%s v%u.%u", pszSoftware, nClientMajVersion, nClientMinVersion);
			}
        } 
		else if (m_bIsHybrid 
			|| m_byCompatibleClient == SO_EDONKEYHYBRID) //>>> WiZaRd::ClientAnalyzer // Spike2 - Enhanced Client Recognition
		{
			m_clientSoft = SO_EDONKEYHYBRID;
			// seen:
			// 105010	0.50.10
			// 10501	0.50.1
			// 10405	1.4.5 // netfinity
			// 10300	1.3.0
			// 10212	1.2.2 // Spike2
			// 10211	1.2.1
			// 10103	1.1.3
			// 10102	1.1.2
			// 10100	1.1
			// 1051		0.51.0
			// 1002		1.0.2
			// 1000		1.0
			// 501		0.50.1

			UINT nClientMajVersion;
			UINT nClientMinVersion;
			UINT nClientUpVersion;
			if (m_nClientVersion > 100000)
			{
				UINT uMaj = m_nClientVersion/100000;
				nClientMajVersion = uMaj - 1;
				nClientMinVersion = (m_nClientVersion - uMaj*100000) / 100;
				nClientUpVersion = m_nClientVersion % 100;
			}
			else if (m_nClientVersion >= 10100 && m_nClientVersion <= 10409) //>>> WiZaRd::ClientAnalyzer // netfinity
			{
				UINT uMaj = m_nClientVersion/10000;
				nClientMajVersion = uMaj;
				nClientMinVersion = (m_nClientVersion - uMaj*10000) / 100;
				nClientUpVersion = m_nClientVersion % 10;
			}
			else if (m_nClientVersion > 10000)
			{
				UINT uMaj = m_nClientVersion/10000;
				nClientMajVersion = uMaj - 1;
				nClientMinVersion = (m_nClientVersion - uMaj*10000) / 10;
				nClientUpVersion = m_nClientVersion % 10;
			}
			else if (m_nClientVersion >= 1000 && m_nClientVersion < 1020)
			{
				UINT uMaj = m_nClientVersion/1000;
				nClientMajVersion = uMaj;
				nClientMinVersion = (m_nClientVersion - uMaj*1000) / 10;
				nClientUpVersion = m_nClientVersion % 10;
			}
			else if (m_nClientVersion > 1000)
			{
				UINT uMaj = m_nClientVersion/1000;
				nClientMajVersion = uMaj - 1;
				nClientMinVersion = m_nClientVersion - uMaj*1000;
				nClientUpVersion = 0;
			}
			else if (m_nClientVersion > 100)
			{
				UINT uMin = m_nClientVersion/10;
				nClientMajVersion = 0;
				nClientMinVersion = uMin;
				nClientUpVersion = m_nClientVersion - uMin*10;
			}
			else
			{
				nClientMajVersion = 0;
				nClientMinVersion = m_nClientVersion;
				nClientUpVersion = 0;
			}
			m_nClientVersion = MAKE_CLIENT_VERSION(nClientMajVersion, nClientMinVersion, nClientUpVersion);

			if (nClientUpVersion)
				szSoftware.Format(L"eDonkeyHybrid v%u.%u.%u", nClientMajVersion, nClientMinVersion, nClientUpVersion);
			else
				szSoftware.Format(L"eDonkeyHybrid v%u.%u", nClientMajVersion, nClientMinVersion);
		}
		else if (m_bIsML || iHashType == SO_MLDONKEY)
		{
			m_clientSoft = SO_MLDONKEY;
			UINT nClientMinVersion = m_nClientVersion;
			m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
			szSoftware.Format(L"MLdonkey v0.%u", nClientMinVersion);
		}
		else if (iHashType == SO_OLDEMULE)
		{
			m_clientSoft = SO_OLDEMULE;
			UINT nClientMinVersion = m_nClientVersion;
			m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
			szSoftware.Format(L"Old eMule v0.%u", nClientMinVersion);
		}
		else 
		{
			m_clientSoft = SO_EDONKEY;
			UINT nClientMinVersion = m_nClientVersion;
			m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
			szSoftware.Format(L"eDonkey v0.%u", nClientMinVersion);
		}
		if(!szSoftware.IsEmpty())
			m_strClientSoftware = szSoftware;
	}
}

int CUpDownClient::GetHashType() const
{
    if (m_achUserHash[5] == 13 && m_achUserHash[14] == 110)
        return SO_OLDEMULE;
    else if (m_achUserHash[5] == 14 && m_achUserHash[14] == 111)
        return SO_EMULE;
    else if (m_achUserHash[5] == 'M' && m_achUserHash[14] == 'L')
        return SO_MLDONKEY;
    else
        return SO_UNKNOWN;
}

void CUpDownClient::SetUserName(LPCTSTR pszNewName)
{
    free(m_pszUsername);
    if (pszNewName)
        m_pszUsername = _tcsdup(pszNewName);
    else
        m_pszUsername = NULL;
}

void CUpDownClient::RequestSharedFileList()
{
    if (m_iFileListRequested == 0)
    {
        AddLogLine(true, GetResString(IDS_SHAREDFILES_REQUEST), GetUserName());
        m_iFileListRequested = 1;
        TryToConnect(true);
    }
    else
    {
        LogWarning(LOG_STATUSBAR, L"Requesting shared files from user %s (%u) is already in progress", GetUserName(), GetUserIDHybrid());
    }
}

void CUpDownClient::ProcessSharedFileList(const uchar* pachPacket, UINT nSize, LPCTSTR pszDirectory)
{
    if (m_iFileListRequested > 0)
    {
        m_iFileListRequested--;
        theApp.searchlist->ProcessSearchAnswer(pachPacket,nSize,this,NULL,pszDirectory);
    }
}

void CUpDownClient::SetUserHash(const uchar* pucUserHash)
{
    if (pucUserHash == NULL)
    {
        md4clr(m_achUserHash);
        return;
    }
    md4cpy(m_achUserHash, pucUserHash);
}

void CUpDownClient::SetBuddyID(const uchar* pucBuddyID)
{
    if (pucBuddyID == NULL)
    {
        md4clr(m_achBuddyID);
        m_bBuddyIDValid = false;
        return;
    }
    m_bBuddyIDValid = true;
    md4cpy(m_achBuddyID, pucBuddyID);
}

void CUpDownClient::SendPublicKeyPacket()
{
    // send our public key to the client who requested it
    if (socket == NULL || credits == NULL || m_SecureIdentState != IS_KEYANDSIGNEEDED)
    {
        ASSERT(false);
        return;
    }
    if (!theApp.clientcredits->CryptoAvailable())
        return;

    Packet* packet = new Packet(OP_PUBLICKEY,theApp.clientcredits->GetPubKeyLen() + 1,OP_EMULEPROT);
    theStats.AddUpDataOverheadOther(packet->size);
    memcpy(packet->pBuffer+1,theApp.clientcredits->GetPublicKey(), theApp.clientcredits->GetPubKeyLen());
    packet->pBuffer[0] = theApp.clientcredits->GetPubKeyLen();
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__PublicKey", this);
    SendPacket(packet, true);
    m_SecureIdentState = IS_SIGNATURENEEDED;
}

void CUpDownClient::SendSignaturePacket()
{
    // signate the public key of this client and send it
    if (socket == NULL || credits == NULL || m_SecureIdentState == 0)
    {
        ASSERT(false);
        return;
    }

    if (!theApp.clientcredits->CryptoAvailable())
        return;
    if (credits->GetSecIDKeyLen() == 0)
        return; // We don't have his public key yet, will be back here later
    // do we have a challenge value received (actually we should if we are in this function)
    if (credits->m_dwCryptRndChallengeFrom == 0)
    {
        if (thePrefs.GetLogSecureIdent())
            AddDebugLogLine(false, L"Want to send signature but challenge value is invalid ('%s')", GetUserName());
        return;
    }
    // v2
    // we will use v1 as default, except if only v2 is supported
    bool bUseV2;
    if ((m_bySupportSecIdent&1) == 1)
        bUseV2 = false;
    else
        bUseV2 = true;

    uint8 byChaIPKind = 0;
    UINT ChallengeIP = 0;
    if (bUseV2)
    {
        // we cannot do not know for sure our public ip, so use the remote clients one
        // TODO: we should NOT rely on servers here - we also have KAD and other means to find out our IP
        ChallengeIP = GetIP();
        byChaIPKind = CRYPT_CIP_REMOTECLIENT;
    }
    //end v2
    uchar achBuffer[250];
    uint8 siglen = theApp.clientcredits->CreateSignature(credits, achBuffer,  250, ChallengeIP, byChaIPKind);
    if (siglen == 0)
    {
        ASSERT(false);
        return;
    }
    Packet* packet = new Packet(OP_SIGNATURE,siglen + 1+ ((bUseV2)? 1:0),OP_EMULEPROT);
    theStats.AddUpDataOverheadOther(packet->size);
    memcpy(packet->pBuffer+1,achBuffer, siglen);
    packet->pBuffer[0] = siglen;
    if (bUseV2)
        packet->pBuffer[1+siglen] = byChaIPKind;
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__Signature", this);
    SendPacket(packet, true);
    m_SecureIdentState = IS_ALLREQUESTSSEND;
}

void CUpDownClient::ProcessPublicKeyPacket(const uchar* pachPacket, UINT nSize)
{
    theApp.clientlist->AddTrackClient(this);

    if (socket == NULL || credits == NULL || pachPacket[0] != nSize-1
            || nSize < 10 || nSize > 250)
    {
        ASSERT(false);
        return;
    }
    if (!theApp.clientcredits->CryptoAvailable())
        return;
    // the function will handle everything (mulitple key etc)
    if (credits->SetSecureIdent(pachPacket+1, pachPacket[0]))
    {
        // if this client wants a signature, now we can send him one
        if (m_SecureIdentState == IS_SIGNATURENEEDED)
        {
            SendSignaturePacket();
        }
        else if (m_SecureIdentState == IS_KEYANDSIGNEEDED)
        {
            // something is wrong
            if (thePrefs.GetLogSecureIdent())
                AddDebugLogLine(false, L"Invalid State error: IS_KEYANDSIGNEEDED in ProcessPublicKeyPacket");
        }
    }
    else
    {
        if (thePrefs.GetLogSecureIdent())
            AddDebugLogLine(false, L"Failed to use new received public key");
    }
}

void CUpDownClient::ProcessSignaturePacket(const uchar* pachPacket, UINT nSize)
{
    // here we spread the good guys from the bad ones ;)

    if (socket == NULL || credits == NULL || nSize > 250 || nSize < 10)
    {
        ASSERT(false);
        return;
    }

    uint8 byChaIPKind;
    if (pachPacket[0] == nSize-1)
        byChaIPKind = 0;
    else if (pachPacket[0] == nSize-2 && (m_bySupportSecIdent & 2) > 0) //v2
        byChaIPKind = pachPacket[nSize-1];
    else
    {
        ASSERT(false);
        return;
    }

    if (!theApp.clientcredits->CryptoAvailable())
        return;

    // we accept only one signature per IP, to avoid floods which need a lot cpu time for cryptfunctions
    if (m_dwLastSignatureIP == GetIP())
    {
        if (thePrefs.GetLogSecureIdent())
            AddDebugLogLine(false, L"received multiple signatures from one client");
        return;
    }

    // also make sure this client has a public key
    if (credits->GetSecIDKeyLen() == 0)
    {
        if (thePrefs.GetLogSecureIdent())
            AddDebugLogLine(false, L"received signature for client without public key");
        return;
    }

    // and one more check: did we ask for a signature and sent a challenge packet?
    if (credits->m_dwCryptRndChallengeFor == 0)
    {
        if (thePrefs.GetLogSecureIdent())
            AddDebugLogLine(false, L"received signature for client with invalid challenge value ('%s')", GetUserName());
        return;
    }

    if (theApp.clientcredits->VerifyIdent(credits, pachPacket+1, pachPacket[0], GetIP(), byChaIPKind))
    {
        // result is saved in function above
        //if (thePrefs.GetLogSecureIdent())
        //	AddDebugLogLine(false, L"'%s' has passed the secure identification, V2 State: %i", GetUserName(), byChaIPKind);

        // inform our friendobject if needed
        if (GetFriend() != NULL && GetFriend()->IsTryingToConnect())
            GetFriend()->UpdateFriendConnectionState(FCR_USERHASHVERIFIED);
    }
    else
    {
        if (GetFriend() != NULL && GetFriend()->IsTryingToConnect())
            GetFriend()->UpdateFriendConnectionState(FCR_SECUREIDENTFAILED);
        if (thePrefs.GetLogSecureIdent())
            AddDebugLogLine(false, L"'%s' has failed the secure identification, V2 State: %i", GetUserName(), byChaIPKind);
    }
    m_dwLastSignatureIP = GetIP();
//>>> WiZaRd::ClientAnalyzer
    //this client has passed the sec ident check... that means we can trust the data we collected :)
    //OR this client (or we) does not support sec ident (IS_UNAVAILABLE) - in that case we "trust" him, too...
    //for other clients, we can only use session data... though we shouldn't care about them too much
    if (HasPassedSecureIdent(true))
        theApp.antileechlist->Verify(this);
//<<< WiZaRd::ClientAnalyzer
}

void CUpDownClient::SendSecIdentStatePacket()
{
    // check if we need public key and signature
    uint8 nValue = 0;
    if (credits)
    {
        if (theApp.clientcredits->CryptoAvailable())
        {
            if (credits->GetSecIDKeyLen() == 0)
                nValue = IS_KEYANDSIGNEEDED;
            else if (m_dwLastSignatureIP != GetIP())
                nValue = IS_SIGNATURENEEDED;
        }
        if (nValue == 0)
        {
            //if (thePrefs.GetLogSecureIdent())
            //	AddDebugLogLine(false, L"Not sending SecIdentState Packet, because State is Zero"));
            return;
        }
        // crypt: send random data to sign
        UINT dwRandom = rand()+1;
        credits->m_dwCryptRndChallengeFor = dwRandom;
        Packet* packet = new Packet(OP_SECIDENTSTATE,5,OP_EMULEPROT);
        theStats.AddUpDataOverheadOther(packet->size);
        packet->pBuffer[0] = nValue;
        PokeUInt32(packet->pBuffer+1, dwRandom);
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__SecIdentState", this);
        SendPacket(packet, true);
    }
    else
        ASSERT(false);
}

void CUpDownClient::ProcessSecIdentStatePacket(const uchar* pachPacket, UINT nSize)
{
    if (nSize != 5)
        return;
    if (!credits)
    {
        ASSERT(false);
        return;
    }
    switch (pachPacket[0])
    {
    case 0:
        m_SecureIdentState = IS_UNAVAILABLE;
        break;
    case 1:
        m_SecureIdentState = IS_SIGNATURENEEDED;
        break;
    case 2:
        m_SecureIdentState = IS_KEYANDSIGNEEDED;
        break;
    }
    credits->m_dwCryptRndChallengeFrom = PeekUInt32(pachPacket+1);
}

void CUpDownClient::InfoPacketsReceived()
{
    // indicates that both Information Packets has been received
    // needed for actions, which process data from both packets
    ASSERT(m_byInfopacketsReceived == IP_BOTH);
    m_byInfopacketsReceived = IP_NONE;

    if (m_bySupportSecIdent)
    {
        SendSecIdentStatePacket();
    }
}

void CUpDownClient::ResetFileStatusInfo()
{
    delete[] m_abyPartStatus;
    m_abyPartStatus = NULL;
//>>> WiZaRd::ICS [enkeyDev]
    delete[] m_abyIncPartStatus;
    m_abyIncPartStatus = NULL;
//<<< WiZaRd::ICS [enkeyDev]
//>>> WiZaRd::AntiHideOS [netfinity]
    delete[] m_abySeenPartStatus;
    m_abySeenPartStatus = NULL;
//<<< WiZaRd::AntiHideOS [netfinity]
    m_nRemoteQueueRank = 0;
    m_nPartCount = 0;
    m_strClientFilename.Empty();
    m_bCompleteSource = false;
    m_uFileRating = 0;
    m_strFileComment.Empty();
    delete m_pReqFileAICHHash;
    m_pReqFileAICHHash = NULL;
}

bool CUpDownClient::IsBanned() const
{
//>>> WiZaRd::Optimization
    if (GetUploadState() == US_BANNED)
        return true;
//<<< WiZaRd::Optimization
    return theApp.clientlist->IsBannedClient(GetConnectIP());
}

void CUpDownClient::SendPreviewRequest(const CAbstractFile* pForFile)
{
    if (m_fPreviewReqPending == 0)
    {
        m_fPreviewReqPending = 1;
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__RequestPreview", this, pForFile->GetFileHash());
        Packet* packet = new Packet(OP_REQUESTPREVIEW,16,OP_EMULEPROT);
        md4cpy(packet->pBuffer,pForFile->GetFileHash());
        theStats.AddUpDataOverheadOther(packet->size);
        SafeConnectAndSendPacket(packet);
    }
    else
    {
        LogWarning(LOG_STATUSBAR, GetResString(IDS_ERR_PREVIEWALREADY));
    }
}

void CUpDownClient::SendPreviewAnswer(const CKnownFile* pForFile, CxImage** imgFrames, uint8 nCount)
{
    m_fPreviewAnsPending = 0;
    CSafeMemFile data(1024);
    if (pForFile)
    {
        data.WriteHash16(pForFile->GetFileHash());
    }
    else
    {
        static const uchar _aucZeroHash[16] = {0};
        data.WriteHash16(_aucZeroHash);
    }
    data.WriteUInt8(nCount);
    for (int i = 0; i != nCount; i++)
    {
        if (imgFrames == NULL)
        {
            ASSERT(false);
            return;
        }
        CxImage* cur_frame = imgFrames[i];
        if (cur_frame == NULL)
        {
            ASSERT(false);
            return;
        }
        BYTE* abyResultBuffer = NULL;
        long nResultSize = 0;
        if (!cur_frame->Encode(abyResultBuffer, nResultSize, CXIMAGE_FORMAT_PNG))
        {
            ASSERT(false);
            return;
        }
        data.WriteUInt32(nResultSize);
        data.Write(abyResultBuffer, nResultSize);
        free(abyResultBuffer);
    }
    Packet* packet = new Packet(&data, OP_EMULEPROT);
    packet->opcode = OP_PREVIEWANSWER;
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__PreviewAnswer", this, (uchar*)packet->pBuffer);
    theStats.AddUpDataOverheadOther(packet->size);
    SafeConnectAndSendPacket(packet);
}

void CUpDownClient::ProcessPreviewReq(const uchar* pachPacket, UINT nSize)
{
    if (nSize < 16)
        throw GetResString(IDS_ERR_WRONGPACKAGESIZE);

    if (m_fPreviewAnsPending || thePrefs.CanSeeShares()==vsfaNobody || (thePrefs.CanSeeShares()==vsfaFriends && !IsFriend()))
        return;

    m_fPreviewAnsPending = 1;
    CKnownFile* previewFile = theApp.sharedfiles->GetFileByID(pachPacket);
    if (previewFile == NULL)
    {
        SendPreviewAnswer(NULL, NULL, 0);
    }
    else
    {
        previewFile->GrabImage(4,0,true,450,this);
    }
}

void CUpDownClient::ProcessPreviewAnswer(const uchar* pachPacket, UINT nSize)
{
    if (m_fPreviewReqPending == 0)
        return;
    m_fPreviewReqPending = 0;
    CSafeMemFile data(pachPacket, nSize);
    uchar Hash[16];
    data.ReadHash16(Hash);
    uint8 nCount = data.ReadUInt8();
    if (nCount == 0)
    {
        LogError(LOG_STATUSBAR, GetResString(IDS_ERR_PREVIEWFAILED), GetUserName());
        return;
    }
    CSearchFile* sfile = theApp.searchlist->GetSearchFileByHash(Hash);
    if (sfile == NULL)
    {
        //already deleted
        return;
    }

    BYTE* pBuffer = NULL;
    try
    {
        for (int i = 0; i != nCount; i++)
        {
            UINT nImgSize = data.ReadUInt32();
            if (nImgSize > nSize)
                throw CString(L"CUpDownClient::ProcessPreviewAnswer - Provided image size exceeds limit");
            pBuffer = new BYTE[nImgSize];
            data.Read(pBuffer, nImgSize);
            CxImage* image = new CxImage(pBuffer, nImgSize, CXIMAGE_FORMAT_PNG);
            delete[] pBuffer;
            pBuffer = NULL;
            if (image->IsValid())
                sfile->AddPreviewImg(image);
            else
                delete image;
        }
    }
    catch (...)
    {
        delete[] pBuffer;
        throw;
    }
    (new PreviewDlg())->SetFile(sfile);
}

// sends a packet, if needed it will establish a connection before
// options used: ignore max connections, control packet, delete packet
// !if the functions returns false that client object was deleted because the connection try failed and the object wasn't needed anymore.
bool CUpDownClient::SafeConnectAndSendPacket(Packet* packet)
{
    if (socket != NULL && socket->IsConnected())
    {
        socket->SendPacket(packet, true, true);
        return true;
    }
    else
    {
        m_WaitingPackets_list.AddTail(packet);
        return TryToConnect(true);
    }
}

bool CUpDownClient::SendPacket(Packet* packet, bool bDeletePacket, bool bVerifyConnection)
{
    if (socket != NULL && (!bVerifyConnection || socket->IsConnected()))
    {
        socket->SendPacket(packet, bDeletePacket, true);
        return true;
    }
    else
    {
        DebugLogError(L"Outgoing packet (0x%X) discarded because expected socket or connection does not exists %s", packet->opcode, DbgGetClientInfo());
        if (bDeletePacket)
            delete packet;
        return false;
    }
}

#ifdef _DEBUG
void CUpDownClient::AssertValid() const
{
    CObject::AssertValid();

	(void)m_iModIconIndex; //>>> WiZaRd::ModIconMappings
//>>> WiZaRd::ClientAnalyzer
    CHECK_PTR(pAntiLeechData);
    (void)m_uiRealVersion;
    CHECK_BOOL(m_bGPLEvilDoerNick);
    CHECK_BOOL(m_bGPLEvilDoerMod);
//<<< WiZaRd::ClientAnalyzer
    (void)m_abyIncPartStatus;  //>>> WiZaRd::ICS [enkeyDev]
    (void)m_abySeenPartStatus; //>>> WiZaRd::AntiHideOS [netfinity]

    CHECK_OBJ(socket);
    CHECK_PTR(credits);
    CHECK_PTR(m_Friend);
    CHECK_OBJ(reqfile);
    (void)m_abyUpPartStatus;
    m_OtherRequests_list.AssertValid();
    m_OtherNoNeeded_list.AssertValid();
    (void)m_lastPartAsked;
    (void)m_cMessagesReceived;
    (void)m_cMessagesSent;
    (void)m_dwUserIP;
    (void)m_dwServerIP;
    (void)m_nUserIDHybrid;
    (void)m_nUserPort;
    (void)m_nServerPort;
    (void)m_nClientVersion;
    (void)m_nUpDatarate;
    (void)m_byEmuleVersion;
    (void)m_byDataCompVer;
    CHECK_BOOL(m_bEmuleProtocol);
    CHECK_BOOL(m_bIsHybrid);
    (void)m_pszUsername;
    (void)m_achUserHash;
    (void)m_achBuddyID;
    (void)m_nBuddyIP;
    (void)m_nBuddyPort;
    (void)m_nUDPPort;
    (void)m_nKadPort;
    (void)m_byUDPVer;
    (void)m_bySourceExchange1Ver;
    (void)m_byAcceptCommentVer;
    (void)m_byExtendedRequestsVer;
    CHECK_BOOL(m_bFriendSlot);
    CHECK_BOOL(m_bCommentDirty);
    CHECK_BOOL(m_bIsML);
    //ASSERT( m_clientSoft >= SO_EMULE && m_clientSoft <= SO_SHAREAZA || m_clientSoft == SO_MLDONKEY || m_clientSoft >= SO_EDONKEYHYBRID && m_clientSoft <= SO_UNKNOWN );
    (void)m_strClientSoftware;
    (void)m_dwLastSourceRequest;
    (void)m_dwLastSourceAnswer;
    (void)m_dwLastAskedForSources;
    (void)m_iFileListRequested;
    (void)m_byCompatibleClient;
    m_WaitingPackets_list.AssertValid();
    m_DontSwap_list.AssertValid();
    (void)m_lastRefreshedDLDisplay;
    ASSERT(m_SecureIdentState >= IS_UNAVAILABLE && m_SecureIdentState <= IS_KEYANDSIGNEEDED);
    (void)m_dwLastSignatureIP;
    ASSERT((m_byInfopacketsReceived & ~IP_BOTH) == 0);
    (void)m_bySupportSecIdent;
    (void)m_nTransferredUp;
    ASSERT(m_nUploadState >= US_UPLOADING && m_nUploadState <= US_NONE);
    (void)m_dwUploadTime;
    (void)m_cAsked;
    (void)m_dwLastUpRequest;
    (void)m_nCurSessionUp;
    (void)m_nCurSessionPayloadUp; //>>> WiZaRd::ZZUL Upload [ZZ]
    (void)m_nCurQueueSessionPayloadUp;
    (void)m_addedPayloadQueueSession;
    (void)m_nUpPartCount;
    (void)m_nUpCompleteSourcesCount;
    (void)s_UpStatusBar;
    (void)requpfileid;
    (void)m_lastRefreshedULDisplay;
    m_AvarageUDR_list.AssertValid();
    m_BlockRequests_queue.AssertValid();
    m_DoneBlocks_list.AssertValid();
    m_RequestedFiles_list.AssertValid();
    ASSERT(m_nDownloadState >= DS_DOWNLOADING && m_nDownloadState <= DS_NONE);
    (void)m_cDownAsked;
    (void)m_abyPartStatus;
    (void)m_strClientFilename;
    (void)m_nTransferredDown;
    (void)m_nCurSessionPayloadDown;
    (void)m_dwDownStartTime;
    (void)m_nLastBlockOffset;
    (void)m_nDownDatarate;
    (void)m_nDownDataRateMS;
    (void)m_nSumForAvgDownDataRate;
    (void)m_cShowDR;
    (void)m_nRemoteQueueRank;
    (void)m_dwLastBlockReceived;
    (void)m_nPartCount;
    ASSERT(m_nSourceFrom >= SF_SERVER && m_nSourceFrom <= SF_LINK);
    CHECK_BOOL(m_bRemoteQueueFull);
    CHECK_BOOL(m_bCompleteSource);
    CHECK_BOOL(m_bReaskPending);
    CHECK_BOOL(m_bUDPPending);
    CHECK_BOOL(m_bTransferredDownMini);
    CHECK_BOOL(m_bUnicodeSupport);
    ASSERT(m_nKadState >= KS_NONE && m_nKadState <= KS_CONNECTING_FWCHECK_UDP);
    m_AvarageDDR_list.AssertValid();
    (void)m_nSumForAvgUpDataRate;
    m_PendingBlocks_list.AssertValid();
    m_DownloadBlocks_list.AssertValid();
    (void)s_StatusBar;
    ASSERT(m_nChatstate >= MS_NONE && m_nChatstate <= MS_UNABLETOCONNECT);
    (void)m_strFileComment;
    (void)m_uFileRating;
#undef CHECK_PTR
#undef CHECK_BOOL
}
#endif

#ifdef _DEBUG
void CUpDownClient::Dump(CDumpContext& dc) const
{
    CObject::Dump(dc);
}
#endif

LPCTSTR CUpDownClient::DbgGetDownloadState() const
{
    const static LPCTSTR apszState[] =
    {
        L"Downloading",
        L"OnQueue",
        L"Connected",
        L"Connecting",
        L"WaitCallback",
        L"WaitCallbackKad",
        L"ReqHashSet",
        L"NoNeededParts",
        L"TooManyConns",
        L"TooManyConnsKad",
        L"LowToLowIp",
        L"Banned",
        L"Error",
        L"None",
        L"RemoteQueueFull"
    };
    if (GetDownloadState() >= ARRSIZE(apszState))
        return L"*Unknown*";
    return apszState[GetDownloadState()];
}

LPCTSTR CUpDownClient::DbgGetUploadState() const
{
    const static LPCTSTR apszState[] =
    {
        L"Uploading",
        L"OnUploadQueue",
        L"Connecting",
        L"Banned",
        L"None"
    };
    if (GetUploadState() >= ARRSIZE(apszState))
        return L"*Unknown*";
    return apszState[GetUploadState()];
}

LPCTSTR CUpDownClient::DbgGetKadState() const
{
    const static LPCTSTR apszState[] =
    {
		L"None",
		L"FwCheckQueued",
		L"FwCheckConnecting",
		L"FwCheckConnected",
		L"BuddyQueued",
		L"BuddyIncoming",
		L"BuddyConnecting",
		L"BuddyConnected",
		L"QueuedFWCheckUDP",
		L"FWCheckUDP",
		L"FwCheckConnectingUDP"
	};
    if (GetKadState() >= ARRSIZE(apszState))
        return L"*Unknown*";
    return apszState[GetKadState()];
}

CString CUpDownClient::DbgGetFullClientSoftVer() const
{
    CString ret = L"";
    if (m_strModVersion.IsEmpty())
        ret = m_strClientSoftware;
    else
        ret.Format(L"%s [%s]", m_strClientSoftware, m_strModVersion);
    return ret;
}

CString CUpDownClient::DbgGetClientInfo(bool bFormatIP) const
{
    CString str;
    if (this != NULL)
    {
        try
        {
            if (HasLowID())
            {
                if (GetConnectIP())
                {
                    str.Format(L"%u@%s (%s) '%s' (%s,%s/%s/%s)",
                               GetUserIDHybrid(), ipstr(GetServerIP()),
                               ipstr(GetConnectIP()),
                               GetUserName(),
                               DbgGetFullClientSoftVer(),
                               DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
                }
                else
                {
                    str.Format(L"%u@%s '%s' (%s,%s/%s/%s)",
                               GetUserIDHybrid(), ipstr(GetServerIP()),
                               GetUserName(),
                               DbgGetFullClientSoftVer(),
                               DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
                }
            }
            else
            {
                str.Format(bFormatIP ? L"%-15s '%s' (%s,%s/%s/%s)" : L"%s '%s' (%s,%s/%s/%s)",
                           ipstr(GetConnectIP()),
                           GetUserName(),
                           DbgGetFullClientSoftVer(),
                           DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
            }
        }
        catch (...)
        {
            str.Format(L"%p - Invalid client instance", this);
        }
    }
    return str;
}

bool CUpDownClient::CheckHandshakeFinished() const
{
    if (m_bHelloAnswerPending)
    {
        // 24-Nov-2004 [bc]: The reason for this is that 2 clients are connecting to each other at the same..
        //if (thePrefs.GetVerbose())
        //	AddDebugLogLine(DLP_VERYLOW, false, L"Handshake not finished - while processing packet: %s; %s", DbgGetClientTCPOpcode(protocol, opcode), DbgGetClientInfo());
        return false;
    }

    return true;
}

void CUpDownClient::OnSocketConnected(int /*nErrorCode*/)
{
}

CString CUpDownClient::GetDownloadStateDisplayString() const
{
    CString strState;
    switch (GetDownloadState())
    {
    case DS_CONNECTING:
        strState = GetResString(IDS_CONNECTING);
        break;
    case DS_CONNECTED:
        strState = GetResString(IDS_ASKING);
        break;
    case DS_WAITCALLBACK:
        strState = GetResString(IDS_CONNVIASERVER);
        break;
    case DS_ONQUEUE:
        if (IsRemoteQueueFull())
            strState = GetResString(IDS_QUEUEFULL);
        else
            strState = GetResString(IDS_ONQUEUE);
        break;
    case DS_DOWNLOADING:
        strState = GetResString(IDS_TRANSFERRING);
        break;
    case DS_REQHASHSET:
        strState = GetResString(IDS_RECHASHSET);
        break;
    case DS_NONEEDEDPARTS:
        strState = GetResString(IDS_NONEEDEDPARTS);
        break;
    case DS_LOWTOLOWIP:
        strState = GetResString(IDS_NOCONNECTLOW2LOW);
        break;
    case DS_TOOMANYCONNS:
        strState = GetResString(IDS_TOOMANYCONNS);
        break;
    case DS_ERROR:
        strState = GetResString(IDS_ERROR);
        break;
    case DS_WAITCALLBACKKAD:
        strState = GetResString(IDS_KAD_WAITCBK);
        break;
    case DS_TOOMANYCONNSKAD:
        strState = GetResString(IDS_KAD_TOOMANDYKADLKPS);
        break;
    }

    return strState;
}

CString CUpDownClient::GetUploadStateDisplayString() const
{
    CString strState;
    switch (GetUploadState())
    {
    case US_ONUPLOADQUEUE:
        strState = GetResString(IDS_ONQUEUE);
        break;
    case US_BANNED:
        strState = GetResString(IDS_BANNED);
        break;
    case US_CONNECTING:
        strState = GetResString(IDS_CONNECTING);
        break;
    case US_UPLOADING:
//>>> WiZaRd::ZZUL Upload [ZZ]
        if (IsScheduledForRemoval())
            strState = GetScheduledRemovalDisplayReason();
        else
//<<< WiZaRd::ZZUL Upload [ZZ]
            if (GetPayloadInBuffer() == 0 && GetNumberOfRequestedBlocksInQueue() == 0 && thePrefs.IsExtControlsEnabled())
                strState = GetResString(IDS_US_STALLEDW4BR);
            else if (GetPayloadInBuffer() == 0 && thePrefs.IsExtControlsEnabled())
                strState = GetResString(IDS_US_STALLEDREADINGFDISK);
            else if (GetSlotNumber() <= theApp.uploadqueue->GetActiveUploadsCount())
                strState = GetResString(IDS_TRANSFERRING);
            else
                strState = GetResString(IDS_TRICKLING);
        break;
    }

    return strState;
}

void CUpDownClient::SendPublicIPRequest()
{
    if (socket && socket->IsConnected())
    {
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__PublicIPReq", this);
        Packet* packet = new Packet(OP_PUBLICIP_REQ,0,OP_EMULEPROT);
        theStats.AddUpDataOverheadOther(packet->size);
        SendPacket(packet, true);
        m_fNeedOurPublicIP = 1;
    }
}

void CUpDownClient::ProcessPublicIPAnswer(const BYTE* pbyData, UINT uSize)
{
    if (uSize != 4)
        throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
    UINT dwIP = PeekUInt32(pbyData);
    if (m_fNeedOurPublicIP == 1)  // did we?
    {
        m_fNeedOurPublicIP = 0;
        if (theApp.GetPublicIP() == 0 && !::IsLowID(dwIP))
            theApp.SetPublicIP(dwIP);
    }
}

void CUpDownClient::CheckFailedFileIdReqs(const uchar* aucFileHash)
{
    if (aucFileHash != NULL && (theApp.sharedfiles->IsUnsharedFile(aucFileHash) || theApp.downloadqueue->GetFileByID(aucFileHash)))
        return;
    //if (GetDownloadState() != DS_DOWNLOADING) // filereq floods are never allowed!
    {
        if (m_fFailedFileIdReqs < 6)// NOTE: Do not increase this nr. without increasing the bits for 'm_fFailedFileIdReqs'
            m_fFailedFileIdReqs++;
        if (m_fFailedFileIdReqs == 6)
        {
            if (theApp.clientlist->GetBadRequests(this) < 2)
                theApp.clientlist->TrackBadRequest(this, 1);
            if (theApp.clientlist->GetBadRequests(this) == 2)
            {
                theApp.clientlist->TrackBadRequest(this, -2); // reset so the client will not be rebanned right after the ban is lifted
                Ban(L"FileReq flood");
            }
            throw CString(thePrefs.GetLogBannedClients() ? L"FileReq flood" : L"");
        }
    }
}

EUtf8Str CUpDownClient::GetUnicodeSupport() const
{
    if (m_bUnicodeSupport)
        return utf8strRaw;
    return utf8strNone;
}

void CUpDownClient::SetSpammer(bool bVal)
{
//>>> WiZaRd::ClientAnalyzer
    //no need to ban them... those spams don't hurt us (except a little overhead)
    if (bVal && pAntiLeechData)
        pAntiLeechData->AddSpam();

    if (bVal && !m_fIsSpammer && thePrefs.GetVerbose())
        AddDebugLogLine(false, L"'%s' has been marked as spammer", DbgGetClientInfo());
//<<< WiZaRd::ClientAnalyzer

    m_fIsSpammer = bVal ? 1 : 0;
}

void  CUpDownClient::SetMessageFiltered(bool bVal)
{
    m_fMessageFiltered = bVal ? 1 : 0;
}

bool  CUpDownClient::IsObfuscatedConnectionEstablished() const
{
    if (socket != NULL && socket->IsConnected())
        return socket->IsObfuscating();
    else
        return false;
}

bool CUpDownClient::ShouldReceiveCryptUDPPackets() const
{
    return (SupportsCryptLayer() && theApp.GetPublicIP() != 0
            && HasValidHash() && (thePrefs.IsClientCryptLayerRequested() || RequestsCryptLayer()));
}

void CUpDownClient::ProcessChatMessage(CSafeMemFile* data, UINT nLength)
{
    //filter me?
    if ((thePrefs.MsgOnlyFriends() && !IsFriend()) || (thePrefs.MsgOnlySecure() && GetUserName()==NULL))
    {
        if (!GetMessageFiltered())
        {
            if (thePrefs.GetVerbose())
                AddDebugLogLine(false,L"Filtered Message from '%s' (IP:%s)", GetUserName(), ipstr(GetConnectIP()));
        }
        SetMessageFiltered(true);
        return;
    }

    CString strMessage(data->ReadString(GetUnicodeSupport()!=utf8strNone, nLength));
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        Debug(L"  %s\n", strMessage);

    // default filtering
    CString strMessageCheck(strMessage);
    strMessageCheck.MakeLower();

//>>> WiZaRd::ClientAnalyzer
    strMessage.Trim();
    if (strMessage.IsEmpty())
    {
        if (!IsFriend() && GetMessagesSent() == 0)
        {
            SetSpammer(true);
            if (!GetMessageFiltered() && thePrefs.GetVerbose())
                AddDebugLogLine(false, L"Filtered Message \"%s\" from %s (filtering applied)", strMessage, DbgGetClientInfo());
            theApp.emuledlg->chatwnd->chatselector.EndSession(this);
        }
        else if (pAntiLeechData)
            pAntiLeechData->AddSpam();
        return;
    }
//<<< WiZaRd::ClientAnalyzer

    CString resToken;
    int curPos = 0;
    resToken = thePrefs.GetMessageFilter().Tokenize(L"|", curPos);
    while (!resToken.IsEmpty())
    {
        resToken.Trim();
        if (strMessageCheck.Find(resToken.MakeLower()) > -1)
        {
            if (!IsFriend() && GetMessagesSent() == 0)
            {
                SetSpammer(true);
                theApp.emuledlg->chatwnd->chatselector.EndSession(this);
            }
//>>> WiZaRd::ClientAnalyzer
            else if (pAntiLeechData)
                pAntiLeechData->AddSpam();
//<<< WiZaRd::ClientAnalyzer
            return;
        }
        resToken = thePrefs.GetMessageFilter().Tokenize(L"|", curPos);
    }

    // advanced spamfilter check
    if (!IsFriend())
    {
        // captcha checks outrank any further checks - if the captcha has been solved, we assume its no spam
        // first check if we need to sent a captcha request to this client
        if (GetMessagesSent() == 0 && GetMessagesReceived() == 0 && GetChatCaptchaState() != CA_CAPTCHASOLVED)
        {
            // we have never sent a message to this client, and no message from him has ever passed our filters
            if (GetChatCaptchaState() != CA_CHALLENGESENT)
            {
                // we also aren't currently expecting a cpatcha response
                if (m_fSupportsCaptcha != NULL)
                {
                    // and he supports captcha, so send him on and store the message (without showing for now)
                    if (m_cCaptchasSent < 3) // no more than 3 tries
                    {
                        m_strCaptchaPendingMsg = strMessage;
                        CSafeMemFile fileAnswer(1024);
                        fileAnswer.WriteUInt8(0); // no tags, for future use
                        CCaptchaGenerator captcha(4);
                        if (captcha.WriteCaptchaImage(fileAnswer))
                        {
                            m_strCaptchaChallenge = captcha.GetCaptchaText();
                            m_nChatCaptchaState = CA_CHALLENGESENT;
                            m_cCaptchasSent++;
                            Packet* packet = new Packet(&fileAnswer, OP_EMULEPROT, OP_CHATCAPTCHAREQ);
                            theStats.AddUpDataOverheadOther(packet->size);
                            if (!SafeConnectAndSendPacket(packet))
                                return; // deleted client while connecting
                        }
                        else
                        {
                            ASSERT(0);
                            DebugLogError(L"Failed to create Captcha for client %s", DbgGetClientInfo());
                        }
                    }
                }
                else
                {
                    // client doesn't supports captchas, but we require them, tell him that its not going to work out
                    // with an answer message (will not be shown and doesn't counts as sent message)
                    if (m_cCaptchasSent < 1) // dont sent this notifier more than once
                    {
                        m_cCaptchasSent++;
                        // always sent in english
                        CString rstrMessage = L"In order to avoid spam messages, this user requires you to solve a captcha before you can send a message to him. However your client does not support captchas, so you will not be able to chat with this user.";
                        DebugLog(L"Received message from client not supporting captchas, filtered and sent notifier (%s)", DbgGetClientInfo());
                        SendChatMessage(rstrMessage); // could delete client
                    }
                    else
                        DebugLog(L"Received message from client not supporting captchas, filtered, didn't sent notifier (%s)", DbgGetClientInfo());
                }
                return;
            }
            else //(GetChatCaptchaState() == CA_CHALLENGESENT)
            {
                // this message must be the answer to the captcha request we send him, lets verify
                ASSERT(!m_strCaptchaChallenge.IsEmpty());
                if (m_strCaptchaChallenge.CompareNoCase(strMessage.Trim().Right(min(strMessage.GetLength(), m_strCaptchaChallenge.GetLength()))) == 0)
                {
                    // allright
                    DebugLog(L"Captcha solved, showing withheld message (%s)", DbgGetClientInfo());
                    m_nChatCaptchaState = CA_CAPTCHASOLVED; // this state isn't persitent, but the messagecounter will be used to determine later if the captcha has been solved
                    // replace captchaanswer with withheld message and show it
                    strMessage = m_strCaptchaPendingMsg;
                    m_cCaptchasSent = 0;
                    m_strCaptchaChallenge = L"";
                    Packet* packet = new Packet(OP_CHATCAPTCHARES, 1, OP_EMULEPROT, false);
                    packet->pBuffer[0] = 0; // status response
                    theStats.AddUpDataOverheadOther(packet->size);
                    if (!SafeConnectAndSendPacket(packet))
                    {
                        ASSERT(0); // deleted client while connecting
                        return;
                    }
                }
                else  // wrong, cleanup and ignore
                {
                    DebugLogWarning(L"Captcha answer failed (%s)", DbgGetClientInfo());
                    m_nChatCaptchaState = CA_NONE;
                    m_strCaptchaChallenge = L"";
                    m_strCaptchaPendingMsg = L"";
                    Packet* packet = new Packet(OP_CHATCAPTCHARES, 1, OP_EMULEPROT, false);
                    packet->pBuffer[0] = (m_cCaptchasSent < 3)? 1 : 2; // status response
                    theStats.AddUpDataOverheadOther(packet->size);
                    SafeConnectAndSendPacket(packet);
                    return; // nothing more todo
                }
            }
        }
        else
            DEBUG_ONLY(DebugLog(L"Message passed CaptchaFilter - already solved or not needed (%s)", DbgGetClientInfo()));

    }
    if (!IsFriend()) // friends are never spammer... (but what if two spammers are friends :P )
    {
        bool bIsSpam = false;
        if (IsSpammer())
            bIsSpam = true;
        else
        {

            // first fixed criteria: If a client  sends me an URL in his first message before I response to him
            // there is a 99,9% chance that it is some poor guy advising his leech mod, or selling you .. well you know :P
            if (GetMessagesSent() == 0)
            {
                int curPos=0;
                CString resToken = CString(URLINDICATOR).Tokenize(L"|", curPos);
                while (resToken != L"")
                {
                    if (strMessage.Find(resToken) > (-1))
                        bIsSpam = true;
                    resToken= CString(URLINDICATOR).Tokenize(L"|",curPos);
                }
                // second fixed criteria: he sent me 4  or more messages and I didn't answered him once
                if (GetMessagesReceived() > 3)
                    bIsSpam = true;
            }
        }
        if (bIsSpam)
        {
            if (IsSpammer())
            {
                if (thePrefs.GetVerbose())
                    AddDebugLogLine(false, L"'%s' has been marked as spammer", GetUserName());
            }
            SetSpammer(true);
            theApp.emuledlg->chatwnd->chatselector.EndSession(this);
            return;

        }
    }

//>>> WiZaRd::ClientAnalyzer
    if (pAntiLeechData)
        pAntiLeechData->DecSpam();
//<<< WiZaRd::ClientAnalyzer

    theApp.emuledlg->chatwnd->chatselector.ProcessMessage(this, strMessage);
}

void CUpDownClient::ProcessCaptchaRequest(CSafeMemFile* data)
{
    // received a captcha request, check if we actually accept it (only after sending a message ourself to this client)
    if (GetChatCaptchaState() == CA_ACCEPTING && GetChatState() != MS_NONE
            && theApp.emuledlg->chatwnd->chatselector.GetItemByClient(this) != NULL)
    {
        // read tags (for future use)
        uint8 nTagCount = data->ReadUInt8();
        for (UINT i = 0; i < nTagCount; i++)
            CTag tag(data, true);
        // sanitize checks - we want a small captcha not a wallpaper
        UINT nSize = (UINT)(data->GetLength() - data->GetPosition());
        if (nSize > 128 && nSize < 4096)
        {
            ULONGLONG pos = data->GetPosition();
            BYTE* byBuffer = data->Detach();
            CxImage imgCaptcha(&byBuffer[pos], nSize, CXIMAGE_FORMAT_BMP);
            //free(byBuffer);
            if (imgCaptcha.IsValid() && imgCaptcha.GetHeight() > 10 && imgCaptcha.GetHeight() < 50
                    && imgCaptcha.GetWidth() > 10 && imgCaptcha.GetWidth() < 150)
            {
                HBITMAP hbmp = imgCaptcha.MakeBitmap();
                if (hbmp != NULL)
                {
                    m_nChatCaptchaState = CA_CAPTCHARECV;
                    theApp.emuledlg->chatwnd->chatselector.ShowCaptchaRequest(this, hbmp);
                    DeleteObject(hbmp);
                }
                else
                    DebugLogWarning(L"Received captcha request from client, Creating bitmap failed (%s)", DbgGetClientInfo());
            }
            else
                DebugLogWarning(L"Received captcha request from client, processing image failed or invalid pixel size (%s)", DbgGetClientInfo());
        }
        else
            DebugLogWarning(L"Received captcha request from client, size sanitize check failed (%u) (%s)", nSize, DbgGetClientInfo());
    }
    else
        DebugLogWarning(L"Received captcha request from client, but don't accepting it at this time (%s)", DbgGetClientInfo());
}

void CUpDownClient::ProcessCaptchaReqRes(uint8 nStatus)
{
    if (GetChatCaptchaState() == CA_SOLUTIONSENT && GetChatState() != MS_NONE
            && theApp.emuledlg->chatwnd->chatselector.GetItemByClient(this) != NULL)
    {
        ASSERT(nStatus < 3);
        m_nChatCaptchaState = CA_NONE;
        theApp.emuledlg->chatwnd->chatselector.ShowCaptchaResult(this, GetResString((nStatus == 0) ? IDS_CAPTCHASOLVED : IDS_CAPTCHAFAILED));
    }
    else
    {
        m_nChatCaptchaState = CA_NONE;
        DebugLogWarning(L"Received captcha result from client, but don't accepting it at this time (%s)", DbgGetClientInfo());
    }
}

CFriend* CUpDownClient::GetFriend() const
{
    if (m_Friend != NULL && theApp.friendlist->IsValid(m_Friend))
        return m_Friend;
    else if (m_Friend != NULL)
        ASSERT(0);
    return NULL;
}

void CUpDownClient::SendChatMessage(CString strMessage)
{
    CSafeMemFile data;
    data.WriteString(strMessage, GetUnicodeSupport());
    Packet* packet = new Packet(&data, OP_EDONKEYPROT, OP_MESSAGE);
    theStats.AddUpDataOverheadOther(packet->size);
    SafeConnectAndSendPacket(packet);
}

bool CUpDownClient::HasPassedSecureIdent(bool bPassIfUnavailable) const
{
    if (credits != NULL)
    {
        if (credits->GetCurrentIdentState(GetConnectIP()) == IS_IDENTIFIED
                || (credits->GetCurrentIdentState(GetConnectIP()) == IS_NOTAVAILABLE && bPassIfUnavailable))
        {
            return true;
        }
    }
    return false;
}

void CUpDownClient::SendFirewallCheckUDPRequest()
{
    ASSERT(GetKadState() == KS_FWCHECK_UDP);
    if (!Kademlia::CKademlia::IsRunning())
    {
        SetKadState(KS_NONE);
        return;
    }
    else if (GetUploadState() != US_NONE || GetDownloadState() != DS_NONE || GetChatState() != MS_NONE
             || GetKadVersion() <= KADEMLIA_VERSION5_48a || GetKadPort() == 0)
    {
        Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, true, ntohl(GetIP()), 0); // inform the tester that this test was cancelled
        SetKadState(KS_NONE);
        return;
    }
    CSafeMemFile data;
    data.WriteUInt16(Kademlia::CKademlia::GetPrefs()->GetInternKadPort());
    data.WriteUInt16(Kademlia::CKademlia::GetPrefs()->GetExternalKadPort());
    data.WriteUInt32(Kademlia::CKademlia::GetPrefs()->GetUDPVerifyKey(GetConnectIP()));
    Packet* packet = new Packet(&data, OP_EMULEPROT, OP_FWCHECKUDPREQ);
    theStats.AddUpDataOverheadKad(packet->size);
    SafeConnectAndSendPacket(packet);
}

void CUpDownClient::ProcessFirewallCheckUDPRequest(CSafeMemFile* data)
{
    if (!Kademlia::CKademlia::IsRunning() || Kademlia::CKademlia::GetUDPListener() == NULL)
    {
        DebugLogWarning(L"Ignored Kad Firewallrequest UDP because Kad is not running (%s)", DbgGetClientInfo());
        return;
    }
    // first search if we know this IP already, if so the result might be biased and we need tell the requester
    bool bErrorAlreadyKnown = false;
    if (GetUploadState() != US_NONE || GetDownloadState() != DS_NONE || GetChatState() != MS_NONE)
        bErrorAlreadyKnown = true;
    else if (Kademlia::CKademlia::GetRoutingZone()->GetContact(ntohl(GetConnectIP()), 0, false) != NULL)
        bErrorAlreadyKnown = true;

    uint16 nRemoteInternPort = data->ReadUInt16();
    uint16 nRemoteExternPort = data->ReadUInt16();
    UINT dwSenderKey = data->ReadUInt32();
    if (nRemoteInternPort == 0)
    {
        DebugLogError(L"UDP Firewallcheck requested with Intern Port == 0 (%s)", DbgGetClientInfo());
        return;
    }
    if (dwSenderKey == 0)
        DebugLogWarning(L"UDP Firewallcheck requested with SenderKey == 0 (%s)", DbgGetClientInfo());

    CSafeMemFile fileTestPacket1;
    fileTestPacket1.WriteUInt8(bErrorAlreadyKnown ? 1 : 0);
    fileTestPacket1.WriteUInt16(nRemoteInternPort);
    if (thePrefs.GetDebugClientKadUDPLevel() > 0)
        DebugSend("KADEMLIA2_FIREWALLUDP", ntohl(GetConnectIP()), nRemoteInternPort);
    Kademlia::CKademlia::GetUDPListener()->SendPacket(&fileTestPacket1, KADEMLIA2_FIREWALLUDP, ntohl(GetConnectIP())
            , nRemoteInternPort, Kademlia::CKadUDPKey(dwSenderKey, theApp.GetPublicIP(false)), NULL);

    // if the client has a router with PAT (and therefore a different extern port than intern), test this port too
    if (nRemoteExternPort != 0 && nRemoteExternPort != nRemoteInternPort)
    {
        CSafeMemFile fileTestPacket2;
        fileTestPacket2.WriteUInt8(bErrorAlreadyKnown ? 1 : 0);
        fileTestPacket2.WriteUInt16(nRemoteExternPort);
        if (thePrefs.GetDebugClientKadUDPLevel() > 0)
            DebugSend("KADEMLIA2_FIREWALLUDP", ntohl(GetConnectIP()), nRemoteExternPort);
        Kademlia::CKademlia::GetUDPListener()->SendPacket(&fileTestPacket2, KADEMLIA2_FIREWALLUDP, ntohl(GetConnectIP())
                , nRemoteExternPort, Kademlia::CKadUDPKey(dwSenderKey, theApp.GetPublicIP(false)), NULL);
    }
    DebugLog(L"Answered UDP Firewallcheck request (%s)", DbgGetClientInfo());
}

void CUpDownClient::SetConnectOptions(uint8 byOptions, bool bEncryption, bool bCallback)
{
    SetCryptLayerSupport((byOptions & 0x01) != 0 && bEncryption);
    SetCryptLayerRequest((byOptions & 0x02) != 0 && bEncryption);
    SetCryptLayerRequires((byOptions & 0x04) != 0 && bEncryption);
    SetDirectUDPCallbackSupport((byOptions & 0x08) != 0 && bCallback);
}

void CUpDownClient::SendSharedDirectories()
{
//>>> WiZaRd::Don't send empty dirs
    CStringList arFolders;
    const bool bFriend = IsFriend();
    theApp.sharedfiles->GetUsefulDirectories(arFolders, bFriend);
    if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && bFriend)) //>>> WiZaRd::SharePermissions - use preferences settings for partial files
    {
        //is not *quite* accurate because completed files will be counted
        // add temporary folder if there are any temp files
        if (theApp.downloadqueue->GetFileCount() > 0)
            arFolders.AddTail(CString(OP_INCOMPLETE_SHARED_FILES));
    }

    if (arFolders.IsEmpty()) // i.e. we don't have any directories to share
    {
        DebugLog(GetResString(IDS_SHAREDREQ1), GetUserName(), GetUserIDHybrid(), GetResString(IDS_DENIED));
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__AskSharedDeniedAnswer", this);
        Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
        theStats.AddUpDataOverheadOther(replypacket->size);
        SendPacket(replypacket, true, true);
    }
    else
    {

        // build packet
        CSafeMemFile tempfile(80);
        tempfile.WriteUInt32(arFolders.GetCount());
        while (!arFolders.IsEmpty())
            tempfile.WriteString(arFolders.RemoveHead(), GetUnicodeSupport());

        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__AskSharedDirsAnswer", this);
        Packet* replypacket = new Packet(&tempfile);
        replypacket->opcode = OP_ASKSHAREDDIRSANS;
        theStats.AddUpDataOverheadOther(replypacket->size);
        VERIFY(SendPacket(replypacket, true, true));
    }
//<<< WiZaRd::Don't send empty dirs
}

//>>> WiZaRd::ClientAnalyzer
//>>> WiZaRd::More GPLEvilDoers
//these clients CAN IMPOSSIBLY be used as "good" mods - thus they do not deserve a single byte - very sad...
//personally, I'd like to send a message to those users to notify them because most do not know what they do
//but this could be considered spam... :(
#define MISC_GPL_CHECKS \
	|| strBuffer.Find(L"APPLEJUICE") != -1 \
	|| strBuffer.Find(L"WIKINGER") != -1 \
	|| strBuffer.Find(L"RC-ATLANTIS") != -1 \
	|| strBuffer.Find(L"SUNPOWER") != -1 \
	|| strBuffer.Find(L"T1N 8OOSTER") != -1 \
	|| strBuffer.Find(L"T L N B O O S T") != -1 \
	|| strBuffer.Find(L"T~L~N BOO$T") != -1 \
	|| strBuffer.Find(L"TLH 8OOSTER") != -1 \
	|| strBuffer.Find(L"TLH 8ooster") != -1 \
	|| strBuffer.Find(L"NFO.CO.IL") != -1 \
	|| strBuffer.Find(L"FIREBALL") != -1 \
	|| strBuffer.Find(L"TITANDONKEY") != -1 \
	|| strBuffer.Find(L"TITANMULE") != -1 \
	|| strBuffer.Find(L"ROCKFORCE") != -1

void CUpDownClient::CheckForGPLEvilDoer(const bool bNick, const bool bMod)
{
    if (bNick)
    {
        // check for known major gpl breaker
        CString strBuffer = m_pszUsername;
        strBuffer.MakeUpper();
        m_bGPLEvilDoerNick = (strBuffer.Find(L"EMULE-CLIENT") != -1
                              || strBuffer.Find(L"POWERMULE") != -1
                              MISC_GPL_CHECKS);
    }
    if (bMod)
    {
        CString strBuffer = m_strModVersion;
        strBuffer.TrimLeft(); // skip leading spaces
        strBuffer.MakeUpper();

        // check for known major gpl breaker
        m_bGPLEvilDoerMod = (_tcsncmp(strBuffer, L"LH", 2) == 0
                             || _tcsncmp(strBuffer, L"LIO", 3) == 0
                             || _tcsncmp(strBuffer, L"PLUS PLUS", 9) == 0
                             MISC_GPL_CHECKS);
    }
}

#undef MISC_GPL_CHECKS

bool CUpDownClient::IsBadGuy() const
{
    if (pAntiLeechData == NULL)
        return false;
    return pAntiLeechData->IsBadGuy();
}

Packet* GetEmptyXSPacket(const CUpDownClient* forClient, CKnownFile* kreqfile, uint8 byRequestedVersion, uint16 nRequestedOptions)
{
    CSafeMemFile data(1024);

    uint8 byUsedVersion;
    bool bIsSX2Packet;
    if (forClient->SupportsSourceExchange2() && byRequestedVersion > 0)
    {
        // the client uses SourceExchange2 and requested the highest version he knows
        // and we send the highest version we know, but of course not higher than his request
        byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
        bIsSX2Packet = true;
        data.WriteUInt8(byUsedVersion);

        // we don't support any special SX2 options yet, reserved for later use
        if (nRequestedOptions != 0)
            DebugLogWarning(L"Client requested unknown options for SourceExchange2: %u (%s)", nRequestedOptions, forClient->DbgGetClientInfo());
    }
    else
    {
        byUsedVersion = forClient->GetSourceExchange1Version();
        bIsSX2Packet = false;
        if (forClient->SupportsSourceExchange2())
            DebugLogWarning(L"Client which announced to support SX2 sent SX1 packet instead (%s)", forClient->DbgGetClientInfo());
    }

    data.WriteHash16(kreqfile->GetFileIdentifier().GetMD4Hash());
    data.WriteUInt16(0);

    Packet* result = new Packet(&data, OP_EMULEPROT);
    result->opcode = bIsSX2Packet ? OP_ANSWERSOURCES2 : OP_ANSWERSOURCES;
    // (1+)16+2+501*(4+2+4+2+16+1) = 14547 (14548) bytes max.
    //	if (result->size > 354)
    //		result->PackPacket();
    if (thePrefs.GetDebugSourceExchange())
        AddDebugLogLine(false, L"SXSend: Client source response SX2=%s, Version=%u; Count=%u, %s, File=\"%s\"", bIsSX2Packet ? L"Yes" : L"No", byUsedVersion, 0, forClient->DbgGetClientInfo(), kreqfile->GetFileName());
    return result;
}

void	CUpDownClient::ProcessSourceRequest(const BYTE* packet, const UINT size, const UINT opcode, const UINT uRawSize, CSafeMemFile* data, CKnownFile* kreqfile)
{
    const bool bMultipacket = data != NULL;
    if (!bMultipacket)
        data = new CSafeMemFile(packet, size);
    const bool bXS2 = opcode == OP_REQUESTSOURCES2;
    if (thePrefs.GetDebugClientTCPLevel() > 0)
    {
        if (bMultipacket)
            DebugRecv(bXS2 ? "OP_MPRequestSources2" : "OP_MPRequestSources", this, packet);
        else
            DebugRecv(bXS2 ? "OP_RequestSources2" : "OP_RequestSources", this, (size >= 16) ? packet : NULL);
    }

    if (!bMultipacket)
    {
        theStats.AddDownDataOverheadSourceExchange(uRawSize);
        CheckHandshakeFinished();
    }

    uint8 byRequestedVersion = 0;
    uint16 byRequestedOptions = 0;
    if (bXS2) // SX2 requests contains additional data
    {
        if (!bMultipacket && size != 19)
            throw GetResString(IDS_ERR_BADSIZE);

        byRequestedVersion = data->ReadUInt8();
        byRequestedOptions = data->ReadUInt16();
    }
    else if (!bMultipacket && size != 16)
        throw GetResString(IDS_ERR_BADSIZE);

    //we will answer even if it IS an exploiter to us because we don't want to appear as exploiters ourselves...
    //if(pAntiLeechData && pAntiLeechData->IsXSExploiter())
    //	break;

    if (thePrefs.GetDebugSourceExchange())
    {
        if (bMultipacket)
            AddDebugLogLine(false, L"SXRecv: Client source request; %s, File=\"%s\"", DbgGetClientInfo(), kreqfile->GetFileName());
        else
            AddDebugLogLine(false, L"SXRecv: Client source request; %s, %s", DbgGetClientInfo(), DbgGetFileInfo(packet));
    }

    //Although this shouldn't happen, it's a just in case to any Mods that mess with version numbers.
    if (byRequestedVersion > 0 || GetSourceExchange1Version() > 1)
    {
        if (!bMultipacket)
        {
            //first check shared file list, then download list
            uchar ucHash[16];
            data->ReadHash16(ucHash);

            kreqfile = theApp.sharedfiles->GetFileByID(ucHash);
            if (kreqfile == NULL)
                kreqfile = theApp.downloadqueue->GetFileByID(ucHash);
            if (kreqfile)
            {
                // There are some clients which do not follow the correct protocol procedure of sending
                // the sequence OP_REQUESTFILENAME, OP_SETREQFILEID, OP_REQUESTSOURCES. If those clients
                // are doing this, they will not get the optimal set of sources which we could offer if
                // the would follow the above noted protocol sequence. They better to it the right way
                // or they will get just a random set of sources because we do not know their download
                // part status which may get cleared with the call of 'SetUploadFileID'.
                SetUploadFileID(kreqfile);
            }
            else
            {
                CheckFailedFileIdReqs(ucHash);
                delete data;
                return;
            }
        }

        DWORD dwTimePassed = ::GetTickCount() - GetLastSrcReqTime() + CONNECTION_LATENCY;
        bool bNeverAskedBefore = GetLastSrcReqTime() == 0;
        if (bNeverAskedBefore ||
                //if not complete and file is rare
                (kreqfile->IsPartFile()
                 && dwTimePassed > SOURCECLIENTREASKS
                 && ((CPartFile*)kreqfile)->GetSourceCount() <= RARE_FILE
                ) ||
                //OR if file is not rare or if file is complete
                dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY
           )
        {
            if (pAntiLeechData)
                pAntiLeechData->DecFastXSCounter(); //valid request

            SetLastSrcReqTime();
            Packet* tosend = kreqfile->CreateSrcInfoPacket(this, byRequestedVersion, byRequestedOptions);
            //WiZaRd: explicitly answer with "0" sources so the remote client doesn't hammer us
            if (tosend == NULL)
                tosend = GetEmptyXSPacket(this, kreqfile, byRequestedVersion, byRequestedOptions);
            if (tosend)
            {
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                    DebugSend("OP__AnswerSources", this, kreqfile->GetFileHash());
                theStats.AddUpDataOverheadSourceExchange(tosend->size);
                //SendPacket(tosend, true, false); //, !bMultipacket
                SendPacket(tosend, true, true); //asserts in emsocket ~ line 1526
            }
        }
        else
        {
            if (pAntiLeechData)
            {
                pAntiLeechData->IncFastXSCounter(); //asked too fast
                if (pAntiLeechData->IsXSSpammer())
                {
                    //we don't ban anyone here...
                }
            }

            //WiZaRd: explicitly answer with "0" sources so the remote client doesn't hammer us
            SetLastSrcReqTime();
            Packet* tosend = GetEmptyXSPacket(this, kreqfile, byRequestedVersion, byRequestedOptions);
            if (tosend)
            {
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                    DebugSend("OP__AnswerSources", this, kreqfile->GetFileHash());
                theStats.AddUpDataOverheadSourceExchange(tosend->size);
                //SendPacket(tosend, true, false); //, !bMultipacket
                SendPacket(tosend, true, true); //asserts in emsocket ~ line 1526
            }
        }
    }
//>>> WiZaRd
    else
        Ban(L"Src-Req with wrong version!");
//<<< WiZaRd
    if (!bMultipacket)
        delete data;
}

void	CUpDownClient::ProcessSourceAnswer(const BYTE* packet, const UINT size, const UINT opcode, const UINT uRawSize)
{
    const bool bXS2 = opcode == OP_ANSWERSOURCES || opcode == OP_ANSWERSOURCES2;
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugRecv(bXS2 ? "OP_AnswerSources2" : "OP_AnswerSources", this, (size >= (UINT)(16+bXS2 ? 1 : 0)) ? packet : NULL);
    theStats.AddDownDataOverheadSourceExchange(uRawSize);

//>>> Security Check
    if (m_fSourceExchangeRequested == FALSE)
    {
        CString msg = L"";
        msg.Format(L"Client %s sent XS response without request!", DbgGetClientInfo());
        throw msg;
    }
    m_fSourceExchangeRequested = FALSE;
//<<< Security Check

    if (pAntiLeechData)
        pAntiLeechData->IncXSAnsw();

    CSafeMemFile data(packet, size);
    uint8 byVersion = bXS2 ? data.ReadUInt8() : GetSourceExchange1Version();
    uchar hash[16];
    data.ReadHash16(hash);
    CKnownFile* file = theApp.downloadqueue->GetFileByID(hash);
    //set the client's answer time
    SetLastSrcAnswerTime();
    if (file)
    {
        if (file->IsPartFile())
        {
// 			//set the client's answer time
// 			SetLastSrcAnswerTime();
            //and set the file's last answer time
            ((CPartFile*)file)->SetLastAnsweredTime();
            ((CPartFile*)file)->AddClientSources(&data, byVersion, bXS2, this);
        }
    }
    else
        CheckFailedFileIdReqs(hash);
}

bool CUpDownClient::HasValidHash() const
{
    return !isnulmd4(m_achUserHash);
}

bool CUpDownClient::DownloadingAndUploadingFilesAreEqual(CKnownFile* reqfile)
{
    return IsCompleteSource() && GetRequestFile() == reqfile;
}
//<<< WiZaRd::ClientAnalyzer

void	CUpDownClient::SetFriendSlot(const bool bNV)
{
    const bool oldValue = m_bFriendSlot;
    m_bFriendSlot = bNV;
//>>> WiZaRd::ZZUL Upload [ZZ]
    if (theApp.uploadqueue && oldValue != m_bFriendSlot)
        theApp.uploadqueue->ReSortUploadSlots(true);
//<<< WiZaRd::ZZUL Upload [ZZ]
}

//>>> WiZaRd::ZZUL Upload [ZZ]
void	CUpDownClient::ScheduleRemovalFromUploadQueue(LPCTSTR pszDebugReason, CString strDisplayReason, bool keepWaitingTimeIntact)
{
    if (!m_bScheduledForRemoval)
    {
        m_bScheduledForRemoval = true;
        m_pszScheduledForRemovalDebugReason = pszDebugReason;
        m_strScheduledForRemovalDisplayReason = strDisplayReason;
        m_bScheduledForRemovalWillKeepWaitingTimeIntact = keepWaitingTimeIntact;
        m_bScheduledForRemovalAtTick = ::GetTickCount();
    }
}

void	CUpDownClient::UnscheduleForRemoval()
{
    m_bScheduledForRemoval = false;

    if (GetQueueSessionPayloadUp() > GetCurrentSessionLimit())
        ++m_curSessionAmountNumber;
}

bool	CUpDownClient::IsScheduledForRemoval() const
{
    return m_bScheduledForRemoval;
}

bool	CUpDownClient::GetScheduledUploadShouldKeepWaitingTime() const
{
    return m_bScheduledForRemovalWillKeepWaitingTimeIntact;
}

LPCTSTR	CUpDownClient::GetScheduledRemovalDebugReason() const
{
    return m_pszScheduledForRemovalDebugReason;
}

CString CUpDownClient::GetScheduledRemovalDisplayReason() const
{
    return m_strScheduledForRemovalDisplayReason;
}

bool	CUpDownClient::GetScheduledRemovalLimboComplete() const
{
    return m_bScheduledForRemoval && ::GetTickCount()-m_bScheduledForRemovalAtTick > SEC2MS(10);
}
//<<< WiZaRd::ZZUL Upload [ZZ]
//>>> WiZaRd::ModIconMapper
void	CUpDownClient::CheckModIconIndex()
{
	if(m_strModVersion.IsEmpty())
	{
		m_iModIconIndex = MODMAP_NONE;
		return;
	}

	m_iModIconIndex = theApp.theModIconMap->GetIconIndexForModstring(m_strModVersion);
}

int	CUpDownClient::GetModIconIndex() const
{
	//if he's a bad one, just return that icon straight away
	if(IsBadGuy())
		return MODMAP_BADGUY;
	return m_iModIconIndex;
}
//<<< WiZaRd::ModIconMapper