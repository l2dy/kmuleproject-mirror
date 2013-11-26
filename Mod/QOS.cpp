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
#include "QOS.h"
#include "types.h"
#include "Log.h"
#include "opcodes.h"
#include "otherfunctions.h"
#include "emule.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define DEBUG_QOS

CQOS theQOSManager;

CQOS::CQOS()
{
    m_qosHandle = NULL;
    m_qosFlowId = 0;

    // Try enable qWAVE API
    m_hQWAVE = ::LoadLibrary(L"Qwave.dll");
    if (m_hQWAVE != NULL)
    {
        m_pQOSCreateHandle = (t_QOSCreateHandle)::GetProcAddress(m_hQWAVE, "QOSCreateHandle");
        m_pQOSCloseHandle = (t_QOSCloseHandle)::GetProcAddress(m_hQWAVE, "QOSCloseHandle");
        m_pQOSAddSocketToFlow = (t_QOSAddSocketToFlow)::GetProcAddress(m_hQWAVE, "QOSAddSocketToFlow");
        m_pQOSRemoveSocketFromFlow = (t_QOSRemoveSocketFromFlow)::GetProcAddress(m_hQWAVE, "QOSRemoveSocketFromFlow");
        m_pQOSSetFlow = (t_QOSSetFlow) ::GetProcAddress(m_hQWAVE, "QOSSetFlow");
        if (m_pQOSCreateHandle == NULL || m_pQOSCloseHandle == NULL || m_pQOSAddSocketToFlow == NULL || m_pQOSRemoveSocketFromFlow == NULL || m_pQOSSetFlow == NULL)
        {
            ::FreeLibrary(m_hQWAVE);
            m_hQWAVE = NULL;
        }
        else
            Reinitialize();
    }
    ::SetCriticalSectionSpinCount(m_qosLocker, 4000); // To reduce task switching on multicore CPU's
}

CQOS::~CQOS()
{
    // just to be sure
    const DWORD error = RemoveSocket_internal(NULL);
#ifndef _DEBUG
	(void)error;
#else
    if (m_qosSockets.IsEmpty())
        ASSERT(error == 0 || error == ERROR_NOT_FOUND);
    else
        ASSERT(error == 0);
#endif

    // Free qWAVE API
    if (m_qosHandle != NULL)
        m_pQOSCloseHandle(m_qosHandle);
    if (m_hQWAVE != NULL)
        ::FreeLibrary(m_hQWAVE);
}

BOOL CQOS::Reinitialize()
{
    BOOL success = FALSE;

    CSingleLock lock(&m_qosLocker, TRUE);

    // Drop old flow
    if (m_qosHandle != NULL)
    {
        const DWORD error = RemoveSocket_internal(NULL);
#ifndef _DEBUG
		(void)error;
#else
        if (m_qosSockets.IsEmpty())
            ASSERT(error == 0 || error == ERROR_NOT_FOUND);
        else
            ASSERT(error == 0);
#endif
        m_pQOSCloseHandle(m_qosHandle);
        m_qosHandle = NULL;
    }

    // Create a new flow
    m_qosFlowId = 0;
    QOS_VERSION	qosVersion = {1, 0};
    m_pQOSCreateHandle(&qosVersion, &m_qosHandle);
    m_qosLastInit = clock();

    if (m_qosHandle != NULL)
    {
        success = TRUE;

        // Add back sockets to the new flow
        for (POSITION pos = m_qosSockets.GetHeadPosition(); pos != 0;)
        {
            POSITION posLast = pos;
            SOCKET	socket = m_qosSockets.GetNext(pos);
            const DWORD error = AddSocket_internal(socket, NULL);
            if (error != 0)
            {
                if (error == ERROR_NOT_FOUND)
                    m_qosSockets.RemoveAt(posLast);
                else
                    success = FALSE;
            }
        }

        // Update flow with the wanted datarate
        const DWORD datarate = m_qosDatarate;
        SetDataRate_internal(1);
        SetDataRate_internal(datarate);
    }

    return success;
}

