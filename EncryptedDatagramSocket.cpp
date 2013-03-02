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

/* Basic Obfuscated Handshake Protocol UDP:
	see EncryptedStreamSocket.h

****************************** ED2K Packets

	-Keycreation Client <-> Clinet:
	 - Client A (Outgoing connection):
				Sendkey:	Md5(<UserHashClientB 16><IPClientA 4><MagicValue91 1><RandomKeyPartClientA 2>)  23
	 - Client B (Incomming connection):
				Receivekey: Md5(<UserHashClientB 16><IPClientA 4><MagicValue91 1><RandomKeyPartClientA 2>)  23
	 - Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to safe CPU time

	- Handshake
			-> The handshake is encrypted - except otherwise noted - by the Keys created above
			-> Padding is cucrently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 7 Bits[Unencrypted]><ED2K Marker 1Bit = 1><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16>

	- Additional Comments:
			- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		    - SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromiss, turning in complete randomness (and nice design) but gaining
			  a lower CPU usage
		    - Kad/Ed2k Marker are only indicators, which possibility could be tried first, and should not be trusted

****************************** Server Packets

	-Keycreation Client <-> Server:
	 - Client A (Outgoing connection client -> server):
				Sendkey:	Md5(<BaseKey 4><MagicValueClientServer 1><RandomKeyPartClientA 2>)  7
	 - Client B (Incomming connection):
				Receivekey: Md5(<BaseKey 4><MagicValueServerClient 1><RandomKeyPartClientA 2>)  7
	 - Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to safe CPU time

	- Handshake
			-> The handshake is encrypted - except otherwise noted - by the Keys created above
			-> Padding is cucrently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 1[Unencrypted]><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16>

	- Overhead: 8 Bytes per UDP Packet

	- Security for Basic Obfuscation:
			- Random looking packets, very limited protection against passive eavesdropping single packets

	- Additional Comments:
			- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		    - SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromiss, turning in complete randomness (and nice design) but gaining
			  a lower CPU usage

****************************** KAD Packets

	-Keycreation Client <-> Client:
											(Used in general in request packets)
	 - Client A (Outgoing connection):
				Sendkey:	Md5(<KadID 16><RandomKeyPartClientA 2>)  18
	 - Client B (Incomming connection):
				Receivekey: Md5(<KadID 16><RandomKeyPartClientA 2>)  18
	               -- OR --					(Used in general in response packets)
	 - Client A (Outgoing connection):
				Sendkey:	Md5(<ReceiverKey 4><RandomKeyPartClientA 2>)  6
	 - Client B (Incomming connection):
				Receivekey: Md5(<ReceiverKey 4><RandomKeyPartClientA 2>)  6

	 - Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to safe CPU time

	- Handshake
			-> The handshake is encrypted - except otherwise noted - by the Keys created above
			-> Padding is cucrently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 6 Bits[Unencrypted]><Kad Marker 2Bit = 0 or 2><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16><ReceiverVerifyKey 4><SenderVerifyKey 4>

	- Overhead: 16 Bytes per UDP Packet

	- Kad/Ed2k Marker:
		 x 1	-> Most likely an ED2k Packet, try Userhash as Key first
		 0 0	-> Most likely an Kad Packet, try NodeID as Key first
		 1 0	-> Most likely an Kad Packet, try SenderKey as Key first

	- Additional Comments:
			- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		    - SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromiss, turning in complete randomness (and nice design) but gaining
			  a lower CPU usage
		    - Kad/Ed2k Marker are only indicators, which possibility could be tried first, and need not be trusted
			- Packets which use the senderkey are prone to BruteForce attacks, which take only a few minutes (2^32)
			  which is while not acceptable for encryption fair enough for obfuscation
*/

#include "stdafx.h"
#include "EncryptedDatagramSocket.h"
#include "emule.h"
#include "md5sum.h"
#include "Log.h"
#include "preferences.h"
#include "opcodes.h"
#include "otherfunctions.h"
#include "Statistics.h"
#include "safefile.h"
#include "./kademlia/kademlia/prefs.h"
#include "./kademlia/kademlia/kademlia.h"
// random generator
#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4146) // Einem vorzeichenlosen Typ wurde ein un�rer Minus-Operator zugewiesen. Das Ergebnis ist weiterhin vorzeichenlos.
#include <crypto51/osrng.h>
#pragma warning(default:4146) // Einem vorzeichenlosen Typ wurde ein un�rer Minus-Operator zugewiesen. Das Ergebnis ist weiterhin vorzeichenlos.
#pragma warning(default:4702) // unreachable code
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define CRYPT_HEADER_WITHOUTPADDING		    8
#define	MAGICVALUE_UDP						91
#define MAGICVALUE_UDP_SYNC_CLIENT			0x395F2EC1
#define MAGICVALUE_UDP_SYNC_SERVER			0x13EF24D5
#define	MAGICVALUE_UDP_SERVERCLIENT			0xA5
#define	MAGICVALUE_UDP_CLIENTSERVER			0x6B

