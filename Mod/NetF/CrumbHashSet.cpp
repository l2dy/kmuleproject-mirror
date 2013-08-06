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
#include "CrumbHashSet.h"
#include "SafeFile.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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