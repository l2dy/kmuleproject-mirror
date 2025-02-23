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
#include "StdAfx.h"
#include "corruptionblackbox.h"
#include "knownfile.h"
#include "updownclient.h"
#include "log.h"
#include "emule.h"
#include "clientlist.h"
#include "opcodes.h"
#include "./Mod/ClientAnalyzer.h" //>>> WiZaRd::ClientAnalyzer
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
#include "partfile.h" // netfinity: Add gaps for banned clients
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	 CBB_BANTHRESHOLD	32 //% max corrupted data	

CCBBRecord::CCBBRecord(uint64 nStartPos, uint64 nEndPos, UINT dwIP, EBBRStatus BBRStatus)
{
    if (nStartPos > nEndPos)
    {
        ASSERT(0);
        return;
    }
    m_nStartPos = nStartPos;
    m_nEndPos = nEndPos;
    m_dwIP = dwIP;
    m_BBRStatus = BBRStatus;
}

CCBBRecord& CCBBRecord::operator=(const CCBBRecord& cv)
{
    m_nStartPos = cv.m_nStartPos;
    m_nEndPos = cv.m_nEndPos;
    m_dwIP = cv.m_dwIP;
    m_BBRStatus = cv.m_BBRStatus;
    return *this;
}

bool CCBBRecord::Merge(uint64 nStartPos, uint64 nEndPos, UINT dwIP, EBBRStatus BBRStatus)
{

    if (m_dwIP == dwIP && m_BBRStatus == BBRStatus && (nStartPos == m_nEndPos + 1 || nEndPos + 1 == m_nStartPos))
    {
        if (nStartPos == m_nEndPos + 1)
            m_nEndPos = nEndPos;
        else if (nEndPos + 1 == m_nStartPos)
            m_nStartPos = nStartPos;
        else
            ASSERT(0);

        return true;
    }
    else
        return false;
}

bool CCBBRecord::CanMerge(uint64 nStartPos, uint64 nEndPos, UINT dwIP, EBBRStatus BBRStatus)
{

    if (m_dwIP == dwIP && m_BBRStatus == BBRStatus && (nStartPos == m_nEndPos + 1 || nEndPos + 1 == m_nStartPos))
    {
        return true;
    }
    else
        return false;
}

void CCorruptionBlackBox::Init(EMFileSize nFileSize)
{
    m_aaRecords.SetSize((INT_PTR)((uint64)(nFileSize + (uint64)(PARTSIZE - 1)) / (PARTSIZE)));
}

void CCorruptionBlackBox::Free()
{
    m_aaRecords.RemoveAll();
    m_aaRecords.FreeExtra();
}

