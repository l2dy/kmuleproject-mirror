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
#include "emule.h"
#include <zlib/zlib.h>
#include "UpDownClient.h"
#include "UrlClient.h"
#include "Opcodes.h"
#include "Packets.h"
#include "UploadQueue.h"
#include "Statistics.h"
#include "ClientList.h"
#include "ClientUDPSocket.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "PartFile.h"
#include "ClientCredits.h"
#include "ListenSocket.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "DownloadQueue.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "Log.h"
#include "./Mod/ClientAnalyzer.h" //>>> WiZaRd::ClientAnalyzer
#include "./Mod/NetF/PartStatus.h" //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//	members of CUpDownClient
//	which are mainly used for uploading functions

CBarShader CUpDownClient::s_UpStatusBar(16);

void CUpDownClient::DrawUpStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat) const
{
    COLORREF crNeither;
    COLORREF crNextSending;
    COLORREF crBoth;
    COLORREF crSending;

    if (GetSlotNumber() <= theApp.uploadqueue->GetActiveUploadsCount() ||
            (GetUploadState() != US_UPLOADING && GetUploadState() != US_CONNECTING))
    {
        crNeither = RGB(224, 224, 224);
        crNextSending = RGB(255,208,0);
        crBoth = bFlat ? RGB(0, 0, 0) : RGB(104, 104, 104);
        crSending = RGB(0, 150, 0);
    }
    else
    {
        // grayed out
        crNeither = RGB(248, 248, 248);
        crNextSending = RGB(255,244,191);
        crBoth = bFlat ? RGB(191, 191, 191) : RGB(191, 191, 191);
        crSending = RGB(191, 229, 191);
    }

    // wistily: UpStatusFix
    CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
	if (currequpfile == NULL && SupportsSCT())
		currequpfile = theApp.downloadqueue->GetFileByID(requpfileid);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    EMFileSize filesize;
    if (currequpfile)
        filesize = currequpfile->GetFileSize();
    else
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
		if(m_pUpPartStatus)
			filesize = (uint64)(CRUMBSIZE * (uint64)m_pUpPartStatus->GetCrumbsCount());
		else
			filesize = (uint64)(PARTSIZE * (uint64)GetUpPartCount());
        //filesize = (uint64)(PARTSIZE * (uint64)m_nUpPartCount);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    // wistily: UpStatusFix

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
	const CPartStatus* m_abyUpPartStatus = GetUpPartStatus();
	const UINT m_nUpPartCount = GetUpPartCount();
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (filesize > (uint64)0)
    {
        s_UpStatusBar.SetFileSize(filesize);
        s_UpStatusBar.SetHeight(rect->bottom - rect->top);
        s_UpStatusBar.SetWidth(rect->right - rect->left);
        s_UpStatusBar.Fill(crNeither);
        if (!onlygreyrect && m_abyUpPartStatus)
        {
            for (UINT i = 0; i < m_nUpPartCount; i++)
                if (IsUpPartAvailable(i))
                    s_UpStatusBar.FillRange(PARTSIZE*(uint64)(i), PARTSIZE*(uint64)(i+1), crBoth);
//>>> WiZaRd::Intelligent SOTN
                else if (m_abyUpPartStatusHidden)
                {
                    if (m_abyUpPartStatusHidden[i] == 1) //hidden via TCP
                        s_UpStatusBar.FillRange(PARTSIZE*(uint64)(i),PARTSIZE*(uint64)(i+1), RGB(0, 192, 192));
                    else if (m_abyUpPartStatusHidden[i] == 2) //hidden via UDP
                        s_UpStatusBar.FillRange(PARTSIZE*(uint64)(i),PARTSIZE*(uint64)(i+1), RGB(0, 100, 100));
                }
//<<< WiZaRd::Intelligent SOTN
        }
        const Requested_Block_Struct* block;
        if (!m_BlockRequests_queue.IsEmpty())
        {
            block = m_BlockRequests_queue.GetHead();
            if (block)
            {
                UINT start = (UINT)(block->StartOffset/PARTSIZE);
                s_UpStatusBar.FillRange((uint64)start*PARTSIZE, (uint64)(start+1)*PARTSIZE, crNextSending);
            }
        }
        if (!m_DoneBlocks_list.IsEmpty())
        {
            block = m_DoneBlocks_list.GetHead();
            if (block)
            {
                UINT start = (UINT)(block->StartOffset/PARTSIZE);
                s_UpStatusBar.FillRange((uint64)start*PARTSIZE, (uint64)(start+1)*PARTSIZE, crNextSending);
            }
        }
        if (!m_DoneBlocks_list.IsEmpty())
        {
            for (POSITION pos=m_DoneBlocks_list.GetHeadPosition(); pos!=0;)
            {
                block = m_DoneBlocks_list.GetNext(pos);
                s_UpStatusBar.FillRange(block->StartOffset, block->EndOffset + 1, crSending);
            }
        }
        s_UpStatusBar.Draw(dc, rect->left, rect->top, bFlat);
    }
}

void CUpDownClient::SetUploadState(EUploadState eNewState)
{
    if (eNewState != m_nUploadState)
    {
        if (m_nUploadState == US_UPLOADING)
        {
            // Reset upload data rate computation
            m_nUpDatarate = 0;
            m_nSumForAvgUpDataRate = 0;
            m_AvarageUDR_list.RemoveAll();
        }
        if (eNewState == US_UPLOADING)
            m_fSentOutOfPartReqs = 0;

        // don't add any final cleanups for US_NONE here
        m_nUploadState = (_EUploadState)eNewState;
        theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(this);
    }
}

/**
 * Gets the queue score multiplier for this client, taking into consideration client's credits
 * and the requested file's priority.
 */
float CUpDownClient::GetCombinedFilePrioAndCredit()
{
    if (credits == 0)
    {
        ASSERT(IsKindOf(RUNTIME_CLASS(CUrlClient)));
        return 0.0F;
    }

//>>> WiZaRd::ClientAnalyzer
    if (pAntiLeechData)
        return 10.0f * pAntiLeechData->GetScore() * (float)GetFilePrioAsNumber();
//<<< WiZaRd::ClientAnalyzer
    return 10.0f * credits->GetScoreRatio(GetIP()) * (float)GetFilePrioAsNumber();
}

/**
 * Gets the file multiplier for the file this client has requested.
 */
int GetFilePrio(const CKnownFile* currequpfile); //>>> WiZaRd::ClientAnalyzer
int CUpDownClient::GetFilePrioAsNumber() const
{
    CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
	if (currequpfile == NULL && SupportsSCT())
		currequpfile = theApp.downloadqueue->GetFileByID(requpfileid);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (!currequpfile)
        return 0;

    return GetFilePrio(currequpfile); //>>> WiZaRd::ClientAnalyzer
}

//>>> WiZaRd::ClientAnalyzer
//WiZaRd: optimized! there's no need to retrieve the reqfile if we already HAVE it
int GetFilePrio(const CKnownFile* currequpfile)
//<<< WiZaRd::ClientAnalyzer
{
    // TODO coded by tecxx & herbert, one yet unsolved problem here:
    // sometimes a client asks for 2 files and there is no way to decide, which file the
    // client finally gets. so it could happen that he is queued first because of a
    // high prio file, but then asks for something completely different.
    int filepriority = 10; // standard
    switch (currequpfile->GetUpPriority())
    {
    case PR_VERYHIGH:
        filepriority = 18;
        break;
    case PR_HIGH:
        filepriority = 9;
        break;
    case PR_LOW:
        filepriority = 6;
        break;
    case PR_VERYLOW:
        filepriority = 2;
        break;
    case PR_NORMAL:
    default:
        filepriority = 7;
        break;
    }

    return filepriority;
}

/**
 * Gets the current waiting score for this client, taking into consideration waiting
 * time, priority of requested file, and the client's credits.
 */
