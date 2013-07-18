/**
 *
  */
#ifndef	CRUMBHASHSET_H
#define	CRUMBHASHSET_H

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
	unsigned char	m_SHAHash[20];
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

#endif	//CRUMBHASHSET_H