void CCorruptionBlackBox::TransferredData(uint64 nStartPos, uint64 nEndPos, const CUpDownClient* pSender)
{
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    // netfinity: We might need to mark larger blocks than this sometimes when reloading a part file
    /*if (nEndPos - nStartPos >= PARTSIZE)
    {
        ASSERT(0);
        return;
    }*/
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (nStartPos > nEndPos)
    {
        ASSERT(0);
        return;
    }

    UINT dwSenderIP = 0;
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
    if (pSender && pSender->GetIP().GetType() == CAddress::IPv4) // IPv6-TODO: Add IPv6 ban list
        dwSenderIP = _ntohl(pSender->GetIP().ToIPv4());
//<<< WiZaRd::IPv6 [Xanatos]
#else
    if (pSender)
        dwSenderIP = pSender->GetIP();
#endif

    // we store records separated for each part, so we don't have to search all entries every time
    // convert pos to relative block pos
    UINT nPart = (UINT)(nStartPos / PARTSIZE);
    uint64 nRelStartPos = nStartPos - (uint64)nPart*PARTSIZE;
    uint64 nRelEndPos = nEndPos - (uint64)nPart*PARTSIZE;
    if (nRelEndPos >= PARTSIZE)
    {
        // data crosses the partborder, split it
        nRelEndPos = PARTSIZE-1;
        uint64 nTmpStartPos = (uint64)nPart*PARTSIZE + nRelEndPos + 1;
        ASSERT(nTmpStartPos % PARTSIZE == 0);  // remove later
        TransferredData(nTmpStartPos, nEndPos, pSender);
    }
    if (nPart >= (UINT)m_aaRecords.GetCount())
    {
        //ASSERT(0);
        m_aaRecords.SetSize(nPart+1);
    }
    int posMerge = -1;
    uint64 ndbgRewritten = 0;
    for (int i= 0; i < m_aaRecords[nPart].GetCount(); i++)
    {
        if (m_aaRecords[nPart][i].CanMerge(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE))
        {
            posMerge = i;
        }
        // check if there is already an pending entry and overwrite it
        else if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE)
        {
            if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos)
            {
                // old one is included in new one -> delete
                if (m_aaRecords[nPart][i].m_dwIP != 0)
                    ndbgRewritten += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
                m_aaRecords[nPart].RemoveAt(i);
                i--;
            }
            else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos)
            {
                // old one includes new one
                // check if the old one and new one have the same ip
                if (dwSenderIP != m_aaRecords[nPart][i].m_dwIP)
                {
                    // different IP, means we have to split it 2 times
                    uint64 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
                    uint64 nTmpStartPos1 = nRelEndPos + 1;
                    uint64 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
                    uint64 nTmpEndPos2 = nRelStartPos - 1;
                    m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
                    m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
                    UINT dwOldIP = m_aaRecords[nPart][i].m_dwIP;
                    m_aaRecords[nPart][i].m_dwIP = dwSenderIP;
                    m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos1,nTmpEndPos1, dwOldIP));
                    m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos2,nTmpEndPos2, dwOldIP));
                    // and are done then
                }
                DEBUG_ONLY(AddDebugLogLine(DLP_DEFAULT, false, L"CorruptionBlackBox: Debug: %i bytes were rewritten and records replaced with new stats (1)", (nRelEndPos - nRelStartPos)+1));
                return;
            }
            else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos)
            {
                // old one laps over new one on the right site
                ASSERT(nRelEndPos - m_aaRecords[nPart][i].m_nStartPos > 0);
                if (m_aaRecords[nPart][i].m_dwIP != 0)
                    ndbgRewritten += nRelEndPos - m_aaRecords[nPart][i].m_nStartPos;
                m_aaRecords[nPart][i].m_nStartPos = nRelEndPos + 1;
            }
            else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos)
            {
                // old one laps over new one on the left site
                ASSERT(m_aaRecords[nPart][i].m_nEndPos - nRelStartPos > 0);
                if (m_aaRecords[nPart][i].m_dwIP != 0)
                    ndbgRewritten += m_aaRecords[nPart][i].m_nEndPos - nRelStartPos;
                m_aaRecords[nPart][i].m_nEndPos = nRelStartPos - 1;
            }
        }
    }

    if (posMerge != (-1))
        VERIFY(m_aaRecords[nPart][posMerge].Merge(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE));
    else
        m_aaRecords[nPart].Add(CCBBRecord(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE));

    if (ndbgRewritten > 0)
        DEBUG_ONLY(AddDebugLogLine(DLP_DEFAULT, false, L"CorruptionBlackBox: Debug: %i bytes were rewritten and records replaced with new stats (2)", ndbgRewritten));
}

