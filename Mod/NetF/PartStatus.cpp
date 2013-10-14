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
#include "stdafx.h"
#include "PartStatus.h"
#include "SafeFile.h"
#include "Log.h"
#include "PartFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPartStatus::CPartStatus()
{
	m_pStatusFile = NULL;
}

CPartStatus::~CPartStatus()
{
	ASSERT(m_pStatusFile == NULL);
}

CKnownFile* CPartStatus::GetStatusFile() const
{
	return m_pStatusFile;
}

void CPartStatus::SetStatusFile(CKnownFile* pFile)
{
	m_pStatusFile = pFile;
}

uint64
CPartStatus::GetCompleted(uint64 start, uint64 stop) const
{
	uint64	bytes = 0ULL;
	uint64	end = stop;
	while (FindFirstComplete(start, stop))
	{
		bytes += (stop - start) + 1ULL;
		start = stop + 1ULL;
		stop = end;
	}
	return bytes;
}

uint64
CPartStatus::GetNeeded(uint64 start, uint64 stop) const
{
	uint64	bytes = 0ULL;
	uint64	end = stop;
	while (FindFirstNeeded(start, stop))
	{
		bytes += (stop - start) + 1ULL;
		start = stop + 1ULL;
		stop = end;
	}
	return bytes;
}

uint64
CPartStatus::GetNeeded(const CPartStatus* const from) const
{
	uint64	bytes = 0ULL;
	uint64	start = 0ULL;
	uint64	stop = ~0ULL;

	if (from == NULL)
		throw CString(_T(__FUNCTION__) _T("; from == NULL"));
	if (from->GetSize() != GetSize())
		throw CString(_T(__FUNCTION__) _T("; size mismatch"));

	while (from->FindFirstComplete(start, stop))
	{
		uint64	start2 = start;
		uint64	stop2 = stop;
		while (FindFirstNeeded(start2, stop2))
		{
			bytes += (stop2 - start2) + 1ULL;
			start2 = stop2 + 1ULL;
			stop2 = stop;
		}
		start = stop + 1ULL;
		stop = ~0ULL;
	}
	return bytes;
}

bool
CPartStatus::FindFirstNeeded(uint64& start, uint64& stop, const CPartStatus* const from) const
{
	const uint64	begin = start;
	const uint64	end = stop;

	if (from == NULL)
		throw CString(_T(__FUNCTION__) _T("; from == NULL"));
	if (from->GetSize() != GetSize())
		throw CString(_T(__FUNCTION__) _T("; size mismatch"));

	if (GetSize() == 0 || start >= GetSize() || start > stop)
		return false;

	while (from->FindFirstComplete(start, stop))
	{
		if (FindFirstNeeded(start, stop))
			return true;
		start = stop + 1;
		stop = end;
	}
	start = begin;
	stop = end;
	return false;
}

