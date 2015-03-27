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
#ifdef NAT_TRAVERSAL
#include <map> //>>> WiZaRd::NatTraversal [Xanatos]
#endif

class Packet;

#pragma pack(1)
struct UDPPack
{
    Packet* packet;
#ifdef IPV6_SUPPORT
    CAddress dwIP; //>>> WiZaRd::IPv6 [Xanatos]
#else
    UINT	dwIP;
#endif
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
#ifdef IPV6_SUPPORT
    bool	SendPacket(Packet* packet, const CAddress& dwIP, const uint16 nPort, const bool bEncrypt, const uchar* pachTargetClientHashORKadID, const bool bKad, const UINT nReceiverVerifyKey); //>>> WiZaRd::IPv6 [Xanatos]
#else
    bool	SendPacket(Packet* packet, const UINT dwIP, const uint16 nPort, const bool bEncrypt, const uchar* pachTargetClientHashORKadID, const bool bKad, const UINT nReceiverVerifyKey);
#endif
    SocketSentBytes  SendControlData(UINT maxNumberOfBytesToSend, UINT minFragSize); // ZZ:UploadBandWithThrottler (UDP)

#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
  public:
    bool	ProcessPacket(const BYTE* packet, const UINT size, const uint8 opcode, const CAddress& ip, const uint16 port);
  protected:
    bool	ProcessModPacket(BYTE* packet, const UINT size, const uint8 opcode, const CAddress& ip, const uint16 port); //>>> WiZaRd::ModProt
//<<< WiZaRd::IPv6 [Xanatos]
#else
    bool	ProcessPacket(const BYTE* packet, const UINT size, uint8 opcode, const UINT ip, const uint16 port);
    bool	ProcessModPacket(BYTE* packet, const UINT size, const uint8 opcode, const UINT ip, const uint16 port); //>>> WiZaRd::ModProt
#endif

    virtual void	OnSend(int nErrorCode);
    virtual void	OnReceive(int nErrorCode);

  private:
#ifdef IPV6_SUPPORT
    int		SendTo(char* lpBuf, int nBufLen, const CAddress& dwIP, uint16 nPort); //>>> WiZaRd::IPv6 [Xanatos]
#else
    int		SendTo(char* lpBuf, int nBufLen, const UINT dwIP, uint16 nPort);
#endif
    bool	IsBusy() const
    {
        return m_bWouldBlock;
    }
    bool	m_bWouldBlock;
    uint16	m_port;

    CTypedPtrList<CPtrList, UDPPack*> controlpacket_queue;

    CCriticalSection sendLocker; // ZZ:UploadBandWithThrottler (UDP)

#ifdef NAT_TRAVERSAL
//>>> WiZaRd::NatTraversal [Xanatos]
  public:
#ifdef IPV6_SUPPORT
    void	SetConnectionEncryption(const CAddress& dwIP, const uint16 nPort, const bool bEncrypt, const uchar* pTargetClientHash = NULL);
    byte*	GetHashForEncryption(const CAddress& dwIP, const uint16 nPort);
    bool	IsObfuscating(const CAddress& dwIP, const uint16 nPort)
#else
    void	SetConnectionEncryption(const UINT dwIP, const uint16 nPort, const bool bEncrypt, const uchar* pTargetClientHash = NULL);
    byte*	GetHashForEncryption(const UINT dwIP, const uint16 nPort);
    bool	IsObfuscating(const UINT dwIP, const uint16 nPort)
#endif
    {
        return GetHashForEncryption(dwIP, nPort) != NULL;
    }
    void	SendUtpPacket(const byte *data, size_t len, const struct sockaddr *to, socklen_t tolen);

  private:
	bool	TryToCreate(const uint8 tries = 0); //>>> WiZaRd::Ensure port creation  
    struct SIpPort
    {
#ifdef IPV6_SUPPORT
        CAddress IP;
#else
        UINT	IP;
#endif
        uint16 nPort;

        bool operator< (const SIpPort &Other) const;
    };

    struct SHash
    {
        byte	UserHash[16];
        UINT	LastUsed;
    };

    std::map<SIpPort, SHash>		m_HashMap;
//<<< WiZaRd::NatTraversal [Xanatos]
#endif
};
