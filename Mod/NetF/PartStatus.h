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
#include "opcodes.h"

class CSafeMemFile;
class CPartFile;

class CPartStatus
{
public:
	// Destructor
	CPartStatus() {}
	virtual			~CPartStatus() {}
	// Manipulation routines
	virtual void	Set(uint64 start, uint64 stop) = 0;
	virtual void	Clear(uint64 start, uint64 stop) = 0;
	// 
	virtual uint64	GetSize() const = 0;
	virtual uint64	GetChunkSize() const = 0;
	virtual bool	IsComplete(uint64 start = 0, uint64 stop = ~0ULL) const = 0;
	virtual bool	FindFirstComplete(uint64& start, uint64& stop) const = 0;
	virtual bool	FindFirstNeeded(uint64& start, uint64& stop) const = 0;
	// Bytes
	virtual uint64	GetCompleted(uint64 start = 0, uint64 stop = ~0ULL) const;
	virtual uint64	GetNeeded(uint64 start = 0, uint64 stop = ~0ULL) const;
	virtual uint64	GetNeeded(const CPartStatus* from) const;
	virtual bool	IsNeeded(uint64 start = 0, uint64 stop = ~0ULL) const {return GetNeeded(start, stop) > 0;}
	virtual bool	IsNeeded(const CPartStatus* from) const {return GetNeeded(from) > 0;}
	virtual bool	FindFirstNeeded(uint64& start, uint64& stop, const CPartStatus* from) const;
	// AICH chunks
	virtual void	SetAICH(UINT chunk) {uint64 startOffset = (uint64) (chunk / 53) * PARTSIZE + (chunk % 53) * EMBLOCKSIZE; Set(startOffset, ((chunk % 53) != 52 ? startOffset + EMBLOCKSIZE - 1ULL : ((chunk + 1) / 53) * PARTSIZE));}
	virtual void	ClearAICH(UINT chunk) {uint64 startOffset = (uint64) (chunk / 53) * PARTSIZE + (chunk % 53) * EMBLOCKSIZE; Clear(startOffset, ((chunk % 53) != 52 ? startOffset + EMBLOCKSIZE - 1ULL : ((chunk + 1) / 53) * PARTSIZE));}
	virtual UINT	GetAICHCount() const throw() {return static_cast<UINT>(53ULL * GetSize() / PARTSIZE + ((GetSize() % PARTSIZE) + EMBLOCKSIZE - 1ULL) / EMBLOCKSIZE);}
	virtual bool	IsCompleteAICH(UINT chunk) const {uint64 startOffset = (uint64) (chunk / 53) * PARTSIZE + (chunk % 53) * EMBLOCKSIZE; return IsComplete(startOffset, ((chunk % 53) != 52 ? startOffset + EMBLOCKSIZE - 1ULL : ((chunk + 1) / 53) * PARTSIZE));}
	// Crumbs
	virtual void	SetCrumb(UINT crumb) {Set((uint64) crumb * CRUMBSIZE, (uint64) (crumb + 1) * CRUMBSIZE - 1ULL);}
	virtual void	ClearCrumb(UINT crumb) {Clear((uint64) crumb * CRUMBSIZE, (uint64) (crumb + 1) * CRUMBSIZE - 1ULL);}
	virtual UINT	GetCrumbsCount() const throw() {return (UINT) ((uint64) (GetSize() + CRUMBSIZE - 1ULL) / CRUMBSIZE);}
	virtual bool	IsCompleteCrumb(UINT crumb) const {return IsComplete((uint64) crumb * CRUMBSIZE, (uint64) (crumb + 1) * CRUMBSIZE - 1ULL);}
	// Parts
	virtual void	SetPart(UINT part) {Set((uint64) part * PARTSIZE, (uint64) (part + 1) * PARTSIZE - 1ULL);}
	virtual void	ClearPart(UINT part) {Clear((uint64) part * PARTSIZE, (uint64) (part + 1) * PARTSIZE - 1ULL);}
	virtual UINT	GetPartCount() const throw() {return (UINT) ((uint64) (GetSize() + PARTSIZE - 1ULL) / PARTSIZE);}
	virtual UINT	GeteD2KPartCount() const throw() {return (UINT) ((uint64) (GetSize() / PARTSIZE + 1));}
	virtual bool	IsCompletePart(UINT part) const {return IsComplete((uint64) part * PARTSIZE, (uint64) (part + 1) * PARTSIZE - 1ULL);}
	// Read/Write part status vectors
	virtual void	ReadPartStatus(CSafeMemFile* data, bool defState);
	virtual void	WritePartStatus(CSafeMemFile* data, bool defState, int protocolRevision = 0) const;
	// Special values
	enum {
		PROTOCOL_REVISION = 1
	};
};

class CPartFileStatus : public CPartStatus
{
public:
	//
	CPartFileStatus(CPartFile* const file):m_file(file) {}
	~CPartFileStatus() {}
	//
private:
	void	Set(uint64, uint64) {}	// Not used! (Should throw exception?)
	void	Clear(uint64, uint64) {}	// Not used! (Should throw exception?)
public:
	uint64	GetSize() const;
	uint64	GetChunkSize() const {return 1;}
	bool	IsComplete(uint64 start, uint64 stop) const;
	bool	FindFirstComplete(uint64& start, uint64& stop) const;
	bool	FindFirstNeeded(uint64& start, uint64& stop) const;
private:
	CPartFile* const	m_file;
};

