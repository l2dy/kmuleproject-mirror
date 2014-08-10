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
#include "ClientList.h"
#include "otherfunctions.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/Kademlia/prefs.h"
#include "Kademlia/Kademlia/search.h"
#include "Kademlia/Kademlia/searchmanager.h"
#include "Kademlia/routing/contact.h"
#include "Kademlia/net/kademliaudplistener.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "kademlia/utils/UInt128.h"
#include "LastCommonRouteFinder.h"
#include "UploadQueue.h"
#include "DownloadQueue.h"
#include "UpDownClient.h"
#include "ClientCredits.h"
#include "ListenSocket.h"
#include "Opcodes.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "Log.h"
#include "packets.h"
#include "Statistics.h"
#ifdef INFO_WND
#include "./Mod/InfoWnd.h" //>>> WiZaRd::ClientAnalyzer
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//>>> WiZaRd::FiX
//helper function... use the available IP
#ifdef IPV6_SUPPORT
bool CompareIP(const CUpDownClient* client, const CAddress& dwIP) //>>> WiZaRd::IPv6 [Xanatos]
#else
bool CompareIP(const CUpDownClient* client, const UINT dwIP)
#endif
{
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
    CAddress comp = dwIP.GetType() == CAddress::IPv6 ? client->GetIPv6() : client->GetIPv4();
    if (comp.IsNull())
        comp = client->GetConnectIP();
    if (!comp.IsNull())
        return comp == dwIP;
    return false;
//<<< WiZaRd::IPv6 [Xanatos]
#else
    if (client->GetIP() != 0)
        return client->GetIP() == dwIP;
    return client->GetConnectIP() == dwIP;
#endif
}
//<<< WiZaRd::FiX

CClientList::CClientList()
{
    m_dwLastBannCleanUp = 0;
    m_dwLastTrackedCleanUp = 0;
    m_dwLastClientCleanUp = 0;
    m_nBuddyStatus = Disconnected;
    m_bannedList.InitHashTable(331);
    m_trackedClientsList.InitHashTable(2011);
    m_globDeadSourceList.Init(true);
    m_pBuddy = NULL;
}

CClientList::~CClientList()
{
    RemoveAllTrackedClients();
}

void CClientList::GetStatistics(UINT &ruTotalClients, int stats[NUM_CLIENTLIST_STATS],
                                CMap<UINT, UINT, UINT, UINT>& clientVersionEDonkey,
                                CMap<UINT, UINT, UINT, UINT>& clientVersionEDonkeyHybrid,
                                CMap<UINT, UINT, UINT, UINT>& clientVersionEMule,
                                CMap<UINT, UINT, UINT, UINT>& clientVersionKMule,
                                CMap<UINT, UINT, UINT, UINT>& clientVersionAMule)
{
    ruTotalClients = list.GetCount();
    memset(stats, 0, sizeof(stats[0]) * NUM_CLIENTLIST_STATS);

    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        const CUpDownClient* cur_client = list.GetNext(pos);

        if (cur_client->HasLowID())
            stats[14]++;

        switch (cur_client->GetClientSoft())
        {
            case SO_EMULE:
            case SO_OLDEMULE:
                stats[2]++;
                clientVersionEMule[cur_client->GetVersion()]++;
                break;

//>>> WiZaRd::kMule Version Ident
            case SO_KMULE:
                ++stats[20];
                ++clientVersionKMule[cur_client->GetVersion()];
                break;
//<<< WiZaRd::kMule Version Ident

            case SO_EDONKEYHYBRID :
                stats[4]++;
                clientVersionEDonkeyHybrid[cur_client->GetVersion()]++;
                break;

            case SO_AMULE:
                stats[10]++;
                clientVersionAMule[cur_client->GetVersion()]++;
                break;

            case SO_EDONKEY:
                stats[1]++;
                clientVersionEDonkey[cur_client->GetVersion()]++;
                break;

            case SO_MLDONKEY:
                stats[3]++;
                break;

            case SO_SHAREAZA:
                stats[11]++;
                break;

            // all remaining 'eMule Compatible' clients
//>>> WiZaRd::ClientAnalyer
// Spike2 - Enhanced Client Recognition - START
            case SO_HYDRANODE:
            case SO_EMULEPLUS:
            case SO_TRUSTYFILES:
// Spike2 - Enhanced Client Recognition - END
//<<< WiZaRd::ClientAnalyer
            case SO_CDONKEY:
            case SO_XMULE:
            case SO_LPHANT:
                stats[5]++;
                break;

            default:
                stats[0]++;
                break;
        }

        if (cur_client->Credits() != NULL)
        {
            switch (cur_client->Credits()->GetCurrentIdentState(cur_client->GetIP()))
            {
                case IS_IDENTIFIED:
                    stats[12]++;
                    break;
                case IS_IDFAILED:
                case IS_IDNEEDED:
                case IS_IDBADGUY:
                    stats[13]++;
                    break;
            }
        }

        if (cur_client->GetDownloadState()==DS_ERROR)
            stats[6]++; // Error

        switch (cur_client->GetUserPort())
        {
            case 4662:
                stats[8]++; // Default Port
                break;
            default:
                stats[9]++; // Other Port
        }

        // Network client stats
        if (cur_client->GetServerIP() && cur_client->GetServerPort())
        {
            stats[15]++;		// eD2K
            if (cur_client->GetKadPort())
            {
                stats[17]++;	// eD2K/Kad
                stats[16]++;	// Kad
            }
        }
        else if (cur_client->GetKadPort())
            stats[16]++;		// Kad
        else
            stats[18]++;		// Unknown

//>>> WiZaRd::Detect UDP problem clients
        if (cur_client->IsUDPDisabled())
            ++stats[19];
//<<< WiZaRd::Detect UDP problem clients
    }
}

