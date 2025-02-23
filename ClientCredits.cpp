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
#include "stdafx.h"
#include <math.h>
#include "emule.h"
#include "ClientCredits.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "Opcodes.h"
#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4146) // Einem vorzeichenlosen Typ wurde ein un�rer Minus-Operator zugewiesen. Das Ergebnis ist weiterhin vorzeichenlos.
#pragma warning(disable:4505) // 'CryptoPP::StringNarrow': Nichtreferenzierte lokale Funktion wurde entfernt
#include <crypto51/base64.h>
#include <crypto51/osrng.h>
#include <crypto51/files.h>
#include <crypto51/sha.h>
#pragma warning(default:4505) // 'CryptoPP::StringNarrow': Nichtreferenzierte lokale Funktion wurde entfernt
#pragma warning(default:4146) // Einem vorzeichenlosen Typ wurde ein un�rer Minus-Operator zugewiesen. Das Ergebnis ist weiterhin vorzeichenlos.
#pragma warning(default:4702) // unreachable code
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#include "emuledlg.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CLIENTS_MET_FILENAME	_T("clients.met")

CClientCredits::CClientCredits(CreditStruct* in_credits)
{
    m_pCredits = in_credits;
    InitalizeIdent();
    m_dwUnSecureWaitTime = 0;
    m_dwSecureWaitTime = 0;
#ifdef IPV6_SUPPORT
    m_dwWaitTimeIP = CAddress(); //>>> WiZaRd::IPv6 [Xanatos]
#else
    m_dwWaitTimeIP = 0;
#endif
//>>> WiZaRd::CPU calm down
    m_bForceCheckScoreRatio = true;
    m_fSavedScoreRatio = 1.0f;
//<<< WiZaRd::CPU calm down
}

CClientCredits::CClientCredits(const uchar* key)
{
    m_pCredits = new CreditStruct;
    memset(m_pCredits, 0, sizeof(CreditStruct));
    md4cpy(m_pCredits->abyKey, key);
    InitalizeIdent();
    m_dwUnSecureWaitTime = ::GetTickCount();
    m_dwSecureWaitTime = ::GetTickCount();
#ifdef IPV6_SUPPORT
    m_dwWaitTimeIP = CAddress(); //>>> WiZaRd::IPv6 [Xanatos]
#else
    m_dwWaitTimeIP = 0;
#endif
//>>> WiZaRd::CPU calm down
    m_bForceCheckScoreRatio = true;
    m_fSavedScoreRatio = 1.0f;
//<<< WiZaRd::CPU calm down
}

CClientCredits::~CClientCredits()
{
    delete m_pCredits;
}

