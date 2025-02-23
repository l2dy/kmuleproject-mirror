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
#pragma warning(disable:4702) // unreachable code
#include <list>
#pragma warning(default:4702) // unreachable code

// CStatistics
#define AVG_SESSION 0
#define AVG_TOTAL 2
#define AVG_TIME 1

enum TBPSTATES
{
    STATE_DOWNLOADING = 0x01,
    STATE_ERROROUS    = 0x10
};

class CStatistics
{
  public:
    CStatistics();   // standard constructor

    void	Init();
    void	RecordRate();
    float	GetAvgDownloadRate(int averageType);
    float	GetAvgUploadRate(int averageType);

    // -khaos--+++> (2-11-03)
    UINT	GetTransferTime()
    {
        return timeTransfers + time_thisTransfer;
    }
    UINT	GetUploadTime()
    {
        return timeUploads + time_thisUpload;
    }
    UINT	GetDownloadTime()
    {
        return timeDownloads + time_thisDownload;
    }
    void	UpdateConnectionStats(float uploadrate, float downloadrate);


    ///////////////////////////////////////////////////////////////////////////
    // Down Overhead
    //
    void	CompDownDatarateOverhead();
    void	ResetDownDatarateOverhead();
    void	AddDownDataOverheadSourceExchange(UINT data)
    {
        m_nDownDataRateMSOverhead += data;
        m_nDownDataOverheadSourceExchange += data;
        m_nDownDataOverheadSourceExchangePackets++;
    }
    void	AddDownDataOverheadFileRequest(UINT data)
    {
        m_nDownDataRateMSOverhead += data;
        m_nDownDataOverheadFileRequest += data;
        m_nDownDataOverheadFileRequestPackets++;
    }
    void	AddDownDataOverheadOther(UINT data)
    {
        m_nDownDataRateMSOverhead += data;
        m_nDownDataOverheadOther += data;
        m_nDownDataOverheadOtherPackets++;
    }
    void	AddDownDataOverheadKad(UINT data)
    {
        m_nDownDataRateMSOverhead += data;
        m_nDownDataOverheadKad += data;
        m_nDownDataOverheadKadPackets++;
    }
    void	AddDownDataOverheadCrypt(UINT /*data*/)
    {
        ;
    }
    UINT	GetDownDatarateOverhead()
    {
        return m_nDownDatarateOverhead;
    }
    uint64	GetDownDataOverheadSourceExchange()
    {
        return m_nDownDataOverheadSourceExchange;
    }
    uint64	GetDownDataOverheadFileRequest()
    {
        return m_nDownDataOverheadFileRequest;
    }
    uint64	GetDownDataOverheadKad()
    {
        return m_nDownDataOverheadKad;
    }
    uint64	GetDownDataOverheadOther()
    {
        return m_nDownDataOverheadOther;
    }
    uint64	GetDownDataOverheadSourceExchangePackets()
    {
        return m_nDownDataOverheadSourceExchangePackets;
    }
    uint64	GetDownDataOverheadFileRequestPackets()
    {
        return m_nDownDataOverheadFileRequestPackets;
    }
    uint64	GetDownDataOverheadKadPackets()
    {
        return m_nDownDataOverheadKadPackets;
    }
    uint64	GetDownDataOverheadOtherPackets()
    {
        return m_nDownDataOverheadOtherPackets;
    }


    ///////////////////////////////////////////////////////////////////////////
    // Up Overhead
    //
    void	CompUpDatarateOverhead();
    void	ResetUpDatarateOverhead();
    void	AddUpDataOverheadSourceExchange(UINT data)
    {
        m_nUpDataRateMSOverhead += data;
        m_nUpDataOverheadSourceExchange += data;
        m_nUpDataOverheadSourceExchangePackets++;
    }
    void	AddUpDataOverheadFileRequest(UINT data)
    {
        m_nUpDataRateMSOverhead += data;
        m_nUpDataOverheadFileRequest += data;
        m_nUpDataOverheadFileRequestPackets++;
    }
    void	AddUpDataOverheadKad(UINT data)
    {
        m_nUpDataRateMSOverhead += data;
        m_nUpDataOverheadKad += data;
        m_nUpDataOverheadKadPackets++;
    }
    void	AddUpDataOverheadOther(UINT data)
    {
        m_nUpDataRateMSOverhead += data;
        m_nUpDataOverheadOther += data;
        m_nUpDataOverheadOtherPackets++;
    }
    void	AddUpDataOverheadCrypt(UINT /*data*/)
    {
        ;
    }

