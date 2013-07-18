/**
 *
  */
#include "stdafx.h"
#include "CrumbHashSet.h"
#include "SafeFile.h"
#include "Log.h"

CCrumbHashSet::CCrumbHashSet(UINT crumbCount, CSafeMemFile* data)
{
	m_SHAHashValid = false;
	m_CrumbCount = crumbCount;
	m_CrumbHashSet = new unsigned char[crumbCount * 8];
	if (data)
	{
		data->Read(m_CrumbHashSet, crumbCount * 8);
	}
	else
	{
		memset(m_CrumbHashSet, 0, crumbCount * 8);
	}
}

CCrumbHashSet::~CCrumbHashSet()
{
	delete[] m_CrumbHashSet;
}