// parts of this file are based on work from pan One (http://home-3.tiscali.nl/~meost/pms/)
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
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#ifdef _DEBUG
#include "DebugHelpers.h"
#endif
#include "emule.h"
#include "KnownFile.h"
#include "KnownFileList.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "ClientList.h"
#include "opcodes.h"
#include "ini2.h"
#include "FrameGrabThread.h"
#include "CxImage/xImage.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "PartFile.h"
#include "Packets.h"
#include "Kademlia/Kademlia/SearchManager.h"
#include "Kademlia/Kademlia/Entry.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "SafeFile.h"
#include "shahashset.h"
#include "Log.h"
#include "MD4.h"
#include "Collection.h"
#include "emuledlg.h"
#include "SharedFilesWnd.h"
#include "MediaInfo.h"
#pragma warning(disable:4100) // unreferenced formal parameter
#include <id3/tag.h>
#include <id3/misc_support.h>
#pragma warning(default:4100) // unreferenced formal parameter
extern wchar_t *ID3_GetStringW(const ID3_Frame *frame, ID3_FieldID fldName);
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "UploadQueue.h" //>>> WiZaRd::PowerShare
#include "./Mod/ProgressIndicator.h" //>>> WiZaRd::HashProgress Indication
#include "./Mod/NetF/PartStatus.h" //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
#include "./Mod/Median.h" //>>> WiZaRd::Complete Files As Median

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Meta data version
// -----------------
//	0	untrusted meta data which was received via search results
//	1	trusted meta data, Unicode (strings where not stored correctly)
//	2	0.49c: trusted meta data, Unicode
#define	META_DATA_VER	2

IMPLEMENT_DYNAMIC(CKnownFile, CShareableFile)

CKnownFile::CKnownFile()
{
    m_bShareOnlyTheNeed = true; //>>> WiZaRd::Intelligent SOTN
    guifileupdatetime = ::GetTickCount() + 1000; //>>> WiZaRd::PowerShare - CPU!
    m_bPowerShared = false; //>>> WiZaRd::PowerShare
    m_iNameContinuityBad = 0; //>>> FDC [BlueSonicBoy]
    m_iPartCount = 0;
    m_iED2KPartCount = 0;
    m_tUtcLastModified = (UINT)-1;
    if (thePrefs.GetNewAutoUp())
    {
        m_iUpPriority = PR_HIGH;
        m_bAutoUpPriority = true;
    }
    else
    {
        m_iUpPriority = PR_NORMAL;
        m_bAutoUpPriority = false;
    }
    statistic.fileParent = this;
    (void)m_strComment;
    kadFileSearchID = 0;
    SetLastPublishTimeKadSrc(0,0);
    m_bCompleteSrcUpdateNecessary = false;
    m_nCompleteSourcesCount = 1;
    m_nCompleteSourcesCountLo = 1;
    m_nCompleteSourcesCountHi = 1;
    m_uMetaDataVer = 0;
    m_lastPublishTimeKadSrc = 0;
    m_lastPublishTimeKadNotes = 0;
    m_lastBuddyIP = 0;
    m_pCollection = NULL;
    m_timeLastSeen = 0;
    m_bAICHRecoverHashSetAvailable = false;
    m_uiQueuedCount = 0; //>>> Queued Count
//>>> WiZaRd::FileHealth
    m_dwLastFileHealthCalc = ::GetTickCount();
    m_fLastHealthValue = 0;
//<<< WiZaRd::FileHealth
}

CKnownFile::~CKnownFile()
{
    delete m_pCollection;
}

#ifdef _DEBUG
void CKnownFile::AssertValid() const
{
    CAbstractFile::AssertValid();

    (void)m_tUtcLastModified;
    (void)statistic;
    CHECK_BOOL(m_bCompleteSrcUpdateNecessary);
    (void)m_nCompleteSourcesCount;
    (void)m_nCompleteSourcesCountLo;
    (void)m_nCompleteSourcesCountHi;
    m_ClientUploadList.AssertValid();
    m_AvailPartFrequency.AssertValid();
    m_SOTNAvailPartFrequency.AssertValid(); //>>> WiZaRd::Intelligent SOTN
    (void)m_strDirectory;
    (void)m_strFilePath;
    (void)m_iPartCount;
    (void)m_iED2KPartCount;
    ASSERT(m_iUpPriority == PR_VERYLOW || m_iUpPriority == PR_LOW || m_iUpPriority == PR_NORMAL || m_iUpPriority == PR_HIGH || m_iUpPriority == PR_VERYHIGH);
    CHECK_BOOL(m_bAutoUpPriority);
    (void)s_ShareStatusBar;
    (void)kadFileSearchID;
    (void)m_lastPublishTimeKadSrc;
    (void)m_lastPublishTimeKadNotes;
    (void)m_lastBuddyIP;
    (void)wordlist;
}

void CKnownFile::Dump(CDumpContext& dc) const
{
    CAbstractFile::Dump(dc);
}
#endif

CBarShader CKnownFile::s_ShareStatusBar(16);

void CKnownFile::DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool  bFlat) const
{
    s_ShareStatusBar.SetFileSize(GetFileSize());
    s_ShareStatusBar.SetHeight(rect->bottom - rect->top);
    s_ShareStatusBar.SetWidth(rect->right - rect->left);

    if (m_ClientUploadList.GetSize() > 0 || m_nCompleteSourcesCountLo > 1)
    {
        // We have info about chunk frequency in the net, so we will color the chunks we have after perceived availability.
        const COLORREF crMissing = RGB(255, 0, 0);
        s_ShareStatusBar.Fill(crMissing);

        if (!onlygreyrect)
        {
            COLORREF crProgress;
            COLORREF crHave;
            COLORREF crPending;
            if (bFlat)
            {
                crProgress = RGB(0, 150, 0);
                crHave = RGB(0, 0, 0);
                crPending = RGB(255,208,0);
            }
            else
            {
                crProgress = RGB(0, 224, 0);
                crHave = RGB(104, 104, 104);
                crPending = RGB(255, 208, 0);
            }

            UINT tempCompleteSources = m_nCompleteSourcesCountLo;
            if (tempCompleteSources > 0)
                --tempCompleteSources;

            for (UINT i = 0; i < GetPartCount(); i++)
            {
                UINT frequency = tempCompleteSources;
                if (!m_AvailPartFrequency.IsEmpty())
                    frequency = max(m_AvailPartFrequency[i], tempCompleteSources);

                if (frequency > 0)
                {
                    COLORREF color = RGB(0, (22*(frequency-1) >= 210)? 0:210-(22*(frequency-1)), 255);
                    s_ShareStatusBar.FillRange(PARTSIZE*(uint64)(i),PARTSIZE*(uint64)(i+1),color);
                }
            }
        }
    }
    else
    {
        // We have no info about chunk frequency in the net, so just color the chunk we have as black.
        COLORREF crNooneAsked;
        if (bFlat)
            crNooneAsked = RGB(0, 0, 0);
        else
            crNooneAsked = RGB(104, 104, 104);
        s_ShareStatusBar.Fill(crNooneAsked);
    }

    s_ShareStatusBar.Draw(dc, rect->left, rect->top, bFlat);
}

void CKnownFile::UpdateFileRatingCommentAvail(bool bForceUpdate)
{
    bool bOldHasComment = m_bHasComment;
    UINT uOldUserRatings = m_uUserRating;

    m_bHasComment = false;
    UINT uRatings = 0;
    UINT uUserRatings = 0;

    for (POSITION pos = m_kadNotes.GetHeadPosition(); pos != NULL;)
    {
        Kademlia::CEntry* entry = m_kadNotes.GetNext(pos);
        if (!m_bHasComment && !entry->GetStrTagValue(TAG_DESCRIPTION).IsEmpty())
            m_bHasComment = true;
        UINT rating = (UINT)entry->GetIntTagValue(TAG_FILERATING);
        if (rating != 0)
        {
            uRatings++;
            uUserRatings += rating;
        }
    }

    if (uRatings)
        m_uUserRating = (UINT)ROUND((float)uUserRatings / uRatings);
    else
        m_uUserRating = 0;

    if (bOldHasComment != m_bHasComment || uOldUserRatings != m_uUserRating || bForceUpdate)
        theApp.sharedfiles->UpdateFile(this);
}

void CKnownFile::UpdatePartsInfo()
{
    // Cache part count
    UINT partcount = GetPartCount();

    // Reset part counters
    if ((UINT)m_AvailPartFrequency.GetSize() < partcount)
        m_AvailPartFrequency.SetSize(partcount);
//>>> WiZaRd::Intelligent SOTN
    if ((UINT)m_SOTNAvailPartFrequency.GetSize() < partcount)
        m_SOTNAvailPartFrequency.SetSize(partcount);
//<<< WiZaRd::Intelligent SOTN
    for (UINT i = 0; i < partcount; i++)
    {
        m_AvailPartFrequency[i] = 0;
        m_SOTNAvailPartFrequency[i] = 0; //>>> WiZaRd::Intelligent SOTN
    }

    for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0;)
    {
        CUpDownClient* cur_src = m_ClientUploadList.GetNext(pos);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        const CPartStatus* cur_status = cur_src->GetUpPartStatus();
        //This could be a partfile that just completed.. Many of these clients will not have this information.
        if (cur_status != NULL && cur_src->GetUpPartCount() == partcount)
            //if(cur_src->m_abyUpPartStatus && cur_src->GetUpPartCount() == partcount)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        {
            for (UINT i = 0; i < partcount; i++)
            {
                if (cur_src->IsUpPartAvailable(i))
                    m_AvailPartFrequency[i] += 1;
            }
        }
        cur_src->GetUploadingAndUploadedPart(m_AvailPartFrequency, m_SOTNAvailPartFrequency); //>>> WiZaRd::Intelligent SOTN
    }

    m_bCompleteSrcUpdateNecessary = true;

    if (theApp.sharedfiles)
        theApp.sharedfiles->UpdateFile(this);
}

void CKnownFile::AddUploadingClient(CUpDownClient* client)
{
    POSITION pos = m_ClientUploadList.Find(client); // to be sure
    if (pos == NULL)
    {
        m_ClientUploadList.AddTail(client);
        //UpdateAutoUpPriority(); //>>> WiZaRd::Queued Count
    }
}

void CKnownFile::RemoveUploadingClient(CUpDownClient* client)
{
    POSITION pos = m_ClientUploadList.Find(client); // to be sure
    if (pos != NULL)
    {
        m_ClientUploadList.RemoveAt(pos);
        //UpdateAutoUpPriority(); //>>> WiZaRd::Queued Count
    }
}

#ifdef _DEBUG
void Dump(const Kademlia::WordList& wordlist)
{
    Kademlia::WordList::const_iterator it;
    for (it = wordlist.begin(); it != wordlist.end(); it++)
    {
        const CStringW& rstrKeyword = *it;
        TRACE(L"  %ls\n", rstrKeyword);
    }
}
#endif

void CKnownFile::SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars, bool bRemoveControlChars)
{
    CKnownFile* pFile = NULL;

    // If this is called within the sharedfiles object during startup,
    // we cannot reference it yet..

    if (theApp.sharedfiles)
        pFile = theApp.sharedfiles->GetFileByID(GetFileHash());

    if (pFile && pFile == this)
        theApp.sharedfiles->RemoveKeywords(this);

    CAbstractFile::SetFileName(pszFileName, bReplaceInvalidFileSystemChars, true, bRemoveControlChars);
    m_verifiedFileType = FILETYPE_UNKNOWN;

    wordlist.clear();
    if (m_pCollection)
    {
        CString sKeyWords;
        sKeyWords.Format(_T("%s %s"), m_pCollection->GetCollectionAuthorKeyString(), GetFileName());
        Kademlia::CSearchManager::GetWords(sKeyWords, &wordlist);
    }
    else
        Kademlia::CSearchManager::GetWords(GetFileName(), &wordlist);

    if (pFile && pFile == this)
        theApp.sharedfiles->AddKeywords(this);
}

bool CKnownFile::CreateFromFile(LPCTSTR in_directory, LPCTSTR in_filename, LPVOID pvProgressParam)
{
    SetPath(in_directory);
    SetFileName(in_filename);

    // open file
    CString strFilePath;
    if (!_tmakepathlimit(strFilePath.GetBuffer(MAX_PATH), NULL, in_directory, in_filename, NULL))
    {
        LogError(GetResString(IDS_ERR_FILEOPEN), in_filename, L"");
        return false;
    }
    strFilePath.ReleaseBuffer();
    SetFilePath(strFilePath);
    FILE* file = _tfsopen(strFilePath, _T("rbS"), _SH_DENYNO); // can not use _SH_DENYWR because we may access a completing part file
    if (!file)
    {
        LogError(GetResString(IDS_ERR_FILEOPEN) + _T(" - %s"), strFilePath, L"", _tcserror(errno));
        return false;
    }

    // set filesize
    __int64 llFileSize = _filelengthi64(_fileno(file));
    if ((uint64)llFileSize > MAX_EMULE_FILE_SIZE)
    {
        if (llFileSize == -1i64)
            LogError(_T("Failed to hash file \"%s\" - %s"), strFilePath, _tcserror(errno));
        else
            LogError(_T("Skipped hashing of file \"%s\" - File size exceeds limit."), strFilePath);
        fclose(file);
        return false; // not supported by network
    }
    SetFileSize((uint64)llFileSize);

    // we are reading the file data later in 8K blocks, adjust the internal file stream buffer accordingly
    setvbuf(file, NULL, _IOFBF, 1024*8*2);

    m_AvailPartFrequency.SetSize(GetPartCount());
    m_SOTNAvailPartFrequency.SetSize(GetPartCount()); //>>> WiZaRd::Intelligent SOTN
    for (UINT i = 0; i < GetPartCount(); i++)
    {
        m_AvailPartFrequency[i] = 0;
        m_SOTNAvailPartFrequency[i] = 0; //>>> WiZaRd::Intelligent SOTN
    }

    // create hashset
    CAICHRecoveryHashSet cAICHHashSet(this, m_nFileSize);
    uint64 togo = m_nFileSize;
    UINT hashcount;
//>>> WiZaRd::HashProgress Indication
    CProgressIndicator progressIndicator;
    progressIndicator.Initialise(strFilePath, GetResString(IDS_HASHINGFILE) + L" %.2f%%", GetResString(IDS_HASH_FILE_FINISHED));
    double dProgress = 0;
//<<< WiZaRd::HashProgress Indication
    for (hashcount = 0; togo >= PARTSIZE;)
    {
        CAICHHashTree* pBlockAICHHashTree = cAICHHashSet.m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, PARTSIZE);
        ASSERT(pBlockAICHHashTree != NULL);

        uchar* newhash = new uchar[16];
        if (!CreateHash(file, PARTSIZE, newhash, pBlockAICHHashTree))
        {
            LogError(_T("Failed to hash file \"%s\" - %s"), strFilePath, _tcserror(errno));
            fclose(file);
            delete[] newhash;
            return false;
        }

        if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning())  // in case of shutdown while still hashing
        {
            fclose(file);
            delete[] newhash;
            return false;
        }

        m_FileIdentifier.GetRawMD4HashSet().Add(newhash);
        togo -= PARTSIZE;
        hashcount++;

