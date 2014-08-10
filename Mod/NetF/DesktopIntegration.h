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
#include <windef.h>
#include <WinSafer.h>

extern "C"
{
    typedef	char* (FAR PASCAL *t_wine_get_unix_file_name)(const LPCWSTR dosW);
    typedef LPCWSTR(FAR PASCAL *t_wine_get_dos_file_name)(const char* dosA);
}

class CDesktopIntegration
{
  public:
    CDesktopIntegration();
    virtual ~CDesktopIntegration();

    HINSTANCE	ShellExecute(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);

  protected:
    enum EDesktop
    {
        WINDOWS_DESKTOP = 0,
        GNOME_DESKTOP,
        KDE_DESKTOP
    };
    EDesktop	_GetDesktopType() const
    {
        return m_eDesktop;
    }
    CString		_NativeToWindowsPath(CStringA path);
    CString		_WindowsToNativePath(CStringW path);

  private:
    EDesktop	m_eDesktop;
    t_wine_get_dos_file_name	m_pWineUnix2NTPath;
    t_wine_get_unix_file_name	m_pWineNT2UnixPath;
    // Safer ShellExecute
    //HINSTANCE _SaferShellExecute(HWND hwnd, LPCTSTR lpOperation, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd, DWORD dwSaferLevelId = SAFER_LEVELID_NORMALUSER);
};

extern CDesktopIntegration theDesktop;