UINT CUpDownClient::GetScore(bool sysvalue, bool isdownloading, bool onlybasevalue) const
{
    if (!m_pszUsername)
        return 0;

    if (credits == 0)
    {
        ASSERT(IsKindOf(RUNTIME_CLASS(CUrlClient)));
        return 0;
    }

    // bad clients (see note in function)
    if (credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY)
        return 0;
    // friend slot
    if (IsFriend() && GetFriendSlot() && !HasLowID())
        return 0x0FFFFFFF;

    if (sysvalue && HasLowID() && !(socket && socket->IsConnected()))
        return 0;

    if (m_bGPLEvilDoerNick || m_bGPLEvilDoerMod || IsBanned()) //>>> WiZaRd::More GPLEvilDoers
        return 0;

    CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
	if (currequpfile == NULL && SupportsSCT())
		currequpfile = theApp.downloadqueue->GetFileByID(requpfileid);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (!currequpfile)
        return 0;

    // calculate score, based on waitingtime and other factors
    float fBaseValue;
    if (onlybasevalue)
        fBaseValue = 100;
    else if (!isdownloading)
        fBaseValue = (float)(::GetTickCount()-GetWaitStartTime())/1000;
    else
    {
        // we dont want one client to download forever
        // the first 15 min downloadtime counts as 15 min waitingtime and you get a 15 min bonus while you are in the first 15 min :)
        // (to avoid 20 sec downloads) after this the score won't raise anymore
        fBaseValue = (float)(m_dwUploadTime-GetWaitStartTime());
        //ASSERT ( m_dwUploadTime-GetWaitStartTime() >= 0 ); //oct 28, 02: changed this from "> 0" to ">= 0" -> // 02-Okt-2006 []: ">=0" is always true!
        fBaseValue += (float)(::GetTickCount() - m_dwUploadTime > 900000)? 900000:1800000;
        fBaseValue /= 1000;
    }

    if (pAntiLeechData)
        fBaseValue *= pAntiLeechData->GetScore();
    else
    {
        ASSERT(0); //should never happen... flow over to default scoring
        fBaseValue *= credits->GetScoreRatio(GetIP());
    }

    if (!onlybasevalue)
    {
        int filepriority = GetFilePrio(currequpfile);
        fBaseValue *= (float(filepriority)/10.0f);
    }

    if ((IsEmuleClient() || this->GetClientSoft() < 10) && m_byEmuleVersion <= 0x19)
        fBaseValue *= 0.5f;
    return (UINT)fBaseValue;
}

class CSyncHelper
{
public:
    CSyncHelper()
    {
        m_pObject = NULL;
    }
    ~CSyncHelper()
    {
        if (m_pObject)
            m_pObject->Unlock();
    }
    CSyncObject* m_pObject;
};

void CUpDownClient::CreateNextBlockPackage(bool bBigBuffer)
{
    // See if we can do an early return. There may be no new blocks to load from disk and add to buffer, or buffer may be large enough allready.
    const UINT nBufferLimit = bBigBuffer ? (800 * 1024) : (50 * 1024);
    if (m_BlockRequests_queue.IsEmpty() || // There are no new blocks requested
            (m_addedPayloadQueueSession > GetQueueSessionPayloadUp() && GetPayloadInBuffer() > nBufferLimit))
    {
        // the buffered data is large enough allready
        return;
    }

    CFile file;
    byte* filedata = 0;
    CString fullname;
    bool bFromPF = true; // Statistic to breakdown uploaded data by complete file vs. partfile.
    CSyncHelper lockFile;
    try
    {
        // Buffer new data if current buffer is less than nBufferLimit Bytes
        while (!m_BlockRequests_queue.IsEmpty() &&
                (m_addedPayloadQueueSession <= GetQueueSessionPayloadUp() || GetPayloadInBuffer() < nBufferLimit))
        {

            Requested_Block_Struct* currentblock = m_BlockRequests_queue.GetHead();
            CKnownFile* srcfile = theApp.sharedfiles->GetFileByID(currentblock->FileID);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
			if (srcfile == NULL && SupportsSCT())
				srcfile = theApp.downloadqueue->GetFileByID(requpfileid);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
            if (!srcfile)
                throw GetResString(IDS_ERR_REQ_FNF);

            if (srcfile->IsPartFile() && ((CPartFile*)srcfile)->GetStatus() != PS_COMPLETE)
            {
                // Do not access a part file, if it is currently moved into the incoming directory.
                // Because the moving of part file into the incoming directory may take a noticable
                // amount of time, we can not wait for 'm_FileCompleteMutex' and block the main thread.
                if (!((CPartFile*)srcfile)->m_FileCompleteMutex.Lock(0))  // just do a quick test of the mutex's state and return if it's locked.
                {
                    return;
                }
                lockFile.m_pObject = &((CPartFile*)srcfile)->m_FileCompleteMutex;
                // If it's a part file which we are uploading the file remains locked until we've read the
                // current block. This way the file completion thread can not (try to) "move" the file into
                // the incoming directory.

                fullname = RemoveFileExtension(((CPartFile*)srcfile)->GetFullName());
            }
            else
            {
                fullname.Format(_T("%s\\%s"),srcfile->GetPath(),srcfile->GetFileName());
            }

            uint64 i64uTogo;
            if (currentblock->StartOffset > currentblock->EndOffset)
            {
                i64uTogo = currentblock->EndOffset + (srcfile->GetFileSize() - currentblock->StartOffset);
            }
            else
            {
                i64uTogo = currentblock->EndOffset - currentblock->StartOffset;
                if (srcfile->IsPartFile() && !((CPartFile*)srcfile)->IsComplete(currentblock->StartOffset,currentblock->EndOffset-1, true))
                    throw GetResString(IDS_ERR_INCOMPLETEBLOCK);
//>>> WiZaRd::Intelligent SOTN
				const CPartStatus* m_abyUpPartStatus = GetUpPartStatus(); //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
                if (m_abyUpPartStatus == NULL)
                {
                    CString err;
                    err.Format(L"Client (%s) asked for blocks without telling us what he needs (should never happen!)", DbgGetClientInfo());
                    throw err;
                }
                if (m_abyUpPartStatusHidden)
                {
                    for (uint16 i = (uint16)(currentblock->StartOffset/PARTSIZE); i < srcfile->GetPartCount() && i < (uint16)((currentblock->EndOffset-1)/PARTSIZE+1); ++i)
                    {
                        if (m_abyUpPartStatusHidden[i] != 0)
                        {
//							CString err;
//							err.Format(L"Client (%s) requested part %u of %s which is actually hidden!?", DbgGetClientInfo(), i, srcfile->GetFileName());
//							throw err;
                            theApp.QueueDebugLogLineEx(LOG_ERROR, L"Client (%s) requested part %u of %s which is actually hidden via %s!?", DbgGetClientInfo(), i, srcfile->GetFileName(), m_abyUpPartStatusHidden[i] == 1 ? L"TCP" : L"UDP");
                        }
                    }
                }
//<<< WiZaRd::Intelligent SOTN
            }

            if (i64uTogo > EMBLOCKSIZE*3)
                throw GetResString(IDS_ERR_LARGEREQBLOCK);
            UINT togo = (UINT)i64uTogo;

            if (!srcfile->IsPartFile())
            {
                bFromPF = false; // This is not a part file...
                if (!file.Open(fullname,CFile::modeRead|CFile::osSequentialScan|CFile::shareDenyNone))
                    throw GetResString(IDS_ERR_OPEN);
                file.Seek(currentblock->StartOffset,0);

                filedata = new byte[togo+500];
                if (UINT done = file.Read(filedata,togo) != togo)
                {
                    file.SeekToBegin();
                    file.Read(filedata + done,togo-done);
                }
                file.Close();
            }
            else
            {
                CPartFile* partfile = (CPartFile*)srcfile;

                partfile->m_hpartfile.Seek(currentblock->StartOffset,0);

                filedata = new byte[togo+500];
                if (UINT done = partfile->m_hpartfile.Read(filedata,togo) != togo)
                {
                    partfile->m_hpartfile.SeekToBegin();
                    partfile->m_hpartfile.Read(filedata + done,togo-done);
                }
            }
            if (lockFile.m_pObject)
            {
                lockFile.m_pObject->Unlock(); // Unlock the (part) file as soon as we are done with accessing it.
                lockFile.m_pObject = NULL;
            }

            SetUploadFileID(srcfile);

            // check extension to decide whether to compress or not
            //This code is EXTREMELY slow... and considering that it's called VERY often, we shouldn't rely so heavily on it!
            //One thing we could do is to cache the fileext, though that requires too many changes right now... and it's the same with
            //some kind of compressability evaluation - so here's a little optimization - better than nothing :)
            //can't be compressed anyways, no need to call expensive checks below
            bool compFlag = m_byDataCompVer == 1;
            if (compFlag)
            {
                EFileType type = srcfile->GetVerifiedFileType();
                //skip archive files in any case... and avi if selected to
                compFlag = type != ARCHIVE_ZIP
                           && type != ARCHIVE_RAR
                           && type != ARCHIVE_ACE
                           && (type != VIDEO_AVI || !thePrefs.GetDontCompressAvi());
                if (compFlag)
                {
//>>> WiZaRd::ExtCheck
                    //just a quick check for the filetype... we don't compress archives (again)
                    if (GetED2KFileTypeID(srcfile->GetFileName()) == ED2KFT_ARCHIVE)
                        compFlag = false;
//<<< WiZaRd::ExtCheck
                    else
                    {
                        //we can't fully rely on the verified filetypes, so flow over to the default selection
                        CString ext = srcfile->GetFileName();
                        int pos = ext.ReverseFind(L'.');
                        if (pos > -1)
                            ext = ext.Mid(pos);
                        ext.MakeLower();
//>>> taz::more
//>>> WiZaRd::ExtCheck
                        //reworked taz' code and removed the duplicate (archive) checks
//						compFlag = (ext!=_T(".zip") && ext!=_T(".cbz") && ext!=_T(".rar") && ext!=_T(".cbr") && ext!=_T(".ace") && ext!=_T(".ogm"));
                        compFlag = (ext!=L".ogm"
                                    && ext!=L".ape" && ext!=L".flac" && ext!=L".wmv" && ext!=L".asf"&& ext!=L".flv" && ext!=L".depot");
//						compFlag = (ext!=L".zip" && ext!=L".cbz" && ext!=L".rar" && ext!=L".cbr" && ext!=L".ace" && ext!=L".ogm"
//							&& ext!=L".7z" && ext!=L".ape" && ext!=L".flac" && ext!=L".wmv" && ext!=L".asf"&& ext!=L".flv" && ext!=L".gz" && ext!=L".tgz" && ext!=L".arj" && ext!=L".bz2" && ext!=L".cab" && ext!=L".depot");
//<<< WiZaRd::ExtCheck
//<<< taz::more
                        if (compFlag && ext==_T(".avi") && thePrefs.GetDontCompressAvi())
                            compFlag = false;
                    }
                }
            }
            if (compFlag)
                CreatePackedPackets(filedata,togo,currentblock,bFromPF);
            else
                CreateStandartPackets(filedata,togo,currentblock,bFromPF);

            // file statistic
            srcfile->statistic.AddTransferred(togo);

            m_addedPayloadQueueSession += togo;

            m_DoneBlocks_list.AddHead(m_BlockRequests_queue.RemoveHead());
            delete[] filedata;
            filedata = 0;
        }
    }
    catch (CString error)
    {
        if (thePrefs.GetVerbose())
            DebugLogWarning(GetResString(IDS_ERR_CLIENTERRORED), GetUserName(), error);
        theApp.uploadqueue->RemoveFromUploadQueue(this, _T("Client error: ") + error);
        delete[] filedata;
        return;
    }
    catch (CFileException* e)
    {
        TCHAR szError[MAX_CFEXP_ERRORMSG];
        e->GetErrorMessage(szError, ARRSIZE(szError));
        if (thePrefs.GetVerbose())
            DebugLogWarning(_T("Failed to create upload package for %s - %s"), GetUserName(), szError);
        theApp.uploadqueue->RemoveFromUploadQueue(this, ((CString)_T("Failed to create upload package.")) + szError);
        delete[] filedata;
        e->Delete();
        return;
    }
}