    UINT	GetUpDatarateOverhead()
    {
        return m_nUpDatarateOverhead;
    }
    uint64	GetUpDataOverheadSourceExchange()
    {
        return m_nUpDataOverheadSourceExchange;
    }
    uint64	GetUpDataOverheadFileRequest()
    {
        return m_nUpDataOverheadFileRequest;
    }
    uint64	GetUpDataOverheadKad()
    {
        return m_nUpDataOverheadKad;
    }
    uint64	GetUpDataOverheadOther()
    {
        return m_nUpDataOverheadOther;
    }
    uint64	GetUpDataOverheadSourceExchangePackets()
    {
        return m_nUpDataOverheadSourceExchangePackets;
    }
    uint64	GetUpDataOverheadFileRequestPackets()
    {
        return m_nUpDataOverheadFileRequestPackets;
    }
    uint64	GetUpDataOverheadKadPackets()
    {
        return m_nUpDataOverheadKadPackets;
    }
    uint64	GetUpDataOverheadOtherPackets()
    {
        return m_nUpDataOverheadOtherPackets;
    }

  public:
    //	Cumulative Stats
    static float	maxDown;
    static float	maxDownavg;
    static float	cumDownavg;
    static float	maxcumDownavg;
    static float	maxcumDown;
    static float	cumUpavg;
    static float	maxcumUpavg;
    static float	maxcumUp;
    static float	maxUp;
    static float	maxUpavg;
    static float	rateDown;
    static float	rateUp;
    static UINT	timeTransfers;
    static UINT	timeDownloads;
    static UINT	timeUploads;
    static UINT	start_timeTransfers;
    static UINT	start_timeDownloads;
    static UINT	start_timeUploads;
    static UINT	time_thisTransfer;
    static UINT	time_thisDownload;
    static UINT	time_thisUpload;
    static DWORD	m_dwOverallStatus;
    static float	m_fGlobalDone;
    static float	m_fGlobalSize;

    static uint64	sessionReceivedBytes;
    static uint64	sessionSentBytes;
    static uint64	sessionSentBytesToFriend;
    static DWORD	transferStarttime;
    static UINT	filteredclients;
    static DWORD	starttime;

  private:
    typedef struct TransferredData
    {
        UINT	datalen;
        DWORD	timestamp;
    };
    std::list<TransferredData> uprateHistory;
    std::list<TransferredData> downrateHistory;

    static UINT	m_nDownDatarateOverhead;
    static UINT	m_nDownDataRateMSOverhead;
    static uint64	m_nDownDataOverheadSourceExchange;
    static uint64	m_nDownDataOverheadSourceExchangePackets;
    static uint64	m_nDownDataOverheadFileRequest;
    static uint64	m_nDownDataOverheadFileRequestPackets;
    static uint64	m_nDownDataOverheadKad;
    static uint64	m_nDownDataOverheadKadPackets;
    static uint64	m_nDownDataOverheadOther;
    static uint64	m_nDownDataOverheadOtherPackets;

    static UINT	m_nUpDatarateOverhead;
    static UINT	m_nUpDataRateMSOverhead;
    static uint64	m_nUpDataOverheadSourceExchange;
    static uint64	m_nUpDataOverheadSourceExchangePackets;
    static uint64	m_nUpDataOverheadFileRequest;
    static uint64	m_nUpDataOverheadFileRequestPackets;
    static uint64	m_nUpDataOverheadKad;
    static uint64	m_nUpDataOverheadKadPackets;
    static uint64	m_nUpDataOverheadOther;
    static uint64	m_nUpDataOverheadOtherPackets;

    static UINT	m_sumavgDDRO;
    static UINT	m_sumavgUDRO;
    CList<TransferredData> m_AvarageDDRO_list;
    CList<TransferredData> m_AvarageUDRO_list;

//>>> WiZaRd::ZZUL Upload [ZZ]
  public:
    uint64  GetTotalCompletedBytes() const;
    void    IncTotalCompletedBytes(const uint64 toAdd);
    void    DecTotalCompletedBytes(const uint64 toDec);
  private:
    uint64  m_nTotalCompletedBytes;
//<<< WiZaRd::ZZUL Upload [ZZ]
};

extern CStatistics theStats;

#if !defined(_DEBUG) && !defined(_AFXDLL) && _MFC_VER==0x0710
//#define USE_MEM_STATS
#define	ALLOC_SLOTS	20
extern ULONGLONG g_aAllocStats[ALLOC_SLOTS];
#endif
