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

class CProgressIndicator
{
public:
	CProgressIndicator();
	void Initialise(const CString& strFilePath, const CString& strProgressString, const CString& strFinishString);
	void UpdateProgress(const double& progress);
	~CProgressIndicator();

private:
	CString m_strFilePath;
	CString m_strProgressString;
	CString	m_strFinishString;

	UINT	m_uLastProgress;
	UINT	m_uProgress;
	double	m_dProgress;
};