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
#include "Modname.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//>>> WiZaRd::Easy ModVersion
CString GetModVersionNumber()
{
    static CString strVersion = L"";
    if(strVersion.IsEmpty())
    {
        //strVersion.Format(L"%u.%02u", MOD_VERSION_MJR, MOD_VERSION_MIN);
        strVersion = MOD_VERSION_BUILD;
    }
    return strVersion;
}
CString GetModVersion()
{
    static CString strMod = L"";
    if(strMod.IsEmpty())
        strMod.Format(L"%s Build%s", MOD_VERSION_PLAIN, GetModVersionNumber());
    return strMod;
}
//<<< WiZaRd::Easy ModVersion