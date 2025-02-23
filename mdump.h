#pragma once

struct _EXCEPTION_POINTERS;

class CMiniDumper
{
  public:
    static void Enable(LPCTSTR pszAppName, bool bShowErrors, LPCTSTR pszDumpDir);

  private:
    static TCHAR m_szAppName[MAX_PATH];
    static TCHAR m_szDumpDir[MAX_PATH];

    static HMODULE GetDebugHelperDll(FARPROC* ppfnMiniDumpWriteDump, bool bShowErrors);
    static LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS* pExceptionInfo);
    static void InvalidParameterHandler(const wchar_t *pszExpression, const wchar_t *pszFunction, const wchar_t *pszFile, unsigned int nLine, uintptr_t pReserved);
};

extern CMiniDumper theCrashDumper;
