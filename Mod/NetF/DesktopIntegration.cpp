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
#include "DesktopIntegration.h"
#include "emule.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDesktopIntegration	theDesktop;

CDesktopIntegration::CDesktopIntegration()
{
	m_pWineNT2UnixPath = (t_wine_get_unix_file_name)::GetProcAddress(GetModuleHandle(L"KERNEL32"), "wine_get_unix_file_name");
	m_pWineUnix2NTPath = (t_wine_get_dos_file_name)::GetProcAddress(GetModuleHandle(L"KERNEL32"), "wine_get_dos_file_name");
	
	m_eDesktop = WINDOWS_DESKTOP;
	if (m_pWineNT2UnixPath != NULL && m_pWineUnix2NTPath != NULL)
	{
		TCHAR	lpBuffer[512];
		if (GetEnvironmentVariable(L"KDE_FULL_SESSION", lpBuffer, _countof(lpBuffer)) != 0)
		{
			theApp.QueueLogLineEx(LOG_WARNING, L"Detected KDE!");
			m_eDesktop = KDE_DESKTOP;
		}
		else if (GetEnvironmentVariable(L"GNOME_DESKTOP_SESSION_ID", lpBuffer, _countof(lpBuffer)) != 0)
		{
			theApp.QueueLogLineEx(LOG_WARNING, L"Detected GNOME!");
			m_eDesktop = GNOME_DESKTOP;
		}
	}
}

CDesktopIntegration::~CDesktopIntegration()
{
}

HINSTANCE CDesktopIntegration::ShellExecute(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	HINSTANCE	result = NULL;

	CString		strApp(lpFile);
	CString		strOptions = lpParameters;

	//if(thePrefs.WeNeedWineCompatibility())
	if(_tcscmp(lpOperation, L"open") == 0)
	{
		switch(_GetDesktopType())
		{
			case GNOME_DESKTOP:
			{
				//lpOperation = L"open";
				strApp.Format(L"\"%s\"", _NativeToWindowsPath("/usr/bin/gnome-open"));
				strOptions.Format(L"\"%s\"", _WindowsToNativePath(lpFile));
				break;
			}

			case KDE_DESKTOP:
			{
				//lpOperation = L"open";
				strApp.Format(L"\"%s\"", _NativeToWindowsPath("/usr/bin/kfmclient"));
				strOptions.Format(L"exec \"file:%s\"", _WindowsToNativePath(lpFile));
				break;
			}

			case WINDOWS_DESKTOP:
			default:
				break;
		}
	}
	if (strOptions.IsEmpty())
	{
		DebugLog(L"ShellExecute: %s %s", lpOperation, strApp);
		result = ::ShellExecute(hwnd, lpOperation, strApp.GetString(), NULL, lpDirectory, nShowCmd);
		//result = _SaferShellExecute(hwnd, lpOperation, strApp.GetString(), NULL, NULL, nShowCmd, SAFER_LEVELID_NORMALUSER);
	}
	else
	{
		DebugLog(L"ShellExecute: %s %s %s", lpOperation, strApp, strOptions);
		result = ::ShellExecute(hwnd, lpOperation, strApp.GetString(), strOptions.GetString(), NULL, nShowCmd);
		//result = _SaferShellExecute(hwnd, lpOperation, strApp.GetString(), strOptions.GetString(), lpDirectory, nShowCmd, SAFER_LEVELID_NORMALUSER);
	}

	return result;
}

CString CDesktopIntegration::_NativeToWindowsPath(CStringA path)
{
	CString ret = CString(path);
	if (m_pWineUnix2NTPath != NULL)
		ret = CString(m_pWineUnix2NTPath(path.GetString()));
	return ret;
}

CString CDesktopIntegration::_WindowsToNativePath(CStringW path)
{
	CString ret = CString(path);
	if (m_pWineNT2UnixPath != NULL)
		ret = CString(m_pWineNT2UnixPath(path));
	return ret;
}

/*HINSTANCE CDesktopIntegration::_SaferShellExecute(HWND hwnd, LPCTSTR lpOperation, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd, DWORD dwSaferLevelId)
{
	HINSTANCE hInstApp;

	SAFER_LEVEL_HANDLE hAuthzLevel = NULL;
	if (::SaferCreateLevel(SAFER_SCOPEID_USER, dwSaferLevelId, 0, &hAuthzLevel, NULL))
	{
			HANDLE hToken = NULL;
			if (::SaferComputeTokenFromLevel(hAuthzLevel, NULL, &hToken, 0, NULL)) 
			{
				SHELLEXECUTEINFO execInfo;
				ZeroMemory(&execInfo, sizeof(SHELLEXECUTEINFO));
				execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
				execInfo.fMask = 0;
				execInfo.hwnd = hwnd;
				execInfo.lpVerb = lpOperation;
				execInfo.lpFile = lpFile;
				execInfo.lpParameters = lpParameters;
				execInfo.lpDirectory = lpDirectory;
				execInfo.nShow = nShowCmd;
				execInfo.hInstApp = NULL;
				
				::SetThreadToken(NULL, hToken);
				::ShellExecuteEx(&execInfo);
				hInstApp = execInfo.hInstApp;
				::SetThreadToken(NULL, NULL);

				::AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, );
			}
			else
				hInstApp = reinterpret_cast<HINSTANCE>(::GetLastError());
			::SaferCloseLevel(hAuthzLevel);
	}
	else
		hInstApp = reinterpret_cast<HINSTANCE>(::GetLastError());
	return hInstApp;
}*/