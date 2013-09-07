//this file is part of kMule
//Copyright (C)2012 WiZaRd ( strEmail.Format("%s@%s", "thewizardofdos", "gmail.com") / http://www.emulefuture.de )
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
#include "StdAfx.h"
#include "NATPMPWrapper.h"
#include "emule.h"
#include "Log.h"
#include "otherfunctions.h"
//////////////////////////////////////////////////////////////////////////
#include "Preferences.h"
#include "emuleDlg.h"
#include "opcodes.h"
#include "UserMsgs.h"
#include "Exceptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef USE_NAT_PMP

//////////////////////////////////////////////////////////////////////////
// CNATPMPThreadWrapper
CNATPMPThreadWrapper::CNATPMPThreadWrapper()
{
    m_hRefreshTimer = NULL;
    m_bAbortDiscovery = false;
    m_hThreadHandle = NULL;

    // allow only ONE wrapper instance!
    if (m_pNATPMP != NULL)
    {
        delete m_pNATPMP;
        m_pNATPMP = NULL;
        ASSERT(0);
    }
    m_pNATPMP = new CNATPMP();
    //m_settingsLock.Lock();
    AddTask(eInitializeNATPMP, false);
    AddTask(eRetrievePublicIP, false);
    //m_settingsLock.Unlock();
}

CNATPMPThreadWrapper::~CNATPMPThreadWrapper()
{
    KillRefreshTimer();

    delete m_pNATPMP;
    m_pNATPMP = NULL;
}

void CNATPMPThreadWrapper::StartRefreshTimer()
{
    if (m_hRefreshTimer == NULL)
    {
        VERIFY((m_hRefreshTimer = SetTimer(0, 0, NATPMP_REFRESH_TIME, RefreshTimer)) != NULL);
        if (thePrefs.GetVerbose() && !m_hRefreshTimer)
            AddDebugLogLine(true, L"Failed to create 'NAT-PMP refresh' timer - %s", GetErrorMessage(GetLastError()));
    }
}

void CNATPMPThreadWrapper::KillRefreshTimer()
{
    if (m_hRefreshTimer)
    {
        KillTimer(0, m_hRefreshTimer);
        m_hRefreshTimer = NULL;
    }
}

