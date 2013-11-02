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
#include "emule.h"
#include "./Mod/ModProt.h"
#include "./Mod/ModOpcodes.h"
#include "./Mod/ModName.h"
#include "Exceptions.h"
#include "updownclient.h"
#include "ListenSocket.h"
#include "ClientUDPSocket.h"
#include "ClientList.h"
#include "Preferences.h"
#include "packets.h"
#include "Statistics.h"
#include "otherfunctions.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CUpDownClient::SendModInfoPacket() const
{
    if (socket == NULL) // first we check if we have a valid socket, actually called from the socket so this check is rather unnecessary, but in case...
    {
        ASSERT(0);
        return;
    }

    CList<CTag*> tagList; // netfinity: Add tags to a linked list to prevent mismatch
    tagList.AddTail(new CTag(CT_MOD_VERSION, MOD_VERSION));
	
	// Misc mod community features	
	const UINT uSupportsUPS			= 1; //>>> WiZaRd::Unsolicited PartStatus [Netfinity]
	const UINT uSupportsIPv6		= 1; //>>> WiZaRd::IPv6 [Xanatos]
	const UINT uSupportsExtXS		= 1; //>>> WiZaRd::ExtendedXS [Xanatos]	
	tagList.AddTail(new CTag(CT_EMULE_MISCOPTIONS1,
		(uSupportsUPS			<<  2) |	//>>> WiZaRd::Unsolicited PartStatus [Netfinity]
		(uSupportsIPv6			<<  1) |	//>>> WiZaRd::IPv6 [Xanatos]
		(uSupportsExtXS			<<  0)		//>>> WiZaRd::ExtendedXS [Xanatos]
		));


    // we are done writing tags... flush them into the packet
    CSafeMemFile data(128);
    data.WriteUInt32(tagList.GetCount()); // nr. of tags
    while (!tagList.IsEmpty())
    {
        CTag* emTag = tagList.RemoveHead();
        if (emTag)
            emTag->WriteNewEd2kTag(&data);
        delete emTag;
    }

    // now we create a packet and put what we have written into it
    Packet* packet = new Packet(&data, OP_MODPROT);
    packet->opcode = OP_MODINFOPACKET;
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__ModInfoPacket", this);
#endif
    theStats.AddUpDataOverheadOther(packet->size);
    socket->SendPacket(packet, true, true); // we send the packet
}

void CUpDownClient::ProcessModInfoPacket(const uchar* pachPacket, const UINT nSize)
{
    CSafeMemFile data(pachPacket, nSize);
    // we read now the amount of tags that the sender put in the packet
    const UINT tagcount = data.ReadUInt32();

#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    const bool bDbgInfo = thePrefs.GetUseDebugDevice();
#endif
    CString m_strModInfo = L"";
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    if (bDbgInfo)
        m_strModInfo.AppendFormat(L"  Tags=%u", tagcount);
#endif

    // now we are going to read all of them
    for (UINT i = 0; i < tagcount; ++i)
    {
        CTag temptag(&data, false);
        switch (temptag.GetNameID()) // here we distinguish only tags with an uint8 ID, all named tags are handled below
        {
			case CT_MOD_VERSION:
			{
				if (temptag.IsStr())
					m_strModVersion = temptag.GetStr();
				else if (temptag.IsInt())
					m_strModVersion.Format(L"%u", temptag.GetInt());
				else
					m_strModVersion = L"<Unknown>";
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
				if (bDbgInfo)
					m_strModInfo.AppendFormat(L"\n  ModID=%s", m_strModVersion);
#endif
				break;
			}

			case CT_EMULE_MISCOPTIONS1:
			{
				//  1 UPS support
				//	1 IPv6 support
				//	1 ExtendedXS
				if (temptag.IsInt())
				{
					m_fSupportsUnsolicitedPartStatus = (temptag.GetInt() >>  2) & 0x01; //>>> WiZaRd::Unsolicited PartStatus [Netfinity]
					m_fSupportsIPv6			= (temptag.GetInt() >>  1) & 0x01; //>>> WiZaRd::IPv6 [Xanatos]
					m_fSupportsExtendedXS	= (temptag.GetInt() >>  0) & 0x01; //>>> WiZaRd::ExtendedXS [Xanatos]
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
					if (bDbgInfo)
						m_strModInfo.AppendFormat(L"\n  UPS=%u  IPv6=%u  ExtendedXS=%u", m_fSupportsUnsolicitedPartStatus, m_fSupportsIPv6, m_fSupportsExtendedXS);
#endif
				}
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
				else if (bDbgInfo)
					m_strModInfo.AppendFormat(L"\n  ***UnkType=%s", temptag.GetFullInfo());
#endif
				break;
			}

			default:
			{
				// now we handle custom named tags, we use a series of (else)if's and strcmp, note: this is *not* unicode!!!
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
				if (bDbgInfo)
					m_strModInfo.AppendFormat(L"\n  ***UnkTag=%s", temptag.GetFullInfo());
#endif
				break;
			}
        }
    }

#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    // some debug output
    if (bDbgInfo && data.GetPosition() < data.GetLength())
        m_strModInfo.AppendFormat(L"\n  ***AddData: %u bytes", data.GetLength() - data.GetPosition());
#endif

	InitClientSoftwareVersion();
}