#ifdef IPV6_SUPPORT
void CClientCredits::AddDownloaded(const UINT bytes, const CAddress& dwForIP) //>>> WiZaRd::IPv6 [Xanatos]
#else
void CClientCredits::AddDownloaded(const UINT bytes, const UINT dwForIP)
#endif
{
    if ((GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable())
        return;

    //encode
    uint64 current = (((uint64)m_pCredits->nDownloadedHi << 32) | m_pCredits->nDownloadedLo) + bytes;

    //recode
    m_pCredits->nDownloadedLo = (UINT)current;
    m_pCredits->nDownloadedHi = (UINT)(current >> 32);
}

#ifdef IPV6_SUPPORT
void CClientCredits::AddUploaded(const UINT bytes, const CAddress& dwForIP) //>>> WiZaRd::IPv6 [Xanatos]
#else
void CClientCredits::AddUploaded(const UINT bytes, const UINT dwForIP)
#endif
{
    if ((GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable())
        return;

    //encode
    uint64 current = (((uint64)m_pCredits->nUploadedHi << 32) | m_pCredits->nUploadedLo) + bytes;

    //recode
    m_pCredits->nUploadedLo = (UINT)current;
    m_pCredits->nUploadedHi = (UINT)(current >> 32);

    m_bForceCheckScoreRatio = true; //>>> WiZaRd::CPU calm down
}

uint64 CClientCredits::GetUploadedTotal() const
{
    return ((uint64)m_pCredits->nUploadedHi << 32) | m_pCredits->nUploadedLo;
}

uint64 CClientCredits::GetDownloadedTotal() const
{
    return ((uint64)m_pCredits->nDownloadedHi << 32) | m_pCredits->nDownloadedLo;
}

#ifdef IPV6_SUPPORT
float CClientCredits::GetScoreRatio(const CAddress& dwForIP) /*const*/ //>>> WiZaRd::CPU calm down //>>> WiZaRd::IPv6 [Xanatos]
#else
float CClientCredits::GetScoreRatio(const UINT dwForIP) /*const*/ //>>> WiZaRd::CPU calm down
#endif
{
    // check the client ident status
    if ((GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable())
    {
        // bad guy - no credits for you
        return 1.0F;
    }

//>>> WiZaRd::CPU calm down
    if (!m_bForceCheckScoreRatio)
        return m_fSavedScoreRatio;
    m_bForceCheckScoreRatio = false;
//<<< WiZaRd::CPU calm down

    if (GetDownloadedTotal() < 1048576)
        return 1.0F;

    //same effect but less code :)
    float result = min(!GetUploadedTotal() ? 10.0F : (float)(((double)GetDownloadedTotal()*2.0)/(double)GetUploadedTotal()), (float)sqrt((float)(GetDownloadedTotal()/1048576.0) + 2.0F));
    if (result < 1.0F)
        result = 1.0F;
    else if (result > 10.0F)
        result = 10.0F;
    m_fSavedScoreRatio = result; //>>> WiZaRd::CPU calm down
    return result;
}


CClientCreditsList::CClientCreditsList()
{
    m_nLastSaved = ::GetTickCount();
    LoadList();

    InitalizeCrypting();
}

CClientCreditsList::~CClientCreditsList()
{
    SaveList();
    CClientCredits* cur_credit;
    CCKey tmpkey(0);
    POSITION pos = m_mapClients.GetStartPosition();
    while (pos)
    {
        m_mapClients.GetNextAssoc(pos, tmpkey, cur_credit);
        delete cur_credit;
    }
    delete m_pSignkey;
}

void CClientCreditsList::LoadList()
{
    CString strFileName = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + CLIENTS_MET_FILENAME;
    const int iOpenFlags = CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite;
    CSafeBufferedFile file;
    CFileException fexp;
    if (!file.Open(strFileName, iOpenFlags, &fexp))
    {
        if (fexp.m_cause != CFileException::fileNotFound)
        {
            CString strError(GetResString(IDS_ERR_LOADCREDITFILE));
            TCHAR szError[MAX_CFEXP_ERRORMSG];
            if (fexp.GetErrorMessage(szError, ARRSIZE(szError)))
            {
                strError += _T(" - ");
                strError += szError;
            }
            LogError(LOG_STATUSBAR, _T("%s"), strError);
        }
        return;
    }
    setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

    try
    {
        uint8 version = file.ReadUInt8();
        if (version != CREDITFILE_VERSION && version != CREDITFILE_VERSION_29)
        {
            LogWarning(GetResString(IDS_ERR_CREDITFILEOLD));
            file.Close();
            return;
        }

        // everything is ok, lets see if the backup exist...
        CString strBakFileName;
        strBakFileName.Format(_T("%s") CLIENTS_MET_FILENAME _T(".bak"), thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));

        DWORD dwBakFileSize = 0;
        BOOL bCreateBackup = TRUE;

        HANDLE hBakFile = ::CreateFile(strBakFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hBakFile != INVALID_HANDLE_VALUE)
        {
            // Ok, the backup exist, get the size
            dwBakFileSize = ::GetFileSize(hBakFile, NULL); //debug
            if (dwBakFileSize > (DWORD)file.GetLength())
            {
                // the size of the backup was larger then the org. file, something is wrong here, don't overwrite old backup..
                bCreateBackup = FALSE;
            }
            //else: backup is smaller or the same size as org. file, proceed with copying of file
            ::CloseHandle(hBakFile);
        }
        //else: the backup doesn't exist, create it

        if (bCreateBackup)
        {
            file.Close(); // close the file before copying

            if (!::CopyFile(strFileName, strBakFileName, FALSE))
                LogError(GetResString(IDS_ERR_MAKEBAKCREDITFILE));

            // reopen file
            CFileException fexp;
            if (!file.Open(strFileName, iOpenFlags, &fexp))
            {
                CString strError(GetResString(IDS_ERR_LOADCREDITFILE));
                TCHAR szError[MAX_CFEXP_ERRORMSG];
                if (fexp.GetErrorMessage(szError, ARRSIZE(szError)))
                {
                    strError += _T(" - ");
                    strError += szError;
                }
                LogError(LOG_STATUSBAR, _T("%s"), strError);
                return;
            }
            setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
            file.Seek(1, CFile::begin); //set filepointer behind file version byte
        }

        UINT count = file.ReadUInt32();
        m_mapClients.InitHashTable(count+5000); // TODO: should be prime number... and 20% larger

        const UINT dwExpired = time(NULL) - 12960000; // today - 150 day
        UINT cDeleted = 0;
        for (UINT i = 0; i < count; i++)
        {
            CreditStruct* newcstruct = new CreditStruct;
            memset(newcstruct, 0, sizeof(CreditStruct));
            if (version == CREDITFILE_VERSION_29)
                file.Read(newcstruct, sizeof(CreditStruct_29a));
            else
                file.Read(newcstruct, sizeof(CreditStruct));

            if (newcstruct->nLastSeen < dwExpired)
            {
                cDeleted++;
                delete newcstruct;
                continue;
            }

            CClientCredits* newcredits = new CClientCredits(newcstruct);
            m_mapClients.SetAt(CCKey(newcredits->GetKey()), newcredits);
        }
        file.Close();

        if (cDeleted>0)
            AddLogLine(false, GetResString(IDS_CREDITFILELOADED) + GetResString(IDS_CREDITSEXPIRED), count-cDeleted,cDeleted);
        else
            AddLogLine(false, GetResString(IDS_CREDITFILELOADED), count);
    }
    catch (CFileException* error)
    {
        if (error->m_cause == CFileException::endOfFile)
            LogError(LOG_STATUSBAR, GetResString(IDS_CREDITFILECORRUPT));
        else
        {
            TCHAR buffer[MAX_CFEXP_ERRORMSG];
            error->GetErrorMessage(buffer, ARRSIZE(buffer));
            LogError(LOG_STATUSBAR, GetResString(IDS_ERR_CREDITFILEREAD), buffer);
        }
        error->Delete();
    }
}

void CClientCreditsList::SaveList()
{
    if (thePrefs.GetLogFileSaving())
        AddDebugLogLine(false, _T("Saving clients credit list file \"%s\""), CLIENTS_MET_FILENAME);
    m_nLastSaved = ::GetTickCount();

    CString name = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + CLIENTS_MET_FILENAME;
    CFile file;// no buffering needed here since we swap out the entire array
    CFileException fexp;
    if (!file.Open(name, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp))
    {
        CString strError(GetResString(IDS_ERR_FAILED_CREDITSAVE));
        TCHAR szError[MAX_CFEXP_ERRORMSG];
        if (fexp.GetErrorMessage(szError, ARRSIZE(szError)))
        {
            strError += _T(" - ");
            strError += szError;
        }
        LogError(LOG_STATUSBAR, _T("%s"), strError);
        return;
    }

    UINT count = m_mapClients.GetCount();
    BYTE* pBuffer = new BYTE[count*sizeof(CreditStruct)];
    CClientCredits* cur_credit;
    CCKey tempkey(0);
    POSITION pos = m_mapClients.GetStartPosition();
    count = 0;
    while (pos)
    {
        m_mapClients.GetNextAssoc(pos, tempkey, cur_credit);
        if (cur_credit->GetUploadedTotal() || cur_credit->GetDownloadedTotal())
        {
            memcpy(pBuffer+(count*sizeof(CreditStruct)), cur_credit->GetDataStruct(), sizeof(CreditStruct));
            count++;
        }
    }

    try
    {
        uint8 version = CREDITFILE_VERSION;
        file.Write(&version, 1);
        file.Write(&count, 4);
        file.Write(pBuffer, count*sizeof(CreditStruct));
        if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning()))
            file.Flush();
        file.Close();
    }
    catch (CFileException* error)
    {
        CString strError(GetResString(IDS_ERR_FAILED_CREDITSAVE));
        TCHAR szError[MAX_CFEXP_ERRORMSG];
        if (error->GetErrorMessage(szError, ARRSIZE(szError)))
        {
            strError += _T(" - ");
            strError += szError;
        }
        LogError(LOG_STATUSBAR, _T("%s"), strError);
        error->Delete();
    }

    delete[] pBuffer;
}

