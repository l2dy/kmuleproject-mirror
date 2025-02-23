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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once

class CEncryptedDatagramSocket
{
  public:
    CEncryptedDatagramSocket();
    virtual ~CEncryptedDatagramSocket();

  protected:
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
    int DecryptReceivedClient(BYTE* pbyBufIn, int nBufLen, BYTE** ppbyBufOut, const CAddress& IP, UINT* nReceiverVerifyKey, UINT* nSenderVerifyKey) const;
    int EncryptSendClient(uchar** ppbyBuf, int nBufLen, const uchar* pachClientHashOrKadID, bool bKad, UINT nReceiverVerifyKey, UINT nSenderVerifyKey, bool bIPv6) const;
//<<< WiZaRd::IPv6 [Xanatos]
#else
    int DecryptReceivedClient(BYTE* pbyBufIn, int nBufLen, BYTE** ppbyBufOut, UINT dwIP, UINT* nReceiverVerifyKey, UINT* nSenderVerifyKey) const;
    int EncryptSendClient(uchar** ppbyBuf, int nBufLen, const uchar* pachClientHashOrKadID, bool bKad, UINT nReceiverVerifyKey, UINT nSenderVerifyKey) const;
#endif
};