void CClientList::AddClient(CUpDownClient* toadd, bool bSkipDupTest)
{
    // skipping the check for duplicate list entries is only to be done for optimization purposes, if the calling
    // function has ensured that this client instance is not already within the list -> there are never duplicate
    // client instances in this list.
    if (!bSkipDupTest)
    {
        if (list.Find(toadd))
            return;
    }
    theApp.emuledlg->transferwnd->GetClientList()->AddClient(toadd);
    list.AddTail(toadd);
}

// ZZ:UploadSpeedSense -->
bool CClientList::GiveClientsForTraceRoute()
{
    // this is a host that lastCommonRouteFinder can use to traceroute
    return theApp.lastCommonRouteFinder->AddHostsToCheck(list);
}
// ZZ:UploadSpeedSense <--

void CClientList::RemoveClient(CUpDownClient* toremove, LPCTSTR pszReason)
{
    POSITION pos = list.Find(toremove);
    if (pos)
    {
        theApp.uploadqueue->RemoveFromUploadQueue(toremove, L"CClientList::RemoveClient: " + CString(pszReason));
        theApp.uploadqueue->RemoveFromWaitingQueue(toremove);
        theApp.downloadqueue->RemoveSource(toremove);
        theApp.emuledlg->transferwnd->GetClientList()->RemoveClient(toremove);
        list.RemoveAt(pos);
    }
    RemoveFromKadList(toremove);
    RemoveConnectingClient(toremove);
}

void CClientList::DeleteAll()
{
    theApp.uploadqueue->DeleteAll();
    theApp.downloadqueue->DeleteAll();
    while (!list.IsEmpty())
        delete list.RemoveHead(); // recursive: this will call RemoveClient :-X
}

bool CClientList::AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender)
{
    POSITION pos1, pos2;
    CUpDownClient* tocheck = (*client);
    CUpDownClient* found_client = NULL;
    CUpDownClient* found_client2 = NULL;
    for (pos1 = list.GetHeadPosition(); (pos2 = pos1) != NULL;)
    {
        list.GetNext(pos1);
        CUpDownClient* cur_client =	list.GetAt(pos2);
        if (tocheck->Compare(cur_client,false))  //matching userhash
        {
            found_client2 = cur_client;
        }
        if (tocheck->Compare(cur_client,true)) 	 //matching IP
        {
            found_client = cur_client;
            break;
        }
    }
    if (found_client == NULL)
        found_client = found_client2;

    if (found_client != NULL)
    {
        if (tocheck == found_client)
        {
            //we found the same client instance (client may have sent more than one OP_HELLO). do not delete that client!
            return true;
        }
        if (sender)
        {
            if (found_client->socket)
            {
                if (found_client->socket->IsConnected()
                        && (found_client->GetIP() != tocheck->GetIP() || found_client->GetUserPort() != tocheck->GetUserPort()))
                {
                    // if found_client is connected and has the IS_IDENTIFIED, it's safe to say that the other one is a bad guy
                    if (found_client->Credits() && found_client->Credits()->GetCurrentIdentState(found_client->GetIP()) == IS_IDENTIFIED)
                    {
                        if (thePrefs.GetLogBannedClients())
                            AddDebugLogLine(false, _T("Clients: %s (%s), Banreason: Userhash invalid"), tocheck->GetUserName(), ipstr(tocheck->GetConnectIP()));
                        tocheck->Ban();
                        return false;
                    }

                    //IDS_CLIENTCOL Warning: Found matching client, to a currently connected client: %s (%s) and %s (%s)
                    if (thePrefs.GetLogBannedClients())
                        AddDebugLogLine(true,GetResString(IDS_CLIENTCOL), tocheck->GetUserName(), ipstr(tocheck->GetConnectIP()), found_client->GetUserName(), ipstr(found_client->GetConnectIP()));
                    return false;
                }
                found_client->socket->client = 0;
                found_client->socket->Safe_Delete();
            }
            found_client->socket = sender;
            tocheck->socket = 0;
        }
        *client = 0;
        delete tocheck;
        *client = found_client;
        return true;
    }
    return false;
}

#ifdef IPV6_SUPPORT
CUpDownClient* CClientList::FindClientByIP(const CAddress& clientip, const UINT port) const //>>> WiZaRd::IPv6 [Xanatos]
#else
CUpDownClient* CClientList::FindClientByIP(const UINT clientip, const UINT port) const
#endif
{
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client = list.GetNext(pos);
        if (CompareIP(cur_client, clientip) && cur_client->GetUserPort() == port)
            return cur_client;
    }
    return 0;
}

#ifdef IPV6_SUPPORT
CUpDownClient* CClientList::FindClientByUserHash(const uchar* clienthash, const CAddress& dwIP, const uint16 nTCPPort) const //>>> WiZaRd::IPv6 [Xanatos]
#else
CUpDownClient* CClientList::FindClientByUserHash(const uchar* clienthash, const UINT dwIP, const uint16 nTCPPort) const
#endif
{
    CUpDownClient* pFound = NULL;
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client = list.GetNext(pos);
        if (!md4cmp(cur_client->GetUserHash() ,clienthash))
        {
#ifdef IPV6_SUPPORT
            if ((dwIP.IsNull() || CompareIP(cur_client, dwIP)) && (nTCPPort == 0 || nTCPPort == cur_client->GetUserPort())) //>>> WiZaRd::IPv6 [Xanatos]
#else
            if ((dwIP == 0 || CompareIP(cur_client, dwIP)) && (nTCPPort == 0 || nTCPPort == cur_client->GetUserPort()))
#endif
                return cur_client;
            else
                pFound = pFound != NULL ? pFound : cur_client;
        }
    }
    return pFound;
}