CClientCredits* CClientCreditsList::GetCredit(const uchar* key)
{
    CClientCredits* result;
    CCKey tkey(key);
    if (!m_mapClients.Lookup(tkey, result))
    {
        result = new CClientCredits(key);
        m_mapClients.SetAt(CCKey(result->GetKey()), result);
    }
    result->SetLastSeen();
    return result;
}

void CClientCreditsList::Process()
{
    if (::GetTickCount() - m_nLastSaved > MIN2MS(13))
        SaveList();
}

void CClientCredits::InitalizeIdent()
{
    if (m_pCredits->nKeySize == 0)
    {
        memset(m_abyPublicKey,0,80); // for debugging
        m_nPublicKeyLen = 0;
        IdentState = IS_NOTAVAILABLE;
    }
    else
    {
        m_nPublicKeyLen = m_pCredits->nKeySize;
        memcpy(m_abyPublicKey, m_pCredits->abySecureIdent, m_nPublicKeyLen);
        IdentState = IS_IDNEEDED;
    }
    m_dwCryptRndChallengeFor = 0;
    m_dwCryptRndChallengeFrom = 0;
#ifdef IPV6_SUPPORT
    m_dwIdentIP = CAddress(); //>>> WiZaRd::IPv6 [Xanatos]
#else
    m_dwIdentIP = 0;
#endif
}