static CryptoPP::AutoSeededRandomPool cryptRandomGen;

CEncryptedDatagramSocket::CEncryptedDatagramSocket()
{
}

CEncryptedDatagramSocket::~CEncryptedDatagramSocket()
{

}

int CEncryptedDatagramSocket::DecryptReceivedClient(BYTE* pbyBufIn, int nBufLen, BYTE** ppbyBufOut, UINT dwIP, UINT* nReceiverVerifyKey, UINT* nSenderVerifyKey) const
{
    int nResult = nBufLen;
    *ppbyBufOut = pbyBufIn;

    if (nReceiverVerifyKey == NULL || nSenderVerifyKey == NULL)
    {
        ASSERT(0);
        return nResult;
    }

    *nReceiverVerifyKey = 0;
    *nSenderVerifyKey = 0;

    if (nResult <= CRYPT_HEADER_WITHOUTPADDING /*|| !thePrefs.IsClientCryptLayerSupported()*/)
        return nResult;

    switch (pbyBufIn[0])
    {
    case OP_EMULEPROT:
    case OP_KADEMLIAPACKEDPROT:
    case OP_KADEMLIAHEADER:
    case OP_UDPRESERVEDPROT1:
    case OP_UDPRESERVEDPROT2:
    case OP_PACKEDPROT:
        return nResult; // no encrypted packet (see description on top)
    }

    // might be an encrypted packet, try to decrypt
    RC4_Key_Struct keyReceiveKey;
    UINT dwValue = 0;
    // check the marker bit which type this packet could be and which key to test first, this is only an indicator since old clients have it set random
    // see the header for marker bits explanation
    byte byCurrentTry = ((pbyBufIn[0] & 0x03) == 3) ? 1 : (pbyBufIn[0] & 0x03);
    byte byTries;
    if (Kademlia::CKademlia::GetPrefs() == NULL)
    {
        // if kad never run, no point in checking anything except for ed2k encryption
        byTries = 1;
        byCurrentTry = 1;
    }
    else
        byTries = 3;
    bool bKadRecvKeyUsed = false;
    bool bKad = false;
    do
    {
        byTries--;
        MD5Sum md5;
        if (byCurrentTry == 0)
        {
            // kad packet with NodeID as key
            bKad = true;
            bKadRecvKeyUsed = false;
            if (Kademlia::CKademlia::GetPrefs())
            {
                uchar achKeyData[18];
                memcpy(achKeyData, Kademlia::CKademlia::GetPrefs()->GetKadID().GetData(), 16);
                memcpy(achKeyData + 16, pbyBufIn + 1, 2); // random key part sent from remote client
                md5.Calculate(achKeyData, sizeof(achKeyData));
            }
        }
        else if (byCurrentTry == 1)
        {
            // ed2k packet
            bKad = false;
            bKadRecvKeyUsed = false;
            uchar achKeyData[23];
            md4cpy(achKeyData, thePrefs.GetUserHash());
            achKeyData[20] = MAGICVALUE_UDP;
            memcpy(achKeyData + 16, &dwIP, 4);
            memcpy(achKeyData + 21, pbyBufIn + 1, 2); // random key part sent from remote client
            md5.Calculate(achKeyData, sizeof(achKeyData));
        }
        else if (byCurrentTry == 2)
        {
            // kad packet with ReceiverKey as key
            bKad = true;
            bKadRecvKeyUsed = true;
            if (Kademlia::CKademlia::GetPrefs())
            {
                uchar achKeyData[6];
                PokeUInt32(achKeyData, Kademlia::CPrefs::GetUDPVerifyKey(dwIP));
                memcpy(achKeyData + 4, pbyBufIn + 1, 2); // random key part sent from remote client
                md5.Calculate(achKeyData, sizeof(achKeyData));
            }
        }
        else
            ASSERT(0);

        RC4CreateKey(md5.GetRawHash(), 16, &keyReceiveKey, true);
        RC4Crypt(pbyBufIn + 3, (uchar*)&dwValue, sizeof(dwValue), &keyReceiveKey);
        byCurrentTry = (byCurrentTry + 1) % 3;
    }
    while (dwValue != MAGICVALUE_UDP_SYNC_CLIENT && byTries > 0);   // try to decrypt as ed2k as well as kad packet if needed (max 3 rounds)

    if (dwValue == MAGICVALUE_UDP_SYNC_CLIENT)
    {
        // yup this is an encrypted packet
        // debugoutput notices
        // the following cases are "allowed" but shouldn't happen given that there is only our implementation yet
        if (bKad && (pbyBufIn[0] & 0x01) != 0)
            DebugLog(_T("Received obfuscated UDP packet from clientIP: %s with wrong key marker bits (kad packet, ed2k bit)"), ipstr(dwIP));
        else if (bKad && !bKadRecvKeyUsed && (pbyBufIn[0] & 0x02) != 0)
            DebugLog(_T("Received obfuscated UDP packet from clientIP: %s with wrong key marker bits (kad packet, nodeid key, recvkey bit)"), ipstr(dwIP));
        else if (bKad && bKadRecvKeyUsed && (pbyBufIn[0] & 0x02) == 0)
            DebugLog(_T("Received obfuscated UDP packet from clientIP: %s with wrong key marker bits (kad packet, recvkey key, nodeid bit)"), ipstr(dwIP));

        uint8 byPadLen;
        RC4Crypt(pbyBufIn + 7, (uchar*)&byPadLen, 1, &keyReceiveKey);
        nResult -= CRYPT_HEADER_WITHOUTPADDING;
        if (nResult <= byPadLen)
        {
            DebugLogError(_T("Invalid obfuscated UDP packet from clientIP: %s, Paddingsize (%u) larger than received bytes"), ipstr(dwIP), byPadLen);
            return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk
        }
        if (byPadLen > 0)
            RC4Crypt(NULL, NULL, byPadLen, &keyReceiveKey);
        nResult -= byPadLen;

        if (bKad)
        {
            if (nResult <= 8)
            {
                DebugLogError(_T("Obfuscated Kad packet with mismatching size (verify keys missing) received from clientIP: %s"), ipstr(dwIP));
                return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk;
            }
            // read the verify keys
            RC4Crypt(pbyBufIn + CRYPT_HEADER_WITHOUTPADDING + byPadLen, (uchar*)nReceiverVerifyKey, 4, &keyReceiveKey);
            RC4Crypt(pbyBufIn + CRYPT_HEADER_WITHOUTPADDING + byPadLen + 4, (uchar*)nSenderVerifyKey, 4, &keyReceiveKey);
            nResult -= 8;
        }
        *ppbyBufOut = pbyBufIn + (nBufLen - nResult);
        RC4Crypt((uchar*)*ppbyBufOut, (uchar*)*ppbyBufOut, nResult, &keyReceiveKey);
        theStats.AddDownDataOverheadCrypt(nBufLen - nResult);
        //DEBUG_ONLY( DebugLog(_T("Received obfuscated UDP packet from clientIP: %s, Key: %s, RKey: %u, SKey: %u"), ipstr(dwIP), bKad ? (bKadRecvKeyUsed ? _T("ReceiverKey") : _T("NodeID")) : _T("UserHash")
        //	, nReceiverVerifyKey != 0 ? *nReceiverVerifyKey : 0, nSenderVerifyKey != 0 ? *nSenderVerifyKey : 0) );
        return nResult; // done
    }
    else
    {
        DebugLogWarning(_T("Obfuscated packet expected but magicvalue mismatch on UDP packet from clientIP: %s, Possible RecvKey: %u"), ipstr(dwIP), Kademlia::CPrefs::GetUDPVerifyKey(dwIP));
        return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk
    }
}