BOOL CQOS::AddSocket(SOCKET socket, const SOCKADDR* const dest)
{
    BOOL success = FALSE;

    if (m_qosHandle != NULL && socket != INVALID_SOCKET)
    {
        success = TRUE;
        CSingleLock lock(&m_qosLocker, TRUE);

        const DWORD error = AddSocket_internal(socket, dest);
        if (error != 0)
        {
            theApp.QueueDebugLogLineEx(LOG_ERROR, L"QOS: QOSAddSocket: Error %d (%s).", error, GetErrorMessage(error, 0));
            if (clock() - m_qosLastInit < SEC2MS(5))
                success = FALSE;
            else if (error == ERROR_SYSTEM_POWERSTATE_TRANSITION
                     || error == ERROR_SYSTEM_POWERSTATE_COMPLEX_TRANSITION
                     || error == ERROR_DEVICE_REINITIALIZATION_NEEDED
                     || error == ERROR_NOT_FOUND/*(error == ERROR_NOT_FOUND && m_qosSockets.IsEmpty())*/)
            {
                lock.Unlock();
                if (!Reinitialize())
                {
                    theApp.QueueDebugLogLineEx(LOG_ERROR, L"QOS: QOSAddSocket: Reinitialization was unsuccessful.");
                    success = FALSE;
                }
                else
                {
                    theApp.QueueDebugLogLineEx(LOG_WARNING, L"QOS: QOSAddSocket: Reinitialized!");
                    success = AddSocket(socket, dest);
                }
            }
            else
            {
                RemoveSocket_internal(socket);
                success = FALSE;
            }
        }
    }

    return success;
}

BOOL CQOS::RemoveSocket(SOCKET socket)
{
    BOOL success = FALSE;

    if (m_qosHandle != NULL && socket != INVALID_SOCKET)
    {
        success = TRUE;
        CSingleLock lock(&m_qosLocker, TRUE);

        const DWORD error = RemoveSocket_internal(socket);
        if (error != 0)
        {
            theApp.QueueDebugLogLineEx(LOG_ERROR, L"QOS: QOSRemoveSocket: Error %d (%s).", error, GetErrorMessage(error, 0));
            if (error == ERROR_NOT_FOUND)
            {
                ASSERT(0);
                success = TRUE; // not found? removed already!
            }
            else if (clock() - m_qosLastInit < SEC2MS(5))
                success = FALSE;
            else if (error == ERROR_SYSTEM_POWERSTATE_TRANSITION
                     || error == ERROR_SYSTEM_POWERSTATE_COMPLEX_TRANSITION
                     || error == ERROR_DEVICE_REINITIALIZATION_NEEDED
                     /*|| error == ERROR_NOT_FOUND*/)
            {
                lock.Unlock();
                if (!Reinitialize())
                {
                    theApp.QueueDebugLogLineEx(LOG_ERROR, L"QOS: QOSRemoveSocket: Reinitialization was unsuccessful.");;
                    success = FALSE;
                }
                else
                    theApp.QueueDebugLogLineEx(LOG_WARNING, L"QOS: QOSRemoveSocket: Reinitialized!");
            }
            else
                success = FALSE;
        }
    }

    return success;
}

BOOL CQOS::SetDataRate(DWORD const datarate)
{
    BOOL success = FALSE;

    if (m_qosHandle != NULL)
    {
        if (m_qosDatarate == datarate)
            success = TRUE;
        else
        {
            success = TRUE;

            CSingleLock lock(&m_qosLocker, TRUE);

            const DWORD error = SetDataRate_internal(datarate);
            if (error != 0)
            {
                theApp.QueueDebugLogLineEx(LOG_ERROR, L"QOS: QOSSetDataRate: Error %d (%s).", error, GetErrorMessage(error, 0));
                if (clock() - m_qosLastInit < SEC2MS(5))
                    success = FALSE;
                else if (error == ERROR_SYSTEM_POWERSTATE_TRANSITION
                         || error == ERROR_SYSTEM_POWERSTATE_COMPLEX_TRANSITION
                         || error == ERROR_DEVICE_REINITIALIZATION_NEEDED
                         || error == ERROR_NOT_FOUND)
                {
                    lock.Unlock();
                    if (!Reinitialize())
                    {
                        theApp.QueueDebugLogLineEx(LOG_ERROR, L"QOS: QOSSetDataRate: Reinitialization was unsuccessful.");
                        success = FALSE;
                    }
                    else
                        theApp.QueueDebugLogLineEx(LOG_WARNING, L"QOS: QOSSetDataRate: Reinitialized!");
                }
                else
                    success = FALSE;
            }
        }
    }

    return success;
}

