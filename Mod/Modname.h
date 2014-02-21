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

#define DEFAULT_NICK		L"kMule User @ http://www.emulefuture.de"
#define MODDER_MAIL			L"thewizardofdos@gmail.com"

#define MOD_AUTOUPDATE_URL	L"http://kMule.emulefuture.de/update/"
#define MOD_IPPFILTER_URL	L"http://ip-filter.emulefuture.de/download.php?file=ipfilter.zip"
#define MOD_MEDIAINFO_URL	L"http://mediainfo.emulefuture.de/download.php?file=MediaInfo.zip"
#define MOD_NODES_URL		L"http://server-met.emulefuture.de/download.php?file=nodes.dat"
#define MOD_MODICON_URL		L"http://modicon.emulefuture.de/"
#define MOD_HOMEPAGE		L"http://www.emulefuture.de"
#define MOD_BASE_HOMEPAGE	L"http://www.emule-project.net"
#define MOD_WIKI			L"http://kMule.emulefuture.de/wiki/"
#define MOD_LANG_URL		L"http://kMule.emulefuture.de/lang/%u/"
#define MOD_DOWNLOAD_PAGE	L"http://sourceforge.net/projects/kmuleproject/"

#define SNARL_APP_TAG		L"kMule" //>>> TuXaRd::SnarlSupport
//>>> WiZaRd::Easy ModVersion
#define MOD_VERSION_PLAIN	(L"kMule")
#define MOD_VERSION			GetModVersion()
//#define MOD_VERSION_MJR		0
//#define MOD_VERSION_MIN		1
#define MOD_VERSION_BUILD	L"20140221"
// has to be changed on lang updates - also remember to recreate and upload the new lang files :)
#define MOD_LANG_VERSION	2 // just to be sure so everybody receives our updates
CString GetModVersionNumber();
CString GetModVersion();
#define MOD_INI_FILE		(L"kMule") //this is the tag used as pref-entry
//<<< WiZaRd::Easy ModVersion