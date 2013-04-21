//////////////////////////////////////////////////////////////////////////
//ModIconDLL
// 
//Copyright (C)2006 WiZaRd ( thewizardofdos@gmail.com / http://kademlia-net.de )
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
//
//////////////////////////////////////////////////////////////////////////
#pragma once

#define MODMAP_NONE		(-1)
#define MODMAP_BADGUY	(0)

#define DFLT_MODICONDLL_FILENAME	L"ModIconDLL.dll"

class CModIconMapper
{
public:
	CModIconMapper();
	~CModIconMapper();

	bool	IsDLLavailable()		{return m_bDLLAvailable;}
	void	Reload();
	void	Load(const CString& strDLLToLoad);
	void	Update();
	void	Unload();

	BOOL	DrawModIcon(CDC* pDC, const int& nImage, POINT pt, const UINT& nStyle);
	CImageList*	GetModImageList() {return &modimagelist;}; //>>> JvA::Redesigned Client Detail Dialog [BlueSonicBoy]

public:
	DWORD	GetVersion() const;
	CString	GetVersionString() const;

private:
	typedef DWORD (__cdecl *GETDLLVERSION)();
	GETDLLVERSION DLLGetVersion;
	
public:
	void	FillModIconList();
private:
	typedef int (__cdecl *DLLFILLMODICONLIST)(CImageList* pImageList);
	DLLFILLMODICONLIST DLLFillModIconList;

public:
	int		GetIconIndexForModstring(const CString& strMod);
private:
	typedef int (__cdecl *DLLGETICONINDEXFORMODSTRING)(const CString& strMessage);
	DLLGETICONINDEXFORMODSTRING DLLGetIconIndexForModstring;

public:
	void	DumpIconList() const;
private:
	typedef CString (__cdecl *DLLDUMPICONLIST)();
	DLLDUMPICONLIST DLLDumpIconList;

private:
	HINSTANCE	m_hDLLInstance;
	bool		m_bDLLAvailable;
	CImageList modimagelist;
};