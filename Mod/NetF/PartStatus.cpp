/**
 *
 */

#include "stdafx.h"
#include "PartStatus.h"
#include "SafeFile.h"
#include "Log.h"
#include "PartFile.h"

//#define CRUMBSIZE	(475 * 1024)
//#define PARTSIZE	(9500 * 1024)

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

void
CPartStatus::ReadPartStatus(CSafeMemFile* const data, const bool defState)
{
	const uint64	size = GetSize();
	uint64	sctSize = PARTSIZE;
	uint16	sctDivider = 1;
	uint16	partCount, expectedPartCount;

	ASSERT(data != 0);
	
	partCount = data->ReadUInt16();
	// Special case
	if (partCount == 0)
	{
		if (defState == true)
			Set(0, size - 1);
		else
			Clear(0, size - 1);	
		return;
	}
	// Future versions of the protocol may send a partmap of size 1 to indicate it doesn't have any data to share yet!
	else if (partCount == 1)
	{
		if (data->ReadUInt8() & 0x01)
			Set(0, size - 1);
		else
			Clear(0, size - 1);	
		return;
	}
	// Check for Crumbs
	if ((UINT) partCount == (UINT) ((size + (CRUMBSIZE - 1)) / CRUMBSIZE)) //20 * (m_size / PARTSIZE) + (m_size % PARTSIZE + (CRUMBSIZE - 1)) / CRUMBSIZE)
	{
		DebugLog(_T(__FUNCTION__) _T("; Part vector has crumbs! :)"));
		sctSize = CRUMBSIZE;
		sctDivider = 20;
	}
	// Validate part count
	//expectedPartCount = (uint16) (size / sctSize + 1); //sctDivider * (m_size / PARTSIZE) + (m_size % PARTSIZE + (sctSize - 1)) / sctSize;
	expectedPartCount = (uint16) ((size + (sctSize - 1)) / sctSize); //sctDivider * (m_size / PARTSIZE) + (m_size % PARTSIZE + (sctSize - 1)) / sctSize;
	if (partCount != expectedPartCount)
	{
		CString error;
		error.Format(_T(__FUNCTION__) _T("; Invalid part count: %u, expected %u"), partCount, expectedPartCount);
		throw error;
	}
	// TODO: Check all bytes of the part status vector is available (final corruption check)
	// Read part status vector
	uint64	start = 0, stop = 0;
	bool	laststate = false;
	uint8	tmp = 0;
	for (uint16 i = 0; i < partCount; i++)
	{
		if (i % 8 == 0)
			tmp = data->ReadUInt8();
		if (((tmp >> (i % 8)) & 0x01) == 0)
		{
			if (laststate == true)
			{
				if (stop != 0)
					Set(start, stop);
				start = i * sctSize; //(i / sctDivider) * PARTSIZE + (i % sctDivider) * sctSize;
			}
			stop = (i + 1) * sctSize - 1; //((i + 1) / sctDivider) * PARTSIZE + ((i + 1) % sctDivider) * sctSize - 1;
			laststate = false;
		}
		else
		{
			if (laststate == false)
			{
				if (stop != 0)
					Clear(start, stop);
				start = i * sctSize; //(i / sctDivider) * PARTSIZE + (i % sctDivider) * sctSize;
			}
			stop = (i + 1) * sctSize - 1; //((i + 1) / sctDivider) * PARTSIZE + ((i + 1) % sctDivider) * sctSize - 1;
			laststate = true;
		}
	}
	stop = size - 1;
	if (start <= stop)
	{
		if (laststate == true) 
			Set(start, stop);
		else
			Clear(start, stop);
	}
}

void
CPartStatus::WritePartStatus(CSafeMemFile* const data, const bool defState, const int protocolRevision) const
{
	const uint64	size = GetSize();
	uint64	sctSize = PARTSIZE;
	uint16	sctCount;
	uint8	tmp;

	// Decide chunk size for part vector
	if (protocolRevision >= 1 && size < (441 * PARTSIZE) && GetChunkSize() < PARTSIZE)
		sctSize = CRUMBSIZE;

	sctCount = (uint16) ((size + sctSize - 1) / sctSize);

	// Special case when all parts have the same state
	if (IsComplete(0, size - 1) && defState == true)
	{
		data->WriteUInt16(0);
		return;
	}

	// Write parts
	data->WriteUInt16(sctCount);
	uint16 i;
	for (i = 0; i < sctCount; i++)
	{
		if (i % 8 == 0)
			tmp = 0;
		if (IsComplete((uint64) i * sctSize, (uint64) (i + 1) * sctSize - 1))
			tmp |= (1 << (i % 8));
		else
			tmp &= ~(1 << (i % 8));
		if (i % 8 == 7)
			data->WriteUInt8(tmp);
	}
	if (i % 8 != 0)
		data->WriteUInt8(tmp);
}

//
//	CAICHMap
//

