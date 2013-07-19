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

CQOS theQOSManager;

CQOS::CQOS()
{
	// Try enable qWAVE API
	QOS_VERSION	qosVersion = {1,0};
	m_qosHandle = NULL;
	m_qosFlowId = 0;
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
		{
			m_pQOSCreateHandle(&qosVersion, &m_qosHandle);
			m_qosLastInit = clock();
		}
	}
	::SetCriticalSectionSpinCount(m_qosLocker, 4000); // To reduce task switching on multicore CPU's
}

CQOS::~CQOS()
{
	// Free qWAVE API
	if (m_qosHandle != NULL)
		m_pQOSCloseHandle(m_qosHandle);
	if (m_hQWAVE != NULL)
		::FreeLibrary(m_hQWAVE);
}

BOOL CQOS::Reinitialize()
{
	BOOL success = FALSE;

	if (m_hQWAVE != NULL)
	{
		CSingleLock lock(&m_qosLocker, TRUE);
		m_qosLastInit = clock();

		// Drop old flow
		if (m_qosHandle != NULL)
		{
			if (m_qosFlowId != 0)
				m_pQOSRemoveSocketFromFlow(m_qosHandle, NULL, m_qosFlowId, 0);
			m_pQOSCloseHandle(m_qosHandle);
		}

		// Create a new flow
		QOS_VERSION	qosVersion = {1,0};
		m_qosHandle = NULL;
		m_qosFlowId = 0;
		m_pQOSCreateHandle(&qosVersion, &m_qosHandle);

		if (m_qosHandle != NULL)
		{
			success = TRUE;

			// Add back sockets to the new flow
			for (POSITION pos = m_qosSockets.GetStartPosition(); pos != 0;)
			{
				SOCKET	socket;
				DWORD	temp;
				m_qosSockets.GetNextAssoc(pos, socket, temp);
				const DWORD error = AddSocket_internal(socket, NULL);
				if (error != 0)
				{
					if (error == ERROR_NOT_FOUND)
						m_qosSockets.RemoveKey(socket);
					else
						success = FALSE;
				}
			}

			// Update flow with the wanted datarate
			const DWORD datarate = m_qosDatarate;
			SetDataRate_internal(1);
			SetDataRate_internal(datarate);
		}
	}

	return FALSE;
}

BOOL CQOS::AddSocket(SOCKET socket, const SOCKADDR* const dest)
{
	BOOL success = FALSE;

	if (m_qosHandle != NULL && socket != INVALID_SOCKET)
	{
		success = TRUE;
		CSingleLock lock(&m_qosLocker, TRUE);

		const DWORD error = AddSocket_internal(socket, dest);
		if (error == 0)
			m_qosSockets.SetAt(socket, 1);
		else if (error == ERROR_SYSTEM_POWERSTATE_TRANSITION 
				|| error == ERROR_SYSTEM_POWERSTATE_COMPLEX_TRANSITION 
				|| error == ERROR_DEVICE_REINITIALIZATION_NEEDED 
				|| (error == ERROR_NOT_FOUND && m_qosSockets.IsEmpty()))
		{
			if (clock() - m_qosLastInit < SEC2MS(5))
				success = FALSE;
			else
			{
				DebugLogError(L"QOSAddSocket: Error %d (%s).", error, GetErrorMessage(error, 0));
				if(!Reinitialize())
				{
					DebugLogError(L"QOSAddSocket: Reinitialization was unsuccessful.");
					success = FALSE;
				}
				else
				{
					DebugLog(L"QOSAddSocket: Reinitialized!");
					success = AddSocket(socket, dest);
				}
			}
		}
		else
		{
			if (error != ERROR_NOT_FOUND)
				DebugLogError(L"QOSAddSocket: Error %d (%s).", error, GetErrorMessage(error, 0));
			m_qosSockets.RemoveKey(socket);
			success = FALSE;
		}
	}

	return success;
}

BOOL CQOS::RemoveSocket(SOCKET socket)
{
	BOOL success = FALSE;

	if (m_qosHandle != NULL && socket != INVALID_SOCKET)
	{
		CSingleLock lock(&m_qosLocker, TRUE);

		if (m_qosSockets.RemoveKey(socket))
		{
			success = TRUE;

			if (m_pQOSRemoveSocketFromFlow(m_qosHandle, socket, m_qosFlowId, 0) == 0)
			{
				const DWORD error = GetLastError();
				DebugLogError(L"QOSRemoveSocket: Error %d (%s).", error, GetErrorMessage(error, 0));
				if (clock() - m_qosLastInit < SEC2MS(5))
					success = FALSE;
				else if (error == ERROR_SYSTEM_POWERSTATE_TRANSITION || error == ERROR_SYSTEM_POWERSTATE_COMPLEX_TRANSITION || error == ERROR_DEVICE_REINITIALIZATION_NEEDED || error == ERROR_NOT_FOUND)
				{
					if(!Reinitialize())
					{
						DebugLogError(L"QOSRemoveSocket: Reinitialization was unsuccessful.");;
						success = FALSE;
					}
					else
						DebugLog(L"QOSRemoveSocket: Reinitialized!");
				}
				else
				{
					success = FALSE;
				}
			}
		}
	}

	return success;
}

BOOL CQOS::SetDataRate(DWORD const datarate)
{
	if (m_qosHandle == NULL)
		return FALSE;

	if (m_qosDatarate == datarate)
		return TRUE;

	CSingleLock lock(&m_qosLocker, TRUE);

	const DWORD error = SetDataRate_internal(datarate);
	if (error != 0)
	{
		DebugLogError(L"QOSSetDataRate: Error %d (%s).", error, GetErrorMessage(error, 0));
		if (clock() - m_qosLastInit < SEC2MS(5))
		{
			return FALSE;
		}
		else if (error == ERROR_SYSTEM_POWERSTATE_TRANSITION || error == ERROR_SYSTEM_POWERSTATE_COMPLEX_TRANSITION || error == ERROR_DEVICE_REINITIALIZATION_NEEDED || error == ERROR_NOT_FOUND)
		{
			if(!Reinitialize())
			{
				DebugLogError(L"QOSSetDataRate: Reinitialization was unsuccessful.");
				return FALSE;
			}
			else
				DebugLog(L"QOSSetDataRate: Reinitialized!");
		}
		else
		{
			return FALSE;
		}
	}
	return TRUE;
}

DWORD CQOS::AddSocket_internal(SOCKET const socket, const SOCKADDR* const dest)
{
	if (m_pQOSAddSocketToFlow(m_qosHandle, socket, const_cast<PSOCKADDR>(dest), QOSTrafficTypeBackground, QOS_NON_ADAPTIVE_FLOW, &m_qosFlowId) == 0)
		return GetLastError();
	return 0;
}

DWORD CQOS::SetDataRate_internal(DWORD datarate)
{
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
		{
			return GetLastError();
		}
		if (m_pQOSSetFlow(m_qosHandle, m_qosFlowId, QOSSetOutgoingRate, sizeof(QOS_FLOWRATE_OUTGOING), &qosFlowRate, 0, NULL) == 0)
		{
			return GetLastError();
		}
	}
	return 0;
}