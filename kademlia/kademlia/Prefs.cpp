/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the official client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "stdafx.h"
#include "./Prefs.h"
#include "./kademlia.h"
#include "./indexed.h"
#include "../routing/RoutingZone.h"
#include "../utils/MiscUtils.h"
#include "../../opcodes.h"
#include "../../preferences.h"
#include "../../emule.h"
#include "../../emuledlg.h"
#include "../../SafeFile.h"
#include "../../Log.h"
#include "../../MD5Sum.h"
#include "../../OtherFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Kademlia;

CPrefs::CPrefs()
{
    CString sFilename = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("preferencesKad.dat");
    Init(sFilename);
}

CPrefs::~CPrefs()
{
    if (m_sFilename.GetLength() > 0)
        WriteFile();
}

void CPrefs::Init(LPCTSTR szFilename)
{
    m_uClientID.SetValueRandom();
    m_tLastContact = 0;
    m_uRecheckip = 0;
    m_uFirewalled = 0;
    m_uTotalFile = 0;
    m_uTotalStoreSrc = 0;
    m_uTotalStoreKey = 0;
    m_uTotalSource = 0;
    m_uTotalNotes = 0;
    m_uTotalStoreNotes = 0;
    m_bPublish = false;
    m_uClientHash.SetValue((uchar*)thePrefs.GetUserHash());
    m_uIP = 0;
    m_uIPLast = 0;
    m_bFindBuddy = false;
    m_uKademliaUsers = 0;
    m_uKademliaFiles = 0;
    m_sFilename = szFilename;
    m_bLastFirewallState = true;
    m_nExternKadPort = 0;
    m_bUseExternKadPort = true;
    m_nStatsUDPOpenNodes = 0;
    m_nStatsUDPFirewalledNodes = 0;
    m_nStatsTCPOpenNodes = 0;
    m_nStatsTCPFirewalledNodes = 0;
    m_nStatsKadV8LastChecked = 0;
    m_fKadV8Ratio = 0;
    ReadFile();
}

void CPrefs::ReadFile()
{
    try
    {
        CSafeBufferedFile file;
        CFileException fexp;
        if (file.Open(m_sFilename, CFile::modeRead | CFile::osSequentialScan | CFile::typeBinary | CFile::shareDenyWrite, &fexp))
        {
            setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
            m_uIP = file.ReadUInt32();
            file.ReadUInt16();
            file.ReadUInt128(&m_uClientID);
            // get rid of invalid kad IDs which may have been stored by older versions
            if (m_uClientID == 0)
                m_uClientID.SetValueRandom();
            file.Close();
        }
    }
    catch (CException *ex)
    {
        ASSERT(0);
        ex->Delete();
    }
    catch (...)
    {
        TRACE(L"Exception in CPrefs::readFile\n");
    }
}

void CPrefs::WriteFile()
{
    try
    {
        CSafeBufferedFile file;
        CFileException fexp;
        if (file.Open(m_sFilename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyWrite, &fexp))
        {
            setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
            file.WriteUInt32(m_uIP);
            file.WriteUInt16(0); //This is no longer used.
            file.WriteUInt128(&m_uClientID);
            file.WriteUInt8(0); //This is to tell older clients there are no tags..
            file.Close();
        }
    }
    catch (CException *ex)
    {
        ASSERT(0);
        ex->Delete();
    }
    catch (...)
    {
        TRACE(L"Exception in CPrefs::writeFile\n");
    }
}

void CPrefs::SetIPAddress(UINT uVal)
{
    //This is our first check on connect, init our IP..
    if (!uVal || !m_uIPLast)
    {
        m_uIP = uVal;
        m_uIPLast = uVal;
    }
    //If the last check matches this one, reset our current IP.
    //If the last check does not match, wait for our next incoming IP.
    //This happens for two reasons.. We just changed our IP, or a client responded with a bad IP.
    if (uVal == m_uIPLast)
        m_uIP = uVal;
    else
        m_uIPLast = uVal;
}


bool CPrefs::HasLostConnection() const
{
    if (m_tLastContact)
        return !((time(NULL) - m_tLastContact) < KADEMLIADISCONNECTDELAY);
    return false;
}