bool CUpDownClient::ProcessExtendedInfo(CSafeMemFile* data, CKnownFile* tempreqfile, const bool bIsUDP)
{
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
	if (m_pUpPartStatus != NULL)
		tempreqfile->RemoveFromPartsInfo(m_pUpPartStatus);
//	delete[] m_abyUpPartStatus;
//	m_abyUpPartStatus = NULL;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
//>>> WiZaRd::Intelligent SOTN
    //No need to clear it here - might be useful later!
    //	delete[] m_abyUpPartStatusHidden;
    //	m_abyUpPartStatusHidden = NULL;
	const UINT nOldUpPartCount = GetUpPartCount();
    //const UINT nOldUpPartCount = m_nUpPartCount;
//<<< WiZaRd::Intelligent SOTN
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
//	m_nUpPartCount = 0;
	delete m_pUpPartStatus;
	m_pUpPartStatus = NULL;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]    	
    m_nUpCompleteSourcesCount= 0;

    if (GetExtendedRequestsVersion() == 0)
    {
//>>> WiZaRd::Intelligent SOTN
        //returning here means that we already lost the partstatus,
        //so we should delete the hidden status, too (I guess?)
        delete[] m_abyUpPartStatusHidden;
        m_abyUpPartStatusHidden = NULL;
//<<< WiZaRd::Intelligent SOTN
        return true;
    }

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
	m_pUpPartStatus = new CCrumbMap(tempreqfile->GetFileSize());
	m_pUpPartStatus->ReadPartStatus(data, true);
		
	// netfinity: Update download partstatus if we are downloading this file and there is more pieces available in the upload partstatus
	if (reqfile == tempreqfile && (m_pPartStatus == NULL || (m_pUpPartStatus->GetCompleted() >= m_pPartStatus->GetCompleted())))
	{
		if (m_pPartStatus != NULL)
			reqfile->RemoveFromPartsInfo(m_pPartStatus);
		delete m_pPartStatus;
		m_pPartStatus = NULL;
		m_pPartStatus = new CCrumbMap(m_pUpPartStatus);

		reqfile->AddToPartsInfo(m_pPartStatus);
	}

	const UINT m_nUpPartCount = GetUpPartCount();

	// Passive Src Finding
	bool bPartsNeeded = false;
	bool bShouldCheck = bIsUDP && tempreqfile->IsPartFile()
		&& (((CPartFile*)tempreqfile)->GetStatus() == PS_EMPTY || ((CPartFile*)tempreqfile)->GetStatus() == PS_READY)
		&& !(GetDownloadState() == DS_ONQUEUE && reqfile == tempreqfile);

//>>> WiZaRd::Intelligent SOTN
    if (m_abyUpPartStatusHidden == NULL || nOldUpPartCount != m_nUpPartCount)
    {
        delete[] m_abyUpPartStatusHidden;
        m_abyUpPartStatusHidden = NULL;
        m_abyUpPartStatusHidden = new uint8[m_nUpPartCount];
        memset(m_abyUpPartStatusHidden, 0, m_nUpPartCount);
    }
