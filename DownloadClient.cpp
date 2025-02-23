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
#include "emule.h"
#include <zlib/zlib.h>
#include "UpDownClient.h"
#include "PartFile.h"
#include "OtherFunctions.h"
#include "ListenSocket.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "Packets.h"
#include "Statistics.h"
#include "ClientCredits.h"
#include "DownloadQueue.h"
#include "ClientUDPSocket.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "Exceptions.h"
#include "clientlist.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "Kademlia/Kademlia/Search.h"
#include "SHAHashSet.h"
#include "SharedFileList.h"
#include "Log.h"
//>>> WiZaRd::ClientAnalyzer
#include "UploadQueue.h"
#include "./Mod/ClientAnalyzer.h"
//<<< WiZaRd::ClientAnalyzer
#include "./Mod/NetF/PartStatus.h" //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//	members of CUpDownClient
//	which are mainly used for downloading functions
CBarShader CUpDownClient::s_StatusBar(16);
void CUpDownClient::DrawStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool  bFlat) const
{
    COLORREF crNeither;
    COLORREF crBoth;
    COLORREF crClientOnly;
    COLORREF crPending;
    COLORREF crNextPending;
    COLORREF crClientPartial = RGB(170, 50, 224); //>>> WiZaRd::ICS [enkeyDEV]
#ifdef ANTI_HIDEOS
    COLORREF crClientSeen = RGB(150, 240, 240); //>>> WiZaRd::AntiHideOS [netfinity]
#endif
    COLORREF crSCT = RGB(255, 128, 0); //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]

    if (bFlat)
    {
        crNeither = RGB(224, 224, 224);
        crBoth = RGB(0, 0, 0);
        crClientOnly = RGB(0, 100, 255);
        crPending = RGB(0, 150, 0);
        crNextPending = RGB(255, 208, 0);
    }
    else
    {
        crNeither = RGB(240, 240, 240);
        crBoth = RGB(104, 104, 104);
        crClientOnly = RGB(0, 100, 255);
        crPending = RGB(0, 150, 0);
        crNextPending = RGB(255, 208, 0);
    }

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    const CPartStatus* abyPartStatus = GetPartStatus();
    UINT nPartCount = GetPartCount();
    const UINT nStepCount = (abyPartStatus && SupportsSCT()) ? abyPartStatus->GetCrumbsCount() : nPartCount;
    EMFileSize filesize = PARTSIZE;
    if (reqfile)
        filesize = reqfile->GetFileSize();
    else
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        if (abyPartStatus && SupportsSCT())
            filesize = (uint64)(CRUMBSIZE * (uint64)abyPartStatus->GetCrumbsCount());
        else
            filesize = (uint64)(PARTSIZE * (uint64)nPartCount);
    //filesize = (uint64)(PARTSIZE * (uint64)nPartCount);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

    s_StatusBar.SetFileSize(filesize);
    s_StatusBar.SetHeight(rect->bottom - rect->top);
    s_StatusBar.SetWidth(rect->right - rect->left);
    s_StatusBar.Fill(crNeither);

    uint64 uStart = 0;
    uint64 uEnd = 0;
    if (!onlygreyrect
            && (abyPartStatus
                || m_abyIncPartStatus	//>>> WiZaRd::ICS [enkeyDEV]
#ifdef ANTI_HIDEOS
                || m_abySeenPartStatus	//>>> WiZaRd::AntiHideOS [netfinity]
#endif
               )
       )
    {
        BYTE* pcNextPendingBlks = NULL;
        if (m_nDownloadState == DS_DOWNLOADING)
        {
            pcNextPendingBlks = new BYTE[nPartCount];
            memset(pcNextPendingBlks, 0, nPartCount); // do not use '_strnset' for uninitialized memory!
            for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != 0;)
            {
                const UINT uPart = (UINT)(m_PendingBlocks_list.GetNext(pos)->block->StartOffset / PARTSIZE);
                if (uPart < nPartCount)
                    pcNextPendingBlks[uPart] = 1;
            }
        }

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        bool bCompletePart = abyPartStatus && SupportsSCT() ? abyPartStatus->IsCompletePart(0) : false;
        const uint64 partsize = abyPartStatus && SupportsSCT() ? CRUMBSIZE : PARTSIZE;
        for (UINT i = 0, nPart = 0; i < nStepCount;)
            //for (UINT i = 0; i < nPartCount; ++i)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        {
            GetPartStartAndEnd(i, partsize, filesize, uStart, uEnd);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
            //if (IsPartAvailable(i))
            if (abyPartStatus && abyPartStatus->IsComplete(uStart, uEnd-1))
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
            {
                if (reqfile->IsComplete(uStart, uEnd-1, false))
                    s_StatusBar.FillRange(uStart, uEnd, crBoth);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                else if (SupportsSCT() && !bCompletePart)
                    s_StatusBar.FillRange(uStart, uEnd, crSCT);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                else if (GetSessionDown() > 0 && m_nDownloadState == DS_DOWNLOADING && m_nLastBlockOffset >= uStart && m_nLastBlockOffset < uEnd)
                    s_StatusBar.FillRange(uStart, uEnd, crPending);
                else if (pcNextPendingBlks != NULL && pcNextPendingBlks[nPart] == 1)
                    s_StatusBar.FillRange(uStart, uEnd, crNextPending);
                else
                    s_StatusBar.FillRange(uStart, uEnd, crClientOnly);
            }
#ifdef ANTI_HIDEOS
//>>> WiZaRd::AntiHideOS [netfinity]
            else if (m_abySeenPartStatus && m_abySeenPartStatus[nPart])
                s_StatusBar.FillRange(uStart, uEnd, crClientSeen);
//<<< WiZaRd::AntiHideOS [netfinity]
#endif
//>>> WiZaRd::ICS [enkeyDEV]
            else if (m_abyIncPartStatus && m_abyIncPartStatus[nPart])
                s_StatusBar.FillRange(uStart, uEnd, crClientPartial);
//<<< WiZaRd::ICS [enkeyDEV]

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
            ++i;
            if (abyPartStatus && SupportsSCT())
            {
                if ((i % CRUMBSPERPART) == 0)
                {
                    ++nPart;
                    bCompletePart = nPart < abyPartStatus->GetPartCount() && abyPartStatus->IsCompletePart(nPart);
                }
            }
            else
                ++nPart;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        }
        delete[] pcNextPendingBlks;
    }
    s_StatusBar.Draw(dc, rect->left, rect->top, bFlat);
}

bool CUpDownClient::Compare(const CUpDownClient* tocomp, bool bIgnoreUserhash) const
{
    //Compare only the user hash..
    if (!bIgnoreUserhash && HasValidHash() && tocomp->HasValidHash())
        return !md4cmp(this->GetUserHash(), tocomp->GetUserHash());

    if (HasLowID())
    {
        //User is firewalled.. Must do two checks..
#ifdef IPV6_SUPPORT
        if ((!GetIPv4().IsNull() && GetIPv4() == tocomp->GetIPv4()) || (!GetIPv6().IsNull() && GetIPv6() == tocomp->GetIPv6())) //>>> WiZaRd::IPv6 [Xanatos]
#else
        if (GetIP()!=0 && GetIP() == tocomp->GetIP())
#endif
        {
            //The IP of both match
            if (GetUserPort()!=0 && GetUserPort() == tocomp->GetUserPort())
                //IP-UserPort matches
                return true;
            if (GetKadPort()!=0	&& GetKadPort() == tocomp->GetKadPort())
                //IP-KadPort Matches
                return true;
        }
        if (GetUserIDHybrid()!=0
                && GetUserIDHybrid() == tocomp->GetUserIDHybrid()
                && GetServerIP()!=0
                && GetServerIP() == tocomp->GetServerIP()
                && GetServerPort()!=0
                && GetServerPort() == tocomp->GetServerPort())
            //Both have the same lowID, Same serverIP and Port..
            return true;

#if defined(_DEBUG)
        if (HasValidBuddyID() && tocomp->HasValidBuddyID())
        {
            //JOHNTODO: This is for future use to see if this will be needed...
            if (!md4cmp(GetBuddyID(), tocomp->GetBuddyID()))
                return true;
        }
#endif

        //Both IP, and Server do not match..
        return false;
    }

#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
    if (((!GetIPv4().IsNull() && GetIPv4() == tocomp->GetIPv4()) || (!GetIPv6().IsNull() && GetIPv6() == tocomp->GetIPv6()))
            || (GetUserIDHybrid()!=0 && GetUserIDHybrid() == tocomp->GetUserIDHybrid())
            || (!GetConnectIP().IsNull() && GetConnectIP() == tocomp->GetConnectIP())) //WiZaRd: fallback for "fresh" clients
//<<< WiZaRd::IPv6 [Xanatos]
#else
    if ((GetIP()!=0 && GetIP() == tocomp->GetIP())
            || (GetUserIDHybrid()!=0 && GetUserIDHybrid() == tocomp->GetUserIDHybrid())
            || (GetConnectIP()!=0 && GetConnectIP() == tocomp->GetConnectIP())) //WiZaRd: fallback for "fresh" clients
#endif
    {
        //The IP of both match
        if (GetUserPort()!=0 && GetUserPort() == tocomp->GetUserPort())
            return true; //IP-UserPort matches
        if (GetKadPort()!=0	&& GetKadPort() == tocomp->GetKadPort())
            return true; //IP-KadPort matches
    }

    //No Matches..
    return false;
}

// Return bool is not if you asked or not..
// false = Client was deleted!
// true = client was not deleted!
bool CUpDownClient::AskForDownload()
{

    if (m_bUDPPending)
    {
        ++m_nFailedUDPPackets;
//>>> WiZaRd::Detect UDP problem clients
        if (m_uiFailedUDPPacketsInARow < 4)
            ++m_uiFailedUDPPacketsInARow;
        if (m_uiFailedUDPPacketsInARow == 4)
            theApp.QueueDebugLogLineEx(LOG_WARNING, L"DBG: Dropped UDP support for %s (%u failed UDP attempts)", DbgGetClientInfo(), m_uiFailedUDPPacketsInARow);
//<<< WiZaRd::Detect UDP problem clients
        theApp.downloadqueue->AddFailedUDPFileReasks();
    }
    m_bUDPPending = false;
    if (!(socket && socket->IsConnected())) // already connected, skip all the special checks
    {
        if (theApp.listensocket->TooManySockets())
        {
            if (GetDownloadState() != DS_TOOMANYCONNS)
                SetDownloadState(DS_TOOMANYCONNS);
            return true;
        }
        m_dwLastTriedToConnect = ::GetTickCount();
        // if its a lowid client which is on our queue we may delay the reask up to 20 min, to give the lowid the chance to
        // connect to us for its own reask
        if (HasLowID() && GetUploadState() == US_ONUPLOADQUEUE && !m_bReaskPending && GetLastAskedTime() > 0)
        {
            SetDownloadState(DS_ONQUEUE);
            m_bReaskPending = true;
            return true;
        }
        // if we are lowid <-> lowid but contacted the source before already, keep it in the hope that we might turn highid again
        if (HasLowID() && !CanDoCallback() && GetLastAskedTime() > 0)
        {
            if (GetDownloadState() != DS_LOWTOLOWIP)
                SetDownloadState(DS_LOWTOLOWIP);
            m_bReaskPending = true;
            return true;
        }
    }

    m_dwLastTriedToConnect = ::GetTickCount();
    SwapToAnotherFile(_T("A4AF check before tcp file reask. CUpDownClient::AskForDownload()"), true, false, false, NULL, true, true);
    SetDownloadState(DS_CONNECTING);
    return TryToConnect();
}

bool CUpDownClient::IsSourceRequestAllowed(const bool bLog) const
{
    return IsSourceRequestAllowed(reqfile, bLog);
}

bool CUpDownClient::IsSourceRequestAllowed(CPartFile* partfile, bool sourceExchangeCheck, const bool bLog) const
{
    //if client has the correct extended protocol
    if (!ExtProtocolAvailable() || !(SupportsSourceExchange2() || GetSourceExchange1Version() > 1))
        return false;

    DWORD dwTickCount = ::GetTickCount() + CONNECTION_LATENCY;
    UINT uSources = partfile->GetSourceCount();
    UINT uValidSources = partfile->GetValidSourcesCount();

    if (partfile != reqfile)
    {
        ++uSources;
        ++uValidSources;
    }

    if (uSources >= reqfile->GetMaxSourcePerFileSoft())
        return false;

    //if the remote client doesn't have any sources to send and we do an XS-request for
    //the next file, it won't send sources, because it thinks it has already answered
    //because of this official bug we refer to GetLastAskedForSources() instead of GetLastSrcAnswerTime()
    const DWORD nTimePassedClient = dwTickCount - GetLastAskedForSources();
    const unsigned int nTimePassedFile   = dwTickCount - partfile->GetLastAnsweredTime();
    const bool bNeverAskedBefore = GetLastAskedForSources() == 0;
    const UINT uReqValidSources = reqfile->GetValidSourcesCount();

    //AND if...
    bool bRequestAllowed = false;
    if (!m_bCompleteSource && (bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASKS))
    {
        //source is not complete and file is very rare
        if (uSources <= RARE_FILE/5 && (!sourceExchangeCheck || partfile == reqfile || uValidSources < uReqValidSources && uReqValidSources > 3))
        {
            bRequestAllowed = true;
            if (bLog)
                theApp.QueueDebugLogLineEx(LOG_WARNING, L"XS request to %s allowed (1) - times passed: client: %s, file: %s", DbgGetClientInfo(), CastSecondsToHM(nTimePassedClient), CastSecondsToHM(nTimePassedFile));
        }
        //source is not complete and file is rare
        else if ((uSources <= RARE_FILE || (!sourceExchangeCheck || partfile == reqfile) && uSources <= RARE_FILE / 2 + uValidSources)
                 && nTimePassedFile > SOURCECLIENTREASKF
                 && (!sourceExchangeCheck || partfile == reqfile || uValidSources < SOURCECLIENTREASKS/SOURCECLIENTREASKF && uValidSources < uReqValidSources))
        {
            bRequestAllowed = true;
            if (bLog)
                theApp.QueueDebugLogLineEx(LOG_WARNING, L"XS request to %s allowed (2) - times passed: client: %s, file: %s", DbgGetClientInfo(), CastSecondsToHM(nTimePassedClient), CastSecondsToHM(nTimePassedFile));
        }
    }
    // OR if file is not rare
    if (!bRequestAllowed && (bNeverAskedBefore || nTimePassedClient > (unsigned)(SOURCECLIENTREASKS * MINCOMMONPENALTY))
            && (nTimePassedFile > (unsigned)(SOURCECLIENTREASKF * MINCOMMONPENALTY))
            && (!sourceExchangeCheck || partfile == reqfile || uValidSources < SOURCECLIENTREASKS/SOURCECLIENTREASKF && uValidSources < uReqValidSources))
    {
        bRequestAllowed = true;
        if (bLog)
            theApp.QueueDebugLogLineEx(LOG_WARNING, L"XS request to %s allowed (3) - times passed: client: %s, file: %s", DbgGetClientInfo(), CastSecondsToHM(nTimePassedClient), CastSecondsToHM(nTimePassedFile));
    }
    if (bLog && !bRequestAllowed)
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"XS request to %s denied - times passed: client: %s, file: %s", DbgGetClientInfo(), CastSecondsToHM(nTimePassedClient), CastSecondsToHM(nTimePassedFile));
    return bRequestAllowed;
}

