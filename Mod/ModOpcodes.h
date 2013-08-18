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

#define MINMAX(val, mini, maxi)		{val = (min(max(mini, val), maxi));}

//>>> WiZaRd::Ratio Indicator
static const CString strRatioSmilies[] =
{
    L"SMILEY_CRY",
    L"SMILEY_SAD",
    L"SMILEY_LOOKSIDE",
    L"SMILEY_DISGUST",
    L"SMILEY_SKEPTIC",
    L"SMILEY_SMILE",
    L"SMILEY_WINK",
    L"SMILEY_HAPPY",
    L"SMILEY_LAUGH",
};
static const UINT ratioSmileyCount = _countof(strRatioSmilies) - 1;
//<<< WiZaRd::Ratio Indicator

//>>> PreviewIndicator [WiZaRd]
enum ePreviewIndicatorMode
{
    ePIM_Disabled = 0,
    ePIM_Icon,
    ePIM_Color
};
//<<< PreviewIndicator [WiZaRd]

//>>> WiZaRd::AntiFake
enum eSpamFilterMode
{
    eSFM_Disabled = 0,	// off
    eSFM_Colored,		// color only
    eSFM_AutoSort,		// sort automatically (default)
    eSFM_Filter			// filter suspect entries
};
//<<< WiZaRd::AntiFake

//>>> WiZaRd::SharePermissions
enum eSharePermissions
{
    eSP_Options = 0,
    eSP_Everybody,
    eSP_Friends,
    eSP_Nobody,
};
//<<< WiZaRd::SharePermissions

#define FDC_SENSITIVITY		83				//>>> FDC [BlueSonicBoy]
#define FT_AUTOHL		"AUTOHL"			//>>> WiZaRd::AutoHL

//>>> WiZaRd::Check DiskSpace
#define RESERVE_MAX			1024			//MB
#define RESERVE_MB			10				//MB
#define RESERVE_PERCENT		5				//%
//<<< WiZaRd::Check DiskSpace

#define FT_POWERSHARE			"ZZUL_POWERSHARE" //>>> WiZaRd::PowerShare
#define FT_SOTN					"SOTN"		//>>> WiZaRd::Intelligent SOTN
#define FT_FOLDER				"FOLDER"	//>>> WiZaRd::CollectionEnhancement

//>>> WiZaRd::ICS [enkeyDEV]
#define ET_INCOMPLETEPARTS		0x3D
#define OP_FILEINCSTATUS		0x8E		//Incomplete part packet (like OP_FILESTATUS)
//<<< WiZaRd::ICS [enkeyDEV]

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
#define FORCE_AICH_TIME					MIN2MS(5)
#define RESCAN_SHAREABLE_INTERVAL		MIN2MS(1)
#define EMBLOCKSPERPART					53			//! Number of AICH blocks fitting in one part
#define CRUMBSIZE						486400ui64  //! Chunk size of revised protocol (475kB)
#define CRUMBSPERPART					20			//! Number of crumbs fitting in one part

// eDonkey hybrid op-codes
#define OP_HORDESLOTREQ			0x65		// <HASH (file) [16]>
#define OP_HORDESLOTREJ			0x66		// <HASH (file) [16]>
#define OP_HORDESLOTANS			0x67		// <HASH (file) [16]>
#define OP_CRUMBSETANS					0x68		// <HASH (file) [16]> <HasCrumbSet [1]> <HASH (crumb) [8]>*(CrumbCNT)  for files containing only one part
// or <HASH (file) [16]> <HasPartHashSet [1]> <HASH (part) [8]>*(PartCNT) <HasCrumbSet [1]> <HASH (crumb) [8]>*(CrumbCNT)  for larger files

#define OP_CRUMBSETREQ			0x69		// <HASH (file) [16]>
#define OP_CRUMBCOMPLETE		0x6A		// <HASH (file) [16]> <Crumb [4]>
#define OP_PUBLICIPNOTIFY		0x6B		// <IP (receiver) [4]>   Hybrids sends this when IP in Hello packet doesn't match

#define OP_CRUMBCOMPLETE		0x6A		// <HASH (file) [16]> <Crumb [4]>
#define CT_PROTOCOLREVISION		"pr"		//! Used to identify crumbs support
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

//>>> WiZaRd::ModProt
//opcodes
#define	OP_MODPROT				0x4D	//'M'
#define	OP_MODPROT_PACKED		0x6D	//'m'
#define	OP_MODINFOPACKET		0x01
#define OP_MODMULTIPACKET 		0x11
#define OP_MODMULTIPACKETANSWER 0x12

//tags
#define MISC_PROTOCOL_EXTENSIONS 	0x4D
#define MISC_PROTOCOL_EXTENSIONS2 	0x6D
#define KAD_EMULE_BUDDYID			0x40
#define XS_EMULE_BUDDYIP			0x42
#define XS_EMULE_BUDDYUDP			0x62
#define NEO_PROTOCOL_EXTENSIONS 	0x4E
#define NEO_PROTOCOL_EXTENSIONS2 	0x6E
//<<< WiZaRd::ModProt
//>>> WiZaRd::NatTraversal [Xanatos]
#define CT_EMULE_BUDDYID			0xBF
#define OP_HOLEPUNCH				0xA1
//<<< WiZaRd::NatTraversal [Xanatos]
//>>> WiZaRd::ExtendedXS [Xanatos]
#define CT_EMULE_ADDRESS			0xB0
#define CT_EMULE_SERVERIP			0xBA
#define CT_EMULE_SERVERTCP			0xBB
#define CT_EMULE_CONOPTS			0xBE
#define CT_EMULE_BUDDYID			0xBF
//>>> WiZaRd::IPv6 [Xanatos]
#define CT_NEOMULE_YOUR_IP			0xAD
#define CT_NEOMULE_IP_V6			0xAE
#define	TAG_IPv6					"ip6"	// Unfirewalled IPv6 
//<<< WiZaRd::IPv6 [Xanatos]

#define	SOURCEEXCHANGEEXT_VERSION	1
//<<< WiZaRd::ExtendedXS [Xanatos]