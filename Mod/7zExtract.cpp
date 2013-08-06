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
#include "7zExtract.h"
#include <SevenZip++/7zpp.h>
#include "emule.h"
#include "emuleDlg.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool ExtractSevenZipArchive(const CString& strArchiveName, const CString& strDestination)
{
    CString msg = L"";
    try
    {
        SevenZip::SevenZipLibrary lib;
        lib.Load();

        const TCHAR* archiveName = strArchiveName;
        const TCHAR* destination = strDestination;
        SevenZip::SevenZipExtractor extractor(lib, archiveName);
        extractor.ExtractArchive(destination);
        return true;
    }
    catch (SevenZip::SevenZipException& ex)
    {
		CString error = ex.GetMessage().c_str();
        msg.Format(L"Failed to extract archive \"%s\" to \"%s\" - %s", strArchiveName, strDestination, error);
    }
    catch (...)
    {
        msg.Format(L"Failed to extract archive \"%s\" to \"%s\" - unknown error", strArchiveName, strDestination);
    }

    theApp.QueueLogLineEx(LOG_ERROR, msg);
    if (theApp.emuledlg)
        theApp.emuledlg->ShowNotifier(msg, TBN_IMPORTANTEVENT);

    return false;

}