bool CPrefs::HasHadContact() const
{
    if (m_tLastContact)
        return ((time(NULL) - m_tLastContact) < KADEMLIADISCONNECTDELAY);
    return false;
}

bool CPrefs::GetFirewalled() const
{
    // We had enough tell us we are not firewalled...
    bool bFirewalled = (m_uFirewalled < 2);
    //Not enough people have told us we are open but we may be doing a recheck
    //at the moment which will give a false lowID.. Therefore we check to see
    //if we are still rechecking and will report our last known state..
    if (bFirewalled && GetRecheckIP())
        bFirewalled = m_bLastFirewallState;
    return bFirewalled;
}
void CPrefs::SetFirewalled()
{
    const bool bFirewalled = GetFirewalled();
    //We are checking our firewall state.. Let keep a snapshot of our
    //current state to prevent false reports during the recheck...
    // Firewalled checks shouldn't be triggered while GetRecheckIP() is still active?
    //ASSERT(bFirewalled == (m_uFirewalled < 2));
    m_bLastFirewallState = bFirewalled;
    m_uFirewalled = 0;
    if (bFirewalled != GetFirewalled())
        theApp.emuledlg->ShowConnectionState();
}

void CPrefs::IncFirewalled()
{
    const bool bFirewalled = GetFirewalled();
    ++m_uFirewalled;
    if (bFirewalled != GetFirewalled())
        theApp.emuledlg->ShowConnectionState();
}

bool CPrefs::GetFindBuddy()
{
    if (m_bFindBuddy)
    {
        m_bFindBuddy = false;
        return true;
    }
    return false;
}

void CPrefs::SetKademliaFiles()
{
    //There is no real way to know how many files are in the Kad network..
    //So we first try to see how many files per user are in the ED2K network..
    //If that fails, we use a set value based on previous tests..
    UINT nKadAverage = Kademlia::CKademlia::GetIndexed()->GetFileKeyCount();

    if (nKadAverage < 108)
    {
#ifdef _DEBUG
        AddDebugLogLine(DLP_VERYLOW, false, L"Kad file estimate used default avg: 108");
#endif
        nKadAverage = 108;
    }
#ifdef _DEBUG
    else
        AddDebugLogLine(DLP_VERYLOW, false, L"Kad file estimate used Kad avg: %u", nKadAverage);
#endif

    m_uKademliaFiles = nKadAverage*m_uKademliaUsers;
}

void CPrefs::GetKadID(CUInt128 *puID) const
{
    puID->SetValue(m_uClientID);
}

void CPrefs::GetKadID(CString *psID) const
{
    m_uClientID.ToHexString(psID);
}

void CPrefs::SetKadID(const CUInt128 &puID)
{
    m_uClientID = puID;
}

CUInt128 CPrefs::GetKadID() const
{
    return m_uClientID;
}

void CPrefs::GetClientHash(CUInt128 *puID) const
{
    puID->SetValue(m_uClientHash);
}

void CPrefs::GetClientHash(CString *psID) const
{
    m_uClientHash.ToHexString(psID);
}

void CPrefs::SetClientHash(const CUInt128 &puID)
{
    m_uClientHash = puID;
}

CUInt128 CPrefs::GetClientHash() const
{
    return m_uClientHash;
}

UINT CPrefs::GetIPAddress() const
{
    return m_uIP;
}

bool CPrefs::GetRecheckIP() const
{
    return (m_uRecheckip < KADEMLIAFIREWALLCHECKS);
}

void CPrefs::SetRecheckIP()
{
    m_uRecheckip = 0;
    SetFirewalled();
}

void CPrefs::IncRecheckIP()
{
    ++m_uRecheckip;
}

void CPrefs::SetLastContact()
{
    //Update connection state only when it changes.
    const bool bCurCon = CKademlia::GetPrefs()->HasHadContact();
    m_tLastContact = time(NULL);
    if (bCurCon != CKademlia::GetPrefs()->HasHadContact())
    {
        Log(LOG_SUCCESS | LOG_STATUSBAR, GetResString(IDS_CONNECTEDTO), GetResString(IDS_KADEMLIA));
        theApp.emuledlg->ShowConnectionState();
    }
}

