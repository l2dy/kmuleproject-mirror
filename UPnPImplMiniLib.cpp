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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "StdAfx.h"
#include "emule.h"
#include "preferences.h"
#include "UPnPImplMiniLib.h"
#include "Log.h"
#include "Otherfunctions.h"
#include ".\miniupnpc\miniupnpc.h"
#include ".\miniupnpc\upnpcommands.h"
#include ".\miniupnpc\upnperrors.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMutex CUPnPImplMiniLib::m_mutBusy;

CUPnPImplMiniLib::CUPnPImplMiniLib()
{
    m_nOldUDPPort = 0;
    m_nOldTCPPort = 0;
    m_nOldTCPWebPort = 0;
    m_pURLs = NULL;
    m_pIGDData = NULL;
    m_bAbortDiscovery = false;
    m_bSucceededOnce = false;
    m_achLanIP[0] = 0;
    m_hThreadHandle = NULL;
}

CUPnPImplMiniLib::~CUPnPImplMiniLib()
{
    if (m_pURLs != NULL)
        FreeUPNPUrls(m_pURLs);
    delete m_pURLs;
    m_pURLs = NULL;
    delete m_pIGDData;
    m_pIGDData = NULL;
}

bool CUPnPImplMiniLib::IsReady()
{
    // the only check we need to do is if we are already busy with some async/threaded function
    CSingleLock lockTest(&m_mutBusy);
    if (!m_bAbortDiscovery && lockTest.Lock(0))
        return true;
    return false;
}

void CUPnPImplMiniLib::StopAsyncFind()
{
    CSingleLock lockTest(&m_mutBusy);
    m_bAbortDiscovery = true; // if there is a thread, tell him to abort as soon as possible - he won't sent a Resultmessage when aborted
    if (!lockTest.Lock(7000)) // give the thread 7 seconds to exit gracefully - it should never really take that long
    {
        // that quite bad, something seems to be locked up. There isn't a good solution here, we need the thread to quit
        // or it might try to access a deleted object later, but terminating him is quite bad too. Well..
        DebugLogError(L"Waiting for UPnP StartDiscoveryThread to quit failed, trying to terminate the thread...");
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
        DebugLog(L"Aborted any possible UPnP StartDiscoveryThread");
    }
    //do that in *any* case!
    m_bAbortDiscovery = false;
    m_hThreadHandle = NULL;
}

void CUPnPImplMiniLib::DeletePorts()
{
    m_nOldUDPPort = (ArePortsForwarded() == TRIS_TRUE) ? m_nUDPPort : 0;
    m_nUDPPort = 0;
    m_nOldTCPPort = (ArePortsForwarded() == TRIS_TRUE) ? m_nTCPPort : 0;
    m_nTCPPort = 0;
    m_nOldTCPWebPort = (ArePortsForwarded() == TRIS_TRUE) ? m_nTCPWebPort : 0;
    m_nTCPWebPort = 0;
    m_bUPnPPortsForwarded = TRIS_FALSE;
    DeletePorts(false);
}

void CUPnPImplMiniLib::DeletePorts(bool bSkipLock)
{
    // this function itself blocking, because its called at the end of eMule and we need to wait for it to finish
    // before going on anyway. It might be called from the non-blocking StartDiscovery() function too however
    CSingleLock lockTest(&m_mutBusy);
    if (bSkipLock || lockTest.Lock(0))
    {
        if (m_nOldTCPPort != 0)
        {
            if (m_pURLs == NULL || m_pURLs->controlURL == NULL || m_pIGDData == NULL)
            {
                ASSERT(0);
                return;
            }
            const char achTCP[] = "TCP";
            char achPort[10];
            sprintf(achPort, "%u", m_nOldTCPPort);
            int nResult = UPNP_DeletePortMapping(m_pURLs->controlURL, m_pIGDData->first.servicetype, achPort, achTCP, NULL);
            if (nResult == UPNPCOMMAND_SUCCESS)
            {
                DebugLog(L"Sucessfully removed mapping for port %u (%s)", m_nOldTCPPort, L"TCP");
                m_nOldTCPPort = 0;
            }
            else
                DebugLogWarning(L"Failed to remove mapping for port %u (%s)", m_nOldTCPPort, L"TCP");

        }
        else
        {
            DebugLog(L"No UPnP Mappings to remove, aborting");
            return; // UDP port cannot be set if TCP port was empty
        }

        if (m_nOldUDPPort != 0)
        {
            const char achTCP[] = "UDP";
            char achPort[10];
            sprintf(achPort, "%u", m_nOldUDPPort);
            int nResult = UPNP_DeletePortMapping(m_pURLs->controlURL, m_pIGDData->first.servicetype, achPort, achTCP, NULL);
            if (nResult == UPNPCOMMAND_SUCCESS)
            {
                DebugLog(L"Sucessfully removed mapping for port %u (%s)", m_nOldUDPPort, L"UDP");
                m_nOldTCPPort = 0;
            }
            else
                DebugLogWarning(L"Failed to remove mapping for port %u (%s)", m_nOldUDPPort, L"UDP");
        }

        if (m_nOldTCPWebPort != 0)
        {
            const char achTCP[] = "TCP";
            char achPort[10];
            sprintf(achPort, "%u", m_nOldTCPWebPort);
            int nResult = UPNP_DeletePortMapping(m_pURLs->controlURL, m_pIGDData->first.servicetype, achPort, achTCP, NULL);
            if (nResult == UPNPCOMMAND_SUCCESS)
            {
                DebugLog(L"Sucessfully removed mapping for webinterface port %u (%s)", m_nOldTCPPort, L"TCP");
                m_nOldTCPWebPort = 0;
            }
            else
                DebugLogWarning(L"Failed to remove mapping for webinterface port %u (%s)", m_nOldTCPPort, L"TCP");
        }
    }
    else
        DebugLogError(L"Unable to remove port mappings - implementation still busy");

}