//>>> WiZaRd::HashProgress Indication
        dProgress = (double)GetFileSize() != 0.0 ? (double)(GetFileSize() - togo)/(double)GetFileSize()*100.0 : 0.0;
        progressIndicator.UpdateProgress(dProgress);
//<<< WiZaRd::HashProgress Indication
        if (pvProgressParam && theApp.emuledlg && theApp.emuledlg->IsRunning())
        {
            ASSERT(((CKnownFile*)pvProgressParam)->IsKindOf(RUNTIME_CLASS(CKnownFile)));
            ASSERT(((CKnownFile*)pvProgressParam)->GetFileSize() == GetFileSize());
//>>> WiZaRd::HashProgress Indication
            //UINT uProgress = (UINT)(uint64)(((uint64)(GetFileSize() - togo) * 100) / GetFileSize());
            const UINT uProgress = (UINT)dProgress;
//<<< WiZaRd::HashProgress Indication
            ASSERT(uProgress <= 100);
            VERIFY(PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)pvProgressParam));
        }
    }

    CAICHHashTree* pBlockAICHHashTree;
    if (togo == 0)
        pBlockAICHHashTree = NULL; // sha hashtree doesnt takes hash of 0-sized data
    else
    {
        pBlockAICHHashTree = cAICHHashSet.m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, togo);
        ASSERT(pBlockAICHHashTree != NULL);
    }

    uchar* lasthash = new uchar[16];
    md4clr(lasthash);
    if (!CreateHash(file, togo, lasthash, pBlockAICHHashTree))
    {
        LogError(_T("Failed to hash file \"%s\" - %s"), strFilePath, _tcserror(errno));
        fclose(file);
        delete[] lasthash;
        return false;
    }

    cAICHHashSet.ReCalculateHash(false);
    if (cAICHHashSet.VerifyHashTree(true))
    {
        cAICHHashSet.SetStatus(AICH_HASHSETCOMPLETE);
        m_FileIdentifier.SetAICHHash(cAICHHashSet.GetMasterHash());
        if (!m_FileIdentifier.SetAICHHashSet(cAICHHashSet))
        {
            ASSERT(0);
            DebugLogError(_T("CreateFromFile() - failed to create AICH PartHashSet out of RecoveryHashSet - %s"), GetFileName());
        }
        if (!cAICHHashSet.SaveHashSet())
            LogError(LOG_STATUSBAR, GetResString(IDS_SAVEACFAILED));
        else
            SetAICHRecoverHashSetAvailable(true);
    }
    else
    {
        // now something went pretty wrong
        DebugLogError(LOG_STATUSBAR, _T("Failed to calculate AICH Hashset from file %s"), GetFileName());
    }

    if (!hashcount)
    {
        m_FileIdentifier.SetMD4Hash(lasthash);
        delete[] lasthash;
    }
    else
    {
        m_FileIdentifier.GetRawMD4HashSet().Add(lasthash);
        m_FileIdentifier.CalculateMD4HashByHashSet(false);
    }

//>>> WiZaRd::HashProgress Indication
    dProgress = 100;
    progressIndicator.UpdateProgress(dProgress);
//<<< WiZaRd::HashProgress Indication
    if (pvProgressParam && theApp.emuledlg && theApp.emuledlg->IsRunning())
    {
        ASSERT(((CKnownFile*)pvProgressParam)->IsKindOf(RUNTIME_CLASS(CKnownFile)));
        ASSERT(((CKnownFile*)pvProgressParam)->GetFileSize() == GetFileSize());
        VERIFY(PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, 100, (LPARAM)pvProgressParam));
    }

    // set lastwrite date
    struct _stat32i64 fileinfo;
    if (_fstat32i64(file->_file, &fileinfo) == 0)
    {
        m_tUtcLastModified = fileinfo.st_mtime;
        AdjustNTFSDaylightFileTime(m_tUtcLastModified, strFilePath);
    }

    fclose(file);
    file = NULL;

    // Add filetags
    UpdateMetaDataTags();

    UpdatePartsInfo();

    return true;
}

bool CKnownFile::CreateAICHHashSetOnly()
{
    ASSERT(!IsPartFile());

    FILE* file = _tfsopen(GetFilePath(), _T("rbS"), _SH_DENYNO); // can not use _SH_DENYWR because we may access a completing part file
    if (!file)
    {
        LogError(GetResString(IDS_ERR_FILEOPEN) + _T(" - %s"), GetFilePath(), L"", _tcserror(errno));
        return false;
    }
    // we are reading the file data later in 8K blocks, adjust the internal file stream buffer accordingly
    setvbuf(file, NULL, _IOFBF, 1024*8*2);

    // create aichhashset
    CAICHRecoveryHashSet cAICHHashSet(this, m_nFileSize);
    uint64 togo = m_nFileSize;
    UINT hashcount;
//>>> WiZaRd::HashProgress Indication
    CProgressIndicator progressIndicator;
    progressIndicator.Initialise(GetFilePath(), GetResString(IDS_HASHINGFILE) + L" %.2f%% (" + GetResString(IDS_AICHHASH) + L")", GetResString(IDS_HASH_FILE_FINISHED) + L" (" + GetResString(IDS_AICHHASH) + L")");
    double dProgress = 0;
//<<< WiZaRd::HashProgress Indication
    for (hashcount = 0; togo >= PARTSIZE;)
    {
        CAICHHashTree* pBlockAICHHashTree = cAICHHashSet.m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, PARTSIZE);
        ASSERT(pBlockAICHHashTree != NULL);
        if (!CreateHash(file, PARTSIZE, NULL, pBlockAICHHashTree))
        {
            LogError(_T("Failed to hash file \"%s\" - %s"), GetFilePath(), _tcserror(errno));
            fclose(file);
            return false;
        }

        if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning())  // in case of shutdown while still hashing
        {
            fclose(file);
            return false;
        }

        togo -= PARTSIZE;
        hashcount++;

//>>> WiZaRd::HashProgress Indication
        dProgress = (double)GetFileSize() != 0.0 ? (double)(GetFileSize() - togo)/(double)GetFileSize()*100.0 : 0.0;
        progressIndicator.UpdateProgress(dProgress);
//<<< WiZaRd::HashProgress Indication
    }

    if (togo != 0)
    {
        CAICHHashTree* pBlockAICHHashTree = cAICHHashSet.m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, togo);
        ASSERT(pBlockAICHHashTree != NULL);
        if (!CreateHash(file, togo, NULL, pBlockAICHHashTree))
        {
            LogError(_T("Failed to hash file \"%s\" - %s"), GetFilePath(), _tcserror(errno));
            fclose(file);
            return false;
        }
    }

    cAICHHashSet.ReCalculateHash(false);
    if (cAICHHashSet.VerifyHashTree(true))
    {
        cAICHHashSet.SetStatus(AICH_HASHSETCOMPLETE);
        if (m_FileIdentifier.HasAICHHash() && m_FileIdentifier.GetAICHHash() != cAICHHashSet.GetMasterHash())
            theApp.knownfiles->AICHHashChanged(&m_FileIdentifier.GetAICHHash(), cAICHHashSet.GetMasterHash(), this);
        else
            theApp.knownfiles->AICHHashChanged(NULL, cAICHHashSet.GetMasterHash(), this);
        m_FileIdentifier.SetAICHHash(cAICHHashSet.GetMasterHash());
        if (!m_FileIdentifier.SetAICHHashSet(cAICHHashSet))
        {
            ASSERT(0);
            DebugLogError(_T("CreateAICHHashSetOnly() - failed to create AICH PartHashSet out of RecoveryHashSet - %s"), GetFileName());
        }
        if (!cAICHHashSet.SaveHashSet())
            LogError(LOG_STATUSBAR, GetResString(IDS_SAVEACFAILED));
        else
        {
//>>> WiZaRd::HashProgress Indication
            dProgress = 100;
            progressIndicator.UpdateProgress(dProgress);
//<<< WiZaRd::HashProgress Indication
            SetAICHRecoverHashSetAvailable(true);
        }
    }
    else
    {
        // now something went pretty wrong
        DebugLogError(LOG_STATUSBAR, _T("Failed to calculate AICH Hashset from file %s"), GetFileName());
    }

    fclose(file);
    file = NULL;

    return true;
}