#ifdef IPV6_SUPPORT
void CClientCredits::Verified(const CAddress& dwForIP) //>>> WiZaRd::IPv6 [Xanatos]
#else
void CClientCredits::Verified(const UINT dwForIP)
#endif
{
    m_dwIdentIP = dwForIP;

    // client was verified, copy the key to store him if not done already
    if (m_pCredits->nKeySize == 0)
    {
        m_pCredits->nKeySize = m_nPublicKeyLen;
        memcpy(m_pCredits->abySecureIdent, m_abyPublicKey, m_nPublicKeyLen);
        if (GetDownloadedTotal() > 0)
        {
            // for security reason, we have to delete all prior credits here
            m_pCredits->nDownloadedHi = 0;
            m_pCredits->nDownloadedLo = 1;
            m_pCredits->nUploadedHi = 0;
            m_pCredits->nUploadedLo = 1; // in order to safe this client, set 1 byte
            if (thePrefs.GetVerbose())
                DEBUG_ONLY(AddDebugLogLine(false, _T("Credits deleted due to new SecureIdent")));
        }
    }
    IdentState = IS_IDENTIFIED;
}

bool CClientCredits::SetSecureIdent(const uchar* pachIdent, uint8 nIdentLen)  // verified Public key cannot change, use only if there is not public key yet
{
    if (MAXPUBKEYSIZE < nIdentLen || m_pCredits->nKeySize != 0)
        return false;
    memcpy(m_abyPublicKey,pachIdent, nIdentLen);
    m_nPublicKeyLen = nIdentLen;
    IdentState = IS_IDNEEDED;
    return true;
}

