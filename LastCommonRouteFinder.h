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

struct CurrentPingStruct
{
    //UINT	datalen;
    CString state;
    UINT	latency;
    UINT	lowest;
    UINT  currentLimit;
};

class LastCommonRouteFinder :
    public CWinThread
{
  public:
    LastCommonRouteFinder();
    ~LastCommonRouteFinder();

    void EndThread();

    bool AddHostToCheck(UINT ip);
    bool AddHostsToCheck(CUpDownClientPtrList &list);

    //UINT GetPingedHost();
    CurrentPingStruct GetCurrentPing();
    bool AcceptNewClient();

//>>> WiZaRd::ZZUL Upload [ZZ]
    void SetPrefs(bool pEnabled, UINT pCurUpload, UINT pMinUpload, UINT pMaxUpload, bool pUseMillisecondPingTolerance, double pPingTolerance, UINT pPingToleranceMilliseconds, UINT pGoingUpDivider, UINT pGoingDownDivider, UINT pNumberOfPingsForAverage, UINT pLowestInitialPingAllowed, bool needMoreSlots);
    //void SetPrefs(bool pEnabled, UINT pCurUpload, UINT pMinUpload, UINT pMaxUpload, bool pUseMillisecondPingTolerance, double pPingTolerance, UINT pPingToleranceMilliseconds, UINT pGoingUpDivider, UINT pGoingDownDivider, UINT pNumberOfPingsForAverage, uint64 pLowestInitialPingAllowed);
//<<< WiZaRd::ZZUL Upload [ZZ]
    void InitiateFastReactionPeriod();

    UINT GetUpload();
  private:
    static UINT RunProc(LPVOID pParam);
    UINT RunInternal();

    void SetUpload(UINT newValue);
    bool AddHostToCheckNoLock(UINT ip);

    typedef CList<UINT,UINT> UInt32Clist;
    UINT Median(UInt32Clist& list);

    bool doRun;
    bool acceptNewClient;
    bool m_enabled;
    bool needMoreHosts;

    CCriticalSection addHostLocker;
    CCriticalSection prefsLocker;
    CCriticalSection uploadLocker;
    CCriticalSection pingLocker;

    CEvent* threadEndedEvent;
    CEvent* newTraceRouteHostEvent;
    CEvent* prefsEvent;

    CMap<UINT,UINT,UINT,UINT> hostsToTraceRoute;

//>>> WiZaRd::ZZUL Upload [ZZ]
    /*
        UInt32Clist pingDelays;

        uint64 pingDelaysTotal;
    */
//<<< WiZaRd::ZZUL Upload [ZZ]

    UINT minUpload;
    UINT maxUpload;
    UINT m_CurUpload;
    UINT m_upload;

    double m_pingTolerance;
    UINT m_iPingToleranceMilliseconds;
    bool m_bUseMillisecondPingTolerance;
    UINT m_goingUpDivider;
    UINT m_goingDownDivider;
    UINT m_iNumberOfPingsForAverage;

    UINT m_pingAverage;
    UINT m_lowestPing;
//>>> WiZaRd::ZZUL Upload [ZZ]
    //uint64 m_LowestInitialPingAllowed;

    UINT m_LowestInitialPingAllowed;
    bool m_NeedMoreSlots;
//<<< WiZaRd::ZZUL Upload [ZZ]

    bool m_initiateFastReactionPeriod;

    CString m_state;
};