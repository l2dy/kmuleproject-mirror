#pragma once

enum EStatusBarPane
{
    SBarLog = 0,
    SBarUsers,
    SBarUpDown,
    SBarConnected,
    SBarChatMsg,
    SBarUSS //>>> WiZaRd::USS Status Pane [Eulero]
};

class CMuleStatusBarCtrl : public CStatusBarCtrl
{
    DECLARE_DYNAMIC(CMuleStatusBarCtrl)

public:
    CMuleStatusBarCtrl();
    virtual ~CMuleStatusBarCtrl();

    void Init(void);

protected:
    int GetPaneAtPosition(CPoint& point) const;

    DECLARE_MESSAGE_MAP()
    afx_msg void OnLButtonDblClk(UINT nFlags,CPoint point);
};