void CCorruptionBlackBox::VerifiedData(uint64 nStartPos, uint64 nEndPos)
{
    if (nEndPos - nStartPos >= PARTSIZE)
    {
        ASSERT(0);
        return;
    }
    // convert pos to relative block pos
    UINT nPart = (UINT)(nStartPos / PARTSIZE);
    uint64 nRelStartPos = nStartPos - (uint64)nPart*PARTSIZE;
    uint64 nRelEndPos = nEndPos - (uint64)nPart*PARTSIZE;
    if (nRelEndPos >= PARTSIZE)
    {
        ASSERT(0);
        return;
    }
    if (nPart >= (UINT)m_aaRecords.GetCount())
    {
        //ASSERT(0);
        m_aaRecords.SetSize(nPart+1);
    }
    uint64 nDbgVerifiedBytes = 0;
    //UINT nDbgOldEntries = m_aaRecords[nPart].GetCount();
#ifdef _DEBUG
    CMap<int, int, int, int> mapDebug;
#endif
    for (int i= 0; i < m_aaRecords[nPart].GetCount(); i++)
    {
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        if (m_aaRecords[nPart][i].m_BBRStatus != BBR_CORRUPTED)
            //if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE || m_aaRecords[nPart][i].m_BBRStatus == BBR_VERIFIED)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        {
            if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos)
            {
                nDbgVerifiedBytes += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
                m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
                DEBUG_ONLY(mapDebug.SetAt(m_aaRecords[nPart][i].m_dwIP, 1));
            }
            else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos)
            {
                // need to split it 2*
                uint64 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
                uint64 nTmpStartPos1 = nRelEndPos + 1;
                uint64 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
                uint64 nTmpEndPos2 = nRelStartPos - 1;
                m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
                m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
                m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos1, nTmpEndPos1, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
                m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos2, nTmpEndPos2, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
                nDbgVerifiedBytes += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
                m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
                DEBUG_ONLY(mapDebug.SetAt(m_aaRecords[nPart][i].m_dwIP, 1));
            }
            else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos)
            {
                // need to split it
                uint64 nTmpEndPos = m_aaRecords[nPart][i].m_nEndPos;
                uint64 nTmpStartPos = nRelEndPos + 1;
                m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
                m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
                nDbgVerifiedBytes += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
                m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
                DEBUG_ONLY(mapDebug.SetAt(m_aaRecords[nPart][i].m_dwIP, 1));
            }
            else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos)
            {
                // need to split it
                uint64 nTmpStartPos = m_aaRecords[nPart][i].m_nStartPos;
                uint64 nTmpEndPos = nRelStartPos - 1;
                m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
                m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
                nDbgVerifiedBytes += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
                m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
                DEBUG_ONLY(mapDebug.SetAt(m_aaRecords[nPart][i].m_dwIP, 1));
            }
        }
    }
    /*#ifdef _DEBUG
    	UINT nClients = mapDebug.GetCount();
    #else
    	UINT nClients = 0;
    #endif
    	AddDebugLogLine(DLP_DEFAULT, false, L"Found and marked %s recorded bytes of %s as verified in the CorruptionBlackBox records, %u(%u) records found, %u different clients", CastItoXBytes(nDbgVerifiedBytes), CastItoXBytes((nEndPos-nStartPos)+1), m_aaRecords[nPart].GetCount(), nDbgOldEntries, nClients);*/
}



