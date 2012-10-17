/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the official client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#pragma once
#include "../routing/Maps.h"
#include "./PacketTracking.h"

class CSafeMemFile;
struct SSearchTerm;

namespace Kademlia
{
class CKadUDPKey;
class CKadClientSearcher;

struct FetchNodeID_Struct
{
    UINT dwIP;
    UINT dwTCPPort;
    UINT dwExpire;
    CKadClientSearcher* pRequester;
};

class CKademliaUDPListener : public CPacketTracking
{
    friend class CSearch;
public:
    ~CKademliaUDPListener();
    void Bootstrap(LPCTSTR uIP, uint16 uUDPPort);
    void Bootstrap(UINT uIP, uint16 uUDPPort, uint8 byKadVersion = 0, const CUInt128* uCryptTargetID = NULL);
    void FirewalledCheck(UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey, uint8 byKadVersion);
    void SendMyDetails(byte byOpcode, UINT uIP, uint16 uUDPPort, uint8 byKadVersion, CKadUDPKey targetUDPKey, const CUInt128* uCryptTargetID, bool bRequestAckPackage);
    void SendPublishSourcePacket(CContact* pContact, const CUInt128& uTargetID, const CUInt128& uContactID, const TagList& tags);
    void SendNullPacket(byte byOpcode, UINT uIP, uint16 uUDPPort, CKadUDPKey targetUDPKey, const CUInt128* uCryptTargetID);
    virtual void ProcessPacket(const byte* pbyData, UINT uLenData, UINT uIP, uint16 uUDPPort, bool bValidReceiverKey, CKadUDPKey senderUDPKey);
    void SendPacket(const byte* pbyData, UINT uLenData, UINT uDestinationHost, uint16 uDestinationPort, CKadUDPKey targetUDPKey, const CUInt128* uCryptTargetID);
    void SendPacket(const byte *pbyData, UINT uLenData, byte byOpcode, UINT uDestinationHost, uint16 uDestinationPort, CKadUDPKey targetUDPKey, const CUInt128* uCryptTargetID);
    void SendPacket(CSafeMemFile* pfileData, byte byOpcode, UINT uDestinationHost, uint16 uDestinationPort, CKadUDPKey targetUDPKey, const CUInt128* uCryptTargetID);

    bool FindNodeIDByIP(CKadClientSearcher* pRequester, UINT dwIP, uint16 nTCPPort, uint16 nUDPPort, uint8 byKadVersion);
    void ExpireClientSearch(CKadClientSearcher* pExpireImmediately = NULL);
private:
    bool AddContact_KADEMLIA2 (const byte* pbyData, UINT uLenData, UINT uIP, uint16& uUDPPort, uint8* pnOutVersion, CKadUDPKey cUDPKey, bool& rbIPVerified, bool bUpdate, bool bFromHelloReq, bool* pbOutRequestsACK, CUInt128* puOutContactID);
    void SendLegacyChallenge(UINT uIP, uint16 uUDPPort, const CUInt128& uContactID);
    static SSearchTerm* CreateSearchExpressionTree(CSafeMemFile& fileIO, int iLevel);
    static void Free(SSearchTerm* pSearchTerms);
    void Process_KADEMLIA2_BOOTSTRAP_REQ (UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_BOOTSTRAP_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey, bool bValidReceiverKey);
    void Process_KADEMLIA2_HELLO_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey, bool bValidReceiverKey);
    void Process_KADEMLIA2_HELLO_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey, bool bValidReceiverKey);
    void Process_KADEMLIA2_HELLO_RES_ACK (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, bool bValidReceiverKey);
    void Process_KADEMLIA2_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_SEARCH_KEY_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_SEARCH_SOURCE_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_SEARCH_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort);
    void Process_KADEMLIA2_SEARCH_RES (const byte* pbyPacketData, UINT uLenPacket, CKadUDPKey senderUDPKey, UINT uIP, uint16 uUDPPort);
    void Process_KADEMLIA2_PUBLISH_KEY_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_PUBLISH_SOURCE_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_PUBLISH_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP);
    void Process_KADEMLIA2_PUBLISH_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_SEARCH_NOTES_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_SEARCH_NOTES_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort);
    void Process_KADEMLIA2_PUBLISH_NOTES_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_FIREWALLED_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_FIREWALLED2_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_FIREWALLED_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_FIREWALLED_ACK_RES (UINT uLenPacket);
    void Process_KADEMLIA_FINDBUDDY_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_FINDBUDDY_RES (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA_CALLBACK_REQ (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_PING (UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_PONG (const byte* pbyPacketData, UINT uLenPacket, UINT uIP, uint16 uUDPPort, CKadUDPKey senderUDPKey);
    void Process_KADEMLIA2_FIREWALLUDP(const byte *pbyPacketData, UINT uLenPacket,UINT uIP, CKadUDPKey senderUDPKey);

    CList<FetchNodeID_Struct> listFetchNodeIDRequests;
    UINT	m_nOpenHellos;
    UINT	m_nFirewalledHellos;
};
}