DWORD CQOS::AddSocket_internal(SOCKET const socket, const SOCKADDR* const dest)
{
    DWORD result = 0;

    POSITION posFind = m_qosSockets.Find(socket);
#ifdef DEBUG_QOS
    theApp.QueueDebugLogLineEx(LOG_WARNING, L"** QOS: adding socket %i (%s:%u) to %u", (int)socket, ipstr(dest ? ((SOCKADDR_IN*)(dest))->sin_addr.s_addr : 0), dest ? ((SOCKADDR_IN*)(dest))->sin_port : 0, m_qosFlowId);
#endif
    if (posFind == 0) // don't "double-add"
    {
        if (m_pQOSAddSocketToFlow(m_qosHandle, socket, (PSOCKADDR)dest, QOSTrafficTypeBackground, QOS_NON_ADAPTIVE_FLOW, &m_qosFlowId) == 0)
            result = GetLastError();
        if (result == 0)
            m_qosSockets.AddTail(socket);
#ifdef DEBUG_QOS
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"** QOS: result %u - flow %u", result, m_qosFlowId);
#endif
    }
#ifdef DEBUG_QOS
    else
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"** QOS: prevented double-add");
#endif

    return result;
}

DWORD CQOS::RemoveSocket_internal(SOCKET const socket)
{
    DWORD result = 0;

    POSITION posFind = m_qosSockets.Find(socket);
#ifdef DEBUG_QOS
    theApp.QueueDebugLogLineEx(LOG_WARNING, L"** QOS: removing socket %i from flow %u", (int)socket, m_qosFlowId);
#endif
    if (posFind || socket == NULL)
    {
        if (socket != NULL)
            m_qosSockets.RemoveAt(posFind);
        if (m_pQOSRemoveSocketFromFlow(m_qosHandle, socket, m_qosFlowId, 0) == 0)
            result = GetLastError();

        // obviously, an empty flow leads to ERROR_NOT_FOUND when adding another socket
        // thus, we reset the flow ID if necessary
#ifdef DEBUG_QOS
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"** QOS: result %u", result);
#endif
        if (m_qosSockets.IsEmpty())
        {
#ifdef DEBUG_QOS
            theApp.QueueDebugLogLineEx(LOG_WARNING, L"** QOS: flow empty - resetting flowID");
#endif
            m_qosFlowId = 0;
        }
    }
#ifdef DEBUG_QOS
    else
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"** QOS: not found!?");
#endif

    return result;
}

DWORD CQOS::SetDataRate_internal(DWORD datarate)
{
    DWORD result = 0;
    if (m_qosFlowId != 0)
    {
        m_qosDatarate = datarate;
        // Make sure flowrate is atleast 1 kB/s
        if (datarate < 1024)
            datarate = 1024;
        // Make flowrate 110% of eMules internal upload speed to give room for IP headers
        QOS_FLOWRATE_OUTGOING	qosFlowRate = {(static_cast<UINT64>(datarate) * 8ULL * 110ULL) / 100ULL, /*QOSShapeOnly*/ QOSShapeAndMark};
        QOS_TRAFFIC_TYPE		qosTrafficType = QOSTrafficTypeBackground;
        if (m_pQOSSetFlow(m_qosHandle, m_qosFlowId, QOSSetTrafficType, sizeof(QOS_TRAFFIC_TYPE), &qosTrafficType, 0, NULL) == 0)
            result = GetLastError();
        else if (m_pQOSSetFlow(m_qosHandle, m_qosFlowId, QOSSetOutgoingRate, sizeof(QOS_FLOWRATE_OUTGOING), &qosFlowRate, 0, NULL) == 0)
            result = GetLastError();
    }
    return result;
}