void CCorruptionBlackBox::CorruptedData(uint64 nStartPos, uint64 nEndPos)
{
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    // netfinity: Even if we don't have an AICH set we may be able to use the CorruptionBlackBox for whole parts
    if (nEndPos - nStartPos >= PARTSIZE)
        //if (nEndPos - nStartPos >= EMBLOCKSIZE)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    {
        ASSERT(0);
        return;
    }
    // convert pos to relative block pos
    UINT nPart = (UINT)(nStartPos / PARTSIZE);
    uint64 nRelStartPos = nStartPos - (uint64)nPart*PARTSIZE;
    uint64 nRelEndPos = nEndPos - (uint64)nPart*PARTSIZE;
    if (nRelEndPos >= PARTSIZE)
    {
        ASSERT(0);
        return;
    }
    if (nPart >= (UINT)m_aaRecords.GetCount())
    {
        //ASSERT(0);
        m_aaRecords.SetSize(nPart+1);
    }
    uint64 nDbgVerifiedBytes = 0;
    for (int i= 0; i < m_aaRecords[nPart].GetCount(); i++)
    {
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE || m_aaRecords[nPart][i].m_BBRStatus == BBR_POSSIBLYCORRUPTED)
            //if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        {
            if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos)
            {
                nDbgVerifiedBytes += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                if (m_aaRecords[nPart][i].m_nStartPos != nRelStartPos || m_aaRecords[nPart][i].m_nEndPos != nRelEndPos)
                    m_aaRecords[nPart][i].m_BBRStatus = BBR_POSSIBLYCORRUPTED;
                else
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                    m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
            }
            else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos)
            {
                // need to split it 2*
                uint64 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
                uint64 nTmpStartPos1 = nRelEndPos + 1;
                uint64 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
                uint64 nTmpEndPos2 = nRelStartPos - 1;
                m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
                m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
                m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos1, nTmpEndPos1, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
                m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos2, nTmpEndPos2, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
                nDbgVerifiedBytes += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
                m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
            }
            else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos)
            {
                // need to split it
                uint64 nTmpEndPos = m_aaRecords[nPart][i].m_nEndPos;
                uint64 nTmpStartPos = nRelEndPos + 1;
                m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
                m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
                nDbgVerifiedBytes += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                if (m_aaRecords[nPart][i].m_nStartPos != nRelStartPos)
                    m_aaRecords[nPart][i].m_BBRStatus = BBR_POSSIBLYCORRUPTED;
                else
//><<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                    m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
            }
            else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos)
            {
                // need to split it
                uint64 nTmpStartPos = m_aaRecords[nPart][i].m_nStartPos;
                uint64 nTmpEndPos = nRelStartPos - 1;
                m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
                m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
                nDbgVerifiedBytes += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                if (m_aaRecords[nPart][i].m_nEndPos != nRelEndPos)
                    m_aaRecords[nPart][i].m_BBRStatus = BBR_POSSIBLYCORRUPTED;
                else
//><<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                    m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
            }
        }
    }
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (nDbgVerifiedBytes != (nEndPos-nStartPos)+1)
        AddDebugLogLine(DLP_HIGH, false, L"Only found and marked %s recorded bytes of %s as corrupted in the CorruptionBlackBox records (this will cause problems!)", CastItoXBytes(nDbgVerifiedBytes), CastItoXBytes((nEndPos-nStartPos)+1));
