#pragma once
#include "PPgGeneral.h"
#include "PPgConnection.h"
#include "PPgDirectories.h"
#include "PPgFiles.h"
#include "PPgStats.h"
#include "PPgTweaks.h"
#include "PPgDisplay.h"
#include "PPgSecurity.h"
#include "PPgProxy.h"
#include "./Mod/PPgAbout.h" //>>> WiZaRd::AboutPage
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
#include "PPgDebug.h"
#endif
#include "otherfunctions.h"
#include "TreePropSheet.h"

class CPreferencesDlg : public CTreePropSheet
{
    DECLARE_DYNAMIC(CPreferencesDlg)

public:
    CPreferencesDlg();
    virtual ~CPreferencesDlg();

    CPPgGeneral		m_wndGeneral;
    CPPgConnection	m_wndConnection;
    CPPgDirectories	m_wndDirectories;
    CPPgFiles		m_wndFiles;
    CPPgStats		m_wndStats;
    CPPgTweaks		m_wndTweaks;
    CPPgDisplay		m_wndDisplay;
    CPPgSecurity	m_wndSecurity;
    CPPgProxy		m_wndProxy;
	CPPgAbout		m_wndAbout; //>>> WiZaRd::AboutPage
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
    CPPgDebug		m_wndDebug;
#endif

    void Localize();
    void SetStartPage(UINT uStartPageID);

protected:
    LPCTSTR m_pPshStartPage;
    bool m_bSaveIniFile;

    virtual BOOL OnInitDialog();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnDestroy();
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

public:
    void UpdateShownPages();

//>>> WiZaRd::Automatic Restart
public:
    static bool	IsRestartPlanned()
    {
        return m_bRestartApp;
    }
    static void	PlanRestart()
    {
        m_bRestartApp = true;
    }
private:
    static bool	m_bRestartApp;
//<<< WiZaRd::Automatic Restart
};