#ifdef IPV6_SUPPORT
EIdentState	CClientCredits::GetCurrentIdentState(const CAddress& dwForIP) const //>>> WiZaRd::IPv6 [Xanatos]
#else
EIdentState	CClientCredits::GetCurrentIdentState(const UINT dwForIP) const
#endif
{
    if (IdentState != IS_IDENTIFIED)
        return IdentState;
    else
    {
        if (dwForIP == m_dwIdentIP)
            return IS_IDENTIFIED;
        else
            return IS_IDBADGUY;
        // mod note: clients which just reconnected after an IP change and have to ident yet will also have this state for 1-2 seconds
        //		 so don't try to spam such clients with "bad guy" messages (besides: spam messages are always bad)
    }
}

using namespace CryptoPP;

void CClientCreditsList::InitalizeCrypting()
{
    m_nMyPublicKeyLen = 0;
    memset(m_abyMyPublicKey,0,80); // not really needed; better for debugging tho
    m_pSignkey = NULL;

    // check if keyfile is there
    bool bCreateNewKey = false;
    HANDLE hKeyFile = ::CreateFile(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("cryptkey.dat"), GENERIC_READ, FILE_SHARE_READ, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hKeyFile != INVALID_HANDLE_VALUE)
    {
        if (::GetFileSize(hKeyFile, NULL) == 0)
            bCreateNewKey = true;
        ::CloseHandle(hKeyFile);
    }
    else
        bCreateNewKey = true;
    if (bCreateNewKey)
        CreateKeyPair();

    // load key
    try
    {
        // load private key
        FileSource filesource(CStringA(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("cryptkey.dat")), true,new Base64Decoder);
        m_pSignkey = new RSASSA_PKCS1v15_SHA_Signer(filesource);
        // calculate and store public key
        RSASSA_PKCS1v15_SHA_Verifier pubkey(*m_pSignkey);
        ArraySink asink(m_abyMyPublicKey, 80);
        pubkey.DEREncode(asink);
        m_nMyPublicKeyLen = (uint8)asink.TotalPutLength();
        asink.MessageEnd();
    }
    catch (...)
    {
        delete m_pSignkey;
        m_pSignkey = NULL;
        LogError(LOG_STATUSBAR, GetResString(IDS_CRYPT_INITFAILED));
        ASSERT(0);
    }
    ASSERT(Debug_CheckCrypting());
}

bool CClientCreditsList::CreateKeyPair()
{
    try
    {
        AutoSeededRandomPool rng;
        InvertibleRSAFunction privkey;
        privkey.Initialize(rng,RSAKEYSIZE);

        Base64Encoder privkeysink(new FileSink(CStringA(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("cryptkey.dat"))));
        privkey.DEREncode(privkeysink);
        privkeysink.MessageEnd();

        if (thePrefs.GetLogSecureIdent())
            AddDebugLogLine(false, _T("Created new RSA keypair"));
    }
    catch (...)
    {
        if (thePrefs.GetVerbose())
            AddDebugLogLine(false, _T("Failed to create new RSA keypair"));
        ASSERT(false);
        return false;
    }
    return true;
}