CPartStatus*
CPartStatus::CreatePartStatus(CSafeMemFile* const data, CKnownFile* pFile, const bool defState)
{
	uint64 size = pFile->GetFileSize();
	uint64	sctSize = 0ULL;
	uint16	sctDivider = 0;
	uint16	sctCount;
	CPartStatus* partStatus = NULL;

	ASSERT(data != 0);
	
	try
	{
		sctCount = data->ReadUInt16();
		// Special case
		if (sctCount == 0)
		{
			partStatus = new CAICHStatusVector(pFile); // temporarily used until standard container is implemented
			if (defState == true)
				partStatus->Set(0, size - 1);
			else
				partStatus->Clear(0, size - 1);	
			return partStatus;
		}
		// Future versions of the protocol may send a partmap of size 1 to indicate it doesn't have any data to share yet!
		else if (sctCount == 1)
		{
			partStatus = new CAICHStatusVector(pFile); // temporarily used until standard container is implemented
			if (data->ReadUInt8() & 0x01)
				partStatus->Set(0, size - 1);
			else
				partStatus->Clear(0, size - 1);	
			return partStatus;
		}

		uint16 const wholePartCount = static_cast<uint16>(size / PARTSIZE);
		// Check for standard part vector
		if ((UINT) sctCount == (UINT) ((size + (PARTSIZE - 1)) / PARTSIZE))
		{
			partStatus = new CAICHStatusVector(pFile); // temporarily used until standard container is implemented
			sctSize = PARTSIZE;
			sctDivider = 1;
		}
		// Check for AICH chunks
		else if ((UINT) sctCount == (UINT) (53 * wholePartCount + ((size % PARTSIZE) + (EMBLOCKSIZE - 1)) / EMBLOCKSIZE))
		{
			DebugLog(_T(__FUNCTION__) _T("; Part vector has AICH sub chunks! :)"));
			partStatus = new CAICHStatusVector(pFile);
			sctSize = EMBLOCKSIZE;
			sctDivider = EMBLOCKSPERPART;
		}
		else if (wholePartCount > 1) // needed to avoid ambiguity with crumbs
		{
			if ((UINT) sctCount == (UINT) (27 * wholePartCount + ((size % PARTSIZE) + (2 * EMBLOCKSIZE - 1)) / (2 * EMBLOCKSIZE)))
			{
				DebugLog(_T(__FUNCTION__) _T("; Part vector has AICH x2 sub chunks! :)"));
				partStatus = new CAICHStatusVector(pFile);
				sctSize = 2 * EMBLOCKSIZE;
				sctDivider = 27;
			}
			else if ((UINT) sctCount == (UINT) (14 * wholePartCount + ((size % PARTSIZE) + (4 * EMBLOCKSIZE - 1)) / (4 * EMBLOCKSIZE)))
			{
				DebugLog(_T(__FUNCTION__) _T("; Part vector has AICH x4 sub chunks! :)"));
				partStatus = new CAICHStatusVector(pFile);
				sctSize = 4 * EMBLOCKSIZE;
				sctDivider = 14;
			}
			else if ((UINT) sctCount == (UINT) (7 * wholePartCount + ((size % PARTSIZE) + (8 * EMBLOCKSIZE - 1)) / (8 * EMBLOCKSIZE)))
			{
				DebugLog(_T(__FUNCTION__) _T("; Part vector has AICH x8 sub chunks! :)"));
				partStatus = new CAICHStatusVector(pFile);
				sctSize = 8 * EMBLOCKSIZE;
				sctDivider = 7;
			}
			else if ((UINT) sctCount == (UINT) (4 * wholePartCount + ((size % PARTSIZE) + (16 * EMBLOCKSIZE - 1)) / (16 * EMBLOCKSIZE)))
			{
				DebugLog(_T(__FUNCTION__) _T("; Part vector has AICH x16 sub chunks! :)"));
				partStatus = new CAICHStatusVector(pFile);
				sctSize = 16 * EMBLOCKSIZE;
				sctDivider = 4;
			}
			else if ((UINT) sctCount == (UINT) (2 * wholePartCount + ((size % PARTSIZE) + (32 * EMBLOCKSIZE - 1)) / (32 * EMBLOCKSIZE)))
			{
				DebugLog(_T(__FUNCTION__) _T("; Part vector has AICH x32 sub chunks! :)"));
				partStatus = new CAICHStatusVector(pFile);
				sctSize = 32 * EMBLOCKSIZE;
				sctDivider = 2;
			}
		}
		// Check for Crumbs
		if (sctSize == 0ULL && (UINT) sctCount == (UINT) ((size + (CRUMBSIZE - 1)) / CRUMBSIZE))
		{
			DebugLog(_T(__FUNCTION__) _T("; Part vector has crumbs! :)"));
			partStatus = new CCrumbStatusVector(pFile);
			sctSize = CRUMBSIZE;
			sctDivider = CRUMBSPERPART;
		}
		// Invalid part status vector?
		if (sctSize == 0ULL) 
		{
			CString error;
			error.Format(_T(__FUNCTION__) _T("; Part/subchunk count (%u) doesn't match any known subchunk format!"), static_cast<unsigned int>(sctCount));
			throw error;
		}

		// Validate part count
		uint16 const expectedSctCount = (uint16) (wholePartCount * sctDivider + ((size % PARTSIZE) + (sctSize - 1)) / sctSize);
		if (sctCount != expectedSctCount)
		{
			CString error;
			error.Format(_T(__FUNCTION__) _T("; Invalid part/subchunk count: %u, expected %u"), static_cast<unsigned int>(sctCount), static_cast<unsigned int>(expectedSctCount));
			throw error;
		}
		// Check all bytes of the part status vector is available (final corruption check)
		if (((sctCount + 7) / 8) > (data->GetLength() - data->GetPosition()))
		{
			CString error;
			error.Format(_T(__FUNCTION__) _T("; Part status vector with %u part/subchunk(s) is incomplete. %u bytes provided but %u needed."), static_cast<unsigned int>(sctCount), static_cast<unsigned int>(data->GetLength() - data->GetPosition()), static_cast<unsigned int>((sctCount + 7) / 8));
			throw error;
		}
		// Read part status vector
		uint64	start = 0, stop = 0;
		bool	laststate = false;
		uint8	tmp = 0;
		for (uint16 i = 0; i < sctCount; i++)
		{
			uint16 const shift = i & 0x0007; 
			uint16 const part = i / sctDivider;
			uint16 const subChunk = i % sctDivider;

			if (shift == 0)
				tmp = data->ReadUInt8();
			if (((tmp >> shift) & 0x01) == 0)
			{
				if (laststate == true)
				{
					if (stop != 0)
						partStatus->Set(start, stop);
					start = part * PARTSIZE + subChunk * sctSize;
				}
				stop = part * PARTSIZE + (subChunk + 1) * sctSize - 1;
				laststate = false;
			}
			else
			{
				if (laststate == false)
				{
					if (stop != 0)
						partStatus->Clear(start, stop);
					start = part * PARTSIZE + subChunk * sctSize;
				}
				stop = part * PARTSIZE + (subChunk + 1) * sctSize - 1;
				laststate = true;
			}
		}
		stop = size - 1;
		if (start <= stop)
		{
			if (laststate == true) 
				partStatus->Set(start, stop);
			else
				partStatus->Clear(start, stop);
		}
	}
	catch(...)
	{
		delete partStatus;
		throw;
	}

	return partStatus;
}

