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
#include "opcodes.h" //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]

class CUpDownClient;
class CPartFile; //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]

enum EBBRStatus
{
    BBR_NONE = 0,
    BBR_VERIFIED,
    BBR_CORRUPTED,
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    BBR_POSSIBLYCORRUPTED, // netfinity: Sender was involved in a corrupt block, but was not alone
    BBR_FLUSHED // netfinity: We need to track which corrupted blocks we added gaps for
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
};


class CCBBRecord
{
  public:
    CCBBRecord(uint64 nStartPos = 0, uint64 nEndPos = 0, UINT dwIP = 0, EBBRStatus BBRStatus = BBR_NONE);
    CCBBRecord(const CCBBRecord& cv)			{ *this = cv; }
    CCBBRecord& operator=(const CCBBRecord& cv);

    bool	Merge(uint64 nStartPos, uint64 nEndPos, UINT dwIP, EBBRStatus BBRStatus = BBR_NONE);
    bool	CanMerge(uint64 nStartPos, uint64 nEndPos, UINT dwIP, EBBRStatus BBRStatus = BBR_NONE);

    uint64	m_nStartPos;
    uint64	m_nEndPos;
    UINT	m_dwIP;
    EBBRStatus 	m_BBRStatus;
};

typedef CArray<CCBBRecord> CRecordArray;


class CCorruptionBlackBox
{
  public:
    CCorruptionBlackBox()			{}
    ~CCorruptionBlackBox()			{}
    void	Init(EMFileSize nFileSize);
    void	Free();
    void	TransferredData(uint64 nStartPos, uint64 nEndPos, const CUpDownClient* pSender);
    void	VerifiedData(uint64 nStartPos, uint64 nEndPos);
    void	CorruptedData(uint64 nStartPos, uint64 nEndPos);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    uint64	EvaluateData(UINT nPart, CPartFile* pFile = NULL, uint64 evalBlockSize = EMBLOCKSIZE);
    //void	EvaluateData(UINT nPart);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

  private:
    CArray<CRecordArray> m_aaRecords;
};