VOID CALLBACK CNATPMPThreadWrapper::RefreshTimer(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
{
    // NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
    try
    {
        // Barry - Don't do anything if the app is shutting down - can cause unhandled exceptions
        if (!theApp.emuledlg->IsRunning())
            return;

        theApp.emuledlg->RefreshNATPMP();
    }
    CATCH_DFLT_EXCEPTIONS(L"CNATPMPThreadWrapper::RefreshTimer")
}

bool	CNATPMPThreadWrapper::IsReady()
{
    // the only check we need to do is if we are already busy with some async/threaded function
    CSingleLock lockTest(&m_mutBusy);
    if (!m_bAbortDiscovery && lockTest.Lock(0))
        return true;
    return false;
}

void	CNATPMPThreadWrapper::StopAsyncFind()
{
    CSingleLock lockTest(&m_mutBusy);
    m_bAbortDiscovery = true; // if there is a thread, tell him to abort as soon as possible - he won't sent a Resultmessage when aborted
    if (!lockTest.Lock(7000)) // give the thread 7 seconds to exit gracefully - it should never really take that long
    {
        // that quite bad, something seems to be locked up. There isn't a good solution here, we need the thread to quit
        // or it might try to access a deleted object later, but terminating him is quite bad too. Well..
        DebugLogError(L"Waiting for NATPMP StartDiscoveryThread to quit failed, trying to terminate the thread...");
        if (m_hThreadHandle != NULL)
        {
            if (TerminateThread(m_hThreadHandle, 0))
                DebugLogError(L"...OK");
            else
                DebugLogError(L"...Failed");
        }
        else
            ASSERT(0);
    }
    else
    {
        DebugLog(L"Aborted any possible NATPMP StartDiscoveryThread");
    }
    //do that in *any* case!
    m_bAbortDiscovery = false;
    m_hThreadHandle = NULL;
}

void	CNATPMPThreadWrapper::DeletePorts()
{
    m_settingsLock.Lock();

    for (int i = 0; i < _countof(m_portList); ++i)
        m_portList[i].RemoveAll();
    AddTask(eDeleteTCPMappings, false);
    AddTask(eDeleteUDPMappings, false);

    m_settingsLock.Unlock();

    KillRefreshTimer();
}

void	CNATPMPThreadWrapper::StartDiscovery(const uint16 nTCPPort, const uint16 nUDPPort)
{
    m_settingsLock.Lock();

    if (nTCPPort != 0)
    {
        AddPort(nTCPPort, NATPMP_PROTOCOL_TCP, false);
        AddTask(eMapTCPPorts, false);
    }
    if (nUDPPort != 0)
    {
        AddPort(nUDPPort, NATPMP_PROTOCOL_UDP, false);
        AddTask(eMapUDPPorts, false);
    }

    m_settingsLock.Unlock();

    if (m_bAbortDiscovery)
        return;

    StartThread();
}

void	CNATPMPThreadWrapper::AddPort(const uint16 port, const int protocol, const bool bLock)
{
    if (bLock)
        m_settingsLock.Lock();

    if (protocol == NATPMP_PROTOCOL_TCP || protocol == NATPMP_PROTOCOL_UDP)
    {
        if (m_portList[protocol-1].Find(port) == NULL)
            m_portList[protocol-1].AddTail(port);
    }
    else
        ASSERT(0);

    if (bLock)
        m_settingsLock.Unlock();
}

void	CNATPMPThreadWrapper::AddTask(_ENATPMPTask task, const bool bLock)
{
    if (bLock)
        m_settingsLock.Lock();

// 	if(m_taskList.Find(task))
// 		ASSERT(0);
// 	else
    m_taskList.AddTail(task);

    if (bLock)
        m_settingsLock.Unlock();
}

void	CNATPMPThreadWrapper::RemovePendingTasks()
{
    m_settingsLock.Lock();
    m_taskList.RemoveAll();
    m_settingsLock.Unlock();
}

void	CNATPMPThreadWrapper::SetMessageWanted(const bool bWanted)
{
    m_bMessageWanted = bWanted;
}

bool CNATPMPThreadWrapper::CheckAndRefresh()
{
    // on a CheckAndRefresh we don't do any new time consuming discovery tries, we expect to find the same router like the first time
    // and of course we also don't delete old ports (this was done on Discovery) but only check that our current mappings still exist
    // and refresh them if not
    if (m_bAbortDiscovery)
    {
        DebugLog(L"Not refreshing NATPMP ports because they don't seem to be mapped in the first place");
        return false;
    }
//>>> WiZaRd
    else if (!IsReady())
    {
        DebugLog(L"Not refreshing NATPMP ports because they are already in the process of being refreshed");
        return false;
    }
//<<< WiZaRd

    DebugLog(L"Checking and refreshing NATPMP ports");

    //m_settingsLock.Lock();
    // add a complete "reset" task
    AddTask(eDeleteTCPMappings, false);
    AddTask(eDeleteUDPMappings, false);
    AddTask(eInitializeNATPMP);
    AddTask(eRetrievePublicIP);
    AddTask(eMapTCPPorts, false);
    AddTask(eMapUDPPorts, false);
    //m_settingsLock.Unlock();
    StartThread();
    return true;
}

void CNATPMPThreadWrapper::StartThread()
{
    if (m_bAbortDiscovery)
        return;

    CNATPMPThread* pThread = (CNATPMPThread*)AfxBeginThread(RUNTIME_CLASS(CNATPMPThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    m_hThreadHandle = pThread->m_hThread;
    pThread->SetValues(this);
    pThread->ResumeThread();
}

//////////////////////////////////////////////////////////////////////////
// CNATPMP
CNATPMP::CNATPMP()
{
    m_bInitialised = false;
    m_ForceGW = 0;
    m_Gateway = 0;
}

CNATPMP::~CNATPMP()
{
    UnInit();
}

// Reset wrapper by removing current mappings and re-initializing the wrapper
bool	CNATPMP::Reset()
{
    UnInit();
    Init();
    return m_bInitialised;
}

bool	CNATPMP::Init()
{
    int success = initnatpmp(&m_natpmp, m_ForceGW, m_Gateway);
    if (success != 0)
    {
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: initnatpmp failed: %hs", strnatpmperr(success));
        m_bInitialised = false;
    }
    else
    {
        in_addr gateway_in_use;
        gateway_in_use.s_addr = m_natpmp.gateway;
        theApp.QueueDebugLogLineEx(LOG_INFO, L"NAT-PMP: gateway: %s", ipstr(gateway_in_use));
        m_bInitialised = true;
    }

    return m_bInitialised;
}

bool	CNATPMP::UnInit()
{
    bool bWasInitialised = m_bInitialised;

    if (m_bInitialised)
    {
        RemoveAllPortMappings();

        int success = closenatpmp(&m_natpmp);
        if (success != 0)
            theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: closenatpmp failed: %hs", strnatpmperr(success));
    }

    m_bInitialised = false;
    m_ForceGW = 0;
    m_Gateway = 0;

    return bWasInitialised;
}

bool	CNATPMP::PerformSelect()
{
    bool bResult = false;

    fd_set fds;
    FD_ZERO(&fds);
#pragma warning(disable:4389) // '==': Konflikt zwischen 'signed' und 'unsigned'
    FD_SET(m_natpmp.s, &fds);
#pragma warning(default:4389) // '==': Konflikt zwischen 'signed' und 'unsigned'
    struct timeval timeout;
    int success = getnatpmprequesttimeout(&m_natpmp, &timeout);
    if (success != 0)
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: getnatpmprequesttimeout failed: %hs", strnatpmperr(success));
    else
    {
        theApp.QueueDebugLogLineEx(LOG_INFO, L"NAT-PMP: timeout: %ds %dµs", timeout.tv_sec, timeout.tv_usec);

        // check if the socket's ready
        success = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
        if (success > 0)
            bResult = true;
        else
        {
            if (success == 0)
                theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: select timed out");
            else
                theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: select failed: %hs", strnatpmperr(success));
        }
    }

    return bResult;
}

bool	CNATPMP::GetPublicIPAddress()
{
    bool bResult = false;

    int success = sendpublicaddressrequest(&m_natpmp);
    if (success != 2)
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: sendpublicaddressrequest failed: %hs", strnatpmperr(success));
    else
    {
        natpmpresp_t response;
        do
        {
            if (!PerformSelect())
                break;

            success = readnatpmpresponseorretry(&m_natpmp, &response);
            if (success == 0)
            {
                /* TODO : check that success.type == 0 */
                memcpy(&m_PublicIP, &response.pnu.publicaddress.addr, sizeof(m_PublicIP));
                theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"NAT-PMP: public IP address: %s", ipstr(m_PublicIP.S_un.S_addr));
                theApp.QueueDebugLogLineEx(LOG_INFO, L"NAT-PMP: epoch: %u", response.epoch);
            }
            else if (success != NATPMP_TRYAGAIN)
                theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: readnatpmpresponseorretry failed: %hs", strnatpmperr(success));
        }
        while (success == NATPMP_TRYAGAIN);
    }

    return bResult;
}

bool	CNATPMP::PerformMapping(const uint16 publicPort, const uint16 privatePort, const int protocol, const UINT lifeTime)
{
    bool bResult = false;

    if (protocol != NATPMP_PROTOCOL_TCP && protocol != NATPMP_PROTOCOL_UDP)
        ASSERT(0);
    else
    {
        int success = sendnewportmappingrequest(&m_natpmp, protocol, privatePort, publicPort, lifeTime);
        if (success != 12)
            theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: sendnewportmappingrequest failed: %hs", strnatpmperr(success));
        else
        {
            natpmpresp_t response;
            do
            {
                if (!PerformSelect())
                    break;

                success = readnatpmpresponseorretry(&m_natpmp, &response);
                if (success == 0)
                {
                    theApp.QueueDebugLogLineEx(LOG_INFO, L"NAT-PMP: epoch: %u", response.epoch);

                    ASSERT(response.pnu.newportmapping.mappedpublicport == publicPort);
                    ASSERT(response.pnu.newportmapping.privateport == privatePort);
                    ASSERT(response.pnu.newportmapping.lifetime == lifeTime);
                    ASSERT(response.type == protocol);

                    bResult = true;
                }
                else if (success != NATPMP_TRYAGAIN)
                    theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: readnatpmpresponseorretry failed: %hs", strnatpmperr(success));
            }
            while (success == NATPMP_TRYAGAIN);
        }
    }

    return bResult;
}

bool	CNATPMP::AddPortMapping(const uint16 publicPort, const uint16 privatePort, const int protocol, const UINT lifeTime)
{
    bool bResult = PerformMapping(publicPort, privatePort, protocol, lifeTime);

    if (bResult)
    {
        theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"NAT-PMP: Mapped public port %u to local port %u via protocol %s with lifetime %u",
                                   publicPort, privatePort,
                                   protocol == NATPMP_PROTOCOL_UDP ? L"UDP" : (protocol == NATPMP_PROTOCOL_TCP ? L"TCP" : L"UNKNOWN"),
                                   lifeTime);
    }

    return bResult;
}

bool	CNATPMP::RemovePortMapping(const uint16 publicPort, const uint16 privatePort, const int protocol)
{
    bool bResult = PerformMapping(publicPort, privatePort, protocol, 0);

    if (bResult)
    {
        theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"NAT-PMP: Deleted %s mapping from public port %u to local port %u",
                                   protocol == NATPMP_PROTOCOL_UDP ? L"UDP" : (protocol == NATPMP_PROTOCOL_TCP ? L"TCP" : L"UNKNOWN"),
                                   publicPort,
                                   privatePort);
    }

    return bResult;
}

