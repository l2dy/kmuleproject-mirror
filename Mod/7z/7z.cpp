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
#include "stdafx.h"
#include "7z.h"
#include "7z_include.h"
#include "emule.h"
#include "Log.h"
#include "Preferences.h"
#include <share.h>
#include "otherfunctions.h"
#include "emuleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
// Helper
// from Main.cpp:
HINSTANCE g_hInstance = 0;

static void ShowMessageAndThrowException(LPCTSTR message, NExitCode::EEnum code)
{
    theApp.QueueLogLineEx(LOG_WARNING, L"7z: %s", message);
    throw code;
}

static void PrintHelpAndExit()
{
    theApp.QueueLogLineEx(LOG_WARNING, L"7z: %s", kHelpString);
    ShowMessageAndThrowException(kUserErrorMessage, NExitCode::kUserError);
}

#ifndef _WIN32
static void GetArguments(int numArgs, const char *args[], UStringVector &parts)
{
    parts.Clear();
    for (int i = 0; i < numArgs; i++)
    {
        UString s = MultiByteToUnicodeString(args[i]);
        parts.Add(s);
    }
}
#endif

static void ShowHelp()
{
    theApp.QueueLogLineEx(LOG_INFO, L"7z: %s", kHelpString);
}

#ifdef EXTERNAL_CODECS
static CString GetPaddedString(const AString &s, int size)
{
    CString ret = L"";

    int len = s.Length();
    CString pad = L"";
    for (int i = len; i < size; i++)
        pad.AppendChar(L' ');
    ret.Format(L"%hs%s", s, pad);

    return ret;
}
#endif

static CString GetPaddedString(const UString &s, int size)
{
    CString ret = L"";

    int len = s.Length();
    CString pad = L"";
    for (int i = len; i < size; i++)
        pad.AppendChar(L' ');
    ret.Format(L"%s%s", s, pad);

    return ret;
}

static inline char GetHex(Byte value)
{
    return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10)));
}

//////////////////////////////////////////////////////////////////////////
// CSevenZipWorkerThread
typedef CSevenZipWorkerThread CSevenZipWorkerThread;
IMPLEMENT_DYNCREATE(CSevenZipWorkerThread, CWinThread)

CSevenZipWorkerThread::CSevenZipWorkerThread()
{
    m_pOwner = NULL;
    m_strCommandLine = L"";
}

void CSevenZipWorkerThread::SetValues(CSevenZipThreadHandler* pOwner, const CString strCommandLine)
{
    m_pOwner = pOwner;
    m_strCommandLine = strCommandLine;
}


BOOL CSevenZipWorkerThread::InitInstance()
{
    InitThreadLocale();
    return TRUE;
}

