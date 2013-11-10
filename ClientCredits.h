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
#include "MapKey.h"
#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4146) // Einem vorzeichenlosen Typ wurde ein unärer Minus-Operator zugewiesen. Das Ergebnis ist weiterhin vorzeichenlos.
#pragma warning(disable:4127) // Bedingter Ausdruck ist konstant
#pragma warning(disable:4189) // Lokale Variable ist initialisiert aber nicht referenziert
#pragma warning(disable:4505) // 'CryptoPP::StringNarrow': Nichtreferenzierte lokale Funktion wurde entfernt
#include <crypto51/rsa.h>
#pragma warning(default:4505) // 'CryptoPP::StringNarrow': Nichtreferenzierte lokale Funktion wurde entfernt
#pragma warning(default:4189) // Lokale Variable ist initialisiert aber nicht referenziert
#pragma warning(default:4127) // Bedingter Ausdruck ist konstant
#pragma warning(default:4146) // Einem vorzeichenlosen Typ wurde ein unärer Minus-Operator zugewiesen. Das Ergebnis ist weiterhin vorzeichenlos.
#pragma warning(default:4702) // unreachable code
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative

#define	 MAXPUBKEYSIZE		80

#define CRYPT_CIP_REMOTECLIENT	10
#define CRYPT_CIP_LOCALCLIENT	20
#define CRYPT_CIP_NONECLIENT	30

#pragma pack(1)
struct CreditStruct_29a
{
    uchar		abyKey[16];
    UINT		nUploadedLo;	// uploaded TO him
    UINT		nDownloadedLo;	// downloaded from him
    UINT		nLastSeen;
    UINT		nUploadedHi;	// upload high 32
    UINT		nDownloadedHi;	// download high 32
    uint16		nReserved3;
};
struct CreditStruct
{
    uchar		abyKey[16];
    UINT		nUploadedLo;	// uploaded TO him
    UINT		nDownloadedLo;	// downloaded from him
    UINT		nLastSeen;
    UINT		nUploadedHi;	// upload high 32
    UINT		nDownloadedHi;	// download high 32
    uint16		nReserved3;
    uint8		nKeySize;
    uchar		abySecureIdent[MAXPUBKEYSIZE];
};
#pragma pack()

enum EIdentState
{
    IS_NOTAVAILABLE,
    IS_IDNEEDED,
    IS_IDENTIFIED,
    IS_IDFAILED,
    IS_IDBADGUY,
};

class CClientCredits
{
    friend class CClientCreditsList;
public:
    CClientCredits(CreditStruct* in_credits);
    CClientCredits(const uchar* key);
    ~CClientCredits();