bool	CNATPMP::RemovePortMappings(const int protocol)
{
    bool bResult = PerformMapping(0, 0, protocol, 0);

    if (bResult)
    {
        theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"NAT-PMP: Deleted %s mappings",
                                   protocol == NATPMP_PROTOCOL_UDP ? L"UDP" : (protocol == NATPMP_PROTOCOL_TCP ? L"TCP" : L"UNKNOWN"));
    }

    return bResult;
}

bool	CNATPMP::RemoveAllPortMappings()
{
    bool bResult = true;

    if (!RemovePortMappings(NATPMP_PROTOCOL_TCP))
        bResult = false;
    if (!RemovePortMappings(NATPMP_PROTOCOL_UDP))
        bResult = false;

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CNATPMPThread
typedef CNATPMPThread CNATPMPThread;
IMPLEMENT_DYNCREATE(CNATPMPThread, CWinThread)

CNATPMPThread::CNATPMPThread()
{
    m_pOwner = NULL;
}

BOOL CNATPMPThread::InitInstance()
{
    DbgSetThreadName("CNATPMPThread");
    InitThreadLocale();
    return TRUE;
}

int CNATPMPThread::Run()
{
    if (!m_pOwner)
        return 0;

    CSingleLock sLock(&m_pOwner->m_mutBusy);
    if (!sLock.Lock(0))
    {
        DebugLogWarning(L"CNATPMPThread::Run, failed to acquire Lock, another mapping try might be running already");
        return 0;
    }

    if (m_pOwner->m_bAbortDiscovery) // requesting to abort ASAP?
        return 0;

    bool bSucceeded = true;
#if !(defined(_DEBUG) || defined(_BETA))
    try
#endif
    {
        m_pOwner->m_settingsLock.Lock();
        for (POSITION pos = m_pOwner->m_taskList.GetHeadPosition(); pos;)
        {
            _ENATPMPTask task = m_pOwner->m_taskList.GetNext(pos);
            switch (task)
            {
            case eInitializeNATPMP:
                if (!m_pNATPMP->Reset())
                    bSucceeded = false;
                break;

            case eRetrievePublicIP:
                if (!m_pNATPMP->Reset())
                    bSucceeded = false;
                break;

            case eMapTCPPorts:
                for (POSITION pos = m_pOwner->m_portList[NATPMP_PROTOCOL_TCP-1].GetHeadPosition(); pos;)
                {
                    uint16 port = m_pOwner->m_portList[NATPMP_PROTOCOL_TCP-1].GetNext(pos);
                    if (m_pNATPMP->AddPortMapping(port, port, NATPMP_PROTOCOL_TCP, DFLT_NATPMP_LIFETIME))
                        m_pOwner->StartRefreshTimer();
                    else
                        bSucceeded = false;
                }
                break;

            case eMapUDPPorts:
                for (POSITION pos = m_pOwner->m_portList[NATPMP_PROTOCOL_UDP-1].GetHeadPosition(); pos;)
                {
                    uint16 port = m_pOwner->m_portList[NATPMP_PROTOCOL_UDP-1].GetNext(pos);
                    if (m_pNATPMP->AddPortMapping(port, port, NATPMP_PROTOCOL_UDP, DFLT_NATPMP_LIFETIME))
                        m_pOwner->StartRefreshTimer();
                    else
                        bSucceeded = false;
                }
                break;

            case eDeleteTCPMappings:
                if (!m_pNATPMP->RemovePortMappings(NATPMP_PROTOCOL_TCP))
                    bSucceeded = false;
                break;

            case eDeleteUDPMappings:
                if (!m_pNATPMP->RemovePortMappings(NATPMP_PROTOCOL_UDP))
                    bSucceeded = false;
                break;

            default:
                theApp.QueueDebugLogLineEx(LOG_ERROR, L"NAT-PMP: can't handle unknown task %u", task);
                break;
            }
        }
        m_pOwner->m_settingsLock.Unlock();
        m_pOwner->RemovePendingTasks();
    }
#if !(defined(_DEBUG) || defined(_BETA))
    catch (...)
    {
        DebugLogError(L"Unknown Exception in CNATPMPThread::Run()");
    }
#endif
    if (!m_pOwner->m_bAbortDiscovery // don't send a result on a abort request
            && m_pOwner->m_bMessageWanted
            && theApp.emuledlg && theApp.emuledlg->IsRunning())
    {
        ::PostMessage(theApp.emuledlg->GetSafeHwnd(), UM_NATPMP_RESULT, (WPARAM)(bSucceeded ? CNATPMP::NATPMP_OK : CNATPMP::NATPMP_FAILED), /*m_bCheckAndRefresh ? 1 :*/ 0);
        m_pOwner->SetMessageWanted(false);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CemuleDlg::NATPMPTimeOutTimer(HWND /*hwnd*/, UINT /*uiMsg*/, UINT /*idEvent*/, DWORD /*dwTime*/)
{
    ::PostMessage(theApp.emuledlg->GetSafeHwnd(), UM_NATPMP_RESULT, (WPARAM)CNATPMP::NATPMP_TIMEOUT, 0);
}


LRESULT CemuleDlg::OnNATPMPResult(WPARAM wParam, LPARAM lParam)
{
    bool bWasRefresh = lParam != 0;
//>>> WiZaRd - handle "NATPMP_TIMEOUT" events!
    //	if (!bWasRefresh && wParam == CNATPMP::NATPMP_FAILED)
    if (!bWasRefresh && wParam != CNATPMP::NATPMP_OK)
//<<< WiZaRd - handle "NATPMP_TIMEOUT" events!
    {
//>>> WiZaRd - handle "NATPMP_TIMEOUT" events!
        //just to be sure, stop any running services and also delete the mapped ports (if necessary)
        if (wParam == CNATPMP::NATPMP_TIMEOUT)
        {
            theApp.m_pNATPMPThreadWrapper->StopAsyncFind();
//			if (thePrefs.CloseUPnPOnExit())
            theApp.m_pNATPMPThreadWrapper->DeletePorts();
        }
//<<< WiZaRd - handle "NATPMP_TIMEOUT" events!
    }
    if (m_hNATPMPTimeOutTimer != 0)
    {
        VERIFY(::KillTimer(NULL, m_hNATPMPTimeOutTimer));
        m_hNATPMPTimeOutTimer = 0;
    }
    if (IsRunning() && m_bConnectRequestDelayedForNATPMP)
    {
        m_bConnectRequestDelayedForNATPMP = false;
        if (!m_bConnectRequestDelayedForUPnP)
            StartConnection();
    }

    if (!bWasRefresh)
    {
        if (wParam == CNATPMP::NATPMP_OK)
        {
            // remember the last working implementation
            // TODO: ports may not be correct here, though that not that import, right now
            theApp.QueueLogLineEx(LOG_SUCCESS, L"NAT-PMP successfully set up mappings for ports %u (TCP) and %u (UDP)", thePrefs.GetPort(), thePrefs.GetUDPPort());
            //theApp.QueueLogLineEx(LOG_SUCCESS, GetResString(IDS_UPNPSUCCESS), theApp.m_pNATPMPThreadWrapper->GetUsedTCPPort(), theApp.m_pNATPMPThreadWrapper->GetUsedUDPPort());
        }
        else
            theApp.QueueLogLineEx(LOG_ERROR, L"NAT-PMP failed to setup port mappings, please map those ports manually if necessary!"); // GetResString(IDS_UPNPFAILED)
    }

    return 0;
}


void CemuleDlg::StartNATPMP(bool bReset, uint16 nForceTCPPort, uint16 nForceUDPPort)
{
//	if (!thePrefs.IsNATPMPEnabled())
//		return;

    if (theApp.m_pNATPMPThreadWrapper != NULL && (m_hNATPMPTimeOutTimer == 0 || !bReset))
    {
        if (bReset)
        {
            Log(L"Trying to setup port mappings with NAT-PMP..."); // GetResString(IDS_UPNPSETUP)
        }
        try
        {
            if (theApp.m_pNATPMPThreadWrapper->IsReady())
            {
                theApp.m_pNATPMPThreadWrapper->SetMessageWanted(true);
                if (bReset)
                    VERIFY((m_hNATPMPTimeOutTimer = ::SetTimer(NULL, NULL, SEC2MS(40), NATPMPTimeOutTimer)) != NULL);
                theApp.m_pNATPMPThreadWrapper->StartDiscovery(((nForceTCPPort != 0) ? nForceTCPPort : thePrefs.GetPort())
                        , ((nForceUDPPort != 0) ? nForceUDPPort : thePrefs.GetUDPPort()));
            }
            else
                ::PostMessage(theApp.emuledlg->GetSafeHwnd(), UM_NATPMP_RESULT, (WPARAM)CNATPMP::NATPMP_FAILED, 0);
        }
        catch (CException* e)
        {
            e->Delete();
        }
    }
    else
        ASSERT(0);
}

//>>> WiZaRd
void	CemuleDlg::RemoveNATPMPMappings()
{
//	if (!thePrefs.IsNATPMPEnabled())
//		return;

    if (theApp.m_pNATPMPThreadWrapper != NULL && m_hNATPMPTimeOutTimer == 0)
    {
        try
        {
            if (theApp.m_pNATPMPThreadWrapper->IsReady())
            {
                theApp.m_pNATPMPThreadWrapper->StopAsyncFind();
                /*if (thePrefs.CloseUPnPOnExit())*/
                theApp.m_pNATPMPThreadWrapper->DeletePorts();
            }
            else
                DebugLogWarning(L"RemoveNATPMPMappings, implementation not ready");
        }
        catch (CException* e)
        {
            e->Delete();
        }
    }
    else
        ASSERT(0);
}
//<<< WiZaRd

void CemuleDlg::RefreshNATPMP(bool bRequestAnswer)
{
//	if (!thePrefs.IsNATPMPEnabled())
//		return;

    if (theApp.m_pNATPMPThreadWrapper != NULL && m_hNATPMPTimeOutTimer == 0)
    {
        try
        {
            if (theApp.m_pNATPMPThreadWrapper->IsReady())
            {
                if (bRequestAnswer)
                    theApp.m_pNATPMPThreadWrapper->SetMessageWanted(true);
                if (theApp.m_pNATPMPThreadWrapper->CheckAndRefresh() && bRequestAnswer)
                    VERIFY((m_hNATPMPTimeOutTimer = ::SetTimer(NULL, NULL, SEC2MS(10), NATPMPTimeOutTimer)) != NULL);
                else
                    theApp.m_pNATPMPThreadWrapper->SetMessageWanted(false);
            }
            else
                DebugLogWarning(L"RefreshNATPMP, implementation not ready");
        }
        catch (CException* e)
        {
            e->Delete();
        }
    }
    else
        ASSERT(0);
}

#endif