int CSevenZipWorkerThread::Run()
{
    DbgSetThreadName("CSevenZipWorkerThread");
    if (m_pOwner == NULL)
        return 0;

    theApp.QueueLogLineEx(LOG_STATUSBAR, L"7z: Job started...");

    //	ASSERT( m_pOwner->m_bDataLoaded == false );
    CSingleLock sLock(&m_pOwner->m_mutSync);
    sLock.Lock();

    int iRet = 0;
    try
    {
        iRet = ParseSevenZipCommandLine(m_strCommandLine);
    }
    catch (...)
    {
        theApp.QueueDebugLogLineEx(LOG_ERROR, L"Exception in CSevenZipWorkerThread::Run");
        ASSERT(0);
    }

    if (iRet == 0)
        theApp.QueueLogLineEx(LOG_STATUSBAR|LOG_SUCCESS, L"7z: Job done!");
    else
        theApp.QueueLogLineEx(LOG_STATUSBAR|LOG_ERROR, L"7z: Job failed!");

    if (theApp.emuledlg && theApp.emuledlg->IsRunning())
    {
        if (iRet == 0)
            VERIFY(PostMessage(theApp.emuledlg->m_hWnd, TM_SEVENZIP_JOB_DONE, (WPARAM)0, (LPARAM)0));
        else
            VERIFY(PostMessage(theApp.emuledlg->m_hWnd, TM_SEVENZIP_JOB_FAILED, (WPARAM)0, (LPARAM)0));
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
// CSevenZipThreadHandler
CSevenZipThreadHandler m_SevenZipThreadHandler;
CSevenZipThreadHandler::CSevenZipThreadHandler()
{
    m_bAbort = false;
    m_Thread = NULL;
    m_pFileHandle = NULL;
}

CSevenZipThreadHandler::~CSevenZipThreadHandler()
{
    if (m_pFileHandle)
        fclose(m_pFileHandle);
}

void CSevenZipThreadHandler::Init()
{
    if (m_pFileHandle == NULL)
        m_pFileHandle = _tfsopen(thePrefs.GetMuleDirectory(EMULE_LOGDIR) + L"7z.log", L"a+b", _SH_DENYWR);
    //theApp.QueueLogLineEx(LOG_INFO, L"7z: # CPUs: %I64u", (UInt64)NWindows::NSystem::GetNumberOfProcessors());
}

void CSevenZipThreadHandler::UnInit()
{
    KillJobQueue();
    AbortThread();
}

bool	CSevenZipThreadHandler::IsSevenZipAvailable() const
{
    DWORD dwVersion = 0;
    HINSTANCE hinstDll = LoadLibrary(L"7z.dll");
    if (hinstDll)
    {
        // TODO: check for a valid .dll version?
        dwVersion = 1;
        FreeLibrary(hinstDll);
    }
    return dwVersion != 0;
}

FILE*	CSevenZipThreadHandler::GetCustomStream()
{
    return m_pFileHandle;
}

void	CSevenZipThreadHandler::AddJob(CSevenZipWorkerThread* thread)
{
    if (m_Thread)
    {
//		JobLocker.Lock();
        m_JobList.AddTail(thread); // TODO: dup job check!
//		JobLocker.Unlock();
    }
    else
    {
        //start thread:
        m_bAbort = false;
        m_Thread = thread;
        m_Thread->ResumeThread();
    }
}

void	CSevenZipThreadHandler::JobDone()
{
    m_Thread = NULL;
    //	JobLocker.Lock();
    if (!m_JobList.IsEmpty())
        m_Thread = m_JobList.RemoveHead();
    //	JobLocker.Unlock();
    if (m_Thread)
    {
        //start thread:
        m_bAbort = false;
        m_Thread->ResumeThread();
    }
    // TODO: reload shared files?
}

void	CSevenZipThreadHandler::KillJobQueue()
{
    //	JobLocker.Lock();
    while (!m_JobList.IsEmpty())
        delete m_JobList.RemoveHead();
    //	JobLocker.Unlock();
}

bool	CSevenZipThreadHandler::AbortThread()
{
    //wait for thread:
    m_bAbort = true;
    CSingleLock sLock(&m_mutSync);
    sLock.Lock(); // wait

    return true;
}

void CSevenZipThreadHandler::ExtractArchive(const CString& strArchive, const CString& strTargetDir)
{
    CString strCommandLine = L"";
    strCommandLine.Format(L"%s.exe x \"%s\" -o\"%s\"", theApp.m_pszExeName, strArchive, strTargetDir);

    CSevenZipWorkerThread* thread = (CSevenZipWorkerThread*)AfxBeginThread(RUNTIME_CLASS(CSevenZipWorkerThread), THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
    thread->SetValues(this, strCommandLine);

    AddJob(thread);
}

void CSevenZipThreadHandler::ExtractFileFromArchive(const CString& strFilename, const CString& strArchive, const CString& strTargetDir)
{
	CString strCommandLine = L"";
	strCommandLine.Format(L"%s.exe e \"%s\"  -o\"%s\" -ir!\"%s\"", theApp.m_pszExeName, strArchive, strTargetDir, strFilename);

	CSevenZipWorkerThread* thread = (CSevenZipWorkerThread*)AfxBeginThread(RUNTIME_CLASS(CSevenZipWorkerThread), THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
	thread->SetValues(this, strCommandLine);

	AddJob(thread);
}

// from MainAr.cpp:
int ParseSevenZipCommandLine(LPCTSTR pszCommandLine)
{
    //NConsoleClose::CCtrlHandlerSetter ctrlHandlerSetter;
    int res = 0;
    try
    {
        res = ExecuteSevenZipCommandLine(pszCommandLine);
    }
    catch (const CNewException &)
    {
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s", kMemoryExceptionMessage);
        return (NExitCode::kMemoryError);
    }
    catch (const NConsoleClose::CCtrlBreakException &)
    {
        theApp.QueueLogLineEx(LOG_WARNING, L"7z: %s", kUserBreak);
        return (NExitCode::kUserBreak);
    }
    catch (const CArchiveCommandLineException &e)
    {
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s%hs", kExceptionErrorMessage, e);
        return (NExitCode::kUserError);
    }
    catch (const CSystemException &systemError)
    {
        if (systemError.ErrorCode == E_OUTOFMEMORY)
        {
            theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s", kMemoryExceptionMessage);
            return (NExitCode::kMemoryError);
        }
        if (systemError.ErrorCode == E_ABORT)
        {
            theApp.QueueLogLineEx(LOG_WARNING, L"7z: %s", kUserBreak);
            return (NExitCode::kUserBreak);
        }
        UString message;
        NError::MyFormatMessage(systemError.ErrorCode, message);
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: System error: %s", message);
        return (NExitCode::kFatalError);
    }
    catch (NExitCode::EEnum &exitCode)
    {
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s%i", kInternalExceptionMessage, exitCode);
        return (exitCode);
    }
    /*
    catch(const NExitCode::CMultipleErrors &multipleErrors)
    {
    	theApp.QueueLogLineEx(LOG_ERROR, L"7z: %i errors", errors);
    	return (NExitCode::kFatalError);
    }
    */
    catch (const UString &s)
    {
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s%s", kExceptionErrorMessage, s);
        return (NExitCode::kFatalError);
    }
    catch (const AString &s)
    {
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s%hs", kExceptionErrorMessage, s);
        return (NExitCode::kFatalError);
    }
    catch (const char *s)
    {
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s%hs", kExceptionErrorMessage, s);
        return (NExitCode::kFatalError);
    }
    catch (int t)
    {
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s%i", kInternalExceptionMessage, t);
        return (NExitCode::kFatalError);
    }
    catch (...)
    {
        theApp.QueueLogLineEx(LOG_ERROR, L"7z: %s", kUnknownExceptionMessage);
        return (NExitCode::kFatalError);
    }
    return  res;
    /*
    va_list args;
    va_start(args, numProperties);
    while (numProperties-- > 0)
    {
    	//CString arg = va_arg(args, CString);
    }
    va_end(args);

    return 0;
    */
}

// from Main.cpp:
int ExecuteSevenZipCommandLine(LPCTSTR pszCommandLine)
{
#if defined(_WIN32) && !defined(UNDER_CE)
    SetFileApisToOEM();
#endif

    UStringVector commandStrings;
#ifdef _WIN32
    NCommandLineParser::SplitCommandLine(pszCommandLine, commandStrings);
#else
    GetArguments(numArgs, args, commandStrings);
#endif

    if (commandStrings.Size() == 1)
    {
        ShowHelp();
        return 0;
    }
    commandStrings.Delete(0);

    CArchiveCommandLineOptions options;
    CArchiveCommandLineParser parser;

    parser.Parse1(commandStrings, options);

    if (options.HelpMode)
    {
        ShowHelp();
        return 0;
    }

#if defined(_WIN32) && defined(_7ZIP_LARGE_PAGES)
    if (options.LargePages)
    {
        SetLargePageSize();
        NSecurity::EnableLockMemoryPrivilege();
    }
#endif

    CStdOutStream  g_CustStream(m_SevenZipThreadHandler.GetCustomStream());
    CStdOutStream& stdStream = g_CustStream;
    //CStdOutStream &stdStream = options.StdOutMode ? g_StdErr : g_StdOut;
    //g_StdStream = &stdStream;

    parser.Parse2(options);

    CCodecs *codecs = new CCodecs;
    CMyComPtr<
#ifdef EXTERNAL_CODECS
    ICompressCodecsInfo
#else
    IUnknown
#endif
    > compressCodecsInfo = codecs;
    HRESULT result = codecs->Load();
    if (result != S_OK)
        throw CSystemException(result);

    bool isExtractGroupCommand = options.Command.IsFromExtractGroup();

    if (codecs->Formats.Size() == 0 &&
            (isExtractGroupCommand ||
             options.Command.CommandType == NCommandType::kList ||
             options.Command.IsFromUpdateGroup()))
        throw kNoFormats;

    CIntVector formatIndices;
    if (!codecs->FindFormatForArchiveType(options.ArcType, formatIndices))
        throw kUnsupportedArcTypeMessage;

    if (options.Command.CommandType == NCommandType::kInfo)
    {
        theApp.QueueLogLineEx(LOG_INFO, L"7z: Formats:");
        int i;
        CString print = L"7z: ";
        for (i = 0; i < codecs->Formats.Size(); i++)
        {
            const CArcInfoEx &arc = codecs->Formats[i];
#ifdef EXTERNAL_CODECS
            if (arc.LibIndex >= 0)
            {
                char s[16];
                ConvertUInt32ToString(arc.LibIndex, s);
                print.Append(GetPaddedString(s, 2));
            }
            else
#endif
                print.Append(L"  ");
            print.AppendFormat(L" %c%c  ", arc.UpdateEnabled ? L'C' : L' ', arc.KeepName ? L'K' : L' ');
            print.Append(GetPaddedString(arc.Name, 6));
            print.Append(L"  ");
            UString s;
            for (int t = 0; t < arc.Exts.Size(); t++)
            {
                const CArcExtInfo &ext = arc.Exts[t];
                s += ext.Ext;
                if (!ext.AddExt.IsEmpty())
                {
                    s += L" (";
                    s += ext.AddExt;
                    s += L')';
                }
                s += L' ';
            }
            print.Append(GetPaddedString(s, 14));
            print.Append(L"  ");
            const CByteBuffer &sig = arc.StartSignature;
            for (size_t j = 0; j < sig.GetCapacity(); j++)
            {
                Byte b = sig[j];
                if (b > 0x20 && b < 0x80)
                    print.AppendChar((TCHAR)b);
                else
                {
                    print.AppendChar(GetHex((Byte)((b >> 4) & 0xF)));
                    print.AppendChar(GetHex((Byte)(b & 0xF)));
                }
                print.Append(L" ");
            }
            theApp.QueueLogLineEx(LOG_INFO, L"7z: %s", print);
        }
        theApp.QueueLogLineEx(LOG_INFO, L"7z: Codecs:");

#ifdef EXTERNAL_CODECS
        UInt32 numMethods;
        if (codecs->GetNumberOfMethods(&numMethods) == S_OK)
        {
            CString print = L"7z: ";
            for (UInt32 j = 0; j < numMethods; j++)
            {
                int libIndex = codecs->GetCodecLibIndex(j);
                if (libIndex >= 0)
                {
                    char s[16];
                    ConvertUInt32ToString(libIndex, s);
                    print.Append(GetPaddedString(s, 2));
                }
                else
                    print.Append(L"  ");
                print.AppendFormat(L" %c  ", codecs->GetCodecEncoderIsAssigned(j) ? L'C' : L' ');
                UInt64 id;
                HRESULT res = codecs->GetCodecId(j, id);
                if (res != S_OK)
                    id = (UInt64)(Int64)-1;
                char s[32];
                ConvertUInt64ToString(id, s, 16);
                print.Append(GetPaddedString(s, 8));
                print.Append(L"  ");
                print.Append(GetPaddedString(codecs->GetCodecName(j), 11));
                theApp.QueueLogLineEx(LOG_INFO, L"7z: %s", print);
                /*
                if (res != S_OK)
                	throw "incorrect Codec ID";
                */
            }
        }
#endif
        return S_OK;
    }
    else if (options.Command.CommandType == NCommandType::kBenchmark)
    {
        if (options.Method.CompareNoCase(L"CRC") == 0)
        {
            HRESULT res = CrcBenchCon((FILE *)stdStream, options.NumIterations, options.NumThreads, options.DictionarySize);
            if (res != S_OK)
            {
                if (res == S_FALSE)
                {
                    theApp.QueueLogLineEx(LOG_ERROR, L"7z: CRC Error");
                    return NExitCode::kFatalError;
                }
                throw CSystemException(res);
            }
        }
        else
        {
            HRESULT res;
#ifdef EXTERNAL_CODECS
            CObjectVector<CCodecInfoEx> externalCodecs;
            res = LoadExternalCodecs(compressCodecsInfo, externalCodecs);
            if (res != S_OK)
                throw CSystemException(res);
#endif
            res = LzmaBenchCon(
#ifdef EXTERNAL_CODECS
                      compressCodecsInfo, &externalCodecs,
#endif
                      (FILE *)stdStream, options.NumIterations, options.NumThreads, options.DictionarySize);
            if (res != S_OK)
            {
                if (res == S_FALSE)
                {
                    theApp.QueueLogLineEx(LOG_ERROR, L"7z: Decoding Error");
                    return NExitCode::kFatalError;
                }
                throw CSystemException(res);
            }
        }
    }
    else if (isExtractGroupCommand || options.Command.CommandType == NCommandType::kList)
    {
        if (isExtractGroupCommand)
        {
            CExtractCallbackConsole *ecs = new CExtractCallbackConsole;
            CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

            ecs->OutStream = &stdStream;

#ifndef _NO_CRYPTO
            ecs->PasswordIsDefined = options.PasswordEnabled;
            ecs->Password = options.Password;
#endif

            ecs->Init();

            COpenCallbackConsole openCallback;
            openCallback.OutStream = &stdStream;

#ifndef _NO_CRYPTO
            openCallback.PasswordIsDefined = options.PasswordEnabled;
            openCallback.Password = options.Password;
#endif

            CExtractOptions eo;
            eo.StdInMode = options.StdInMode;
            eo.StdOutMode = options.StdOutMode;
            eo.PathMode = options.Command.GetPathMode();
            eo.TestMode = options.Command.IsTestMode();
            eo.OverwriteMode = options.OverwriteMode;
            eo.OutputDir = options.OutputDir;
            eo.YesToAll = options.YesToAll;
            eo.CalcCrc = options.CalcCrc;
#if !defined(_7ZIP_ST) && !defined(_SFX)
            eo.Properties = options.ExtractProperties;
#endif
            UString errorMessage;
            CDecompressStat stat;
            HRESULT result = DecompressArchives(
                                 codecs,
                                 formatIndices,
                                 options.ArchivePathsSorted,
                                 options.ArchivePathsFullSorted,
                                 options.WildcardCensor.Pairs.Front().Head,
                                 eo, &openCallback, ecs, errorMessage, stat);
            if (!errorMessage.IsEmpty())
            {
                theApp.QueueLogLineEx(LOG_ERROR, L"7z: Error: %s", errorMessage);
                if (result == S_OK)
                    result = E_FAIL;
            }

            if (ecs->NumArchives > 1)
                theApp.QueueLogLineEx(LOG_INFO, L"7zip: Archives: %I64u", ecs->NumArchives);
            if (ecs->NumArchiveErrors != 0 || ecs->NumFileErrors != 0)
            {
                if (ecs->NumArchives > 1)
                {
                    if (ecs->NumArchiveErrors != 0)
                        theApp.QueueLogLineEx(LOG_ERROR, L"7zip: Archive Errors: %I64u", ecs->NumArchiveErrors);
                    if (ecs->NumFileErrors != 0)
                        theApp.QueueLogLineEx(LOG_ERROR, L"7zip: Sub items Errors: %I64u", ecs->NumFileErrors);
                }
                if (result != S_OK)
                    throw CSystemException(result);
                return NExitCode::kFatalError;
            }
            if (result != S_OK)
                throw CSystemException(result);
            if (stat.NumFolders != 0)
                theApp.QueueLogLineEx(LOG_INFO, L"7zip: Folders: %I64u", stat.NumFolders);
            if (stat.NumFiles != 1 || stat.NumFolders != 0)
                theApp.QueueLogLineEx(LOG_INFO, L"7zip: Files: %I64u", stat.NumFiles);
            theApp.QueueLogLineEx(LOG_INFO, L"7zip: Size: %I64u", stat.UnpackSize);
            theApp.QueueLogLineEx(LOG_INFO, L"7zip: Compressed: %I64u", stat.PackSize);
            if (options.CalcCrc)
            {
                char s[16];
                ConvertUInt32ToHexWithZeros(stat.CrcSum, s);
                theApp.QueueLogLineEx(LOG_INFO, L"7zip: CRC: %hs", s);
            }
        }
        else
        {
            UInt64 numErrors = 0;
            HRESULT result = ListArchives(
                                 codecs,
                                 formatIndices,
                                 options.StdInMode,
                                 options.ArchivePathsSorted,
                                 options.ArchivePathsFullSorted,
                                 options.WildcardCensor.Pairs.Front().Head,
                                 options.EnableHeaders,
                                 options.TechMode,
#ifndef _NO_CRYPTO
                                 options.PasswordEnabled,
                                 options.Password,
#endif
                                 numErrors);
            if (numErrors > 0)
            {
                theApp.QueueLogLineEx(LOG_INFO, L"7zip: Errors: %I64u", numErrors);
                return NExitCode::kFatalError;
            }
            if (result != S_OK)
                throw CSystemException(result);
        }
    }
    else if (options.Command.IsFromUpdateGroup())
    {
        CUpdateOptions &uo = options.UpdateOptions;
        if (uo.SfxMode && uo.SfxModule.IsEmpty())
            uo.SfxModule = kDefaultSfxModule;

        COpenCallbackConsole openCallback;
        openCallback.OutStream = &stdStream;

#ifndef _NO_CRYPTO
        bool passwordIsDefined =
            options.PasswordEnabled && !options.Password.IsEmpty();
        openCallback.PasswordIsDefined = passwordIsDefined;
        openCallback.Password = options.Password;
#endif

        CUpdateCallbackConsole callback;
        callback.EnablePercents = options.EnablePercents;

#ifndef _NO_CRYPTO
        callback.PasswordIsDefined = passwordIsDefined;
        callback.AskPassword = options.PasswordEnabled && options.Password.IsEmpty();
        callback.Password = options.Password;
#endif
        callback.StdOutMode = uo.StdOutMode;
        callback.Init(&stdStream);

        CUpdateErrorInfo errorInfo;

        if (!uo.Init(codecs, formatIndices, options.ArchiveName))
            throw kUnsupportedArcTypeMessage;
        HRESULT result = UpdateArchive(codecs,
                                       options.WildcardCensor, uo,
                                       errorInfo, &openCallback, &callback);

        int exitCode = NExitCode::kSuccess;
        if (callback.CantFindFiles.Size() > 0)
        {
            theApp.QueueLogLineEx(LOG_WARNING, L"7z: WARNINGS for files:\n");
            int numErrors = callback.CantFindFiles.Size();
            CString print = L"7z: ";
            for (int i = 0; i < numErrors; i++)
                print.AppendFormat(L"%s : %s\n", callback.CantFindFiles[i], NError::MyFormatMessageW(callback.CantFindCodes[i]));
            print.Append(L"----------------\n");
            print.AppendFormat(L"WARNING: Cannot find %i file%c", numErrors, numErrors > 1 ? L's' : L'');
            theApp.QueueLogLineEx(LOG_INFO, L"%s", print);
            exitCode = NExitCode::kWarning;
        }

        if (result != S_OK)
        {
            UString message;
            if (!errorInfo.Message.IsEmpty())
            {
                message += errorInfo.Message;
                message += L"\n";
            }
            if (!errorInfo.FileName.IsEmpty())
            {
                message += errorInfo.FileName;
                message += L"\n";
            }
            if (!errorInfo.FileName2.IsEmpty())
            {
                message += errorInfo.FileName2;
                message += L"\n";
            }
            if (errorInfo.SystemError != 0)
            {
                message += NError::MyFormatMessageW(errorInfo.SystemError);
                message += L"\n";
            }
            if (!message.IsEmpty())
                theApp.QueueLogLineEx(LOG_ERROR, L"7z: Error:\n%s", message);
            throw CSystemException(result);
        }
        int numErrors = callback.FailedFiles.Size();
        if (numErrors == 0)
        {
            if (callback.CantFindFiles.Size() == 0)
                theApp.QueueDebugLogLineEx(LOG_SUCCESS, L"7z: %s", kEverythingIsOk);
        }
        else
        {
            theApp.QueueLogLineEx(LOG_WARNING, L"7z: WARNINGS for files:\n");
            CString print = L"7z: ";
            for (int i = 0; i < numErrors; i++)
                print.AppendFormat(L"%s : %s\n", callback.FailedFiles[i], NError::MyFormatMessageW(callback.FailedCodes[i]));
            print.Append(L"----------------\n");
            print.AppendFormat(L"WARNING: Cannot open %i file%c", numErrors, numErrors > 1 ? L's' : L'');
            theApp.QueueLogLineEx(LOG_INFO, L"%s", print);
            exitCode = NExitCode::kWarning;
        }
        return exitCode;
    }
    else
        PrintHelpAndExit();
    return 0;
}