void CUpDownClient::SendFileRequest()
{
    // normally asktime has already been reset here, but then SwapToAnotherFile will return without much work, so check to make sure
    SwapToAnotherFile(_T("A4AF check before tcp file reask. CUpDownClient::SendFileRequest()"), true, false, false, NULL, true, true);

    ASSERT(reqfile != NULL);
    if (!reqfile)
        return;
    AddAskedCountDown();

    if (SupportMultiPacket() || SupportsFileIdentifiers())
    {
        CSafeMemFile dataFileReq(96);
        if (SupportsFileIdentifiers())
        {
            reqfile->GetFileIdentifier().WriteIdentifier(&dataFileReq);
            if (thePrefs.GetDebugClientTCPLevel() > 0)
                DebugSend("OP__MultiPacket_Ext2", this, reqfile->GetFileHash());
        }
        else
        {
            dataFileReq.WriteHash16(reqfile->GetFileHash());
            if (SupportExtMultiPacket())
            {
                dataFileReq.WriteUInt64(reqfile->GetFileSize());
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                    DebugSend("OP__MultiPacket_Ext", this, reqfile->GetFileHash());
            }
            else
            {
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                    DebugSend("OP__MultiPacket", this, reqfile->GetFileHash());
            }
        }

        // OP_REQUESTFILENAME + ExtInfo
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__MPReqFileName", this, reqfile->GetFileHash());
        dataFileReq.WriteUInt8(OP_REQUESTFILENAME);
        if (GetExtendedRequestsVersion() > 0)
            reqfile->WriteSafePartStatus(&dataFileReq, this); //>>> WiZaRd::Intelligent SOTN
        if (GetExtendedRequestsVersion() > 1)
            reqfile->WriteCompleteSourcesCount(&dataFileReq);

        // OP_SETREQFILEID
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__MPSetReqFileID", this, reqfile->GetFileHash());
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        if (reqfile->GetPartCount() > 1 || SupportsSCT())
            //if (reqfile->GetPartCount() > 1)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
            dataFileReq.WriteUInt8(OP_SETREQFILEID);

//>>> WiZaRd::FiX? Moved down!
        /*if (IsEmuleClient())
        {
            SetRemoteQueueFull(true);
            SetRemoteQueueRank(0);
        }*/
//<<< WiZaRd::FiX? Moved down!

        // OP_REQUESTSOURCES // OP_REQUESTSOURCES2
        if (IsSourceRequestAllowed(true))
        {
            if (thePrefs.GetDebugClientTCPLevel() > 0)
            {
                DebugSend("OP__MPReqSources", this, reqfile->GetFileHash());
                if (GetLastAskedForSources() == 0)
                    Debug(_T("  first source request\n"));
                else
                    Debug(_T("  last source request was before %s\n"), CastSecondsToHM((GetTickCount() - GetLastAskedForSources())/1000));
            }
            if (SupportsSourceExchange2())
            {
                dataFileReq.WriteUInt8(OP_REQUESTSOURCES2);
//>>> WiZaRd::ExtendedXS [Xanatos]
                if (SupportsExtendedSourceExchange())
                    dataFileReq.WriteUInt8(SOURCEEXCHANGEEXT_VERSION);
                else
//<<< WiZaRd::ExtendedXS [Xanatos]
                    dataFileReq.WriteUInt8(SOURCEEXCHANGE2_VERSION);
                const uint16 nOptions = 0; // 16 ... Reserved
                dataFileReq.WriteUInt16(nOptions);
            }
            else
                dataFileReq.WriteUInt8(OP_REQUESTSOURCES);

            reqfile->SetLastAnsweredTimeTimeout();
            SetLastAskedForSources();
            if (thePrefs.GetDebugSourceExchange())
                AddDebugLogLine(false, L"SX: sending %s request to %s for file \"%s\"", SupportsSourceExchange2() ? (SupportsExtendedSourceExchange() ? L"extXS" : L"v2") : L"v1", DbgGetClientInfo(), reqfile->GetFileName()); //>>> WiZaRd::ExtendedXS [Xanatos]
//>>> WiZaRd::ClientAnalyzer
            m_fSourceExchangeRequested = TRUE; //>>> Security Check
            //Because official client has a bug, only count the XSReqs under special conditions
            //remark: this fails for complete sources the first time.. but this doesn't hurt
            //complete source or we want the same file...
            if (m_bCompleteSource || !md4cmp(GetUploadFileID(), reqfile->GetFileHash()))
                //we can't expect an answer for a pretty rare file
                //the src might not even know other sources that it could pass on to us!
                if (pAntiLeechData && reqfile->GetSourceCount() > AT_RAREFILE)
                    pAntiLeechData->IncXSAsks();
//<<< WiZaRd::ClientAnalyzer
        }

        // OP_AICHFILEHASHREQ - deprecated with fileidentifiers
        if (IsSupportingAICH() && !SupportsFileIdentifiers())
        {
            m_fAICHHashRequested = TRUE; //>>> WiZaRd::ClientAnalyzer //>>> Security Check
            if (thePrefs.GetDebugClientTCPLevel() > 0)
                DebugSend("OP__MPAichFileHashReq", this, reqfile->GetFileHash());
            dataFileReq.WriteUInt8(OP_AICHFILEHASHREQ);
        }

        Packet* packet = new Packet(&dataFileReq, OP_EMULEPROT);
        if (SupportsFileIdentifiers())
            packet->opcode = OP_MULTIPACKET_EXT2;
        else if (SupportExtMultiPacket())
            packet->opcode = OP_MULTIPACKET_EXT;
        else
            packet->opcode = OP_MULTIPACKET;
        theStats.AddUpDataOverheadFileRequest(packet->size);
        SendPacket(packet, true);
    }
    else
    {
        CSafeMemFile dataFileReq(96);
        dataFileReq.WriteHash16(reqfile->GetFileHash());
        //This is extended information
        if (GetExtendedRequestsVersion() > 0)
            reqfile->WriteSafePartStatus(&dataFileReq, this); //>>> WiZaRd::Intelligent SOTN
        if (GetExtendedRequestsVersion() > 1)
            reqfile->WriteCompleteSourcesCount(&dataFileReq);
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__FileRequest", this, reqfile->GetFileHash());
        Packet* packet = new Packet(&dataFileReq);
        packet->opcode = OP_REQUESTFILENAME;
        theStats.AddUpDataOverheadFileRequest(packet->size);
        SendPacket(packet, true);

        // 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
        // if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
        // know that the file is shared, we know also that the file is complete and don't need to request the file status.
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        if (reqfile->GetPartCount() > 1 || SupportsSCT())
            //if (reqfile->GetPartCount() > 1)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        {
            if (thePrefs.GetDebugClientTCPLevel() > 0)
                DebugSend("OP__SetReqFileID", this, reqfile->GetFileHash());
            CSafeMemFile dataSetReqFileID(16);
            dataSetReqFileID.WriteHash16(reqfile->GetFileHash());
            packet = new Packet(&dataSetReqFileID);
            packet->opcode = OP_SETREQFILEID;
            theStats.AddUpDataOverheadFileRequest(packet->size);
            SendPacket(packet, true);
        }

//>>> WiZaRd::FiX? Moved down!
        /*if (IsEmuleClient())
        {
            SetRemoteQueueFull(true);
            SetRemoteQueueRank(0);
        }*/
//<<< WiZaRd::FiX? Moved down!

        if (IsSourceRequestAllowed(true))
        {
            if (thePrefs.GetDebugClientTCPLevel() > 0)
            {
                DebugSend("OP__RequestSources", this, reqfile->GetFileHash());
                if (GetLastAskedForSources() == 0)
                    Debug(_T("  first source request\n"));
                else
                    Debug(_T("  last source request was before %s\n"), CastSecondsToHM((GetTickCount() - GetLastAskedForSources())/1000));
            }
            reqfile->SetLastAnsweredTimeTimeout();

            Packet* packet;
            if (SupportsSourceExchange2())
            {
                packet = new Packet(OP_REQUESTSOURCES2,19,OP_EMULEPROT);
//>>> WiZaRd::ExtendedXS [Xanatos]
                if (SupportsExtendedSourceExchange())
                    PokeUInt8(&packet->pBuffer[0], SOURCEEXCHANGEEXT_VERSION);
                else
//<<< WiZaRd::ExtendedXS [Xanatos]
                    PokeUInt8(&packet->pBuffer[0], SOURCEEXCHANGE2_VERSION);
                const uint16 nOptions = 0; // 16 ... Reserved
                PokeUInt16(&packet->pBuffer[1], nOptions);
                md4cpy(&packet->pBuffer[3],reqfile->GetFileHash());
            }
            else
            {
                packet = new Packet(OP_REQUESTSOURCES,16,OP_EMULEPROT);
                md4cpy(packet->pBuffer,reqfile->GetFileHash());
            }

            theStats.AddUpDataOverheadSourceExchange(packet->size);
            SendPacket(packet, true);
            SetLastAskedForSources();
            if (thePrefs.GetDebugSourceExchange())
                AddDebugLogLine(false, L"SX: sending %s request to %s for file \"%s\"", SupportsSourceExchange2() ? (SupportsExtendedSourceExchange() ? L"extXS" : L"v2") : L"v1", DbgGetClientInfo(), reqfile->GetFileName()); //>>> WiZaRd::ExtendedXS [Xanatos]
//>>> WiZaRd::ClientAnalyzer
            m_fSourceExchangeRequested = TRUE; //>>> Security Check
            //Because official client has a bug, only count the XSReqs under special conditions
            //remark: this fails for complete sources the first time.. but this doesn't hurt
            //complete source or we want the same file...
            if (m_bCompleteSource || !md4cmp(GetUploadFileID(), reqfile->GetFileHash()))
                //we can't expect an answer for a pretty rare file
                //the src might not even know other sources that it could pass on to us!
                if (pAntiLeechData && reqfile->GetSourceCount() > AT_RAREFILE)
                    pAntiLeechData->IncXSAsks();
//<<< WiZaRd::ClientAnalyzer
        }

        if (IsSupportingAICH())
        {
            m_fAICHHashRequested = TRUE; //>>> WiZaRd::ClientAnalyzer //>>> Security Check
            if (thePrefs.GetDebugClientTCPLevel() > 0)
                DebugSend("OP__AichFileHashReq", this, reqfile->GetFileHash());
            Packet* packet = new Packet(OP_AICHFILEHASHREQ,16,OP_EMULEPROT);
            md4cpy(packet->pBuffer,reqfile->GetFileHash());
            theStats.AddUpDataOverheadFileRequest(packet->size);
            SendPacket(packet, true);
        }
    }
    SetLastAskedTime();
    m_byFileRequestState = 1; //>>> WiZaRd::Unsolicited PartStatus [Netfinity]
}

void CUpDownClient::SendStartupLoadReq()
{
    if (socket == NULL || reqfile == NULL)
    {
        ASSERT(0);
        return;
    }

//>>> WiZaRd::FiX? Moved down!
    if (GetDownloadState() == DS_DOWNLOADING)
    {
        SendBlockRequests();
        return;
    }

    if (IsEmuleClient())
    {
        SetRemoteQueueFull(true);
        SetRemoteQueueRank(0);
    }
//<<< WiZaRd::FiX? Moved down!

    if (m_byFileRequestState != 1) //>>> WiZaRd::Unsolicited PartStatus [Netfinity]
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"%hs with FileRequestState=%u", __FUNCTION__, m_byFileRequestState);
    {
        m_fQueueRankPending = 1;
        //m_fUnaskQueueRankRecv = 0; //>>> WiZaRd - FiX? Why should we reset it here?!
        m_byFileRequestState = 2; //>>> WiZaRd::Unsolicited PartStatus [Netfinity]
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__StartupLoadReq", this);
        CSafeMemFile dataStartupLoadReq(16);
        dataStartupLoadReq.WriteHash16(reqfile->GetFileHash());
        Packet* packet = new Packet(&dataStartupLoadReq);
        packet->opcode = OP_STARTUPLOADREQ;
        theStats.AddUpDataOverheadFileRequest(packet->size);
        SetDownloadState(DS_ONQUEUE);
        SendPacket(packet, true);
    }
}

void CUpDownClient::ProcessFileInfo(CSafeMemFile* data, CPartFile* file)
{
    if (file==NULL)
        throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileInfo; file==NULL)");
    if (reqfile==NULL)
        throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileInfo; reqfile==NULL)");
    if (file != reqfile)
        throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileInfo; reqfile!=file)");
    m_strClientFilename = data->ReadString(GetUnicodeSupport()!=utf8strNone);
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        Debug(_T("  Filename=\"%s\"\n"), m_strClientFilename);
//>>> FDC [BlueSonicBoy]
//	if(!reqfile->DissimilarName())
    reqfile->CheckFilename(m_strClientFilename);
//<<< FDC [BlueSonicBoy]

    // 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
    // if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
    // know that the file is shared, we know also that the file is complete and don't need to request the file status.
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (!SupportsSCT() && reqfile->GetPartCount() == 1)
        //if (reqfile->GetPartCount() == 1)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    {
        const UINT nOldPartCount = GetPartCount();
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        delete m_pPartStatus;
        m_pPartStatus = NULL; // In case CAICHStatusVector constructor fails
        m_pPartStatus = new CAICHStatusVector(file->GetFileSize());
        m_pPartStatus->Set(0, reqfile->GetFileSize() - 1ULL);
        const UINT nPartCount = GetPartCount();
        //delete[] m_abyPartStatus;
        //m_abyPartStatus = NULL;
        //m_nPartCount = reqfile->GetPartCount();
        //m_abyPartStatus = new uint8[m_nPartCount];
        //memset(m_abyPartStatus,1,m_nPartCount);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
#ifdef ANTI_HIDEOS
//>>> WiZaRd::AntiHideOS [netfinity]
        // no need for AHOS on single part files
        delete[] m_abySeenPartStatus;
        m_abySeenPartStatus = NULL;
//<<< WiZaRd::AntiHideOS [netfinity]
#endif
//>>> WiZaRd::ICS [enkeyDEV]
        if (nOldPartCount != nPartCount)
        {
            delete[] m_abyIncPartStatus;
            m_abyIncPartStatus = NULL;
        }
//<<< WiZaRd::ICS [enkeyDEV]
        ProcessDownloadFileStatus(false);
    }
}

void CUpDownClient::ProcessFileStatus(bool bUdpPacket, CSafeMemFile* data, CPartFile* file)
{
    if (!reqfile || file != reqfile)
    {
        if (reqfile == NULL)
            throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileStatus; reqfile==NULL)");
        throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileStatus; reqfile!=file)");
    }

    if (file->GetStatus() == PS_COMPLETE || file->GetStatus() == PS_COMPLETING)
        return;

    const UINT nOldPartCount = GetPartCount();
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    delete m_pPartStatus;
    m_pPartStatus = NULL; // In case we fail to create the part status object
    m_pPartStatus = CPartStatus::CreatePartStatus(data, reqfile->GetFileSize());
    const UINT nPartCount = GetPartCount();
#ifdef ANTI_HIDEOS
//>>> WiZaRd::AntiHideOS [netfinity]
    if (m_abySeenPartStatus == NULL || nOldPartCount != nPartCount)
    {
        delete[] m_abySeenPartStatus;
        m_abySeenPartStatus = NULL;
        m_abySeenPartStatus = new uint8[nPartCount];
        memset(m_abySeenPartStatus, 0, nPartCount);
    }
//<<< WiZaRd::AntiHideOS [netfinity]
#endif
//>>> WiZaRd::ICS [enkeyDEV]
    if (nOldPartCount != nPartCount)
    {
        delete[] m_abyIncPartStatus;
        m_abyIncPartStatus = NULL;
    }
//<<< WiZaRd::ICS [enkeyDEV]

    ProcessDownloadFileStatus(bUdpPacket);
}