//<<< WiZaRd::Intelligent SOTN

	uint16 done = 0;
	uint16 complcount = 0; //>>> WiZaRd::ClientAnalyzer
	while (done != m_nUpPartCount)
	{
		if(m_pPartStatus->IsComplete((uint64)done*PARTSIZE, ((uint64)(done+1)*PARTSIZE)-1))
		{
			if (bShouldCheck && !bPartsNeeded && !((CPartFile*)tempreqfile)->IsComplete((uint64)done*PARTSIZE, ((uint64)(done+1)*PARTSIZE)-1, false))
				bPartsNeeded = true;
			//We may want to use this for another feature..
//			if (!tempreqfile->IsComplete((uint64)done*PARTSIZE,((uint64)(done+1)*PARTSIZE)-1))
//				bPartsNeeded = true;
			++complcount; //>>> WiZaRd::ClientAnalyzer
		}
		++done;
	}
//>>> WiZaRd::ClientAnalyzer
	if (pAntiLeechData)
	{
		if (complcount == m_nUpPartCount)
			pAntiLeechData->SetBadForThisSession(AT_FILEFAKER);
		else
			pAntiLeechData->ClearBadForThisSession(AT_FILEFAKER);
	}
//<<< WiZaRd::ClientAnalyzer
/*
    // Passive Src Finding
    bool bPartsNeeded = false;
    bool bShouldCheck = bIsUDP && tempreqfile->IsPartFile()
                        && (((CPartFile*)tempreqfile)->GetStatus() == PS_EMPTY || ((CPartFile*)tempreqfile)->GetStatus() == PS_READY)
                        && !(GetDownloadState() == DS_ONQUEUE && reqfile == tempreqfile);

    uint16 nED2KUpPartCount = data->ReadUInt16();
    if (!nED2KUpPartCount)
    {
        m_nUpPartCount = tempreqfile->GetPartCount();
        m_abyUpPartStatus = new uint8[m_nUpPartCount];
        memset(m_abyUpPartStatus, 0, m_nUpPartCount);
//>>> WiZaRd::Intelligent SOTN
        if (m_abyUpPartStatusHidden == NULL || nOldUpPartCount != m_nUpPartCount)
        {
            delete[] m_abyUpPartStatusHidden;
            m_abyUpPartStatusHidden = NULL;
            m_abyUpPartStatusHidden = new uint8[m_nUpPartCount];
            memset(m_abyUpPartStatusHidden, 0, m_nUpPartCount);
        }
//<<< WiZaRd::Intelligent SOTN
    }
    else
    {
        if (tempreqfile->GetED2KPartCount() != nED2KUpPartCount)
        {
            //We already checked if we are talking about the same file.. So if we get here, something really strange happened!
            m_nUpPartCount = 0;
//>>> WiZaRd::Intelligent SOTN
            //as stated above - if we get here, something's REALLY wrong!
            delete[] m_abyUpPartStatusHidden;
            m_abyUpPartStatusHidden = NULL;
//<<< WiZaRd::Intelligent SOTN
            return false;
        }
        m_nUpPartCount = tempreqfile->GetPartCount();
        m_abyUpPartStatus = new uint8[m_nUpPartCount];
        uint16 done = 0;
//>>> WiZaRd::Intelligent SOTN
        if (m_abyUpPartStatusHidden == NULL || nOldUpPartCount != m_nUpPartCount)
        {
            delete[] m_abyUpPartStatusHidden;
            m_abyUpPartStatusHidden = NULL;
            m_abyUpPartStatusHidden = new uint8[m_nUpPartCount];
            memset(m_abyUpPartStatusHidden, 0, m_nUpPartCount);
        }
//<<< WiZaRd::Intelligent SOTN
        uint16 complcount = 0; //>>> WiZaRd::ClientAnalyzer
        while (done != m_nUpPartCount)
        {
            const uint8 toread = data->ReadUInt8();
            for (UINT i = 0; i != 8; i++)
            {
				const bool partDone = ((toread >> i) & 1) ? 1 : 0;
                m_abyUpPartStatus[done] = partDone;

				if(partDone)
				{
					if (bShouldCheck && !bPartsNeeded && !((CPartFile*)tempreqfile)->IsComplete((uint64)done*PARTSIZE, ((uint64)(done+1)*PARTSIZE)-1, false))
						bPartsNeeded = true;
					//We may want to use this for another feature..
//					if (!tempreqfile->IsComplete((uint64)done*PARTSIZE,((uint64)(done+1)*PARTSIZE)-1))
//						bPartsNeeded = true;
                    ++complcount; //>>> WiZaRd::ClientAnalyzer
				}
                ++done;
                if (done == m_nUpPartCount)
                    break;
            }
        }
//>>> WiZaRd::ClientAnalyzer
        if (pAntiLeechData)
        {
            if (complcount == m_nUpPartCount)
                pAntiLeechData->SetBadForThisSession(AT_FILEFAKER);
            else
                pAntiLeechData->ClearBadForThisSession(AT_FILEFAKER);
        }
//<<< WiZaRd::ClientAnalyzer
    }
*/
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

    if (GetExtendedRequestsVersion() > 1)
    {
        uint16 nCompleteCountLast = GetUpCompleteSourcesCount();
        uint16 nCompleteCountNew = data->ReadUInt16();
        SetUpCompleteSourcesCount(nCompleteCountNew);
        if (nCompleteCountLast != nCompleteCountNew)
		{
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
			if (m_pUpPartStatus != NULL)
				tempreqfile->AddToPartsInfo(m_pUpPartStatus);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
            //tempreqfile->UpdatePartsInfo();
		}
    }

    // Passive Src Finding
    if (bPartsNeeded)
    {
        //the client was a NNS but isn't any more
        if (GetDownloadState() == DS_NONEEDEDPARTS && reqfile == tempreqfile)
        {
            if (GetTimeUntilReask(reqfile, true) == 0)
                AskForDownload();
        }
        else if (GetDownloadState() != DS_ONQUEUE)
        {
            //the client maybe isn't in our downloadqueue.. let's look if we should add the client
            if ((((CPartFile*)tempreqfile)->GetSourceCount() < ((CPartFile*)tempreqfile)->GetMaxSources())
                    || ((CPartFile*)tempreqfile)->GetSourceCount() < ((CPartFile*)tempreqfile)->GetMaxSources()*0.8f + 1)
            {
                if (theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)tempreqfile,this, true))
                    AddDebugLogLine(false, L"Found new source on reask-ping: %s, file: %s", DbgGetClientInfo(), tempreqfile->GetFileName());
            }
        }
        else
        {
            if (AddRequestForAnotherFile((CPartFile*)tempreqfile))
            {
                theApp.emuledlg->transferwnd->GetDownloadList()->AddSource((CPartFile*)tempreqfile,this,true);
                AddDebugLogLine(false, L"Found new A4AF source on reask-ping: %s, file: %s", DbgGetClientInfo(), tempreqfile->GetFileName());
            }
        }
    }

    theApp.emuledlg->transferwnd->GetQueueList()->RefreshClient(this);
    return true;
}

void CUpDownClient::CreateStandartPackets(byte* data,UINT togo, Requested_Block_Struct* currentblock, bool bFromPF)
{
    UINT nPacketSize;
    CMemFile memfile((BYTE*)data,togo);
    if (togo > 10240)
        nPacketSize = togo/(UINT)(togo/10240);
    else
        nPacketSize = togo;
    while (togo)
    {
        if (togo < nPacketSize*2)
            nPacketSize = togo;
        ASSERT(nPacketSize);
        togo -= nPacketSize;

        uint64 statpos = (currentblock->EndOffset - togo) - nPacketSize;
        uint64 endpos = (currentblock->EndOffset - togo);

        Packet* packet;
        if (statpos > 0xFFFFFFFF || endpos > 0xFFFFFFFF)
        {
            packet = new Packet(OP_SENDINGPART_I64,nPacketSize+32, OP_EMULEPROT, bFromPF);
            md4cpy(&packet->pBuffer[0],GetUploadFileID());
            PokeUInt64(&packet->pBuffer[16], statpos);
            PokeUInt64(&packet->pBuffer[24], endpos);
            memfile.Read(&packet->pBuffer[32],nPacketSize);
            theStats.AddUpDataOverheadFileRequest(32);
        }
        else
        {
            packet = new Packet(OP_SENDINGPART,nPacketSize+24, OP_EDONKEYPROT, bFromPF);
            md4cpy(&packet->pBuffer[0],GetUploadFileID());
            PokeUInt32(&packet->pBuffer[16], (UINT)statpos);
            PokeUInt32(&packet->pBuffer[20], (UINT)endpos);
            memfile.Read(&packet->pBuffer[24],nPacketSize);
            theStats.AddUpDataOverheadFileRequest(24);
        }

        if (thePrefs.GetDebugClientTCPLevel() > 0)
        {
            DebugSend("OP__SendingPart", this, GetUploadFileID());
            Debug(_T("  Start=%I64u  End=%I64u  Size=%u\n"), statpos, endpos, nPacketSize);
        }
        // put packet directly on socket

        socket->SendPacket(packet,true,false, nPacketSize);
    }
}