uint8 CClientCreditsList::CreateSignature(CClientCredits* pTarget, uchar* pachOutput, uint8 nMaxSize,
#ifdef IPV6_SUPPORT
        const CAddress& ChallengeIP, uint8 byChaIPKind, //>>> WiZaRd::IPv6 [Xanatos]
#else
        const UINT ChallengeIP, uint8 byChaIPKind,
#endif
        CryptoPP::RSASSA_PKCS1v15_SHA_Signer* sigkey)
{
    // sigkey param is used for debug only
    if (sigkey == NULL)
        sigkey = m_pSignkey;

    // create a signature of the public key from pTarget
    ASSERT(pTarget);
    ASSERT(pachOutput);
    uint8 nResult;
    if (!CryptoAvailable())
        return 0;
    try
    {

        SecByteBlock sbbSignature(sigkey->SignatureLength());
        AutoSeededRandomPool rng;
#ifdef IPV6_SUPPORT
        byte abyBuffer[MAXPUBKEYSIZE+4+16+1]; //>>> WiZaRd::IPv6 [Xanatos]
#else
        byte abyBuffer[MAXPUBKEYSIZE+9];
#endif
        UINT keylen = pTarget->GetSecIDKeyLen();
        memcpy(abyBuffer,pTarget->GetSecureIdent(),keylen);
        // 4 additional bytes random data send from this client
        UINT challenge = pTarget->m_dwCryptRndChallengeFrom;
        ASSERT(challenge != 0);
        PokeUInt32(abyBuffer+keylen, challenge);
        uint16 ChIpLen = 0;
        if (byChaIPKind != 0)
        {
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
            if (ChallengeIP.GetType() == CAddress::IPv6)
            {
                ChIpLen = 16+1;
                memcpy(abyBuffer+keylen+4, ChallengeIP.Data(), 16);
                PokeUInt8(abyBuffer+keylen+16+4, byChaIPKind);
            }
            else
            {
                ChIpLen = 4+1;
                PokeUInt32(abyBuffer+keylen+4, _ntohl(ChallengeIP.ToIPv4()));
                PokeUInt8(abyBuffer+keylen+4+4, byChaIPKind);
            }
//<<< WiZaRd::IPv6 [Xanatos]
#else
            ChIpLen = 5;
            PokeUInt32(abyBuffer+keylen+4, ChallengeIP);
            PokeUInt8(abyBuffer+keylen+4+4, byChaIPKind);
#endif
        }
        sigkey->SignMessage(rng, abyBuffer ,keylen+4+ChIpLen , sbbSignature.begin());
        ArraySink asink(pachOutput, nMaxSize);
        asink.Put(sbbSignature.begin(), sbbSignature.size());
        nResult = (uint8)asink.TotalPutLength();
    }
    catch (...)
    {
        ASSERT(false);
        nResult = 0;
    }
    return nResult;
}

bool CClientCreditsList::VerifyIdent(CClientCredits* pTarget, const uchar* pachSignature, uint8 nInputSize,
#ifdef IPV6_SUPPORT
                                     const CAddress& dwForIP, uint8 byChaIPKind) //>>> WiZaRd::IPv6 [Xanatos]
#else
                                     const UINT dwForIP, uint8 byChaIPKind)
