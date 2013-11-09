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
#include "BarShader.h"
#include "StatisticFile.h"
#include "ShareableFile.h"

class CxImage;
class CUpDownClient;
class Packet;
class CFileDataIO;
class CAICHHashTree;
class CAICHRecoveryHashSet;
class CCollection;
class CAICHHashAlgo;
class CPartStatus; //>>> Sub-Chunk-Transfer [WiZaRd]

typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CKnownFile : public CShareableFile
{
    DECLARE_DYNAMIC(CKnownFile)

public:
    CKnownFile();
    virtual ~CKnownFile();

    virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false, bool bRemoveControlChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

    bool	CreateFromFile(LPCTSTR directory, LPCTSTR filename, LPVOID pvProgressParam); // create date, hashset and tags from a file
    bool	LoadFromFile(CFileDataIO* file);	//load date, hashset and tags from a .met file
    bool	WriteToFile(CFileDataIO* file);
    bool	CreateAICHHashSetOnly();

    // last file modification time in (DST corrected, if NTFS) real UTC format
    // NOTE: this value can *not* be compared with NT's version of the UTC time
    CTime	GetUtcCFileDate() const
    {
        return CTime(m_tUtcLastModified);
    }
    UINT	GetUtcFileDate() const
    {
        return m_tUtcLastModified;
    }

    // Did we not see this file for a long time so that some information should be purged?
    bool	ShouldPartiallyPurgeFile() const;
    void	SetLastSeen()
    {
        m_timeLastSeen = time(NULL);
    }

    virtual void	SetFileSize(EMFileSize nFileSize);

    // nr. of 9MB parts (file data)
    __inline uint16 GetPartCount() const
    {
        return m_iPartCount;
    }

    // nr. of 9MB parts according the file size wrt ED2K protocol (OP_FILESTATUS)
    __inline uint16 GetED2KPartCount() const
    {
        return m_iED2KPartCount;
    }

    // file upload priority
    uint8	GetUpPriority(void) const
    {
        return m_iUpPriority;
    }
    void	SetUpPriority(uint8 iNewUpPriority, bool bSave = true);
    bool	IsAutoUpPriority(void) const
    {
        return m_bAutoUpPriority;
    }
    void	SetAutoUpPriority(bool NewAutoUpPriority)
    {
        m_bAutoUpPriority = NewAutoUpPriority;
    }
    void	UpdateAutoUpPriority();

    // This has lost it's meaning here.. This is the total clients we know that want this file..
    // Right now this number is used for auto priorities..
    // This may be replaced with total complete source known in the network..
    UINT	GetQueuedCount() const
    {
        return m_ClientUploadList.GetCount();
    }

    void	AddUploadingClient(CUpDownClient* client);
    void	RemoveUploadingClient(CUpDownClient* client);
    virtual void	UpdatePartsInfo();
    virtual	void	DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool bFlat) const;

    // comment
    void	SetFileComment(LPCTSTR pszComment);

    void	SetFileRating(UINT uRating);

    UINT	GetKadFileSearchID() const
    {
        return kadFileSearchID;
    }
    void	SetKadFileSearchID(UINT id)
    {
        kadFileSearchID = id;    //Don't use this unless you know what your are DOING!! (Hopefully I do.. :)
    }

    const Kademlia::WordList& GetKadKeywords() const
    {
        return wordlist;
    }

    UINT	GetLastPublishTimeKadSrc() const
    {
        return m_lastPublishTimeKadSrc;
    }
    void	SetLastPublishTimeKadSrc(UINT time, UINT buddyip)
    {
        m_lastPublishTimeKadSrc = time;
        m_lastBuddyIP = buddyip;
    }
    UINT	GetLastPublishBuddy() const
    {
        return m_lastBuddyIP;
    }
    void	SetLastPublishTimeKadNotes(UINT time)
    {
        m_lastPublishTimeKadNotes = time;
    }
    UINT	GetLastPublishTimeKadNotes() const
    {
        return m_lastPublishTimeKadNotes;
    }

    bool	PublishSrc();
    bool	PublishNotes();

    // file sharing
	Packet* GetEmptyXSPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions) const;
    virtual Packet* CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions) const;
    UINT	GetMetaDataVer() const
    {
        return m_uMetaDataVer;
    }
    void	UpdateMetaDataTags();
    void	RemoveMetaDataTags(UINT uTagType = 0);
    void	RemoveBrokenUnicodeMetaDataTags();

    // preview
    bool	IsMovie() const;
    bool    IsMusic() const; //<<< MusicPreview
    virtual bool GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
    virtual void GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender);

    // Display / Info / Strings
    virtual CString	GetInfoSummary(bool bNoFormatCommands = false) const;
    CString			GetUpPriorityDisplayString() const;

    //aich
    void	SetAICHRecoverHashSetAvailable(bool bVal)
    {
        m_bAICHRecoverHashSetAvailable = bVal;
    }
    bool	IsAICHRecoverHashSetAvailable() const
    {
        return m_bAICHRecoverHashSetAvailable;
    }

    static bool	CreateHash(const uchar* pucData, UINT uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL);



    // last file modification time in (DST corrected, if NTFS) real UTC format
    // NOTE: this value can *not* be compared with NT's version of the UTC time
    UINT	m_tUtcLastModified;

    CStatisticFile statistic;
    uint16 m_nCompleteSourcesCount;
    uint16 m_nCompleteSourcesCountLo;
    uint16 m_nCompleteSourcesCountHi;
    CUpDownClientPtrList m_ClientUploadList;
    CArray<uint16/*, uint16*/> m_AvailPartFrequency;
    CCollection* m_pCollection;

