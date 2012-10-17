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

namespace Kademlia
{
struct TrackPackets_Struct
{
    UINT dwIP;
    UINT dwInserted;
    uint8  byOpcode;
};

struct TrackChallenge_Struct
{
    UINT uIP;
    UINT dwInserted;
    uint8  byOpcode;
    CUInt128 uContactID;
    CUInt128 uChallenge;
};

struct TrackPacketsIn_Struct
{
    struct TrackedRequestIn_Struct
    {
        UINT	m_nCount;
        UINT	m_dwFirstAdded;
        uint8	m_byOpcode;
        bool	m_bDbgLogged;
    };

    TrackPacketsIn_Struct()
    {
        m_dwLastExpire = 0;
        m_uIP = 0;
    }

    UINT	m_uIP;
    UINT	m_dwLastExpire;
    CArray<TrackedRequestIn_Struct> m_aTrackedRequests;
};

class CPacketTracking
{
public:
    CPacketTracking();
    virtual ~CPacketTracking();

protected:
    void AddTrackedOutPacket(UINT dwIP, uint8 byOpcode);
    bool IsOnOutTrackList(UINT dwIP, uint8 byOpcode, bool bDontRemove = false);
    bool InTrackListIsAllowedPacket(UINT uIP, uint8 byOpcode, bool bValidReceiverkey);
    void InTrackListCleanup();
    void AddLegacyChallenge(CUInt128 uContactID, CUInt128 uChallengeID, UINT uIP, uint8 byOpcode);
    bool IsLegacyChallenge(CUInt128 uChallengeID, UINT uIP, uint8 byOpcode, CUInt128& ruContactID);
    bool HasActiveLegacyChallenge(UINT uIP) const;

private:
    bool IsTrackedOutListRequestPacket(uint8 byOpcode) const;
    CList<TrackPackets_Struct> listTrackedRequests;
    CList<TrackChallenge_Struct> listChallengeRequests;
    CTypedPtrList<CPtrList, TrackPacketsIn_Struct*>					m_liTrackPacketsIn;
    CMap<int, int, TrackPacketsIn_Struct*, TrackPacketsIn_Struct*>	m_mapTrackPacketsIn;
    UINT dwLastTrackInCleanup;
};
}