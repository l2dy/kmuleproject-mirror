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
#include "FileIdentifier.h"

class CKnownFileList;
class CServerConnect;
class CPartFile;
class CKnownFile;
class CPublishKeywordList;
class CSafeMemFile;
class CServer;
class CCollection;

struct UnknownFile_Struct
{
    UnknownFile_Struct(const CString& sName, const CString& sDirectory, const CString& sSharedDirectory = L""); //>>> WiZaRd::HashingQueue Optimisation

    CString strName;
    CString strDirectory;
    CString strSharedDirectory;
    uint64	uiFileSize; //>>> WiZaRd::HashingQueue Optimisation
};

class CSharedFileList
{
    friend class CSharedFilesCtrl;
    friend class CClientReqSocket;

  public:
    CSharedFileList();
    ~CSharedFileList();

    void	Reload();
    void	Save() const;
    void	Process();
    void	Publish();
    void	RebuildMetaData();
    void	DeletePartFileInstances() const;
    void	ClearKadSourcePublishInfo();

    void	CreateOfferedFilePacket(CKnownFile* cur_file, CSafeMemFile* files, CUpDownClient* pClient = NULL);

    bool	SafeAddKFile(CKnownFile* toadd, bool bOnlyAdd = false);
    void	SetOutputCtrl(CSharedFilesCtrl* in_ctrl);
    bool	RemoveFile(CKnownFile* toremove, bool bDeleted = false);	// removes a specific shared file from the list
    void	UpdateFile(CKnownFile* toupdate);
    void	AddFileFromNewlyCreatedCollection(const CString& rstrFilePath)
    {
        CheckAndAddSingleFile(rstrFilePath);
    }

    // GUI is not initially updated
    bool	AddSingleSharedFile(const CString& rstrFilePath, bool bNoUpdate = false); // includes updating sharing preferences, calls CheckAndAddSingleSharedFile afterwards
    bool	AddSingleSharedDirectory(const CString& rstrFilePath, bool bNoUpdate = false);
    bool	ExcludeFile(CString strFilePath);	// excludes a specific file from being shared and removes it from the list if it exists

    void	AddKeywords(CKnownFile* pFile);
    void	RemoveKeywords(CKnownFile* pFile);

    void	CopySharedFileMap(CMap<CCKey,const CCKey&,CKnownFile*,CKnownFile*> &Files_Map);

    CKnownFile* GetFileByID(const uchar* filehash) const;
    CKnownFile* GetFileByIdentifier(const CFileIdentifierBase& rFileIdent, bool bStrict = false) const;
    CKnownFile*	GetFileByIndex(int index) const; // slow
    CKnownFile* GetFileByAICH(const CAICHHash& rHash) const; // slow
    bool	IsFilePtrInList(const CKnownFile* file) const; // slow
    bool	IsUnsharedFile(const uchar* auFileHash) const;
    bool	ShouldBeShared(CString strPath, CString strFilePath, bool bMustBeShared) const;
    bool	ContainsSingleSharedFiles(CString strDirectory) const; // includes subdirs
    CString GetPseudoDirName(const CString& strDirectoryName);
    CString GetDirNameByPseudo(const CString& strPseudoName) const;

    uint64	GetDatasize(uint64 &pbytesLargest) const;
    int		GetCount()
    {
        return m_Files_map.GetCount();
    }
    int		GetHashingCount()
    {
        return waitingforhash_list.GetCount()+currentlyhashing_list.GetCount();
    }
    bool	ProbablyHaveSingleSharedFiles() const
    {
        return bHaveSingleSharedFiles && !m_liSingleSharedFiles.IsEmpty();    // might not be always up-to-date, could give false "true"s, not a problem currently
    }

    void	HashFailed(UnknownFile_Struct* hashed);		// SLUGFILLER: SafeHash
    void	FileHashingFinished(CKnownFile* file);

    bool	GetPopularityRank(const CKnownFile* pFile, UINT& rnOutSession, UINT& rnOutTotal) const;

    CCriticalSection	m_mutWriteList; // don't acquire other locks while having this one in the main thread or make sure deadlocks are impossible

  protected:
    bool	AddFile(CKnownFile* pFile);
    void	AddFilesFromDirectory(const CString& rstrDirectory);
    void	FindSharedFiles();

    void	HashNextFile();
    bool	IsHashing(const CString& rstrDirectory, const CString& rstrName);
    void	RemoveFromHashing(CKnownFile* hashed);
    void	LoadSingleSharedFilesList();

    void	CheckAndAddSingleFile(const CFileFind& ff);
    bool	CheckAndAddSingleFile(const CString& rstrFilePath); // add specific files without editing sharing preferences

  private:
    CMap<CCKey,const CCKey&,CKnownFile*,CKnownFile*> m_Files_map;
    CMap<CSKey,const CSKey&, bool, bool>			 m_UnsharedFiles_map;
    CMapStringToString m_mapPseudoDirNames;
    CPublishKeywordList* m_keywords;
    CTypedPtrList<CPtrList, UnknownFile_Struct*> waitingforhash_list;
    CTypedPtrList<CPtrList, UnknownFile_Struct*> currentlyhashing_list;	// SLUGFILLER: SafeHash
    CSharedFilesCtrl*	output;
    CStringList			m_liSingleSharedFiles;
    CStringList			m_liSingleExcludedFiles;

    int m_currFileSrc;
    int m_currFileNotes;
    int m_currFileKey;
    UINT m_lastPublishKadSrc;
    UINT m_lastPublishKadNotes;
    bool bHaveSingleSharedFiles;

//>>> WiZaRd::Don't send empty dirs
  public:
    void GetUsefulDirectories(CStringList& dirList, const bool bForAFriend) const; //>>> WiZaRd::SharePermissions
//<<< WiZaRd::Don't send empty dirs
//>>> WiZaRd::HashingQueue Optimisation
  private:
    UnknownFile_Struct*	getNextFileToHash();
//<<< WiZaRd::HashingQueue Optimisation
//>>> Queued Count
  private:
    UINT	m_uiQueuedFilesCount;
  public:
    UINT	GetQueuedFilesCount() const	{return m_uiQueuedFilesCount;}
    void	IncQueuedFilesCount()		{++m_uiQueuedFilesCount;}
    void	DecQueuedFilesCount()		{--m_uiQueuedFilesCount;}
//<<< Queued Count
  private:
    time_t	m_nFileUpdateTime;
};

class CAddFileThread : public CWinThread
{
    DECLARE_DYNCREATE(CAddFileThread)
  protected:
    CAddFileThread();
  public:
    virtual BOOL InitInstance();
    virtual int	Run();
    void	SetValues(CSharedFileList* pOwner, LPCTSTR directory, LPCTSTR filename, LPCTSTR strSharedDir, CPartFile* partfile = NULL);

  private:
    CSharedFileList* m_pOwner;
    CString			 m_strDirectory;
    CString			 m_strFilename;
    CString			 m_strSharedDir;
    CPartFile*		 m_partfile;
};