CAICHMap::CAICHMap(const uint64 size)
{
	UINT const chunks_count = _GetAICHCount(size);
	m_size = size;
	m_chunks = new uint8[chunks_count];
	for (UINT i = 0; i < chunks_count; i++)
		m_chunks[i] = 0;
}

CAICHMap::CAICHMap(const CPartStatus* const source)
{
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

CAICHMap::~CAICHMap()
{
	delete[] m_chunks;
}

void
CAICHMap::Set(uint64 start, uint64 stop)
{
	ASSERT(start < m_size && start <= stop);
	if (m_size == 0)
		return;
	UINT const start_chunk = _GetAICHStart(start);
	UINT end_chunk;
	if (stop >= m_size - 1ULL)
		end_chunk =  _GetAICHCount(m_size);
	else
		end_chunk = _GetAICHEnd(stop);
	for (UINT i = start_chunk; i < end_chunk; i++)
		_SetAICH(i);
}

void
CAICHMap::Clear(uint64 start, uint64 stop)
{
	ASSERT(start < m_size && start <= stop);
	if (m_size == 0)
		return;
	UINT const start_chunk = _GetAICHStart(start);
	UINT end_chunk;
	if (stop >= m_size - 1ULL)
		end_chunk =  _GetAICHCount(m_size);
	else
		end_chunk = _GetAICHEnd(stop);
	for (UINT i = start_chunk; i < end_chunk; i++)
		_ClearAICH(i);
}

bool
CAICHMap::IsComplete(uint64 start, uint64 stop) const
{
	ASSERT(start < m_size && start <= stop);
	if (m_size == 0)
		return true;
	if (stop >= m_size)
		stop = m_size - 1ULL;
	UINT const start_chunk = _GetAICHStart(start);
	UINT const end_chunk = _GetAICHEnd(stop);
	for (UINT i = start_chunk; i < end_chunk; i++)
		if (!_IsCompleteAICH(i)) return false;
	return true;
}

bool
CAICHMap::FindFirstComplete(uint64& start, uint64& stop) const
{
	if (m_size == 0 || start >= m_size || start > stop)
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
		stop = min(stop, (uint64) (last_chunk / 53ULL) * PARTSIZE + (last_chunk % 53ULL) * EMBLOCKSIZE) - 1ULL;
	}
	return bComplete;
}

bool
CAICHMap::FindFirstNeeded(uint64& start, uint64& stop) const
{
	if (m_size == 0 || start >= m_size || start > stop)
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
		stop = min(stop, (uint64) (last_chunk / 53ULL) * PARTSIZE + (last_chunk % 53ULL) * EMBLOCKSIZE) - 1ULL;
	}
	return bNeeded;
}

//
//	CCrumbMap
//

CCrumbMap::CCrumbMap(const uint64 size)
{
	UINT const chunks_count = (UINT) ((size + (CRUMBSIZE - 1)) / CRUMBSIZE);
	m_size = size;
	m_chunks = new uint8[chunks_count];
	for (UINT i = 0; i < chunks_count; i++)
		m_chunks[i] = 0;
}

CCrumbMap::CCrumbMap(const CPartStatus* const source)
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

CCrumbMap::~CCrumbMap()
{
	delete[] m_chunks;
}

void
CCrumbMap::Set(uint64 start, uint64 stop)
{
	ASSERT(start < m_size && start <= stop);
	UINT	start_chunk, end_chunk;
	if (m_size == 0)
		return;
	start_chunk = (UINT) ((start + (CRUMBSIZE - 1ULL)) / CRUMBSIZE);
	if (stop >= m_size - 1ULL)
		end_chunk = (UINT) ((m_size - 1ULL) / CRUMBSIZE + 1ULL);
	else
		end_chunk = (UINT) ((stop + 1ULL) / CRUMBSIZE);
	for (UINT i = start_chunk; i < end_chunk; i++)
		_SetCrumb(i);
}

void
CCrumbMap::Clear(uint64 start, uint64 stop)
{
	ASSERT(start < m_size && start <= stop);
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
CCrumbMap::IsComplete(uint64 start, uint64 stop) const
{
	ASSERT(start < m_size && start <= stop);
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
CCrumbMap::FindFirstComplete(uint64& start, uint64& stop) const
{
	if (m_size == 0 || start >= m_size || start > stop)
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
CCrumbMap::FindFirstNeeded(uint64& start, uint64& stop) const
{
	if (m_size == 0 || start >= m_size || start > stop)
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
CCrumbMap::GetNeeded(const CPartStatus* const from) const
{
	uint64	needed = 0;
	for (int i=0; i<_GetCrumbsCount(); i++)
	{
		if (from->IsCompleteCrumb(i) == !_IsCompleteCrumb(i))
			needed += CRUMBSIZE;
	}
	return needed;
}*/

uint64	CPartFileStatus::GetSize() const 
{
	return m_file->GetFileSize();
}

bool	CPartFileStatus::IsComplete(uint64 start, uint64 stop) const 
{
	return m_file->IsComplete(start, stop, false);
}