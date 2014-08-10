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

class CFriend;
class CFriendListCtrl;
class CUpDownClient;

class CFriendList
{
  public:
    CFriendList();
    ~CFriendList();

    bool		IsAlreadyFriend(CString strUserHash) const;
    bool		IsValid(CFriend* pToCheck) const;
    void		SaveList();
    bool		LoadList();
    void		RefreshFriend(CFriend* torefresh) const;
#ifdef IPV6_SUPPORT
    CFriend*	SearchFriend(const uchar* achUserHash, CAddress dwIP, uint16 nPort) const; //>>> WiZaRd::IPv6 [Xanatos]
#else
    CFriend*	SearchFriend(const uchar* achUserHash, const UINT dwIP, const uint16 nPort) const;
#endif
    void		SetWindow(CFriendListCtrl* NewWnd)
    {
        m_wndOutput = NewWnd;
    }
    void		ShowFriends() const;
    bool		AddFriend(CUpDownClient* toadd);
#ifdef IPV6_SUPPORT
    bool		AddFriend(const uchar* abyUserhash, UINT dwLastSeen, const CAddress& dwLastUsedIP, uint16 nLastUsedPort, //>>> WiZaRd::IPv6 [Xanatos]
#else
    bool		AddFriend(const uchar* abyUserhash, UINT dwLastSeen, UINT dwLastUsedIP, uint16 nLastUsedPort,
#endif
                          UINT dwLastChatted, LPCTSTR pszName, UINT dwHasHash, const CString& strComment); //>>> WiZaRd::FriendComment
    void		RemoveFriend(CFriend* todel);
    void		RemoveAllFriendSlots();
    void		Process();
    int			GetCount()
    {
        return m_listFriends.GetCount();
    }

  private:
    CTypedPtrList<CPtrList, CFriend*>	m_listFriends;
    CFriendListCtrl*					m_wndOutput;
    UINT								m_nLastSaved;
};