bool CClientReqSocket::ProcessModPacket(const BYTE* packet, const UINT size, const UINT opcode, const UINT uRawSize)
{
    //const UINT protocol = OP_MODPROT;
    try
    {
        try
        {
            // we must have a client at this point !always!
            if (!client)
            {
                theStats.AddDownDataOverheadOther(uRawSize);
                throw GetResString(IDS_ERR_UNKNOWNCLIENTACTION);
            }
            if (thePrefs.m_iDbgHeap >= 2)
                ASSERT_VALID(client);

            switch (opcode) // dispatch particular packets
            {
                // for the mod prot v1 (capability exchange) only this one packet is relevant, the mod info packet
            case OP_MODINFOPACKET:
            {
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
                if (thePrefs.GetDebugClientTCPLevel() > 0)
                    DebugRecv("OP__ModInfoPacket", client);
#endif
                theStats.AddDownDataOverheadOther(uRawSize);

                if (!client->SupportsModProt()) // just pure formality
                    throw CString(L"Received Mod Info Packet from a client which does not support this feature!");

                // Process the packet
                client->ProcessModInfoPacket(packet, size);

                // Now after we have all informations about the client the connection is established
                client->ConnectionEstablished();

                // start secure identification, if
                //  - we have received OP_MODINFOPACKET, (Mod prot compatible new eMule mods)
                if (client->GetInfoPacketsReceived() == IP_BOTH)
                    client->InfoPacketsReceived();
                break;
            }

            default: // handle unknown packets
            {
                theStats.AddDownDataOverheadOther(uRawSize);
                PacketToDebugLogLine(L"Mod", packet, size, opcode);
                break;
            }
            }
        }
        catch (CFileException* error)
        {
            error->Delete();
            throw GetResString(IDS_ERR_INVALIDPACKAGE);
        }
        catch (CMemoryException* error)
        {
            error->Delete();
            throw CString(L"Memory exception");
        }
    }
    // Note: don't disconnect clients on mod prot errors, the extensions are just addons and if they fail the client will work anyway
    catch (CClientException* ex) // nearly same as the 'CString' exception but with optional deleting of the client
    {
        if (thePrefs.GetVerbose() && !ex->m_strMsg.IsEmpty())
            DebugLogError(L"Error: %s - while processing mod packet: opcode=%s  size=%s; %s", ex->m_strMsg, DbgGetModTCPOpcode(opcode), CastItoXBytes(size), DbgGetClientInfo());
        if (client && ex->m_bDelete)
            client->SetDownloadState(DS_ERROR, L"Error while processing mod packet (CClientException): " + ex->m_strMsg);
        Disconnect(ex->m_strMsg);
        ex->Delete();
        return false;
    }
    catch (CString error)
    {
        if (thePrefs.GetVerbose() && !error.IsEmpty())
            DebugLogError(L"Error: %s - while processing mod packet: opcode=%s;  size=%s; %s", error, DbgGetModTCPOpcode(opcode), CastItoXBytes(size), DbgGetClientInfo());
        if (client)
            client->SetDownloadState(DS_ERROR, L"Error while processing mod packet (CString exception): " + error);
        Disconnect(_T("Error when processing packet.") + error);
        return false;
    }

    return true;
}

//>>> WiZaRd::IPv6 [Xanatos]
bool CClientUDPSocket::ProcessModPacket(BYTE* /*packet*/, const UINT size, const uint8 opcode, const _CIPAddress& ip, const uint16 port)
//bool CClientUDPSocket::ProcessModPacket(BYTE* /*packet*/, const UINT size, const uint8 opcode, const UINT ip, const uint16 port)
//<<< WiZaRd::IPv6 [Xanatos]
{
//	const UINT protocol = OP_MODPROT;
    CUpDownClient* client = theApp.clientlist->FindClientByIP_UDP(ip, port);
    if (client == NULL)
    {
        theApp.QueueDebugLogLineEx(LOG_WARNING, L"DBG: Received mod UDP packet (0x%02x) from unknown client (UDP) - %s:%u", opcode, ipstr(ip), port);
        return false;
    }

//	switch(opcode)
//	{
//		default:
    //CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(ip, port, true);
    CUpDownClient* sender = theApp.clientlist->FindClientByIP_UDP(ip, port);
//			CString strError = L"";
//			strError.Format(L"UDP (OP_MODPROT): 0x%02x", opcode);
//			throw strError;
    if (thePrefs.GetVerbose())
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"Unknown mod UDP packet: host=%s:%u (%s) opcode=0x%02x  size=%s", ipstr(ip), port, sender ? sender->DbgGetClientInfo() : L"NULL", opcode, CastItoXBytes(size));
    /*if(sender)
    {
    	CString post;
    	post.Format(L"UDP (OP_EMULEPROT): 0x%02x", opcode);
    	sender->Ban(post);
    }*/
    return false;
//	}
//	return true;
}