void
CPartStatus::WritePartStatus(CSafeMemFile* const data, const int protocolRevision, const bool defState) const
{
	const uint64	size = GetSize();
	uint64	sctSize = PARTSIZE;
	uint16	sctDivider = 1;
	uint16	sctCount;
	uint8	tmp;

	// Special case when all parts have the same state
	uint64 sizeNeeded = GetNeeded(0, size - 1);
	if (defState == true && sizeNeeded == 0)
	{
		data->WriteUInt16(0);
		return;
	}
	else if (sizeNeeded == size)
	{
		if (defState == false)
		{
			data->WriteUInt16(0);
			return;
		}
		else if (protocolRevision >= PROTOCOL_REVISION_2)
		{
			data->WriteUInt16(1);
			data->WriteUInt8(0);
			return;
		}
	}

	// Decide chunk size for part vector (limit to 8000 sub chunks, to keep part vector size less than 1kB?)
	if (protocolRevision >= PROTOCOL_REVISION_1)
	{
		bool isSubChunksAvailable = false;
		uint16 partCount = static_cast<uint16>((size + PARTSIZE - 1) / PARTSIZE);
		for (uint16 i = 0; i < partCount; ++i)
		{
			if (IsPartialPart(i))
			{
				isSubChunksAvailable = true;
				break;
			}
		}

		// Only send sub chunks if there are incomplete parts shared
		if (isSubChunksAvailable)
		{
			if (protocolRevision >= PROTOCOL_REVISION_2)
			{
				if (size <= (8000 * EMBLOCKSIZE) && GetChunkSize() < PARTSIZE)
				{
					sctSize = EMBLOCKSIZE;
				}
				sctDivider = static_cast<uint16>((PARTSIZE + sctSize - 1) / sctSize);
			}
			else if (size <= (8000 * CRUMBSIZE) && GetChunkSize() < PARTSIZE)
			{
				sctSize = CRUMBSIZE;
				sctDivider = CRUMBSPERPART;
			}
		}
	}

	sctCount = static_cast<uint16>(sctDivider * (size / PARTSIZE) + ((size % PARTSIZE) + sctSize - 1) / sctSize);

	// Write parts
	data->WriteUInt16(sctCount);
	uint16 i;
	for (i = 0; i < sctCount; i++)
	{
		uint16 const shift = i & 0x0007;
		uint16 const part = i / sctDivider;
		uint16 const subChunk = i % sctDivider;

		if (shift == 0)
			tmp = 0;
		if (IsComplete((uint64) part * PARTSIZE + subChunk * sctSize, (uint64) part * PARTSIZE + (subChunk + 1) * sctSize - 1))
			tmp |= (1 << shift);
		else
			tmp &= ~(1 << shift);
		if (shift == 7)
			data->WriteUInt8(tmp);
	}
	if (i % 8 != 0)
		data->WriteUInt8(tmp);
}