UINT CPrefs::GetLastContact() const
{
    return m_tLastContact;
}

uint8 CPrefs::GetTotalFile() const
{
    return m_uTotalFile;
}

void CPrefs::SetTotalFile(uint8 uVal)
{
    m_uTotalFile = uVal;
}

uint8 CPrefs::GetTotalStoreSrc() const
{
    return m_uTotalStoreSrc;
}

void CPrefs::SetTotalStoreSrc(uint8 uVal)
{
    m_uTotalStoreSrc = uVal;
}

uint8 CPrefs::GetTotalStoreKey() const
{
    return m_uTotalStoreKey;
}

void CPrefs::SetTotalStoreKey(uint8 uVal)
{
    m_uTotalStoreKey = uVal;
}

uint8 CPrefs::GetTotalSource() const
{
    return m_uTotalSource;
}

void CPrefs::SetTotalSource(uint8 uVal)
{
    m_uTotalSource = uVal;
}

uint8 CPrefs::GetTotalNotes() const
{
    return m_uTotalNotes;
}

void CPrefs::SetTotalNotes(uint8 uVal)
{
    m_uTotalNotes = uVal;
}

uint8 CPrefs::GetTotalStoreNotes() const
{
    return m_uTotalStoreNotes;
}

void CPrefs::SetTotalStoreNotes(uint8 uVal)
{
    m_uTotalStoreNotes = uVal;
}

UINT CPrefs::GetKademliaUsers() const
{
    return m_uKademliaUsers;
}

void CPrefs::SetKademliaUsers(UINT uVal)
{
    m_uKademliaUsers = uVal;
}

UINT CPrefs::GetKademliaFiles() const
{
    return m_uKademliaFiles;
}

bool CPrefs::GetPublish() const
{
    return m_bPublish;
}

void CPrefs::SetPublish(bool bVal)
{
    m_bPublish = bVal;
}

void CPrefs::SetFindBuddy(bool bVal)
{
    m_bFindBuddy = bVal;
}

UINT CPrefs::GetUDPVerifyKey(UINT dwTargetIP)
{
    uint64 ui64Buffer = thePrefs.GetKadUDPKey();
    ui64Buffer <<= 32;
    ui64Buffer |= dwTargetIP;
    MD5Sum md5((uchar*)&ui64Buffer, 8);
    return ((UINT)(PeekUInt32(md5.GetRawHash() + 0) ^ PeekUInt32(md5.GetRawHash() + 4) ^ PeekUInt32(md5.GetRawHash() + 8) ^ PeekUInt32(md5.GetRawHash() + 12)) % 0xFFFFFFFE) + 1;
}

bool CPrefs::GetUseExternKadPort() const
{
    return m_bUseExternKadPort && !Kademlia::CKademlia::IsRunningInLANMode();
}

void CPrefs::SetUseExternKadPort(bool bVal)
{
    m_bUseExternKadPort = bVal;
}

uint16 CPrefs::GetExternalKadPort() const
{
    return m_nExternKadPort;
}