bool CUpDownClient::ProcessDownloadFileStatus(const bool bUDPPacket, bool bMergeIfPossible)
{
    m_bCompleteSource = false; // clear that flag, just to be sure
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    // netfinity: Update upload partstatus if we are downloading this file and there is more pieces available in the download partstatus
    bMergeIfPossible = bMergeIfPossible && GetUploadFileID() && !md4cmp(GetUploadFileID(), reqfile->GetFileHash());
    if (bMergeIfPossible)
    {
        if (m_pUpPartStatus == NULL)
            m_pUpPartStatus = m_pPartStatus->Clone();
        else
            m_pUpPartStatus->Merge(m_pPartStatus);
    }
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

    const UINT nPartCount = GetPartCount();

    bool bPartsNeeded = false;
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (!reqfile->GetDonePartStatus() || reqfile->GetDonePartStatus()->GetNeeded(m_pPartStatus) > 0)
        bPartsNeeded = true;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

    uint16 done = 0;
    uint16 complcount = 0; //>>> WiZaRd: Anti HideOS
#ifdef ANTI_HIDEOS
    const bool checkSeenParts = m_abySeenPartStatus != NULL && GetUploadFileID() && !md4cmp(GetUploadFileID(), reqfile->GetFileHash()); //>>> WiZaRd::AntiHideOS [netfinity]
#endif
//>>> WiZaRd::Auto-ICS for Sub-Chunk-Transfer
    bool bPartialPartFound = false;
    if (m_abyIncPartStatus == NULL)
    {
        m_abyIncPartStatus = new uint8[nPartCount];
        memset(m_abyIncPartStatus, 0, nPartCount);
    }
//<<< WiZaRd::Auto-ICS for Sub-Chunk-Transfer
    while (done != nPartCount)
    {
//>>> WiZaRd::Auto-ICS for Sub-Chunk-Transfer
        if (m_pPartStatus->IsPartialPart(done))
        {
            bPartialPartFound = true;
            m_abyIncPartStatus[done] = 1;
        }
//<<< WiZaRd::Auto-ICS for Sub-Chunk-Transfer
        bool bPartAvail = IsPartAvailable(done);
#ifdef ANTI_HIDEOS
//>>> WiZaRd::AntiHideOS [netfinity]
        if (checkSeenParts && bPartAvail)
            m_abySeenPartStatus[done] = 1;
//<<< WiZaRd::AntiHideOS [netfinity]
#endif
        if (bPartAvail
#ifdef ANTI_HIDEOS
//>>> WiZaRd::AntiHideOS [netfinity]
                || (checkSeenParts && m_abySeenPartStatus[done])
//<<< WiZaRd::AntiHideOS [netfinity]
#endif
           )
        {
            if (!bPartsNeeded && !reqfile->IsCompletePart(done, false))
                bPartsNeeded = true;
            ++complcount; //>>> WiZaRd: Anti HideOS
        }
        ++done;
    }
    m_bCompleteSource = (complcount == nPartCount); //>>> WiZaRd: Anti HideOS
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
//>>> WiZaRd::Auto-ICS for Sub-Chunk-Transfer
    if (!bPartialPartFound)
    {
        delete[] m_abyIncPartStatus;
        m_abyIncPartStatus = NULL;
    }
//<<< WiZaRd::Auto-ICS for Sub-Chunk-Transfer
//>>> WiZaRd::ClientAnalyzer
    if (pAntiLeechData)
        pAntiLeechData->Check4FileFaker();
//<<< WiZaRd::ClientAnalyzer

    if (bUDPPacket ? (thePrefs.GetDebugClientUDPLevel() > 0) : (thePrefs.GetDebugClientTCPLevel() > 0))
    {
        TCHAR* psz = new TCHAR[nPartCount + 1];
        int iNeeded = 0;
        for (UINT i = 0; i < nPartCount; i++)
        {
            if (IsPartAvailable(i))
            {
                if (!reqfile->IsCompletePart(i, false))
                {
                    psz[i] = L'X';
                    ++iNeeded;
                }
                else
                    psz[i] = L'#';
            }
            else
            {
                if (!reqfile->IsCompletePart(i, false))
                    ++iNeeded;
                psz[i] = L'.';
            }
        }
        psz[nPartCount] = L'\0';
        Debug(L"  Parts=%u  %s  Needed=%u\n", nPartCount, psz, iNeeded);
        delete[] psz;
    }

    UpdateDisplayedInfo(bUDPPacket);
    reqfile->UpdateAvailablePartsCount();

    // NOTE: This function is invoked from TCP and UDP socket!
    if (!bUDPPacket)
    {
        if (!bPartsNeeded)
        {
            if (GetDownloadState() != DS_DOWNLOADING) // If we are in downloading state this is handled in a different place (e.g Delayed NNP)
            {
                SetDownloadState(DS_NONEEDEDPARTS);
                if (SwapToAnotherFile(_T("A4AF for NNP file. CUpDownClient::ProcessFileStatus() TCP"), true, false, false, NULL, true, true))
                    SendFileRequest(); // netfinity: We need to send request here, or we might get stuck!!!
            }
        }
        else if (GetDownloadState() == DS_DOWNLOADING) // We might have asked for a new file status if we got NNP
            SendBlockRequests();
        else if (reqfile->m_bMD4HashsetNeeded || (reqfile->IsAICHPartHashSetNeeded() && SupportsFileIdentifiers()
                 && GetReqFileAICHHash() != NULL && *GetReqFileAICHHash() == reqfile->GetFileIdentifier().GetAICHHash())) //If we are using the eMule filerequest packets, this is taken care of in the Multipacket!
            SendHashSetRequest();
        else
            SendStartupLoadReq();
    }
    else
    {
        if (!bPartsNeeded)
        {
            if (GetDownloadState() != DS_DOWNLOADING) // If we are in downloading state this is handled in a different place (e.g Delayed NNP)
            {
                SetDownloadState(DS_NONEEDEDPARTS);
                //SwapToAnotherFile(_T("A4AF for NNP file. CUpDownClient::ProcessFileStatus() UDP"), true, false, false, NULL, true, false);
            }
        }
        else
            SetDownloadState(DS_ONQUEUE);
    }
    reqfile->UpdatePartsInfo();
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (bMergeIfPossible)
        ProcessUploadFileStatus(bUDPPacket, reqfile, false);
    return true;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
}

bool CUpDownClient::AddRequestForAnotherFile(CPartFile* file)
{
    for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition(); pos != 0;)
    {
        if (m_OtherNoNeeded_list.GetNext(pos) == file)
            return false;
    }
    for (POSITION pos = m_OtherRequests_list.GetHeadPosition(); pos != 0;)
    {
        if (m_OtherRequests_list.GetNext(pos) == file)
            return false;
    }
    m_OtherRequests_list.AddTail(file);
    file->A4AFsrclist.AddTail(this); // [enkeyDEV(Ottavio84) -A4AF-]

    return true;
}

void CUpDownClient::ClearDownloadBlockRequests()
{
    for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition(); pos != 0;)
    {
        Requested_Block_Struct* cur_block = m_DownloadBlocks_list.GetNext(pos);
        if (reqfile)
        {
            reqfile->RemoveBlockFromList(cur_block->StartOffset,cur_block->EndOffset);
        }
        delete cur_block;
    }
    m_DownloadBlocks_list.RemoveAll();

    for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != 0;)
    {
        Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
        if (reqfile)
            reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);

        delete pending->block;
        // Not always allocated
        if (pending->zStream)
        {
            inflateEnd(pending->zStream);
            delete pending->zStream;
        }
        delete pending;
    }
    m_PendingBlocks_list.RemoveAll();
}

void CUpDownClient::SetDownloadState(EDownloadState nNewState, LPCTSTR pszReason)
{
    if (m_nDownloadState != nNewState)
    {
//>>> WiZaRd::NoNeededReqeue [SlugFiller]
        //This was added by SlugFiller ages ago but never made it into many mods... recently I saw it in Xtreme and
        //remembered it, so here it is in quick'n'dirty way.
        //Basically, if we go NNS then we should refresh the partstatus because a NNS source may have new parts meanwhile...
        //Thus, we set the state to OnQueue because that way we will ask in FILEREASKTIME instead of 2*FILEREASKTIME
        //which saves us about 29mins. of waiting ;)
        if (m_nDownloadState == DS_DOWNLOADING && nNewState == DS_NONEEDEDPARTS)
            nNewState = DS_ONQUEUE;
//<<< WiZaRd::NoNeededReqeue [SlugFiller]

        switch (nNewState)
        {
            case DS_CONNECTING:
                m_dwLastTriedToConnect = ::GetTickCount();
                break;
            case DS_TOOMANYCONNSKAD:
                //This client had already been set to DS_CONNECTING.
                //So we reset this time so it isn't stuck at TOOMANYCONNS for 20mins.
                m_dwLastTriedToConnect = ::GetTickCount()-20*60*1000;
                break;
            case DS_WAITCALLBACKKAD:
            case DS_WAITCALLBACK:
                break;
            case DS_NONEEDEDPARTS:
                // Since tcp asks never sets reask time if the result is DS_NONEEDEDPARTS
                // If we set this, we will not reask for that file until some time has passed.
                SetLastAskedTime();
                //DontSwapTo(reqfile);

                /*default:
                	switch( m_nDownloadState )
                	{
                		case DS_WAITCALLBACK:
                		case DS_WAITCALLBACKKAD:
                			break;
                		default:
                			m_dwLastTriedToConnect = ::GetTickCount()-20*60*1000;
                			break;
                	}
                	break;*/
        }

        if (reqfile)
        {
            if (nNewState == DS_DOWNLOADING)
            {
                if (thePrefs.GetLogUlDlEvents())
                    AddDebugLogLine(DLP_VERYLOW, false, _T("Download session started. User: %s in SetDownloadState(). New State: %i"), DbgGetClientInfo(), nNewState);

                reqfile->AddDownloadingSource(this);
            }
            else if (m_nDownloadState == DS_DOWNLOADING)
            {
                reqfile->RemoveDownloadingSource(this);
            }
        }

        if (nNewState == DS_DOWNLOADING && socket)
        {
            socket->SetTimeOut(CONNECTION_TIMEOUT*4);
        }

        bool bBan = false; //>>> WiZaRd::ClientAnalyzer
        if (m_nDownloadState == DS_DOWNLOADING)
        {
            if (socket)
                socket->SetTimeOut(CONNECTION_TIMEOUT);

            if (thePrefs.GetLogUlDlEvents())
            {
                switch (nNewState)
                {
                    case DS_NONEEDEDPARTS:
                        pszReason = _T("NNP. You don't need any parts from this client.");
                }

                if (thePrefs.GetLogUlDlEvents())
                    AddDebugLogLine(DLP_VERYLOW, false, _T("Download session ended: %s User: %s in SetDownloadState(). New State: %i, Length: %s, Payload: %s, Transferred: %s, Req blocks not yet completed: %i."), pszReason, DbgGetClientInfo(), nNewState, CastSecondsToHM(GetDownTimeDifference(false)/1000), CastItoXBytes(GetSessionPayloadDown(), false, false), CastItoXBytes(GetSessionDown(), false, false), m_PendingBlocks_list.GetCount());
            }

            ResetSessionDown();

            // -khaos--+++> Extended Statistics (Successful/Failed Download Sessions)
            if (m_bTransferredDownMini && nNewState != DS_ERROR)
            {
                thePrefs.Add2DownSuccessfulSessions(); // Increment our counters for successful sessions (Cumulative AND Session)
//>>> WiZaRd::ClientAnalyzer
                if (pAntiLeechData)
                    pAntiLeechData->AddDLSession(false);
//<<< WiZaRd::ClientAnalyzer
            }
            else
            {
                thePrefs.Add2DownFailedSessions(); // Increment our counters failed sessions (Cumulative AND Session)
//>>> WiZaRd::ClientAnalyzer
                if (pAntiLeechData)
                {
                    pAntiLeechData->AddDLSession(true);
                    bBan = pAntiLeechData->ShouldBanForBadDownloads();
                }
//<<< WiZaRd::ClientAnalyzer
            }
            thePrefs.Add2DownSAvgTime(GetDownTimeDifference()/1000);
            // <-----khaos-

            m_nDownloadState = (_EDownloadState)nNewState;

            ClearDownloadBlockRequests();

            m_nDownDatarate = 0;
            m_AvarageDDR_list.RemoveAll();
            m_nSumForAvgDownDataRate = 0;

            if (nNewState == DS_NONE)
            {
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                delete m_pPartStatus;
                m_pPartStatus = NULL;
                //delete[] m_abyPartStatus;
                //m_abyPartStatus = NULL;
                //m_nPartCount = 0;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
//>>> WiZaRd::ICS [enkeyDEV]
                delete[] m_abyIncPartStatus;
                m_abyIncPartStatus = NULL;
//<<< WiZaRd::ICS [enkeyDEV]
#ifdef ANTI_HIDEOS
//>>> WiZaRd::AntiHideOS [netfinity]
                delete[] m_abySeenPartStatus;
                m_abySeenPartStatus = NULL;
//<<< WiZaRd::AntiHideOS [netfinity]
#endif
            }
            if (socket && nNewState != DS_ERROR)
                socket->DisableDownloadLimit();
        }
        m_nDownloadState = (_EDownloadState)nNewState;
        if (GetDownloadState() == DS_DOWNLOADING)
        {
            if (IsEmuleClient())
                SetRemoteQueueFull(false);
            SetRemoteQueueRank(0);
            SetAskedCountDown(0);
        }
        UpdateDisplayedInfo(true);
//>>> WiZaRd::ClientAnalyzer
        if (bBan)
            Ban(L"Too many failed download sessions");
//<<< WiZaRd::ClientAnalyzer
    }
}

void CUpDownClient::ProcessHashSet(const uchar* packet, UINT size, bool bFileIdentifiers)
{
    CSafeMemFile data(packet, size);
    if (bFileIdentifiers)
    {
        if (!m_fHashsetRequestingMD4 && !m_fHashsetRequestingAICH)
            throw CString(L"unwanted hashset2");
        CFileIdentifierSA fileIdent;
        if (!fileIdent.ReadIdentifier(&data))
            throw CString(L"Invalid FileIdentifier");
        if (reqfile == NULL || !reqfile->GetFileIdentifier().CompareRelaxed(fileIdent))
        {
            CheckFailedFileIdReqs(packet);
            throw GetResString(IDS_ERR_WRONGFILEID) + L" (ProcessHashSet2)";
        }
        bool bMD4 = m_fHashsetRequestingMD4 != 0;
        bool bAICH = m_fHashsetRequestingAICH != 0;
        if (!reqfile->GetFileIdentifier().ReadHashSetsFromPacket(&data, bMD4, bAICH))
        {
            if (m_fHashsetRequestingMD4)
                reqfile->m_bMD4HashsetNeeded = true;
            if (m_fHashsetRequestingAICH)
                reqfile->SetAICHHashSetNeeded(true);
            m_fHashsetRequestingMD4 = 0;
            m_fHashsetRequestingAICH = 0;
            throw GetResString(IDS_ERR_BADHASHSET);
        }
        if (m_fHashsetRequestingMD4 && !bMD4)
        {
            DebugLogWarning(L"Client was unable to deliver requested MD4 hashset (shouldn't happen) - %s, file: %s", DbgGetClientInfo(), reqfile->GetFileName());
            reqfile->m_bMD4HashsetNeeded = true;
        }
        else if (m_fHashsetRequestingMD4)
            DebugLog(L"Received valid MD4 Hashset (FileIdentifiers) form %s, file: %s", DbgGetClientInfo(), reqfile->GetFileName());

        if (m_fHashsetRequestingAICH && !bAICH)
        {
            DebugLogWarning(L"Client was unable to deliver requested AICH part hashset, asking other clients - %s, file: %s", DbgGetClientInfo(), reqfile->GetFileName());
            reqfile->SetAICHHashSetNeeded(true);
        }
        else if (m_fHashsetRequestingAICH)
            DebugLog(L"Received valid AICH Part Hashset from %s, file: %s", DbgGetClientInfo(), reqfile->GetFileName());
        m_fHashsetRequestingMD4 = 0;
        m_fHashsetRequestingAICH = 0;
    }
    else
    {
        if (!m_fHashsetRequestingMD4)
            throw CString(L"unwanted hashset");
        if ((!reqfile) || md4cmp(packet,reqfile->GetFileHash()))
        {
            CheckFailedFileIdReqs(packet);
            throw GetResString(IDS_ERR_WRONGFILEID) + L" (ProcessHashSet)";
        }
        m_fHashsetRequestingMD4 = 0;
        if (!reqfile->GetFileIdentifier().LoadMD4HashsetFromFile(&data, true))
        {
            reqfile->m_bMD4HashsetNeeded = true;
            throw GetResString(IDS_ERR_BADHASHSET);
        }
    }

//>>> WiZaRd::Immediate File Sharing
    if (reqfile->GetStatus(true) == PS_READY
            || (reqfile->GetStatus() == PS_EMPTY && !reqfile->m_bMD4HashsetNeeded))
    {
        reqfile->SetStatus(PS_READY);
        if (theApp.sharedfiles->GetFileByID(reqfile->GetFileHash()) == NULL)
            theApp.sharedfiles->SafeAddKFile(reqfile);
    }
//<<< WiZaRd::Immediate File Sharing
//>>> WiZaRd:FiX?
    // the remote client may have added us to his upload in the meantime
    if (GetDownloadState() != DS_DOWNLOADING)
//<<< WiZaRd:FiX?
        SendStartupLoadReq();
}

void CUpDownClient::CreateBlockRequests(int iMaxBlocks)
{
    ASSERT(iMaxBlocks == -1 || iMaxBlocks >= 1 /*&& iMaxBlocks <= 3*/); //>>> WiZaRd::Endgame Improvement
//>>> WiZaRd::Endgame Improvement
    // we all know about the endgame problem:
    // we have nearly finished our file and have to wait for a slow speed client to finish the transfer instead of using a highspeed one.
    // there are several approaches to solve that issue but IMHO it's best to combine them, i.e. change the number of requested blocks AND drop slow users

    // prevent locking of too many blocks when we are on a slow (probably standby/trickle) slot
    const UINT nDownDatarate = GetAvgDownSpeed();
    if (iMaxBlocks == -1)
    {
        const uint64 iDiff = reqfile->GetFileSize() - reqfile->GetCompletedSize();
        if (iDiff < 2*PARTSIZE)
        {
            iMaxBlocks = max(1, (UINT)iDiff / EMBLOCKSIZE / (reqfile->GetSrcStatisticsValue(DS_DOWNLOADING)+1));
            iMaxBlocks = min(iMaxBlocks, (int)max(EMBLOCKSIZE, 60*nDownDatarate)/EMBLOCKSIZE);
        }
        else //Beta48: cap down to PARTSIZE at max!
            iMaxBlocks = max(3*EMBLOCKSIZE, min(PARTSIZE, 60*nDownDatarate))/EMBLOCKSIZE;

        if (!(IsEmuleClient() && m_byCompatibleClient == 0))
            iMaxBlocks = min(3, iMaxBlocks);
    }

    //determine the "real" number of pending blocks:
    const UINT futurePossiblePendingBlock = (UINT)(m_PendingBlocks_list.GetCount() + m_DownloadBlocks_list.GetCount());
    if (futurePossiblePendingBlock < (UINT)iMaxBlocks)
    {
        uint16 neededblock = (uint16)(iMaxBlocks - futurePossiblePendingBlock);
        neededblock = (neededblock/3+1)*3;
        Requested_Block_Struct** toadd = new Requested_Block_Struct*[neededblock];

        const uint16 oldNeededBlock = neededblock;
        bool bSucceeded = reqfile->GetNextRequestedBlock(this, toadd, &neededblock);
        if (!bSucceeded)
        {
            neededblock = oldNeededBlock;
            bSucceeded = reqfile->IsNextRequestPossible(this) && reqfile->DropSlowestSource(this) && reqfile->GetNextRequestedBlock(this, toadd, &neededblock);
        }
        if (bSucceeded)
        {
            for (uint16 i = 0; i < neededblock; ++i)
                m_DownloadBlocks_list.AddTail(toadd[i]);
        }
        delete[] toadd;
    }

    //fill the pending list with the created/cached blocks
    while ((UINT)m_PendingBlocks_list.GetCount() < (UINT)iMaxBlocks
            && !m_DownloadBlocks_list.IsEmpty())
    {
        Pending_Block_Struct* pblock = new Pending_Block_Struct();
        pblock->block = m_DownloadBlocks_list.RemoveHead();
        m_PendingBlocks_list.AddTail(pblock);
    }
//<<< WiZaRd::Endgame Improvement
}

