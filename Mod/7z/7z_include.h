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
// from stdafx.h
#include "Common/MyWindows.h"
#include "Common/NewHandler.h"

// from Main.cpp && MainAr.cpp:
#if defined( _WIN32) && defined( _7ZIP_LARGE_PAGES)
#include "../../C/Alloc.h"
#endif
#include "../../MyVersion.h"

#include "Common/MyInitGuid.h"
#include "Common/CommandLineParser.h"
#include "Common/IntToString.h"
#include "Common/MyException.h"
#include "Common/StdOutStream.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/Error.h"
#include "Windows/NtCheck.h"
#ifdef _WIN32
#include "Windows/MemoryLock.h"
#endif

#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"
#include "../Common/Extract.h"
#ifdef EXTERNAL_CODECS
#include "../Common/LoadCodecs.h"
#endif
#include "Common/MyException.h"

#include "BenchCon.h"
#include "ExtractCallbackConsole.h"
#include "List.h"
#include "OpenCallbackConsole.h"
#include "UpdateCallbackConsole.h"
#include "ConsoleClose.h"

// from Main.cpp:
HINSTANCE g_hInstance = 0;

static LPCTSTR kDefaultSfxModule = L"7zCon.sfx";
static LPCTSTR kCopyrightString = L"7-Zip"
#ifndef EXTERNAL_CODECS
	L" (A)"
#endif

#ifdef _WIN64
	L" [64]"
#endif
	L" " MY_VERSION_COPYRIGHT_DATE;

static LPCTSTR kHelpString =
	L"\nUsage: 7z"
#ifdef _NO_CRYPTO
	L"r"
#else
#ifndef EXTERNAL_CODECS
	L"a"
#endif
#endif
	L" <command> [<switches>...] <archive_name> [<file_names>...]\n"
	L"       [<@listfiles...>]\n"
	L"\n"
	L"<Commands>\n"
	L"  a: Add files to archive\n"
	L"  b: Benchmark\n"
	L"  d: Delete files from archive\n"
	L"  e: Extract files from archive (without using directory names)\n"
	L"  l: List contents of archive\n"
	//    L"  l[a|t][f]: List contents of archive\n"
	//    L"    a - with Additional fields\n"
	//    L"    t - with all fields\n"
	//    L"    f - with Full pathnames\n"
	L"  t: Test integrity of archive\n"
	L"  u: Update files to archive\n"
	L"  x: eXtract files with full paths\n"
	L"<Switches>\n"
	L"  -ai[r[-|0]]{@listfile|!wildcard}: Include archives\n"
	L"  -ax[r[-|0]]{@listfile|!wildcard}: eXclude archives\n"
	L"  -bd: Disable percentage indicator\n"
	L"  -i[r[-|0]]{@listfile|!wildcard}: Include filenames\n"
	L"  -m{Parameters}: set compression Method\n"
	L"  -o{Directory}: set Output directory\n"
#ifndef _NO_CRYPTO
	L"  -p{Password}: set Password\n"
#endif
	L"  -r[-|0]: Recurse subdirectories\n"
	L"  -scs{UTF-8 | WIN | DOS}: set charset for list files\n"
	L"  -sfx[{name}]: Create SFX archive\n"
	L"  -si[{name}]: read data from stdin\n"
	L"  -slt: show technical information for l (List) command\n"
	L"  -so: write data to stdout\n"
	L"  -ssc[-]: set sensitive case mode\n"
	L"  -ssw: compress shared files\n"
	L"  -t{Type}: Set type of archive\n"
	L"  -u[-][p#][q#][r#][x#][y#][z#][!newArchiveName]: Update options\n"
	L"  -v{Size}[b|k|m|g]: Create volumes\n"
	L"  -w[{path}]: assign Work directory. Empty path means a temporary directory\n"
	L"  -x[r[-|0]]]{@listfile|!wildcard}: eXclude filenames\n"
	L"  -y: assume Yes on all queries\n";

// exception messages
static LPCTSTR kEverythingIsOk		= L"Everything is OK";
static LPCTSTR kUserErrorMessage	= L"Incorrect command line";
static LPCTSTR kNoFormats			= L"7-Zip cannot find the code that works with archives.";
static LPCTSTR kUnsupportedArcTypeMessage	= L"Unsupported archive type";

// from MainAr.cpp:
static LPCTSTR kExceptionErrorMessage	= L"Error: ";
static LPCTSTR kUserBreak				= L"Break signaled";
static LPCTSTR kMemoryExceptionMessage	= L"ERROR: Can't allocate required memory!";
static LPCTSTR kUnknownExceptionMessage = L"Unknown Error";
static LPCTSTR kInternalExceptionMessage	= L"Internal Error #";

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;