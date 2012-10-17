// ZZ:UploadBandWithThrottler (UDP) -->

#pragma once

struct SocketSentBytes
{
    bool    success;
    UINT	sentBytesStandardPackets;
    UINT	sentBytesControlPackets;
	UINT  errorThatOccured; //>>> WiZaRd::ZZUL Upload [ZZ]
};

class ThrottledControlSocket
{
public:
    virtual SocketSentBytes SendControlData(UINT maxNumberOfBytesToSend, UINT minFragSize) = 0;
};

class ThrottledFileSocket : public ThrottledControlSocket
{
public:
    virtual SocketSentBytes SendFileAndControlData(UINT maxNumberOfBytesToSend, UINT minFragSize) = 0;
//>>> WiZaRd::ZZUL Upload [ZZ]
//    virtual DWORD GetLastCalledSend() = 0;
//    virtual UINT	GetNeededBytes() = 0;
//<<< WiZaRd::ZZUL Upload [ZZ]
    virtual bool	IsBusy() const = 0;
    virtual bool    HasQueues() const = 0;
    virtual bool	UseBigSendBuffer()
    {
        return false;
    }
//>>> WiZaRd::Count block/success send [Xman?]
	virtual float GetBlockingRatio() const		{AfxDebugBreak(); return 0;}	//= 0;
	virtual float GetOverallBlockingRatio() const {AfxDebugBreak(); return 0;}	//= 0;
	virtual float GetAndStepBlockRatio()	{AfxDebugBreak(); return 0;}	//= 0;
//<<< WiZaRd::Count block/success send [Xman?]
};

// <-- ZZ:UploadBandWithThrottler (UDP)
