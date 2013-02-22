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
#include "stdafx.h"
#include "emule.h"
#include "Opcodes.h"
#include "LastCommonRouteFinder.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "Preferences.h"
#include "Pinger.h"
#include "emuledlg.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


LastCommonRouteFinder::LastCommonRouteFinder()
{
    minUpload = 1;
    maxUpload = _UI32_MAX;
    m_upload = _UI32_MAX;
    m_CurUpload = 1;

    m_iPingToleranceMilliseconds = 200;
    m_bUseMillisecondPingTolerance = false;
    m_iNumberOfPingsForAverage = 0;
    m_pingAverage = 0;
    m_lowestPing = 0;
//>>> WiZaRd::ZZUL Upload [ZZ]
    //m_LowestInitialPingAllowed = 20;
    //pingDelaysTotal = 0;
    m_LowestInitialPingAllowed = 10; // this is overridden by the SetPrefs(...) call
    m_NeedMoreSlots = true;
//<<< WiZaRd::ZZUL Upload [ZZ]

    m_state = _T("");

    needMoreHosts = false;

    threadEndedEvent = new CEvent(0, 1);
    newTraceRouteHostEvent = new CEvent(0, 0);
    prefsEvent = new CEvent(0, 0);

    m_initiateFastReactionPeriod = false;

    m_enabled = false;
    doRun = true;
    AfxBeginThread(RunProc, (LPVOID)this);
}

LastCommonRouteFinder::~LastCommonRouteFinder()
{
    delete threadEndedEvent;
    delete newTraceRouteHostEvent;
    delete prefsEvent;
}

bool LastCommonRouteFinder::AddHostToCheck(UINT ip)
{
    bool gotEnoughHosts = true;

    if (needMoreHosts && IsGoodIP(ip, true))
    {
        addHostLocker.Lock();

        if (needMoreHosts)
        {
            gotEnoughHosts = AddHostToCheckNoLock(ip);
        }

        addHostLocker.Unlock();
    }

    return gotEnoughHosts;
}

bool LastCommonRouteFinder::AddHostToCheckNoLock(UINT ip)
{
    if (needMoreHosts && IsGoodIP(ip, true))
    {
        //hostsToTraceRoute.AddTail(ip);
        hostsToTraceRoute.SetAt(ip, 0);

        if (hostsToTraceRoute.GetCount() >= 10)
        {
            needMoreHosts = false;

            // Signal that there's hosts to fetch.
            newTraceRouteHostEvent->SetEvent();
        }
    }

    return !needMoreHosts;
}

bool LastCommonRouteFinder::AddHostsToCheck(CUpDownClientPtrList &list)
{
    bool gotEnoughHosts = true;

    if (needMoreHosts)
    {
        addHostLocker.Lock();

        if (needMoreHosts)
        {
            POSITION pos = list.GetHeadPosition();

            if (pos)
            {
                UINT startPos = rand()/(RAND_MAX/(min(list.GetCount(), 100)));

                for (UINT skipCounter = 0; skipCounter < startPos && pos != NULL; skipCounter++)
                {
                    list.GetNext(pos);
                }

                if (!pos)
                {
                    pos = list.GetHeadPosition();
                }

                UINT tryCount = 0;
                while (needMoreHosts && tryCount < (UINT)list.GetCount())
                {
                    tryCount++;
                    CUpDownClient* client = list.GetNext(pos);
                    if (!pos)
                    {
                        pos = list.GetHeadPosition();
                    }

                    UINT ip = client->GetIP();

                    AddHostToCheckNoLock(ip);
                }
            }
        }

        gotEnoughHosts = !needMoreHosts;

        addHostLocker.Unlock();
    }

    return gotEnoughHosts;
}

CurrentPingStruct LastCommonRouteFinder::GetCurrentPing()
{
    CurrentPingStruct returnVal;

    if (m_enabled)
    {
        pingLocker.Lock();
        returnVal.state = m_state;
        returnVal.latency = m_pingAverage;
        returnVal.lowest = m_lowestPing;
        returnVal.currentLimit = m_upload;
        pingLocker.Unlock();
    }
    else
    {
        returnVal.state = _T("");
        returnVal.latency = 0;
        returnVal.lowest = 0;
        returnVal.currentLimit = 0;
    }

    return returnVal;
}

bool LastCommonRouteFinder::AcceptNewClient()
{
    return acceptNewClient || !m_enabled; // if enabled, then return acceptNewClient, otherwise return true
}