void CKnownFile::SetFileSize(EMFileSize nFileSize)
{
    CAbstractFile::SetFileSize(nFileSize);

    // Examples of parthashs, hashsets and filehashs for different filesizes
    // according the ed2k protocol
    //----------------------------------------------------------------------
    //
    //File size: 3 bytes
    //File hash: 2D55E87D0E21F49B9AD25F98531F3724
    //Nr. hashs: 0
    //
    //
    //File size: 1*PARTSIZE
    //File hash: A72CA8DF7F07154E217C236C89C17619
    //Nr. hashs: 2
    //Hash[  0]: 4891ED2E5C9C49F442145A3A5F608299
    //Hash[  1]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
    //
    //
    //File size: 1*PARTSIZE + 1 byte
    //File hash: 2F620AE9D462CBB6A59FE8401D2B3D23
    //Nr. hashs: 2
    //Hash[  0]: 121795F0BEDE02DDC7C5426D0995F53F
    //Hash[  1]: C329E527945B8FE75B3C5E8826755747
    //
    //
    //File size: 2*PARTSIZE
    //File hash: A54C5E562D5E03CA7D77961EB9A745A4
    //Nr. hashs: 3
    //Hash[  0]: B3F5CE2A06BF403BFB9BFFF68BDDC4D9
    //Hash[  1]: 509AA30C9EA8FC136B1159DF2F35B8A9
    //Hash[  2]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
    //
    //
    //File size: 3*PARTSIZE
    //File hash: 5E249B96F9A46A18FC2489B005BF2667
    //Nr. hashs: 4
    //Hash[  0]: 5319896A2ECAD43BF17E2E3575278E72
    //Hash[  1]: D86EF157D5E49C5ED502EDC15BB5F82B
    //Hash[  2]: 10F2D5B1FCB95C0840519C58D708480F
    //Hash[  3]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
    //
    //
    //File size: 3*PARTSIZE + 1 byte
    //File hash: 797ED552F34380CAFF8C958207E40355
    //Nr. hashs: 4
    //Hash[  0]: FC7FD02CCD6987DCF1421F4C0AF94FB8
    //Hash[  1]: 2FE466AF8A7C06DA3365317B75A5ACFE
    //Hash[  2]: 873D3BF52629F7C1527C6E8E473C1C30
    //Hash[  3]: BCE50BEE7877BB07BB6FDA56BFE142FB
    //

    // File size       Data parts      ED2K parts      ED2K part hashs		AICH part hashs
    // -------------------------------------------------------------------------------------------
    // 1..PARTSIZE-1   1               1               0(!)					0 (!)
    // PARTSIZE        1               2(!)            2(!)					0 (!)
    // PARTSIZE+1      2               2               2					2
    // PARTSIZE*2      2               3(!)            3(!)					2
    // PARTSIZE*2+1    3               3               3					3

    if (nFileSize == (uint64)0)
    {
        ASSERT(0);
        m_iPartCount = 0;
        m_iED2KPartCount = 0;
        return;
    }

    // nr. of data parts
    ASSERT((uint64)(((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE) <= (UINT)USHRT_MAX);
    m_iPartCount = (uint16)(((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE);

    // nr. of parts to be used with OP_FILESTATUS
    m_iED2KPartCount = (uint16)((uint64)nFileSize / PARTSIZE + 1);
}

bool CKnownFile::LoadTagsFromFile(CFileDataIO* file)
{
    UINT tagcount = file->ReadUInt32();
    bool bHadAICHHashSetTag = false;
    for (UINT j = 0; j < tagcount; j++)
    {
        CTag* newtag = new CTag(file, false);
        switch (newtag->GetNameID())
        {
            case FT_FILENAME:
            {
                ASSERT(newtag->IsStr());
                if (newtag->IsStr())
                {
                    if (GetFileName().IsEmpty())
                        SetFileName(newtag->GetStr());
                }
                delete newtag;
                break;
            }
            case FT_FILESIZE:
            {
                ASSERT(newtag->IsInt64(true));
                if (newtag->IsInt64(true))
                {
                    SetFileSize(newtag->GetInt64());
                    m_AvailPartFrequency.SetSize(GetPartCount());
                    m_SOTNAvailPartFrequency.SetSize(GetPartCount()); //>>> WiZaRd::Intelligent SOTN
                    for (UINT i = 0; i < GetPartCount(); i++)
                    {
                        m_AvailPartFrequency[i] = 0;
                        m_SOTNAvailPartFrequency[i] = 0; //>>> WiZaRd::Intelligent SOTN
                    }
                }
                delete newtag;
                break;
            }
            case FT_ATTRANSFERRED:
            {
                ASSERT(newtag->IsInt());
                if (newtag->IsInt())
                    statistic.SetAllTimeTransferred(newtag->GetInt());
                delete newtag;
                break;
            }
            case FT_ATTRANSFERREDHI:
            {
                ASSERT(newtag->IsInt());
                if (newtag->IsInt())
                    statistic.SetAllTimeTransferred(((uint64)newtag->GetInt() << 32) | (UINT)statistic.GetAllTimeTransferred());
                delete newtag;
                break;
            }
            case FT_ATREQUESTED:
            {
                ASSERT(newtag->IsInt());
                if (newtag->IsInt())
                    statistic.SetAllTimeRequests(newtag->GetInt());
                delete newtag;
                break;
            }
            case FT_ATACCEPTED:
            {
                ASSERT(newtag->IsInt());
                if (newtag->IsInt())
                    statistic.SetAllTimeAccepts(newtag->GetInt());
                delete newtag;
                break;
            }
            case FT_ULPRIORITY:
            {
                ASSERT(newtag->IsInt());
                if (newtag->IsInt())
                {
                    m_iUpPriority = (uint8)newtag->GetInt();
                    if (m_iUpPriority == PR_AUTO)
                    {
                        m_iUpPriority = PR_HIGH;
                        m_bAutoUpPriority = true;
                    }
                    else
                    {
                        if (m_iUpPriority != PR_VERYLOW && m_iUpPriority != PR_LOW && m_iUpPriority != PR_NORMAL && m_iUpPriority != PR_HIGH && m_iUpPriority != PR_VERYHIGH)
                            m_iUpPriority = PR_NORMAL;
                        m_bAutoUpPriority = false;
                    }
                }
                delete newtag;
                break;
            }
            case FT_KADLASTPUBLISHSRC:
            {
                ASSERT(newtag->IsInt());
                if (newtag->IsInt())
                    SetLastPublishTimeKadSrc(newtag->GetInt(), 0);
                if (GetLastPublishTimeKadSrc() > (UINT)time(NULL)+KADEMLIAREPUBLISHTIMES)
                {
                    //There may be a possibility of an older client that saved a random number here.. This will check for that..
                    SetLastPublishTimeKadSrc(0,0);
                }
                delete newtag;
                break;
            }
            case FT_KADLASTPUBLISHNOTES:
            {
                ASSERT(newtag->IsInt());
                if (newtag->IsInt())
                    SetLastPublishTimeKadNotes(newtag->GetInt());
                delete newtag;
                break;
            }
            case FT_FLAGS:
                // Misc. Flags
                // ------------------------------------------------------------------------------
                // Bits  3-0: Meta data version
                //				0	untrusted meta data which was received via search results
                //				1	trusted meta data, Unicode (strings where not stored correctly)
                //				2	0.49c: trusted meta data, Unicode
                // Bits 31-4: Reserved
                ASSERT(newtag->IsInt());
                if (newtag->IsInt())
                    m_uMetaDataVer = newtag->GetInt() & 0x0F;
                delete newtag;
                break;
            // old tags: as long as they are not needed, take the chance to purge them
            case FT_PERMISSIONS:
                ASSERT(newtag->IsInt());
                delete newtag;
                break;
            case FT_KADLASTPUBLISHKEY:
                ASSERT(newtag->IsInt());
                delete newtag;
                break;
            case FT_AICH_HASH:
            {
                if (!newtag->IsStr())
                {
                    //ASSERT(0); uncomment later
                    break;
                }
                CAICHHash hash;
                if (DecodeBase32(newtag->GetStr(),hash) == (UINT)CAICHHash::GetHashSize())
                    m_FileIdentifier.SetAICHHash(hash);
                else
                    ASSERT(0);
                delete newtag;
                break;
            }
            case FT_LASTSHARED:
                if (newtag->IsInt())
                    m_timeLastSeen = newtag->GetInt();
                else
                    ASSERT(0);
                delete newtag;
                break;
            case FT_AICHHASHSET:
                if (newtag->IsBlob())
                {
                    CSafeMemFile aichHashSetFile(newtag->GetBlob(), newtag->GetBlobSize());
                    m_FileIdentifier.LoadAICHHashsetFromFile(&aichHashSetFile, false);
                    aichHashSetFile.Detach();
                    bHadAICHHashSetTag = true;
                }
                else
                    ASSERT(0);
                delete newtag;
                break;
            default:
//>>> WiZaRd::PowerShare
                if (!newtag->GetNameID() && newtag->IsInt() && newtag->GetName())
                {
                    LPCSTR tagname = newtag->GetName();
                    if (strcmp(tagname, FT_POWERSHARE) == 0)
                    {
                        SetPowerShared(newtag->GetInt() == 1);
                        delete newtag;
                        break;
                    }
//>>> WiZaRd::Intelligent SOTN
                    if (strcmp(tagname, FT_SOTN) == 0)
                    {
                        SetShareOnlyTheNeed(newtag->GetInt() == 1);
                        delete newtag;
                        break;
                    }
//<<< WiZaRd::Intelligent SOTN
                }
//<<< WiZaRd::PowerShare
                ConvertED2KTag(newtag);
                if (newtag)
                    taglist.Add(newtag);
        }
    }
    if (bHadAICHHashSetTag)
    {
        if (!m_FileIdentifier.VerifyAICHHashSet())
            DebugLogError(_T("Failed to load AICH Part HashSet for file %s"), GetFileName());
        //else
        //	DebugLog(_T("Succeeded to load AICH Part HashSet for file %s"), GetFileName());
    }

    // 05-J�n-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
    // the chance to clean any available meta data tags and provide only tags which were determined by us.
    // It's a brute force method, but that wrong meta data is driving me crazy because wrong meta data is even worse than
    // missing meta data.
    if (m_uMetaDataVer == 0)
        RemoveMetaDataTags();
    else if (m_uMetaDataVer == 1)
    {
        // Meta data tags v1 did not store Unicode strings correctly.
        // Remove broken Unicode string meta data tags from v1, but keep the integer tags.
        RemoveBrokenUnicodeMetaDataTags();
        m_uMetaDataVer = META_DATA_VER;
    }

    return true;
}

bool CKnownFile::LoadDateFromFile(CFileDataIO* file)
{
    m_tUtcLastModified = file->ReadUInt32();
    return true;
}

bool CKnownFile::LoadFromFile(CFileDataIO* file)
{
    // SLUGFILLER: SafeHash - load first, verify later
    bool ret1 = LoadDateFromFile(file);
    bool ret2 = m_FileIdentifier.LoadMD4HashsetFromFile(file, false);
    bool ret3 = LoadTagsFromFile(file);
    UpdatePartsInfo();
    return ret1 && ret2 && ret3 && m_FileIdentifier.HasExpectedMD4HashCount();// Final hash-count verification, needs to be done after the tags are loaded.
    // SLUGFILLER: SafeHash
}

bool CKnownFile::WriteToFile(CFileDataIO* file)
{
    // date
    file->WriteUInt32(m_tUtcLastModified);

    // hashset
    m_FileIdentifier.WriteMD4HashsetToFile(file);

    UINT uTagCount = 0;
    ULONG uTagCountFilePos = (ULONG)file->GetPosition();
    file->WriteUInt32(uTagCount);

    CTag nametag(FT_FILENAME, GetFileName());
    nametag.WriteTagToFile(file, utf8strOptBOM);
    uTagCount++;

    CTag sizetag(FT_FILESIZE, m_nFileSize, IsLargeFile());
    sizetag.WriteTagToFile(file);
    uTagCount++;

    //AICH Filehash
    if (m_FileIdentifier.HasAICHHash())
    {
        CTag aichtag(FT_AICH_HASH, m_FileIdentifier.GetAICHHash().GetString());
        aichtag.WriteTagToFile(file);
        uTagCount++;
    }

    // last shared
    static bool sDbgWarnedOnZero = false;
    if (!sDbgWarnedOnZero && m_timeLastSeen == 0)
    {
        DebugLog(_T("Unknown last seen date on stored file(s), upgrading from old version?"));
        sDbgWarnedOnZero = true;
    }
    ASSERT(m_timeLastSeen <= time(NULL));
    time_t timeLastShared = (m_timeLastSeen > 0 && m_timeLastSeen <= time(NULL)) ? m_timeLastSeen : time(NULL);
    CTag lastSharedTag(FT_LASTSHARED, (UINT)timeLastShared);
    lastSharedTag.WriteTagToFile(file);
    uTagCount++;

    if (!ShouldPartiallyPurgeFile())
    {
        // those tags are no longer stored for long time not seen (shared) known files to tidy up known.met and known2.met

        // AICH Part HashSet
        // no point in permanently storing the AICH part hashset if we need to rehash the file anyway to fetch the full recovery hashset
        // the tag will make the known.met incompatible with emule version prior 0.44a - but that one is nearly 6 years old
        if (m_FileIdentifier.HasAICHHash() && m_FileIdentifier.HasExpectedAICHHashCount())
        {
            UINT nAICHHashSetSize = (CAICHHash::GetHashSize() * (m_FileIdentifier.GetAvailableAICHPartHashCount() + 1)) + 2;
            BYTE* pHashBuffer = new BYTE[nAICHHashSetSize];
            CSafeMemFile hashSetFile(pHashBuffer, nAICHHashSetSize);
            bool bWriteHashSet = true;
            try
            {
                m_FileIdentifier.WriteAICHHashsetToFile(&hashSetFile);
            }
            catch (CFileException* pError)
            {
                ASSERT(0);
                DebugLogError(_T("Memfile Error while storing AICH Part HashSet"));
                bWriteHashSet = false;
                delete[] hashSetFile.Detach();
                pError->Delete();
            }
            if (bWriteHashSet)
            {
                CTag tagAICHHashSet(FT_AICHHASHSET, hashSetFile.Detach(), nAICHHashSetSize);
                tagAICHHashSet.WriteTagToFile(file);
                uTagCount++;
            }
        }

        // statistic
        if (statistic.GetAllTimeTransferred())
        {
            CTag attag1(FT_ATTRANSFERRED, (UINT)statistic.GetAllTimeTransferred());
            attag1.WriteTagToFile(file);
            uTagCount++;

            CTag attag4(FT_ATTRANSFERREDHI, (UINT)(statistic.GetAllTimeTransferred() >> 32));
            attag4.WriteTagToFile(file);
            uTagCount++;
        }

        if (statistic.GetAllTimeRequests())
        {
            CTag attag2(FT_ATREQUESTED, statistic.GetAllTimeRequests());
            attag2.WriteTagToFile(file);
            uTagCount++;
        }

        if (statistic.GetAllTimeAccepts())
        {
            CTag attag3(FT_ATACCEPTED, statistic.GetAllTimeAccepts());
            attag3.WriteTagToFile(file);
            uTagCount++;
        }

        // priority N permission
        CTag priotag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : m_iUpPriority);
        priotag.WriteTagToFile(file);
        uTagCount++;


        if (m_lastPublishTimeKadSrc)
        {
            CTag kadLastPubSrc(FT_KADLASTPUBLISHSRC, m_lastPublishTimeKadSrc);
            kadLastPubSrc.WriteTagToFile(file);
            uTagCount++;
        }

        if (m_lastPublishTimeKadNotes)
        {
            CTag kadLastPubNotes(FT_KADLASTPUBLISHNOTES, m_lastPublishTimeKadNotes);
            kadLastPubNotes.WriteTagToFile(file);
            uTagCount++;
        }

        if (m_uMetaDataVer > 0)
        {
            // Misc. Flags
            // ------------------------------------------------------------------------------
            // Bits  3-0: Meta data version
            //				0	untrusted meta data which was received via search results
            //				1	trusted meta data, Unicode (strings where not stored correctly)
            //				2	0.49c: trusted meta data, Unicode
            // Bits 31-4: Reserved
            ASSERT(m_uMetaDataVer <= 0x0F);
            UINT uFlags = m_uMetaDataVer & 0x0F;
            CTag tagFlags(FT_FLAGS, uFlags);
            tagFlags.WriteTagToFile(file);
            uTagCount++;
        }

        // other tags
        for (int j = 0; j < taglist.GetCount(); j++)
        {
            if (taglist[j]->IsStr() || taglist[j]->IsInt())
            {
                taglist[j]->WriteTagToFile(file, utf8strOptBOM);
                uTagCount++;
            }
        }

//>>> WiZaRd::PowerShare
        if (IsPowerShared())
        {
            CTag powersharetag(FT_POWERSHARE, 1);
            powersharetag.WriteTagToFile(file);
            ++uTagCount;
        }
//<<< WiZaRd::PowerShare
//>>> WiZaRd::Intelligent SOTN
        if (m_bShareOnlyTheNeed)
        {
            CTag sotn(FT_SOTN, 1);
            sotn.WriteTagToFile(file);
            ++uTagCount;
        }
//<<< WiZaRd::Intelligent SOTN
    }

    file->Seek(uTagCountFilePos, CFile::begin);
    file->WriteUInt32(uTagCount);
    file->Seek(0, CFile::end);

    return true;
}

void CKnownFile::CreateHash(CFile* pFile, uint64 Length, uchar* pMd4HashOut, CAICHHashTree* pShaHashOut)
{
    ASSERT(pFile != NULL);
    ASSERT(pMd4HashOut != NULL || pShaHashOut != NULL);

    uint64  Required = Length;
    uchar   X[64*128];
    uint64	posCurrentEMBlock = 0;
    uint64	nIACHPos = 0;
    CMD4 md4;
    CAICHHashAlgo* pHashAlg = NULL;
    if (pShaHashOut != NULL)
        pHashAlg = CAICHRecoveryHashSet::GetNewHashAlgo();

    while (Required >= 64)
    {
        UINT len;
        if ((Required / 64) > sizeof(X)/(64 * sizeof(X[0])))
            len = sizeof(X)/(64 * sizeof(X[0]));
        else
            len = (UINT)Required / 64;
        pFile->Read(X, len*64);

        // SHA hash needs 180KB blocks
        if (pShaHashOut != NULL && pHashAlg != NULL)
        {
            if (nIACHPos + len*64 >= EMBLOCKSIZE)
            {
                UINT nToComplete = (UINT)(EMBLOCKSIZE - nIACHPos);
                pHashAlg->Add(X, nToComplete);
                ASSERT(nIACHPos + nToComplete == EMBLOCKSIZE);
                pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
                posCurrentEMBlock += EMBLOCKSIZE;
                pHashAlg->Reset();
                pHashAlg->Add(X+nToComplete,(len*64) - nToComplete);
                nIACHPos = (len*64) - nToComplete;
            }
            else
            {
                pHashAlg->Add(X, len*64);
                nIACHPos += len*64;
            }
        }

        if (pMd4HashOut != NULL)
        {
            md4.Add(X, len*64);
        }
        Required -= len*64;
    }

    Required = Length % 64;
    if (Required != 0)
    {
        pFile->Read(X, (UINT)Required);

        if (pShaHashOut != NULL)
        {
            if (nIACHPos + Required >= EMBLOCKSIZE)
            {
                UINT nToComplete = (UINT)(EMBLOCKSIZE - nIACHPos);
                pHashAlg->Add(X, nToComplete);
                ASSERT(nIACHPos + nToComplete == EMBLOCKSIZE);
                pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
                posCurrentEMBlock += EMBLOCKSIZE;
                pHashAlg->Reset();
                pHashAlg->Add(X+nToComplete, (UINT)(Required - nToComplete));
                nIACHPos = Required - nToComplete;
            }
            else
            {
                pHashAlg->Add(X, (UINT)Required);
                nIACHPos += Required;
            }
        }
    }
    if (pShaHashOut != NULL)
    {
        if (nIACHPos > 0)
        {
            pShaHashOut->SetBlockHash(nIACHPos, posCurrentEMBlock, pHashAlg);
            posCurrentEMBlock += nIACHPos;
        }
        ASSERT(posCurrentEMBlock == Length);
        VERIFY(pShaHashOut->ReCalculateHash(pHashAlg, false));
    }

    if (pMd4HashOut != NULL)
    {
        md4.Add(X, (UINT)Required);
        md4.Finish();
        md4cpy(pMd4HashOut, md4.GetHash());
    }

    delete pHashAlg;
}

bool CKnownFile::CreateHash(FILE* fp, uint64 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut)
{
    bool bResult = false;
    CStdioFile file(fp);
    try
    {
        CreateHash(&file, uSize, pucHash, pShaHashOut);
        bResult = true;
    }
    catch (CFileException* ex)
    {
        ex->Delete();
    }
    return bResult;
}

bool CKnownFile::CreateHash(const uchar* pucData, UINT uSize, uchar* pucHash, CAICHHashTree* pShaHashOut)
{
    bool bResult = false;
    CMemFile file(const_cast<uchar*>(pucData), uSize);
    try
    {
        CreateHash(&file, uSize, pucHash, pShaHashOut);
        bResult = true;
    }
    catch (CFileException* ex)
    {
        ex->Delete();
    }
    return bResult;
}

Packet*	CKnownFile::CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions) const
{
    if (m_ClientUploadList.IsEmpty())
        return NULL;

    if (md4cmp(forClient->GetUploadFileID(), GetFileHash())!=0)
    {
        // should never happen
        DEBUG_ONLY(DebugLogError(L"*** %hs - client (%s) upload file \"%s\" does not match file \"%s\"", __FUNCTION__, forClient->DbgGetClientInfo(), DbgGetFileInfo(forClient->GetUploadFileID()), GetFileName()));
        ASSERT(0);
        return NULL;
    }

    // check whether client has either no download status at all or a download status which is valid for this file
    if (!(forClient->GetUpPartCount()==0 && forClient->GetUpPartStatus()==NULL)
            && !(forClient->GetUpPartCount()==GetPartCount() && forClient->GetUpPartStatus()!=NULL))
    {
        // should never happen
        DEBUG_ONLY(DebugLogError(L"*** %hs - part count (%u) of client (%s) does not match part count (%u) of file \"%s\"", __FUNCTION__, forClient->GetUpPartCount(), forClient->DbgGetClientInfo(), GetPartCount(), GetFileName()));
        ASSERT(0);
        return NULL;
    }

    CSafeMemFile data(1024);

    uint8 byUsedVersion;
    bool bIsSX2Packet;
    if (forClient->SupportsSourceExchange2() && byRequestedVersion > 0)
    {
        // the client uses SourceExchange2 and requested the highest version he knows
        // and we send the highest version we know, but of course not higher than his request
//>>> WiZaRd::ExtendedXS [Xanatos]
        if (forClient->SupportsExtendedSourceExchange())
            byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGEEXT_VERSION);
        else
//<<< WiZaRd::ExtendedXS [Xanatos]
            byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
        bIsSX2Packet = true;
        data.WriteUInt8(byUsedVersion);
//>>> WiZaRd::ExtendedXS [Xanatos]
        // WiZaRd: Fix extendedXS: the byUsedVersion value is used below to determine which data is written
        // The remote client will set it to its XS2 version if we are using extXS and thus, XS packets will be malformed and cause exceptions if we don't sync it here
        if (forClient->SupportsExtendedSourceExchange())
            byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
//<<< WiZaRd::ExtendedXS [Xanatos]

        // we don't support any special SX2 options yet, reserved for later use
        if (nRequestedOptions != 0)
            DebugLogWarning(L"Client requested unknown options for SourceExchange2: %u (%s)", nRequestedOptions, forClient->DbgGetClientInfo());
    }
    else
    {
        byUsedVersion = forClient->GetSourceExchange1Version();
        bIsSX2Packet = false;
        if (forClient->SupportsSourceExchange2())
            DebugLogWarning(L"Client which announced to support SX2 sent SX1 packet instead (%s)", forClient->DbgGetClientInfo());
    }

    uint16 nCount = 0;
    data.WriteHash16(forClient->GetUploadFileID());
    data.WriteUInt16(nCount);

    UINT cDbgNoSrc = 0;
    bool bNeeded = false;
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    const CPartStatus* rcvstatus = forClient->GetUpPartStatus();
    //const uint8* rcvstatus = forClient->GetUpPartStatus();
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (rcvstatus == NULL)
    {
        ASSERT(forClient->GetUpPartCount() == 0);
        TRACE(L"%hs, requesting client has no chunk status - %s", __FUNCTION__, forClient->DbgGetClientInfo());
    }
    else
        ASSERT(forClient->GetUpPartCount() == GetPartCount());
    for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0;)
    {
        bNeeded = false;
        /*const*/ CUpDownClient* cur_src = m_ClientUploadList.GetNext(pos);

        // some rare issue seen in a crashdumps, hopefully fixed already, but to be sure we double check here
        // TODO: remove check next version, as it uses resources and shouldn't be necessary
        if (!theApp.clientlist->IsValidClient(cur_src))
        {
#ifdef _BETA
            throw new CUserException();
#endif
            ASSERT(0);
            DebugLogError(L"Invalid client in uploading list for file %s", GetFileName());
            continue;
        }
#ifdef NAT_TRAVERSAL
        if ((cur_src->HasLowID() && !cur_src->SupportsNatTraversal()) || cur_src == forClient || !(cur_src->GetUploadState() == US_UPLOADING || cur_src->GetUploadState() == US_ONUPLOADQUEUE)) //>>> WiZaRd::NatTraversal [Xanatos]
#else
        if (cur_src->HasLowID() || cur_src == forClient || !(cur_src->GetUploadState() == US_UPLOADING || cur_src->GetUploadState() == US_ONUPLOADQUEUE))
#endif
            continue;
        if (!cur_src->IsEd2kClient())
            continue;

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
        const CPartStatus* const srcstatus = cur_src->GetUpPartStatus();
        //const uint8* srcstatus = cur_src->GetUpPartStatus();
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        if (srcstatus)
        {
            if (cur_src->GetUpPartCount() == GetPartCount())
            {
                if (rcvstatus)
                {
                    // only send sources which have needed parts for this client
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                    bNeeded = rcvstatus->IsNeeded(srcstatus);
                    /*for (UINT x = 0; x < GetUpPartCount(); x++)
                    {
                        if (srcstatus[x] && !rcvstatus[x])
                        {
                            bNeeded = true;
                            break;
                        }
                    }*/
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                }
                else
                {
                    // remote client does not support chunk status, search sources which have at least one complete part
                    // we could even sort the list of sources by available chunks to return as much sources as possible which
                    // have the most available chunks. but this could be a noticeable performance problem.
                    // We know this client is valid. But don't know the part count status.. So, currently we just send them.
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                    bNeeded = rcvstatus->IsNeeded(srcstatus);
                    /*for (UINT x = 0; x < GetUpPartCount(); x++)
                    {
                        if (srcstatus[x])
                        {
                            bNeeded = true;
                            break;
                        }
                    }*/
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                }
            }
            else
            {
                // should never happen
                if (thePrefs.GetVerbose())
                    DEBUG_ONLY(DebugLogError(L"*** %hs - found source (%s) with wrong partcount (%u) attached to file \"%s\" (partcount=%u)", __FUNCTION__, cur_src->DbgGetClientInfo(), cur_src->GetPartCount(), GetFileName(), GetPartCount()));
            }
        }
        else
        {
            ++cDbgNoSrc;
            // This client doesn't support chunk status. So just send it and hope for the best.
            bNeeded = true;
        }

        if (bNeeded)
        {
            ++nCount;
            UINT dwID;
//>>> WiZaRd::NatTraversal [Xanatos]
            if (byUsedVersion >= 3 && !cur_src->HasLowID())
                //if (byUsedVersion >= 3)
//<<< WiZaRd::NatTraversal [Xanatos]
                dwID = cur_src->GetUserIDHybrid();
            else
//>>> WiZaRd::NatTraversal [Xanatos]
                dwID = ntohl(cur_src->GetUserIDHybrid()); // consistency for NAT-T
            //dwID = cur_src->GetIP();
//<<< WiZaRd::NatTraversal [Xanatos]
            data.WriteUInt32(dwID);
            data.WriteUInt16(cur_src->GetUserPort());
//>>> WiZaRd::ExtendedXS [Xanatos]
            if (forClient->SupportsExtendedSourceExchange())
                cur_src->WriteExtendedSourceExchangeData(data);
            else
//<<< WiZaRd::ExtendedXS [Xanatos]
            {
                data.WriteUInt32(cur_src->GetServerIP());
                data.WriteUInt16(cur_src->GetServerPort());
            }
            if (byUsedVersion >= 2)
                data.WriteHash16(cur_src->GetUserHash());
            if (byUsedVersion >= 4)
#ifdef NAT_TRAVERSAL
                data.WriteUInt8(cur_src->GetConnectOptions(true, forClient->SupportsNatTraversal(), forClient->SupportsNatTraversal())); //>>> WiZaRd::NatTraversal [Xanatos]
#else
                data.WriteUInt8(cur_src->GetConnectOptions(true, false));
#endif
            if (nCount > 500)
                break;
        }
    }
    TRACE(L"%hs: Out of %u clients, %u had no valid chunk status\n", __FUNCTION__, m_ClientUploadList.GetCount(), cDbgNoSrc);
    Packet* result = NULL;
    if (nCount)
    {
        data.Seek(bIsSX2Packet ? 17 : 16, SEEK_SET);
        data.WriteUInt16(nCount);

        result = new Packet(&data, OP_EMULEPROT);
        result->opcode = bIsSX2Packet ? OP_ANSWERSOURCES2 : OP_ANSWERSOURCES;
        // (1+)16+2+501*(4+2+4+2+16+1) = 14547 (14548) bytes max.
        if (result->size > 354)
            result->PackPacket();
        if (thePrefs.GetDebugSourceExchange())
            AddDebugLogLine(false, L"SX: created response extXS=%s, SX2=%s, Version=%u; Count=%u for %s on file \"%s\"", forClient->SupportsExtendedSourceExchange() ? GetResString(IDS_YES) : GetResString(IDS_NO), bIsSX2Packet ? GetResString(IDS_YES) : GetResString(IDS_NO), byUsedVersion, nCount, forClient->DbgGetClientInfo(), GetFileName()); //>>> WiZaRd::ExtendedXS [Xanatos]
    }
    return result;
}

