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

//>>> WiZaRd::SharePermissions
enum eSharePermissions
{
    eSP_Options = 0,
    eSP_Everybody,
    eSP_Friends,
    eSP_Nobody,
};
//<<< WiZaRd::SharePermissions

#define FDC_SENSITIVITY		83	//>>> FDC [BlueSonicBoy]

//>>> WiZaRd::Check DiskSpace
#define RESERVE_MAX			1024 //MB
#define RESERVE_MB			10 //MB
#define RESERVE_PERCENT		5  //%
//<<< WiZaRd::Check DiskSpace

#define FT_POWERSHARE			"ZZUL_POWERSHARE" //>>> WiZaRd::PowerShare
#define FT_SOTN					"SOTN" //>>> WiZaRd::Intelligent SOTN
#define FT_FOLDER				"FOLDER" //>>> WiZaRd::CollectionEnhancement

//>>> WiZaRd::ICS [enkeyDEV]
#define ET_INCOMPLETEPARTS		0x3D
#define OP_FILEINCSTATUS		0x8E //Incomplete part packet (like OP_FILESTATUS)
//<<< WiZaRd::ICS [enkeyDEV]