    const uchar* GetKey() const
    {
        return m_pCredits->abyKey;
    }
    uchar*	GetSecureIdent()
    {
        return m_abyPublicKey;
    }
    uint8	GetSecIDKeyLen() const
    {
        return m_nPublicKeyLen;
    }
    CreditStruct* GetDataStruct() const
    {
        return m_pCredits;
    }
    void	ClearWaitStartTime();
//>>> WiZaRd::IPv6 [Xanatos]
    void	AddDownloaded(UINT bytes, const _CIPAddress& ForIP);
    void	AddUploaded(UINT bytes, const _CIPAddress& ForIP);
    float	GetScoreRatio(const _CIPAddress& ForIP) /*const*/; //>>> WiZaRd::CPU calm down
    EIdentState	GetCurrentIdentState(const _CIPAddress& ForIP) const; // can be != IdentState
    UINT	GetSecureWaitStartTime(const _CIPAddress& ForIP);
    void	SetSecWaitStartTime(const _CIPAddress& ForIP);
//  void	AddDownloaded(UINT bytes, UINT dwForIP);
//  void	AddUploaded(UINT bytes, UINT dwForIP);
// 	float	GetScoreRatio(UINT dwForIP) /*const*/; //>>> WiZaRd::CPU calm down
// 	EIdentState	GetCurrentIdentState(UINT dwForIP) const; // can be != IdentState
// 	UINT	GetSecureWaitStartTime(UINT dwForIP);
// 	void	SetSecWaitStartTime(UINT dwForIP);
//<<< WiZaRd::IPv6 [Xanatos]
    uint64	GetUploadedTotal() const;
    uint64	GetDownloadedTotal() const;
    void	SetLastSeen()
    {
        m_pCredits->nLastSeen = time(NULL);
    }
    bool	SetSecureIdent(const uchar* pachIdent, uint8 nIdentLen); // Public key cannot change, use only if there is not public key yet
    UINT	m_dwCryptRndChallengeFor;
    UINT	m_dwCryptRndChallengeFrom;
protected:
//>>> WiZaRd::IPv6 [Xanatos]
    void	Verified(const _CIPAddress& ForIP);
    //void	Verified(UINT dwForIP);
//<<< WiZaRd::IPv6 [Xanatos]
    EIdentState IdentState;
private:
    void			InitalizeIdent();
    CreditStruct*	m_pCredits;
    byte			m_abyPublicKey[80];			// even keys which are not verified will be stored here, and - if verified - copied into the struct
    uint8			m_nPublicKeyLen;
//>>> WiZaRd::IPv6 [Xanatos]
    _CIPAddress		m_dwIdentIP;
    _CIPAddress		m_dwWaitTimeIP;			   // client IP assigned to the waittime
    //UINT			m_dwIdentIP;
    //UINT			m_dwWaitTimeIP;			   // client IP assigned to the waittime
//<<< WiZaRd::IPv6 [Xanatos]
    UINT			m_dwSecureWaitTime;
    UINT			m_dwUnSecureWaitTime;

//>>> WiZaRd::CPU calm down
private:
    bool	m_bForceCheckScoreRatio;
    float	m_fSavedScoreRatio;
//<<< WiZaRd::CPU calm down
};

class CClientCreditsList
{
public:
    CClientCreditsList();
    ~CClientCreditsList();

    // return signature size, 0 = Failed | use sigkey param for debug only
//>>> WiZaRd::IPv6 [Xanatos]
    uint8	CreateSignature(CClientCredits* pTarget, uchar* pachOutput, uint8 nMaxSize, const _CIPAddress& ChallengeIP, uint8 byChaIPKind, CryptoPP::RSASSA_PKCS1v15_SHA_Signer* sigkey = NULL);
    bool	VerifyIdent(CClientCredits* pTarget, const uchar* pachSignature, uint8 nInputSize, const _CIPAddress& dwForIP, uint8 byChaIPKind);
//     uint8	CreateSignature(CClientCredits* pTarget, uchar* pachOutput, uint8 nMaxSize, UINT ChallengeIP, uint8 byChaIPKind, CryptoPP::RSASSA_PKCS1v15_SHA_Signer* sigkey = NULL);
//     bool	VerifyIdent(CClientCredits* pTarget, const uchar* pachSignature, uint8 nInputSize, UINT dwForIP, uint8 byChaIPKind);
//<<< WiZaRd::IPv6 [Xanatos]

    CClientCredits* GetCredit(const uchar* key) ;
    void	Process();
    uint8	GetPubKeyLen() const
    {
        return m_nMyPublicKeyLen;
    }
    byte*	GetPublicKey()
    {
        return m_abyMyPublicKey;
    }
    bool	CryptoAvailable();
protected:
    void	LoadList();
    void	SaveList();
    void	InitalizeCrypting();
    bool	CreateKeyPair();
#ifdef _DEBUG
    bool	Debug_CheckCrypting();
#endif
private:
    CMap<CCKey, const CCKey&, CClientCredits*, CClientCredits*> m_mapClients;
    UINT			m_nLastSaved;
    CryptoPP::RSASSA_PKCS1v15_SHA_Signer*		m_pSignkey;
    byte			m_abyMyPublicKey[80];
    uint8			m_nMyPublicKeyLen;
};
