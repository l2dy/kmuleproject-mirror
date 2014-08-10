// ZZ:UploadBandWithThrottler (UDP) -->

#pragma once

struct SocketSentBytes
{
  public:
    SocketSentBytes(const bool success, const UINT sentBytesStandardPackets, const UINT sentBytesControlPackets, const UINT errorThatOccured = 0) //>>> WiZaRd::ZZUL Upload [ZZ]
    {
        this->success = success;
        this->sentBytesStandardPackets = sentBytesStandardPackets;
        this->sentBytesControlPackets = sentBytesControlPackets;
        this->errorThatOccured = errorThatOccured; //>>> WiZaRd::ZZUL Upload [ZZ]
    }

    bool    success;
    UINT	sentBytesStandardPackets;
    UINT	sentBytesControlPackets;
    UINT	errorThatOccured; //>>> WiZaRd::ZZUL Upload [ZZ]
};

class ThrottledControlSocket
{
  public:
    virtual SocketSentBytes SendControlData(UINT /*maxNumberOfBytesToSend*/, UINT /*minFragSize*/)
    {
        AfxDebugBreak();
        return SocketSentBytes(false, 0, 0);
    };
};

class ThrottledFileSocket : public ThrottledControlSocket
{
  public:
    virtual SocketSentBytes SendFileAndControlData(UINT /*maxNumberOfBytesToSend*/, UINT /*minFragSize*/)
    {
        AfxDebugBreak();
        return SocketSentBytes(false, 0, 0);;
    }
//>>> WiZaRd::ZZUL Upload [ZZ]
//    virtual DWORD GetLastCalledSend() { AfxDebugBreak(); return 0; }
//    virtual UINT	GetNeededBytes()	{ AfxDebugBreak(); return 0; }
//<<< WiZaRd::ZZUL Upload [ZZ]
    virtual bool IsBusyExtensiveCheck()
    {
        AfxDebugBreak();
        return false;
    }
    virtual bool IsBusyQuickCheck() const
    {
        AfxDebugBreak();
        return false;
    }
    virtual bool IsEnoughFileDataQueued(UINT /*nMinFilePayloadBytes*/) const
    {
        AfxDebugBreak();
        return false;
    }
    virtual bool HasQueues(bool /*bOnlyStandardPackets*/ = false) const
    {
        AfxDebugBreak();
        return false;
    }
    virtual bool UseBigSendBuffer()								{ return false; }

//>>> WiZaRd::Count block/success send [Xman?]
    virtual float GetBlockingRatio() const
    {
        AfxDebugBreak();
        return 0;
    }
    virtual float GetOverallBlockingRatio() const
    {
        AfxDebugBreak();
        return 0;
    }
    virtual float GetAndStepBlockRatio()
    {
        AfxDebugBreak();
        return 0;
    }
//<<< WiZaRd::Count block/success send [Xman?]
};

// <-- ZZ:UploadBandWithThrottler (UDP)
