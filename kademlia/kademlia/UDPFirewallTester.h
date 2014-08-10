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
#include "../utils/UInt128.h"
#include "../utils/KadUDPKey.h"
#include "../routing/Contact.h"

namespace Kademlia
{
struct UsedClient_Struct
{
    CContact	contact;
    bool		bAnswered;
};
class CUDPFirewallTester
{

  public:
    static bool		IsFirewalledUDP(const bool bLastStateIfTesting); // Are we UDP firewalled - if unknown open is assumed unless bOnlyVerified == true
    static void		SetUDPFWCheckResult(const bool bSucceeded, const bool bTestCancelled, const UINT uFromIP, const uint16 nIncomingPort);
    static bool		ReCheckFirewallUDP(const bool bSetUnverified);
    static bool		IsFWCheckUDPRunning();
    static bool		IsVerified();
    static void		AddPossibleTestContact(const CUInt128 &uClientID, const UINT uIp, const uint16 uUdpPort, const uint16 uTcpPort, const CUInt128 &uTarget, const uint8 uVersion, const CKadUDPKey& cUDPKey, const bool bIPVerified);
    static void		Reset(); // when stopping Kad
    static void		Connected();
    static void		QueryNextClient(); // try the next available client for the firewallcheck
  private:
    static uint8	GetRunningFWChecks();
    static bool		GetUDPCheckClientsNeeded(); // are we in search for testclients
    static bool		m_bFirewalledUDP;
    static bool		m_bFirewalledLastStateUDP;
    static bool		m_bIsFWVerifiedUDP;
    static bool		m_bNodeSearchStarted;
    static bool		m_bTimedOut;
    static uint8	m_byFWChecksRunningUDP;
    static uint8	m_byFWChecksFinishedUDP;
    static UINT	m_dwTestStart;
    static UINT	m_dwLastSucceededTime;
    static CList<CContact> m_liPossibleTestClients;
    static CList<UsedClient_Struct> m_liUsedTestClients;
};
}