Packet* CKnownFile::GetEmptyXSPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions) const
{
    CSafeMemFile data(1+16+2); // max packet size

    uint8 byUsedVersion;
    bool bIsSX2Packet;
    if (forClient->SupportsSourceExchange2() && byRequestedVersion > 0)
    {
        // the client uses SourceExchange2 and requested the highest version he knows
        // and we send the highest version we know, but of course not higher than his request
//>>> WiZaRd::ExtendedXS [Xanatos]
        if (forClient->SupportsExtendedSourceExchange())
            byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGEEXT_VERSION);
        else
//<<< WiZaRd::ExtendedXS [Xanatos]
            byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
        bIsSX2Packet = true;
        data.WriteUInt8(byUsedVersion);
//>>> WiZaRd::ExtendedXS [Xanatos]
        // WiZaRd: Fix extendedXS: the byUsedVersion value is used below to determine which data is written
        // The remote client will set it to its XS2 version if we are using extXS and thus, XS packets will be malformed and cause exceptions if we don't sync it here
        if (forClient->SupportsExtendedSourceExchange())
            byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
//<<< WiZaRd::ExtendedXS [Xanatos]

        // we don't support any special SX2 options yet, reserved for later use
        if (nRequestedOptions != 0)
            DebugLogWarning(L"Client requested unknown options for SourceExchange2: %u (%s)", nRequestedOptions, forClient->DbgGetClientInfo());
    }
    else
    {
        byUsedVersion = forClient->GetSourceExchange1Version();
        bIsSX2Packet = false;
        if (forClient->SupportsSourceExchange2())
            DebugLogWarning(L"Client which announced to support SX2 sent SX1 packet instead (%s)", forClient->DbgGetClientInfo());
    }

    data.WriteHash16(forClient->GetUploadFileID());
    data.WriteUInt16(0);

    Packet* result = new Packet(&data, OP_EMULEPROT);
    result->opcode = bIsSX2Packet ? OP_ANSWERSOURCES2 : OP_ANSWERSOURCES;
    // (1+)16+2+501*(4+2+4+2+16+1) = 14547 (14548) bytes max.
    //	if (result->size > 354)
    //		result->PackPacket();
    if (thePrefs.GetDebugSourceExchange())
        AddDebugLogLine(false, L"SX: created empty response extXS=%s, SX2=%s, Version=%u; Count=%u for %s on file \"%s\"", forClient->SupportsExtendedSourceExchange() ? GetResString(IDS_YES) : GetResString(IDS_NO), bIsSX2Packet ? GetResString(IDS_YES) : GetResString(IDS_NO), byUsedVersion, 0, forClient->DbgGetClientInfo(), GetFileName()); //>>> WiZaRd::ExtendedXS [Xanatos]
    return result;
}

void CKnownFile::SetFileComment(LPCTSTR pszComment)
{
    if (m_strComment.Compare(pszComment) != 0)
    {
        SetLastPublishTimeKadNotes(0);
        CIni ini(thePrefs.GetFileCommentsFilePath(), md4str(GetFileHash()));
        ini.WriteStringUTF8(_T("Comment"), pszComment);
        m_strComment = pszComment;

        for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0;)
            m_ClientUploadList.GetNext(pos)->SetCommentDirty();
    }
}

void CKnownFile::SetFileRating(UINT uRating)
{
    if (m_uRating != uRating)
    {
        SetLastPublishTimeKadNotes(0);
        CIni ini(thePrefs.GetFileCommentsFilePath(), md4str(GetFileHash()));
        ini.WriteInt(_T("Rate"), uRating);
        m_uRating = uRating;

        for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0;)
            m_ClientUploadList.GetNext(pos)->SetCommentDirty();
    }
}

void CKnownFile::UpdateAutoUpPriority()
{
    if (!IsAutoUpPriority())
        return;

//>>> WiZaRd::Improved Auto Prio
    // calc average src and adapt the prio to it
    UINT maxsrc = theApp.uploadqueue->GetWaitingUserCount();
    float avg = 0.0f;
    UINT nCount = theApp.sharedfiles->GetQueuedFilesCount();
    if (nCount)
        avg = (float)maxsrc/nCount;
    else
        avg = (float)maxsrc;

    const uint8 toSet = CalcPrioFromSrcAverage(GetRealQueuedCount(), avg);
    if (GetUpPriority() != toSet)
    {
        SetUpPriority(toSet);
        theApp.sharedfiles->UpdateFile(this);
    }
    /*if (GetQueuedCount() > 20)
    {
        if (GetUpPriority() != PR_LOW)
        {
            SetUpPriority(PR_LOW);
            theApp.sharedfiles->UpdateFile(this);
        }
        return;
    }
    if (GetQueuedCount() > 1)
    {
        if (GetUpPriority() != PR_NORMAL)
        {
            SetUpPriority(PR_NORMAL);
            theApp.sharedfiles->UpdateFile(this);
        }
        return;
    }
    if (GetUpPriority() != PR_HIGH)
    {
        SetUpPriority(PR_HIGH);
        theApp.sharedfiles->UpdateFile(this);
    }*/
//<<< WiZaRd::Improved Auto Prio
}