//>>> WiZaRd::ZZUL Upload [ZZ]
//void LastCommonRouteFinder::SetPrefs(bool pEnabled, UINT pCurUpload, UINT pMinUpload, UINT pMaxUpload, bool pUseMillisecondPingTolerance, double pPingTolerance, UINT pPingToleranceMilliseconds, UINT pGoingUpDivider, UINT pGoingDownDivider, UINT pNumberOfPingsForAverage, uint64 pLowestInitialPingAllowed)
void LastCommonRouteFinder::SetPrefs(bool pEnabled, UINT pCurUpload, UINT pMinUpload, UINT pMaxUpload, bool pUseMillisecondPingTolerance, double pPingTolerance, UINT pPingToleranceMilliseconds, UINT pGoingUpDivider, UINT pGoingDownDivider, UINT pNumberOfPingsForAverage, UINT pLowestInitialPingAllowed, bool needMoreSlots)
//<<< WiZaRd::ZZUL Upload [ZZ]
{
    bool sendEvent = false;

    prefsLocker.Lock();

    if (pMinUpload <= 1024)
    {
        minUpload = 1024;
    }
    else
    {
        minUpload = pMinUpload;
    }

    if (pMaxUpload != 0)
    {
        maxUpload = pMaxUpload;
        if (maxUpload < minUpload)
        {
            minUpload = maxUpload;
        }
    }
    else
    {
        maxUpload = pCurUpload+10*1024; //_UI32_MAX;
    }

    bool bStatBarResetted = false;
    if (pEnabled && m_enabled == false)
    {
        sendEvent = true;
        // this will show the area for ping info in status bar.
        theApp.emuledlg->SetStatusBarPartsSize();
        bStatBarResetted = true;
    }
    else if (pEnabled == false)
    {
        if (m_enabled)
        {
            // this will remove the area for ping info in status bar.
            theApp.emuledlg->SetStatusBarPartsSize();
            bStatBarResetted = true;
        }
        //prefsEvent->ResetEvent();
        sendEvent = true;
    }

    // this will resize the area for ping info in status bar.
    if (m_bUseMillisecondPingTolerance != pUseMillisecondPingTolerance)
    {
        if (!bStatBarResetted)
            theApp.emuledlg->SetStatusBarPartsSize();
    }

    m_enabled = pEnabled;
    m_bUseMillisecondPingTolerance = pUseMillisecondPingTolerance;
    m_pingTolerance = pPingTolerance;
    m_iPingToleranceMilliseconds = pPingToleranceMilliseconds;
    m_goingUpDivider = pGoingUpDivider;
    m_goingDownDivider = pGoingDownDivider;
    m_CurUpload = pCurUpload;
    m_iNumberOfPingsForAverage = pNumberOfPingsForAverage;
    m_LowestInitialPingAllowed = pLowestInitialPingAllowed;
    m_NeedMoreSlots = needMoreSlots; //>>> WiZaRd::ZZUL Upload [ZZ]

    uploadLocker.Lock();

    if (m_upload > maxUpload || pEnabled == false)
    {
        m_upload = maxUpload;
    }

    uploadLocker.Unlock();
    prefsLocker.Unlock();

    if (sendEvent)
    {
        prefsEvent->SetEvent();
    }
}

void LastCommonRouteFinder::InitiateFastReactionPeriod()
{
    prefsLocker.Lock();

    m_initiateFastReactionPeriod = true;

    prefsLocker.Unlock();
}

UINT LastCommonRouteFinder::GetUpload()
{
    UINT returnValue;

    uploadLocker.Lock();

    returnValue = m_upload;

    uploadLocker.Unlock();

    return returnValue;
}

void LastCommonRouteFinder::SetUpload(UINT newValue)
{
    uploadLocker.Lock();

    m_upload = newValue;

    uploadLocker.Unlock();
}

/**
 * Make the thread exit. This method will not return until the thread has stopped
 * looping.
 */
void LastCommonRouteFinder::EndThread()
{
    // signal the thread to stop looping and exit.
    doRun = false;

    prefsEvent->SetEvent();
    newTraceRouteHostEvent->SetEvent();

    // wait for the thread to signal that it has stopped looping.
    threadEndedEvent->Lock();
}

/**
 * Start the thread. Called from the constructor in this class.
 *
 * @param pParam
 *
 * @return
 */
UINT AFX_CDECL LastCommonRouteFinder::RunProc(LPVOID pParam)
{
    DbgSetThreadName("LastCommonRouteFinder");
    InitThreadLocale();
    LastCommonRouteFinder* lastCommonRouteFinder = (LastCommonRouteFinder*)pParam;

    return lastCommonRouteFinder->RunInternal();
}

/**
 * @return always returns 0.
 */