//
//	CAICHStatusVector
//

// only used within partfile.cpp!
CAICHStatusVector::CAICHStatusVector(const uint64 size)
{
	UINT const chunks_count = _GetAICHCount(size);
	size_t const vector_size = (chunks_count + 7) / 8;
	m_size = size;
	m_chunks = new uint8[vector_size];
	for (UINT i = 0; i < vector_size; i++)
		m_chunks[i] = 0;
}

CAICHStatusVector::CAICHStatusVector(CKnownFile* pFile)
{
	SetStatusFile(pFile);
	uint64 size = GetStatusFile()->GetFileSize();

	UINT const chunks_count = _GetAICHCount(size);
	size_t const vector_size = (chunks_count + 7) / 8;
	m_size = size;
	m_chunks = new uint8[vector_size];
	for (UINT i = 0; i < vector_size; i++)
		m_chunks[i] = 0;
}

CAICHStatusVector::CAICHStatusVector(const CPartStatus* const source)
{
	SetStatusFile(source->GetStatusFile());

	const uint64 size = source->GetSize();
	const UINT chunks_count = _GetAICHCount(size);
	m_size = size;
	m_chunks = new uint8[(chunks_count + 7) / 8];
	for (UINT i = 0; i < chunks_count; i++)
	{
		if (source->IsCompleteAICH(i))
			_SetAICH(i);
		else
			_ClearAICH(i);
	}
}

CAICHStatusVector::~CAICHStatusVector()
{
	if(GetStatusFile())
	{
		//GetStatusFile()->RemoveFromPartsInfo(this);
		SetStatusFile(NULL);
	}
	delete[] m_chunks;
}

void
CAICHStatusVector::Set(uint64 start, uint64 stop)
{
	if (start >= m_size || start > stop)
		throw CString(_T(__FUNCTION__) _T("; invalid start position"));
	if (stop >= m_size)
		stop = m_size - 1ULL;
	UINT const start_chunk = _GetAICHStart(start);
	UINT const end_chunk = _GetAICHEnd(stop);
	for (UINT i = start_chunk; i < end_chunk; i++)
		_SetAICH(i);
}

void
CAICHStatusVector::Clear(uint64 start, uint64 stop)
{
	if (start >= m_size || start > stop)
		throw CString(_T(__FUNCTION__) _T("; invalid start position"));
	if (stop >= m_size)
		stop = m_size - 1ULL;
	UINT const start_chunk = _GetAICHStart(start);
	UINT const end_chunk = _GetAICHEnd(stop);
	for (UINT i = start_chunk; i < end_chunk; i++)
		_ClearAICH(i);
}

bool
CAICHStatusVector::IsComplete(uint64 start, uint64 stop) const
{
	if (start >= m_size || start > stop)
		throw CString(_T(__FUNCTION__) _T("; invalid start position"));
	if (stop >= m_size)
		stop = m_size - 1ULL;
	UINT const start_chunk = _GetAICHStart(start);
	UINT const end_chunk = _GetAICHEnd(stop);
	for (UINT i = start_chunk; i < end_chunk; i++)
		if (!_IsCompleteAICH(i)) return false;
	return true;
}