void CKnownFile::SetUpPriority(uint8 iNewUpPriority, bool bSave)
{
    m_iUpPriority = iNewUpPriority;
    ASSERT(m_iUpPriority == PR_VERYLOW || m_iUpPriority == PR_LOW || m_iUpPriority == PR_NORMAL || m_iUpPriority == PR_HIGH || m_iUpPriority == PR_VERYHIGH);

    if (IsPartFile() && bSave)
        ((CPartFile*)this)->SavePartFile();
}

void SecToTimeLength(unsigned long ulSec, CStringA& rstrTimeLength)
{
    // this function creates the content for the "length" ed2k meta tag which was introduced by eDonkeyHybrid
    // with the data type 'string' :/  to save some bytes we do not format the duration with leading zeros
    if (ulSec >= 3600)
    {
        UINT uHours = ulSec/3600;
        UINT uMin = (ulSec - uHours*3600)/60;
        UINT uSec = ulSec - uHours*3600 - uMin*60;
        rstrTimeLength.Format("%u:%02u:%02u", uHours, uMin, uSec);
    }
    else
    {
        UINT uMin = ulSec/60;
        UINT uSec = ulSec - uMin*60;
        rstrTimeLength.Format("%u:%02u", uMin, uSec);
    }
}

void SecToTimeLength(unsigned long ulSec, CStringW& rstrTimeLength)
{
    // this function creates the content for the "length" ed2k meta tag which was introduced by eDonkeyHybrid
    // with the data type 'string' :/  to save some bytes we do not format the duration with leading zeros
    if (ulSec >= 3600)
    {
        UINT uHours = ulSec/3600;
        UINT uMin = (ulSec - uHours*3600)/60;
        UINT uSec = ulSec - uHours*3600 - uMin*60;
        rstrTimeLength.Format(L"%u:%02u:%02u", uHours, uMin, uSec);
    }
    else
    {
        UINT uMin = ulSec/60;
        UINT uSec = ulSec - uMin*60;
        rstrTimeLength.Format(L"%u:%02u", uMin, uSec);
    }
}

void CKnownFile::RemoveMetaDataTags(UINT uTagType)
{
    static const struct
    {
        uint8	nID;
        uint8	nType;
    } _aEmuleMetaTags[] =
    {
        { FT_MEDIA_ARTIST,  TAGTYPE_STRING },
        { FT_MEDIA_ALBUM,   TAGTYPE_STRING },
        { FT_MEDIA_TITLE,   TAGTYPE_STRING },
        { FT_MEDIA_LENGTH,  TAGTYPE_UINT32 },
        { FT_MEDIA_BITRATE, TAGTYPE_UINT32 },
        { FT_MEDIA_CODEC,   TAGTYPE_STRING }
    };

    // 05-J�n-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
    // the chance to clean any available meta data tags and provide only tags which were determined by us.
    // Remove all meta tags. Never ever trust the meta tags received from other clients or servers.
    for (int j = 0; j < _countof(_aEmuleMetaTags); j++)
    {
        if (uTagType == 0 || (uTagType == _aEmuleMetaTags[j].nType))
        {
            int i = 0;
            while (i < taglist.GetSize())
            {
                const CTag* pTag = taglist[i];
                if (pTag->GetNameID() == _aEmuleMetaTags[j].nID)
                {
                    delete pTag;
                    taglist.RemoveAt(i);
                }
                else
                    i++;
            }
        }
    }

    m_uMetaDataVer = 0;
}

void CKnownFile::RemoveBrokenUnicodeMetaDataTags()
{
    static const struct
    {
        uint8	nID;
        uint8	nType;
    } _aEmuleMetaTags[] =
    {
        { FT_MEDIA_ARTIST,  TAGTYPE_STRING },
        { FT_MEDIA_ALBUM,   TAGTYPE_STRING },
        { FT_MEDIA_TITLE,   TAGTYPE_STRING },
        { FT_MEDIA_CODEC,   TAGTYPE_STRING }	// This one actually contains only ASCII
    };

    for (int j = 0; j < _countof(_aEmuleMetaTags); j++)
    {
        int i = 0;
        while (i < taglist.GetSize())
        {
            // Meta data strings of older eMule versions did store Unicode strings as MBCS strings,
            // which means that - depending on the Unicode string content - particular characters
            // got lost. Unicode characters which cannot get converted into the local codepage
            // will get replaced by Windows with a '?' character. So, to estimate if we have a
            // broken Unicode string (due to the conversion between Unicode/MBCS), we search the
            // strings for '?' characters. This is not 100% perfect, as it would also give
            // false results for strings which do contain the '?' character by intention. It also
            // would give wrong results for particular characters which got mapped to ASCII chars
            // due to the conversion from Unicode->MBCS. But at least it prevents us from deleting
            // all the existing meta data strings.
            const CTag* pTag = taglist[i];
            if (pTag->GetNameID() == _aEmuleMetaTags[j].nID
                    && pTag->IsStr()
                    && _tcschr(pTag->GetStr(), _T('?')) != NULL)
            {
                delete pTag;
                taglist.RemoveAt(i);
            }
            else
                i++;
        }
    }
}

CStringA GetED2KAudioCodec(WORD wFormatTag)
{
    CStringA strCodec(GetAudioFormatCodecId(wFormatTag));
    strCodec.Trim();
    strCodec.MakeLower();
    return strCodec;
}

CStringA GetED2KVideoCodec(DWORD biCompression)
{
    if (biCompression == BI_RGB)
        return "rgb";
    else if (biCompression == BI_RLE8)
        return "rle8";
    else if (biCompression == BI_RLE4)
        return "rle4";
    else if (biCompression == BI_BITFIELDS)
        return "bitfields";
    else if (biCompression == BI_JPEG)
        return "jpeg";
    else if (biCompression == BI_PNG)
        return "png";

    LPCSTR pszCompression = (LPCSTR)&biCompression;
    for (int i = 0; i < 4; i++)
    {
        if (!__iscsym((unsigned char)pszCompression[i])
                && pszCompression[i] != '.'
                && pszCompression[i] != ' ')
            return "";
    }

    CStringA strCodec;
    memcpy(strCodec.GetBuffer(4), &biCompression, 4);
    strCodec.ReleaseBuffer(4);
    strCodec.Trim();
    if (strCodec.GetLength() < 2)
        return "";
    strCodec.MakeLower();
    return strCodec;
}

SMediaInfo *GetRIFFMediaInfo(LPCTSTR pszFullPath)
{
    bool bIsAVI;
    SMediaInfo *mi = new SMediaInfo;
    if (!GetRIFFHeaders(pszFullPath, mi, bIsAVI))
    {
        delete mi;
        return NULL;
    }
    return mi;
}

SMediaInfo *GetRMMediaInfo(LPCTSTR pszFullPath)
{
    bool bIsRM;
    SMediaInfo *mi = new SMediaInfo;
    if (!GetRMHeaders(pszFullPath, mi, bIsRM))
    {
        delete mi;
        return NULL;
    }
    return mi;
}

SMediaInfo *GetWMMediaInfo(LPCTSTR pszFullPath)
{
#ifdef HAVE_WMSDK_H
    bool bIsWM;
    SMediaInfo *mi = new SMediaInfo;
    if (!GetWMHeaders(pszFullPath, mi, bIsWM))
    {
        delete mi;
        return NULL;
    }
    return mi;
#else//HAVE_WMSDK_H
    UNREFERENCED_PARAMETER(pszFullPath);
    return NULL;
#endif//HAVE_WMSDK_H
}

// Max. string length which is used for string meta tags like TAG_MEDIA_TITLE, TAG_MEDIA_ARTIST, ...
#define	MAX_METADATA_STR_LEN	80

void TruncateED2KMetaData(CString& rstrData)
{
    rstrData.Trim();
    if (rstrData.GetLength() > MAX_METADATA_STR_LEN)
    {
        rstrData.Truncate(MAX_METADATA_STR_LEN);
        rstrData.Trim();
    }
}

void CKnownFile::UpdateMetaDataTags()
{
    // 05-J�n-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
    // the chance to clean any available meta data tags and provide only tags which were determined by us.
    RemoveMetaDataTags();

    if (thePrefs.GetExtractMetaData() == 0)
        return;

    TCHAR szExt[_MAX_EXT];
    _tsplitpath(GetFileName(), NULL, NULL, NULL, szExt);
    _tcslwr(szExt);
    if (_tcscmp(szExt, _T(".mp3"))==0 || _tcscmp(szExt, _T(".mp2"))==0 || _tcscmp(szExt, _T(".mp1"))==0 || _tcscmp(szExt, _T(".mpa"))==0)
    {
        TCHAR szFullPath[MAX_PATH];
        if (_tmakepathlimit(szFullPath, NULL, GetPath(), GetFileName(), NULL))
        {
            try
            {
                // ID3LIB BUG: If there are ID3v2 _and_ ID3v1 tags available, id3lib
                // destroys (actually corrupts) the Unicode strings from ID3v2 tags due to
                // converting Unicode to ASCII and then conversion back from ASCII to Unicode.
                // To prevent this, we force the reading of ID3v2 tags only, in case there are
                // also ID3v1 tags available.
                ID3_Tag myTag;
                CStringA strFilePathA(szFullPath);
                size_t id3Size = myTag.Link(strFilePathA, ID3TT_ID3V2);
                if (id3Size == 0)
                {
                    myTag.Clear();
                    myTag.Link(strFilePathA, ID3TT_ID3V1);
                }

                const Mp3_Headerinfo* mp3info;
                mp3info = myTag.GetMp3HeaderInfo();
                if (mp3info)
                {
                    // length
                    if (mp3info->time)
                    {
                        CTag* pTag = new CTag(FT_MEDIA_LENGTH, (UINT)mp3info->time);
                        AddTagUnique(pTag);
                        m_uMetaDataVer = META_DATA_VER;
                    }

                    // here we could also create a "codec" ed2k meta tag.. though it would probable not be worth the
                    // extra bytes which would have to be sent to the servers..

                    // bitrate
                    UINT uBitrate = (mp3info->vbr_bitrate ? mp3info->vbr_bitrate : mp3info->bitrate) / 1000;
                    if (uBitrate)
                    {
                        CTag* pTag = new CTag(FT_MEDIA_BITRATE, (UINT)uBitrate);
                        AddTagUnique(pTag);
                        m_uMetaDataVer = META_DATA_VER;
                    }
                }

                // WiZaRd: this seems to crash on id3lib 3.8.3 - here's the stack trace:
                /*kMule.exe!std::list<ID3_Frame *,std::allocator<ID3_Frame *> >::_Orphan_ptr(std::list<ID3_Frame *,std::allocator<ID3_Frame *> > & _Cont, std::_List_nod<ID3_Frame *,std::allocator<ID3_Frame *> >::_Node * _Ptr)  Zeile 1533 + 0x8 Bytes	C++
                kMule.exe!std::list<ID3_Frame *,std::allocator<ID3_Frame *> >::clear()  Zeile 1102	C++
                kMule.exe!ID3_TagImpl::Clear()  Zeile 137	C++
                kMule.exe!ID3_TagImpl::~ID3_TagImpl()  Zeile 124	C++
                kMule.exe!ID3_TagImpl::`scalar deleting destructor'()  + 0x16 Bytes	C++
                kMule.exe!ID3_Tag::~ID3_Tag()  Zeile 305 + 0x25 Bytes	C++*/
                // It's definitely caused by myTag.CreateIterator and delete iter afterwards, so either live with mem leaks or let it crash?
                ID3_Tag::Iterator* iter = myTag.CreateIterator();
                const ID3_Frame* frame;
                while ((frame = iter->GetNext()) != NULL)
                {
                    ID3_FrameID eFrameID = frame->GetID();
                    switch (eFrameID)
                    {
                        case ID3FID_LEADARTIST:
                        {
                            wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
                            CString strText(pszText);
                            TruncateED2KMetaData(strText);
                            if (!strText.IsEmpty())
                            {
                                CTag* pTag = new CTag(FT_MEDIA_ARTIST, strText);
                                AddTagUnique(pTag);
                                m_uMetaDataVer = META_DATA_VER;
                            }
                            delete[] pszText;
                            break;
                        }
                        case ID3FID_ALBUM:
                        {
                            wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
                            CString strText(pszText);
                            TruncateED2KMetaData(strText);
                            if (!strText.IsEmpty())
                            {
                                CTag* pTag = new CTag(FT_MEDIA_ALBUM, strText);
                                AddTagUnique(pTag);
                                m_uMetaDataVer = META_DATA_VER;
                            }
                            delete[] pszText;
                            break;
                        }
                        case ID3FID_TITLE:
                        {
                            wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
                            CString strText(pszText);
                            TruncateED2KMetaData(strText);
                            if (!strText.IsEmpty())
                            {
                                CTag* pTag = new CTag(FT_MEDIA_TITLE, strText);
                                AddTagUnique(pTag);
                                m_uMetaDataVer = META_DATA_VER;
                            }
                            delete[] pszText;
                            break;
                        }
                    }
                }
                delete iter; // accept mem leaks?
            }
            catch (...)
            {
                if (thePrefs.GetVerbose())
                    AddDebugLogLine(false, _T("Unhandled exception while extracting file meta (MP3) data from \"%s\""), szFullPath);
                ASSERT(0);
            }
        }
    }
    else
    {
        TCHAR szFullPath[MAX_PATH];
        if (_tmakepathlimit(szFullPath, NULL, GetPath(), GetFileName(), NULL))
        {
            SMediaInfo* mi = NULL;
            try
            {
                mi = GetRIFFMediaInfo(szFullPath);
                if (mi == NULL)
                    mi = GetRMMediaInfo(szFullPath);
                if (mi == NULL)
                    mi = GetWMMediaInfo(szFullPath);
                if (mi)
                {
                    mi->InitFileLength();
                    UINT uLengthSec = (UINT)mi->fFileLengthSec;

                    CStringA strCodec;
                    UINT uBitrate = 0;
                    if (mi->iVideoStreams)
                    {
                        strCodec = GetED2KVideoCodec(mi->video.bmiHeader.biCompression);
                        uBitrate = (mi->video.dwBitRate + 500) / 1000;
                    }
                    else if (mi->iAudioStreams)
                    {
                        strCodec = GetED2KAudioCodec(mi->audio.wFormatTag);
                        uBitrate = (DWORD)(((mi->audio.nAvgBytesPerSec * 8.0) + 500.0) / 1000.0);
                    }

                    if (uLengthSec)
                    {
                        CTag* pTag = new CTag(FT_MEDIA_LENGTH, (UINT)uLengthSec);
                        AddTagUnique(pTag);
                        m_uMetaDataVer = META_DATA_VER;
                    }

                    if (!strCodec.IsEmpty())
                    {
                        CTag* pTag = new CTag(FT_MEDIA_CODEC, CString(strCodec));
                        AddTagUnique(pTag);
                        m_uMetaDataVer = META_DATA_VER;
                    }

                    if (uBitrate)
                    {
                        CTag* pTag = new CTag(FT_MEDIA_BITRATE, (UINT)uBitrate);
                        AddTagUnique(pTag);
                        m_uMetaDataVer = META_DATA_VER;
                    }

                    TruncateED2KMetaData(mi->strTitle);
                    if (!mi->strTitle.IsEmpty())
                    {
                        CTag* pTag = new CTag(FT_MEDIA_TITLE, mi->strTitle);
                        AddTagUnique(pTag);
                        m_uMetaDataVer = META_DATA_VER;
                    }

                    TruncateED2KMetaData(mi->strAuthor);
                    if (!mi->strAuthor.IsEmpty())
                    {
                        CTag* pTag = new CTag(FT_MEDIA_ARTIST, mi->strAuthor);
                        AddTagUnique(pTag);
                        m_uMetaDataVer = META_DATA_VER;
                    }

                    TruncateED2KMetaData(mi->strAlbum);
                    if (!mi->strAlbum.IsEmpty())
                    {
                        CTag* pTag = new CTag(FT_MEDIA_ALBUM, mi->strAlbum);
                        AddTagUnique(pTag);
                        m_uMetaDataVer = META_DATA_VER;
                    }

                    delete mi;
                    return;
                }
            }
            catch (...)
            {
                if (thePrefs.GetVerbose())
                    AddDebugLogLine(false, _T("Unhandled exception while extracting file meta (AVI) data from \"%s\""), szFullPath);
                ASSERT(0);
            }
            delete mi;
        }
    }
}