void CUpDownClient::SendBlockRequests()
{
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__RequestParts", this, reqfile!=NULL ? reqfile->GetFileHash() : NULL);

    m_dwLastBlockReceived = ::GetTickCount();
    if (!reqfile)
        return;

    CreateBlockRequests(); //>>> WiZaRd::Endgame Improvement

    if (m_PendingBlocks_list.IsEmpty())
    {
        SendCancelTransfer();
        SetDownloadState(DS_NONEEDEDPARTS);
        SwapToAnotherFile(_T("A4AF for NNP file. CUpDownClient::SendBlockRequests()"), true, false, false, NULL, true, true);
        return;
    }

    bool bI64Offsets = false;
    POSITION pos = m_PendingBlocks_list.GetHeadPosition();
    for (UINT i = 0; i != 3; i++)
    {
        if (pos)
        {
            Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
            ASSERT(pending->block->StartOffset <= pending->block->EndOffset);
            if (pending->block->StartOffset > 0xFFFFFFFF || pending->block->EndOffset >= 0xFFFFFFFF)
            {
                bI64Offsets = true;
                if (!SupportsLargeFiles())
                {
                    ASSERT(0);
                    SendCancelTransfer();
                    SetDownloadState(DS_ERROR);
                    return;
                }
                break;
            }
        }
    }

    Packet* packet;
    if (bI64Offsets)
    {
        const int iPacketSize = 16+(3*8)+(3*8); // 64
        packet = new Packet(OP_REQUESTPARTS_I64, iPacketSize, OP_EMULEPROT);
        CSafeMemFile data((const BYTE*)packet->pBuffer, iPacketSize);
        data.WriteHash16(reqfile->GetFileHash());
        pos = m_PendingBlocks_list.GetHeadPosition();
        for (UINT i = 0; i != 3; i++)
        {
            if (pos)
            {
                Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
                ASSERT(pending->block->StartOffset <= pending->block->EndOffset);
                //ASSERT( pending->zStream == NULL );
                //ASSERT( pending->totalUnzipped == 0 );
                pending->fZStreamError = 0;
                pending->fRecovered = 0;
                data.WriteUInt64(pending->block->StartOffset);
            }
            else
                data.WriteUInt64(0);
        }
        pos = m_PendingBlocks_list.GetHeadPosition();
        for (UINT i = 0; i != 3; i++)
        {
            if (pos)
            {
                Requested_Block_Struct* block = m_PendingBlocks_list.GetNext(pos)->block;
                uint64 endpos = block->EndOffset+1;
                data.WriteUInt64(endpos);
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                {
                    CString strInfo;
                    strInfo.Format(_T("  Block request %u: "), i);
                    strInfo += DbgGetBlockInfo(block);
                    strInfo.AppendFormat(_T(",  Complete=%s"), reqfile->IsComplete(block->StartOffset, block->EndOffset, false) ? _T("Yes(NOTE:)") : GetResString(IDS_NO));
                    strInfo.AppendFormat(_T(",  PureGap=%s"), reqfile->IsPureGap(block->StartOffset, block->EndOffset) ? GetResString(IDS_YES) : _T("No(NOTE:)"));
                    strInfo.AppendFormat(_T(",  AlreadyReq=%s"), reqfile->IsAlreadyRequested(block->StartOffset, block->EndOffset) ? GetResString(IDS_YES) : _T("No(NOTE:)"));
                    strInfo += _T('\n');
                    Debug(strInfo);
                }
            }
            else
            {
                data.WriteUInt64(0);
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                    Debug(_T("  Block request %u: <empty>\n"), i);
            }
        }
    }
    else
    {
        const int iPacketSize = 16+(3*4)+(3*4); // 40
        packet = new Packet(OP_REQUESTPARTS,iPacketSize);
        CSafeMemFile data((const BYTE*)packet->pBuffer, iPacketSize);
        data.WriteHash16(reqfile->GetFileHash());
        pos = m_PendingBlocks_list.GetHeadPosition();
        for (UINT i = 0; i != 3; i++)
        {
            if (pos)
            {
                Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
                ASSERT(pending->block->StartOffset <= pending->block->EndOffset);
                //ASSERT( pending->zStream == NULL );
                //ASSERT( pending->totalUnzipped == 0 );
                pending->fZStreamError = 0;
                pending->fRecovered = 0;
                data.WriteUInt32((UINT)pending->block->StartOffset);
            }
            else
                data.WriteUInt32(0);
        }
        pos = m_PendingBlocks_list.GetHeadPosition();
        for (UINT i = 0; i != 3; i++)
        {
            if (pos)
            {
                Requested_Block_Struct* block = m_PendingBlocks_list.GetNext(pos)->block;
                uint64 endpos = block->EndOffset+1;
                data.WriteUInt32((UINT)endpos);
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                {
                    CString strInfo;
                    strInfo.Format(_T("  Block request %u: "), i);
                    strInfo += DbgGetBlockInfo(block);
                    strInfo.AppendFormat(_T(",  Complete=%s"), reqfile->IsComplete(block->StartOffset, block->EndOffset, false) ? _T("Yes(NOTE:)") : GetResString(IDS_NO));
                    strInfo.AppendFormat(_T(",  PureGap=%s"), reqfile->IsPureGap(block->StartOffset, block->EndOffset) ? GetResString(IDS_YES) : _T("No(NOTE:)"));
                    strInfo.AppendFormat(_T(",  AlreadyReq=%s"), reqfile->IsAlreadyRequested(block->StartOffset, block->EndOffset) ? GetResString(IDS_YES) : _T("No(NOTE:)"));
                    strInfo += _T('\n');
                    Debug(strInfo);
                }
            }
            else
            {
                data.WriteUInt32(0);
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                    Debug(_T("  Block request %u: <empty>\n"), i);
            }
        }
    }

    theStats.AddUpDataOverheadFileRequest(packet->size);
    SendPacket(packet, true);

    // on highspeed downloads, we want this packet to get out asap, so wakeup the throttler if he is sleeping
    // because there was nothing to send yet
    theApp.uploadBandwidthThrottler->NewUploadDataAvailable();
}

/* Barry - Originally this only wrote to disk when a full 180k block
           had been received from a client, and only asked for data in
		   180k blocks.

		   This meant that on average 90k was lost for every connection
		   to a client data source. That is a lot of wasted data.

		   To reduce the lost data, packets are now written to a buffer
		   and flushed to disk regularly regardless of size downloaded.
		   This includes compressed packets.

		   Data is also requested only where gaps are, not in 180k blocks.
		   The requests will still not exceed 180k, but may be smaller to
		   fill a gap.
*/
void CUpDownClient::ProcessBlockPacket(const uchar *packet, UINT size, bool packed, bool bI64Offsets)
{
    if (!bI64Offsets)
    {
        UINT nDbgStartPos = *((UINT*)(packet+16));
        if (thePrefs.GetDebugClientTCPLevel() > 1)
        {
            if (packed)
                Debug(_T("  Start=%u  BlockSize=%u  Size=%u  %s\n"), nDbgStartPos, *((UINT*)(packet + 16+4)), size-24, DbgGetFileInfo(packet));
            else
                Debug(_T("  Start=%u  End=%u  Size=%u  %s\n"), nDbgStartPos, *((UINT*)(packet + 16+4)), *((UINT*)(packet + 16+4)) - nDbgStartPos, DbgGetFileInfo(packet));
        }
    }

    // Ignore if no data required
    if (!(GetDownloadState() == DS_DOWNLOADING || GetDownloadState() == DS_NONEEDEDPARTS))
    {
        TRACE(L"%s - Invalid download state\n", __FUNCTION__);
        return;
    }



    // Update stats
    m_dwLastBlockReceived = ::GetTickCount();

    // Read data from packet
    CSafeMemFile data(packet, size);
    uchar fileID[16];
    data.ReadHash16(fileID);
    int nHeaderSize = 16;

    // Check that this data is for the correct file
    if ((!reqfile) || md4cmp(packet, reqfile->GetFileHash()))
    {
        throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessBlockPacket)");
    }

    // Find the start & end positions, and size of this chunk of data
    uint64 nStartPos;
    uint64 nEndPos;
    UINT nBlockSize = 0;

    if (bI64Offsets)
    {
        nStartPos = data.ReadUInt64();
        nHeaderSize += 8;
    }
    else
    {
        nStartPos = data.ReadUInt32();
        nHeaderSize += 4;
    }
    if (packed)
    {
        nBlockSize = data.ReadUInt32();
        nHeaderSize += 4;
        nEndPos = nStartPos + (size - nHeaderSize);
    }
    else
    {
        if (bI64Offsets)
        {
            nEndPos = data.ReadUInt64();
            nHeaderSize += 8;
        }
        else
        {
            nEndPos = data.ReadUInt32();
            nHeaderSize += 4;
        }
    }
    UINT uTransferredFileDataSize = size - nHeaderSize;

    // Check that packet size matches the declared data size + header size (24)
    if (nEndPos == nStartPos || size != ((nEndPos - nStartPos) + nHeaderSize))
        throw GetResString(IDS_ERR_BADDATABLOCK) + _T(" (ProcessBlockPacket)");

    // -khaos--+++>
    // Extended statistics information based on which client and remote port sent this data.
    // The new function adds the bytes to the grand total as well as the given client/port.
    // bFromPF is not relevant to downloaded data.  It is purely an uploads statistic.
    thePrefs.Add2SessionTransferData(GetClientSoft(), GetUserPort(), false, false, uTransferredFileDataSize, false);
    // <-----khaos-

    m_nDownDataRateMS += uTransferredFileDataSize;
    if (credits)
        credits->AddDownloaded(uTransferredFileDataSize, GetIP());
//>>> WiZaRd::ClientAnalyzer
    if (pAntiLeechData)
    {
        //Reported by Seppl: rare upload isn't correct if we only look at our current sources!
        UINT srccount = reqfile->m_nCompleteSourcesCount;
        //srccount += reqfile->GetRealQueuedCount();
        pAntiLeechData->AddDownloaded(uTransferredFileDataSize, GetAvailablePartCount() != reqfile->GetPartCount(), srccount);
    }
//<<< WiZaRd::ClientAnalyzer

    // Move end back one, should be inclusive
    nEndPos--;

    // Loop through to find the reserved block that this is within
    for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != NULL;)
    {
        POSITION posLast = pos;
        Pending_Block_Struct *cur_block = m_PendingBlocks_list.GetNext(pos);
        if ((cur_block->block->StartOffset <= nStartPos) && (cur_block->block->EndOffset >= nStartPos))
        {
            // Found reserved block

            if (cur_block->fZStreamError)
            {
                if (thePrefs.GetVerbose())
                    AddDebugLogLine(false, _T("PrcBlkPkt: Ignoring %u bytes of block starting at %I64u because of errornous zstream state for file \"%s\" - %s"), uTransferredFileDataSize, nStartPos, reqfile->GetFileName(), DbgGetClientInfo());
                reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
                return;
            }

            // Remember this start pos, used to draw part downloading in list
            m_nLastBlockOffset = nStartPos;

            // Occasionally packets are duplicated, no point writing it twice
            // This will be 0 in these cases, or the length written otherwise
            UINT lenWritten = 0;

            // Handle differently depending on whether packed or not
            if (!packed)
            {
                // security sanitize check
                if (nEndPos > cur_block->block->EndOffset)
                {
                    DebugLogError(_T("Received Blockpacket exceeds requested boundaries (requested end: %I64u, Part %u, received end  %I64u, Part %u), file %s, client %s"), cur_block->block->EndOffset
                                  , (UINT)(cur_block->block->EndOffset / PARTSIZE), nEndPos, (UINT)(nEndPos / PARTSIZE), reqfile->GetFileName(), DbgGetClientInfo());
                    reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
                    return;
                }
                // Write to disk (will be buffered in part file class)
                lenWritten = reqfile->WriteToBuffer(uTransferredFileDataSize,
                                                    packet + nHeaderSize,
                                                    nStartPos,
                                                    nEndPos,
                                                    cur_block->block,
                                                    this);
            }
            else // Packed
            {
                ASSERT((int)size > 0);
                // Create space to store unzipped data, the size is only an initial guess, will be resized in unzip() if not big enough
                UINT lenUnzipped = (size * 2);
                // Don't get too big
                if (lenUnzipped > (EMBLOCKSIZE + 300))
                    lenUnzipped = (EMBLOCKSIZE + 300);
                BYTE *unzipped = new BYTE[lenUnzipped];

                // Try to unzip the packet
                int result = unzip(cur_block, packet + nHeaderSize, uTransferredFileDataSize, &unzipped, &lenUnzipped);
                // no block can be uncompressed to >2GB, 'lenUnzipped' is obviously errornous.
                if (result == Z_OK && (int)lenUnzipped >= 0)
                {
                    if (lenUnzipped > 0) // Write any unzipped data to disk
                    {
                        ASSERT((int)lenUnzipped > 0);

                        // Use the current start and end positions for the uncompressed data
                        nStartPos = cur_block->block->StartOffset + cur_block->totalUnzipped - lenUnzipped;
                        nEndPos = cur_block->block->StartOffset + cur_block->totalUnzipped - 1;

                        if (nStartPos > cur_block->block->EndOffset || nEndPos > cur_block->block->EndOffset)
                        {
                            DebugLogError(_T("PrcBlkPkt: ") + GetResString(IDS_ERR_CORRUPTCOMPRPKG),reqfile->GetFileName(),666);
                            reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
                            // There is no chance to recover from this error
                        }
                        else
                        {
                            // Write uncompressed data to file
                            lenWritten = reqfile->WriteToBuffer(uTransferredFileDataSize,
                                                                unzipped,
                                                                nStartPos,
                                                                nEndPos,
                                                                cur_block->block,
                                                                this);
                        }
                    }
                }
                else
                {
                    if (thePrefs.GetVerbose())
                    {
                        CString strZipError;
                        if (cur_block->zStream && cur_block->zStream->msg)
                            strZipError.Format(_T(" - %hs"), cur_block->zStream->msg);
                        if (result == Z_OK && (int)lenUnzipped < 0)
                        {
                            ASSERT(0);
                            strZipError.AppendFormat(_T("; Z_OK,lenUnzipped=%d"), lenUnzipped);
                        }
                        DebugLogError(_T("PrcBlkPkt: ") + GetResString(IDS_ERR_CORRUPTCOMPRPKG) + strZipError, reqfile->GetFileName(), result);
                    }
                    reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);

                    // If we had an zstream error, there is no chance that we could recover from it nor that we
                    // could use the current zstream (which is in error state) any longer.
                    if (cur_block->zStream)
                    {
                        inflateEnd(cur_block->zStream);
                        delete cur_block->zStream;
                        cur_block->zStream = NULL;
                    }

                    // Although we can't further use the current zstream, there is no need to disconnect the sending
                    // client because the next zstream (a series of 10K-blocks which build a 180K-block) could be
                    // valid again. Just ignore all further blocks for the current zstream.
                    cur_block->fZStreamError = 1;
                    cur_block->totalUnzipped = 0;
                }
                delete [] unzipped;
            }

            // These checks only need to be done if any data was written
            if (lenWritten > 0)
            {
                m_nTransferredDown += uTransferredFileDataSize;
                m_nCurSessionPayloadDown += lenWritten;
                SetTransferredDownMini();

                // If finished reserved block
                if (nEndPos == cur_block->block->EndOffset)
                {
                    reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
                    delete cur_block->block;
                    // Not always allocated
                    if (cur_block->zStream)
                    {
                        inflateEnd(cur_block->zStream);
                        delete cur_block->zStream;
                    }
                    delete cur_block;
                    m_PendingBlocks_list.RemoveAt(posLast);

                    // Request next block
                    if (thePrefs.GetDebugClientTCPLevel() > 0)
                        DebugSend("More block requests", this);
                    SendBlockRequests();
                }
            }

            // Stop looping and exit method
            return;
        }
    }

    TRACE(L"%s - Dropping packet\n", __FUNCTION__);
}