#define EXTERNAL_PORT_ASKIPS	3
void CPrefs::SetExternKadPort(uint16 uVal, UINT uFromIP)
{
    if (FindExternKadPort(false))
    {
        // check if the remote client already sent a possible port
        bool bFound = false;
        for (int i = 0; i < m_anExternPortReplies.GetCount(); ++i)
        {
            if (m_anExternPortReplies[i].dwIP == uFromIP)
            {
                if (m_anExternPortReplies[i].nPort == uVal)
                    return;
                else
                {
                    DebugLog(L"Received new possible external Kad Port %u (old: %u) from %s", uVal, m_anExternPortReplies[i].nPort, ipstr(ntohl(uFromIP)));
                    m_anExternPortReplies[i].nPort = uVal;
                    bFound = true;
                    break;
                }
            }
        }
        if (!bFound)
        {
            DebugLog(L"Received possible external Kad Port %u from %s", uVal, ipstr(ntohl(uFromIP)));
            m_anExternPortReplies.Add(externPortReply(uFromIP, uVal));
        }

        // if 2 out of 3 tries result in the same external port its fine, otherwise consider it as unreliable
        for (int i = 0; i < m_anExternPortReplies.GetCount(); ++i)
        {
            if (m_anExternPortReplies[i].nPort == uVal)
            {
                m_nExternKadPort = uVal;
                DebugLog(L"Set external Kad Port to %u", uVal);
                while (m_anExternPortReplies.GetCount() < EXTERNAL_PORT_ASKIPS)
                    m_anExternPortReplies.Add(externPortReply(uFromIP, uVal)); // add empty entries so we know the check has finished even if we asked less than max IPs
                return;
            }
        }

        // if we have already received EXTERNAL_PORT_ASKIPS replies without getting a valid external port, we should cancel searching
        if (!FindExternKadPort(false))
        {
            DebugLog(L"Our external port seems unreliable, not using it for firewallchecks", uVal);
            m_nExternKadPort = 0;
        }
    }
}

uint16 CPrefs::GetInternKadPort() const
{
    return thePrefs.GetUDPPort();
}

bool CPrefs::FindExternKadPort(bool bReset)
{
    if (bReset)
        m_anExternPortReplies.RemoveAll();

    return m_anExternPortReplies.GetCount() < EXTERNAL_PORT_ASKIPS && !Kademlia::CKademlia::IsRunningInLANMode();
}

float CPrefs::StatsGetFirewalledRatio(bool bUDP) const
{
    // gives an estimated percentage of TCP firewalled clients in the network
    // will only work once enough > 0.49b nodes have spread and only if we are not UDP firewalled ourself
    if (bUDP)
    {
        if (m_nStatsUDPFirewalledNodes > 0 && m_nStatsUDPOpenNodes > 10)
            return ((float)m_nStatsUDPFirewalledNodes / (float)(m_nStatsUDPFirewalledNodes + m_nStatsUDPOpenNodes));
        else
            return 0;
    }
    else
    {
        if (m_nStatsTCPFirewalledNodes > 0 && m_nStatsTCPOpenNodes > 10)
            return ((float)m_nStatsTCPFirewalledNodes / (float)(m_nStatsTCPFirewalledNodes + m_nStatsTCPOpenNodes));
        else
            return 0;
    }
}

void CPrefs::StatsIncUDPFirewalledNodes(bool bFirewalled)
{
    if (bFirewalled)
        m_nStatsUDPFirewalledNodes++;
    else
        m_nStatsUDPOpenNodes++;
}

void CPrefs::StatsIncTCPFirewalledNodes(bool bFirewalled)
{
    if (bFirewalled)
        m_nStatsTCPFirewalledNodes++;
    else
        m_nStatsTCPOpenNodes++;
}

float CPrefs::StatsGetKadV8Ratio()
{
    // this function is basically just a buffer, so we don't recount all nodes everytime we need the result
    if (m_nStatsKadV8LastChecked + 60 < time(NULL))
    {
        m_nStatsKadV8LastChecked = time(NULL);
        UINT nV8Contacts = 0;
        UINT nNonV8Contacts = 0;
        CKademlia::GetRoutingZone()->GetNumContacts(nV8Contacts, nNonV8Contacts, KADEMLIA_VERSION8_49b);
        //DEBUG_ONLY( AddDebugLogLine(DLP_LOW, false, _T("Counted Kad V8 Contacts: %u out of %u in routing table. FirewalledRatios: UDP - %.02f%% | TCP - %.02f%%")
        //	, nV8Contacts, nNonV8Contacts + nV8Contacts, StatsGetFirewalledRatio(true) * 100, StatsGetFirewalledRatio(false) * 100) );
        if (nV8Contacts > 0)
            m_fKadV8Ratio = ((float)nV8Contacts / (float)(nV8Contacts + nNonV8Contacts));
        else
            m_fKadV8Ratio = 0;
    }
    return m_fKadV8Ratio;
}