bool CKnownFile::PublishNotes()
{
    if (m_lastPublishTimeKadNotes > (UINT)time(NULL))
    {
        return false;
    }
    if (GetFileComment() != L"")
    {
        m_lastPublishTimeKadNotes = (UINT)time(NULL)+KADEMLIAREPUBLISHTIMEN;
        return true;
    }
    if (GetFileRating() != 0)
    {
        m_lastPublishTimeKadNotes = (UINT)time(NULL)+KADEMLIAREPUBLISHTIMEN;
        return true;
    }

    return false;
}

bool CKnownFile::PublishSrc()
{
    UINT lastBuddyIP = 0;
    if (theApp.IsFirewalled() &&
            (Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) || !Kademlia::CUDPFirewallTester::IsVerified()))
    {
        CUpDownClient* buddy = theApp.clientlist->GetBuddy();
        if (buddy)
        {
#ifdef IPV6_SUPPORT
            lastBuddyIP = _ntohl(theApp.clientlist->GetBuddy()->GetIP().ToIPv4()); //>>> WiZaRd::IPv6 [Xanatos]
#else
            lastBuddyIP = theApp.clientlist->GetBuddy()->GetIP();
#endif
            if (lastBuddyIP != m_lastBuddyIP)
            {
                SetLastPublishTimeKadSrc((UINT)time(NULL)+KADEMLIAREPUBLISHTIMES, lastBuddyIP);
                return true;
            }
        }
        else
            return false;
    }

    if (m_lastPublishTimeKadSrc > (UINT)time(NULL))
        return false;

    SetLastPublishTimeKadSrc((UINT)time(NULL)+KADEMLIAREPUBLISHTIMES,lastBuddyIP);
    return true;
}

bool CKnownFile::IsMovie() const
{
    return (ED2KFT_VIDEO == GetED2KFileTypeID(GetFileName()));
}

//>>> MusicPreview
bool CKnownFile::IsMusic() const
{
    return (ED2KFT_AUDIO == GetED2KFileTypeID(GetFileName()));
}
//<<< MusicPreview

// function assumes that this file is shared and that any needed permission to preview exists. checks have to be done before calling!
bool CKnownFile::GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender)
{
    return GrabImage(GetPath() + CString(L"\\") + GetFileName(), nFramesToGrab,  dStartTime, bReduceColor, nMaxWidth, pSender);
}

