//this file is part of kMule
//Copyright (C)2012 WiZaRd ( strEmail.Format("%s@%s", "thewizardofdos", "gmail.com") / http://www.emulefuture.de )
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

class CSevenZipThreadHandler;
class CSevenZipWorkerThread;

//////////////////////////////////////////////////////////////////////////
// Helper
// these functions implement the example console application that came along with 7z src
int ParseSevenZipCommandLine(LPCTSTR pszCommandLine);
int ExecuteSevenZipCommandLine(LPCTSTR pszCommandLine);

//////////////////////////////////////////////////////////////////////////
// CSevenZipWorkerThread
class CSevenZipWorkerThread : public CWinThread
{
	DECLARE_DYNCREATE(CSevenZipWorkerThread)
protected:
	CSevenZipWorkerThread();
public:
	virtual BOOL InitInstance();
	virtual int	Run();
	void	SetValues(CSevenZipThreadHandler* pOwner, const CString pszCommandLine);

private:	
	CSevenZipThreadHandler* m_pOwner;

	CString		m_strCommandLine;
};

//////////////////////////////////////////////////////////////////////////
// CSevenZipThreadHandler
class CSevenZipThreadHandler
{
	friend class CSevenZipWorkerThread;
public:
	CSevenZipThreadHandler();
	virtual ~CSevenZipThreadHandler();

	void	Init();
	void	UnInit();
	void	JobDone();
	bool	AbortThread();
	FILE*	GetCustomStream();

	void	ExtractArchive(const CString& strArchive, const CString& strTargetDir);
	bool	IsSevenZipAvailable() const;
	
private:
	void	AddJob(CSevenZipWorkerThread* thread);
	void	KillJobQueue();

	CCriticalSection JobLocker;
	CList<CSevenZipWorkerThread*>	m_JobList;
	bool	m_bAbort;
	CMutex	m_mutSync;
	CSevenZipWorkerThread*	m_Thread;
	FILE*	m_pFileHandle;
};

extern CSevenZipThreadHandler m_SevenZipThreadHandler;