#ifdef IPV6_SUPPORT
CUpDownClient* CClientList::FindClientByIP(const CAddress& clientip) const //>>> WiZaRd::IPv6 [Xanatos]
#else
CUpDownClient* CClientList::FindClientByIP(const UINT clientip) const
#endif
{
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client = list.GetNext(pos);
        if (CompareIP(cur_client, clientip))
            return cur_client;
    }
    return 0;
}

#ifdef IPV6_SUPPORT
CUpDownClient* CClientList::FindClientByIP_UDP(const CAddress& clientip, const UINT nUDPport) const //>>> WiZaRd::IPv6 [Xanatos]
#else
CUpDownClient* CClientList::FindClientByIP_UDP(const UINT clientip, const UINT nUDPport) const
#endif
{
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client = list.GetNext(pos);
        if (CompareIP(cur_client, clientip) && cur_client->GetUDPPort() == nUDPport)
            return cur_client;
    }
    return 0;
}

CUpDownClient* CClientList::FindClientByUserID_KadPort(UINT clientID, uint16 kadPort) const
{
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client = list.GetNext(pos);
        if (cur_client->GetUserIDHybrid() == clientID && cur_client->GetKadPort() == kadPort)
            return cur_client;
    }
    return 0;
}

#ifdef IPV6_SUPPORT
CUpDownClient* CClientList::FindClientByIP_KadPort(const CAddress& ip, const uint16 port) const //>>> WiZaRd::IPv6 [Xanatos]
#else
CUpDownClient* CClientList::FindClientByIP_KadPort(const UINT ip, const uint16 port) const
#endif
{
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client = list.GetNext(pos);
        if (CompareIP(cur_client, ip) && cur_client->GetKadPort() == port)
            return cur_client;
    }
    return 0;
}

