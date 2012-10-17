//this file is part of eMule
//Copyright (C)2002-2010 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "EncryptedStreamSocket.h"
#include "OtherFunctions.h"
#include "ThrottledSocket.h" // ZZ:UploadBandWithThrottler (UDP)

class CAsyncProxySocketLayer;
class Packet;

#define ES_DISCONNECTED		0xFF
#define ES_NOTCONNECTED		0x00
#define ES_CONNECTED		0x01

#define PACKET_HEADER_SIZE	6

struct StandardPacketQueueEntry
{
    UINT actualPayloadSize;
    Packet* packet;
};

class CEMSocket : public CEncryptedStreamSocket, public ThrottledFileSocket // ZZ:UploadBandWithThrottler (UDP)
{
    DECLARE_DYNAMIC(CEMSocket)
public:
    CEMSocket();
    virtual ~CEMSocket();

    virtual void 	SendPacket(Packet* packet, bool delpacket = true, bool controlpacket = true, UINT actualPayloadSize = 0, bool bForceImmediateSend = false);
    bool	IsConnected() const
    {
        return byConnected == ES_CONNECTED;
    }
    uint8	GetConState() const
    {
        return byConnected;
    }
    virtual bool IsRawDataMode() const
    {
        return false;
    }
    void	SetDownloadLimit(UINT limit);
    void	DisableDownloadLimit();
    BOOL	AsyncSelect(long lEvent);
    virtual bool IsBusy() const
    {
        return m_bBusy;
    }
    virtual bool HasQueues() const
    {
        return (sendbuffer || standartpacket_queue.GetCount() > 0 || controlpacket_queue.GetCount() > 0);   // not trustworthy threaded? but it's ok if we don't get the correct result now and then
    }
    virtual bool UseBigSendBuffer();

    virtual UINT GetTimeOut() const;
    virtual void SetTimeOut(UINT uTimeOut);

    virtual BOOL Connect(LPCSTR lpszHostAddress, UINT nHostPort);
    virtual BOOL Connect(SOCKADDR* pSockAddr, int iSockAddrLen);

    void InitProxySupport();
    virtual void RemoveAllLayers();
    const CString GetLastProxyError() const
    {
        return m_strLastProxyError;
    }
    bool GetProxyConnectFailed() const
    {
        return m_bProxyConnectFailed;
    }

    CString GetFullErrorMessage(DWORD dwError);

//>>> WiZaRd::ZZUL Upload [ZZ]
/*
    DWORD GetLastCalledSend() const
    {
        return lastCalledSend;
    }
*/
//<<< WiZaRd::ZZUL Upload [ZZ]
    uint64 GetSentBytesCompleteFileSinceLastCallAndReset();
    uint64 GetSentBytesPartFileSinceLastCallAndReset();
    uint64 GetSentBytesControlPacketSinceLastCallAndReset();
    uint64 GetSentPayloadSinceLastCallAndReset();
    void TruncateQueues();

    virtual SocketSentBytes SendControlData(UINT maxNumberOfBytesToSend, UINT minFragSize)
    {
        return Send(maxNumberOfBytesToSend, minFragSize, true);
    };
    virtual SocketSentBytes SendFileAndControlData(UINT maxNumberOfBytesToSend, UINT minFragSize)
    {
        return Send(maxNumberOfBytesToSend, minFragSize, false);
    };

//>>> WiZaRd::ZZUL Upload [ZZ]
    //UINT	GetNeededBytes();
//<<< WiZaRd::ZZUL Upload [ZZ]
#ifdef _DEBUG
    // Diagnostic Support
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    virtual int	OnLayerCallback(const CAsyncSocketExLayer *pLayer, int nType, int nCode, WPARAM wParam, LPARAM lParam);

    virtual void	DataReceived(const BYTE* pcData, UINT uSize);
    virtual bool	PacketReceived(Packet* packet) = 0;
    virtual void	OnError(int nErrorCode) = 0;
    virtual void	OnClose(int nErrorCode);
    virtual void	OnSend(int nErrorCode);
    virtual void	OnReceive(int nErrorCode);
    uint8	byConnected;
    UINT	m_uTimeOut;
    bool	m_bProxyConnectFailed;
    CAsyncProxySocketLayer* m_pProxyLayer;
    CString m_strLastProxyError;

private:
    virtual SocketSentBytes Send(UINT maxNumberOfBytesToSend, UINT minFragSize, bool onlyAllowedToSendControlPacket);
    void	ClearQueues();
    virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);

    UINT GetNextFragSize(UINT current, UINT minFragSize);
    bool    HasSent()
    {
        return m_hasSent;
    }

    // Download (pseudo) rate control
    UINT	downloadLimit;
    bool	downloadLimitEnable;
    bool	pendingOnReceive;

    // Download partial header
    char	pendingHeader[PACKET_HEADER_SIZE];	// actually, this holds only 'PACKET_HEADER_SIZE-1' bytes.
    UINT	pendingHeaderSize;

    // Download partial packet
    Packet* pendingPacket;
    UINT	pendingPacketSize;

    // Upload control
    char*	sendbuffer;
    UINT	sendblen;
    UINT	sent;

    CTypedPtrList<CPtrList, Packet*> controlpacket_queue;
    CList<StandardPacketQueueEntry> standartpacket_queue;
    bool m_currentPacket_is_controlpacket;
    CCriticalSection sendLocker;
    uint64 m_numberOfSentBytesCompleteFile;
    uint64 m_numberOfSentBytesPartFile;
    uint64 m_numberOfSentBytesControlPacket;
    bool m_currentPackageIsFromPartFile;
    bool m_bAccelerateUpload;
    DWORD lastCalledSend;
    DWORD lastSent;
    UINT lastFinishedStandard;
    UINT m_actualPayloadSize;
    UINT m_actualPayloadSizeSent;
    bool m_bBusy;
    bool m_hasSent;
    bool m_bUsesBigSendBuffers;
//>>> WiZaRd::Count block/success send [Xman?]
private:
	CList<float> m_blockhistory;

	float	avg_block_ratio;		//the average block of last 20 seconds
	float	sum_blockhistory;		//the sum of all stored ratio samples
	UINT	blockedsendcount;
	UINT	sendcount;
	UINT	blockedsendcount_overall;
	UINT	sendcount_overall;
public:
	virtual	float	GetOverallBlockingRatio() const;
	virtual	float	GetBlockingRatio() const;
	virtual	float	GetAndStepBlockRatio();
//<<< WiZaRd::Count block/success send [Xman?]
//>>> WiZaRd::ZZUL Upload [ZZ]
private:
	UINT	GetNeededBytes(const char* sendbuffer, const UINT sendblen, const UINT sent, const bool currentPacket_is_controlpacket, const DWORD lastCalledSend);
	UINT	m_actualPayloadSizeSentForThisPacket;
	bool	m_bConnectionIsReadyForSend;
	bool	useLargeBuffer;
	bool	bufferExpanded;
	CCriticalSection statsLocker;	
//<<< WiZaRd::ZZUL Upload [ZZ]
};
