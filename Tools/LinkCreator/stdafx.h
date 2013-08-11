// stdafx.h : Includedatei für Standardsystem-Includedateien,
// oder häufig verwendete, projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.

#pragma once
#include "types.h"

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Selten verwendete Teile der Windows-Header nicht einbinden
#endif

#include "emule_site_config.h"

// MSDN: Using the Windows Headers
// ===========================================================
//Windows Vista			_WIN32_WINNT>=0x0600	WINVER>=0x0600
//Windows Server 2003	_WIN32_WINNT>=0x0502    WINVER>=0x0502
//Windows XP			_WIN32_WINNT>=0x0501	WINVER>=0x0501
//Windows 2000			_WIN32_WINNT>=0x0500    WINVER>=0x0500
//Windows NT 4.0		_WIN32_WINNT>=0x0400	WINVER>=0x0400
//Windows Me			_WIN32_WINDOWS=0x0500	WINVER>=0x0500
//Windows 98			_WIN32_WINDOWS>=0x0410	WINVER>=0x0410
//Windows 95			_WIN32_WINDOWS>=0x0400	WINVER>=0x0400
//
//IE 7.0				_WIN32_IE>=0x0700
//IE 6.0 SP2			_WIN32_IE>=0x0603
//IE 6.0 SP1			_WIN32_IE>=0x0601
//IE 6.0				_WIN32_IE>=0x0600
//IE 5.5				_WIN32_IE>=0x0550
//IE 5.01				_WIN32_IE>=0x0501
//IE 5.0, 5.0a, 5.0b	_WIN32_IE>=0x0500
//IE 4.01				_WIN32_IE>=0x0401
//IE 4.0				_WIN32_IE>=0x0400
//IE 3.0, 3.01, 3.02	_WIN32_IE>=0x0300

#if defined(HAVE_VISTA_SDK)

#ifndef WINVER
#define WINVER 0x0502			// 0x0502 == Windows Server 2003, Windows XP (same as VS2005-MFC)
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER		// same as VS2005-MFC
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410	// 0x0410 == Windows 98
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0560		// 0x0560 == Internet Explorer 5.6 -> Comctl32.dll v5.8
#endif

#else//HAVE_VISTA_SDK

#ifndef WINVER
#define WINVER 0x0400			// 0x0400 == Windows 98 and Windows NT 4.0 (because of '_WIN32_WINDOWS=0x0410')
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400		// 0x0400 == Windows NT 4.0
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410	// 0x0410 == Windows 98
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0560		// 0x0560 == Internet Explorer 5.6 -> Comctl32.dll v5.8 (same as MFC internally used)
#endif

#endif//HAVE_VISTA_SDK

#if _MSC_VER>=1400

// _CRT_SECURE_NO_DEPRECATE - Disable all warnings for not using "_s" functions.
//
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

// _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES - Overloads all standard string functions (e.g. strcpy) with "_s" functions
// if, and only if, the size of the output buffer is known at compile time (so, if it is a static array). If there is
// a buffer overflow during runtime, it will throw an exception.
//
#ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif

// _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT - This is a cool CRT feature but does not make sense for our code.
// With our existing code we could get exceptions which are though not justifiable because we explicitly
// terminate all our string buffers. This define could be enabled for debug builds though.
//
//#ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT
//#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 1
//#endif

#if !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES) || (_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES==0)
#ifndef _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_NON_CONFORMING_SWPRINTFS
#endif
#endif//!defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES) || (_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES==0)

#ifndef _USE_32BIT_TIME_T
#define _USE_32BIT_TIME_T
#endif

#endif//_MSC_VER>=1400

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// einige CString-Konstruktoren sind explizit

// Deaktiviert das Ausblenden von einigen häufigen und oft ignorierten Warnungen
#define _AFX_ALL_WARNINGS

#include <afxwin.h>	// MFC core and standard components
#include <afxext.h>	// MFC extensions
#include <afxdlgs.h>	// MFC Standard dialogs
#include <shlwapi.h>

#include <afxdtctl.h>		// MFC-Unterstützung für allgemeine Steuerelemente von Internet Explorer 4
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC-Unterstützung für allgemeine Windows-Steuerelemente
#endif // _AFX_NO_AFXCMN_SUPPORT

#define ARRSIZE(x)	(sizeof(x)/sizeof(x[0]))
