//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "Kademlia/Utils/KadClientSearcher.h"

class CUpDownClient;
class CFileDataIO;
class CFriendConnectionListener;

enum EFriendConnectState
{
    FCS_NONE = 0,
    FCS_CONNECTING,
    FCS_AUTH,
    FCS_KADSEARCHING,
};

enum EFriendConnectReport
{
    FCR_ESTABLISHED = 0,
    FCR_DISCONNECTED,
    FCR_USERHASHVERIFIED,
    FCR_USERHASHFAILED,
    FCR_SECUREIDENTFAILED,
    FCR_DELETED
};

#define	FF_NAME		0x01
#define	FF_KADID	0x02
#define FF_COMMENT	"CMT" //>>> WiZaRd::FriendComment
//>>> WiZaRd::Data without Client
#define FF_SOFT			"SFT"
#define FF_UPLOAD		"FUL"
#define FF_DOWNLOAD		"FDL"
//<<< WiZaRd::Data without Client

///////////////////////////////////////////////////////////////////////////////
// CFriendConnectionListener

class CFriendConnectionListener
{
  public:
    virtual void	ReportConnectionProgress(CUpDownClient* /*pClient*/, CString /*strProgressDesc*/, bool /*bNoTimeStamp*/)	{ AfxDebugBreak(); }
    virtual void	ConnectingResult(CUpDownClient* /*pClient*/, bool /*bSuccess*/)												{ AfxDebugBreak(); }
    virtual void	ClientObjectChanged(CUpDownClient* /*pOldClient*/, CUpDownClient* /*pNewClient*/)							{ AfxDebugBreak(); }
};

///////////////////////////////////////////////////////////////////////////////
// CFriend
class CFriend : public Kademlia::CKadClientSearcher
{
  public:
    CFriend();
    CFriend(CUpDownClient* client);
#ifdef IPV6_SUPPORT
    CFriend(const uchar* abyUserhash, UINT dwLastSeen, const CAddress& LastUsedIP, uint16 nLastUsedPort, //>>> WiZaRd::IPv6 [Xanatos]
#else
    CFriend(const uchar* abyUserhash, UINT dwLastSeen, UINT dwLastUsedIP, uint16 nLastUsedPort,
#endif
            UINT dwLastChatted, LPCTSTR pszName, UINT dwHasHash, const CString& strComment); //>>> WiZaRd::FriendComment

    ~CFriend();

    uchar	m_abyUserhash[16];

    UINT	m_dwLastSeen;
#ifdef IPV6_SUPPORT
    CAddress	m_dwLastUsedIP;
#else
    UINT	m_dwLastUsedIP;
#endif
    uint16	m_nLastUsedPort;
    UINT	m_dwLastChatted;
    CString m_strName;
    CString m_strComment; //>>> WiZaRd::FriendComment

    CUpDownClient*	GetLinkedClient(bool bValidCheck = false) const;
    void			SetLinkedClient(CUpDownClient* linkedClient);
    CUpDownClient*	GetClientForChatSession();

    void	LoadFromFile(CFileDataIO* file);
    void	WriteToFile(CFileDataIO* file);

    bool	TryToConnect(CFriendConnectionListener* pConnectionReport);
    void	UpdateFriendConnectionState(EFriendConnectReport eEvent);
    bool	IsTryingToConnect() const
    {
        return m_FriendConnectState != FCS_NONE;
    }
    bool	CancelTryToConnect(CFriendConnectionListener* pConnectionReport);
    void	FindKadID();
    void	KadSearchNodeIDByIPResult(Kademlia::EKadClientSearchRes eStatus, const uchar* pachNodeID);
    void	KadSearchIPByNodeIDResult(Kademlia::EKadClientSearchRes eStatus, UINT dwIP, uint16 nPort);

    void	SendMessage(CString strMessage);

    void	SetFriendSlot(bool newValue);
    bool	GetFriendSlot() const;

    bool	HasUserhash() const;
    bool	HasKadID() const;

  private:
    uchar	m_abyKadID[16];
    bool	m_friendSlot;
    UINT	m_dwLastKadSearch;

    EFriendConnectState			m_FriendConnectState;
    CTypedPtrList<CPtrList, CFriendConnectionListener*> m_liConnectionReport;
    CUpDownClient*				m_LinkedClient;

//>>> WiZaRd::Data without Client
  public:
    CString	GetFriendName() const;
    CString GetFriendHash() const;
    CString	GetFriendKadID() const;
    CString	GetIdentState() const;
    CString	GetFriendSoft() const;
    CString	GetFriendUpload() const;
    CString	GetFriendUploaded() const;
    CString	GetFriendDownload() const;
    CString	GetFriendDownloaded() const;
  private:
    sint64	m_uiUploaded;
    sint64	m_uiDownloaded;
    CString	m_strSoft;
//<<< WiZaRd::Data without Client
};