int CUpDownClient::unzip(Pending_Block_Struct* block, const BYTE* zipped, UINT lenZipped, BYTE** unzipped, UINT* lenUnzipped, int iRecursion)
{
//#define TRACE_UNZIP	TRACE
#define TRACE_UNZIP	__noop
    TRACE_UNZIP("unzip: Zipd=%6u Unzd=%6u Rcrs=%d", lenZipped, *lenUnzipped, iRecursion);
    int err = Z_DATA_ERROR;
    try
    {
        // Save some typing
        z_stream *zS = block->zStream;

        // Is this the first time this block has been unzipped
        if (zS == NULL)
        {
            // Create stream
            block->zStream = new z_stream;
            zS = block->zStream;

            // Initialise stream values
            zS->zalloc = (alloc_func)0;
            zS->zfree = (free_func)0;
            zS->opaque = (voidpf)0;

            // Set output data streams, do this here to avoid overwriting on recursive calls
            zS->next_out = (*unzipped);
            zS->avail_out = (*lenUnzipped);

            // Initialise the z_stream
            err = inflateInit(zS);
            if (err != Z_OK)
            {
                TRACE_UNZIP("; Error: new stream failed: %d\n", err);
                return err;
            }

            ASSERT(block->totalUnzipped == 0);
        }

        // Use whatever input is provided
        zS->next_in	 = const_cast<Bytef*>(zipped);
        zS->avail_in = lenZipped;

        // Only set the output if not being called recursively
        if (iRecursion == 0)
        {
            zS->next_out = (*unzipped);
            zS->avail_out = (*lenUnzipped);
        }

        // Try to unzip the data
        TRACE_UNZIP("; inflate(ain=%6u tin=%6u aout=%6u tout=%6u)", zS->avail_in, zS->total_in, zS->avail_out, zS->total_out);
        err = inflate(zS, Z_SYNC_FLUSH);

        // Is zip finished reading all currently available input and writing all generated output
        if (err == Z_STREAM_END)
        {
            // Finish up
            err = inflateEnd(zS);
            if (err != Z_OK)
            {
                TRACE_UNZIP("; Error: end stream failed: %d\n", err);
                return err;
            }
            TRACE_UNZIP("; Z_STREAM_END\n");

            // Got a good result, set the size to the amount unzipped in this call (including all recursive calls)
            (*lenUnzipped) = (zS->total_out - block->totalUnzipped);
            block->totalUnzipped = zS->total_out;
        }
        else if ((err == Z_OK) && (zS->avail_out == 0) && (zS->avail_in != 0))
        {
            // Output array was not big enough, call recursively until there is enough space
            TRACE_UNZIP("; output array not big enough (ain=%u)\n", zS->avail_in);

            // What size should we try next
            UINT newLength = (*lenUnzipped) *= 2;
            if (newLength == 0)
                newLength = lenZipped * 2;

            // Copy any data that was successfully unzipped to new array
            BYTE *temp = new BYTE[newLength];
            ASSERT(zS->total_out - block->totalUnzipped <= newLength);
            memcpy(temp, (*unzipped), (zS->total_out - block->totalUnzipped));
            delete[](*unzipped);
            (*unzipped) = temp;
            (*lenUnzipped) = newLength;

            // Position stream output to correct place in new array
            zS->next_out = (*unzipped) + (zS->total_out - block->totalUnzipped);
            zS->avail_out = (*lenUnzipped) - (zS->total_out - block->totalUnzipped);

            // Try again
            err = unzip(block, zS->next_in, zS->avail_in, unzipped, lenUnzipped, iRecursion + 1);
        }
        else if ((err == Z_OK) && (zS->avail_in == 0))
        {
            TRACE_UNZIP("; all input processed\n");
            // All available input has been processed, everything ok.
            // Set the size to the amount unzipped in this call (including all recursive calls)
            (*lenUnzipped) = (zS->total_out - block->totalUnzipped);
            block->totalUnzipped = zS->total_out;
        }
        else
        {
            // Should not get here unless input data is corrupt
            if (thePrefs.GetVerbose())
            {
                CString strZipError;
                if (zS->msg)
                    strZipError.Format(_T(" %d: '%hs'"), err, zS->msg);
                else if (err != Z_OK)
                    strZipError.Format(_T(" %d: '%hs'"), err, zError(err));
                TRACE_UNZIP("; Error: %s\n", strZipError);
                DebugLogError(_T("Unexpected zip error%s in file \"%s\""), strZipError, reqfile ? reqfile->GetFileName() : NULL);
            }
        }

        if (err != Z_OK)
            (*lenUnzipped) = 0;
    }
    catch (...)
    {
        if (thePrefs.GetVerbose())
            DebugLogError(_T("Unknown exception in %hs: file \"%s\""), __FUNCTION__, reqfile ? reqfile->GetFileName() : NULL);
        err = Z_DATA_ERROR;
        ASSERT(0);
    }

    return err;
}

UINT CUpDownClient::CalculateDownloadRate()
{
    // Patch By BadWolf - Accurate datarate Calculation
    TransferredData newitem = {m_nDownDataRateMS,::GetTickCount()};
    m_AvarageDDR_list.AddTail(newitem);
    m_nSumForAvgDownDataRate += m_nDownDataRateMS;
    m_nDownDataRateMS = 0;

    while (m_AvarageDDR_list.GetCount()>500)
        m_nSumForAvgDownDataRate -= m_AvarageDDR_list.RemoveHead().datalen;

    if (m_AvarageDDR_list.GetCount() > 1)
    {
        DWORD dwDuration = m_AvarageDDR_list.GetTail().timestamp - m_AvarageDDR_list.GetHead().timestamp;
        if (dwDuration)
            m_nDownDatarate = (UINT)(1000U * (ULONGLONG)m_nSumForAvgDownDataRate / dwDuration);
    }
    else
        m_nDownDatarate = 0;
    // END Patch By BadWolf
    m_cShowDR++;
    if (m_cShowDR == 30)
    {
        m_cShowDR = 0;
        UpdateDisplayedInfo();
    }

    return m_nDownDatarate;
}

void CUpDownClient::CheckDownloadTimeout()
{
    if ((::GetTickCount() - m_dwLastBlockReceived) > DOWNLOADTIMEOUT)
    {
        ASSERT(socket != NULL);
        if (socket != NULL)
        {
            ASSERT(!socket->IsRawDataMode());
            if (!socket->IsRawDataMode())
                SendCancelTransfer();
        }
        SetDownloadState(DS_ONQUEUE, _T("Timeout. More than 100 seconds since last complete block was received."));
    }
}

uint16 CUpDownClient::GetAvailablePartCount() const
{
    UINT result = 0;
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (m_pPartStatus == NULL)
        return (uint16) 0;
    for (uint16 i = 0; i < m_pPartStatus->GetPartCount(); ++i)
        //for (UINT i = 0; i < m_nPartCount; ++i)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    {
        if (IsPartAvailable(i))
            ++result;
    }
    return (uint16)result;
}

void CUpDownClient::SetRemoteQueueRank(UINT nr, bool bUpdateDisplay)
{
//>>> WiZaRd::QR History
    if (nr == 0 || GetDownloadState() == DS_NONEEDEDPARTS) //buggy, e.g. Shareaza
        m_iQueueRankDifference = 0;
    else if (m_nRemoteQueueRank /*&& nr != m_nRemoteQueueRank*/)
        m_iQueueRankDifference = (nr - m_nRemoteQueueRank);
//<<< WiZaRd::QR History
    m_nRemoteQueueRank = nr;
    UpdateDisplayedInfo(bUpdateDisplay);
}

void CUpDownClient::UDPReaskACK(uint16 nNewQR)
{
    m_bUDPPending = false;
//>>> WiZaRd::Detect UDP problem clients
    if (m_uiFailedUDPPacketsInARow == 4)
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"DBG: Re-enabled UDP support for %s (%u failed UDP attempts)", DbgGetClientInfo(), m_uiFailedUDPPacketsInARow);
    m_uiFailedUDPPacketsInARow = 0;
//<<< WiZaRd::Detect UDP problem clients
    SetRemoteQueueRank(nNewQR, true);
    SetLastAskedTime();
}

void CUpDownClient::UDPReaskFNF()
{
    m_bUDPPending = false;
//>>> WiZaRd::Detect UDP problem clients
    if (m_uiFailedUDPPacketsInARow == 4)
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"DBG: Re-enabled UDP support for %s (%u failed UDP attempts)", DbgGetClientInfo(), m_uiFailedUDPPacketsInARow);
    m_uiFailedUDPPacketsInARow = 0;
//<<< WiZaRd::Detect UDP problem clients
    if (GetDownloadState() != DS_DOWNLOADING)  // avoid premature deletion of 'this' client
    {
        if (thePrefs.GetVerbose())
            AddDebugLogLine(DLP_LOW, false, _T("UDP FNF-Answer: %s - %s"),DbgGetClientInfo(), DbgGetFileInfo(reqfile ? reqfile->GetFileHash() : NULL));

//>>> WiZaRd::ClientAnalyzer
        //A leecher can tell other users that he does not have the file he/she requests
        //this is pretty bad behavior and up2now a possible exploit!
        //If a source tells he does not have a file we want but requests the SAME file himself
        //then we will drop him if he wants that file...
        //Please note that this is *not* yet 100% secure
        //a client could cancel a file so he would still stay requesting in our queue
        //for ~1h - but he won't be harmed if we drop him
        if (reqfile && GetUploadFileID() != NULL)
        {
            if (md4cmp(GetUploadFileID(), reqfile->GetFileHash()) == 0) //we speak about the same file
            {
                //Packet* response = new Packet(OP_FILENOTFOUND, 0, OP_EMULEPROT);
                //theStats.AddUpDataOverheadFileRequest(response->size);
                //theApp.clientudp->SendPacket(response, GetIP(), GetUDPPort());

                if (thePrefs.GetLogAnalyzerEvents())
                    DebugLogWarning(L"Dropped src: (%s) does not seem to have own reqfile (UDP)!", DbgGetClientInfo());

                if (GetUploadState() != US_NONE)
                {
                    if (theApp.uploadqueue->IsDownloading(this))
                        theApp.uploadqueue->RemoveFromUploadQueue(this, L"Src says he does not have the file he's downloading (UDP)", true);
                    else //if(GetUploadState() == US_ONUPLOADQUEUE)
                        theApp.uploadqueue->RemoveFromWaitingQueue(this);
                }
                if (pAntiLeechData)
                    pAntiLeechData->SetBadForThisSession(AT_UDPFNFFAKER);
            }
            else
            {
                if (pAntiLeechData)
                    pAntiLeechData->ClearBadForThisSession(AT_UDPFNFFAKER);
            }
        }
        else
        {
            if (pAntiLeechData)
                pAntiLeechData->ClearBadForThisSession(AT_UDPFNFFAKER);
        }
//<<< WiZaRd::ClientAnalyzer

        if (reqfile)
            reqfile->m_DeadSourceList.AddDeadSource(this);
        switch (GetDownloadState())
        {
            case DS_ONQUEUE:
            case DS_NONEEDEDPARTS:
                DontSwapTo(reqfile);
                if (SwapToAnotherFile(_T("Source says it doesn't have the file. CUpDownClient::UDPReaskFNF()"), true, true, true, NULL, false, false))
                    break;
            /*fall through*/
            default:
                theApp.downloadqueue->RemoveSource(this);
                if (!socket)
                {
                    if (Disconnected(_T("UDPReaskFNF socket=NULL")))
                        delete this;
                }
        }
    }
    else
    {
        if (thePrefs.GetVerbose())
            DebugLogWarning(_T("UDP FNF-Answer: %s - did not remove client because of current download state"),GetUserName());
    }
}

void CUpDownClient::UDPReaskForDownload()
{
    ASSERT(reqfile);
    if (!reqfile || m_bUDPPending)
        return;

//>>> WiZaRd::Detect UDP problem clients
    if (m_uiFailedUDPPacketsInARow > 3)
        return;
//<<< WiZaRd::Detect UDP problem clients
    if (m_nTotalUDPPackets > 3 && ((float)(m_nFailedUDPPackets/m_nTotalUDPPackets) > .3))
        return;

    if (GetUDPPort() != 0
            && GetUDPVersion() != 0
            && thePrefs.GetUDPPort() != 0
//>>> WiZaRd::ModProt
            //&& !theApp.IsFirewalled()
            && (!theApp.IsFirewalled() || SupportsLowIDUDPPing()) //>>> LowID UDP Ping Support
//<<< WiZaRd::ModProt
            && !(socket && socket->IsConnected())
            && !thePrefs.GetProxySettings().UseProxy)
    {
        if (!HasLowID())
        {
            //don't use udp to ask for sources
            if (IsSourceRequestAllowed())
                return;

            if (SwapToAnotherFile(_T("A4AF check before OP__ReaskFilePing. CUpDownClient::UDPReaskForDownload()"), true, false, false, NULL, true, true))
                return; // we swapped, so need to go to tcp

            m_bUDPPending = true;
            CSafeMemFile data(128);
            data.WriteHash16(reqfile->GetFileHash());
            if (GetUDPVersion() > 3)
                reqfile->WriteSafePartStatus(&data, this, true); //>>> WiZaRd::Intelligent SOTN
            if (GetUDPVersion() > 2)
                data.WriteUInt16(reqfile->m_nCompleteSourcesCount);
            if (thePrefs.GetDebugClientUDPLevel() > 0)
                DebugSend("OP__ReaskFilePing", this, reqfile->GetFileHash());
            Packet* response = new Packet(&data, OP_EMULEPROT);
            response->opcode = OP_REASKFILEPING;
            theStats.AddUpDataOverheadFileRequest(response->size);
            theApp.downloadqueue->AddUDPFileReasks();
            theApp.clientudp->SendPacket(response, GetIP(), GetUDPPort(), ShouldReceiveCryptUDPPackets(), GetUserHash(), false, 0);
            ++m_nTotalUDPPackets;
        }
        else if (HasLowID() && GetBuddyIP() && GetBuddyPort() && HasValidBuddyID())
        {
            m_bUDPPending = true;
            CSafeMemFile data(128);
            data.WriteHash16(GetBuddyID());
            data.WriteHash16(reqfile->GetFileHash());
            if (GetUDPVersion() > 3)
                reqfile->WriteSafePartStatus(&data, this, true); //>>> WiZaRd::Intelligent SOTN
            if (GetUDPVersion() > 2)
                data.WriteUInt16(reqfile->m_nCompleteSourcesCount);
            if (thePrefs.GetDebugClientUDPLevel() > 0)
                DebugSend("OP__ReaskCallbackUDP", this, reqfile->GetFileHash());
            Packet* response = new Packet(&data, OP_EMULEPROT);
            response->opcode = OP_REASKCALLBACKUDP;
            theStats.AddUpDataOverheadFileRequest(response->size);
            theApp.downloadqueue->AddUDPFileReasks();
            // FIXME: We don't know which kadversion the buddy has, so we need to send unencrypted
#ifdef IPV6_SUPPORT
            theApp.clientudp->SendPacket(response, CAddress(_ntohl(GetBuddyIP())), GetBuddyPort(), false, NULL, true, 0); //>>> WiZaRd::IPv6 [Xanatos]
#else
            theApp.clientudp->SendPacket(response, GetBuddyIP(), GetBuddyPort(), false, NULL, true, 0);
#endif
            ++m_nTotalUDPPackets;
        }
    }
}

void CUpDownClient::UpdateDisplayedInfo(bool force)
{
#ifdef _DEBUG
    force = true;
#endif
    DWORD curTick = ::GetTickCount();
    if (force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+m_random_update_wait)
    {
        theApp.emuledlg->transferwnd->GetDownloadList()->UpdateItem(this);
        theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(this);
        theApp.emuledlg->transferwnd->GetDownloadClientsList()->RefreshClient(this);
        m_lastRefreshedDLDisplay = curTick;
    }
}

const bool CUpDownClient::IsInNoNeededList(const CPartFile* fileToCheck) const
{
    for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition(); pos != 0; m_OtherNoNeeded_list.GetNext(pos))
    {
        if (m_OtherNoNeeded_list.GetAt(pos) == fileToCheck)
            return true;
    }

    return false;
}

