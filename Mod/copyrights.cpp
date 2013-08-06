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
#include "StdAfx.h"
#include "copyrights.h"
#include "emule.h"
#include "Log.h"
#include "Version.h"
#include "./Mod/7z/7z_include.h" //>>> WiZaRd::7zip	
#include ".\miniupnpc\miniupnpcstrings.h"
#ifdef USE_NAT_PMP
#include "./Mod/NATPMPWrapper.h"  //>>> WiZaRd::NAT-PMP
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void PrintCopyrights()
{
	theApp.QueueDebugLogLineEx(LOG_WARNING, L"** Copyright Information:");
	theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"%s (c) 2012-2013 %s Team (tuxman/WiZaRd) - %s - Build%s", MOD_VERSION_PLAIN, MOD_VERSION_PLAIN, MOD_HOMEPAGE, GetModVersionNumber());
	theApp.QueueDebugLogLineEx(LOG_WARNING, L"%s is using code from:", MOD_VERSION_PLAIN);
	theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"eMule (c) 2002-2009 Merkur - http://www.emule-project.net/ - v%u.%u%c",VERSION_MJR, VERSION_MIN, L'a' + VERSION_UPDATE);
	theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"%s - 7z.org - %s (%s)", MY_COPYRIGHT, kCopyrightString, MY_7ZIP_VERSION, MY_DATE); //>>> WiZaRd::7zip	
	theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"miniupnpc (c) 2006-2009 Thomas Bernard - http://miniupnp.free.fr/ - Build %hs (%hs)", MINIUPNPC_VERSION_STRING, OS_STRING);
#ifdef USE_NAT_PMP
	theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"libnatpmp (c) 2007-2012 Thomas Bernard - http://miniupnp.free.fr/ - Build %s", LIBNAT_VERSION_STRING); //>>> WiZaRd::NAT-PMP
#endif
	theApp.QueueDebugLogLineEx(LOG_WARNING, L"**");
}