#endif
{
    ASSERT(pTarget);
    ASSERT(pachSignature);
    if (!CryptoAvailable())
    {
        pTarget->IdentState = IS_NOTAVAILABLE;
        return false;
    }
    bool bResult;
    try
    {
        StringSource ss_Pubkey((byte*)pTarget->GetSecureIdent(),pTarget->GetSecIDKeyLen(),true,0);
        RSASSA_PKCS1v15_SHA_Verifier pubkey(ss_Pubkey);
        // 4 additional bytes random data send from this client +5 bytes v2
#ifdef IPV6_SUPPORT
        byte abyBuffer[MAXPUBKEYSIZE+4+16+1]; //>>> WiZaRd::IPv6 [Xanatos]
#else
        byte abyBuffer[MAXPUBKEYSIZE+9];
#endif
        memcpy(abyBuffer,m_abyMyPublicKey,m_nMyPublicKeyLen);
        UINT challenge = pTarget->m_dwCryptRndChallengeFor;
        ASSERT(challenge != 0);
        PokeUInt32(abyBuffer+m_nMyPublicKeyLen, challenge);

        // v2 security improvements (not supported by 29b, not used as default by 29c)
        uint8 nChIpSize = 0;
        if (byChaIPKind != 0)
        {
#ifdef IPV6_SUPPORT
            CAddress ChallengeIP; //>>> WiZaRd::IPv6 [Xanatos]
#else
            nChIpSize = 5;
            UINT ChallengeIP = 0;
#endif
            switch (byChaIPKind)
            {
                case CRYPT_CIP_LOCALCLIENT:
                    ChallengeIP = dwForIP;
                    break;
                case CRYPT_CIP_REMOTECLIENT:
                    // removed servers
                {
                    if (thePrefs.GetLogSecureIdent())
                        AddDebugLogLine(false, _T("Warning: Maybe SecureHash Ident fails because LocalIP is unknown"));
#ifdef IPV6_SUPPORT
                    ChallengeIP = CAddress(_ntohl(GetLocalIP())); //>>> WiZaRd::IPv6 [Xanatos]
#else
                    ChallengeIP = GetLocalIP();
#endif
                }
                break;
                case CRYPT_CIP_NONECLIENT: // maybe not supported in future versions
#ifdef IPV6_SUPPORT
                    ChallengeIP = CAddress(); //>>> WiZaRd::IPv6 [Xanatos]
#else
                    ChallengeIP = 0;
#endif
                    break;
            }
#ifdef IPV6_SUPPORT
//>>> WiZaRd::IPv6 [Xanatos]
            if (ChallengeIP.GetType() == CAddress::IPv6)
            {
                nChIpSize = 16+1;
                memcpy(abyBuffer+m_nMyPublicKeyLen+4, ChallengeIP.Data(), 16);
                PokeUInt8(abyBuffer+m_nMyPublicKeyLen+16+4, byChaIPKind);
            }
            else
            {
                nChIpSize = 4+1;
                PokeUInt32(abyBuffer+m_nMyPublicKeyLen+4, _ntohl(ChallengeIP.ToIPv4()));
                PokeUInt8(abyBuffer+m_nMyPublicKeyLen+4+4, byChaIPKind);
            }
//<<< WiZaRd::IPv6 [Xanatos]
#else
            PokeUInt32(abyBuffer+m_nMyPublicKeyLen+4, ChallengeIP);
            PokeUInt8(abyBuffer+m_nMyPublicKeyLen+4+4, byChaIPKind);
#endif
        }
        //v2 end

        bResult = pubkey.VerifyMessage(abyBuffer, m_nMyPublicKeyLen+4+nChIpSize, pachSignature, nInputSize);
    }
    catch (...)
    {
        if (thePrefs.GetVerbose())
            AddDebugLogLine(false, _T("Error: Unknown exception in %hs"), __FUNCTION__);
        //ASSERT(0);
        bResult = false;
    }
    if (!bResult)
    {
        if (pTarget->IdentState == IS_IDNEEDED)
            pTarget->IdentState = IS_IDFAILED;
    }
    else
    {
        pTarget->Verified(dwForIP);
    }
    return bResult;
}

bool CClientCreditsList::CryptoAvailable()
{
    return (m_nMyPublicKeyLen > 0 && m_pSignkey != 0);
}


