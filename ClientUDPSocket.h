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
#include "UploadBandwidthThrottler.h" // ZZ:UploadBandWithThrottler (UDP)
#include "EncryptedDatagramSocket.h"
#include <map> //>>> WiZaRd::NatTraversal [Xanatos]

class Packet;

#pragma pack(1)
struct UDPPack
{
    Packet* packet;
    _CIPAddress dwIP;
    uint16	nPort;
    UINT	dwTime;
    bool	bEncrypt;
    bool	bKad;
    UINT  nReceiverVerifyKey;
    uchar	pachTargetClientHashORKadID[16];
    //uint16 nPriority; We could add a priority system here to force some packets.
};
#pragma pack()

class CClientUDPSocket : public CAsyncSocket, public CEncryptedDatagramSocket, public ThrottledControlSocket // ZZ:UploadBandWithThrottler (UDP)
{
public:
    CClientUDPSocket();
    virtual ~CClientUDPSocket();

    bool	Create();
    bool	Rebind();
    uint16	GetConnectedPort()
    {
        return m_port;
    }
    bool	SendPacket(Packet* packet, const _CIPAddress& dwIP, uint16 nPort, bool bEncrypt, const uchar* pachTargetClientHashORKadID, bool bKad, UINT nReceiverVerifyKey);
    SocketSentBytes  SendControlData(UINT maxNumberOfBytesToSend, UINT minFragSize); // ZZ:UploadBandWithThrottler (UDP)

protected:
    //bool	ProcessPacket(const BYTE* packet, UINT size, uint8 opcode, const _CIPAddress& ip, uint16 port); //>>> WiZaRd::NatTraversal [Xanatos]
    bool	ProcessModPacket(BYTE* packet, const UINT size, const uint8 opcode, const _CIPAddress& ip, const uint16 port); //>>> WiZaRd::ModProt

    virtual void	OnSend(int nErrorCode);
    virtual void	OnReceive(int nErrorCode);

private:
    int		SendTo(char* lpBuf, int nBufLen, const _CIPAddress& dwIP, uint16 nPort);
    bool	IsBusy() const
    {
        return m_bWouldBlock;
    }
    bool	m_bWouldBlock;
    uint16	m_port;

    CTypedPtrList<CPtrList, UDPPack*> controlpacket_queue;

    CCriticalSection sendLocker; // ZZ:UploadBandWithThrottler (UDP)

//>>> WiZaRd::NatTraversal [Xanatos]
public:
    bool	ProcessPacket(const BYTE* packet, UINT size, uint8 opcode, const _CIPAddress& ip, uint16 port);
    void	SetConnectionEncryption(const _CIPAddress& dwIP, uint16 nPort, bool bEncrypt, const uchar* pTargetClientHash = NULL);
    byte*	GetHashForEncryption(const _CIPAddress& dwIP, uint16 nPort);
    bool	IsObfuscating(const _CIPAddress& dwIP, uint16 nPort)
    {
        return GetHashForEncryption(dwIP, nPort) != NULL;
    }
    void	SendUtpPacket(const byte *data, size_t len, const struct sockaddr *to, socklen_t tolen);

private:
    struct SIpPort
    {
        _CIPAddress IP;
        uint16 nPort;

        bool operator< (const SIpPort &Other) const
        {
            if (IP.Type() != Other.IP.Type())
                return IP.Type() < Other.IP.Type();
            if (IP.Type() == _CIPAddress::IPv6)
            {
                if (int cmp = memcmp(IP.Data(), Other.IP.Data(), 16))
                    return cmp < 0;
            }
            else if (IP.Type() == _CIPAddress::IPv4)
            {
                UINT r = IP.ToIPv4();
                UINT l = Other.IP.ToIPv4();
                if (r != l)
                    return r < l;
            }
            else
            {
                ASSERT(0);
                return false;
            }
            return nPort < Other.nPort;
        }
    };

    struct SHash
    {
        byte	UserHash[16];
        UINT	LastUsed;
    };

    std::map<SIpPort, SHash>		m_HashMap;
//<<< WiZaRd::NatTraversal [Xanatos]
};
