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
#define ENABLE_STRNATPMPERR
#include ".\libnatpmp\natpmp.h"

#define LIBNAT_VERSION_STRING	L"20120821"	//v1.3
#define DFLT_NATPMP_LIFETIME	3600
#define NATPMP_REFRESH_TIME		(DFLT_NATPMP_LIFETIME - 100)

class CNATPMPThreadWrapper;
class CNATPMPThread;
class CNATPMP;

static CNATPMP* m_pNATPMP = NULL;

enum ENATPMPTask
{
    eInitializeNATPMP = 0,
    eRetrievePublicIP,
    eMapTCPPorts,
    eMapUDPPorts,
    eDeleteTCPMappings,
    eDeleteUDPMappings,
};

#ifdef _DEBUG
#define _ENATPMPTask	ENATPMPTask
#else
#define _ENATPMPTask	uint8
#endif

class CNATPMPThreadWrapper
{
    friend class CNATPMPThread;

  public:
    CNATPMPThreadWrapper();
    virtual ~CNATPMPThreadWrapper();

    bool		IsReady();
    void		StopAsyncFind();
    void		DeletePorts();
    void		StartDiscovery(uint16 nTCPPort, uint16 nUDPPort);
    void		SetMessageWanted(const bool bWanted);
    bool		CheckAndRefresh();
    void		AddTask(_ENATPMPTask task, const bool bLock = true);
    void		RemovePendingTasks();
    void		StartRefreshTimer();
    void		KillRefreshTimer();

  private:
    void		StartThread();
    void		AddPort(const uint16 port, const int protocol, const bool bLock = true);
    static VOID CALLBACK RefreshTimer(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime);
    HANDLE		m_hThreadHandle;

    UINT_PTR	m_hRefreshTimer;
    CList<uint16>	m_portList[2];
    CList<ENATPMPTask>	m_taskList;
    bool			m_bMessageWanted;
    CCriticalSection	m_settingsLock;
    CMutex			m_mutBusy;
    volatile bool	m_bAbortDiscovery;
};

class CNATPMPThread : public CWinThread
{
    DECLARE_DYNCREATE(CNATPMPThread)
  protected:
    CNATPMPThread();

  public:
    virtual BOOL InitInstance();
    virtual int	Run();
    void	SetValues(CNATPMPThreadWrapper* pOwner)
    {
        m_pOwner = pOwner;
    }

  private:
    CNATPMPThreadWrapper*	m_pOwner;
};

class CNATPMP
{
    friend class CNATPMPThreadWrapper;
  public:
    CNATPMP();
    virtual ~CNATPMP();

    enum
    {
        NATPMP_OK,
        NATPMP_FAILED,
        NATPMP_TIMEOUT
    };

    bool		GetPublicIPAddress();
    bool		AddPortMapping(const uint16 publicPort, const uint16 privatePort, const int protocol, const UINT lifeTime);
    bool		RemovePortMapping(const uint16 publicPort, const uint16 privatePort, const int protocol);
    bool		RemovePortMappings(const int protocol);
    bool		RemoveAllPortMappings();
    bool		Reset();

  private:
    bool		Init();
    bool		UnInit();
    bool		PerformMapping(const uint16 publicPort, const uint16 privatePort, const int protocol, const UINT lifeTime);
    bool		PerformSelect();

    bool		m_bInitialised;
    natpmp_t	m_natpmp;
    int			m_ForceGW;
    in_addr_t	m_Gateway;
    in_addr		m_PublicIP;
};