#ifdef _DEBUG
bool CClientCreditsList::Debug_CheckCrypting()
{
    // create random key
    AutoSeededRandomPool rng;

    RSASSA_PKCS1v15_SHA_Signer priv(rng, 384);
    RSASSA_PKCS1v15_SHA_Verifier pub(priv);

    byte abyPublicKey[80];
    ArraySink asink(abyPublicKey, 80);
    pub.DEREncode(asink);
    uint8 PublicKeyLen = (uint8)asink.TotalPutLength();
    asink.MessageEnd();
    UINT challenge = rand();
    // create fake client which pretends to be this emule
    CreditStruct* newcstruct = new CreditStruct;
    memset(newcstruct, 0, sizeof(CreditStruct));
    CClientCredits* newcredits = new CClientCredits(newcstruct);
    newcredits->SetSecureIdent(m_abyMyPublicKey,m_nMyPublicKeyLen);
    newcredits->m_dwCryptRndChallengeFrom = challenge;
    // create signature with fake priv key
    uchar pachSignature[200];
    memset(pachSignature,0,200);
#ifdef IPV6_SUPPORT
    uint8 sigsize = CreateSignature(newcredits,pachSignature,200,CAddress(),false, &priv); //>>> WiZaRd::IPv6 [Xanatos]
#else
    uint8 sigsize = CreateSignature(newcredits,pachSignature,200,0,false, &priv);
#endif

    // next fake client uses the random created public key
    CreditStruct* newcstruct2 = new CreditStruct;
    memset(newcstruct2, 0, sizeof(CreditStruct));
    CClientCredits* newcredits2 = new CClientCredits(newcstruct2);
    newcredits2->m_dwCryptRndChallengeFor = challenge;

    // if you uncomment one of the following lines the check has to fail
    //abyPublicKey[5] = 34;
    //m_abyMyPublicKey[5] = 22;
    //pachSignature[5] = 232;

    newcredits2->SetSecureIdent(abyPublicKey,PublicKeyLen);

    //now verify this signature - if it's true everything is fine
#ifdef IPV6_SUPPORT
    bool bResult = VerifyIdent(newcredits2,pachSignature,sigsize,CAddress(),0); //>>> WiZaRd::IPv6 [Xanatos]
#else
    bool bResult = VerifyIdent(newcredits2,pachSignature,sigsize,0,0);
#endif

    delete newcredits;
    delete newcredits2;

    return bResult;
}
#endif

#ifdef IPV6_SUPPORT
UINT CClientCredits::GetSecureWaitStartTime(const CAddress& dwForIP) //>>> WiZaRd::IPv6 [Xanatos]
#else
UINT CClientCredits::GetSecureWaitStartTime(const UINT dwForIP)
#endif
{
    if (m_dwUnSecureWaitTime == 0 || m_dwSecureWaitTime == 0)
        SetSecWaitStartTime(dwForIP);

    if (m_pCredits->nKeySize != 0) 	// this client is a SecureHash Client
    {
        if (GetCurrentIdentState(dwForIP) == IS_IDENTIFIED)  // good boy
        {
            return m_dwSecureWaitTime;
        }
        else 	// not so good boy
        {
            if (dwForIP == m_dwWaitTimeIP)
            {
                return m_dwUnSecureWaitTime;
            }
            else 	// bad boy
            {
                // this can also happen if the client has not identified himself yet, but will do later - so maybe he is not a bad boy :) .
                CString buffer2, buffer;
                /*for (uint16 i = 0;i != 16;i++){
                	buffer2.Format("%02X",this->m_pCredits->abyKey[i]);
                	buffer+=buffer2;
                }
                if (thePrefs.GetLogSecureIdent())
                	AddDebugLogLine(false,"Warning: WaitTime resetted due to Invalid Ident for Userhash %s", buffer);*/

                m_dwUnSecureWaitTime = ::GetTickCount();
                m_dwWaitTimeIP = dwForIP;
                return m_dwUnSecureWaitTime;
            }
        }
    }
    else 	// not a SecureHash Client - handle it like before for now (no security checks)
    {
        return m_dwUnSecureWaitTime;
    }
}

#ifdef IPV6_SUPPORT
void CClientCredits::SetSecWaitStartTime(const CAddress& dwForIP) //>>> WiZaRd::IPv6 [Xanatos]
#else
void CClientCredits::SetSecWaitStartTime(const UINT dwForIP)
#endif
{
    m_dwUnSecureWaitTime = ::GetTickCount()-1;
    m_dwSecureWaitTime = ::GetTickCount()-1;
    m_dwWaitTimeIP = dwForIP;
}

void CClientCredits::ClearWaitStartTime()
{
    m_dwUnSecureWaitTime = 0;
    m_dwSecureWaitTime = 0;
}
