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

class CUpDownClient;
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CUploadQueue
{

public:
    CUploadQueue();
    ~CUploadQueue();

    void	Process();
    //void	AddClientToQueue(CUpDownClient* client,bool bIgnoreTimelimit = false); //>>> WiZaRd::ZZUL Upload [ZZ]
    bool	RemoveFromUploadQueue(CUpDownClient* client, LPCTSTR pszReason = NULL, bool updatewindow = true, bool earlyabort = false);
    bool	RemoveFromWaitingQueue(CUpDownClient* client,bool updatewindow = true);
    bool	IsOnUploadQueue(CUpDownClient* client)	const
    {
        return (waitinglist.Find(client) != 0);
    }
    bool	IsDownloading(CUpDownClient* client)	const
    {
        return (uploadinglist.Find(client) != 0);
    }

    void    UpdateDatarates();
    UINT	GetDatarate() const;
    UINT  GetToNetworkDatarate();

    bool	CheckForTimeOver(CUpDownClient* client);
    int		GetWaitingUserCount() const
    {
        return waitinglist.GetCount();
    }
    int		GetUploadQueueLength() const
    {
        return uploadinglist.GetCount();
    }
    UINT	GetActiveUploadsCount()	const
    {
        return m_MaxActiveClientsShortTime;
    }
    UINT	GetWaitingUserForFileCount(const CSimpleArray<CObject*>& raFiles, bool bOnlyIfChanged);
    UINT	GetDatarateForFile(const CSimpleArray<CObject*>& raFiles) const;

    POSITION GetFirstFromUploadList()
    {
        return uploadinglist.GetHeadPosition();
    }
    CUpDownClient* GetNextFromUploadList(POSITION &curpos)
    {
        return uploadinglist.GetNext(curpos);
    }
    CUpDownClient* GetQueueClientAt(POSITION &curpos)
    {
        return uploadinglist.GetAt(curpos);
    }

    POSITION GetFirstFromWaitingList()
    {
        return waitinglist.GetHeadPosition();
    }
    CUpDownClient* GetNextFromWaitingList(POSITION &curpos)
    {
        return waitinglist.GetNext(curpos);
    }
    CUpDownClient* GetWaitClientAt(POSITION &curpos)
    {
        return waitinglist.GetAt(curpos);
    }

    CUpDownClient*	GetWaitingClientByIP_UDP(UINT dwIP, uint16 nUDPPort, bool bIgnorePortOnUniqueIP, bool* pbMultipleIPs = NULL);
    CUpDownClient*	GetWaitingClientByIP(UINT dwIP);
    CUpDownClient*	GetNextClient(const CUpDownClient* update);


    void	DeleteAll();
    UINT	GetWaitingPosition(CUpDownClient* client);

    UINT	GetSuccessfullUpCount()
    {
        return successfullupcount;
    }
    UINT	GetFailedUpCount()
    {
        return failedupcount;
    }
    UINT	GetAverageUpTime();

    CUpDownClient* FindBestClientInQueue();
    void ReSortUploadSlots(bool force = false);

    CUpDownClientPtrList waitinglist;
    CUpDownClientPtrList uploadinglist;

protected:
    void		RemoveFromWaitingQueue(POSITION pos, bool updatewindow);    
    bool		AcceptNewClient(UINT curUploadSlots) const;
//>>> WiZaRd::ZZUL Upload [ZZ]
	//bool		AcceptNewClient(bool addOnNextConnect = false) const;
    //bool		ForceNewClient(bool allowEmptyWaitingQueue = false);
    //bool		AddUpNextClient(LPCTSTR pszReason, CUpDownClient* directadd = 0);
//<<< WiZaRd::ZZUL Upload [ZZ]
    void		UseHighSpeedUploadTimer(bool bEnable);

    static VOID CALLBACK UploadTimer(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime);
    static VOID CALLBACK HSUploadTimer(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime);

private:
    void	UpdateMaxClientScore();
    UINT	GetMaxClientScore()
    {
        return m_imaxscore;
    }
    void    UpdateActiveClientsInfo(DWORD curTick);

    void InsertInUploadingList(CUpDownClient* newclient);
    float GetAverageCombinedFilePrioAndCredit();


    // By BadWolf - Accurate Speed Measurement
    typedef struct TransferredData
    {
        UINT	datalen;
        DWORD	timestamp;
    };
    CList<uint64> avarage_dr_list;
    CList<uint64> avarage_friend_dr_list;
    CList<DWORD,DWORD> avarage_tick_list;
    CList<int,int> activeClients_list;
    CList<DWORD,DWORD> activeClients_tick_list;
    UINT	datarate;   //datarate sent to network (including friends)
    UINT  friendDatarate; // datarate of sent to friends (included in above total)
    // By BadWolf - Accurate Speed Measurement

    UINT_PTR h_timer;
    UINT_PTR m_hHighSpeedUploadTimer;
    UINT	successfullupcount;
    UINT	failedupcount;
    UINT	totaluploadtime;
    UINT	m_nLastStartUpload;

    UINT	m_imaxscore;

    DWORD   m_dwLastCalculatedAverageCombinedFilePrioAndCredit;
    float   m_fAverageCombinedFilePrioAndCredit;
    UINT  m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
    UINT  m_MaxActiveClients;
    UINT  m_MaxActiveClientsShortTime;

    DWORD   m_lastCalculatedDataRateTick;
    uint64  m_avarage_dr_sum;

    DWORD   m_dwLastResortedUploadSlots;
    bool	m_bStatisticsWaitingListDirty;

//>>> WiZaRd::Small File Slot
public:
    uint64 GetSmallFileSize() const;
//<<< WiZaRd::Small File Slot
//>>> WiZaRd::ZZUL Upload [ZZ]
/*
//>>> WiZaRd::Dynamic Datarate
public:
    UINT GetClientDataRateCheck() const;
    UINT GetClientDataRate() const;
//<<< WiZaRd::Dynamic Datarate
*/
//<<< WiZaRd::ZZUL Upload [ZZ]
//>>> WiZaRd::Payback First
private:
    CList<CUpDownClient*>	m_lPaybackList;
    uint8					m_uiPaybackCount;
//<<< WiZaRd::Payback First
public:
    void	StartTimer();
//>>> WiZaRd::Drop Blocking Sockets [Xman?]
private:
	CList<DWORD>	m_BlockStopList;
//<<< WiZaRd::Drop Blocking Sockets [Xman?]
//>>> WiZaRd::ZZUL Upload [ZZ]
public:
	void	AddClientToQueue(CUpDownClient* client,bool bIgnoreTimelimit = false, bool addInFirstPlace = false);
	void	ScheduleRemovalFromUploadQueue(CUpDownClient* client, LPCTSTR pszDebugReason, CString strDisplayReason, bool earlyabort = false);
	UINT	GetActiveUploadsCountLongPerspective() const;
	UINT	GetEffectiveUploadListCount() const;
	bool    RemoveOrMoveDown(CUpDownClient* client, bool onlyCheckForRemove = false);
	void	MoveDownInUploadQueue(CUpDownClient* client);
	CUpDownClient* FindBestClientInQueue(bool allowLowIdAddNextConnectToBeSet = false, CUpDownClient* lowIdClientMustBeInSameOrBetterClassAsThisClient = NULL);
	bool	RightClientIsBetter(CUpDownClient* leftClient, UINT leftScore, CUpDownClient* rightClient, UINT rightScore);
	bool	AcceptNewClient() const;
	bool	ForceNewClient(bool simulateScheduledClosingOfSlot = false);
	bool    CanForceClient(UINT curUploadSlots) const;
	bool	AddUpNextClient(LPCTSTR pszReason, CUpDownClient* directadd = 0, bool highPrioCheck = false);
private:
	UINT	GetWantedNumberOfTrickleUploads() const;
	void	CheckForHighPrioClient();

	CUpDownClient* FindLastUnScheduledForRemovalClientInUploadList() const;
	CUpDownClient* FindBestScheduledForRemovalClientInUploadListThatCanBeReinstated() const;
	DWORD   m_dwLastCheckedForHighPrioClient;
//<<< WiZaRd::ZZUL Upload [ZZ]
};