void CUPnPImplMiniLib::StartDiscovery(uint16 nTCPPort, uint16 nUDPPort, uint16 nTCPWebPort)
{
    DebugLog(L"Using MiniUPnPLib based implementation");
    m_nOldUDPPort = (ArePortsForwarded() == TRIS_TRUE) ? m_nUDPPort : 0;
    m_nUDPPort = nUDPPort;
    m_nOldTCPPort = (ArePortsForwarded() == TRIS_TRUE) ? m_nTCPPort : 0;
    m_nTCPPort = nTCPPort;
    m_nOldTCPWebPort = (ArePortsForwarded() == TRIS_TRUE) ? m_nTCPWebPort : 0;
    m_nTCPWebPort = nTCPWebPort;
    m_bUPnPPortsForwarded = TRIS_UNKNOWN;
    m_bCheckAndRefresh = false;

    if (m_pURLs != NULL)
        FreeUPNPUrls(m_pURLs);
    delete m_pURLs;
    m_pURLs = NULL;
    delete m_pIGDData;
    m_pIGDData = NULL;

    if (m_bAbortDiscovery)
        return;

    StartThread();
}

bool CUPnPImplMiniLib::CheckAndRefresh()
{
    // on a CheckAndRefresh we don't do any new time consuming discovery tries, we expect to find the same router like the first time
    // and of course we also don't delete old ports (this was done on Discovery) but only check that our current mappings still exist
    // and refresh them if not
    if (m_bAbortDiscovery || !m_bSucceededOnce || m_pURLs == NULL || m_pIGDData == NULL || m_pURLs->controlURL == NULL || m_nTCPPort == 0)
    {
        DebugLog(L"Not refreshing UPnP ports because they don't seem to be forwarded in the first place");
        return false;
    }
//>>> WiZaRd
    else if (!IsReady())
    {
        DebugLog(L"Not refreshing UPnP ports because they are already in the process of being refreshed");
        return false;
    }
//<<< WiZaRd

    DebugLog(L"Checking and refreshing UPnP ports");
    m_bCheckAndRefresh = true;
    StartThread();
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CUPnPImplMiniLib::CStartDiscoveryThread Implementation
typedef CUPnPImplMiniLib::CStartDiscoveryThread CStartDiscoveryThread;
IMPLEMENT_DYNCREATE(CStartDiscoveryThread, CWinThread)

CUPnPImplMiniLib::CStartDiscoveryThread::CStartDiscoveryThread()
{
    m_pOwner = NULL;
}

BOOL CUPnPImplMiniLib::CStartDiscoveryThread::InitInstance()
{
    DbgSetThreadName("CUPnPImplMiniLib::CStartDiscoveryThread");
    InitThreadLocale();
    return TRUE;
}

int CUPnPImplMiniLib::CStartDiscoveryThread::Run()
{
    if (!m_pOwner)
        return 0;

    CSingleLock sLock(&m_pOwner->m_mutBusy);
    if (!sLock.Lock(0))
    {
        DebugLogWarning(L"CUPnPImplMiniLib::CStartDiscoveryThread::Run, failed to acquire Lock, another Mapping try might be running already");
        return 0;
    }

    if (m_pOwner->m_bAbortDiscovery) // requesting to abort ASAP?
        return 0;

    bool bSucceeded = false;
#if !(defined(_DEBUG) || defined(_BETA))
    try
#endif
    {
        if (!m_pOwner->m_bCheckAndRefresh)
        {
            int error = 0;
            UPNPDev* structDeviceList = upnpDiscover(2000, NULL, NULL, 0, 0, 2, &error);
            if (structDeviceList == NULL)
            {
                DebugLog(L"UPNP: No Internet Gateway Devices found, aborting");
                m_pOwner->m_bUPnPPortsForwarded = TRIS_FALSE;
                m_pOwner->SendResultMessage();
                return 0;
            }

            if (m_pOwner->m_bAbortDiscovery) // requesting to abort ASAP?
            {
                freeUPNPDevlist(structDeviceList);
                return 0;
            }

            DebugLog(L"List of UPNP devices found on the network:");
            for (UPNPDev* pDevice = structDeviceList; pDevice != NULL; pDevice = pDevice->pNext)
            {
                DebugLog(L"Desc: %S, st: %S", pDevice->descURL, pDevice->st);
            }
            m_pOwner->m_pURLs = new UPNPUrls;
            ZeroMemory(m_pOwner->m_pURLs, sizeof(UPNPUrls));
            m_pOwner->m_pIGDData = new IGDdatas;
            ZeroMemory(m_pOwner->m_pIGDData, sizeof(IGDdatas));

            m_pOwner->m_achLanIP[0] = 0;
            int iResult = UPNP_GetValidIGD(structDeviceList, m_pOwner->m_pURLs, m_pOwner->m_pIGDData, m_pOwner->m_achLanIP, sizeof(m_pOwner->m_achLanIP));
            freeUPNPDevlist(structDeviceList);
            bool bNotFound = false;
            switch (iResult)
            {
                case 1:
                    DebugLog(L"Found valid IGD : %S", m_pOwner->m_pURLs->controlURL);
                    break;
                case 2:
                    DebugLog(L"Found a (not connected?) IGD : %S - Trying to continue anyway", m_pOwner->m_pURLs->controlURL);
                    break;
                case 3:
                    DebugLog(L"UPnP device found. Is it an IGD ? : %S - Trying to continue anyway", m_pOwner->m_pURLs->controlURL);
                    break;
                default:
                    DebugLog(L"Found device (igd ?) : %S - Aborting", m_pOwner->m_pURLs->controlURL != NULL ? m_pOwner->m_pURLs->controlURL : "(none)");
                    bNotFound = true;
            }
            if (bNotFound || m_pOwner->m_pURLs->controlURL == NULL)
            {
                m_pOwner->m_bUPnPPortsForwarded = TRIS_FALSE;
                m_pOwner->SendResultMessage();
                return 0;
            }
            DebugLog(L"Our LAN IP: %S", m_pOwner->m_achLanIP);

            if (m_pOwner->m_bAbortDiscovery) // requesting to abort ASAP?
                return 0;

            // do we still have old mappings? Remove them first
            m_pOwner->DeletePorts(true);
        }

        bSucceeded = OpenPort(m_pOwner->m_nTCPPort, true, m_pOwner->m_achLanIP, m_pOwner->m_bCheckAndRefresh);
        if (bSucceeded && m_pOwner->m_nUDPPort != 0)
            bSucceeded = OpenPort(m_pOwner->m_nUDPPort, false, m_pOwner->m_achLanIP, m_pOwner->m_bCheckAndRefresh);
        if (bSucceeded && m_pOwner->m_nTCPWebPort != 0)
            OpenPort(m_pOwner->m_nTCPWebPort, true, m_pOwner->m_achLanIP, m_pOwner->m_bCheckAndRefresh); // don't fail if only the webinterface port fails for some reason
    }
#if !(defined(_DEBUG) || defined(_BETA))
    catch (...)
    {
        DebugLogError(L"Unknown Exception in CUPnPImplMiniLib::CStartDiscoveryThread::Run()");
    }
#endif
    if (!m_pOwner->m_bAbortDiscovery) // dont send a result on a abort request
    {
        m_pOwner->m_bUPnPPortsForwarded = bSucceeded ? TRIS_TRUE : TRIS_FALSE;
        m_pOwner->m_bSucceededOnce |= bSucceeded;
        m_pOwner->SendResultMessage();
    }
    return 0;
}

bool CUPnPImplMiniLib::CStartDiscoveryThread::OpenPort(uint16 nPort, bool bTCP, char* pachLANIP, bool bCheckAndRefresh)
{
    const char achTCP[] = "TCP";
    const char achUDP[] = "UDP";
    const char achDescTCP[] = "eMule_TCP";
    const char achDescUDP[] = "eMule_UDP";
    char achPort[10];
    sprintf(achPort, "%u", nPort);

    if (m_pOwner->m_bAbortDiscovery)
        return false;

    int nResult;

    // if we are refreshing ports, check first if the mapping is still fine and only try to open if not
    char achOutIP[20];
    char achOutPort[6]; // see upnpcommands.h
    char achOutDesc[80];
    char achOutEnabled[4];
    char achOutLeaseDuration[16];
    achOutPort[0] = 0;
    if (bCheckAndRefresh)
    {
        achOutIP[0] = 0;
        if (bTCP)
            nResult = UPNP_GetSpecificPortMappingEntry(m_pOwner->m_pURLs->controlURL, m_pOwner->m_pIGDData->first.servicetype
                      , achPort, achTCP, NULL/*remoteHost*/, achOutIP, achOutPort, achOutDesc, achOutEnabled, achOutLeaseDuration);
        else
            nResult = UPNP_GetSpecificPortMappingEntry(m_pOwner->m_pURLs->controlURL, m_pOwner->m_pIGDData->first.servicetype
                      , achPort, achUDP, NULL/*remoteHost*/, achOutIP, achOutPort, achOutDesc, achOutEnabled, achOutLeaseDuration);

        if (nResult == UPNPCOMMAND_SUCCESS && achOutIP[0] != 0)
        {
            DebugLog(L"Checking UPnP: Mapping for port %u (%s) on local IP %S still exists", nPort, bTCP ? L"TCP" : L"UDP", achOutIP);
            return true;
        }
        else
            DebugLogWarning(L"Checking UPnP: Mapping for port %u (%s) on local IP %S is gone, trying to reopen port", nPort, bTCP ? L"TCP" : L"UDP", achOutIP);
    }

    if (bTCP)
        nResult = UPNP_AddPortMapping(m_pOwner->m_pURLs->controlURL, m_pOwner->m_pIGDData->first.servicetype
                                      , achPort, achPort, pachLANIP, achDescTCP, achTCP, NULL, NULL);
    else
        nResult = UPNP_AddPortMapping(m_pOwner->m_pURLs->controlURL, m_pOwner->m_pIGDData->first.servicetype
                                      , achPort, achPort, pachLANIP, achDescUDP, achUDP, NULL, NULL);

    if (nResult != UPNPCOMMAND_SUCCESS)
    {
        DebugLog(L"Adding PortMapping failed, Error Code %u", nResult);
        return false;
    }

    if (m_pOwner->m_bAbortDiscovery)
        return false;

    // make sure it really worked
    achOutIP[0] = 0;
    if (bTCP)
        nResult = UPNP_GetSpecificPortMappingEntry(m_pOwner->m_pURLs->controlURL, m_pOwner->m_pIGDData->first.servicetype
                  , achPort, achTCP, NULL/*remoteHost*/, achOutIP, achOutPort, achOutDesc, achOutEnabled, achOutLeaseDuration);
    else
        nResult = UPNP_GetSpecificPortMappingEntry(m_pOwner->m_pURLs->controlURL, m_pOwner->m_pIGDData->first.servicetype
                  , achPort, achUDP, NULL/*remoteHost*/, achOutIP, achOutPort, achOutDesc, achOutEnabled, achOutLeaseDuration);

    if (nResult == UPNPCOMMAND_SUCCESS && achOutIP[0] != 0)
    {
        DebugLog(L"Successfully added mapping for port %u (%s) on local IP %S", nPort, bTCP ? L"TCP" : L"UDP", achOutIP);
        return true;
    }
    else
    {
        DebugLogWarning(L"Failed to verify mapping for port %u (%s) on local IP %S - considering as failed", nPort, bTCP ? L"TCP" : L"UDP", achOutIP);
        // maybe counting this as error is a bit harsh as this may lead to false negatives, however if we would risk false positives
        // this would mean that the fallback implementations are not tried because eMule thinks it worked out fine
        return false;
    }
}

void CUPnPImplMiniLib::StartThread()
{
    CStartDiscoveryThread* pStartDiscoveryThread = (CStartDiscoveryThread*) AfxBeginThread(RUNTIME_CLASS(CStartDiscoveryThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    m_hThreadHandle = pStartDiscoveryThread->m_hThread;
    pStartDiscoveryThread->SetValues(this);
    pStartDiscoveryThread->ResumeThread();
}