UINT LastCommonRouteFinder::RunInternal()
{
    Pinger pinger;
    bool hasSucceededAtLeastOnce = false;

    while (doRun)
    {
        // wait for updated prefs
        prefsEvent->Lock();

        bool enabled = m_enabled;

        // retry loop. enabled will be set to false in end of this loop, if to many failures (tries too large)
        while (doRun && enabled)
        {
            bool foundLastCommonHost = false;
            UINT lastCommonHost = 0;
            UINT lastCommonTTL = 0;
            UINT hostToPing = 0;
            bool useUdp = false;

            hostsToTraceRoute.RemoveAll();

//>>> WiZaRd::ZZUL Upload [ZZ]
            //pingDelays.RemoveAll();
            //pingDelaysTotal = 0;
            UInt32Clist pingDelays;
            uint64 pingDelaysTotal = 0;
//<<< WiZaRd::ZZUL Upload [ZZ]

            pingLocker.Lock();
            m_pingAverage = 0;
            m_lowestPing = 0;
            m_state = GetResString(IDS_USS_STATE_PREPARING);
            pingLocker.Unlock();

            // Calculate a good starting value for the upload control. If the user has entered a max upload value, we use that. Otherwise 10 KBytes/s
            int startUpload = (maxUpload != _UI32_MAX)?maxUpload:10*1024;

            bool atLeastOnePingSucceded = false;
            while (doRun && enabled && foundLastCommonHost == false)
            {
                UINT traceRouteTries = 0;
                while (doRun && enabled && foundLastCommonHost == false && (traceRouteTries < 5 || hasSucceededAtLeastOnce && traceRouteTries < _UI32_MAX) && (hostsToTraceRoute.GetCount() < 10 || hasSucceededAtLeastOnce))
                {
                    traceRouteTries++;

                    lastCommonHost = 0;

                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Try #%i. Collecting hosts..."), traceRouteTries);

                    addHostLocker.Lock();
                    needMoreHosts = true;
                    addHostLocker.Unlock();

                    // wait for hosts to traceroute
                    newTraceRouteHostEvent->Lock();

                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Got enough hosts. Listing the hosts that will be tracerouted:"));

                    POSITION pos = hostsToTraceRoute.GetStartPosition();
                    int counter = 0;
                    while (pos != NULL)
                    {
                        counter++;
                        UINT hostToTraceRoute, dummy;
                        hostsToTraceRoute.GetNextAssoc(pos, hostToTraceRoute, dummy);
                        IN_ADDR stDestAddr;
                        stDestAddr.s_addr = hostToTraceRoute;

                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Host #%i: %s"), counter, ipstr(stDestAddr));
                    }

                    // find the last common host, using traceroute
                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Starting traceroutes to find last common host."));

                    // for the tracerouting phase (preparing...) we need to disable uploads so we get a faster traceroute and better ping values.
                    SetUpload(2*1024);
                    Sleep(SEC2MS(1));

                    if (m_enabled == false)
                    {
                        enabled = false;
                    }

                    bool failed = false;

                    UINT curHost = 0;
//>>> WiZaRd::ZZUL Upload [ZZ]
                    for (UINT ttl = 1; doRun && enabled && (curHost != 0 && ttl <= 64 || curHost == 0 && ttl < 10) && foundLastCommonHost == false && failed == false; ttl++)
                        //for(UINT ttl = 1; doRun && enabled && (curHost != 0 && ttl <= 64 || curHost == 0 && ttl < 5) && foundLastCommonHost == false && failed == false; ttl++)
//<<< WiZaRd::ZZUL Upload [ZZ]
                    {
                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Pinging for TTL %i..."), ttl);

                        useUdp = false; // PENDING: Get default value from prefs?

                        curHost = 0;
                        if (m_enabled == false)
                        {
                            enabled = false;
                        }

                        UINT lastSuccedingPingAddress = 0;
                        UINT lastDestinationAddress = 0;
                        UINT hostsToTraceRouteCounter = 0;
                        bool failedThisTtl = false;
                        POSITION pos = hostsToTraceRoute.GetStartPosition();
//>>> WiZaRd::ZZUL Upload [ZZ]
//						while(doRun && enabled && failed == false && failedThisTtl == false && pos != NULL &&
//							( lastDestinationAddress == 0 || lastDestinationAddress == curHost)) // || pingStatus.success == false && pingStatus.error == IP_REQ_TIMED_OUT ))
                        bool unexpectedTimeout = false;
                        while (doRun && enabled && failed == false && failedThisTtl == false && pos != NULL && !unexpectedTimeout &&
                                (lastDestinationAddress == 0 || lastDestinationAddress == curHost))  // || pingStatus.success == false && pingStatus.error == IP_REQ_TIMED_OUT ))
//<<< WiZaRd::ZZUL Upload [ZZ]
                        {
                            PingStatus pingStatus = {0};

                            hostsToTraceRouteCounter++;

                            // this is the current address we send ping to, in loop below.
                            // PENDING: Don't confuse this with curHost, which is unfortunately almost
                            // the same name. Will rename one of these variables as soon as possible, to
                            // get more different names.
                            UINT curAddress, dummy;
                            hostsToTraceRoute.GetNextAssoc(pos, curAddress, dummy);

                            pingStatus.success = false;
                            for (int counter = 0; doRun && enabled && counter < 2 && (pingStatus.success == false || pingStatus.success == true && pingStatus.status != IP_SUCCESS && pingStatus.status != IP_TTL_EXPIRED_TRANSIT); counter++)
                            {
                                pingStatus = pinger.Ping(curAddress, ttl, true, useUdp);
                                if (doRun && enabled &&
                                        (
                                            pingStatus.success == false ||
                                            pingStatus.success == true &&
                                            pingStatus.status != IP_SUCCESS &&
                                            pingStatus.status != IP_TTL_EXPIRED_TRANSIT
                                        ) &&
                                        counter < 3-1)
                                {
                                    IN_ADDR stDestAddr;
                                    stDestAddr.s_addr = curAddress;
                                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Failure #%i to ping host! (TTL: %i IP: %s error: %i). Sleeping 1 sec before retry. Error info follows."), counter+1, ttl, ipstr(stDestAddr), (pingStatus.success)?pingStatus.status:pingStatus.error);
                                    pinger.PIcmpErr((pingStatus.success)?pingStatus.status:pingStatus.error);

                                    Sleep(1000);

                                    if (m_enabled == false)
                                        enabled = false;

                                    // trying other ping method
                                    useUdp = !useUdp;
                                }
                            }

                            if (pingStatus.success == true && pingStatus.status == IP_TTL_EXPIRED_TRANSIT)
                            {
                                if (curHost == 0)
                                {
//>>> WiZaRd::ZZUL Upload [ZZ]
                                    curHost = pingStatus.destinationAddress;
                                    if (hostsToTraceRouteCounter > 1)
                                    {
                                        unexpectedTimeout = true;
                                        theApp.QueueDebugLogLine(false, L"UploadSpeedSense: First successful ping at this ttl, which is unexpected. Assuming differing responder.");
                                    }
//<<< WiZaRd::ZZUL Upload [ZZ]
                                }
                                atLeastOnePingSucceded = true;
                                lastSuccedingPingAddress = curAddress;
                                lastDestinationAddress = pingStatus.destinationAddress;
                            }
                            else
                            {
                                // failed to ping this host for some reason.
                                // Or we reached the actual host we are pinging. We don't want that, since it is too close.
                                // Remove it.
                                IN_ADDR stDestAddr;
                                stDestAddr.s_addr = curAddress;
                                if (pingStatus.success == true && pingStatus.status == IP_SUCCESS)
                                {
                                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Host was too close! Removing this host. (TTL: %i IP: %s status: %i). Removing this host and restarting host collection."), ttl, ipstr(stDestAddr), pingStatus.status);

                                    hostsToTraceRoute.RemoveKey(curAddress);
                                }
                                else if (pingStatus.success == true && pingStatus.status == IP_DEST_HOST_UNREACHABLE)
                                {
                                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Host unreacheable! (TTL: %i IP: %s status: %i). Removing this host. Status info follows."), ttl, ipstr(stDestAddr), pingStatus.status);
                                    pinger.PIcmpErr(pingStatus.status);

                                    hostsToTraceRoute.RemoveKey(curAddress);
                                }
                                else if (pingStatus.success == true)
                                {
                                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Unknown ping status! (TTL: %i IP: %s status: %i). Reason follows. Changing ping method to see if it helps."), ttl, ipstr(stDestAddr), pingStatus.status);
                                    pinger.PIcmpErr(pingStatus.status);
                                    useUdp = !useUdp;
                                }
                                else
                                {
                                    if (pingStatus.error == IP_REQ_TIMED_OUT)
                                    {
                                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Timeout when pinging a host! (TTL: %i IP: %s Error: %i). Keeping host. Error info follows."), ttl, ipstr(stDestAddr), pingStatus.error);
                                        pinger.PIcmpErr(pingStatus.error);

                                        if (hostsToTraceRouteCounter > 2 && lastSuccedingPingAddress == 0)
                                        {
                                            // several pings have timed out on this ttl. Probably we can't ping on this ttl at all
                                            failedThisTtl = true;
                                        }
//>>> WiZaRd::ZZUL Upload [ZZ]
                                        else if (lastSuccedingPingAddress != 0)
                                        {
                                            unexpectedTimeout = true;
                                            theApp.QueueDebugLogLine(false, L"UploadSpeedSense: Unexpected timeout.");
                                        }
//<<< WiZaRd::ZZUL Upload [ZZ]
                                    }
                                    else
                                    {
                                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Unknown pinging error! (TTL: %i IP: %s status: %i). Reason follows. Changing ping method to see if it helps."), ttl, ipstr(stDestAddr), pingStatus.error);
                                        pinger.PIcmpErr(pingStatus.error);
                                        useUdp = !useUdp;
                                    }
                                }

                                if (hostsToTraceRoute.GetSize() <= 8)
                                {
                                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: To few hosts to traceroute left. Restarting host colletion."));
                                    failed = true;
                                }
                            }
                        }

                        if (failed == false)
                        {
                            if (curHost != 0 && lastDestinationAddress != 0)
                            {
//>>> WiZaRd::ZZUL Upload [ZZ]
                                //if(lastDestinationAddress == curHost)
                                if (lastDestinationAddress == curHost && !unexpectedTimeout)
//<<< WiZaRd::ZZUL Upload [ZZ]
                                {
                                    IN_ADDR stDestAddr;
                                    stDestAddr.s_addr = curHost;
                                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Host at TTL %i: %s"), ttl, ipstr(stDestAddr));

                                    lastCommonHost = curHost;
                                    lastCommonTTL = ttl;
                                }
                                else /*if(lastSuccedingPingAddress != 0)*/
                                {
                                    foundLastCommonHost = true;
                                    hostToPing = lastSuccedingPingAddress;

                                    CString hostToPingString = ipstr(hostToPing);

//>>> WiZaRd::ZZUL Upload [ZZ]
                                    if (unexpectedTimeout)
                                        theApp.QueueDebugLogLine(false, L"UploadSpeedSense: Unexpected timeout at TTL %i: Assuming it means responder differs. This will be the host to ping: %s", ttl, hostToPingString);
//<<< WiZaRd::ZZUL Upload [ZZ]
                                    if (lastCommonHost != 0)
                                    {
                                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Found differing host at TTL %i: %s. This will be the host to ping."), ttl, hostToPingString);
                                    }
                                    else
                                    {
                                        CString lastCommonHostString = ipstr(lastDestinationAddress);

                                        lastCommonHost = lastDestinationAddress;
                                        lastCommonTTL = ttl;
                                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Found differing host at TTL %i, but last ttl couldn't be pinged so we don't know last common host. Taking a chance and using first differing ip as last commonhost. Host to ping: %s. Faked LastCommonHost: %s"), ttl, hostToPingString, lastCommonHostString);
                                    }
                                }
                            }
                            else
                            {
                                if (ttl < 4)
                                {
                                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Could perform no ping at all at TTL %i. Trying next ttl."), ttl);
                                }
                                else
                                {
                                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Could perform no ping at all at TTL %i. Giving up."), ttl);
                                }
                                lastCommonHost = 0;
                            }
                        }
                    }

                    if (foundLastCommonHost == false && traceRouteTries >= 3)
                    {
                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Tracerouting failed several times. Waiting a few minutes before trying again."));

                        SetUpload(maxUpload);

                        pingLocker.Lock();
                        m_state = GetResString(IDS_USS_STATE_WAITING);
                        pingLocker.Unlock();

                        prefsEvent->Lock(3*60*1000);

                        pingLocker.Lock();
                        m_state = GetResString(IDS_USS_STATE_PREPARING);
                        pingLocker.Unlock();
                    }

                    if (m_enabled == false)
                    {
                        enabled = false;
                    }
                }

                if (enabled)
                {
                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Done tracerouting. Evaluating results."));

                    if (foundLastCommonHost == true)
                    {
                        IN_ADDR stLastCommonHostAddr;
                        stLastCommonHostAddr.s_addr = lastCommonHost;

                        // log result
                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Found last common host. LastCommonHost: %s @ TTL: %i"), ipstr(stLastCommonHostAddr), lastCommonTTL);

                        IN_ADDR stHostToPingAddr;
                        stHostToPingAddr.s_addr = hostToPing;
                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Found last common host. HostToPing: %s"), ipstr(stHostToPingAddr));
                    }
                    else
                    {
                        theApp.QueueDebugLogLine(false,GetResString(IDS_USS_TRACEROUTEOFTENFAILED));
                        theApp.QueueLogLine(true, GetResString(IDS_USS_TRACEROUTEOFTENFAILED));
                        enabled = false;

                        pingLocker.Lock();
                        m_state = GetResString(IDS_USS_STATE_ERROR);
                        pingLocker.Unlock();

                        // PENDING: this may not be thread safe
                        thePrefs.SetDynUpEnabled(false);
                    }
                }
            }

            if (m_enabled == false)
            {
                enabled = false;
            }

            if (doRun && enabled)
            {
                theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Finding a start value for lowest ping..."));
            }

            // PENDING:
            prefsLocker.Lock();
//>>> WiZaRd::ZZUL Upload [ZZ]
            //uint64 lowestInitialPingAllowed = m_LowestInitialPingAllowed;
            UINT lowestInitialPingAllowed = m_LowestInitialPingAllowed;
//<<< WiZaRd::ZZUL Upload [ZZ]
            prefsLocker.Unlock();

//>>> WiZaRd::ZZUL Upload [ZZ]
            //UINT initial_ping = _I32_MAX;
            UINT initial_ping = _UI32_MAX;
//<<< WiZaRd::ZZUL Upload [ZZ]

            bool foundWorkingPingMethod = false;
            // finding lowest ping
            for (int initialPingCounter = 0; doRun && enabled && initialPingCounter < 10; initialPingCounter++)
            {
                Sleep(200);

                PingStatus pingStatus = pinger.Ping(hostToPing, lastCommonTTL, true, useUdp);

                if (pingStatus.success && pingStatus.status == IP_TTL_EXPIRED_TRANSIT)
                {
                    foundWorkingPingMethod = true;

                    if (pingStatus.delay > 0 && pingStatus.delay < initial_ping)
//>>> WiZaRd::ZZUL Upload [ZZ]
                        //initial_ping = (UINT)max(pingStatus.delay, lowestInitialPingAllowed);
                        initial_ping = (UINT)pingStatus.delay;
//<<< WiZaRd::ZZUL Upload [ZZ]
                }
                else
                {
                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: %s-Ping #%i failed. Reason follows"), useUdp?_T("UDP"):_T("ICMP"), initialPingCounter);
                    pinger.PIcmpErr(pingStatus.error);

                    // trying other ping method
                    if (!pingStatus.success && !foundWorkingPingMethod)
                    {
                        useUdp = !useUdp;
                    }
                }

                if (m_enabled == false)
                    enabled = false;
            }

            // Set the upload to a good starting point
            SetUpload(startUpload);
            Sleep(SEC2MS(1));
            DWORD initTime = ::GetTickCount();

            // if all pings returned 0, initial_ping will not have been changed from default value.
            // then set initial_ping to lowestInitialPingAllowed
//>>> WiZaRd::ZZUL Upload [ZZ]
            //if(initial_ping == _I32_MAX)
            //    initial_ping = (UINT)lowestInitialPingAllowed;
            if (initial_ping == _UI32_MAX)
                initial_ping = lowestInitialPingAllowed;
            else if (initial_ping < lowestInitialPingAllowed)
                initial_ping = lowestInitialPingAllowed;
            else if (initial_ping % 10 == 0 && initial_ping < 30)
            {
                // workaround for some OS:es where realtime measurement doesn't work, and ping times are always multiples of 10
                initial_ping += 5;
                lowestInitialPingAllowed = initial_ping;
            }
//<<< WiZaRd::ZZUL Upload [ZZ]

            UINT upload = 0;

            hasSucceededAtLeastOnce = true;

            if (doRun && enabled)
            {
//>>> WiZaRd::ZZUL Upload [ZZ]
                //if(initial_ping > lowestInitialPingAllowed)
                if (initial_ping != lowestInitialPingAllowed)
//<<< WiZaRd::ZZUL Upload [ZZ]
                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Lowest ping: %i ms"), initial_ping);
                else
//>>> WiZaRd::ZZUL Upload [ZZ]
                    //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Lowest ping: %i ms. (Filtered lower values. Lowest ping is never allowed to go under %i ms)"), initial_ping, lowestInitialPingAllowed);
                    theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Lowest ping: %i ms. (Filtered lower values. Lowest ping is not allowed to go under %i ms)"), initial_ping, lowestInitialPingAllowed);
//<<< WiZaRd::ZZUL Upload [ZZ]
                prefsLocker.Lock();
                upload = m_CurUpload;

                if (upload < minUpload)
                    upload = minUpload;
                if (upload > maxUpload)
                    upload = maxUpload;
                prefsLocker.Unlock();
            }

            if (m_enabled == false)
                enabled = false;

            if (doRun && enabled)
            {
                theApp.QueueDebugLogLine(false, GetResString(IDS_USS_STARTING));
                theApp.QueueLogLine(true, GetResString(IDS_USS_STARTING));
            }

            pingLocker.Lock();
            m_state = _T("");
            pingLocker.Unlock();

            // There may be several reasons to start over with tracerouting again.
            // Currently we only restart if we get an unexpected ip back from the
            // ping at the set TTL.
            bool restart = false;

            DWORD lastLoopTick = ::GetTickCount();
            DWORD lastUploadReset = 0;

//>>> WiZaRd::ZZUL Upload [ZZ]
            sint64 integral = 0;
            CList<sint64, sint64> oldErrors;
            sint32 damper = 0;
            UINT damperCounter = 0;
//<<< WiZaRd::ZZUL Upload [ZZ]
            while (doRun && enabled && restart == false)
            {
                DWORD ticksBetweenPings = 1000;
//>>> WiZaRd::ZZUL Upload [ZZ]
                //if(upload > 0)
                if (maxUpload > 0)
//<<< WiZaRd::ZZUL Upload [ZZ]
                {
                    // ping packages being 64 bytes, this should use 1% of bandwidth (one hundredth of bw).
//>>> WiZaRd::ZZUL Upload [ZZ]
                    //ticksBetweenPings = (64*100*1000)/upload;
                    ticksBetweenPings = (64*100*1000)/maxUpload;
//<<< WiZaRd::ZZUL Upload [ZZ]

                    if (ticksBetweenPings < 125)
                        // never ping more than 8 packages a second
                        ticksBetweenPings = 125;
                    else if (ticksBetweenPings > 1000)
                        ticksBetweenPings = 1000;
                }

                DWORD curTick = ::GetTickCount();

                DWORD timeSinceLastLoop = curTick-lastLoopTick;
                if (timeSinceLastLoop < ticksBetweenPings)
                {
                    //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Sleeping %i ms, timeSinceLastLoop %i ms ticksBetweenPings %i ms"), ticksBetweenPings-timeSinceLastLoop, timeSinceLastLoop, ticksBetweenPings);
                    Sleep(ticksBetweenPings-timeSinceLastLoop);
                }

                lastLoopTick = curTick;

                prefsLocker.Lock();
                double pingTolerance = m_pingTolerance;
                UINT pingToleranceMilliseconds = m_iPingToleranceMilliseconds;
                bool useMillisecondPingTolerance = m_bUseMillisecondPingTolerance;
                UINT goingUpDivider = m_goingUpDivider;
                UINT goingDownDivider = m_goingDownDivider;
                UINT numberOfPingsForAverage = m_iNumberOfPingsForAverage;
                lowestInitialPingAllowed = m_LowestInitialPingAllowed; // PENDING
                UINT curUpload = m_CurUpload;

                bool initiateFastReactionPeriod = m_initiateFastReactionPeriod;
                m_initiateFastReactionPeriod = false;
                prefsLocker.Unlock();

                if (initiateFastReactionPeriod)
                {
                    theApp.QueueDebugLogLine(false, GetResString(IDS_USS_MANUALUPLOADLIMITDETECTED));
                    theApp.QueueLogLine(true, GetResString(IDS_USS_MANUALUPLOADLIMITDETECTED));

                    // the first 60 seconds will use hardcoded up/down slowness that is faster
                    initTime = ::GetTickCount();
                }

                DWORD tempTick = ::GetTickCount();

//>>> WiZaRd::ZZUL Upload [ZZ]
                /*
                                if(tempTick - initTime < SEC2MS(20))
                                {
                                    goingUpDivider = 1;
                                    goingDownDivider = 1;
                                }
                                else if(tempTick - initTime < SEC2MS(30))
                                {
                                    goingUpDivider = (UINT)(goingUpDivider * 0.25);
                                    goingDownDivider = (UINT)(goingDownDivider * 0.25);
                                }
                                else if(tempTick - initTime < SEC2MS(40))
                                {
                                    goingUpDivider = (UINT)(goingUpDivider * 0.5);
                                    goingDownDivider = (UINT)(goingDownDivider * 0.5);
                                }
                                else if(tempTick - initTime < SEC2MS(60))
                                {
                                    goingUpDivider = (UINT)(goingUpDivider * 0.75);
                                    goingDownDivider = (UINT)(goingDownDivider * 0.75);
                                }
                                else if(tempTick - initTime < SEC2MS(61))
                                {
                                    lastUploadReset = tempTick;
                                    prefsLocker.Lock();
                                    upload = m_CurUpload;
                                    prefsLocker.Unlock();
                                }
                */
                if (tempTick - initTime < SEC2MS(1))
                {
                    goingUpDivider = 1;
                    goingDownDivider = 1;
                }
                else if (tempTick - initTime < SEC2MS(60))
                {
                    // slide from 1 to the target divider value over 60 seconds. (dividerNow = (maxDivider-1)/(60secs-1sec)*(currentTime-starTime)+1)
                    goingUpDivider = (UINT)(((double)max(goingUpDivider, 1)-1)/(SEC2MS(60)-SEC2MS(1))*((tempTick-initTime)-SEC2MS(1)) + 1);
                    goingDownDivider = (UINT)(((double)max(goingDownDivider, 1)-1)/(SEC2MS(60)-SEC2MS(1))*((tempTick-initTime)-SEC2MS(1)) + 1);
                    //theApp.QueueDebugLogLine(false, _T("tempTick-initTime: %i, goingUpDivider: %i, goingDownDivider: %i"), tempTick-initTime, goingUpDivider, goingDownDivider);
                }
                else if (tempTick - initTime < SEC2MS(61))
                {
                    lastUploadReset = tempTick;
                    prefsLocker.Lock();
                    upload = m_CurUpload;
                    prefsLocker.Unlock();
                }
//<<< WiZaRd::ZZUL Upload [ZZ]

                goingDownDivider = max(goingDownDivider, 1);
                goingUpDivider = max(goingUpDivider, 1);

                UINT target_ping = (UINT)(initial_ping*pingTolerance);
                if (useMillisecondPingTolerance)
                    target_ping = pingToleranceMilliseconds;
                else
                    target_ping = (UINT)(initial_ping*pingTolerance);

                UINT raw_ping = target_ping; // this value will cause the upload speed not to change at all.

                bool pingFailure = false;
                for (uint64 pingTries = 0; doRun && enabled && (pingTries == 0 || pingFailure) && pingTries < 60; pingTries++)
                {
                    if (m_enabled == false)
                    {
                        enabled = false;
                    }

                    // ping the host to ping
                    PingStatus pingStatus = pinger.Ping(hostToPing, lastCommonTTL, false, useUdp);

                    if (pingStatus.success && pingStatus.status == IP_TTL_EXPIRED_TRANSIT)
                    {
                        if (pingStatus.destinationAddress != lastCommonHost)
                        {
                            // something has changed about the topology! We got another ip back from this ttl than expected.
                            // Do the tracerouting again to figure out new topology
                            CString lastCommonHostAddressString = ipstr(lastCommonHost);
                            CString destinationAddressString = ipstr(pingStatus.destinationAddress);

                            theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Network topology has changed. TTL: %i Expected ip: %s Got ip: %s Will do a new traceroute."), lastCommonTTL, lastCommonHostAddressString, destinationAddressString);
                            restart = true;
                        }

                        raw_ping = (UINT)pingStatus.delay;

                        if (pingFailure)
                        {
                            // only several pings in row should fails, the total doesn't count, so reset for each successful ping
                            pingFailure = false;

                            //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Ping #%i successful. Continuing."), pingTries);
                        }
                    }
                    else
                    {
                        raw_ping = target_ping*3+initial_ping*3; // this value will cause the upload speed be lowered.

                        pingFailure = true;

                        if (m_enabled == false)
                            enabled = false;
                        else if (pingTries > 3)
                            prefsEvent->Lock(1000);

                        //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: %s-Ping #%i failed. Reason follows"), useUdp?_T("UDP"):_T("ICMP"), pingTries);
                        //pinger.PIcmpErr(pingStatus.error);
                    }

                    if (m_enabled == false)
                        enabled = false;
                }

                if (pingFailure)
                {
                    if (enabled)
                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: No response to pings for a long time. Restarting..."));
                    restart = true;
                }

                if (restart == false)
                {
                    if (raw_ping > 0 && raw_ping < initial_ping && initial_ping > lowestInitialPingAllowed)
                    {
                        theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: New lowest ping: %i ms. Old: %i ms"), max(raw_ping,lowestInitialPingAllowed), initial_ping);
                        initial_ping = (UINT)max(raw_ping, lowestInitialPingAllowed);
                    }

                    pingDelaysTotal += raw_ping;
                    pingDelays.AddTail(raw_ping);
                    while (!pingDelays.IsEmpty() && (UINT)pingDelays.GetCount() > numberOfPingsForAverage)
                    {
                        UINT pingDelay = pingDelays.RemoveHead();
                        pingDelaysTotal -= pingDelay;
                    }

                    UINT pingAverage = Median(pingDelays); //(pingDelaysTotal/pingDelays.GetCount());
                    //int normalized_ping = pingAverage - initial_ping; //>>> WiZaRd::ZZUL Upload [ZZ]

                    //{
                    //    prefsLocker.Lock();
                    //    UINT tempCurUpload = m_CurUpload;
                    //    prefsLocker.Unlock();

                    //    theApp.QueueDebugLogLine(false, _T("USS-Debug: %i %i %i"), raw_ping, upload, tempCurUpload);
                    //}

                    pingLocker.Lock();
                    m_pingAverage = (UINT)pingAverage;
                    m_lowestPing = initial_ping;
                    pingLocker.Unlock();

                    // Calculate Waiting Time
//>>> WiZaRd::ZZUL Upload [ZZ]
                    /*
                                        sint64 hping = ((int)target_ping) - normalized_ping;

                                        // Calculate change of upload speed
                                        if(hping < 0)
                                        {
                                            //Ping too high
                                            acceptNewClient = false;

                                            // lower the speed
                                            sint64 ulDiff = hping*1024*10 / (sint64)(goingDownDivider*initial_ping);

                                            //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Down! Ping cur %i ms. Ave %I64i ms %i values. New Upload %i + %I64i = %I64i"), raw_ping, pingDelaysTotal/pingDelays.GetCount(), pingDelays.GetCount(), upload, ulDiff, upload+ulDiff);
                                            // prevent underflow
                                            if(upload > -ulDiff)
                                                upload = (UINT)(upload + ulDiff);
                                            else
                                                upload = 0;
                                        }
                                        else if(hping > 0)
                                        {
                                            //Ping lower than max allowed
                                            acceptNewClient = true;

                                            if(curUpload+30*1024 > upload)
                                            {
                                                // raise the speed
                                                uint64 ulDiff = hping*1024*10 / (uint64)(goingUpDivider*initial_ping);

                                                //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Up! Ping cur %i ms. Ave %I64i ms %i values. New Upload %i + %I64i = %I64i"), raw_ping, pingDelaysTotal/pingDelays.GetCount(), pingDelays.GetCount(), upload, ulDiff, upload+ulDiff);
                                                // prevent overflow
                                                if(_I32_MAX-upload > ulDiff)
                                                    upload = (UINT)(upload + ulDiff);
                                                else
                                                    upload = _I32_MAX;
                                            }
                                        }
                    */
                    sint64 error = ((int)target_ping) - (int)pingAverage;

                    //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: ----- Ping cur %3i ms. Median: %3i ms %2i values. Error: %3I64i Integral: %5I64i"), raw_ping, pingAverage, pingDelays.GetCount(), error, integral);
                    //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: ----- Ping cur %3i %u"), raw_ping, timeGetTime());

                    // Some kind of PID-controller.
                    // To get the same behaviour as old code, set pconst = 1.0f and the others to 0
                    // Someone may want to expose these to prefs to enable easier experimentation with the values
                    const float pconst = 0.90f;
                    const float iconst = 0.10f;
                    const float dconst = 0.30f;
                    const UINT numberOfOldErrors = 30*1000/ticksBetweenPings;

                    //if(error < -(sint32)target_ping)
                    //    error = -(sint32)target_ping;

                    sint64 lastError = error;
                    if (!oldErrors.IsEmpty())
                        lastError = oldErrors.GetTail();

                    sint64 derivate = error-lastError;

                    integral += error;
                    oldErrors.AddTail(error);
                    while (oldErrors.GetCount() > 1 && (m_NeedMoreSlots || (UINT)oldErrors.GetCount() > numberOfOldErrors))
                    {
                        sint64 oldError = oldErrors.RemoveHead();
                        integral -= oldError;
                    }

                    // calculate the change of speed
                    sint64 ulDeltaTemp = (sint64)(pconst*error + iconst*(integral/oldErrors.GetCount()) + dconst*derivate); //*1024*10 / (sint64)(goingDownDivider*initial_ping);

                    damperCounter++;

                    // Calculate change of upload speed
                    if (ulDeltaTemp < 0)
                    {
                        //Ping too high
                        acceptNewClient = false;

                        damper++;

                        if (damper > 10)
                        {
                            damper = -10;
                            // lower the speed
                            sint64 ulDelta = min(ulDeltaTemp*1024*10*20*20 / (sint64)(goingDownDivider*initial_ping*max(damperCounter,20)), -1);

                            damperCounter = 0;

                            UINT newUpload;

                            // prevent underflow
                            if (upload > -ulDelta)
                                newUpload = (UINT)(upload + ulDelta);
                            else
                                newUpload = 0;

                            //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Down! Ping cur %3i ms. Median: %3i ms %2i values. Error: %3I64i Integral: %5I64i New Upload %6i + %4I64i = %6i"), raw_ping, pingAverage, pingDelays.GetCount(), error, integral, upload, ulDelta, newUpload);

                            upload = newUpload;
                        }
                    }
                    else if (ulDeltaTemp > 0)
                    {
                        //Ping lower than max allowed
                        acceptNewClient = true;

                        if (curUpload+10*1024 > upload && !m_NeedMoreSlots)
                        {
                            damper--;

                            if (damper < -10)
                            {
                                damper = 10;

                                // raise the speed
                                uint64 ulDelta = max(ulDeltaTemp*1024*10*20*20 / (uint64)(goingUpDivider*initial_ping*max(damperCounter,20)), 1);

                                damperCounter = 0;

                                UINT newUpload;

                                // prevent overflow
                                if (_I32_MAX-upload > ulDelta)
                                    newUpload = (UINT)(upload + ulDelta);
                                else
                                    newUpload = _I32_MAX;

                                //theApp.QueueDebugLogLine(false,_T("UploadSpeedSense: Up!   Ping cur %3i ms. Median: %3i ms %2i values. Error: %3I64i Integral: %5I64i New Upload %6i + %4I64i = %6i"), raw_ping, pingAverage, pingDelays.GetCount(), error, integral, upload, ulDelta, newUpload);

                                upload = newUpload;
                            }
                        }
                    }
//<<< WiZaRd::ZZUL Upload [ZZ]
                    prefsLocker.Lock();
                    if (upload < minUpload)
                    {
                        upload = minUpload;
//>>> WiZaRd::ZZUL Upload [ZZ]
                        //acceptNewClient = true;
//<<< WiZaRd::ZZUL Upload [ZZ]
                    }
                    if (upload > maxUpload)
                        upload = maxUpload;
                    prefsLocker.Unlock();
                    SetUpload(upload);
                    if (m_enabled == false)
                        enabled = false;
                }
            }
        }
    }

    // Signal that we have ended.
    threadEndedEvent->SetEvent();

    return 0;
}
UINT LastCommonRouteFinder::Median(UInt32Clist& list)
{
    UINT size = list.GetCount();

    if (size == 1)
        return list.GetHead();

//>>> WiZaRd::ZZUL Upload [ZZ]
    /*
        if(size == 2)
            return (list.GetHead()+list.GetTail())/2;

        if(size > 2)
        {
            // if more than 2 elements, we need to sort them to find the middle.
            UINT* arr = new UINT[size];

            UINT counter = 0;
            for(POSITION pos = list.GetHeadPosition(); pos; list.GetNext(pos))
            {
                arr[counter] = list.GetAt(pos);
                counter++;
            }

            std::sort(arr, arr+size);

            double returnVal;

            if(size%2)
                returnVal = arr[size/2];
            else
                returnVal = (arr[size/2-1] + arr[size/2])/2;

            delete[] arr;

            return (UINT)returnVal;
        }
    */
    if (size == 2)
        return max(list.GetHead(), list.GetTail());

    if (size > 2)
    {
        UINT maxVal = 0;

        for (POSITION pos = list.GetHeadPosition(); pos; list.GetNext(pos))
            maxVal = max(maxVal, list.GetAt(pos));

        return maxVal;
    }
//<<< WiZaRd::ZZUL Upload [ZZ]

    // Undefined! Shouldn't be called with no elements in list.
    return 0;
}