void CUpDownClient::CreatePackedPackets(byte* data, UINT togo, Requested_Block_Struct* currentblock, bool bFromPF)
{
    BYTE* output = new BYTE[togo+300];
    uLongf newsize = togo+300;
    UINT result = compress2(output, &newsize, data, togo, 9);
    if (result != Z_OK || togo <= newsize)
    {
        delete[] output;
        CreateStandartPackets(data,togo,currentblock,bFromPF);
        return;
    }
    CMemFile memfile(output,newsize);
    UINT oldSize = togo;
    togo = newsize;
    UINT nPacketSize;
    if (togo > 10240)
        nPacketSize = togo/(UINT)(togo/10240);
    else
        nPacketSize = togo;

    UINT totalPayloadSize = 0;

    while (togo)
    {
        if (togo < nPacketSize*2)
            nPacketSize = togo;
        ASSERT(nPacketSize);
        togo -= nPacketSize;
        uint64 statpos = currentblock->StartOffset;
        Packet* packet;
        if (currentblock->StartOffset > 0xFFFFFFFF || currentblock->EndOffset > 0xFFFFFFFF)
        {
            packet = new Packet(OP_COMPRESSEDPART_I64,nPacketSize+28,OP_EMULEPROT,bFromPF);
            md4cpy(&packet->pBuffer[0],GetUploadFileID());
            PokeUInt64(&packet->pBuffer[16], statpos);
            PokeUInt32(&packet->pBuffer[24], newsize);
            memfile.Read(&packet->pBuffer[28],nPacketSize);
        }
        else
        {
            packet = new Packet(OP_COMPRESSEDPART,nPacketSize+24,OP_EMULEPROT,bFromPF);
            md4cpy(&packet->pBuffer[0],GetUploadFileID());
            PokeUInt32(&packet->pBuffer[16], (UINT)statpos);
            PokeUInt32(&packet->pBuffer[20], newsize);
            memfile.Read(&packet->pBuffer[24],nPacketSize);
        }

        if (thePrefs.GetDebugClientTCPLevel() > 0)
        {
            DebugSend("OP__CompressedPart", this, GetUploadFileID());
            Debug(_T("  Start=%I64u  BlockSize=%u  Size=%u\n"), statpos, newsize, nPacketSize);
        }
        // approximate payload size
        UINT payloadSize = nPacketSize*oldSize/newsize;

        if (togo == 0 && totalPayloadSize+payloadSize < oldSize)
        {
            payloadSize = oldSize-totalPayloadSize;
        }
        totalPayloadSize += payloadSize;

        // put packet directly on socket
        theStats.AddUpDataOverheadFileRequest(24);
        socket->SendPacket(packet,true,false, payloadSize);
    }
    delete[] output;
}

void CUpDownClient::SetUploadFileID(CKnownFile* newreqfile)
{
    CKnownFile* oldreqfile;
    //We use the knownfilelist because we may have unshared the file..
    //But we always check the download list first because that person may have decided to redownload that file.
    //Which will replace the object in the knownfilelist if completed.
    if ((oldreqfile = theApp.downloadqueue->GetFileByID(requpfileid)) == NULL)
        oldreqfile = theApp.knownfiles->FindKnownFileByID(requpfileid);
    else
    {
        // In some _very_ rare cases it is possible that we have different files with the same hash in the downloadlist
        // as well as in the sharedlist (redownloading a unshared file, then resharing it before the first part has been downloaded)
        // to make sure that in no case a deleted client object is left on the list, we need to doublecheck
        // TODO: Fix the whole issue properly
        CKnownFile* pCheck = theApp.sharedfiles->GetFileByID(requpfileid);
        if (pCheck != NULL && pCheck != oldreqfile)
        {
            ASSERT(0);
            pCheck->RemoveUploadingClient(this);
        }
    }

    if (newreqfile == oldreqfile)
        return;

    // clear old status
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
	delete m_pUpPartStatus;
	m_pUpPartStatus = NULL;
    //delete[] m_abyUpPartStatus;
    //m_abyUpPartStatus = NULL;
	//m_nUpPartCount = 0;
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
//>>> WiZaRd::Intelligent SOTN
    delete[] m_abyUpPartStatusHidden;
    m_abyUpPartStatusHidden = NULL;
//<<< WiZaRd::Intelligent SOTN    
    m_nUpCompleteSourcesCount= 0;

    if (newreqfile)
    {
        newreqfile->AddUploadingClient(this);
        md4cpy(requpfileid, newreqfile->GetFileHash());
    }
    else
        md4clr(requpfileid);

    if (oldreqfile)
        oldreqfile->RemoveUploadingClient(this);
}

bool CUpDownClient::AddReqBlock(Requested_Block_Struct* reqblock, const bool bChecksNecessary)
{
    if (bChecksNecessary)
    {
        if (GetUploadState() != US_UPLOADING)
        {
            if (thePrefs.GetLogUlDlEvents())
                AddDebugLogLine(DLP_LOW, false, _T("UploadClient: Client tried to add req block when not in upload slot! Prevented req blocks from being added. %s"), DbgGetClientInfo());
            delete reqblock;
            return false;
        }

        CKnownFile* pDownloadingFile = theApp.sharedfiles->GetFileByID(reqblock->FileID);
//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
		if (pDownloadingFile == NULL && SupportsSCT())
			pDownloadingFile = theApp.downloadqueue->GetFileByID(reqblock->FileID);
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]
        if (pDownloadingFile == NULL)
        {
            theApp.QueueDebugLogLineEx(LOG_WARNING, L"Client %s tried to add a blockrequest for a non-existing file!", DbgGetClientInfo());
            delete reqblock;
            return false;
        }

//>>> WiZaRd::Small File Slot
        const uint64 smallFileSize = theApp.uploadqueue->GetSmallFileSize();
        if (pDownloadingFile && pDownloadingFile->GetFileSize() < smallFileSize)
        {
            //he requests a different file?
            if (md4cmp(reqblock->FileID, requpfileid) != 0)
            {
                const bool bigNew = pDownloadingFile->GetFileSize() >= smallFileSize;
                theApp.QueueDebugLogLineEx(bigNew ? LOG_ERROR : LOG_WARNING, L"Client %s tried to add a blockrequest for a changed small file - request was %s", DbgGetClientInfo(), bigNew ? L"blocked" : L"granted");
                if (bigNew)
                {
                    delete reqblock;
                    return false;
                }
            }
        }
//<<< WiZaRd::Small File Slot
    }

    for (POSITION pos = m_DoneBlocks_list.GetHeadPosition(); pos != 0;)
    {
        const Requested_Block_Struct* cur_reqblock = m_DoneBlocks_list.GetNext(pos);
//>>> WiZaRd::FiX!?
        if (!md4cmp(reqblock->FileID, cur_reqblock->FileID))
//<<< WiZaRd::FiX!?
        {
//zz_fly :: Don't transmit for nested/overlapping data requests :: emuleplus :: start
//		    if (reqblock->StartOffset == cur_reqblock->StartOffset && reqblock->EndOffset == cur_reqblock->EndOffset)
            if (reqblock->StartOffset >= cur_reqblock->StartOffset && reqblock->EndOffset <= cur_reqblock->EndOffset)
//zz_fly :: Don't transmit for nested/overlapping data requests :: emuleplus :: end
            {
                delete reqblock;
                return true; // already added... no error
            }
        }
    }
    for (POSITION pos = m_BlockRequests_queue.GetHeadPosition(); pos != 0;)
    {
        const Requested_Block_Struct* cur_reqblock = m_BlockRequests_queue.GetNext(pos);
//>>> WiZaRd::FiX!?
        if (!md4cmp(reqblock->FileID, cur_reqblock->FileID))
//<<< WiZaRd::FiX!?
        {
//zz_fly :: Don't transmit for nested/overlapping data requests :: emuleplus :: start
//		    if (reqblock->StartOffset == cur_reqblock->StartOffset && reqblock->EndOffset == cur_reqblock->EndOffset)
            if (reqblock->StartOffset >= cur_reqblock->StartOffset && reqblock->EndOffset <= cur_reqblock->EndOffset)
//zz_fly :: Don't transmit for nested/overlapping data requests :: emuleplus :: end
            {
                delete reqblock;
                return true; // already added... no error
            }
        }
    }

    m_BlockRequests_queue.AddTail(reqblock);
    return true; // alrighty :)
}