bool
CAICHStatusVector::FindFirstComplete(uint64& start, uint64& stop) const
{
	if (start >= m_size || start > stop)
		return false;

	if (stop >= m_size)
		stop = m_size - 1ULL;
	bool bComplete = false;
	const UINT	start_chunk = _GetAICHStart(start);
	const UINT	end_chunk = _GetAICHEnd(stop);
	UINT	first_chunk = end_chunk;
	UINT	last_chunk = end_chunk;
	UINT	i;
	for (i = start_chunk; i < end_chunk; i++)
	{
		if (_IsCompleteAICH(i))
		{
			first_chunk = i;
			bComplete = true;
			break;
		}
	}
	for (; i < end_chunk; i++)
	{
		if (!_IsCompleteAICH(i))
		{
			last_chunk = i;
			break;
		}
	}
	if (first_chunk < end_chunk && bComplete)
	{
		start = max(start, (uint64) (first_chunk / 53ULL) * PARTSIZE + (first_chunk % 53ULL) * EMBLOCKSIZE);
		stop = min(stop, (uint64) (last_chunk / 53ULL) * PARTSIZE + min((last_chunk % 53ULL) * EMBLOCKSIZE, PARTSIZE) - 1ULL);
	}
	return bComplete;
}

bool
CAICHStatusVector::FindFirstNeeded(uint64& start, uint64& stop) const
{
	if (start >= m_size || start > stop)
		return false;

	if (stop >= m_size)
		stop = m_size - 1ULL;
	bool bNeeded = false;
	const UINT	start_chunk = _GetAICHStart(start);
	const UINT	end_chunk = _GetAICHEnd(stop);
	UINT	first_chunk = end_chunk;
	UINT	last_chunk = end_chunk;
	UINT	i;
	for (i = start_chunk; i < end_chunk; i++)
	{
		if (!_IsCompleteAICH(i))
		{
			first_chunk = i;
			bNeeded = true;
			break;
		}
	}
	for (; i < end_chunk; i++)
	{
		if (_IsCompleteAICH(i))
		{
			last_chunk = i;
			break;
		}
	}
	if (first_chunk < end_chunk && bNeeded)
	{
		start = max(start, (uint64) (first_chunk / 53ULL) * PARTSIZE + (first_chunk % 53ULL) * EMBLOCKSIZE);
		stop = min(stop, (uint64) (last_chunk / 53ULL) * PARTSIZE + min((last_chunk % 53ULL) * EMBLOCKSIZE, PARTSIZE) - 1ULL);
	}
	return bNeeded;
}

//
//	CCrumbStatusVector
//

CCrumbStatusVector::CCrumbStatusVector(CKnownFile* pFile)
{
	SetStatusFile(pFile);
	uint64 size = GetStatusFile()->GetFileSize();

	UINT const chunks_count = (UINT) ((size + (CRUMBSIZE - 1)) / CRUMBSIZE);
	size_t const vector_size = (chunks_count + 7) / 8;
	m_size = size;
	m_chunks = new uint8[vector_size];
	for (UINT i = 0; i < vector_size; i++)
		m_chunks[i] = 0;
}

CCrumbStatusVector::CCrumbStatusVector(const CPartStatus* const source)
{
	const uint64 size = source->GetSize();
	const UINT chunks_count = (UINT) ((size + (CRUMBSIZE - 1)) / CRUMBSIZE);
	m_size = size;
	m_chunks = new uint8[(chunks_count + 7) / 8];
	for (UINT i = 0; i < chunks_count; i++)
	{
		if (source->IsCompleteCrumb(i))
			_SetCrumb(i);
		else
			_ClearCrumb(i);
	}
}

CCrumbStatusVector::~CCrumbStatusVector()
{
	if(GetStatusFile())
	{
		//GetStatusFile()->RemoveFromPartsInfo(this);
		SetStatusFile(NULL);
	}
	delete[] m_chunks;
}

void
CCrumbStatusVector::Set(uint64 start, uint64 stop)
{
	if (start >= m_size || start > stop)
		throw CString(_T(__FUNCTION__) _T("; invalid start position"));

	UINT	start_chunk, end_chunk;
	if (m_size == 0)
		return;
	if (stop >= m_size)
		stop = m_size - 1ULL;
	start_chunk = (UINT) ((start + (CRUMBSIZE - 1ULL)) / CRUMBSIZE);
	end_chunk = (UINT) ((stop + 1ULL) / CRUMBSIZE);
	for (UINT i = start_chunk; i < end_chunk; i++)
		_SetCrumb(i);
}

