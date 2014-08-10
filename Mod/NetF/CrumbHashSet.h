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

class CCrumbHashSet
{
  public:
    CCrumbHashSet(UINT crumbCount, CSafeMemFile* data = NULL);
    ~CCrumbHashSet();
    const unsigned char*	GetSHAHash() const;
    const unsigned char*	GetCrumbHash(UINT index) const;
    void					SetCrumbHash(UINT index, unsigned char* hash);
  private:
    unsigned char*	m_CrumbHashSet;
    UINT			m_CrumbCount;
    unsigned char	m_SHAHash[CRUMBSPERPART];
    bool			m_SHAHashValid;
};

inline
const unsigned char* CCrumbHashSet::GetCrumbHash(UINT index) const
{
    if (index >= m_CrumbCount)
        throw CString(_T(__FUNCTION__) _T("; index out of range"));
    return (m_CrumbHashSet + index * 8);
}

inline
void CCrumbHashSet::SetCrumbHash(UINT index, unsigned char* hash)
{
    if (index >= m_CrumbCount)
        throw CString(_T(__FUNCTION__) _T("; index out of range"));
    m_SHAHashValid = false;
    memcpy(m_CrumbHashSet + (index * 8), hash, 8);
}