// Encrypt packet. Key used:
// pachClientHashOrKadID != NULL									-> pachClientHashOrKadID
// pachClientHashOrKadID == NULL && bKad && nReceiverVerifyKey != 0 -> nReceiverVerifyKey
// else																-> ASSERT
int CEncryptedDatagramSocket::EncryptSendClient(uchar** ppbyBuf, int nBufLen, const uchar* pachClientHashOrKadID, bool bKad, UINT nReceiverVerifyKey, UINT nSenderVerifyKey) const
{
    ASSERT(theApp.GetPublicIP() != 0 || bKad);
    ASSERT(pachClientHashOrKadID != NULL || nReceiverVerifyKey != 0);
    ASSERT((nReceiverVerifyKey == 0 && nSenderVerifyKey == 0) || bKad);

    uint8 byPadLen = 0;			// padding disabled for UDP currently
    const UINT nCryptHeaderLen = byPadLen + CRYPT_HEADER_WITHOUTPADDING + (bKad ? 8 : 0);

    UINT nCryptedLen = nBufLen + nCryptHeaderLen;
    uchar* pachCryptedBuffer = new uchar[nCryptedLen];
    bool bKadRecKeyUsed = false;

    uint16 nRandomKeyPart = (uint16)cryptRandomGen.GenerateWord32(0x0000, 0xFFFF);
    MD5Sum md5;
    if (bKad)
    {
        if ((pachClientHashOrKadID == NULL || isnulmd4(pachClientHashOrKadID)) && nReceiverVerifyKey != 0)
        {
            bKadRecKeyUsed = true;
            uchar achKeyData[6];
            PokeUInt32(achKeyData, nReceiverVerifyKey);
            PokeUInt16(achKeyData+4, nRandomKeyPart);
            md5.Calculate(achKeyData, sizeof(achKeyData));
            //DEBUG_ONLY( DebugLog(_T("Creating obfuscated Kad packet encrypted by ReceiverKey (%u)"), nReceiverVerifyKey) );
        }
        else if (pachClientHashOrKadID != NULL && !isnulmd4(pachClientHashOrKadID))
        {
            uchar achKeyData[18];
            md4cpy(achKeyData, pachClientHashOrKadID);
            PokeUInt16(achKeyData+16, nRandomKeyPart);
            md5.Calculate(achKeyData, sizeof(achKeyData));
            //DEBUG_ONLY( DebugLog(_T("Creating obfuscated Kad packet encrypted by Hash/NodeID %s"), md4str(pachClientHashOrKadID)) );
        }
        else
        {
            ASSERT(0);
            delete[] pachCryptedBuffer;
            return nBufLen;
        }
    }
    else
    {
        uchar achKeyData[23];
        md4cpy(achKeyData, pachClientHashOrKadID);
        UINT dwIP = theApp.GetPublicIP();
        memcpy(achKeyData+16, &dwIP, 4);
        memcpy(achKeyData+21, &nRandomKeyPart, 2);
        achKeyData[20] = MAGICVALUE_UDP;
        md5.Calculate(achKeyData, sizeof(achKeyData));
    }
    RC4_Key_Struct keySendKey;
    RC4CreateKey(md5.GetRawHash(), 16, &keySendKey, true);

    // create the semi random byte encryption header
    uint8 bySemiRandomNotProtocolMarker = 0;
    int i;
    for (i = 0; i < 128; i++)
    {
        bySemiRandomNotProtocolMarker = cryptRandomGen.GenerateByte();
        bySemiRandomNotProtocolMarker = bKad ? (bySemiRandomNotProtocolMarker & 0xFE) : (bySemiRandomNotProtocolMarker | 0x01); // set the ed2k/kad marker bit
        if (bKad)
            bySemiRandomNotProtocolMarker = bKadRecKeyUsed ? ((bySemiRandomNotProtocolMarker & 0xFE) | 0x02) : (bySemiRandomNotProtocolMarker & 0xFC); // set the ed2k/kad and nodeid/reckey markerbit
        else
            bySemiRandomNotProtocolMarker = (bySemiRandomNotProtocolMarker | 0x01); // set the ed2k/kad marker bit

        bool bOk = false;
        switch (bySemiRandomNotProtocolMarker)  // not allowed values
        {
        case OP_EMULEPROT:
        case OP_KADEMLIAPACKEDPROT:
        case OP_KADEMLIAHEADER:
        case OP_UDPRESERVEDPROT1:
        case OP_UDPRESERVEDPROT2:
        case OP_PACKEDPROT:
            break;
        default:
            bOk = true;
        }
        if (bOk)
            break;
    }
    if (i >= 128)
    {
        // either we have _really_ bad luck or the randomgenerator is a bit messed up
        ASSERT(0);
        bySemiRandomNotProtocolMarker = 0x01;
    }

    UINT dwMagicValue = MAGICVALUE_UDP_SYNC_CLIENT;
    pachCryptedBuffer[0] = bySemiRandomNotProtocolMarker;
    memcpy(pachCryptedBuffer + 1, &nRandomKeyPart, 2);
    RC4Crypt((uchar*)&dwMagicValue, pachCryptedBuffer + 3, 4, &keySendKey);
    RC4Crypt((uchar*)&byPadLen, pachCryptedBuffer + 7, 1, &keySendKey);

    for (int j = 0; j < byPadLen; j++)
    {
        uint8 byRand = (uint8)rand();	// they actually dont really need to be random, but it doesn't hurts either
        RC4Crypt((uchar*)&byRand, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + j, 1, &keySendKey);
    }

    if (bKad)
    {
        RC4Crypt((uchar*)&nReceiverVerifyKey, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + byPadLen, 4, &keySendKey);
        RC4Crypt((uchar*)&nSenderVerifyKey, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + byPadLen + 4, 4, &keySendKey);
    }

    RC4Crypt(*ppbyBuf, pachCryptedBuffer + nCryptHeaderLen, nBufLen, &keySendKey);
    delete[] *ppbyBuf;
    *ppbyBuf = pachCryptedBuffer;

    theStats.AddUpDataOverheadCrypt(nCryptedLen - nBufLen);
    return nCryptedLen;
}
