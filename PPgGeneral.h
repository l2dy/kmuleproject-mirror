#pragma once
#include "HyperTextCtrl.h" //>>> WiZaRd::Honor translators

class CPPgGeneral : public CPropertyPage
{
    DECLARE_DYNAMIC(CPPgGeneral)

  public:
    CPPgGeneral();
    virtual ~CPPgGeneral();

// Dialog Data
    enum { IDD = IDD_PPG_GENERAL };

    void Localize(void);

  protected:
    CComboBox m_language;
    void LoadSettings(void);
    void UpdateEd2kLinkFixCtrl();

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    virtual BOOL OnSetActive();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnSettingsChange()
    {
        SetModified();
    }
    afx_msg void OnBnClickedEd2kfix();
    afx_msg void OnBnClickedEditWebservices();
    afx_msg void OnLangChange();
    afx_msg void OnBnClickedCheck4Update();
    afx_msg void OnCbnCloseupLangs();
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

//>>> WiZaRd::Honor translators
    CHyperTextCtrl m_wndShowTranslatorHP;
    void UpdateTranslatorField();
//<<< WiZaRd::Honor translators
};
