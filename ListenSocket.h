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
#include "EMSocket.h"

class CUpDownClient;
class CPacket;
class CTimerWnd;
enum EDebugLogPriority;

enum SocketState
{
    SS_Other,		//These are sockets we created that may or may not be used.. Or incoming connections.
    SS_Half,		//These are sockets that we called ->connect(..) and waiting for some kind of response.
    SS_Complete	//These are sockets that have responded with either a connection or error.
};

class CClientReqSocket : public CEMSocket
{
    friend class CListenSocket;
    DECLARE_DYNCREATE(CClientReqSocket)

  public:
    CClientReqSocket(CUpDownClient* in_client = NULL);

    void	SetClient(CUpDownClient* pClient);
    void	Disconnect(LPCTSTR pszReason);
    void	WaitForOnConnect();
    void	ResetTimeOutTimer();
    bool	CheckTimeOut();
    virtual UINT GetTimeOut();
    virtual void Safe_Delete();

    bool	Create();
    virtual void SendPacket(Packet* packet, bool delpacket = true, bool controlpacket = true, UINT actualPayloadSize = 0, bool bForceImmediateSend = false);
    virtual SocketSentBytes SendControlData(UINT maxNumberOfBytesToSend, UINT overchargeMaxBytesToSend);
    virtual SocketSentBytes SendFileAndControlData(UINT maxNumberOfBytesToSend, UINT overchargeMaxBytesToSend);

    void	DbgAppendClientInfo(CString& str);
    CString DbgGetClientInfo();

    CUpDownClient*	client;
    void		 OnReceive(int nErrorCode);
  protected:
    virtual ~CClientReqSocket();
    virtual void Close()
    {
        CAsyncSocketEx::Close();
    }
    void	Delete_Timed();

    virtual void OnConnect(int nErrorCode);
    void		 OnClose(int nErrorCode);
    void		 OnSend(int nErrorCode);

    void		 OnError(int nErrorCode);

    virtual bool PacketReceived(Packet* packet);
    int			 PacketReceivedSEH(Packet* packet);
    bool		 PacketReceivedCppEH(Packet* packet);

    bool	ProcessPacket(const BYTE* packet, UINT size,UINT opcode);
    bool	ProcessExtPacket(const BYTE* packet, UINT size, UINT opcode, UINT uRawSize);
    bool	ProcessModPacket(const BYTE* packet, const UINT size, const UINT opcode, const UINT uRawSize); //>>> WiZaRd::ModProt
    void	PacketToDebugLogLine(LPCTSTR protocol, const uchar* packet, UINT size, UINT opcode);
    void	SetConState(SocketState val);

    UINT	timeout_timer;
    bool	deletethis;
    UINT	deltimer;
    bool	m_bPortTestCon;
    UINT	m_nOnConnect;

//>>> WiZaRd::ZZUL Upload [ZZ]
  public:
    bool    ExpandReceiveBuffer();
//<<< WiZaRd::ZZUL Upload [ZZ]
};


class CListenSocket : public CAsyncSocketEx
{
    friend class CClientReqSocket;

  public:
    CListenSocket();
    virtual ~CListenSocket();

    bool	StartListening();
    void	StopListening();
    virtual void OnAccept(int nErrorCode);
    void	Process();
    void	RemoveSocket(CClientReqSocket* todel);
    void	AddSocket(CClientReqSocket* toadd);
    UINT	GetOpenSockets()
    {
        return socket_list.GetCount();
    }
    void	KillAllSockets();
    bool	TooManySockets(bool bIgnoreInterval = false);
    UINT	GetMaxConnectionReached()
    {
        return maxconnectionreached;
    }
    bool    IsValidSocket(CClientReqSocket* totest);
    void	AddConnection();
    void	RecalculateStats();
    void	ReStartListening();
    void	Debug_ClientDeleted(CUpDownClient* deleted);
    bool	Rebind();
    bool	SendPortTestReply(char result,bool disconnect=false);

    void	UpdateConnectionsStatus();
    float	GetMaxConperFiveModifier();
    UINT	GetPeakConnections() const
    {
        return peakconnections;
    }
    UINT	GetTotalConnectionChecks() const
    {
        return totalconnectionchecks;
    }
    float	GetAverageConnections() const
    {
        return averageconnections;
    }
    UINT	GetActiveConnections() const
    {
        return activeconnections;
    }
    uint16	GetConnectedPort() const
    {
        return m_port;
    }
    UINT	GetTotalHalfCon() const
    {
        return m_nHalfOpen;
    }
    UINT	GetTotalComp() const
    {
        return m_nComp;
    }

  private:
	bool	TryToCreate(const uint8 tries = 0); //>>> WiZaRd::Ensure port creation
    bool bListening;
    CTypedPtrList<CPtrList, CClientReqSocket*> socket_list;
    uint16	m_OpenSocketsInterval;
    UINT	maxconnectionreached;
    uint16	m_ConnectionStates[3];
    int		m_nPendingConnections;
    UINT	peakconnections;
    UINT	totalconnectionchecks;
    float	averageconnections;
    UINT	activeconnections;
    uint16  m_port;
    UINT	m_nHalfOpen;
    UINT	m_nComp;
};
