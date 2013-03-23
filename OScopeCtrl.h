#pragma once

/////////////////////////////////////////////////////////////////////////////
// COScopeCtrl window

class COScopeCtrl : public CWnd
{
public:
    COScopeCtrl(const int NTrends = 1);
    virtual ~COScopeCtrl();

    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, const UINT nID = NULL);

    void SetTrendRatio(const int iTrend, const UINT iRatio = 1);
    void SetLegendLabel(const CString& string, const int iTrend);
    void SetBarsPlot(const bool BarsPlot, const int iTrend);
    void SetRange(const double& dLower, const double& dUpper, const int iTrend = 0);
    void SetRanges(const double& dLower, const double& dUpper);
    void SetXUnits(const CString& string, const CString& XMin = L"", const CString& XMax = L"");
    void SetYUnits(const CString& string, const CString& YMin = L"", const CString& YMax = L"");
    void SetGridColor(const COLORREF color);
    void SetPlotColor(const COLORREF color, const int iTrend = 0);
    COLORREF GetPlotColor(const int iTrend = 0);
    void SetBackgroundColor(const COLORREF color);
    void GetPlotRect(CRect& rPlotRect)
    {
        rPlotRect = m_rectPlot;
    }

    void AppendPoints(const double dNewPoint[], const bool bInvalidate = true, const bool bAdd2List = true, const bool bUseTrendRatio = true);
    void AppendEmptyPoints(const double dNewPoint[], const bool bInvalidate = true, const bool bAdd2List = true, const bool bUseTrendRatio = true);
    void InvalidateCtrl(const bool deleteGraph = true);
    void DrawPoint();
    void Reset();
    int ReCreateGraph();

    bool ready;
    bool drawBars;
    bool autofitYscale;
    int m_nXGrids;
    int m_nYGrids;
    int m_nXPartial;
    int m_nShiftPixels;         // amount to shift with each new point
    int m_nTrendPoints;			// when you set this to > 0, then plot will
    int m_nMaxPointCnt;
    int m_nYDecimals;

    typedef struct m_str_struct
    {
        CString XUnits, XMin, XMax;
        CString YUnits, YMin, YMax;
    } m_str_t;
    m_str_t m_str;

    COLORREF m_crBackColor;        // background color
    COLORREF m_crGridColor;        // grid color

protected:
    int m_NTrends;

    struct CustShiftStruct  // when m_nTrendPoints > 0, this structure will contain needed vars
    {
        int m_nRmndr;		// reminder after dividing m_nWidthToDo/m_nPointsToDo
        int m_nWidthToDo;
        int m_nPointsToDo;
    } CustShift;

    typedef struct PlotDataStruct
    {
        COLORREF crPlotColor;       // data plot color
        CPen   penPlot;
        double dCurrentPosition;    // current position
        double dPreviousPosition;   // previous position
        int	nPrevY;
        double dLowerLimit;         // lower bounds
        double dUpperLimit;         // upper bounds
        double dRange;				// = UpperLimit - LowerLimit
        double dVerticalFactor;
        // Optional variable to set a ratio for a given "trend".
        // The purpose here is to better implement the customizable
        // active connections ratio, so that points are redrawn correctly
        // when the ratio is changed, rather than having all previous points
        // no longer match up with the new ratio.
        int		iTrendRatio;
        CString LegendLabel;
        bool BarsPlot;
        CList<double> lstPoints;
    } PlotData_t;
    PlotData_t* m_PlotData;

    int m_nClientHeight;
    int m_nClientWidth;
    int m_nPlotHeight;
    int m_nPlotWidth;

    CRect	m_rectClient;
    CRect	m_rectPlot;
    CDC     m_dcGrid;
    CDC     m_dcPlot;
    CBitmap m_bitmapOldGrid;
    CBitmap m_bitmapOldPlot;
    CBitmap m_bitmapGrid;
    CBitmap m_bitmapPlot;

    static CFont	sm_fontAxis;
    static LOGFONT	sm_logFontAxis;

    bool m_bDoUpdate;
    UINT m_nRedrawTimer;
    UINT m_uLastMouseFlags;
    CPoint m_ptLastMousePos;

    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnSysColorChange();
};
