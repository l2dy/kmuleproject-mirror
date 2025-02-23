/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the official client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#pragma once
#include "../utils/UInt128.h"

namespace Kademlia
{
struct externPortReply
{
    externPortReply(const UINT IP = 0, const uint16 Port = 0)
    {
        dwIP = IP;
        nPort = Port;
    }
    UINT	dwIP;
    uint16	nPort;
};

class CPrefs
{
  public:
    CPrefs();
    ~CPrefs();

    void		GetKadID(CUInt128 *puID) const;
    void		GetKadID(CString *psID) const;
    void		SetKadID(const CUInt128 &puID);
    CUInt128	GetKadID() const;
    void		GetClientHash(CUInt128 *puID) const;
    void		GetClientHash(CString *psID) const;
    void		SetClientHash(const CUInt128 &puID);
    CUInt128	GetClientHash() const;
    UINT		GetIPAddress() const;
    void		SetIPAddress(UINT uVal);
    bool		GetRecheckIP() const;
    void		SetRecheckIP();
    void		IncRecheckIP();
    bool		HasHadContact() const;
    void		SetLastContact();
    bool		HasLostConnection() const;
    UINT		GetLastContact() const;
    bool		GetFirewalled() const;
    void		SetFirewalled();
    void		IncFirewalled();

    uint8		GetTotalFile() const;
    void		SetTotalFile(uint8 uVal);
    uint8		GetTotalStoreSrc() const;
    void		SetTotalStoreSrc(uint8 uVal);
    uint8		GetTotalStoreKey() const;
    void		SetTotalStoreKey(uint8 uVal);
    uint8		GetTotalSource() const;
    void		SetTotalSource(uint8 uVal);
    uint8		GetTotalNotes() const;
    void		SetTotalNotes(uint8 uVal);
    uint8		GetTotalStoreNotes() const;
    void		SetTotalStoreNotes(uint8 uVal);
    UINT		GetKademliaUsers() const;
    void		SetKademliaUsers(UINT uVal);
    UINT		GetKademliaFiles() const;
    void		SetKademliaFiles();
    bool		GetPublish() const;
    void		SetPublish(bool bVal);
    bool		GetFindBuddy();
    void		SetFindBuddy(bool bVal = true);
    bool		GetUseExternKadPort() const;
    void		SetUseExternKadPort(bool bVal);
    uint16		GetExternalKadPort() const;
    void		SetExternKadPort(uint16 uVal, UINT nFromIP);
    bool		FindExternKadPort(bool bReset = false);
    uint16		GetInternKadPort() const;
    void		StatsIncUDPFirewalledNodes(bool bFirewalled);
    void		StatsIncTCPFirewalledNodes(bool bFirewalled);
    float		StatsGetFirewalledRatio(bool bUDP) const;
    float		StatsGetKadV8Ratio();
    bool		GetLastFirewalledState() const		{return m_bLastFirewallState;}
    bool		GetCurrentFirewalledState() const	{return m_uFirewalled < 2;}

    static UINT GetUDPVerifyKey(UINT dwTargetIP);
  private:
    void Init(LPCTSTR szFilename);
    void Reset();
    void SetDefaults();
    void ReadFile();
    void WriteFile();
    CString	m_sFilename;
    time_t m_tLastContact;
    CUInt128 m_uClientID;
    CUInt128 m_uClientHash;
    UINT m_uIP;
    UINT m_uIPLast;
    UINT m_uRecheckip;
    UINT m_uFirewalled;
    UINT m_uKademliaUsers;
    UINT m_uKademliaFiles;
    uint8 m_uTotalFile;
    uint8 m_uTotalStoreSrc;
    uint8 m_uTotalStoreKey;
    uint8 m_uTotalSource;
    uint8 m_uTotalNotes;
    uint8 m_uTotalStoreNotes;
    bool m_bPublish;
    bool m_bFindBuddy;
    bool m_bLastFirewallState;
    bool m_bUseExternKadPort;
    uint16 m_nExternKadPort;
    CArray<externPortReply> m_anExternPortReplies;
    UINT m_nStatsUDPOpenNodes;
    UINT m_nStatsUDPFirewalledNodes;
    UINT m_nStatsTCPOpenNodes;
    UINT m_nStatsTCPFirewalledNodes;
    time_t m_nStatsKadV8LastChecked;
    float  m_fKadV8Ratio;
};
}
