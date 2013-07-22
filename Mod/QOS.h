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
#pragma once

// Class of service (eg. DIFFSERV)
#include <Qos2.h>

typedef BOOL (WINAPI *t_QOSCreateHandle)(PQOS_VERSION Version, PHANDLE QOSHandle);
typedef BOOL (WINAPI *t_QOSCloseHandle)(HANDLE QOSHandle);
typedef BOOL (WINAPI *t_QOSAddSocketToFlow)(HANDLE QOSHandle, SOCKET Socket, PSOCKADDR DestAddr, QOS_TRAFFIC_TYPE TrafficType, DWORD Flags, PQOS_FLOWID FlowId);
typedef BOOL (WINAPI *t_QOSRemoveSocketFromFlow)(HANDLE QOSHandle, SOCKET Socket, QOS_FLOWID FlowId, DWORD Flags);
typedef BOOL (WINAPI *t_QOSSetFlow)(HANDLE QOSHandle, QOS_FLOWID FlowId, QOS_SET_FLOW Operation, ULONG Size, PVOID Buffer, DWORD Flags, LPOVERLAPPED Overlapped);

// Adaption of Netfinitys' QOS implementation from NetF WARP
class CQOS
{
public:
	CQOS();
	virtual ~CQOS();

	BOOL				Reinitialize();
	BOOL				AddSocket(SOCKET socket, const SOCKADDR* dest);
	BOOL				RemoveSocket(SOCKET socket);
	BOOL				SetDataRate(DWORD datarate);

private:
	DWORD				AddSocket_internal(SOCKET socket, const SOCKADDR* dest);
	DWORD				RemoveSocket_internal(SOCKET socket);
	DWORD				SetDataRate_internal(DWORD datarate);

	HMODULE				m_hQWAVE;
	HANDLE				m_qosHandle;
	QOS_FLOWID			m_qosFlowId;
	DWORD				m_qosDatarate;
	CList<SOCKET>		m_qosSockets;
	clock_t				m_qosLastInit;
	CCriticalSection	m_qosLocker;

	t_QOSCreateHandle	m_pQOSCreateHandle;
	t_QOSCloseHandle	m_pQOSCloseHandle;
	t_QOSAddSocketToFlow	m_pQOSAddSocketToFlow;
	t_QOSRemoveSocketFromFlow	m_pQOSRemoveSocketFromFlow;
	t_QOSSetFlow		m_pQOSSetFlow;
};

extern CQOS theQOSManager;