//     if (nDbgVerifiedBytes != 0)
//         AddDebugLogLine(DLP_HIGH, false, L"Found and marked %s recorded bytes of %s as corrupted in the CorruptionBlackBox records", CastItoXBytes(nDbgVerifiedBytes), CastItoXBytes((nEndPos-nStartPos)+1));
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
}

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
uint64 CCorruptionBlackBox::EvaluateData(UINT nPart, CPartFile* pFile, uint64 evalBlockSize)
//void CCorruptionBlackBox::EvaluateData(UINT nPart)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
{
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    bool	bBannedClient = false;
    bool	bUnknownCorruptor = false;
    uint64	nFlushedBytes = 0;

    CArray<UINT, UINT> aGuiltyClients;
    for (int i = 0; i < m_aaRecords[nPart].GetCount(); ++i)
    {
        if (m_aaRecords[nPart][i].m_BBRStatus == BBR_CORRUPTED || m_aaRecords[nPart][i].m_BBRStatus == BBR_POSSIBLYCORRUPTED)
        {
            aGuiltyClients.Add(m_aaRecords[nPart][i].m_dwIP);
            // netfinity: Is corrupting source unknown?
            if (m_aaRecords[nPart][i].m_dwIP == 0)
                bUnknownCorruptor = true;
        }
    }

    // netfinity: Clean corrupt blocks of unknown sources first before banning
    if (bUnknownCorruptor)
    {
        if (pFile)
        {
            for (int i = 0; i < m_aaRecords[nPart].GetCount(); i++)
            {
                if (m_aaRecords[nPart][i].m_dwIP == 0 && (m_aaRecords[nPart][i].m_BBRStatus == BBR_CORRUPTED || m_aaRecords[nPart][i].m_BBRStatus == BBR_POSSIBLYCORRUPTED))
                {
                    pFile->AddGap(nPart * PARTSIZE + m_aaRecords[nPart][i].m_nStartPos, nPart * PARTSIZE + m_aaRecords[nPart][i].m_nEndPos);
                    nFlushedBytes += (m_aaRecords[nPart][i].m_nEndPos - m_aaRecords[nPart][i].m_nStartPos) + 1;
                    // netfinity: We need to track which corrupted blocks we added gaps for
                    m_aaRecords[nPart][i].m_BBRStatus = BBR_FLUSHED;
                }
            }
        }
        DebugLogWarning(false, L"Flushed %s bytes as marked as corrupted in the CorruptionBlackBox records from unknown source(s)!)", CastItoXBytes(nFlushedBytes));
        return nFlushedBytes;
    }

    /*
        CArray<UINT, UINT> aGuiltyClients;
        for (int i= 0; i < m_aaRecords[nPart].GetCount(); i++)
            if (m_aaRecords[nPart][i].m_BBRStatus == BBR_CORRUPTED)
                aGuiltyClients.Add(m_aaRecords[nPart][i].m_dwIP);

        // check if any IPs are already banned, so we can skip the test for those
        for (int k = 0; k < aGuiltyClients.GetCount();)
        {
            // remove doubles
            for (int y = k+1; y < aGuiltyClients.GetCount();)
            {
                if (aGuiltyClients[k] == aGuiltyClients[y])
                    aGuiltyClients.RemoveAt(y);
                else
                    y++;
            }
            if (theApp.clientlist->IsBannedClient(aGuiltyClients[k]))
            {
                AddDebugLogLine(DLP_DEFAULT, false, L"CorruptionBlackBox: Suspicous IP (%s) is already banned, skipping recheck"), ipstr(aGuiltyClients[k]));
                aGuiltyClients.RemoveAt(k);
            }
            else
                k++;
        }
    */
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

    if (aGuiltyClients.GetCount() > 0)
    {
        // parse all recorded data for this file to produce a statistic for the involved clients

        // first init arrays for the statistic
        CArray<uint64> aDataCorrupt;
        CArray<uint64> aDataVerified;
        aDataCorrupt.SetSize(aGuiltyClients.GetCount());
        aDataVerified.SetSize(aGuiltyClients.GetCount());
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        CArray<uint64> aDataPossiblyCorrupt;
        aDataPossiblyCorrupt.SetSize(aGuiltyClients.GetCount());
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        for (int j = 0; j < aGuiltyClients.GetCount(); j++)
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
            aDataCorrupt[j] = aDataVerified[j] = aDataPossiblyCorrupt[j] = 0;
        //aDataCorrupt[j] = aDataVerified[j] = 0;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

        // now the parsing
        for (int iPart = 0; iPart < m_aaRecords.GetCount(); iPart++)
        {
            for (int i = 0; i < m_aaRecords[iPart].GetCount(); i++)
            {
                for (int k = 0; k < aGuiltyClients.GetCount(); k++)
                {
                    if (m_aaRecords[iPart][i].m_dwIP == aGuiltyClients[k])
                    {
                        if (m_aaRecords[iPart][i].m_BBRStatus == BBR_CORRUPTED)
                        {
                            // corrupted data records are always counted as at least blocksize or bigger
                            aDataCorrupt[k] += max((m_aaRecords[iPart][i].m_nEndPos-m_aaRecords[iPart][i].m_nStartPos)+1, evalBlockSize); //EMBLOCKSIZE
                        }
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                        //else if (m_aaRecords[iPart][i].m_BBRStatus == BBR_VERIFIED)
                        else if (m_aaRecords[iPart][i].m_BBRStatus == BBR_POSSIBLYCORRUPTED)
                        {
                            // corrupted data records are always counted as at least blocksize or bigger
                            aDataPossiblyCorrupt[k] += max((m_aaRecords[iPart][i].m_nEndPos-m_aaRecords[iPart][i].m_nStartPos)+1, evalBlockSize);
                        }
                        // netfinity: If there is a banned client we want to see it as an 100% corruptor
                        else if (m_aaRecords[iPart][i].m_BBRStatus == BBR_VERIFIED && !theApp.clientlist->IsBannedClient(aGuiltyClients[k]))
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                        {
                            aDataVerified[k] += (m_aaRecords[iPart][i].m_nEndPos-m_aaRecords[iPart][i].m_nStartPos)+1;
                        }
                    }
                }
            }
        }

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        // netfinity: Raise threshold on mass corruption, to avoid banning good clients
        int	nGuiltyClientsCount = static_cast<int>(aGuiltyClients.GetCount());
        int* nCorruptPercentage = new int[nGuiltyClientsCount];
        int nCorruptPercentageAvg = 0;
        int nCorruptPercentageHigh = 0;
        int nCorruptPercentageThreshold = CBB_BANTHRESHOLD;

        for (int k = 0; k < aGuiltyClients.GetCount(); ++k)
        {
            // calculate the percentage of corrupted data for each client and ban
            // him if the limit is reached
            if ((aDataVerified[k] + aDataCorrupt[k] + aDataPossiblyCorrupt[k]) > 0)
                nCorruptPercentage[k] = (int)(((uint64)(aDataCorrupt[k]+aDataPossiblyCorrupt[k])*100)/(aDataVerified[k] + aDataCorrupt[k] + aDataPossiblyCorrupt[k]));
            else
            {
                AddDebugLogLine(DLP_HIGH, false, L"CorruptionBlackBox: Program Error: No records for guilty client found!");
                ASSERT(0);
                nCorruptPercentage[k] = 0;
            }
            nCorruptPercentageAvg += nCorruptPercentage[k];
            if (nCorruptPercentage[k] > nCorruptPercentageHigh)
                nCorruptPercentageHigh = nCorruptPercentage[k];
        }
        nCorruptPercentageAvg /= nGuiltyClientsCount;
        if (nCorruptPercentageAvg > nCorruptPercentageThreshold)
            nCorruptPercentageThreshold = nCorruptPercentageAvg;
        if (4 * nCorruptPercentageHigh / 5 > nCorruptPercentageThreshold)
            nCorruptPercentageThreshold = 4 * nCorruptPercentageHigh / 5;

        for (int k = 0; k < aGuiltyClients.GetCount(); ++k)
        {
            if (nCorruptPercentage[k] > nCorruptPercentageThreshold || aDataCorrupt[k] > 0)
            {
                // netfinity: Don't ban on a single partial involvement
                if (aDataVerified[k] + aDataCorrupt[k] + aDataPossiblyCorrupt[k] > evalBlockSize || aDataCorrupt[k] > 0)
                {
#ifdef IPV6_SUPPORT
                    CUpDownClient* pEvilClient = theApp.clientlist->FindClientByIP(CAddress(_ntohl(aGuiltyClients[k]))); //>>> WiZaRd::IPv6 [Xanatos]
#else
                    CUpDownClient* pEvilClient = theApp.clientlist->FindClientByIP(aGuiltyClients[k]);
#endif
                    if (pEvilClient != NULL)
                    {
                        AddDebugLogLine(DLP_HIGH, false, L"CorruptionBlackBox: Banning: Found client which sent %s of %s corrupted data, %s", CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), pEvilClient->DbgGetClientInfo());
                        theApp.clientlist->AddTrackClient(pEvilClient);
                        pEvilClient->Ban(L"Identified as sender of corrupt data");
                    }
                    else
                    {
                        AddDebugLogLine(DLP_HIGH, false, L"CorruptionBlackBox: Banning: Found client which sent %s of %s corrupted data, %s", CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), ipstr(aGuiltyClients[k]));
                        theApp.clientlist->AddBannedClient(aGuiltyClients[k]);
                    }
                    bBannedClient = true;
                }
                else
                    AddDebugLogLine(DLP_HIGH, false, L"CorruptionBlackBox: Flushed: Found client which sent %s of %s corrupted data, %s", CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), ipstr(aGuiltyClients[k]));

                // netfinity: Mark data downloaded by banned client(s) as incomplete if not already done
                if (pFile)
                {
                    for (int i = 0; i < m_aaRecords[nPart].GetCount(); i++)
                    {
                        if (m_aaRecords[nPart][i].m_dwIP == aGuiltyClients[k] && (m_aaRecords[nPart][i].m_BBRStatus == BBR_CORRUPTED || m_aaRecords[nPart][i].m_BBRStatus == BBR_POSSIBLYCORRUPTED))
                        {
                            pFile->AddGap(nPart * PARTSIZE + m_aaRecords[nPart][i].m_nStartPos, nPart * PARTSIZE + m_aaRecords[nPart][i].m_nEndPos);
                            nFlushedBytes += (m_aaRecords[nPart][i].m_nEndPos - m_aaRecords[nPart][i].m_nStartPos) + 1;
                            // netfinity: We need to track which corrupted blocks we added gaps for
                            m_aaRecords[nPart][i].m_BBRStatus = BBR_FLUSHED;
                        }
                    }
                }
            }
            else
            {
#ifdef IPV6_SUPPORT
                CUpDownClient* pSuspectClient = theApp.clientlist->FindClientByIP(CAddress(_ntohl(aGuiltyClients[k]))); //>>> WiZaRd::IPv6 [Xanatos]
#else
                CUpDownClient* pSuspectClient = theApp.clientlist->FindClientByIP(aGuiltyClients[k]);
#endif
                if (pSuspectClient != NULL)
                {
                    AddDebugLogLine(DLP_DEFAULT, false, L"CorruptionBlackBox: Reporting: Found client which probably sent %s of %s corrupted data, but it is within the acceptable limit, %s", CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), pSuspectClient->DbgGetClientInfo());
                    theApp.clientlist->AddTrackClient(pSuspectClient);
                }
                else
                    AddDebugLogLine(DLP_DEFAULT, false, L"CorruptionBlackBox: Reporting: Found client which probably sent %s of %s corrupted data, but it is within the acceptable limit, %s", CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), ipstr(aGuiltyClients[k]));
            }
        }
    }

    // netfinity: Mark corrupt data as incomplete if no client was banned for it
    if (pFile)
    {
        int purged = 0;
        for (uint64 startBlock = (nPart * PARTSIZE); startBlock < ((nPart + 1) * PARTSIZE); startBlock += evalBlockSize)
        {
            uint64 endBlock = startBlock + evalBlockSize;
            if (endBlock >= ((nPart + 1) * PARTSIZE))
                endBlock = ((nPart + 1) * PARTSIZE) - 1;
            // If for some reason no data been purged for the blocks where there is corruption, then do it now
            if (pFile->IsComplete(startBlock, endBlock, false))
            {
                for (int i = 0; i < m_aaRecords[nPart].GetCount(); i++)
                {
                    if (m_aaRecords[nPart][i].m_nStartPos <= endBlock && m_aaRecords[nPart][i].m_nEndPos >= startBlock)
                    {
                        if (m_aaRecords[nPart][i].m_BBRStatus == BBR_CORRUPTED || m_aaRecords[nPart][i].m_BBRStatus == BBR_POSSIBLYCORRUPTED)
                        {
                            pFile->AddGap(nPart * PARTSIZE + m_aaRecords[nPart][i].m_nStartPos, nPart * PARTSIZE + m_aaRecords[nPart][i].m_nEndPos);
                            nFlushedBytes += (m_aaRecords[nPart][i].m_nEndPos - m_aaRecords[nPart][i].m_nStartPos) + 1;
                            // netfinity: We need to track which corrupted blocks we added gaps for
                            m_aaRecords[nPart][i].m_BBRStatus = BBR_FLUSHED;
                            purged++;
                        }
                    }
                }
            }
        }
        if (purged > 0)
            AddDebugLogLine(DLP_DEFAULT, false, L"CorruptionBlackBox: Reporting: Purged %i block(s) that was still complete while marked as corrupt after checking and banning operation!", purged);
    }
    if (nFlushedBytes > 0)
        DebugLogWarning(false, L"Flushed %s bytes as marked as corrupted in the CorruptionBlackBox records!)", CastItoXBytes(nFlushedBytes));

    return nFlushedBytes;
    /*
            for (int k = 0; k < aGuiltyClients.GetCount(); k++)
            {
                // calculate the percentage of corrupted data for each client and ban
                // him if the limit is reached
                int nCorruptPercentage;
                if ((aDataVerified[k] + aDataCorrupt[k]) > 0)
                    nCorruptPercentage = (int)(((uint64)aDataCorrupt[k]*100)/(aDataVerified[k] + aDataCorrupt[k]));
                else
                {
                    AddDebugLogLine(DLP_HIGH, false, L"CorruptionBlackBox: Program Error: No records for guilty client found!"));
                    ASSERT(0);
                    nCorruptPercentage = 0;
                }
                if (nCorruptPercentage > CBB_BANTHRESHOLD)
                {

    #ifdef IPV6_SUPPORT
                    CUpDownClient* pEvilClient = theApp.clientlist->FindClientByIP(CAddress(_ntohl(aGuiltyClients[k]))); //>>> WiZaRd::IPv6 [Xanatos]
    #else
                    CUpDownClient* pEvilClient = theApp.clientlist->FindClientByIP(aGuiltyClients[k]);
    #endif
                    if (pEvilClient != NULL)
                    {
                        AddDebugLogLine(DLP_HIGH, false, L"CorruptionBlackBox: Banning: Found client which send %s of %s corrupted data, %s"), CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), pEvilClient->DbgGetClientInfo());
                        theApp.clientlist->AddTrackClient(pEvilClient);
                        theApp.antileechlist->AddCorruptPartSender(pEvilClient->GetUserHash()); //>>> WiZaRd::ClientAnalyzer
                        pEvilClient->Ban(L"Identified as sender of corrupt data"));
                    }
                    else
                    {
                        AddDebugLogLine(DLP_HIGH, false, L"CorruptionBlackBox: Banning: Found client which send %s of %s corrupted data, %s"), CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), ipstr(aGuiltyClients[k]));
                        theApp.clientlist->AddBannedClient(aGuiltyClients[k]);
                    }
                }
                else
                {
    #ifdef IPV6_SUPPORT
                    CUpDownClient* pSuspectClient = theApp.clientlist->FindClientByIP(CAddress(_ntohl(aGuiltyClients[k]))); //>>> WiZaRd::IPv6 [Xanatos]
    #else
                    CUpDownClient* pSuspectClient = theApp.clientlist->FindClientByIP(aGuiltyClients[k]);
    #endif
                    if (pSuspectClient != NULL)
                    {
                        AddDebugLogLine(DLP_DEFAULT, false, L"CorruptionBlackBox: Reporting: Found client which probably sent %s of %s corrupted data, but it is within the acceptable limit, %s", CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), pSuspectClient->DbgGetClientInfo());
                        theApp.clientlist->AddTrackClient(pSuspectClient);
                    }
                    else
                        AddDebugLogLine(DLP_DEFAULT, false, L"CorruptionBlackBox: Reporting: Found client which probably sent %s of %s corrupted data, but it is within the acceptable limit, %s", CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), ipstr(aGuiltyClients[k]));
                }
            }
        }
    */
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
}