class CAICHMap : public CPartStatus
{
public:
	// Constructors / Destructor
			CAICHMap(uint64 size);
			CAICHMap(const CPartStatus* source);
			~CAICHMap();
	// Bytes
	uint64	GetSize() const throw() {return m_size;}
	uint64	GetChunkSize() const throw() {return EMBLOCKSIZE;}
	void	Set(uint64 start, uint64 stop);
	void	Clear(uint64 start, uint64 stop);
	bool	IsComplete(uint64 start, uint64 stop) const;
	bool	FindFirstComplete(uint64& start, uint64& stop) const;
	bool	FindFirstNeeded(uint64& start, uint64& stop) const;
	// AICH
	void	SetAICH(UINT chunk) {ASSERT(chunk < _GetAICHCount()); _SetAICH(chunk);}
	void	ClearAICH(UINT chunk) {ASSERT(chunk < _GetAICHCount()); _ClearAICH(chunk);}
	UINT	GetAICHCount() const throw() {return _GetAICHCount();}
	bool	IsCompleteAICH(UINT chunk) const {ASSERT(chunk < _GetAICHCount()); return _IsCompleteAICH(chunk);}
private:
	void	_SetAICH(const UINT chunk) throw() {m_chunks[chunk >> 3] |= (1 << (chunk & 0x7));}
	void	_ClearAICH(const UINT chunk) throw() {m_chunks[chunk >> 3] &= ~(1 << (chunk & 0x7));}
	UINT	_GetAICHCount() const throw() {return _GetAICHCount(m_size);}
	UINT	_GetAICHCount(const uint64 size) const throw() {return static_cast<UINT>(53ULL * size / PARTSIZE + ((size % PARTSIZE) + EMBLOCKSIZE - 1ULL) / EMBLOCKSIZE);}
	UINT	_GetAICHStart(const uint64 start) const throw() {return static_cast<UINT>(53ULL * start / PARTSIZE + ((start % PARTSIZE) + EMBLOCKSIZE - 1ULL) / EMBLOCKSIZE);}
	UINT	_GetAICHEnd(const uint64 stop) const throw() {return static_cast<UINT>(53ULL * stop / PARTSIZE + ((stop % PARTSIZE) + 1ULL) / EMBLOCKSIZE);}
	bool	_IsCompleteAICH(const UINT chunk) const throw() {return ((m_chunks[chunk >> 3] >> (chunk & 0x7)) & 0x1) != 0;}
	uint64	m_size;
	uint8*	m_chunks;
};

class CCrumbMap : public CPartStatus
{
public:
	// Constructors / Destructor
			CCrumbMap(uint64 size);
			CCrumbMap(const CPartStatus* source);
			~CCrumbMap();
	// Bytes
	uint64	GetSize() const throw() {return m_size;}
	uint64	GetChunkSize() const throw() {return CRUMBSIZE;}
	void	Set(uint64 start, uint64 stop);
	void	Clear(uint64 start, uint64 stop);
	bool	IsComplete(uint64 start, uint64 stop) const;
	bool	FindFirstComplete(uint64& start, uint64& stop) const;
	bool	FindFirstNeeded(uint64& start, uint64& stop) const;
	// Crumbs
	void	SetCrumb(UINT crumb) {ASSERT(crumb < _GetCrumbsCount()); _SetCrumb(crumb);}
	void	ClearCrumb(UINT crumb) {ASSERT(crumb < _GetCrumbsCount()); _ClearCrumb(crumb);}
	UINT	GetCrumbsCount() const throw() {return _GetCrumbsCount();}
	bool	IsCompleteCrumb(UINT crumb) const {ASSERT(crumb < _GetCrumbsCount()); return _IsCompleteCrumb(crumb);}
private:
	void	_SetCrumb(const UINT crumb) throw() {m_chunks[crumb >> 3] |= (1 << (crumb & 0x7));}
	void	_ClearCrumb(const UINT crumb) throw() {m_chunks[crumb >> 3] &= ~(1 << (crumb & 0x7));}
	UINT	_GetCrumbsCount() const throw() {return (UINT) ((uint64) (m_size + CRUMBSIZE - 1ULL) / CRUMBSIZE);}
	bool	_IsCompleteCrumb(const UINT crumb) const throw() {return ((m_chunks[crumb >> 3] >> (crumb & 0x7)) & 0x1) != 0;}
	uint64	m_size;
	uint8*	m_chunks;
};

/*class CPartMap : public CPartStatus
{
public:
	// Constructors / Destructor
			CPartMap(uint64 size);
			CPartMap(const CPartStatus* source);
			~CPartMap();
	// Bytes
	uint64	GetSize() const throw() {return m_size;}
	uint64	GetChunkSize() const throw() {return PARTSIZE;}
	void	Set(uint64 start, uint64 stop);
	void	Clear(uint64 start, uint64 stop);
	bool	IsComplete(uint64 start, uint64 stop) const;
	bool	FindFirstComplete(uint64& start, uint64& stop) const;
	bool	FindFirstNeeded(uint64& start, uint64& stop) const;
	// Parts
	void	SetPart(UINT part) {ASSERT(part < _GetPartsCount()); _SetPart(part);}
	void	ClearPart(UINT part) {ASSERT(part < _GetPartsCount()); _ClearPart(part);}
	UINT	GetPartsCount() const throw() {return _GetPartCount();}
	bool	IsCompletePart(UINT part) const {ASSERT(part < _GetPartsCount()); return _IsCompletePart(part);}
private:
	void	_SetPart(const UINT part) throw() {m_chunks[part >> 3] |= (1 << (part & 0x7));}
	void	_ClearPart(const UINT part) throw() {m_chunks[part >> 3] &= ~(1 << (part & 0x7));}
	UINT	_GetPartsCount() const throw() {return (UINT) ((uint64) (m_size + PARTSIZE - 1ULL) / PARTSIZE);}
	bool	_IsCompletePart(const UINT part) const throw() {return ((m_chunks[part >> 3] >> (part & 0x7)) & 0x1) != 0;}
	uint64	m_size;
	uint8*	m_chunks;
};*/