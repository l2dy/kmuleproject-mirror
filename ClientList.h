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
#include "DeadSourceList.h"

class CClientReqSocket;
class CUpDownClient;
namespace Kademlia
{
class CContact;
class CUInt128;
};
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

#define	NUM_CLIENTLIST_STATS	22 //>>> WiZaRd::Detect UDP problem clients //>>> WiZaRd::kMule Version Ident
#define BAN_CLEANUP_TIME		1200000 // 20 min

//------------CDeletedClient Class----------------------
// this class / list is a bit overkill, but currently needed to avoid any exploit possibility
// it will keep track of certain clients attributes for 2 hours, while the CUpDownClient object might be deleted already
// currently: IP, Port, UserHash
struct PORTANDHASH
{
    uint16 nPort;
    void* pHash;
};

struct IPANDTICS
{
    UINT dwIP;
    UINT dwInserted;
};
struct CONNECTINGCLIENT
{
    CUpDownClient* pClient;
    UINT dwInserted;
};


class CDeletedClient
{
public:
    CDeletedClient(const CUpDownClient* pClient);
    CArray<PORTANDHASH> m_ItemsList;
    UINT				m_dwInserted;
    UINT				m_cBadRequest;
};

enum buddyState
{
    Disconnected,
    Connecting,
    Connected
};

// ----------------------CClientList Class---------------
class CClientList
{
    friend class CClientListCtrl;

public:
    CClientList();
    ~CClientList();

    // Clients
    void	AddClient(CUpDownClient* toadd,bool bSkipDupTest = false);
    void	RemoveClient(CUpDownClient* toremove, LPCTSTR pszReason = NULL);
    void	GetStatistics(UINT& totalclient, int stats[NUM_CLIENTLIST_STATS],
                          CMap<UINT, UINT, UINT, UINT>& clientVersionEDonkey,
                          CMap<UINT, UINT, UINT, UINT>& clientVersionEDonkeyHybrid,
                          CMap<UINT, UINT, UINT, UINT>& clientVersionEMule,
						  CMap<UINT, UINT, UINT, UINT>& clientVersionKMule,
                          CMap<UINT, UINT, UINT, UINT>& clientVersionAMule);
    UINT	GetClientCount()
    {
        return list.GetCount();
    }
    void	DeleteAll();
    bool	AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender);
    CUpDownClient* FindClientByIP(UINT clientip, UINT port) const;
    CUpDownClient* FindClientByUserHash(const uchar* clienthash, UINT dwIP = 0, uint16 nTCPPort = 0) const;
    CUpDownClient* FindClientByIP(UINT clientip) const;
    CUpDownClient* FindClientByIP_UDP(UINT clientip, UINT nUDPport) const;
    CUpDownClient* FindClientByServerID(UINT uServerIP, UINT uUserID) const;
    CUpDownClient* FindClientByUserID_KadPort(UINT clientID,uint16 kadPort) const;
    CUpDownClient* FindClientByIP_KadPort(UINT ip, uint16 port) const;

    // Banned clients
    void	AddBannedClient(UINT dwIP);
    bool	IsBannedClient(UINT dwIP) const;
    void	RemoveBannedClient(UINT dwIP);
    UINT	GetBannedCount() const
    {
        return m_bannedList.GetCount();
    }
    void	RemoveAllBannedClients();

    // Tracked clients
    void	AddTrackClient(CUpDownClient* toadd);
    bool	ComparePriorUserhash(UINT dwIP, uint16 nPort, void* pNewHash);
    UINT	GetClientsFromIP(UINT dwIP) const;
    void	TrackBadRequest(const CUpDownClient* upcClient, int nIncreaseCounter);
    UINT	GetBadRequests(const CUpDownClient* upcClient) const;
    UINT	GetTrackedCount() const
    {
        return m_trackedClientsList.GetCount();
    }
    void	RemoveAllTrackedClients();

    // Kad client list, buddy handling
    bool	RequestTCP(Kademlia::CContact* contact, uint8 byConnectOptions);
    void	RequestBuddy(Kademlia::CContact* contact, uint8 byConnectOptions);
    bool	IncomingBuddy(Kademlia::CContact* contact, Kademlia::CUInt128* buddyID);
    void	RemoveFromKadList(CUpDownClient* torem);
    void	AddToKadList(CUpDownClient* toadd);
    bool	DoRequestFirewallCheckUDP(const Kademlia::CContact& contact);
    //bool	DebugDoRequestFirewallCheckUDP(UINT ip, uint16 port);
    uint8	GetBuddyStatus()
    {
        return m_nBuddyStatus;
    }
    CUpDownClient* GetBuddy()
    {
        return m_pBuddy;
    }

    void	AddKadFirewallRequest(UINT dwIP);
    bool	IsKadFirewallCheckIP(UINT dwIP) const;

    // Direct Callback List
    void	AddTrackCallbackRequests(UINT dwIP);
    bool	AllowCalbackRequest(UINT dwIP) const;

    // Connecting Clients
    void	AddConnectingClient(CUpDownClient* pToAdd);
    void	RemoveConnectingClient(CUpDownClient* pToRemove);

    void	Process();
    bool	IsValidClient(CUpDownClient* tocheck) const;
    void	Debug_SocketDeleted(CClientReqSocket* deleted) const;

    // ZZ:UploadSpeedSense -->
    bool GiveClientsForTraceRoute();
    // ZZ:UploadSpeedSense <--

    void	ProcessA4AFClients() const; // ZZ:DownloadManager
    CDeadSourceList	m_globDeadSourceList;

protected:
    void	CleanUpClientList();
    void	ProcessConnectingClientsList();

private:
    CUpDownClientPtrList list;
    CUpDownClientPtrList m_KadList;
    CMap<UINT, UINT, UINT, UINT> m_bannedList;
    CMap<UINT, UINT, CDeletedClient*, CDeletedClient*> m_trackedClientsList;
    UINT	m_dwLastBannCleanUp;
    UINT	m_dwLastTrackedCleanUp;
    UINT m_dwLastClientCleanUp;
    CUpDownClient* m_pBuddy;
    uint8 m_nBuddyStatus;
    CList<IPANDTICS> listFirewallCheckRequests;
    CList<IPANDTICS> listDirectCallbackRequests;
    CList<CONNECTINGCLIENT> m_liConnectingClients;

//>>> WiZaRd::ModIconMapper
public:
	void	UpdateModIconIndexes() const;
//<<< WiZaRd::ModIconMapper
};