bool CKnownFile::GrabImage(CString strFileName,uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender)
{
    if (!IsMovie())
        return false;
    CFrameGrabThread* framegrabthread = (CFrameGrabThread*) AfxBeginThread(RUNTIME_CLASS(CFrameGrabThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
    framegrabthread->SetValues(this, strFileName, nFramesToGrab, dStartTime, bReduceColor, nMaxWidth, pSender);
    framegrabthread->ResumeThread();
    return true;
}

// imgResults[i] can be NULL
void CKnownFile::GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender)
{
    // continue processing
    if (theApp.clientlist->IsValidClient((CUpDownClient*)pSender))
    {
        ((CUpDownClient*)pSender)->SendPreviewAnswer(this, imgResults, nFramesGrabbed);
    }
    else
    {
        //probably a client which got deleted while grabbing the frames for some reason
        if (thePrefs.GetVerbose())
            AddDebugLogLine(false, _T("Couldn't find Sender of FrameGrabbing Request"));
    }
    //cleanup
    for (int i = 0; i != nFramesGrabbed; i++)
        delete imgResults[i];
    delete[] imgResults;
}

CString CKnownFile::GetInfoSummary(bool bNoFormatCommands) const
{
    CString strFolder = GetPath();
    PathRemoveBackslash(strFolder.GetBuffer());
    strFolder.ReleaseBuffer();

    CString strAccepts, strRequests, strTransferred;
    strRequests.Format(_T("%u (%u)"), statistic.GetRequests(), statistic.GetAllTimeRequests());
    strAccepts.Format(_T("%u (%u)"), statistic.GetAccepts(), statistic.GetAllTimeAccepts());
    strTransferred.Format(_T("%s (%s)"), CastItoXBytes(statistic.GetTransferred(), false, false), CastItoXBytes(statistic.GetAllTimeTransferred(), false, false));
    CString strType = GetFileTypeDisplayStr();
    if (strType.IsEmpty())
        strType = _T("-");
    CString dbgInfo;
#ifdef _DEBUG
    dbgInfo.Format(_T("\nAICH Part HashSet: %s\nAICH Rec HashSet: %s"), m_FileIdentifier.HasExpectedAICHHashCount() ? GetResString(IDS_YES) : GetResString(IDS_NO)
                   , IsAICHRecoverHashSetAvailable() ? GetResString(IDS_YES) : GetResString(IDS_NO));
#endif

    CString strHeadFormatCommand = bNoFormatCommands ? L"" : _T("<br_head>");
    CString info;
    info.Format(_T("%s\n")
                + CString(_T("eD2K ")) + GetResString(IDS_FD_HASH) + _T(" %s\n")
                + GetResString(IDS_AICHHASH) + _T(": %s\n")
                + GetResString(IDS_FD_SIZE) + _T(" %s\n") + strHeadFormatCommand + _T("\n")
                + GetResString(IDS_TYPE) + _T(": %s\n")
                + GetResString(IDS_FOLDER) + _T(": %s\n\n")
                + GetResString(IDS_PRIORITY) + _T(": %s\n")
                + GetResString(IDS_SF_REQUESTS) + _T(": %s\n")
                + GetResString(IDS_SF_ACCEPTS) + _T(": %s\n")
                + GetResString(IDS_SF_TRANSFERRED) + _T(": %s%s"),
                GetFileName(),
                md4str(GetFileHash()),
                m_FileIdentifier.GetAICHHash().GetString(),
                CastItoXBytes(GetFileSize(), false, false),
                strType,
                strFolder,
                GetUpPriorityDisplayString(),
                strRequests,
                strAccepts,
                strTransferred,
                dbgInfo);
    return info;
}

CString CKnownFile::GetUpPriorityDisplayString() const
{
//>>> WiZaRd::PowerShare
    CString buffer = L"";
    switch (GetUpPriority())
    {
        case PR_VERYLOW:
            if (IsAutoUpPriority())
                buffer = GetResString(IDS_PRIOAUTOVERYLOW);
            else
                buffer = GetResString(IDS_PRIOVERYLOW);
            break;
        case PR_LOW:
            if (IsAutoUpPriority())
                buffer = GetResString(IDS_PRIOAUTOLOW);
            else
                buffer =  GetResString(IDS_PRIOLOW);
            break;
        case PR_NORMAL:
            if (IsAutoUpPriority())
                buffer = GetResString(IDS_PRIOAUTONORMAL);
            else
                buffer = GetResString(IDS_PRIONORMAL);
            break;
        case PR_HIGH:
            if (IsAutoUpPriority())
                buffer = GetResString(IDS_PRIOAUTOHIGH);
            else
                buffer = GetResString(IDS_PRIOHIGH);
            break;
        case PR_VERYHIGH:
            if (IsAutoUpPriority())
                buffer = GetResString(IDS_PRIOAUTOVERYHIGH);
            else
                buffer = GetResString(IDS_PRIORELEASE);
            break;
        default:
            buffer = L"";
            break;
    }

    if (IsPowerShared())
    {
        CString temp = buffer;
        buffer.Format(L"%s %s", GetResString(IDS_POWERSHARE_PREFIX), temp);
    }
    return buffer;
//<<< WiZaRd::PowerShare
}

bool CKnownFile::ShouldPartiallyPurgeFile() const
{
    return thePrefs.DoPartiallyPurgeOldKnownFiles() && m_timeLastSeen > 0
           && time(NULL) > m_timeLastSeen && time(NULL) - m_timeLastSeen > OLDFILES_PARTIALLYPURGE;
}

//>>> WiZaRd::PowerShare
bool CKnownFile::IsPowerShared() const
{
    if (IsPartFile())
        return false;
    return m_bPowerShared;
}

void CKnownFile::SetPowerShared(const bool b)
{
    if (m_bPowerShared == b)
        return;

    m_bPowerShared = b;
    if (theApp.uploadqueue)
        theApp.uploadqueue->ReSortUploadSlots(true);
}
//<<< WiZaRd::PowerShare
//>>> WiZaRd::Intelligent SOTN
//this function is to retrieve the "show parts" map for a specific client
//WiZaRd: this works pretty good but there is ONE big issue:
//imagine that we send our partstatus to the first client... we are using SOTN so he will see only 2 chunks (max)
//now other clients will send their requests and we will answer but as we are still the only source, SOTN will return the same scheme
//and thus we will show the same parts (again)
//this could cause that the remote clients download the same parts until parts got completed and we are reasked :(
//solution!? well... we *could* parse the queue and count any part that has already been shown in some way...
//m_SOTNAvailPartFrequency counts up how often a chunk is visible to remote users
//true: something was hidden (i.e. partial file)
//false: nothing was hidden (i.e. complete file)
bool CKnownFile::WriteSafePartStatus(CSafeMemFile* data_out, CUpDownClient* sender, const bool bUDP)
{
//>>> WiZaRd::ICS [enkeyDEV]
    // We don't use SotN for ICS enabled clients because they should always see the real file status and will select the rarest chunk by themselves
    // Unfortunately, we cannot trust that they implemented it properly, so we opt for using SOTN in that case to improve file spreading
    bool bSOTN = GetShareOnlyTheNeed() /*&& (!client || client->GetIncompletePartVersion() == 0)*/;
//<<< WiZaRd::ICS [enkeyDEV]
    const uint16 filePartCount = GetPartCount();
    CPartFile* pThis = IsPartFile() ? (CPartFile*)this : NULL;
    const bool bUpload = sender && sender->GetUploadFileID() && !md4cmp(sender->GetUploadFileID(), GetFileHash());

//>>> Create tmp array
    CList<int> shownChunks;
    if (bSOTN)
    {
        //the problem is that m_AvailPartFrequency will only check clients in our queue!
        //so create a tmparray and update its values
        CArray<uint16, uint16> tmparray;
        tmparray.SetSize(filePartCount);

        // search the least available parts
        uint16 iMinAvailablePartFrenquency = _UI16_MAX;
        uint16 iMinAvailablePartFrenquencyPrev = _UI16_MAX;
        for (uint16 i = 0; i < filePartCount; i++)
        {
            if (pThis)
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                tmparray[i] = (pThis->GetSrcPartFrequency(i) + (pThis->GetPublishedPartStatus() && pThis->GetPublishedPartStatus()->IsCompletePart(i)) ? 1 : 0);
            //tmparray[i] = (pThis->GetSrcPartFrequency(i) + (pThis->IsCompletePart(i, true) ? 1 : 0));
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
            else if (!m_AvailPartFrequency.IsEmpty())
                tmparray[i] = m_AvailPartFrequency[i];
            else
                tmparray[i] = 0;
            if (!m_SOTNAvailPartFrequency.IsEmpty())
                tmparray[i] = tmparray[i] + m_SOTNAvailPartFrequency[i];

            // a part qualifies if the remote client doesn't have it but we have
            if ((sender == NULL						//we don't have to check client
                    || (bUpload && !sender->IsUpPartAvailable(i))	//or he does not own that part
                    || (!bUpload && !sender->IsPartAvailable(i)))	//or he does not own that part
                    && (pThis == NULL				//of course it's complete if it's a complete file
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                        || (pThis->GetPublishedPartStatus() && pThis->GetPublishedPartStatus()->IsCompletePart(i))) //or the part is complete
                    //|| pThis->IsCompletePart(i, true)) //or the part is complete
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
               )
            {
                if (tmparray[i] < iMinAvailablePartFrenquency)
                    iMinAvailablePartFrenquency = tmparray[i];
                else if (tmparray[i] < iMinAvailablePartFrenquencyPrev)
                    iMinAvailablePartFrenquencyPrev = tmparray[i];
            }
        }

        /*
        // if all parts are equally rare, don't hide anything
        if (iMinAvailablePartFrenquency == _UI16_MAX)
        	bSOTN = false;
        */

        CArray<int> minFreqArray;
        CArray<int> lastminFreqArray;
        for (uint16 i = 0; i < filePartCount; i++)
        {
            if (tmparray[i] == iMinAvailablePartFrenquency)
                minFreqArray.Add(i);
            else if (tmparray[i] == iMinAvailablePartFrenquencyPrev)
                lastminFreqArray.Add(i);
        }
        const UINT targetValues = (minFreqArray.GetCount() + lastminFreqArray.GetCount()) < 3 ? (minFreqArray.GetCount() + lastminFreqArray.GetCount()) : 3;
        int index = 0;
        int val = 0;
        while ((UINT)shownChunks.GetCount() < targetValues && !minFreqArray.IsEmpty())
        {
            index = rand() % minFreqArray.GetCount();
            val = minFreqArray[index];

            shownChunks.AddTail(val);
            minFreqArray.RemoveAt(index);
        }
        while ((UINT)shownChunks.GetCount() < targetValues && !lastminFreqArray.IsEmpty())
        {
            index = rand() % lastminFreqArray.GetCount();
            val = lastminFreqArray[index];

            shownChunks.AddTail(val);
            lastminFreqArray.RemoveAt(index);
        }
    }
//<<< Create tmp array

    uint16	sctCount = GetED2KPartCount();
    uint64	partSize = PARTSIZE;
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    UINT sctDivider = 1;
    if (pThis)
    {
        // parts taken from CPartStatus::WritePartStatus
        // Decide chunk size for part vector
        if (pThis->GetPublishedPartStatus() == NULL)
            DebugLogError(L"%hs: No part data available (part file not initialized!?)", __FUNCTION__);
        else if (sender && sender->SupportsSCT())
        {
            uint64	size = GetFileSize();
            // Decide chunk size for part vector (limit to 8000 sub chunks, to keep part vector size less than 1kB?)
            for (UINT i = 0; i < filePartCount; ++i)
            {
                if (pThis->GetPublishedPartStatus()->IsPartialPart(i))
                {
#ifdef _DEBUG
                    //theApp.QueueDebugLogLineEx(LOG_WARNING, L"Part %u is partial - SCT file status is possible (%s - %s)", i, sender ? sender->DbgGetClientInfo() : L"", GetFileName());
#endif
                    // Only send sub chunks if there are incomplete parts shared
                    if (sender->GetSCTVersion() >= PROTOCOL_REVISION_2)
                    {
                        if (size <= (8000 * EMBLOCKSIZE) && pThis->GetPublishedPartStatus()->GetChunkSize() < PARTSIZE)
                            partSize = EMBLOCKSIZE;
                        sctDivider = (uint16)((PARTSIZE + partSize - 1) / partSize); // TODO: floor() needed?
                    }
                    else if (size <= (8000 * CRUMBSIZE) && pThis->GetPublishedPartStatus()->GetChunkSize() < PARTSIZE)
                    {
                        partSize = CRUMBSIZE;
                        sctDivider = CRUMBSPERPART;
                    }
                    break;
                }
            }

            sctCount = (uint16)(sctDivider * (size / PARTSIZE) + ((size % PARTSIZE) + partSize - 1) / partSize);
        }
    }
#ifdef _DEBUG
    //theApp.QueueDebugLogLineEx(LOG_WARNING, L"Sending part status: partcount: %u, SCT v%u, partsize: %I64u, sctDevider: %u, sctCount: %u (%s - %s)", filePartCount, sender ? sender->GetSCTVersion() : 0, partSize, sctDivider, sctCount, sender ? sender->DbgGetClientInfo() : L"", GetFileName());
#endif
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

    bool bNeedThisFunction = pThis != NULL; // we need it for part files in any case!
    CList<uint8> partInfo;
    //if the data is sent via UDP it is not guaranteed to arrive... so what to do!?
    const uint8 toSet = bUDP ? 2 : 1;
//>>> WiZaRd::FiX
    //check if we sent the status for the same file as we requested to download off that client
    //if NOT then we can't write the data because the partcount may differ, etc.
    //The best way to "fix" that would be to save all partstatus' we receive
    const bool bWriteStatus = bUpload && sender->m_abyUpPartStatusHidden;
//<<< WiZaRd::FiX

    //and now write it down...
    uint16 done = 0;
    UINT lastPart = 0;
    UINT cur_part = 0;
    // only mark COMPLETE parts as hidden
    bool bPartComplete = pThis == NULL || (pThis->GetPublishedPartStatus() && pThis->GetPublishedPartStatus()->IsCompletePart(cur_part));
    bool bPartCompletelyShown = bPartComplete;
    while (done != sctCount)
    {
        //fill one uint8 with data...
        uint8 towrite = 0;
        for (uint8 i = 0; i < 8; ++i)
        {
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
            cur_part = done / sctDivider;
            const UINT subChunk = done % sctDivider;
            //cur_part = done;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

            if (cur_part < filePartCount) // valid part?
            {
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                // is this part/crumb available for sharing?
                //if((pThis == NULL || (pThis->IsCompletePart(curPart))
                if ((pThis == NULL || (pThis->GetPublishedPartStatus() && pThis->GetPublishedPartStatus()->IsComplete((uint64)cur_part*PARTSIZE + subChunk*partSize, (uint64)cur_part*PARTSIZE + (subChunk+1)*partSize - 1)))
                        && (
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                            (!bSOTN	|| shownChunks.Find(cur_part))				// and SOTN is OFF or SOTN allows this chunk (one of the 3 rarest chunks)
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                            || (sender && ((bUpload && sender->IsUpPartAvailable(cur_part) || (!bUpload && sender->IsPartAvailable(cur_part)))))	// or the remote client also has this chunk
                            || !bPartComplete									// or the part is not shown completely (i.e. it's a crumb - always show those!)
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
                        )
                   )
                    towrite |= (1<<i);
                else
                {
                    bNeedThisFunction = true;						// SOTN indicator
                    bPartCompletelyShown = false;
                }
            }
            ++done;

            // update the hidden status on part change
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
            cur_part = done / sctDivider;
            //cur_part = done;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
            if (lastPart != cur_part)
            {
                if (bWriteStatus && lastPart < filePartCount) // valid part
                {
                    if (!bPartCompletelyShown && bPartComplete) // don't flag partial parts as "hidden"
                        sender->m_abyUpPartStatusHidden[lastPart] = toSet;
                    else
                        sender->m_abyUpPartStatusHidden[lastPart] = 0;  //not hidden anymore

                }
                lastPart = cur_part;
                bPartComplete = cur_part < filePartCount // valid part
                                && (pThis == NULL || (pThis->GetPublishedPartStatus() && pThis->GetPublishedPartStatus()->IsCompletePart(cur_part)));
                bPartCompletelyShown = bPartComplete;
            }
            if (done == sctCount)
                break;
        }
        partInfo.AddTail(towrite);
    }

    // write down the part info if we hid something...
    if (bNeedThisFunction)
    {
        data_out->WriteUInt16(sctCount);
        while (!partInfo.IsEmpty())
            data_out->WriteUInt8(partInfo.RemoveHead());
    }
    // otherwise, show the complete file
    else
        data_out->WriteUInt16(0);
    return bNeedThisFunction;
}

bool CKnownFile::GetShareOnlyTheNeed(const bool m_bOnlyFile) const
{
    if (IsPartFile())
        return false;
    if (m_bOnlyFile)
        return m_bShareOnlyTheNeed;
    switch (thePrefs.IsShareOnlyTheNeed())
    {
        case 0:		return false;
        case 1:		return true;
        default:	return m_bShareOnlyTheNeed;
    }
    //return true;
}
//<<< WiZaRd::Intelligent SOTN

bool CKnownFile::IsSharedInKad() const
{
    bool bSharedInKad = false;
    if ((UINT)time(NULL) < GetLastPublishTimeKadSrc())
    {
        if (theApp.IsFirewalled() && theApp.IsConnected())
        {
#ifdef IPV6_SUPPORT
            if ((theApp.clientlist->GetBuddy() && (GetLastPublishBuddy() == _ntohl(theApp.clientlist->GetBuddy()->GetIP().ToIPv4()))) //>>> WiZaRd::IPv6 [Xanatos]
#else
            if ((theApp.clientlist->GetBuddy() && (GetLastPublishBuddy() == theApp.clientlist->GetBuddy()->GetIP()))
#endif
                    || (Kademlia::CKademlia::IsRunning() && !Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) && Kademlia::CUDPFirewallTester::IsVerified()))
            {
                bSharedInKad = true;
            }
        }
        else
            bSharedInKad = true;
    }
    return bSharedInKad;
}

//>>> FDC [BlueSonicBoy]
wchar_t GetCleanChar(wchar_t tc)
{
//Start - convert accents
    if (tc > 0xdf && tc < 0xe6)
        tc = L'a';
    else if (tc == 0x0e)
        tc = L'c';
    else if (tc > 0xe7 && tc < 0xec)
        tc = L'e';
    else if (tc > 0xeb && tc < 0xf0)
        tc = L'i';
    else if (tc == 0xf1)
        tc = L'n';
    else if (tc > 0xf1 && tc < 0xf7)
        tc = L'o';
    else if (tc > 0xf8 && tc < 0xfd)
        tc = L'u';
    else if (tc > 0xfc && tc < 0x100)
        tc = L'y';
    else
//End - convert accents
    {
        if ((tc < L'a' || tc > L'z') //not English
                && (tc < 0x5d0 || tc > 0x5ea) //not Hebrew
                && (tc < 0x900 || tc > 0xAff) //not Indic (Devanagari(hindi),Bengali,Gujarati,Gurmukhi)
                && (tc < 0x3105 || tc > 0x312c) //not Chinese
                && (tc < 0xe00 || tc > 0xe3A) //not Thai
                && (tc < 0xe3f || tc > 0x65b)
                && (tc < 0x3ac || tc > 0x3ce) //not Greek
                && (tc < 0x1f00 || tc > 0x1ff7)
                && (tc < 0x3041 || tc > 0x30fa) //not Japanese
                && (tc < 0x31f0 || tc > 0x31ff)
                && (tc < 0x621 || tc > 0x63a) //not Arabic
                && (tc < 0x640 || tc > 0x652)
                && (tc < 0x671 || tc > 0x6ba)
                && (tc < 0x430 || tc > 0x44F) //not Cyrillic
                && (tc < 0x451 || tc > 0x45c)
                && (tc != 0x45e && tc != 0x45f)
                && (tc != 0x500 && tc != 0x50f)
                && (tc < 0x3300 || tc > 0x4dbf) //no CJK Unified Ideographs
                && (tc < 0x4e00 || tc > 0x9fbf)
                && (tc < 0xf900 || tc > 0xfaff))
            tc = L' ';	//none of these character sets? then make it a space!
    }
    return tc;
}

void CKnownFile::CheckFilename(CString FileName)
{
    //get our filename
    CString OurFilename(GetFileName());
    //Make both filenames lower case
    OurFilename.MakeLower();
    FileName.MakeLower();

    //Try to remove links (http/www) from the name to compare to reduce false positives
#define URL_END		L" )}]><[{("
    const CString strHTTPPatterns[] =
    {
        L"http---",
        L"http!``",
        L"http___"
    };

    int foundAt = -1;
    int fLen = 0;
    int iLen = 0;
    for (int i = 0; i != _countof(strHTTPPatterns); ++i)
    {
        foundAt = FileName.Find(strHTTPPatterns[i]);
        while (foundAt != -1)
        {
            fLen = FileName.GetLength();
            iLen = FileName.Right(fLen-foundAt).FindOneOf(URL_END);
            if (iLen != -1) //if we have found an end of link the remove the link!
                FileName = FileName.Left(foundAt) + FileName.Right(fLen - (foundAt + iLen));
            else
                break;
            foundAt = FileName.Find(strHTTPPatterns[i]);
        }
    }

    //remove web links!
    foundAt = FileName.Find(L"www.");
    while (foundAt != -1)
    {
        fLen = FileName.GetLength();
        iLen = FileName.Right(fLen-foundAt).FindOneOf(URL_END);
        if (iLen != -1) //if we have found an end of link then remove the link!
            FileName = FileName.Left(foundAt) + FileName.Right(fLen - (foundAt + iLen));
        else
        {
            iLen = FileName.Right(fLen-foundAt).Find(L".co");//gets .com and .co.xx
            if (iLen != -1)
            {
                iLen += 3;
                FileName = FileName.Left(foundAt) + FileName.Right(fLen - (foundAt + iLen));
            }
            else
            {
                iLen = FileName.Right(fLen-foundAt).Find(L".net");
                if (iLen == -1)
                    iLen = FileName.Right(fLen-foundAt).Find(L".org");
                if (iLen != -1)
                {
                    iLen += 4;
                    FileName = FileName.Left(foundAt) + FileName.Right(fLen - (foundAt + iLen));
                }
                else  //no common domain postfix remove just the first word in the domain name
                {
                    foundAt += 4;//skip the '.' in www. and find the next
                    int iLen = FileName.Right(fLen-(foundAt)).Find(L".");
                    if (iLen != -1)
                    {
                        iLen++;
                        FileName = FileName.Left(foundAt - 4) + FileName.Right(fLen - (foundAt + iLen));
                    }
                }
            }
        }
        foundAt = FileName.Find(L"www."); //there COULD be more than one...
    }

    //clean it
    CString CleanName = L"";
    int len = OurFilename.GetLength();
    wchar_t lastadded = L' ';
    for (int i = 0; i < len; i++)
    {
        wchar_t tc = GetCleanChar(OurFilename.GetAt(i));
        if (tc != L' ' || lastadded != L' ')
        {
            CleanName.AppendChar(tc);
            lastadded = tc;
        }
    }

    //the source's filename's length
    len = FileName.GetLength();
    //nothing to compare?
    if (CleanName.GetLength() < 4 || len < 4)
        return;

    //Tweaks to reduce the chance of false positives.
    //if our filename is say x.htm and the source's filename is x.html,(the long four letter version) don't count the different extention
    bool isnotlongext = true;
    if ((OurFilename.Right(4) == L".htm" && FileName.Right(5) == L".html") ||
            (OurFilename.Right(4) == L".mpg" && FileName.Right(5) == L".mpeg") ||
            (OurFilename.Right(4) == L".jpg" && FileName.Right(5) == L".jpeg"))
        isnotlongext = false;

    //if *both* of our files are same type of video files don't count codec info 'divx' as a discriptive word
    bool bothnotvideo = true;
    uint16 nontrivialword = 0;
    if (FileName.Right(4) == OurFilename.Right(4))
    {
        if (OurFilename.Right(4) == L".avi" || OurFilename.Right(4) == L".wmv")
            bothnotvideo = false;
    }
    else
    {
        //assess extensions if they are not the same and are both three character extensions set a one word mismatch
        if (OurFilename.GetAt(OurFilename.GetLength() - 4)== L'.' && FileName.GetAt(FileName.GetLength() - 4) == L'.')
            nontrivialword = 1;
    }

    uint16 count = 0;
    uint16 spaces = 0;
    uint16 matched = 0;
    CString newword = L"";
    //clean source's filename and search it for matches with ours
    for (int i = 0; i< len ; i++)
    {
        wchar_t tc = GetCleanChar(FileName.GetAt(i));
        //allow only one dividing character between words, a space.
        if (tc != L' ' || lastadded != L' ')
        {
            lastadded = tc; //to prevent multiple separating characters
            if (tc == L' ' || (count>0 && newword[0]>0x3040)) //evaluate oriental ideograms one character at a time
            {
                //end of word. do comparison
                spaces++; //we are using spaces to count word separation so individual ideograms have intrinsic separation...
                if (newword[0]>0x3040 || (count > 3 && (isnotlongext || (newword != L"html" || newword != L"mpeg" || newword != L"jpeg"))
                                          && (bothnotvideo || (newword != L"divx" || newword != L"xvid"))))
                {
                    //word is less likely to be a conjunction and is not various common additions OR work could be chinese or japanese
                    nontrivialword++;
                    //check for matches
                    if (CleanName.Find(newword) != -1)
                        matched++;
                }
                if (tc != L' ')
                    i--; //Chinese Japanese Korean compare each character
                count = 0;
                newword.Empty();
            }
            else //add to word and update character count
            {
                newword.AppendChar(tc);
                count++;
            }
        }
    }
    //new for version 1.5a a bad name could slip through but on balance it will reduce false positives
    if (spaces < 2 && nontrivialword == 1 && FileName.GetLength() < OurFilename.GetLength())
        return; //protect against a name like "mozillafirefox zip" compared to "mozilla firefox version zip"

    //compare non-matched words to 70% to 94% words, default %83 eg: a minimum 17% match.
    if ((uint16)(nontrivialword - matched) > (uint16)((float)nontrivialword/(float)(100.0F/((float)FDC_SENSITIVITY))))
        ++m_iNameContinuityBad; //set warning flag for this file
    else
        --m_iNameContinuityBad; //proper name for this file, increase trust
}

bool CKnownFile::DissimilarName() const
{
    // wait for at least 2 "bad" hits
    return m_iNameContinuityBad > 1;
}

void CKnownFile::ResetFDC()
{
    m_iNameContinuityBad = 0;
}
//<<< FDC [BlueSonicBoy]
//>>> WiZaRd::Ratio Indicator
double CKnownFile::GetSharingRatio() const
{
    uint64 downloaded = 0;
    if (IsPartFile())
        downloaded = ((const CPartFile*)this)->GetCompletedSize();
    else
        downloaded = GetFileSize();
    uint64 uploaded = statistic.GetAllTimeTransferred();

    return GetRatioDouble(uploaded, downloaded);
}
//<<< WiZaRd::Ratio Indicator
//>>> WiZaRd::Upload Feedback
CString CKnownFile::GetFeedBackString() const
{
    CString feed = L"";
    CString tmp = L"";

    //Mod-Info
    feed.AppendFormat(L"%s: %s@%s\r\n",
                      GetResString(IDS_FEEDBACKBY), thePrefs.GetUserNick(), MOD_VERSION);

    //Time
    CTime time = CTime::GetCurrentTime();
    feed.AppendFormat(L"%s: %s\r\n",
                      GetResString(IDS_FEEDBACKTIME), time.Format(thePrefs.GetDateTimeFormat()));

    //Filename
    feed.AppendFormat(L"%s: %s\r\n",
                      GetResString(IDS_DL_FILENAME), GetFileName());

    //Filetype
    feed.AppendFormat(L"%s: %s\r\n",
                      GetResString(IDS_TYPE), GetFileTypeDisplayStr());

    //Filesize
    feed.AppendFormat(L"%s: %s\r\n",
                      GetResString(IDS_DL_SIZE), CastItoXBytes(GetFileSize()));

    //DL progress/speed
    CPartFile* pfile = NULL;
    if (IsPartFile())
        pfile = (CPartFile*)this;
    if (pfile)
    {
        feed.AppendFormat(L"%s: %.2f%% (=%s)\r\n",
                          GetResString(IDS_DL_TRANSFCOMPL), pfile->GetPercentCompleted(), CastItoXBytes(pfile->GetCompletedSize()));
        tmp.Format(L"%s: %s\r\n",
                   GetResString(IDS_DL_SPEED), GetResString(IDS_CURRENT_TRANSFER_DL));
        feed.AppendFormat(tmp, pfile->GetTransferringSrcCount(), CastItoXBytes(pfile->GetDatarate(), false, true));
    }
    else
        feed.AppendFormat(L"%s: 100%%\r\n",
                          GetResString(IDS_DL_TRANSFCOMPL));

    //UL progress/speed
    UINT speed = 0;
    UINT counter = 0;
    for (POSITION pos = theApp.uploadqueue->uploadinglist.GetHeadPosition(); pos;)
    {
        CUpDownClient* client = theApp.uploadqueue->uploadinglist.GetNext(pos);
        if (md4cmp(client->GetUploadFileID(), GetFileHash()) == 0)
        {
            ++counter;
            speed += client->GetDatarate();
        }
    }
    if (counter > 0)
    {
        CString tmp;
        tmp.Format(L"%s: %s\r\n",
                   GetResString(IDS_ST_UPLOAD), GetResString(IDS_CURRENT_TRANSFER_UL));
        feed.AppendFormat(tmp, counter, CastItoXBytes(speed, false, true));
    }

    //Filepriority
    feed.AppendFormat(L"%s: %s\r\n",
                      GetResString(IDS_PRIORITY), GetUpPriorityDisplayString());

    //Uploaded data
    feed.AppendFormat(L"%s: %s/%s\r\n",
                      GetResString(IDS_SF_TRANSFERRED), CastItoXBytes(statistic.GetTransferred()), CastItoXBytes(statistic.GetAllTimeTransferred()));

    //Srccount
    uint16 uiCompleteSourcesCountLo = m_nCompleteSourcesCountLo;
    uint16 uiCompleteSourcesCountHi = m_nCompleteSourcesCountHi;
    if (pfile)
    {
        uiCompleteSourcesCountLo = pfile->m_nCompleteSourcesCountLo;
        uiCompleteSourcesCountHi = pfile->m_nCompleteSourcesCountHi;
    }

    if (uiCompleteSourcesCountLo == uiCompleteSourcesCountHi)
        feed.AppendFormat(L"%s: %u\r\n",
                          GetResString(IDS_COMPLSOURCES), uiCompleteSourcesCountLo);
    else if (uiCompleteSourcesCountLo == 0)
        feed.AppendFormat(L"%s: < %u\r\n",
                          GetResString(IDS_COMPLSOURCES), uiCompleteSourcesCountHi+1);
    else
        feed.AppendFormat(L"%s: %u-%u\r\n",
                          GetResString(IDS_COMPLSOURCES), uiCompleteSourcesCountLo, uiCompleteSourcesCountHi);

    //Waiting
    const UINT queued = GetQueuedCount();
    if (queued)
        feed.AppendFormat(L"%s: %u\r\n",
                          GetResString(IDS_ONQUEUE), queued);

    //Requests
    feed.AppendFormat(L"%s: %u/%u\r\n",
                      GetResString(IDS_SF_REQUESTS), statistic.GetRequests(), statistic.GetAllTimeRequests());

    //Accepts
    feed.AppendFormat(L"%s: %u/%u\r\n",
                      GetResString(IDS_SF_ACCEPTS), statistic.GetAccepts(), statistic.GetAllTimeAccepts());

    return feed;
}
//<<< WiZaRd::Upload Feedback
//>>> WiZaRd::Queued Count
void	CKnownFile::IncRealQueuedCount(CUpDownClient* /*client*/)
{
    if (m_uiQueuedCount == 0)
        theApp.sharedfiles->IncQueuedFilesCount();
    ++m_uiQueuedCount;
    UpdateAutoUpPriority(); //>>> WiZaRd::Improved Auto Prio
}

void	CKnownFile::DecRealQueuedCount(CUpDownClient* /*client*/)
{
    if (m_uiQueuedCount > 0)
        --m_uiQueuedCount;
    else
        ASSERT(0);
    if (m_uiQueuedCount == 0)
        theApp.sharedfiles->DecQueuedFilesCount();
    UpdateAutoUpPriority(); //>>> WiZaRd::Improved Auto Prio
}
//<<< WiZaRd::Queued Count
//>>> WiZaRd::FileHealth
float CKnownFile::GetFileHealth()
{
    if (::GetTickCount() - m_dwLastFileHealthCalc >= SEC2MS(15))
    {
        m_dwLastFileHealthCalc = ::GetTickCount();

        //just to be sure... clear variables before proceeding
        m_fLastHealthValue = 0;

        //now, here, we calculate the average availability of each chunk
        const uint16 parts = GetPartCount();
        UINT completesources = 1;
        UINT counter = 0;
        float sum = 0;

        //if we do not have the file complete
        CPartFile* pFile = NULL;
        if (IsPartFile())
            pFile = (CPartFile*)this;

        CList<const CUpDownClient*> list;
        if (pFile)
        {
            completesources = 0; //... remove us as "complete source"
            sum += pFile->GetPercentCompleted(); //... and add us as a partial source
            ++counter;

            for (POSITION pos = pFile->srclist.GetHeadPosition(); pos;)
            {
                const CUpDownClient* client = pFile->srclist.GetNext(pos);
                const CPartStatus* status = client->GetPartStatus();
                if (!status /*|| client->GetPartCount() != parts*/)
                    continue;

                list.AddTail(client);
                uint16 tmpcount = 0;
                for (uint16 i = 0; i < parts; ++i)
                {
                    if (status->IsCompletePart(i))
                        ++tmpcount;
                }

                if (tmpcount == parts)
                    ++completesources;
                else
                {
                    ++counter;
                    sum += (float)((double)parts != 0.0 ? (double)tmpcount/(double)parts*100.0 : 0.0);
                }
            }
//>>> Keep A4AF infos
            /*for (POSITION pos = pFile->A4AFsrclist.GetHeadPosition(); pos;)
            {
            	const CUpDownClient* client = pFile->A4AFsrclist.GetNext(pos);
            	const CPartStatus* status = client->GetPartStatus(pFile);
            	if(!status)
            		continue;

            	list.AddTail(client);
            	uint16 tmpcount = 0;
            	for(uint16 i = 0; i < parts; ++i)
            	{
            		if(status->IsCompletePart(i))
            			++tmpcount;
            	}

            	if(tmpcount == parts)
            		++completesources;
            	else
            	{
            		++counter;
            		sum += (float)((double)parts != 0.0 ? (double)tmpcount/(double)parts*100.0 : 0.0);
            	}
            }*/
//<<< Keep A4AF infos
        }

        for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos;)
        {
            const CUpDownClient* client = m_ClientUploadList.GetNext(pos);
            const CPartStatus* status = client->GetUpPartStatus();
            if (status == NULL /*|| client->GetUpPartCount() != parts*/)
                continue;
            if (list.Find(client))
                continue;

            uint16 tmpcount = 0;
            for (uint16 i = 0; i < parts; ++i)
            {
                if (status->IsCompletePart(i))
                    ++tmpcount;
            }

            if (tmpcount == parts)
                ++completesources;
            else
            {
                ++counter;
                sum += (float)((double)parts != 0.0 ? (double)tmpcount/(double)parts*100.0 : 0.0);
            }
        }

        //we have at least X complete sources
        completesources = max(completesources, m_nCompleteSourcesCount);
        m_fLastHealthValue = 100.0f*completesources; // base

        //add average completion of each chunk
        if (counter)
            m_fLastHealthValue += sum/counter;
    }
    return m_fLastHealthValue;
}
//<<< WiZaRd::FileHealth