UINT CUpDownClient::SendBlockData()
{
    DWORD curTick = ::GetTickCount();

    uint64 sentBytesCompleteFile = 0;
    uint64 sentBytesPartFile = 0;
    uint64 sentBytesPayload = 0;

    if (GetFileUploadSocket())
    {
        CEMSocket* s = GetFileUploadSocket();
        UINT uUpStatsPort = GetUserPort();

        // Extended statistics information based on which client software and which port we sent this data to...
        // This also updates the grand total for sent bytes, etc.  And where this data came from.
        sentBytesCompleteFile = s->GetSentBytesCompleteFileSinceLastCallAndReset();
        sentBytesPartFile = s->GetSentBytesPartFileSinceLastCallAndReset();
        thePrefs.Add2SessionTransferData(GetClientSoft(), uUpStatsPort, false, true, (UINT)sentBytesCompleteFile, (IsFriend() && GetFriendSlot()));
        thePrefs.Add2SessionTransferData(GetClientSoft(), uUpStatsPort, true, true, (UINT)sentBytesPartFile, (IsFriend() && GetFriendSlot()));

        m_nTransferredUp = (UINT)(m_nTransferredUp + sentBytesCompleteFile + sentBytesPartFile);
        credits->AddUploaded((UINT)(sentBytesCompleteFile + sentBytesPartFile), GetIP());

//>>> WiZaRd::ClientAnalyzer
        if (pAntiLeechData)
        {
            UINT srccount = _UI32_MAX; //no file? then default to non-rare...
            if (GetUploadFileID())
            {
                CKnownFile* file = theApp.sharedfiles->GetFileByID(GetUploadFileID());
                if (file)
                {
                    srccount = file->m_nCompleteSourcesCount;
                    //					srccount += file->GetRealQueuedCount(); //if you implemented that...
                }
            }
            pAntiLeechData->AddUploaded(sentBytesCompleteFile, false, srccount);
            pAntiLeechData->AddUploaded(sentBytesPartFile, true, srccount);
        }
//<<< WiZaRd::ClientAnalyzer

        sentBytesPayload = s->GetSentPayloadSinceLastCallAndReset();
        m_nCurQueueSessionPayloadUp = (UINT)(m_nCurQueueSessionPayloadUp + sentBytesPayload);

//>>> WiZaRd::ZZUL Upload [ZZ]
        if (GetUploadState() == US_UPLOADING)
        {
            bool wasRemoved = false;
            //if(!IsScheduledForRemoval() && GetQueueSessionPayloadUp() > SESSIONMAXTRANS+1*1024 && curTick-m_dwLastCheckedForEvictTick >= 5*1000) {
            //    m_dwLastCheckedForEvictTick = curTick;
            //    wasRemoved = theApp.uploadqueue->RemoveOrMoveDown(this, true);
            //}

            if (!IsScheduledForRemoval() && /*wasRemoved == false &&*/ GetQueueSessionPayloadUp() > GetCurrentSessionLimit())
            {
                // Should we end this upload?

                // first clear the average speed, to show ?? as speed in upload slot display
                m_AvarageUDR_list.RemoveAll();
                m_nSumForAvgUpDataRate = 0;

                // Give clients in queue a chance to kick this client out.
                // It will be kicked out only if queue contains a client
                // of same/higher class as this client, and that new
                // client must either be a high ID client, or a firewalled
                // client that is currently connected.
                wasRemoved = theApp.uploadqueue->RemoveOrMoveDown(this);

                // It wasn't removed, so it is allowed to pass into the next amount.
                if (!wasRemoved)
                    ++m_curSessionAmountNumber;
            }

            if (wasRemoved)
            {
                //if (thePrefs.GetDebugClientTCPLevel() > 0)
                // DebugSend("OP__OutOfPartReqs", this);
                //Packet* pCancelTransferPacket = new Packet(OP_OUTOFPARTREQS, 0);
                //theStats.AddUpDataOverheadFileRequest(pCancelTransferPacket->size);
                //socket->SendPacket(pCancelTransferPacket,true,true);
                // WiZaRd:: todo?
            }
            else
            {
                // read blocks from file and put on socket
                CreateNextBlockPackage();
            }
        }
        /*
                if (theApp.uploadqueue->CheckForTimeOver(this))
        		{
        			theApp.uploadqueue->RemoveFromUploadQueue(this, _T("Completed transfer"), true);
        			SendOutOfPartReqsAndAddToWaitingQueue();
        		}
        		else
        		{
        			// read blocks from file and put on socket
        			CreateNextBlockPackage();
        		}
        */
//<<< WiZaRd::ZZUL Upload [ZZ]
    }

    if (sentBytesCompleteFile + sentBytesPartFile > 0 ||
            m_AvarageUDR_list.GetCount() == 0 || (curTick - m_AvarageUDR_list.GetTail().timestamp) > 1*1000)
    {
        // Store how much data we've transferred this round,
        // to be able to calculate average speed later
        // keep sum of all values in list up to date
        TransferredData newitem = {(UINT)(sentBytesCompleteFile + sentBytesPartFile), curTick};
        m_AvarageUDR_list.AddTail(newitem);
        m_nSumForAvgUpDataRate = (UINT)(m_nSumForAvgUpDataRate + sentBytesCompleteFile + sentBytesPartFile);
    }

    // remove to old values in list
    while (m_AvarageUDR_list.GetCount() > 0 && (curTick - m_AvarageUDR_list.GetHead().timestamp) > 10*1000)
    {
        // keep sum of all values in list up to date
        m_nSumForAvgUpDataRate -= m_AvarageUDR_list.RemoveHead().datalen;
    }

    // Calculate average speed for this slot
    if (m_AvarageUDR_list.GetCount() > 0 && (curTick - m_AvarageUDR_list.GetHead().timestamp) > 0 && GetUpStartTimeDelay() > 2*1000)
    {
        m_nUpDatarate = (UINT)(((ULONGLONG)m_nSumForAvgUpDataRate*1000) / (curTick - m_AvarageUDR_list.GetHead().timestamp));
    }
    else
    {
        // not enough values to calculate trustworthy speed. Use -1 to tell this
        m_nUpDatarate = 0; //-1;
    }

    // Check if it's time to update the display.
    if (curTick-m_lastRefreshedULDisplay > MINWAIT_BEFORE_ULDISPLAY_WINDOWUPDATE+(UINT)(rand()*800/RAND_MAX))
    {
        // Update display
        theApp.emuledlg->transferwnd->GetUploadList()->RefreshClient(this);
        theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(this);
        m_lastRefreshedULDisplay = curTick;
    }

    return (UINT)(sentBytesCompleteFile + sentBytesPartFile);
}