#ifdef _DEBUG
    // Diagnostic Support
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    //preview
    bool	GrabImage(CString strFileName, uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender);
    bool	LoadTagsFromFile(CFileDataIO* file);
    bool	LoadDateFromFile(CFileDataIO* file);
    static void	CreateHash(CFile* pFile, uint64 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL);
    static bool	CreateHash(FILE* fp, uint64 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut = NULL);
    virtual void	UpdateFileRatingCommentAvail(bool bForceUpdate = false);

private:
    static CBarShader s_ShareStatusBar;
    uint16	m_iPartCount;
    uint16	m_iED2KPartCount;
    uint8	m_iUpPriority;
    bool	m_bAutoUpPriority;

    UINT	kadFileSearchID;
    UINT	m_lastPublishTimeKadSrc;
    UINT	m_lastPublishTimeKadNotes;
    UINT	m_lastBuddyIP;
    Kademlia::WordList wordlist;
    UINT	m_uMetaDataVer;
    time_t	m_timeLastSeen; // we only "see" files when they are in a shared directory
    bool	m_bAICHRecoverHashSetAvailable;

//>>> WiZaRd::PowerShare
private:
    bool	m_bPowerShared;
public:
    UINT	guifileupdatetime; //>>> WiZaRd::PowerShare - CPU!
    bool	IsPowerShared() const;
    void	SetPowerShared(const bool b);
//<<< WiZaRd::PowerShare
//>>> WiZaRd::Intelligent SOTN
private:
    CArray<uint16/*, uint16*/> m_SOTNAvailPartFrequency;
    bool	m_bShareOnlyTheNeed;
public:
    bool	WriteSafePartStatus(CSafeMemFile* file, CUpDownClient* client, const bool bUDP = false);
    bool	WritePartStatus(CSafeMemFile* file, CUpDownClient* client, const bool bUDP);
    void	SetShareOnlyTheNeed(bool newValue)
    {
        m_bShareOnlyTheNeed = newValue;
    };
    bool	GetShareOnlyTheNeed(bool m_bOnlyFile = false) const;
    uint8*	GetPartStatus(CUpDownClient* client = NULL) const;
//<<< WiZaRd::Intelligent SOTN
public:
    bool	IsSharedInKad() const;
//>>> FDC [BlueSonicBoy]
private:
    int 	m_iNameContinuityBad;
public:
    void	CheckFilename(CString FileName);
    bool	DissimilarName() const;
    void	ResetFDC();
//<<< FDC [BlueSonicBoy]
//>>> WiZaRd::Ratio Indicator
public:
    double	GetSharingRatio() const;
//<<< WiZaRd::Ratio Indicator
//>>> WiZaRd::Upload Feedback
public:
    CString GetFeedBackString() const;
//<<< WiZaRd::Upload Feedback
//>>> WiZaRd::Queued Count
private:
	UINT	m_uiQueuedCount;
public:
	UINT	GetRealQueuedCount() const	{return m_uiQueuedCount;}
	void	IncRealQueuedCount(CUpDownClient* client);
	void	DecRealQueuedCount(CUpDownClient* client);
//<<< WiZaRd::Queued Count
//>>> WiZaRd::FileHealth
public:
	float	GetFileHealth();
private:
	UINT	m_dwLastFileHealthCalc;
	float	m_fLastHealthValue;
//<<< WiZaRd::FileHealth
protected:
	bool	m_bCompleteSrcUpdateNecessary;
public:
	void	ProcessFile();
protected:
	void	UpdateCompleteSrcCount();
	void	GetCompleteSrcCount(CArray<uint16, uint16>& count);
};