void CKnownFile::UpdateCompleteSrcCount()
{
    const UINT partcount = GetPartCount();

    m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;

    // retrieve users complete src count values
    CArray<uint16, uint16> count;
    CPartFile* pFile = IsPartFile() ? (CPartFile*)this : NULL;
    GetCompleteSrcCount(count);

    if ((UINT)m_AvailPartFrequency.GetSize() < partcount)
    {
        m_AvailPartFrequency.SetSize(partcount);
        for (UINT i = 0; i < partcount; ++i)
            m_AvailPartFrequency[i] = 0;
    }

    CArray<uint16, uint16> availPartFrequency;
    availPartFrequency.SetSize(partcount);
    for (UINT i = 0; i < partcount; ++i)
    {
        availPartFrequency[i] = m_AvailPartFrequency[i];
        if (pFile) // we may have complete src on partfiles that won't request the file, so we adjust the src count to the max of both
            availPartFrequency[i] = max(pFile->GetSrcPartFrequency(i), availPartFrequency[i]);
        if (i == 0)
            m_nCompleteSourcesCount = availPartFrequency[i];
        else if (m_nCompleteSourcesCount > availPartFrequency[i])
            m_nCompleteSourcesCount = availPartFrequency[i];
    }
    if (m_nCompleteSourcesCount == 0 && pFile == NULL)
        m_nCompleteSourcesCount = 1; // we have the file complete, too
    count.Add(m_nCompleteSourcesCount);

    const int n = count.GetSize();
    /*if(bPartFile && n < 5)
    {
    	//Not many sources, so just use what you see..
    	m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
    	m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
    }
    else*/ if (n > 0)
    {
//>>> WiZaRd::Complete Files As Median
        CMedian<uint16> median;
        for (int i = 0; i < n; ++i)
            median.AddVal(count[i]);
        median.SortVals(); // sorts the internal vector
        std::vector<uint16> medianVector = median.GetMedianVector();
        std::vector<uint16> median1;
        std::vector<uint16> median2;
        const int half = (n / 2);
        for (int i = 0; i < half; ++i)
        {
            TRACE(L"Median Vector %i: %i\n", i, medianVector[i]);
            median1.push_back(medianVector[i]);
        }
        for (int i = half; i < n; ++i)
        {
            TRACE(L"Median Vector %i: %i\n", i, medianVector[i]);
            median2.push_back(medianVector[i]);
        }
        median.SetMedianVector(median1);
        median1 = median.GetMedianVector();
        for (size_t i = 0; i < median1.size(); ++i)
            TRACE(L"Low Vector %i: %i\n", i, median1[i]);
        m_nCompleteSourcesCountLo = median.GetMedian();
        TRACE(L"Low Median: %u\n", m_nCompleteSourcesCountLo);

        median.SetMedianVector(median2);
        median2 = median.GetMedianVector();
        for (size_t i = 0; i < median2.size(); ++i)
            TRACE(L"High Vector %i: %i\n", i, median2[i]);
        m_nCompleteSourcesCountHi = median.GetMedian();
        TRACE(L"High Median: %u\n", m_nCompleteSourcesCountHi);

        if (m_nCompleteSourcesCountLo < m_nCompleteSourcesCount)
            m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
        if (m_nCompleteSourcesCountHi < m_nCompleteSourcesCount)
            m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
        if (m_nCompleteSourcesCount == 0)
            m_nCompleteSourcesCount = (m_nCompleteSourcesCountLo + m_nCompleteSourcesCountHi) / 2;
//<<< WiZaRd::Complete Files As Median
    }
}

void CKnownFile::GetCompleteSrcCount(CArray<uint16, uint16>& count)
{
    for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0;)
    {
        CUpDownClient* cur_src = m_ClientUploadList.GetNext(pos);
        if (cur_src->GetUpCompleteSourcesCount() != _UI16_MAX) //>>> WiZaRd::Use client info only if sent!
            count.Add(cur_src->GetUpCompleteSourcesCount());
    }
}

void CKnownFile::ProcessFile()
{
    if (m_bCompleteSrcUpdateNecessary)
    {
        m_bCompleteSrcUpdateNecessary = false;
        UpdateCompleteSrcCount();
    }
}