const bool CUpDownClient::SwapToRightFile(CPartFile* SwapTo, CPartFile* cur_file, bool ignoreSuspensions, bool SwapToIsNNPFile, bool curFileisNNPFile, bool& wasSkippedDueToSourceExchange, bool doAgressiveSwapping, bool debug)
{
    bool printDebug = debug && thePrefs.GetLogA4AF();

    if (printDebug)
    {
        AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: SwapToRightFile. Start compare SwapTo: %s and cur_file %s"), SwapTo?SwapTo->GetFileName():_T("null"), cur_file->GetFileName());
        AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: doAgressiveSwapping: %s"), doAgressiveSwapping?_T("true"):_T("false"));
    }

    if (!SwapTo)
    {
        return true;
    }

    if (!curFileisNNPFile && cur_file->GetSourceCount() < cur_file->GetMaxSources() ||
            curFileisNNPFile && cur_file->GetSourceCount() < cur_file->GetMaxSources()*.8)
    {

        if (printDebug)
            AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: cur_file does probably not have too many sources."));

        if (SwapTo->GetSourceCount() > SwapTo->GetMaxSources() ||
                SwapTo->GetSourceCount() >= SwapTo->GetMaxSources()*.8 &&
                SwapTo == reqfile &&
                (
                    GetDownloadState() == DS_LOWTOLOWIP ||
                    GetDownloadState() == DS_REMOTEQUEUEFULL
                )
           )
        {
            if (printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: SwapTo is about to be deleted due to too many sources on that file, so we can steal it."));
            return true;
        }

        if (ignoreSuspensions  || !IsSwapSuspended(cur_file, doAgressiveSwapping, curFileisNNPFile))
        {
            if (printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: No suspend block."));

            DWORD tempTick = ::GetTickCount();
            bool rightFileHasHigherPrio = CPartFile::RightFileHasHigherPrio(SwapTo, cur_file);
            UINT allNnpReaskTime = FILEREASKTIME*2*(m_OtherNoNeeded_list.GetSize() + ((GetDownloadState() == DS_NONEEDEDPARTS)?1:0)); // wait two reask interval for each nnp file before reasking an nnp file

            if (!SwapToIsNNPFile && (!curFileisNNPFile || GetLastAskedTime(cur_file) == 0 || tempTick-GetLastAskedTime(cur_file) > allNnpReaskTime) && rightFileHasHigherPrio ||
                    SwapToIsNNPFile && curFileisNNPFile &&
                    (
                        GetLastAskedTime(SwapTo) != 0 &&
                        (
                            GetLastAskedTime(cur_file) == 0 ||
                            tempTick-GetLastAskedTime(SwapTo) < tempTick-GetLastAskedTime(cur_file) && (tempTick-GetLastAskedTime(cur_file) > allNnpReaskTime || rightFileHasHigherPrio && tempTick-GetLastAskedTime(SwapTo) < allNnpReaskTime)
                        ) ||
                        rightFileHasHigherPrio && GetLastAskedTime(SwapTo) == 0 && GetLastAskedTime(cur_file) == 0
                    ) ||
                    SwapToIsNNPFile && !curFileisNNPFile)
            {
                if (printDebug)
                    if (!SwapToIsNNPFile && !curFileisNNPFile && rightFileHasHigherPrio)
                        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Higher prio."));
                    else if (!SwapToIsNNPFile && (GetLastAskedTime(cur_file) == 0 || tempTick-GetLastAskedTime(cur_file) > allNnpReaskTime) && rightFileHasHigherPrio)
                        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Time to reask nnp and it had higher prio."));
                    else if (GetLastAskedTime(SwapTo) != 0 &&
                             (
                                 GetLastAskedTime(cur_file) == 0 ||
                                 tempTick-GetLastAskedTime(SwapTo) < tempTick-GetLastAskedTime(cur_file) && (tempTick-GetLastAskedTime(cur_file) > allNnpReaskTime || rightFileHasHigherPrio && tempTick-GetLastAskedTime(SwapTo) < allNnpReaskTime)
                             )
                            )
                        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Both nnp and cur_file has longer time since reasked."));
                    else if (SwapToIsNNPFile && !curFileisNNPFile)
                        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: SwapToIsNNPFile && !curFileisNNPFile"));
                    else
                        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Higher prio for unknown reason!"));

                if (IsSourceRequestAllowed(cur_file) && (cur_file->AllowSwapForSourceExchange() || cur_file == reqfile && RecentlySwappedForSourceExchange()) ||
                        !(IsSourceRequestAllowed(SwapTo) && (SwapTo->AllowSwapForSourceExchange() || SwapTo == reqfile && RecentlySwappedForSourceExchange())) ||
                        (GetDownloadState()==DS_ONQUEUE && GetRemoteQueueRank() <= 50))
                {
                    if (printDebug)
                        AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: Source Request check ok."));
                    return true;
                }
                else
                {
                    if (printDebug)
                        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Source Request check failed."));
                    wasSkippedDueToSourceExchange = true;
                }
            }

            if (IsSourceRequestAllowed(cur_file, true) && (cur_file->AllowSwapForSourceExchange() || cur_file == reqfile && RecentlySwappedForSourceExchange()) &&
                    !(IsSourceRequestAllowed(SwapTo, true) && (SwapTo->AllowSwapForSourceExchange() || SwapTo == reqfile && RecentlySwappedForSourceExchange())) &&
                    (GetDownloadState()!=DS_ONQUEUE || GetDownloadState()==DS_ONQUEUE && GetRemoteQueueRank() > 50))
            {
                wasSkippedDueToSourceExchange = true;

                if (printDebug)
                    AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: Source Exchange."));
                return true;
            }
        }
        else if (printDebug)
        {
            AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Suspend block."));
        }
    }
    else if (printDebug)
    {
        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: cur_file probably has too many sources."));
    }

    if (printDebug)
        AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: Return false"));

    return false;
}

bool CUpDownClient::SwapToAnotherFile(LPCTSTR reason, bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile, bool allowSame, bool isAboutToAsk, bool debug)
{
    bool printDebug = debug && thePrefs.GetLogA4AF();

    if (printDebug)
        AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Switching source %s Remove = %s; bIgnoreNoNeeded = %s; allowSame = %s; Reason = \"%s\""), DbgGetClientInfo(), (bRemoveCompletely ? GetResString(IDS_YES) : GetResString(IDS_NO)), (bIgnoreNoNeeded ? GetResString(IDS_YES) : GetResString(IDS_NO)), (allowSame ? GetResString(IDS_YES) : GetResString(IDS_NO)), reason);

    if (!bRemoveCompletely && allowSame && thePrefs.GetA4AFSaveCpu())
    {
        // Only swap if we can't keep the old source
        if (printDebug)
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: return false since prefs setting to save cpu is enabled."));
        return false;
    }

    bool doAgressiveSwapping = (bRemoveCompletely || !allowSame || isAboutToAsk);
    if (printDebug)
        AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: doAgressiveSwapping: %s"), doAgressiveSwapping?_T("true"):_T("false"));

    if (!bRemoveCompletely && !ignoreSuspensions && allowSame && GetTimeUntilReask(reqfile, doAgressiveSwapping, true, false) > 0 && (GetDownloadState() != DS_NONEEDEDPARTS || m_OtherRequests_list.IsEmpty()))
    {
        if (printDebug)
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: return false due to not reached reask time: GetTimeUntilReask(...) > 0"));

        return false;
    }

    if (!bRemoveCompletely && allowSame && m_OtherRequests_list.IsEmpty() && (/* !bIgnoreNoNeeded ||*/ m_OtherNoNeeded_list.IsEmpty()))
    {
        // no file to swap too, and it's ok to keep it
        if (printDebug)
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: return false due to no file to swap too, and it's ok to keep it."));
        return false;
    }

    if (!bRemoveCompletely &&
            (GetDownloadState() != DS_ONQUEUE &&
             GetDownloadState() != DS_NONEEDEDPARTS &&
             GetDownloadState() != DS_TOOMANYCONNS &&
             GetDownloadState() != DS_REMOTEQUEUEFULL &&
             GetDownloadState() != DS_CONNECTED
            ))
    {
        if (printDebug)
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: return false due to wrong state."));
        return false;
    }

    CPartFile* SwapTo = NULL;
    CPartFile* cur_file = NULL;
    POSITION finalpos = NULL;
    CTypedPtrList<CPtrList, CPartFile*>* usedList = NULL;

    if (allowSame && !bRemoveCompletely)
    {
        SwapTo = reqfile;
        if (printDebug)
            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: allowSame: File %s SourceReq: %s"), reqfile->GetFileName(), IsSourceRequestAllowed(reqfile)?_T("true"):_T("false"));
    }

    bool SwapToIsNNP = (SwapTo != NULL && SwapTo == reqfile && GetDownloadState() == DS_NONEEDEDPARTS);

    CPartFile* skippedDueToSourceExchange = NULL;
    bool skippedIsNNP = false;

    if (!m_OtherRequests_list.IsEmpty())
    {
        if (printDebug)
            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: m_OtherRequests_list"));

        for (POSITION pos = m_OtherRequests_list.GetHeadPosition(); pos != 0; m_OtherRequests_list.GetNext(pos))
        {
            cur_file = m_OtherRequests_list.GetAt(pos);

            if (printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Checking file: %s SoureReq: %s"), cur_file->GetFileName(), IsSourceRequestAllowed(cur_file)?_T("true"):_T("false"));

            if (!bRemoveCompletely && !ignoreSuspensions && allowSame && IsSwapSuspended(cur_file, doAgressiveSwapping, false))
            {
                if (printDebug)
                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: continue due to IsSwapSuspended(file) == true"));
                continue;
            }

            if (cur_file != reqfile && theApp.downloadqueue->IsPartFile(cur_file) && !cur_file->IsStopped()
                    && (cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY))
            {
                if (printDebug)
                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: It's a partfile, not stopped, etc."));

                if (toFile != NULL)
                {
                    if (cur_file == toFile)
                    {
                        if (printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Found toFile."));

                        SwapTo = cur_file;
                        SwapToIsNNP = false;
                        usedList = &m_OtherRequests_list;
                        finalpos = pos;
                        break;
                    }
                }
                else
                {
                    bool wasSkippedDueToSourceExchange = false;
                    if (SwapToRightFile(SwapTo, cur_file, ignoreSuspensions, SwapToIsNNP, false, wasSkippedDueToSourceExchange, doAgressiveSwapping, debug))
                    {
                        if (printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Swapping to file %s"), cur_file->GetFileName());

                        if (SwapTo && wasSkippedDueToSourceExchange)
                        {
                            if (debug && thePrefs.GetLogA4AF()) AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Swapped due to source exchange possibility"));
                            bool discardSkipped = false;
                            if (SwapToRightFile(skippedDueToSourceExchange, SwapTo, ignoreSuspensions, skippedIsNNP, SwapToIsNNP, discardSkipped, doAgressiveSwapping, debug))
                            {
                                skippedDueToSourceExchange = SwapTo;
                                skippedIsNNP = skippedIsNNP?true:(SwapTo == reqfile && GetDownloadState() == DS_NONEEDEDPARTS);
                                if (printDebug)
                                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Skipped file was better than last skipped file."));
                            }
                        }

                        SwapTo = cur_file;
                        SwapToIsNNP = false;
                        usedList = &m_OtherRequests_list;
                        finalpos=pos;
                    }
                    else
                    {
                        if (printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Keeping file %s"), SwapTo->GetFileName());
                        if (wasSkippedDueToSourceExchange)
                        {
                            if (printDebug)
                                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Kept the file due to source exchange possibility"));
                            bool discardSkipped = false;
                            if (SwapToRightFile(skippedDueToSourceExchange, cur_file, ignoreSuspensions, skippedIsNNP, false, discardSkipped, doAgressiveSwapping, debug))
                            {
                                skippedDueToSourceExchange = cur_file;
                                skippedIsNNP = false;
                                if (printDebug)
                                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Skipped file was better than last skipped file."));
                            }
                        }
                    }
                }
            }
        }
    }

    //if ((!SwapTo || SwapTo == reqfile && GetDownloadState() == DS_NONEEDEDPARTS) && bIgnoreNoNeeded){
    if (printDebug)
        AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: m_OtherNoNeeded_list"));

    for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition(); pos != 0; m_OtherNoNeeded_list.GetNext(pos))
    {
        cur_file = m_OtherNoNeeded_list.GetAt(pos);

        if (printDebug)
            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Checking file: %s "), cur_file->GetFileName());

        if (!bRemoveCompletely && !ignoreSuspensions && allowSame && IsSwapSuspended(cur_file, doAgressiveSwapping, true))
        {
            if (printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: continue due to !IsSwapSuspended(file) == true"));
            continue;
        }

        if (cur_file != reqfile && theApp.downloadqueue->IsPartFile(cur_file) && !cur_file->IsStopped()
                && (cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY))
        {
            if (printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: It's a partfile, not stopped, etc."));

            if (toFile != NULL)
            {
                if (cur_file == toFile)
                {
                    if (printDebug)
                        AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Found toFile."));

                    SwapTo = cur_file;
                    usedList = &m_OtherNoNeeded_list;
                    finalpos = pos;
                    break;
                }
            }
            else
            {
                bool wasSkippedDueToSourceExchange = false;
                if (SwapToRightFile(SwapTo, cur_file, ignoreSuspensions, SwapToIsNNP, true, wasSkippedDueToSourceExchange, doAgressiveSwapping, debug))
                {
                    if (printDebug)
                        AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Swapping to file %s"), cur_file->GetFileName());

                    if (SwapTo && wasSkippedDueToSourceExchange)
                    {
                        if (printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Swapped due to source exchange possibility"));
                        bool discardSkipped = false;
                        if (SwapToRightFile(skippedDueToSourceExchange, SwapTo, ignoreSuspensions, skippedIsNNP, SwapToIsNNP, discardSkipped, doAgressiveSwapping, debug))
                        {
                            skippedDueToSourceExchange = SwapTo;
                            skippedIsNNP = skippedIsNNP?true:(SwapTo == reqfile && GetDownloadState() == DS_NONEEDEDPARTS);
                            if (printDebug)
                                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Skipped file was better than last skipped file."));
                        }
                    }

                    SwapTo = cur_file;
                    SwapToIsNNP = true;
                    usedList = &m_OtherNoNeeded_list;
                    finalpos=pos;
                }
                else
                {
                    if (printDebug)
                        AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Keeping file %s"), SwapTo->GetFileName());
                    if (wasSkippedDueToSourceExchange)
                    {
                        if (debug && thePrefs.GetVerbose()) AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Kept the file due to source exchange possibility"));
                        bool discardSkipped = false;
                        if (SwapToRightFile(skippedDueToSourceExchange, cur_file, ignoreSuspensions, skippedIsNNP, true, discardSkipped, doAgressiveSwapping, debug))
                        {
                            skippedDueToSourceExchange = cur_file;
                            skippedIsNNP = true;
                            if (printDebug)
                                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Skipped file was better than last skipped file."));
                        }
                    }
                }
            }
        }
    }
    //}

    if (SwapTo)
    {
        if (printDebug)
        {
            if (SwapTo != reqfile)
            {
                AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Found file to swap to %s"), SwapTo->GetFileName());
            }
            else
            {
                AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Will keep current file. %s"), SwapTo->GetFileName());
            }
        }

        CString strInfo(reason);
        if (skippedDueToSourceExchange)
        {
            bool wasSkippedDueToSourceExchange = false;
            bool skippedIsBetter = SwapToRightFile(SwapTo, skippedDueToSourceExchange, ignoreSuspensions, SwapToIsNNP, skippedIsNNP, wasSkippedDueToSourceExchange, doAgressiveSwapping, debug);
            if (skippedIsBetter || wasSkippedDueToSourceExchange)
            {
                SwapTo->SetSwapForSourceExchangeTick();
                SetSwapForSourceExchangeTick();

                strInfo = _T("******SourceExchange-Swap****** ") + strInfo;
                if (printDebug)
                {
                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Due to sourceExchange."));
                }
                else if (thePrefs.GetLogA4AF() && reqfile == SwapTo)
                {
                    AddDebugLogLine(DLP_LOW, false, _T("ooo Didn't swap source due to source exchange possibility. %s Remove = %s '%s' Reason: %s"), DbgGetClientInfo(), (bRemoveCompletely ? GetResString(IDS_YES) : GetResString(IDS_NO)), (this->reqfile)?this->reqfile->GetFileName():_T("null"), strInfo);
                }
            }
            else if (printDebug)
            {
                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Normal. SwapTo better than skippedDueToSourceExchange."));
            }
        }
        else if (printDebug)
        {
            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Normal. skippedDueToSourceExchange == NULL"));
        }

        if (SwapTo != reqfile && DoSwap(SwapTo,bRemoveCompletely, strInfo))
        {
            if (debug && thePrefs.GetLogA4AF()) AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Swap successful."));
            if (usedList && finalpos)
            {
                usedList->RemoveAt(finalpos);
            }
            return true;
        }
        else if (printDebug)
        {
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Swap didn't happen."));
        }
    }

    if (printDebug)
        AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Done %s"), DbgGetClientInfo());

    return false;
}

bool CUpDownClient::DoSwap(CPartFile* SwapTo, bool bRemoveCompletely, LPCTSTR reason)
{
    if (thePrefs.GetLogA4AF())
        AddDebugLogLine(DLP_LOW, false, _T("ooo Swapped source %s Remove = %s '%s'   -->   %s Reason: %s"), DbgGetClientInfo(), (bRemoveCompletely ? GetResString(IDS_YES) : GetResString(IDS_NO)), (this->reqfile)?this->reqfile->GetFileName():_T("null"), SwapTo->GetFileName(), reason);

    // 17-Dez-2003 [bc]: This "reqfile->srclists[sourcesslot].Find(this)" was the only place where
    // the usage of the "CPartFile::srclists[100]" is more effective than using one list. If this
    // function here is still (again) a performance problem there is a more effective way to handle
    // the 'Find' situation. Hint: usage of a node ptr which is stored in the CUpDownClient.
    POSITION pos = reqfile->srclist.Find(this);
    if (pos)
    {
        reqfile->srclist.RemoveAt(pos);
    }
    else
    {
        AddDebugLogLine(DLP_HIGH, true, _T("o-o Unsync between parfile->srclist and client otherfiles list. Swapping client where client has file as reqfile, but file doesn't have client in srclist. %s Remove = %s '%s'   -->   '%s'  SwapReason: %s"), DbgGetClientInfo(), (bRemoveCompletely ? GetResString(IDS_YES) : GetResString(IDS_NO)), (this->reqfile)?this->reqfile->GetFileName():_T("null"), SwapTo->GetFileName(), reason);
    }

    // remove this client from the A4AF list of our new reqfile
    POSITION pos2 = SwapTo->A4AFsrclist.Find(this);
    if (pos2)
    {
        SwapTo->A4AFsrclist.RemoveAt(pos2);
    }
    else
    {
        AddDebugLogLine(DLP_HIGH, true, _T("o-o Unsync between parfile->srclist and client otherfiles list. Swapping client where client has file in another list, but file doesn't have client in a4af srclist. %s Remove = %s '%s'   -->   '%s'  SwapReason: %s"), DbgGetClientInfo(), (bRemoveCompletely ? GetResString(IDS_YES) : GetResString(IDS_NO)), (this->reqfile)?this->reqfile->GetFileName():_T("null"), SwapTo->GetFileName(), reason);
    }
    theApp.emuledlg->transferwnd->GetDownloadList()->RemoveSource(this,SwapTo);

    reqfile->RemoveDownloadingSource(this);

    if (!bRemoveCompletely)
    {
        reqfile->A4AFsrclist.AddTail(this);
        if (GetDownloadState() == DS_NONEEDEDPARTS)
            m_OtherNoNeeded_list.AddTail(reqfile);
        else
            m_OtherRequests_list.AddTail(reqfile);

        theApp.emuledlg->transferwnd->GetDownloadList()->AddSource(reqfile,this,true);
    }
    else
    {
        m_fileReaskTimes.RemoveKey(reqfile);
    }

    SetDownloadState(DS_NONE);
    CPartFile* pOldRequestFile = reqfile;
    SetRequestFile(SwapTo);
    pOldRequestFile->UpdatePartsInfo();
    pOldRequestFile->UpdateAvailablePartsCount();

    SwapTo->srclist.AddTail(this);
    theApp.emuledlg->transferwnd->GetDownloadList()->AddSource(SwapTo,this,false);

    return true;
}

void CUpDownClient::DontSwapTo(/*const*/ CPartFile* file)
{
    DWORD dwNow = ::GetTickCount();

    for (POSITION pos = m_DontSwap_list.GetHeadPosition(); pos != 0; m_DontSwap_list.GetNext(pos))
        if (m_DontSwap_list.GetAt(pos).file == file)
        {
            m_DontSwap_list.GetAt(pos).timestamp = dwNow ;
            return;
        }
    PartFileStamp newfs = {file, dwNow };
    m_DontSwap_list.AddHead(newfs);
}

bool CUpDownClient::IsSwapSuspended(const CPartFile* file, const bool allowShortReaskTime, const bool fileIsNNP)
{
    if (file == reqfile)
    {
        return false;
    }

    // Don't swap if we have reasked this client too recently
    if (GetTimeUntilReask(file, allowShortReaskTime, true, fileIsNNP) > 0)
        return true;

    if (m_DontSwap_list.GetCount()==0)
        return false;

    for (POSITION pos = m_DontSwap_list.GetHeadPosition(); pos != 0 && m_DontSwap_list.GetCount()>0; m_DontSwap_list.GetNext(pos))
    {
        if (m_DontSwap_list.GetAt(pos).file == file)
        {
            if (::GetTickCount() - m_DontSwap_list.GetAt(pos).timestamp  >= PURGESOURCESWAPSTOP)
            {
                m_DontSwap_list.RemoveAt(pos);
                return false;
            }
            else
                return true;
        }
        else if (m_DontSwap_list.GetAt(pos).file == NULL) // in which cases should this happen?
            m_DontSwap_list.RemoveAt(pos);
    }

    return false;
}

UINT CUpDownClient::GetTimeUntilReask(const CPartFile* file, const bool allowShortReaskTime, const bool useGivenNNP, const bool givenNNP) const
{
    DWORD lastAskedTimeTick = GetLastAskedTime(file);
    if (lastAskedTimeTick != 0)
    {
        DWORD tick = ::GetTickCount();

        DWORD reaskTime;
        if (allowShortReaskTime || file == reqfile && GetDownloadState() == DS_NONE)
        {
            reaskTime = MIN_REQUESTTIME;
        }
//>>> WiZaRd::ClientAnalyzer
        //I decided to add that to CA to balance the scales
        else if (pAntiLeechData && pAntiLeechData->GetReaskTime() != FILEREASKTIME)
            reaskTime = pAntiLeechData->GetReaskTime();
//>>> Spike2::Emulate other [Spike2]
        // mlDonkey uses a ReaskTime of 10 Minutes. So we use the same ReaskTime for them (suggestion by spiralvoice from mlDonkey-DevTeam).
        //WiZaRd: moved it up, otherwise it's useless :)
        //seeing that MLDonkey has a reask which is ~3 times as fast as eMules and MLDonkey downloads ~3 times as much from each other as an eMule client
        //this seems to be a sensible approach, though hammering is NOT a good solution...
        //WiZaRd: removed the switch here because this should always be enabled to ensure fairness
        else if (GetClientSoft() == SO_MLDONKEY /*&& thePrefs.IsEmuMLDonkey()*/)
            reaskTime = MIN_REQUESTTIME;
        // Shareaza allows its users to lower the ReaskTime down to 20 Minutes !! Okay, so do we.... but hardcoded.
        else if (GetClientSoft() == SO_SHAREAZA)
            reaskTime = MIN_REQUESTTIME*2;
        else if (GetClientSoft() == SO_EMULEPLUS)
            reaskTime = MIN2MS(28);
//<<< Spike2::Emulate other [Spike2]
//>>> WiZaRd::BadClientFlag
        //currently this isn't necessary but we might add spread reask later and
        //in that case we have to make an exception for those bad clients
        else if (IsBadClient())
            reaskTime = MIN2MS(29);
//<<< WiZaRd::BadClientFlag
//<<< WiZaRd::ClientAnalyzer
        else if (useGivenNNP && givenNNP ||
                 file == reqfile && GetDownloadState() == DS_NONEEDEDPARTS ||
                 file != reqfile && IsInNoNeededList(file))
        {
            reaskTime = FILEREASKTIME*2;
        }
        else
        {
            reaskTime = FILEREASKTIME;
        }

        if (tick-lastAskedTimeTick < reaskTime)
        {
            return reaskTime-(tick-lastAskedTimeTick);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

UINT CUpDownClient::GetTimeUntilReask(const CPartFile* file) const
{
    return GetTimeUntilReask(file, false);
}

UINT CUpDownClient::GetTimeUntilReask() const
{
    return GetTimeUntilReask(reqfile);
}

bool CUpDownClient::IsValidSource() const
{
    bool valid = false;
    switch (GetDownloadState())
    {
        case DS_DOWNLOADING:
        case DS_ONQUEUE:
        case DS_CONNECTED:
        case DS_NONEEDEDPARTS:
        case DS_REMOTEQUEUEFULL:
        case DS_REQHASHSET:
            valid = IsEd2kClient();
    }
    return valid;
}

void CUpDownClient::StartDownload()
{
    SetDownloadState(DS_DOWNLOADING);
    socket->ExpandReceiveBuffer(); //>>> WiZaRd::ZZUL Upload [ZZ]
    InitTransferredDownMini();
    SetDownStartTime();
    m_lastPartAsked = (uint16)-1;
    SendBlockRequests();
}

void CUpDownClient::SendCancelTransfer(Packet* packet)
{
    if (socket == NULL || !IsEd2kClient())
    {
        ASSERT(0);
        return;
    }

    if (!GetSentCancelTransfer())
    {
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__CancelTransfer", this);

        bool bDeletePacket;
        Packet* pCancelTransferPacket;
        if (packet)
        {
            pCancelTransferPacket = packet;
            bDeletePacket = false;
        }
        else
        {
            pCancelTransferPacket = new Packet(OP_CANCELTRANSFER, 0);
            bDeletePacket = true;
        }
        theStats.AddUpDataOverheadFileRequest(pCancelTransferPacket->size);
        socket->SendPacket(pCancelTransferPacket,bDeletePacket,true);
        SetSentCancelTransfer(1);
    }
}

void CUpDownClient::SetRequestFile(CPartFile* pReqFile)
{
    if (pReqFile != reqfile || reqfile == NULL)
        ResetFileStatusInfo();
    reqfile = pReqFile;
}

void CUpDownClient::ProcessAcceptUpload()
{
    m_fQueueRankPending = 1;
    if (reqfile && !reqfile->IsStopped() && (reqfile->GetStatus()==PS_READY || reqfile->GetStatus()==PS_EMPTY))
    {
        SetSentCancelTransfer(0);
        if (GetDownloadState() == DS_ONQUEUE)
            StartDownload();
    }
    else
    {
        SendCancelTransfer();
        SetDownloadState((reqfile==NULL || reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
    }
}

void CUpDownClient::ProcessEdonkeyQueueRank(const uchar* packet, UINT size)
{
    CSafeMemFile data(packet, size);
    UINT rank = data.ReadUInt32();
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        Debug(_T("  QR=%u (prev. %d)\n"), rank, IsRemoteQueueFull() ? (UINT)-1 : (UINT)GetRemoteQueueRank());
    SetRemoteQueueRank(rank, GetDownloadState() == DS_ONQUEUE);
    CheckQueueRankFlood();
}

void CUpDownClient::ProcessEmuleQueueRank(const uchar* packet, UINT size)
{
    if (size != 12)
        throw GetResString(IDS_ERR_BADSIZE);
    uint16 rank = PeekUInt16(packet);
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        Debug(_T("  QR=%u\n"), rank); // no prev. QR available for eMule clients
    SetRemoteQueueFull(false);
    SetRemoteQueueRank(rank, GetDownloadState() == DS_ONQUEUE);
    CheckQueueRankFlood();
}

void CUpDownClient::CheckQueueRankFlood()
{
    if (m_fQueueRankPending == 0)
    {
        if (GetDownloadState() != DS_DOWNLOADING)
        {
            if (m_fUnaskQueueRankRecv < 3) // NOTE: Do not increase this nr. without increasing the bits for 'm_fUnaskQueueRankRecv'
                m_fUnaskQueueRankRecv++;
            if (m_fUnaskQueueRankRecv == 3)
            {
                if (theApp.clientlist->GetBadRequests(this) < 2)
                    theApp.clientlist->TrackBadRequest(this, 1);
                if (theApp.clientlist->GetBadRequests(this) == 2)
                {
                    theApp.clientlist->TrackBadRequest(this, -2); // reset so the client will not be rebanned right after the ban is lifted
                    Ban(_T("QR flood"));
                }
                throw CString(thePrefs.GetLogBannedClients() ? _T("QR flood") : L"");
            }
        }
    }
    else
    {
        m_fQueueRankPending = 0;
        m_fUnaskQueueRankRecv = 0;
    }
}

UINT CUpDownClient::GetLastAskedTime(const CPartFile* partFile) const
{
    CPartFile* file = (CPartFile*)partFile;
    if (file == NULL)
        file = reqfile;

    DWORD lastChangedTick;
    return m_fileReaskTimes.Lookup(file, lastChangedTick)? lastChangedTick : 0;
}

void CUpDownClient::SetReqFileAICHHash(CAICHHash* val)
{
    // TODO fileident optimize to save some memory
    if (m_pReqFileAICHHash != NULL && m_pReqFileAICHHash != val)
        delete m_pReqFileAICHHash;
    m_pReqFileAICHHash = val;
}

void CUpDownClient::SendAICHRequest(CPartFile* pForFile, uint16 nPart)
{
    ASSERT(m_fAICHRequested == NULL);
    CAICHRequestedData request;
    request.m_nPart = nPart;
    request.m_pClient = this;
    request.m_pPartFile = pForFile;
    CAICHRecoveryHashSet::m_liRequestedData.AddTail(request);
    m_fAICHRequested = TRUE;
    CSafeMemFile data;
    data.WriteHash16(pForFile->GetFileHash());
    data.WriteUInt16(nPart);
    pForFile->GetAICHRecoveryHashSet()->GetMasterHash().Write(&data);
    Packet* packet = new Packet(&data, OP_EMULEPROT, OP_AICHREQUEST);
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__AichRequest", this, (uchar*)packet->pBuffer);
    theStats.AddUpDataOverheadFileRequest(packet->size);
    SafeConnectAndSendPacket(packet);
}

void CUpDownClient::ProcessAICHAnswer(const uchar* packet, UINT size)
{
    if (!IsAICHReqPending())
        throw CString(L"Received unrequested AICH Packet");
    m_fAICHRequested = FALSE;
    if (size < 16)
    {
        CAICHRecoveryHashSet::ClientAICHRequestFailed(this);
        throw CString(L"Received AICH Answer Packet with wrong size");
    }
    else if (size != 16)
    {
        CSafeMemFile data(packet, size);
        uchar abyHash[16];
        data.ReadHash16(abyHash);
        CPartFile* pPartFile = theApp.downloadqueue->GetFileByID(abyHash);
        if (pPartFile != NULL)
        {
            CAICHRequestedData request = CAICHRecoveryHashSet::GetAICHReqDetails(this);
            if (request.m_pPartFile == pPartFile)
            {
                if (request.m_pClient == this)
                {
                    uint16 nPart = data.ReadUInt16();
                    if (nPart == request.m_nPart)
                    {
                        CAICHHash ahMasterHash(&data);
                        if (pPartFile->GetAICHRecoveryHashSet()->GetStatus() == AICH_TRUSTED || pPartFile->GetAICHRecoveryHashSet()->GetStatus() == AICH_VERIFIED)
                        {
                            if (ahMasterHash == pPartFile->GetAICHRecoveryHashSet()->GetMasterHash())
                            {
                                if (pPartFile->GetAICHRecoveryHashSet()->ReadRecoveryData((uint64)request.m_nPart*PARTSIZE, &data))
                                {
                                    // finally all checks passed, everything seem to be fine
                                    theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"AICH Packet Answer: Succeeded to read and validate received recoverydata");
                                    CAICHRecoveryHashSet::RemoveClientAICHRequest(this);
                                    pPartFile->AICHRecoveryDataAvailable(request.m_nPart);
                                    return;
                                }
                                else
                                    theApp.QueueDebugLogLineEx(LOG_ERROR, L"AICH Packet Answer: Failed to read and validate received recoverydata from %s", DbgGetClientInfo());
                            }
                            else
                                theApp.QueueDebugLogLineEx(LOG_ERROR, L"AICH Packet Answer: Masterhash differs from packethash in packet from %s", DbgGetClientInfo());
                        }
                        else
                            theApp.QueueDebugLogLineEx(LOG_ERROR, L"AICH Packet Answer: Hashset has no trusted Masterhash in packet from %s", DbgGetClientInfo());
                    }
                    else
                        theApp.QueueDebugLogLineEx(LOG_ERROR, L"AICH Packet Answer: received data for part %u instead of %u from %s", nPart, request.m_nPart, DbgGetClientInfo());
                }
                else
                    theApp.QueueDebugLogLineEx(LOG_ERROR, L"AICH Packet Answer: received data from client '%s' instead of '%s' (should never happen)", DbgGetClientInfo(), request.m_pClient->DbgGetClientInfo());
            }
            else
                theApp.QueueDebugLogLineEx(LOG_ERROR, L"AICH Packet Answer: received data for file '%s' instead of '%s' from %s", pPartFile->GetFileName(), request.m_pPartFile->GetFileName(), DbgGetClientInfo());
        }
        else
            theApp.QueueDebugLogLineEx(LOG_ERROR, L"AICH Packet Answer: file with hash %s not found in packet from %s", md4str(abyHash), DbgGetClientInfo());
    }
    CAICHRecoveryHashSet::ClientAICHRequestFailed(this);
}

void CUpDownClient::ProcessAICHRequest(const uchar* packet, UINT size)
{
    if (size != (UINT)(16 + 2 + CAICHHash::GetHashSize()))
        throw L"Received AICH Request Packet with wrong size";

    CSafeMemFile data(packet, size);
    uchar abyHash[16];
    data.ReadHash16(abyHash);
    uint16 nPart = data.ReadUInt16();
    CAICHHash ahMasterHash(&data);
    CKnownFile* pKnownFile = theApp.sharedfiles->GetFileByID(abyHash);
    if (pKnownFile != NULL)
    {
        if (pKnownFile->GetFileIdentifier().HasAICHHash())
        {
            if (pKnownFile->IsAICHRecoverHashSetAvailable())
            {
                if (pKnownFile->GetPartCount() > nPart)
                {
                    if (pKnownFile->GetFileSize() > (uint64)EMBLOCKSIZE && (uint64)pKnownFile->GetFileSize() - PARTSIZE*(uint64)nPart > EMBLOCKSIZE)
                    {
                        if (pKnownFile->GetFileIdentifier().GetAICHHash() == ahMasterHash)
                        {
                            CSafeMemFile fileResponse;
                            fileResponse.WriteHash16(pKnownFile->GetFileHash());
                            fileResponse.WriteUInt16(nPart);
                            pKnownFile->GetFileIdentifier().GetAICHHash().Write(&fileResponse);
                            CAICHRecoveryHashSet recHashSet(pKnownFile, pKnownFile->GetFileSize());
                            recHashSet.SetMasterHash(pKnownFile->GetFileIdentifier().GetAICHHash(), AICH_HASHSETCOMPLETE);
                            if (recHashSet.CreatePartRecoveryData((uint64)nPart*PARTSIZE, &fileResponse))
                            {
                                AddDebugLogLine(DLP_HIGH, false, L"AICH Packet Request: Successfully created and sent recoverydata for %s to %s", pKnownFile->GetFileName(), DbgGetClientInfo());
                                if (thePrefs.GetDebugClientTCPLevel() > 0)
                                    DebugSend("OP__AichAnswer", this, pKnownFile->GetFileHash());
                                Packet* packAnswer = new Packet(&fileResponse, OP_EMULEPROT, OP_AICHANSWER);
                                theStats.AddUpDataOverheadFileRequest(packAnswer->size);
                                SafeConnectAndSendPacket(packAnswer);
                                return;
                            }
                            else
                                AddDebugLogLine(DLP_HIGH, false, L"AICH Packet Request: Failed to create recoverydata for %s to %s", pKnownFile->GetFileName(), DbgGetClientInfo());
                        }
                        else
                            AddDebugLogLine(DLP_HIGH, false, L"AICH Packet Request: Failed to create recoverydata - requested Hash differs from Masterhash for %s to %s", pKnownFile->GetFileName(), DbgGetClientInfo());
                    }
                    else
                        AddDebugLogLine(DLP_HIGH, false, L"AICH Packet Request: Failed to create recoverydata - partsize insufficient for %s to %s", pKnownFile->GetFileName(), DbgGetClientInfo());
                }
                else
                    AddDebugLogLine(DLP_HIGH, false, L"AICH Packet Request: Failed to create recoverydata - part out of bounds for %s to %s", pKnownFile->GetFileName(), DbgGetClientInfo());
            }
            else
                AddDebugLogLine(DLP_HIGH, false, L"AICH Packet Request: Failed to create recoverydata - Hashset not ready for %s to %s", pKnownFile->GetFileName(), DbgGetClientInfo());
        }
        else
            AddDebugLogLine(DLP_HIGH, false, L"AICH Packet Request: Failed to create recoverydata - AICH hash missing for %s to %s", pKnownFile->GetFileName(), DbgGetClientInfo());
    }
    else
        AddDebugLogLine(DLP_HIGH, false, L"AICH Packet Request: Failed to find requested shared file with hash %s -  %s", md4str(abyHash), DbgGetClientInfo());

    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__AichAnswer", this, abyHash);
    Packet* packAnswer = new Packet(OP_AICHANSWER, 16, OP_EMULEPROT);
    md4cpy(packAnswer->pBuffer, abyHash);
    //PokeUInt16(packAnswer->pBuffer+16, nPart);
    theStats.AddUpDataOverheadFileRequest(packAnswer->size);
    SafeConnectAndSendPacket(packAnswer);
}

void CUpDownClient::ProcessAICHFileHash(CSafeMemFile* data, CPartFile* file, const CAICHHash* pAICHHash)
{
    CPartFile* pPartFile = file;
    if (pPartFile == NULL && data != NULL)
    {
        uchar abyHash[16];
        data->ReadHash16(abyHash);
        pPartFile = theApp.downloadqueue->GetFileByID(abyHash);
    }
//>>> WiZaRd::ClientAnalyzer
//>>> WiZaRd::Additional Security Check
    CString msg = L"";
//     if (!IsCompleteSource() && file == reqfile)
//     {
//         theApp.QueueDebugLogLineEx(LOG_WARNING, L"Received AICH hash from client %s that CAN not have the AICH hash", DbgGetClientInfo());
//         //TODO? mark all chunks as being "available"
//         //Also: is this logline still correct with >v0.50?
//     }
    if (!IsSupportingAICH())
        msg.Format(L"Client %s sent AICH hash without supporting the protocol!", DbgGetClientInfo());
    //>0.50 sent the AICH hash on every file request... even if we didn't request it (isn't that some wasting of OH?)
    else if (pAICHHash != NULL && !SupportsFileIdentifiers())
        msg.Format(L"Client %s sent file identifiers without supporting the protocol!", DbgGetClientInfo());
    else if (pAICHHash == NULL && m_fAICHHashRequested == FALSE)
        msg.Format(L"Client %s sent AICH hash without request!", DbgGetClientInfo());
    if (!msg.IsEmpty())
        throw msg;
    m_fAICHHashRequested = FALSE;
//<<< WiZaRd::Additional Security Check
//<<< WiZaRd::ClientAnalyzer
    CAICHHash ahMasterHash;
    if (pAICHHash == NULL && data != NULL)
        ahMasterHash.Read(data);
    else
        ahMasterHash = *pAICHHash;
    if (pPartFile != NULL && pPartFile == GetRequestFile())
    {
        SetReqFileAICHHash(new CAICHHash(ahMasterHash));
        pPartFile->GetAICHRecoveryHashSet()->UntrustedHashReceived(ahMasterHash, GetConnectIP());

        if (pPartFile->GetFileIdentifierC().HasAICHHash() && pPartFile->GetFileIdentifierC().GetAICHHash() != ahMasterHash)
        {
            // this an legacy client and he sent us a hash different from our verified one, which menas the fileidentifiers
            // are different. We handle this just like a FNF-Answer to our downloadrequest and remove the client from our sourcelist, because we
            // sure don't want to download from him
            pPartFile->m_DeadSourceList.AddDeadSource(this);
            DebugLogWarning(_T("Client answered with different AICH hash than local verified on in ProcessAICHFileHash, removing source. File %s, client %s"), pPartFile->GetFileName(), DbgGetClientInfo());
            // if that client does not have my file maybe has another different
            // we try to swap to another file ignoring no needed parts files
            switch (GetDownloadState())
            {
                case DS_REQHASHSET:
                    // for the love of eMule, don't accept a hashset from him :)
                    if (m_fHashsetRequestingMD4)
                    {
                        DebugLogWarning(_T("... also cancelled hash set request from client due to AICH mismatch"));
                        pPartFile->m_bMD4HashsetNeeded = true;
                    }
                    if (m_fHashsetRequestingAICH)
                    {
                        ASSERT(0);
                        pPartFile->SetAICHHashSetNeeded(true);
                    }
                    m_fHashsetRequestingMD4 = false;
                    m_fHashsetRequestingAICH = false;
                case DS_CONNECTED:
                case DS_ONQUEUE:
                case DS_NONEEDEDPARTS:
                case DS_DOWNLOADING:
                    DontSwapTo(pPartFile); // ZZ:DownloadManager
                    if (!SwapToAnotherFile(_T("Source says it doesn't have the file (AICH mismatch). CUpDownClient::ProcessAICHFileHash"), true, true, true, NULL, false, false))   // ZZ:DownloadManager
                    {
                        theApp.downloadqueue->RemoveSource(this);
                    }
                    return;
            }
        }
    }
    else
        AddDebugLogLine(DLP_HIGH, false, _T("ProcessAICHFileHash(): PartFile not found or Partfile differs from requested file, %s"), DbgGetClientInfo());
}

void CUpDownClient::SendHashSetRequest()
{
    if (socket && socket->IsConnected())
    {
        Packet* packet = NULL;
        if (SupportsFileIdentifiers())
        {
            if (thePrefs.GetDebugClientTCPLevel() > 0)
                DebugSend("OP__HashSetRequest2", this, reqfile->GetFileHash());
            CSafeMemFile filePacket(60);
            reqfile->GetFileIdentifier().WriteIdentifier(&filePacket);
            // 6 Request Options - RESERVED
            // 1 Request AICH HashSet
            // 1 Request MD4 HashSet
            uint8 byOptions = 0;
            if (reqfile->m_bMD4HashsetNeeded)
            {
                m_fHashsetRequestingMD4 = 1;
                byOptions |= 0x01;
                reqfile->m_bMD4HashsetNeeded = false;
            }
            if (reqfile->IsAICHPartHashSetNeeded() && GetReqFileAICHHash() != NULL && *GetReqFileAICHHash() == reqfile->GetFileIdentifier().GetAICHHash())
            {
                m_fHashsetRequestingAICH = 1;
                byOptions |= 0x02;
                reqfile->SetAICHHashSetNeeded(false);
            }
            if (byOptions == 0)
            {
                ASSERT(0);
                return;
            }
            DEBUG_ONLY(DebugLog(_T("Sending HashSet Request: MD4 %s, AICH %s to client %s"), m_fHashsetRequestingMD4 ? GetResString(IDS_YES) : GetResString(IDS_NO)
                                , m_fHashsetRequestingAICH ? GetResString(IDS_YES) : GetResString(IDS_NO), DbgGetClientInfo()));
            filePacket.WriteUInt8(byOptions);
            packet = new Packet(&filePacket, OP_EMULEPROT, OP_HASHSETREQUEST2);
        }
        else
        {
            if (thePrefs.GetDebugClientTCPLevel() > 0)
                DebugSend("OP__HashSetRequest", this, reqfile->GetFileHash());
            packet = new Packet(OP_HASHSETREQUEST,16);
            md4cpy(packet->pBuffer,reqfile->GetFileHash());
            m_fHashsetRequestingMD4 = 1;
            reqfile->m_bMD4HashsetNeeded = false;
        }
        theStats.AddUpDataOverheadFileRequest(packet->size);
        SendPacket(packet, true);
        SetDownloadState(DS_REQHASHSET);
    }
    else
        ASSERT(0);
}

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
/*
bool	CUpDownClient::IsPartAvailable(UINT iPart) const
{
	return (iPart >= m_nPartCount || !m_abyPartStatus) ? false : m_abyPartStatus[iPart] != 0;
}
uint8*	CUpDownClient::GetPartStatus() const
{
	return m_abyPartStatus;
}
UINT	CUpDownClient::GetPartCount() const
{
	return m_nPartCount;
}
*/
bool	CUpDownClient::IsPartAvailable(UINT iPart) const
{
    return m_bCompleteSource || ((m_pPartStatus && iPart < m_pPartStatus->GetPartCount()) ? m_pPartStatus->IsCompletePart(iPart) : false);
}

const CPartStatus*	CUpDownClient::GetPartStatus() const
{
    return m_pPartStatus;
}

UINT	CUpDownClient::GetPartCount() const
{
    return (m_pPartStatus != NULL ? m_pPartStatus->GetPartCount() : 0);
}
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

//>>> WiZaRd::ICS [enkeyDEV]
void CUpDownClient::ProcessFileIncStatus(CSafeMemFile* data, const UINT size, const bool readHash)
{
    delete[] m_abyIncPartStatus;
    m_abyIncPartStatus = NULL;

    UNREFERENCED_PARAMETER(size);
    if (GetIncompletePartVersion() == 0)
        throw CString(L"Client sent ICS data without supporting it!");
    if (readHash)
    {
        if (reqfile == NULL)
            throw GetResString(IDS_ERR_WRONGFILEID) + L" (ProcessFileIncStatus; reqfile==NULL)";
        uchar cfilehash[16];
        data->ReadHash16(cfilehash);
        if (md4cmp(cfilehash, reqfile->GetFileHash()))
            throw GetResString(IDS_ERR_WRONGFILEID) + L" (ProcessFileIncStatus; reqfile!=file)";
    }

    uint16 nED2KPartCount = data->ReadUInt16();
    if (!nED2KPartCount)
    {
        //no need to allocate! no info available!
//		m_nPartCount = reqfile->GetPartCount();
//		m_abyIncPartStatus = new uint8[m_nPartCount];
//		memset(m_abyIncPartStatus, 0, m_nPartCount);
        throw CString(L"Client sent ICS data for a complete file?!");
    }
    else
    {
        if (reqfile->GetED2KPartCount() != nED2KPartCount)
        {
            if (thePrefs.GetVerbose())
            {
                DebugLogWarning(L"FileName: \"%s\" (\"%s\")", reqfile->GetFileName(), m_strClientFilename);
                DebugLogWarning(L"FileStatus: %s", DbgGetFileStatus(nED2KPartCount, data));
            }

            CString strError = L"";
            strError.Format(L"ProcessFileIncStatus - wrong part number recv=%u  expected=%u  %s", nED2KPartCount, reqfile->GetED2KPartCount(), DbgGetFileInfo(reqfile->GetFileHash()));
//			m_nPartCount = 0;  //do not reset... in case of malformed packet we would loose all data
            throw strError;
        }
        const UINT m_nPartCount = GetPartCount(); //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        ASSERT(m_nPartCount == reqfile->GetPartCount()); //do not reset... in case of malformed packet we would loose all data
//		m_nPartCount = reqfile->GetPartCount();

        m_abyIncPartStatus = new uint8[m_nPartCount];
        memset(m_abyIncPartStatus, 0, m_nPartCount);
        uint16 done = 0;
        while (done != m_nPartCount)
        {
            const uint8 toread = data->ReadUInt8();
            for (uint8 i = 0; i != 8; ++i)
            {
                m_abyIncPartStatus[done] = ((toread>>i)&1)? 1:0;
                ++done;
                if (done == m_nPartCount)
                    break;
            }
        }
    }

    reqfile->NewSrcIncPartsInfo();
}

bool CUpDownClient::IsIncPartAvailable(const UINT iPart) const
{
//>>> WiZaRd::Optimization
    if (IsCompleteSource())
        return false; //if he has the file complete then NO part can be incomplete...
//<<< WiZaRd::Optimization
    const UINT m_nPartCount = GetPartCount(); //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    return	(iPart >= m_nPartCount || m_abyIncPartStatus == NULL) ? 0 : m_abyIncPartStatus[iPart] == 1;
}
//<<< WiZaRd::ICS [enkeyDEV]
//>>> WiZaRd::Endgame Improvement
UINT CUpDownClient::GetAvgDownSpeed() const
{
    UINT ret = GetDownloadDatarate();

    const UINT data = GetSessionDown();
    const DWORD dwTime = GetDownTime();
    if (dwTime != 0 && data > EMBLOCKSIZE)
        ret = data / dwTime;

    return ret;
}

UINT	CUpDownClient::GetDownTime() const
{
    if (m_dwDownStartTime == 0)
        return 0;
    return ::GetTickCount() - m_dwDownStartTime;
}
//<<< WiZaRd::Endgame Improvement
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
void CUpDownClient::ProcessCrumbComplete(CSafeMemFile* data)
{
    uchar		abyHash[16];
    data->ReadHash16(abyHash);
    UINT crumbIndex = data->ReadUInt32();
    CPartFile* pPartFile = theApp.downloadqueue->GetFileByID(abyHash);

    if (pPartFile && pPartFile == reqfile && m_pPartStatus != NULL)
    {
        m_pPartStatus->SetCrumb(crumbIndex);

        UpdateDisplayedInfo(false);
        reqfile->UpdateAvailablePartsCount();

        // TODO: Test and check if we exited NNP state
    }
}
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