CUpDownClient* CClientList::FindClientByServerID(UINT uServerIP, UINT uED2KUserID) const
{
    UINT uHybridUserID = ntohl(uED2KUserID);
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client =	list.GetNext(pos);
        if (cur_client->GetServerIP() == uServerIP && cur_client->GetUserIDHybrid() == uHybridUserID)
            return cur_client;
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Banned clients

void CClientList::AddBannedClient(UINT dwIP)
{
    m_bannedList.SetAt(dwIP, ::GetTickCount());
}

bool CClientList::IsBannedClient(UINT dwIP) const
{
    UINT dwBantime;
    if (m_bannedList.Lookup(dwIP, dwBantime))
    {
        if (dwBantime + CLIENTBANTIME > ::GetTickCount())
            return true;
    }
    return false;
}

void CClientList::RemoveBannedClient(UINT dwIP)
{
    m_bannedList.RemoveKey(dwIP);
}

void CClientList::RemoveAllBannedClients()
{
    m_bannedList.RemoveAll();
}


///////////////////////////////////////////////////////////////////////////////
// Tracked clients

void CClientList::AddTrackClient(CUpDownClient* toadd)
{
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
    if (toadd->GetIP().GetType() != CAddress::IPv4) // IPv6-TODO: Add IPv6 tracking
        return;

    UINT dwIP = _ntohl(toadd->GetIP().ToIPv4());
//<<< WiZaRd::IPv6 [Xanatos]
#else
    UINT dwIP = toadd->GetIP();
#endif
    CDeletedClient* pResult = 0;
    if (m_trackedClientsList.Lookup(dwIP, pResult))
    {
        pResult->m_dwInserted = ::GetTickCount();
        for (int i = 0; i != pResult->m_ItemsList.GetCount(); i++)
        {
            if (pResult->m_ItemsList[i].nPort == toadd->GetUserPort())
            {
                // already tracked, update
                pResult->m_ItemsList[i].pHash = toadd->Credits();
                return;
            }
        }
        PORTANDHASH porthash = { toadd->GetUserPort(), toadd->Credits()};
        pResult->m_ItemsList.Add(porthash);
    }
    else
        m_trackedClientsList.SetAt(dwIP, new CDeletedClient(toadd));
}

// true = everything ok, hash didn't changed
// false = hash changed
bool CClientList::ComparePriorUserhash(UINT dwIP, uint16 nPort, void* pNewHash)
{
    CDeletedClient* pResult = 0;
    if (m_trackedClientsList.Lookup(dwIP, pResult))
    {
        for (int i = 0; i != pResult->m_ItemsList.GetCount(); i++)
        {
            if (pResult->m_ItemsList[i].nPort == nPort)
            {
                if (pResult->m_ItemsList[i].pHash != pNewHash)
                    return false;
                else
                    break;
            }
        }
    }
    return true;
}

UINT CClientList::GetClientsFromIP(UINT dwIP) const
{
    CDeletedClient* pResult;
    if (m_trackedClientsList.Lookup(dwIP, pResult))
        return pResult->m_ItemsList.GetCount();
    return 0;
}

void CClientList::TrackBadRequest(const CUpDownClient* upcClient, int nIncreaseCounter)
{
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
    if (upcClient->GetIP().GetType() != CAddress::IPv4) // IPv6-TODO: Add IPv6 tracking
        return;

    UINT dwIP = _ntohl(upcClient->GetIP().ToIPv4());
//<<< WiZaRd::IPv6 [Xanatos]
#else
    UINT dwIP = upcClient->GetIP();
#endif
    if (dwIP == 0)
    {
        ASSERT(0);
        return;
    }
    CDeletedClient* pResult = NULL;
    if (m_trackedClientsList.Lookup(dwIP, pResult))
    {
        pResult->m_dwInserted = ::GetTickCount();
        pResult->m_cBadRequest += nIncreaseCounter;
    }
    else
    {
        CDeletedClient* ccToAdd = new CDeletedClient(upcClient);
        ccToAdd->m_cBadRequest = nIncreaseCounter;
        m_trackedClientsList.SetAt(dwIP, ccToAdd);
    }
}

UINT CClientList::GetBadRequests(const CUpDownClient* upcClient) const
{
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
    if (upcClient->GetIP().GetType() != CAddress::IPv4) // IPv6-TODO: Add IPv6 tracking
        return 0;

    UINT dwIP = _ntohl(upcClient->GetIP().ToIPv4());
//<<< WiZaRd::IPv6 [Xanatos]
#else
    UINT dwIP = upcClient->GetIP();
#endif
    if (dwIP == 0)
    {
        ASSERT(0);
        return 0;
    }
    CDeletedClient* pResult = NULL;
    if (m_trackedClientsList.Lookup(dwIP, pResult))
        return pResult->m_cBadRequest;
    else
        return 0;
}

void CClientList::RemoveAllTrackedClients()
{
    POSITION pos = m_trackedClientsList.GetStartPosition();
    UINT nKey;
    CDeletedClient* pResult;
    while (pos != NULL)
    {
        m_trackedClientsList.GetNextAssoc(pos, nKey, pResult);
        m_trackedClientsList.RemoveKey(nKey);
        delete pResult;
    }
}

void CClientList::Process()
{
    ///////////////////////////////////////////////////////////////////////////
    // Cleanup banned client list
    //
    const UINT cur_tick = ::GetTickCount();
    if (m_dwLastBannCleanUp + BAN_CLEANUP_TIME < cur_tick)
    {
        m_dwLastBannCleanUp = cur_tick;

        POSITION pos = m_bannedList.GetStartPosition();
        UINT nKey;
        UINT dwBantime;
        while (pos != NULL)
        {
            m_bannedList.GetNextAssoc(pos, nKey, dwBantime);
            if (dwBantime + CLIENTBANTIME < cur_tick)
                RemoveBannedClient(nKey);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Cleanup tracked client list
    //
    if (m_dwLastTrackedCleanUp + TRACKED_CLEANUP_TIME < cur_tick)
    {
        m_dwLastTrackedCleanUp = cur_tick;
        if (thePrefs.GetLogBannedClients())
            AddDebugLogLine(false, _T("Cleaning up TrackedClientList, %i clients on List..."), m_trackedClientsList.GetCount());
        POSITION pos = m_trackedClientsList.GetStartPosition();
        UINT nKey;
        CDeletedClient* pResult;
        while (pos != NULL)
        {
            m_trackedClientsList.GetNextAssoc(pos, nKey, pResult);
            if (pResult->m_dwInserted + KEEPTRACK_TIME < cur_tick)
            {
                m_trackedClientsList.RemoveKey(nKey);
                delete pResult;
            }
        }
        if (thePrefs.GetLogBannedClients())
            AddDebugLogLine(false, _T("...done, %i clients left on list"), m_trackedClientsList.GetCount());
    }

    ///////////////////////////////////////////////////////////////////////////
    // Process Kad client list
    //
    //We need to try to connect to the clients in m_KadList
    //If connected, remove them from the list and send a message back to Kad so we can send a ACK.
    //If we don't connect, we need to remove the client..
    //The sockets timeout should delete this object.

    // buddy is just a flag that is used to make sure we are still connected or connecting to a buddy.
    buddyState buddy = Disconnected;

    for (POSITION pos = m_KadList.GetHeadPosition(); pos != NULL;)
    {
        POSITION posLast = pos;
        CUpDownClient* cur_client =	m_KadList.GetNext(pos);
        if (!Kademlia::CKademlia::IsRunning())
        {
            //Clear out this list if we stop running Kad.
            //Setting the Kad state to KS_NONE causes it to be removed in the switch below.
            cur_client->SetKadState(KS_NONE);
        }
        switch (cur_client->GetKadState())
        {
            case KS_QUEUED_FWCHECK:
            case KS_QUEUED_FWCHECK_UDP:
                //Another client asked us to try to connect to them to check their firewalled status.
                cur_client->TryToConnect(true, true);
                break;
            case KS_CONNECTING_FWCHECK:
                //Ignore this state as we are just waiting for results.
                break;
            case KS_FWCHECK_UDP:
            case KS_CONNECTING_FWCHECK_UDP:
                // we want a UDP firewallcheck from this client and are just waiting to get connected to send the request
                break;
            case KS_CONNECTED_FWCHECK:
                //We successfully connected to the client.
                //We now send a ack to let them know.
                if (cur_client->GetKadVersion() >= KADEMLIA_VERSION7_49a)
                {
                    // the result is now sent per TCP instead of UDP, because this will fail if our intern UDP port is unreachable.
                    // But we want the TCP testresult regardless if UDP is firewalled, the new UDP state and test takes care of the rest
                    ASSERT(cur_client->socket != NULL && cur_client->socket->IsConnected());
                    if (thePrefs.GetDebugClientTCPLevel() > 0)
                        DebugSend("OP_KAD_FWTCPCHECK_ACK", cur_client);
                    Packet* pPacket = new Packet(OP_KAD_FWTCPCHECK_ACK, 0, OP_EMULEPROT);
                    if (!cur_client->SafeConnectAndSendPacket(pPacket))
                        cur_client = NULL;
                }
                else
                {
                    if (thePrefs.GetDebugClientKadUDPLevel() > 0)
                        DebugSend("KADEMLIA_FIREWALLED_ACK_RES", cur_client->GetIP(), cur_client->GetKadPort());
#ifdef IPV6_SUPPORT
                    Kademlia::CKademlia::GetUDPListener()->SendNullPacket(KADEMLIA_FIREWALLED_ACK_RES, cur_client->GetIP().ToIPv4(), cur_client->GetKadPort(), 0, NULL); //>>> WiZaRd::IPv6 [Xanatos]
#else
                    Kademlia::CKademlia::GetUDPListener()->SendNullPacket(KADEMLIA_FIREWALLED_ACK_RES, ntohl(cur_client->GetIP()), cur_client->GetKadPort(), 0, NULL);
#endif
                }
                //We are done with this client. Set Kad status to KS_NONE and it will be removed in the next cycle.
                if (cur_client != NULL)
                    cur_client->SetKadState(KS_NONE);
                break;

            case KS_INCOMING_BUDDY:
                //A firewalled client wants us to be his buddy.
                //If we already have a buddy, we set Kad state to KS_NONE and it's removed in the next cycle.
                //If not, this client will change to KS_CONNECTED_BUDDY when it connects.
                if (m_nBuddyStatus == Connected)
                    cur_client->SetKadState(KS_NONE);
                break;

            case KS_QUEUED_BUDDY:
                //We are firewalled and want to request this client to be a buddy.
                //But first we check to make sure we are not already trying another client.
                //If we are not already trying. We try to connect to this client.
                //If we are already connected to a buddy, we set this client to KS_NONE and it's removed next cycle.
                //If we are trying to connect to a buddy, we just ignore as the one we are trying may fail and we can then try this one.
                if (m_nBuddyStatus == Disconnected)
                {
                    buddy = Connecting;
                    m_nBuddyStatus = Connecting;
                    cur_client->SetKadState(KS_CONNECTING_BUDDY);
                    cur_client->TryToConnect(true, true);
#ifdef INFO_WND
                    theApp.emuledlg->infoWnd->UpdateMyInfo(); //>>> WiZaRd::InfoWnd
#endif
                }
                else if (m_nBuddyStatus == Connected)
                    cur_client->SetKadState(KS_NONE);
                break;

            case KS_CONNECTING_BUDDY:
                //We are trying to connect to this client.
                //Although it should NOT happen, we make sure we are not already connected to a buddy.
                //If we are we set to KS_NONE and it's removed next cycle.
                //But if we are not already connected, make sure we set the flag to connecting so we know
                //things are working correctly.
                if (m_nBuddyStatus == Connected)
                    cur_client->SetKadState(KS_NONE);
                else
                {
                    ASSERT(m_nBuddyStatus == Connecting);
                    buddy = Connecting;
                }
                break;

            case KS_CONNECTED_BUDDY:
                //A potential connected buddy client wanting to me in the Kad network
                //We set our flag to connected to make sure things are still working correctly.
                buddy = Connected;

                //If m_nBuddyStatus is not connected already, we set this client as our buddy!
                if (m_nBuddyStatus != Connected)
                {
                    m_pBuddy = cur_client;
                    m_nBuddyStatus = Connected;
#ifdef INFO_WND
                    theApp.emuledlg->infoWnd->UpdateMyInfo(); //>>> WiZaRd::InfoWnd
#endif
                }
                if (m_pBuddy == cur_client && theApp.IsFirewalled() && cur_client->SendBuddyPingPong())
                {
                    if (thePrefs.GetDebugClientTCPLevel() > 0)
                        DebugSend("OP__BuddyPing", cur_client);
                    Packet* buddyPing = new Packet(OP_BUDDYPING, 0, OP_EMULEPROT);
                    theStats.AddUpDataOverheadOther(buddyPing->size);
                    VERIFY(cur_client->SendPacket(buddyPing, true, true));
                    cur_client->SetLastBuddyPingPongTime();
                }
                break;

            default:
                if (cur_client == m_pBuddy)
                {
                    buddy = Disconnected;
                    m_nBuddyStatus = Disconnected; //>>> WiZaRd::FiX?
                    m_pBuddy = NULL;
                }
                m_KadList.RemoveAt(posLast);
                break;
        }
    }

    //We either never had a buddy, or lost our buddy..
    if (buddy == Disconnected)
    {
        if (m_nBuddyStatus != Disconnected || m_pBuddy)
        {
            if (Kademlia::CKademlia::IsRunning() && theApp.IsFirewalled() && Kademlia::CUDPFirewallTester::IsFirewalledUDP(true))
            {
                //We are a lowID client and we just lost our buddy.
                //Go ahead and instantly try to find a new buddy.
                Kademlia::CKademlia::GetPrefs()->SetFindBuddy();
            }
            m_pBuddy = NULL;
            m_nBuddyStatus = Disconnected;
#ifdef INFO_WND
            theApp.emuledlg->infoWnd->UpdateMyInfo(); //>>> WiZaRd::InfoWnd
#endif
        }
    }

    if (Kademlia::CKademlia::IsConnected())
    {
        //we only need a buddy if direct callback is not available
        if (Kademlia::CKademlia::IsFirewalled() && Kademlia::CUDPFirewallTester::IsFirewalledUDP(true))
        {
            //TODO 0.49b: Kad buddies won'T work with RequireCrypt, so it is disabled for now but should (and will)
            //be fixed in later version
            // Update: Buddy connections itself support obfuscation properly since 0.49a (this makes it work fine if our buddy uses require crypt)
            // ,however callback requests don't support it yet so we wouldn't be able to answer callback requests with RequireCrypt, protocolchange intended for the next version
            if (m_nBuddyStatus == Disconnected && Kademlia::CKademlia::GetPrefs()->GetFindBuddy() && !thePrefs.IsClientCryptLayerRequired())
            {
                DEBUG_ONLY(DebugLog(_T("Starting Buddysearch")));
                //We are a firewalled client with no buddy. We have also waited a set time
                //to try to avoid a false firewalled status.. So lets look for a buddy..
                if (!Kademlia::CSearchManager::PrepareLookup(Kademlia::CSearch::FINDBUDDY, true, Kademlia::CUInt128(true).Xor(Kademlia::CKademlia::GetPrefs()->GetKadID())))
                {
                    //This search ID was already going. Most likely reason is that
                    //we found and lost our buddy very quickly and the last search hadn't
                    //had time to be removed yet. Go ahead and set this to happen again
                    //next time around.
                    Kademlia::CKademlia::GetPrefs()->SetFindBuddy();
                }
            }
        }
        else
        {
            if (m_pBuddy)
            {
                //Lets make sure that if we have a buddy, they are firewalled!
                //If they are also not firewalled, then someone must have fixed their firewall or stopped saturating their line..
                //We just set the state of this buddy to KS_NONE and things will be cleared up with the next cycle.
                if (!m_pBuddy->HasLowID())
                    m_pBuddy->SetKadState(KS_NONE);
            }
        }
    }
    else
    {
        if (m_pBuddy)
        {
            //We are not connected anymore. Just set this buddy to KS_NONE and things will be cleared out on next cycle.
            m_pBuddy->SetKadState(KS_NONE);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Cleanup client list
    //
    CleanUpClientList();

    ///////////////////////////////////////////////////////////////////////////
    // Process Direct Callbacks for Timeouts
    //
    ProcessConnectingClientsList();
}

#ifdef _DEBUG
void CClientList::Debug_SocketDeleted(CClientReqSocket* deleted) const
{
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client =	list.GetNext(pos);
        if (!AfxIsValidAddress(cur_client, sizeof(CUpDownClient)))
        {
            AfxDebugBreak();
        }
        if (thePrefs.m_iDbgHeap >= 2)
            ASSERT_VALID(cur_client);
        if (cur_client->socket == deleted)
        {
            AfxDebugBreak();
        }
    }
}
#endif

bool CClientList::IsValidClient(CUpDownClient* tocheck) const
{
    if (thePrefs.m_iDbgHeap >= 2)
        ASSERT_VALID(tocheck);
    return list.Find(tocheck)!=NULL;
}


///////////////////////////////////////////////////////////////////////////////
// Kad client list

bool CClientList::RequestTCP(Kademlia::CContact* contact, uint8 byConnectOptions)
{
    UINT nContactIP = ntohl(contact->GetIPAddress());
    // don't connect ourself
    if (GetLocalIP() == nContactIP && thePrefs.GetPort() == contact->GetTCPPort())
        return false;

#ifdef IPV6_SUPPORT
    CUpDownClient* pNewClient = FindClientByIP(CAddress(_ntohl(nContactIP)), contact->GetTCPPort()); //>>> WiZaRd::IPv6 [Xanatos]
#else
    CUpDownClient* pNewClient = FindClientByIP(nContactIP, contact->GetTCPPort());
#endif

    if (!pNewClient)
        pNewClient = new CUpDownClient(0, contact->GetTCPPort(), contact->GetIPAddress(), 0, 0, false);
    else if (pNewClient->GetKadState() != KS_NONE)
        return false; // already busy with this client in some way (probably buddy stuff), don't mess with it

    //Add client to the lists to be processed.
    pNewClient->SetKadPort(contact->GetUDPPort());
    pNewClient->SetKadState(KS_QUEUED_FWCHECK);
    if (contact->GetClientID() != 0)
    {
        byte ID[16];
        contact->GetClientID().ToByteArray(ID);
        pNewClient->SetUserHash(ID);
#if 0
        pNewClient->SetConnectOptions(byConnectOptions, true, false);
#endif
    }
#if 1
    pNewClient->SetConnectOptions(byConnectOptions, true, false); // WiZaRd: need to set encrypt options anyways?
#endif
    m_KadList.AddTail(pNewClient);
    //This method checks if this is a dup already.
    AddClient(pNewClient);
    return true;
}

void CClientList::RequestBuddy(Kademlia::CContact* contact, uint8 byConnectOptions)
{
    UINT nContactIP = ntohl(contact->GetIPAddress());
    // don't connect ourself
    if (GetLocalIP() == nContactIP && thePrefs.GetPort() == contact->GetTCPPort())
        return;

#ifdef IPV6_SUPPORT
    CUpDownClient* pNewClient = FindClientByIP(CAddress(_ntohl(nContactIP)), contact->GetTCPPort()); //>>> WiZaRd::IPv6 [Xanatos]
#else
    CUpDownClient* pNewClient = FindClientByIP(nContactIP, contact->GetTCPPort());
#endif
    if (!pNewClient)
        pNewClient = new CUpDownClient(0, contact->GetTCPPort(), contact->GetIPAddress(), 0, 0, false);
    else if (pNewClient->GetKadState() != KS_NONE)
        return; // already busy with this client in some way (probably fw stuff), don't mess with it
    else if (IsKadFirewallCheckIP(nContactIP))  // doing a kad firewall check with this IP, abort
    {
        DEBUG_ONLY(DebugLogWarning(_T("KAD tcp Firewallcheck / Buddy request collosion for IP %s"), ipstr(nContactIP)));
        return;
    }
    //Add client to the lists to be processed.
    pNewClient->SetKadPort(contact->GetUDPPort());
    pNewClient->SetKadState(KS_QUEUED_BUDDY);
    byte ID[16];
    contact->GetClientID().ToByteArray(ID);
    pNewClient->SetUserHash(ID);
    pNewClient->SetConnectOptions(byConnectOptions, true, false);
    AddToKadList(pNewClient);
    //This method checks if this is a dup already.
    AddClient(pNewClient);
}

bool CClientList::IncomingBuddy(Kademlia::CContact* contact, Kademlia::CUInt128* buddyID)
{
    UINT nContactIP = ntohl(contact->GetIPAddress());
    //If eMule already knows this client, abort this.. It could cause conflicts.
    //Although the odds of this happening is very small, it could still happen.
#ifdef IPV6_SUPPORT
    if (FindClientByIP(CAddress(_ntohl(nContactIP)), contact->GetTCPPort())) //>>> WiZaRd::IPv6 [Xanatos]
#else
    if (FindClientByIP(nContactIP, contact->GetTCPPort()))
#endif
        return false;
    else if (IsKadFirewallCheckIP(nContactIP))  // doing a kad firewall check with this IP, abort
    {
        DEBUG_ONLY(DebugLogWarning(_T("KAD tcp Firewallcheck / Buddy request collosion for IP %s"), ipstr(nContactIP)));
        return false;
    }
    else if (GetLocalIP() == nContactIP && thePrefs.GetPort() == contact->GetTCPPort())
        return false; // don't connect ourself

//>>> WiZaRd::FiX?
    if (buddyID == NULL)
    {
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"Received empty buddyID in %hs", __FUNCTION__);
        return false;
    }
    byte ID[16];
    buddyID->ToByteArray(ID);
    if (isnulmd4(ID))
    {
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"Received invalid buddyID in %hs", __FUNCTION__);
        return false;
    }
//<<< WiZaRd::FiX?

    //Add client to the lists to be processed.
    CUpDownClient* pNewClient = new CUpDownClient(0, contact->GetTCPPort(), contact->GetIPAddress(), 0, 0, false);
    pNewClient->SetKadPort(contact->GetUDPPort());
    pNewClient->SetKadState(KS_INCOMING_BUDDY);
    pNewClient->SetBuddyID(ID);
    contact->GetClientID().ToByteArray(ID);
    pNewClient->SetUserHash(ID); //??
    AddToKadList(pNewClient);
    AddClient(pNewClient);
    return true;
}

void CClientList::RemoveFromKadList(CUpDownClient* torem)
{
    POSITION pos = m_KadList.Find(torem);
    if (pos)
    {
        if (torem == m_pBuddy)
        {
            m_nBuddyStatus = Disconnected; //>>> WiZaRd::FiX?
            m_pBuddy = NULL;
#ifdef INFO_WND
            theApp.emuledlg->infoWnd->UpdateMyInfo(); //>>> WiZaRd::InfoWnd
#endif
        }
        m_KadList.RemoveAt(pos);
    }
}

void CClientList::AddToKadList(CUpDownClient* toadd)
{
    if (!toadd)
        return;
    POSITION pos = m_KadList.Find(toadd);
    if (pos)
    {
        return;
    }
    m_KadList.AddTail(toadd);
}

bool CClientList::DoRequestFirewallCheckUDP(const Kademlia::CContact& contact)
{
    // first make sure we don't know this IP already from somewhere
#ifdef IPV6_SUPPORT
    if (FindClientByIP(CAddress(contact.GetIPAddress())) != NULL) //>>> WiZaRd::IPv6 [Xanatos]
#else
    if (FindClientByIP(ntohl(contact.GetIPAddress())) != NULL)
#endif
        return false;
    // fine, just create the client object, set the state and wait
    // TODO: We don't know the clients usershash, this means we cannot build an obfuscated connection, which
    // again mean that the whole check won't work on "Require Obfuscation" setting, which is not a huge problem,
    // but certainly not nice. Only somewhat acceptable way to solve this is to use the KadID instead.
    CUpDownClient* pNewClient = new CUpDownClient(0, contact.GetTCPPort(), contact.GetIPAddress(), 0, 0, false);
    pNewClient->SetKadState(KS_QUEUED_FWCHECK_UDP);
    DebugLog(_T("Selected client for UDP Firewallcheck: %s"), ipstr(ntohl(contact.GetIPAddress())));
    AddToKadList(pNewClient);
    AddClient(pNewClient);
    ASSERT(!pNewClient->SupportsDirectUDPCallback());
    return true;
}

/*bool CClientList::DebugDoRequestFirewallCheckUDP(UINT ip, uint16 port){
	// first make sure we don't know this IP already from somewhere
	// fine, justcreate the client object, set the state and wait
	// TODO: We don't know the clients usershash, this means we cannot build an obfuscated connection, which
	// again mean that the whole check won't work on "Require Obfuscation" setting, which is not a huge problem,
	// but certainly not nice. Only somewhat acceptable way to solve this is to use the KadID instead.
	CUpDownClient* pNewClient = new CUpDownClient(0, port, ip, 0, 0, false );
	pNewClient->SetKadState(KS_QUEUED_FWCHECK_UDP);
	DebugLog(_T("Selected client for UDP Firewallcheck: %s"), ipstr(ntohl(ip)));
	AddToKadList(pNewClient);
	AddClient(pNewClient);
	ASSERT( !pNewClient->SupportsDirectUDPCallback() );
	return true;
}*/



void CClientList::CleanUpClientList()
{
    // we remove clients which are not needed any more by time
    // this check is also done on CUpDownClient::Disconnected, however it will not catch all
    // cases (if a client changes the state without beeing connected
    //
    // Adding this check directly to every point where any state changes would be more effective,
    // is however not compatible with the current code, because there are points where a client has
    // no state for some code lines and the code is also not prepared that a client object gets
    // invalid while working with it (aka setting a new state)
    // so this way is just the easy and safe one to go (as long as emule is basically single threaded)
    const UINT cur_tick = ::GetTickCount();
    if (m_dwLastClientCleanUp + CLIENTLIST_CLEANUP_TIME < cur_tick)
    {
        m_dwLastClientCleanUp = cur_tick;
        POSITION pos1, pos2;
        UINT cDeleted = 0;
        for (pos1 = list.GetHeadPosition(); (pos2 = pos1) != NULL;)
        {
            list.GetNext(pos1);
            CUpDownClient* pCurClient =	list.GetAt(pos2);
            if ((pCurClient->GetUploadState() == US_NONE || pCurClient->GetUploadState() == US_BANNED && !pCurClient->IsBanned())
                    && pCurClient->GetDownloadState() == DS_NONE
                    && pCurClient->GetChatState() == MS_NONE
                    && pCurClient->GetKadState() == KS_NONE
                    && pCurClient->socket == NULL)
            {
                cDeleted++;
                delete pCurClient;
            }
        }
        DEBUG_ONLY(AddDebugLogLine(false,_T("Cleaned ClientList, removed %i not used known clients"), cDeleted));
    }
}


CDeletedClient::CDeletedClient(const CUpDownClient* pClient)
{
    m_cBadRequest = 0;
    m_dwInserted = ::GetTickCount();
    PORTANDHASH porthash = { pClient->GetUserPort(), pClient->Credits()};
    m_ItemsList.Add(porthash);
}

// ZZ:DownloadManager -->
void CClientList::ProcessA4AFClients() const
{
    //if(thePrefs.GetLogA4AF()) AddDebugLogLine(false, _T(">>> Starting A4AF check"));
    POSITION pos1, pos2;
    for (pos1 = list.GetHeadPosition(); (pos2 = pos1) != NULL;)
    {
        list.GetNext(pos1);
        CUpDownClient* cur_client =	list.GetAt(pos2);

        if (cur_client->GetDownloadState() != DS_DOWNLOADING &&
                cur_client->GetDownloadState() != DS_CONNECTED &&
                (!cur_client->m_OtherRequests_list.IsEmpty() || !cur_client->m_OtherNoNeeded_list.IsEmpty()))
        {
            //AddDebugLogLine(false, _T("+++ ZZ:DownloadManager: Trying for better file for source: %s '%s'"), cur_client->GetUserName(), cur_client->reqfile->GetFileName());
            cur_client->SwapToAnotherFile(_T("Periodic A4AF check CClientList::ProcessA4AFClients()"), false, false, false, NULL, true, false);
        }
    }
    //if(thePrefs.GetLogA4AF()) AddDebugLogLine(false, _T(">>> Done with A4AF check"));
}
// <-- ZZ:DownloadManager

void CClientList::AddKadFirewallRequest(UINT dwIP)
{
    IPANDTICS add = {dwIP, ::GetTickCount()};
    listFirewallCheckRequests.AddHead(add);
    while (!listFirewallCheckRequests.IsEmpty())
    {
        if (::GetTickCount() - listFirewallCheckRequests.GetTail().dwInserted > SEC2MS(180))
            listFirewallCheckRequests.RemoveTail();
        else
            break;
    }
}

bool CClientList::IsKadFirewallCheckIP(UINT dwIP) const
{
    for (POSITION pos = listFirewallCheckRequests.GetHeadPosition(); pos != NULL; listFirewallCheckRequests.GetNext(pos))
    {
        if (listFirewallCheckRequests.GetAt(pos).dwIP == dwIP && ::GetTickCount() - listFirewallCheckRequests.GetAt(pos).dwInserted < SEC2MS(180))
            return true;
    }
    return false;
}

void CClientList::AddConnectingClient(CUpDownClient* pToAdd)
{
    for (POSITION pos = m_liConnectingClients.GetHeadPosition(); pos != NULL; m_liConnectingClients.GetNext(pos))
    {
        if (m_liConnectingClients.GetAt(pos).pClient == pToAdd)
        {
            ASSERT(0);
            return;
        }
    }
    ASSERT(pToAdd->GetConnectingState() != CCS_NONE);
    CONNECTINGCLIENT cc = {pToAdd, ::GetTickCount()};
    m_liConnectingClients.AddTail(cc);
}

void CClientList::ProcessConnectingClientsList()
{
    // we do check if any connects have timed out by now
    const UINT cur_tick = ::GetTickCount();
    POSITION pos1, pos2;
    for (pos1 = m_liConnectingClients.GetHeadPosition(); (pos2 = pos1) != NULL;)
    {
        m_liConnectingClients.GetNext(pos1);
        CONNECTINGCLIENT cc = m_liConnectingClients.GetAt(pos2);
        if (cc.dwInserted + SEC2MS(45) < cur_tick)
        {
            ASSERT(cc.pClient->GetConnectingState() != CCS_NONE);
            m_liConnectingClients.RemoveAt(pos2);
            CString dbgInfo;
            if (cc.pClient->Disconnected(_T("Connectiontry Timeout")))
                delete cc.pClient;
        }
    }
}

void CClientList::RemoveConnectingClient(CUpDownClient* pToRemove)
{
    for (POSITION pos = m_liConnectingClients.GetHeadPosition(); pos != NULL; m_liConnectingClients.GetNext(pos))
    {
        if (m_liConnectingClients.GetAt(pos).pClient == pToRemove)
        {
            m_liConnectingClients.RemoveAt(pos);
            return;
        }
    }
}

void CClientList::AddTrackCallbackRequests(UINT dwIP)
{
    IPANDTICS add = {dwIP, ::GetTickCount()};
    listDirectCallbackRequests.AddHead(add);
    while (!listDirectCallbackRequests.IsEmpty())
    {
        if (::GetTickCount() - listDirectCallbackRequests.GetTail().dwInserted > MIN2MS(3))
            listDirectCallbackRequests.RemoveTail();
        else
            break;
    }
}

bool CClientList::AllowCallbackRequest(UINT dwIP) const
{
    for (POSITION pos = listDirectCallbackRequests.GetHeadPosition(); pos != NULL; listDirectCallbackRequests.GetNext(pos))
    {
        if (listDirectCallbackRequests.GetAt(pos).dwIP == dwIP && ::GetTickCount() - listDirectCallbackRequests.GetAt(pos).dwInserted < MIN2MS(3))
            return false;
    }
    return true;
}

//>>> WiZaRd::ModIconMapper
void CClientList::UpdateModIconIndexes() const
{
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
        list.GetNext(pos)->CheckModIconIndex();
}
//<<< WiZaRd::ModIconMapper