void CUpDownClient::SendOutOfPartReqsAndAddToWaitingQueue()
{
    //OP_OUTOFPARTREQS will tell the downloading client to go back to OnQueue..
    //The main reason for this is that if we put the client back on queue and it goes
    //back to the upload before the socket times out... We get a situation where the
    //downloader thinks it already sent the requested blocks and the uploader thinks
    //the downloader didn't send any request blocks. Then the connection times out..
    //I did some tests with eDonkey also and it seems to work well with them also..
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__OutOfPartReqs", this);
    Packet* pPacket = new Packet(OP_OUTOFPARTREQS, 0);
    theStats.AddUpDataOverheadFileRequest(pPacket->size);
    SendPacket(pPacket, true);
    m_fSentOutOfPartReqs = 1;
    theApp.uploadqueue->AddClientToQueue(this, true);
}

/**
 * See description for CEMSocket::TruncateQueues().
 */
void CUpDownClient::FlushSendBlocks()  // call this when you stop upload, or the socket might be not able to send
{
    if (socket)      //socket may be NULL...
        socket->TruncateQueues();
}

void CUpDownClient::SendHashsetPacket(const uchar* pData, UINT nSize, bool bFileIdentifiers)
{
    Packet* packet;
    CSafeMemFile fileResponse(1024);
    if (bFileIdentifiers)
    {
        CSafeMemFile data(pData, nSize);
        CFileIdentifierSA fileIdent;
        if (!fileIdent.ReadIdentifier(&data))
            throw _T("Bad FileIdentifier (OP_HASHSETREQUEST2)");
        CKnownFile* file = theApp.sharedfiles->GetFileByIdentifier(fileIdent, false);
        if (file == NULL)
        {
            CheckFailedFileIdReqs(fileIdent.GetMD4Hash());
            throw GetResString(IDS_ERR_REQ_FNF) + _T(" (SendHashsetPacket2)");
        }
        uint8 byOptions = data.ReadUInt8();
        bool bMD4 = (byOptions & 0x01) > 0;
        bool bAICH = (byOptions & 0x02) > 0;
        if (!bMD4 && !bAICH)
        {
            DebugLogWarning(_T("Client sent HashSet request with none or unknown HashSet type requested (%u) - file: %s, client %s")
                            , byOptions, file->GetFileName(), DbgGetClientInfo());
            return;
        }
        file->GetFileIdentifier().WriteIdentifier(&fileResponse);
        // even if we don't happen to have an AICH hashset yet for some reason we send a proper (possible empty) response
        file->GetFileIdentifier().WriteHashSetsToPacket(&fileResponse, bMD4, bAICH);
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__HashSetAnswer", this, file->GetFileIdentifier().GetMD4Hash());
        packet = new Packet(&fileResponse, OP_EMULEPROT, OP_HASHSETANSWER2);
    }
    else
    {
        if (nSize != 16)
        {
            ASSERT(0);
            return;
        }
        CKnownFile* file = theApp.sharedfiles->GetFileByID(pData);
        if (!file)
        {
            CheckFailedFileIdReqs(pData);
            throw GetResString(IDS_ERR_REQ_FNF) + _T(" (SendHashsetPacket)");
        }
        file->GetFileIdentifier().WriteMD4HashsetToFile(&fileResponse);
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__HashSetAnswer", this, pData);
        packet = new Packet(&fileResponse, OP_EDONKEYPROT, OP_HASHSETANSWER);
    }
    theStats.AddUpDataOverheadFileRequest(packet->size);
    SendPacket(packet, true);
}

void CUpDownClient::ClearUploadBlockRequests()
{
    FlushSendBlocks();

    for (POSITION pos = m_BlockRequests_queue.GetHeadPosition(); pos != 0;)
        delete m_BlockRequests_queue.GetNext(pos);
    m_BlockRequests_queue.RemoveAll();

    for (POSITION pos = m_DoneBlocks_list.GetHeadPosition(); pos != 0;)
        delete m_DoneBlocks_list.GetNext(pos);
    m_DoneBlocks_list.RemoveAll();
}

void CUpDownClient::SendRankingInfo()
{
    if (!ExtProtocolAvailable())
        return;
    UINT nRank = theApp.uploadqueue->GetWaitingPosition(this);
    if (!nRank)
        return;
    Packet* packet = new Packet(OP_QUEUERANKING,12,OP_EMULEPROT);
    PokeUInt16(packet->pBuffer+0, (uint16)nRank);
    memset(packet->pBuffer+2, 0, 10);
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__QueueRank", this);
    theStats.AddUpDataOverheadFileRequest(packet->size);
    SendPacket(packet, true);
}

void CUpDownClient::SendCommentInfo(/*const*/ CKnownFile *file)
{
    if (!m_bCommentDirty || file == NULL || !ExtProtocolAvailable() || m_byAcceptCommentVer < 1)
        return;
    m_bCommentDirty = false;

    UINT rating = file->GetFileRating();
    const CString& desc = file->GetFileComment();
    if (file->GetFileRating() == 0 && desc.IsEmpty())
        return;

    CSafeMemFile data(256);
    data.WriteUInt8((uint8)rating);
    data.WriteLongString(desc, GetUnicodeSupport());
    if (thePrefs.GetDebugClientTCPLevel() > 0)
        DebugSend("OP__FileDesc", this, file->GetFileHash());
    Packet *packet = new Packet(&data,OP_EMULEPROT);
    packet->opcode = OP_FILEDESC;
    theStats.AddUpDataOverheadFileRequest(packet->size);
    SendPacket(packet, true);
}

void CUpDownClient::AddRequestCount(const uchar* fileid)
{
    for (POSITION pos = m_RequestedFiles_list.GetHeadPosition(); pos != 0;)
    {
        Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetNext(pos);
        if (!md4cmp(cur_struct->fileid,fileid))
        {
//>>> WiZaRd::ClientAnalyzer
            if (pAntiLeechData)
                pAntiLeechData->AddReask(::GetTickCount()-cur_struct->lastasked);
//<<< WiZaRd::ClientAnalyzer
            if (::GetTickCount() - cur_struct->lastasked < MIN_REQUESTTIME && !GetFriendSlot())
            {
                if (GetDownloadState() != DS_DOWNLOADING)
                    cur_struct->badrequests++;
                if (cur_struct->badrequests == BADCLIENTBAN)
                {
                    Ban();
                }
            }
            else
            {
                if (cur_struct->badrequests)
                    cur_struct->badrequests--;
            }
            cur_struct->lastasked = ::GetTickCount();
            return;
        }
    }
    Requested_File_Struct* new_struct = new Requested_File_Struct;
    md4cpy(new_struct->fileid,fileid);
    new_struct->lastasked = ::GetTickCount();
    new_struct->badrequests = 0;
    m_RequestedFiles_list.AddHead(new_struct);
}

void  CUpDownClient::UnBan()
{
    theApp.clientlist->AddTrackClient(this);
    theApp.clientlist->RemoveBannedClient(GetIP());
    SetUploadState(US_NONE);
    ClearWaitStartTime();
    theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
//>>> WiZaRd::ClientAnalyzer
    //we can safely delete this data... we reset it anyhow!
    //also, on a sidenote, resetting the timestamp to 0 would cause problems with the CA (see "AddReask")
    while (!m_RequestedFiles_list.IsEmpty())
        delete m_RequestedFiles_list.RemoveHead();
//<<< WiZaRd::ClientAnalyzer
}