void
CCrumbStatusVector::Clear(uint64 start, uint64 stop)
{
	if (start >= m_size || start > stop)
		throw CString(_T(__FUNCTION__) _T("; invalid start position"));

	UINT	start_chunk, end_chunk;
	if (m_size == 0)
		return;
	if (stop >= m_size)
		stop = m_size - 1ULL;
	start_chunk = (UINT) ((start) / CRUMBSIZE);
	end_chunk = (UINT) ((stop) / CRUMBSIZE + 1ULL);
	for (UINT i = start_chunk; i < end_chunk; i++)
		_ClearCrumb(i);
}

bool
CCrumbStatusVector::IsComplete(uint64 start, uint64 stop) const
{
	if (start >= m_size || start > stop)
		throw CString(_T(__FUNCTION__) _T("; invalid start position"));

	UINT	start_chunk, end_chunk;
	if (m_size == 0)
		return true;
	if (stop >= m_size)
		stop = m_size - 1ULL;
	start_chunk = (UINT) ((start) / CRUMBSIZE);
	end_chunk = (UINT) ((stop) / CRUMBSIZE + 1ULL);
	for (UINT i = start_chunk; i < end_chunk; i++)
		if (!_IsCompleteCrumb(i)) return false;
	return true;
}

bool
CCrumbStatusVector::FindFirstComplete(uint64& start, uint64& stop) const
{
	if (start >= m_size || start > stop)
		return false;

	if (stop >= m_size)
		stop = m_size - 1ULL;
	bool bComplete = false;
	const UINT	start_chunk = (UINT) ((start) / CRUMBSIZE);
	const UINT	end_chunk = (UINT) ((stop) / CRUMBSIZE + 1ULL);
	UINT	first_chunk = end_chunk;
	UINT	last_chunk = end_chunk;
	UINT	i;
	for (i = start_chunk; i < end_chunk; i++)
	{
		if (_IsCompleteCrumb(i))
		{
			first_chunk = i;
			bComplete = true;
			break;
		}
	}
	for (; i < end_chunk; i++)
	{
		if (!_IsCompleteCrumb(i))
		{
			last_chunk = i;
			break;
		}
	}
	if (first_chunk < end_chunk && bComplete)
	{
		start = max(start, (uint64) first_chunk * CRUMBSIZE);
		stop = min(stop, (uint64) last_chunk * CRUMBSIZE - 1ULL);
	}
	return bComplete;
}

bool
CCrumbStatusVector::FindFirstNeeded(uint64& start, uint64& stop) const
{
	if (start >= m_size || start > stop)
		return false;

	if (stop >= m_size)
		stop = m_size - 1ULL;
	bool bNeeded = false;
	const UINT	start_chunk = (UINT) ((start) / CRUMBSIZE);
	const UINT	end_chunk = (UINT) ((stop) / CRUMBSIZE + 1ULL);
	UINT	first_chunk = end_chunk;
	UINT	last_chunk = end_chunk;
	UINT	i;
	for (i = start_chunk; i < end_chunk; i++)
	{
		if (!_IsCompleteCrumb(i))
		{
			first_chunk = i;
			bNeeded = true;
			break;
		}
	}
	for (; i < end_chunk; i++)
	{
		if (_IsCompleteCrumb(i))
		{
			last_chunk = i;
			break;
		}
	}
	if (first_chunk < end_chunk && bNeeded)
	{
		start = max(start, (uint64) first_chunk * CRUMBSIZE);
		stop = min(stop, (uint64) last_chunk * CRUMBSIZE - 1ULL);
	}
	return bNeeded;
}

/*uint64
CCrumbStatusVector::GetNeeded(const CPartStatus* const from) const
{
	uint64	needed = 0;
	for (int i=0; i<_GetCrumbsCount(); i++)
	{
		if (from->IsCompleteCrumb(i) == !_IsCompleteCrumb(i))
			needed += CRUMBSIZE;
	}
	return needed;
}*/