void CUpDownClient::Ban(LPCTSTR pszReason)
{
    SetChatState(MS_NONE);
    theApp.clientlist->AddTrackClient(this);
    if (!IsBanned())
    {
        if (thePrefs.GetLogBannedClients())
            AddDebugLogLine(false,_T("Banned: %s; %s"), pszReason==NULL ? _T("Aggressive behaviour") : pszReason, DbgGetClientInfo());
    }
#ifdef _DEBUG
    else
    {
        if (thePrefs.GetLogBannedClients())
            AddDebugLogLine(false,_T("Banned: (refreshed): %s; %s"), pszReason==NULL ? _T("Aggressive behaviour") : pszReason, DbgGetClientInfo());
    }
#endif
    theApp.clientlist->AddBannedClient(GetIP());
    SetUploadState(US_BANNED);
    theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
    theApp.emuledlg->transferwnd->GetQueueList()->RefreshClient(this);
    if (socket != NULL && socket->IsConnected())
        socket->ShutDown(SD_RECEIVE); // let the socket timeout, since we dont want to risk to delete the client right now. This isnt acutally perfect, could be changed later
}

UINT CUpDownClient::GetWaitStartTime() const
{
    if (credits == NULL)
    {
        ASSERT(false);
        return 0;
    }
    UINT dwResult = credits->GetSecureWaitStartTime(GetIP());
    if (dwResult > m_dwUploadTime && IsDownloading())
    {
        //this happens only if two clients with invalid securehash are in the queue - if at all
        dwResult = m_dwUploadTime-1;

        if (thePrefs.GetVerbose())
            DEBUG_ONLY(AddDebugLogLine(false,_T("Warning: CUpDownClient::GetWaitStartTime() waittime Collision (%s)"),GetUserName()));
    }
    return dwResult;
}

void CUpDownClient::SetWaitStartTime()
{
    if (credits == NULL)
    {
        return;
    }
    credits->SetSecWaitStartTime(GetIP());
}

void CUpDownClient::ClearWaitStartTime()
{
    if (credits == NULL)
    {
        return;
    }
    credits->ClearWaitStartTime();
}

bool CUpDownClient::GetFriendSlot() const
{
    if (credits && theApp.clientcredits->CryptoAvailable())
    {
        switch (credits->GetCurrentIdentState(GetIP()))
        {
        case IS_IDFAILED:
        case IS_IDNEEDED:
        case IS_IDBADGUY:
            return false;
        }
    }
    return m_bFriendSlot;
}

CEMSocket* CUpDownClient::GetFileUploadSocket(bool /*bLog*/)
{
//	if (bLog && thePrefs.GetVerbose())
//		AddDebugLogLine(false, _T("%s got normal socket."), DbgGetClientInfo());
    return socket;
}

//>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
/*
bool	CUpDownClient::IsUpPartAvailable(UINT iPart) const
{
	return (iPart >= m_nUpPartCount || !m_abyUpPartStatus) ? false : m_abyUpPartStatus[iPart] != 0;
}

UINT	CUpDownClient::GetUpPartCount() const
{
	return m_nUpPartCount;
}

uint8* CUpDownClient::GetUpPartStatus() const
{
	return m_abyUpPartStatus;
}
*/
bool	CUpDownClient::IsUpPartAvailable(UINT iPart) const 
{
	return ((m_pUpPartStatus && iPart < m_pUpPartStatus->GetPartCount()) ? m_pUpPartStatus->IsCompletePart(iPart) : false);
}

const CPartStatus*	CUpDownClient::GetUpPartStatus() const							
{ 
	return m_pUpPartStatus; 
}

UINT	CUpDownClient::GetUpPartCount() const
{ 
	return (m_pUpPartStatus != NULL ? m_pUpPartStatus->GetPartCount() : 0); 
}
//<<< WiZaRd::Sub-Chunk-Transfer [Netfinity]

//>>> WiZaRd::Small File Slot
bool	CUpDownClient::HasSmallFileUploadSlot() const
{
    if (GetUploadFileID() == NULL)
        return false;

    CKnownFile* file = theApp.sharedfiles->GetFileByID(GetUploadFileID());
    return file && file->GetFileSize() < theApp.uploadqueue->GetSmallFileSize();
}
//<<< WiZaRd::Small File Slot

bool CUpDownClient::SendHttpBlockRequests()
{
    ASSERT(m_nDownloadState == DS_DOWNLOADING);
    ASSERT(0);
    return false;
}

//>>> WiZaRd::PowerShare
bool CUpDownClient::IsPowerShared() const
{
    const CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
    return currequpfile && currequpfile->IsPowerShared();
}
//<<< WiZaRd::PowerShare
//>>> WiZaRd::Intelligent SOTN
void CUpDownClient::GetUploadingAndUploadedPart(CArray<uint16>& arr, CArray<uint16>& arrHidden)
{
    //count any part that has NOT been hidden so we have an array that counts the visibility of the chunks
    //with that information, we can select the least visible chunk later on
	const UINT m_nUpPartCount = GetUpPartCount(); //>>> WiZaRd::Sub-Chunk-Transfer [Netfinity]
    if (m_abyUpPartStatusHidden)
    {
        for (UINT i = 0; i < m_nUpPartCount; ++i)
            if (m_abyUpPartStatusHidden[i] == 0)
                ++arrHidden[i];
    }

    if (!IsDownloading())
        return;

    const Requested_Block_Struct* block = NULL;
    //uploading...
    if (!m_BlockRequests_queue.IsEmpty())
    {
        block = m_BlockRequests_queue.GetHead();
        if (block)
        {
            const UINT part = (UINT)block->StartOffset/PARTSIZE;
            arr[part] = max(1, arr[part]);
        }
    }
    //... and uploaded :)
    for (POSITION pos = m_DoneBlocks_list.GetHeadPosition(); pos;)
    {
        block = m_DoneBlocks_list.GetNext(pos);
        if (block)
        {
            const UINT part = (UINT)block->StartOffset/PARTSIZE;
            arr[part] = max(1, arr[part]);
        }
    }
}
//<<< WiZaRd::Intelligent SOTN
void CUpDownClient::ResetSessionUp()
{
    m_nCurSessionUp = m_nTransferredUp;
//>>> WiZaRd::ZZUL Upload [ZZ]
    //m_addedPayloadQueueSession = 0;
    //m_nCurQueueSessionPayloadUp = 0;
    m_addedPayloadQueueSession = GetQueueSessionPayloadUp();
    m_nCurSessionPayloadUp = m_addedPayloadQueueSession;
//<<< WiZaRd::ZZUL Upload [ZZ]
}

UINT CUpDownClient::GetPayloadInBuffer() const
{
//>>> WiZaRd::ZZUL Upload [ZZ]
    //return m_addedPayloadQueueSession - GetQueueSessionPayloadUp();
    UINT addedPayloadQueueSession = m_addedPayloadQueueSession;
    UINT queueSessionPayloadUp = GetQueueSessionPayloadUp();
    return (addedPayloadQueueSession > queueSessionPayloadUp ? addedPayloadQueueSession - queueSessionPayloadUp : 0);
//<<< WiZaRd::ZZUL Upload [ZZ]
}

//>>> WiZaRd::ZZUL Upload [ZZ]
UINT CUpDownClient::GetSessionPayloadUp() const
{
    return GetQueueSessionPayloadUp() - m_nCurSessionPayloadUp;
}

UINT CUpDownClient::GetQueueSessionUp() const
{
    return m_nTransferredUp - m_nCurQueueSessionUp;
}

void CUpDownClient::ResetQueueSessionUp()
{
    m_nCurQueueSessionUp = m_nTransferredUp;
    m_nCurQueueSessionPayloadUp = 0;
    m_curSessionAmountNumber = 0;
}
void CUpDownClient::AddToAddedPayloadQueueSession(const UINT toAdd)
{
    m_addedPayloadQueueSession += toAdd;
}

uint64 CUpDownClient::GetCurrentSessionLimit() const
{
    return (uint64)SESSIONMAXTRANS*(m_curSessionAmountNumber+1)+1*1024;
}
//<<< WiZaRd::ZZUL Upload [ZZ]