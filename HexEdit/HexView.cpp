/*++

Copyright (c) Andrey Bazhan

--*/

#include "stdafx.h"
#include "HexView.h"


#define IDT_TIMER     1

#define NUMBEROF_CHARS_IN_FIRST_COLUMN_64BIT    1 + 16 + 2
#define NUMBEROF_CHARS_IN_FIRST_COLUMN_32BIT    1 + 8 + 2
#define NUMBEROF_CHARS_IN_SECOND_COLUMN         16 * 3 + 1
#define NUMBEROF_CHARS_IN_THIRD_COLUMN          16


VOID
HexView_SendNotify(
    _In_ HWND hWnd,
    _In_ UINT Code,
    _In_ LPNMHDR NmHdr
    )
{
    NmHdr->hwndFrom = hWnd;
    NmHdr->code = Code;

    SendMessage(GetParent(hWnd), WM_NOTIFY, 0, (LPARAM)NmHdr);
}

VOID
HexView_DrawLine(
    _In_ HWND hWnd,
    _In_ HDC hdcMem,
    _In_ PHEXVIEW HexView,
    _In_ int NumberOfLine
    )
{
    RECT rc = {0};

    //
    // Clean hdcMem
    //

    rc.right = HexView->WidthView;
    rc.bottom = HexView->HeightChar;
    ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    if ((HexView->VscrollPos + NumberOfLine) < HexView->TotalLines) {

        NMHVDISPINFO DispInfo = {0};
        TCHAR Buffer[32];
        int ShiftPosX;
        int Position1, Position2;
        SIZE_T SelectionStart, SelectionEnd;
        SIZE_T i, NumberOfItem;
        COLORREF clrBk;

        //
        // Address column
        //

        NumberOfItem = (HexView->VscrollPos + NumberOfLine) * 16;

        DispInfo.Item.Mask = HVIF_ADDRESS;
        DispInfo.Item.NumberOfItem = NumberOfItem;
        HexView_SendNotify(hWnd, HVN_GETDISPINFO, (LPNMHDR)&DispInfo);

        SetTextColor(hdcMem, HexView->clrText);

        StringCchPrintf(Buffer, _countof(Buffer), (HexView->ExStyle & HVS_ADDRESS64) ? _T("%016I64X") : _T("%08X"), DispInfo.Item.Address);
        
        ShiftPosX = -(HexView->WidthChar * HexView->HscrollPos) + HexView->WidthChar;
        TextOut(hdcMem, ShiftPosX, 0, (PCTSTR)Buffer, (int)_tcslen(Buffer));

        //
        // DATA and VALUE columns
        //

        Position1 = HexView->Line1;
        Position2 = HexView->Line2;

        SelectionStart = min(HexView->SelectionStart, HexView->SelectionEnd);
        SelectionEnd   = max(HexView->SelectionStart, HexView->SelectionEnd);

        for (i = NumberOfItem; i <= NumberOfItem + 15 && i < HexView->TotalItems; i++) {

            DispInfo.Item.Mask = HVIF_BYTE;
            DispInfo.Item.State = 0;
            DispInfo.Item.NumberOfItem = i;
            DispInfo.Item.Address = 0;
            DispInfo.Item.Value = 0;
            HexView_SendNotify(hWnd, HVN_GETDISPINFO, (LPNMHDR)&DispInfo);

            if ((HexView->Flags & HVF_SELECTED) && (i >= SelectionStart && i <= SelectionEnd)) {

                clrBk = SetBkColor(hdcMem, HexView->clrSelectedTextBackground);

                if (i != SelectionEnd && i != NumberOfItem + 15) {

                    rc.top = 0;
                    rc.bottom = HexView->HeightView;
                    rc.left = Position1 + HexView->WidthChar * 2;
                    rc.right = rc.left + HexView->WidthChar;

                    ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
                }
            }
            else {

                clrBk = SetBkColor(hdcMem, HexView->clrTextBackground);
            }

            if (DispInfo.Item.State & HVIS_MODIFIED) {

                SetTextColor(hdcMem, HexView->clrModifiedText);
            }
            else {

                SetTextColor(hdcMem, HexView->clrText);
            }

            //
            // Draw a hex value
            //

            StringCchPrintf(Buffer, _countof(Buffer), _T("%02X"), DispInfo.Item.Value);

            TextOut(hdcMem, Position1, 0, (PCTSTR)Buffer, 2);

            //
            // Draw a char value
            //

            if (isprint(DispInfo.Item.Value)) {

                StringCchPrintf(Buffer, _countof(Buffer), _T("%c"), DispInfo.Item.Value);
            }
            else {

                _tcscpy_s(Buffer, _countof(Buffer), _T("."));
            }

            TextOut(hdcMem, Position2, 0, (PCTSTR)Buffer, 1);

            SetBkColor(hdcMem, clrBk);

            Position1 = Position1 + HexView->WidthChar * 3;
            Position2 = Position2 + HexView->WidthChar;
        }
    }
}

VOID
HexView_Paint(
    _In_ HWND hWnd,
    _In_ HDC hdc,
    _In_ PHEXVIEW HexView
    )
{
    HDC hdcMem;
    HBITMAP hbmMem;
    HANDLE hOld;
    HANDLE hOldFont;
    COLORREF clrBk;
    int NumberOfLine;

    hdcMem = CreateCompatibleDC(hdc);
    hbmMem = CreateCompatibleBitmap(hdc, HexView->WidthView, HexView->HeightChar);

    hOld = SelectObject(hdcMem, hbmMem);
    hOldFont = SelectObject(hdcMem, HexView->hFont);

    clrBk = SetBkColor(hdcMem, HexView->clrTextBackground);

    for (NumberOfLine = 0; NumberOfLine <= HexView->VisibleLines; NumberOfLine++) {

        HexView_DrawLine(hWnd, hdcMem, HexView, NumberOfLine);

        BitBlt(hdc, 0, HexView->HeightChar * NumberOfLine, HexView->WidthView, HexView->HeightChar, hdcMem, 0, 0, SRCCOPY);
    }

    SetBkColor(hdcMem, clrBk);

    SelectObject(hdcMem, hOldFont);
    SelectObject(hdcMem, hOld);

    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

_Check_return_
BOOL
HexView_PinToBottom(
    _Inout_ PHEXVIEW HexView
    )
{
    BOOL IsOk = FALSE;
    LONG_PTR VisibleLines;
    int VisibleChars;

    VisibleLines = min(HexView->VisibleLines, HexView->TotalLines);
    VisibleChars = min(HexView->VisibleChars, HexView->LongestLine);

    if (HexView->VscrollPos + VisibleLines > HexView->TotalLines) {

        HexView->VscrollPos = HexView->TotalLines - VisibleLines;

        IsOk = TRUE;
    }

    if (HexView->HscrollPos + VisibleChars > HexView->LongestLine) {

        int HscrollPos = HexView->LongestLine - VisibleChars;

        HexView->Line1 -= HexView->WidthChar * (HscrollPos - HexView->HscrollPos);
        HexView->Line2 -= HexView->WidthChar * (HscrollPos - HexView->HscrollPos);

        HexView->HscrollPos = HscrollPos;

        IsOk = TRUE;
    }

    return IsOk;
}

_Check_return_
LONG_PTR
HexView_GetTrackPos(
    _In_ HWND hWnd,
    _In_ PHEXVIEW HexView,
    _In_ int nBar
    )
{
    SCROLLINFO si = {0};

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_TRACKPOS | SIF_PAGE;

    GetScrollInfo(hWnd, nBar, &si);

    if (nBar == SB_VERT) {

        if (HexView->VscrollMax > MAXLONG) {

            if (si.nTrackPos == (int)(MAXSHORT - si.nPage + 1)) {

                return HexView->VscrollMax - si.nPage + 1;
            }

            return (HexView->VscrollMax / MAXSHORT) * si.nTrackPos;
        }
    }

    return si.nTrackPos;
}

VOID
HexView_SetScrollInfo(
    _In_ HWND hWnd,
    _In_ PHEXVIEW HexView
    )
{
    SCROLLINFO si = {0};

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;

    if (HexView->VscrollMax <= MAXLONG) {

        si.nPos  = (int)HexView->VscrollPos;
        si.nPage = (UINT)HexView->VisibleLines;
        si.nMin  = 0;
        si.nMax  = (int)HexView->VscrollMax;
    }
    else {

        si.nPos  = (int)(HexView->VscrollPos / (HexView->VscrollMax / MAXSHORT));
        si.nPage = (UINT)HexView->VisibleLines;
        si.nMin  = 0;
        si.nMax  = MAXSHORT;
    }

    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

    si.nPos  = HexView->HscrollPos;
    si.nPage = (UINT)HexView->VisibleChars;
    si.nMin  = 0;
    si.nMax  = HexView->HscrollMax;

    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
}

VOID
HexView_SetCaret(
    _In_ HWND hWnd,
    _Inout_ PHEXVIEW HexView
    )

/*++

Note:

    Hiding is cumulative.
    If your application calls HideCaret five times in a row,
    it must also call ShowCaret five times before the caret is displayed.
    So we need IsCaretVisible in order to hide the caret only once.

--*/

{
    LONG_PTR NumberOfLine = HexView->NumberOfItem / 16;
    int NumberOfItemInLine = (int)(HexView->NumberOfItem % 16);

    if (HexView->TotalItems &&
        NumberOfLine >= HexView->VscrollPos &&
        NumberOfLine <= HexView->VscrollPos + HexView->VisibleLines)
    {
        switch (HexView->ActiveColumn) {

        case COLUMN_DATA:

            SetCaretPos(HexView->Line1 + (HexView->WidthChar * ((NumberOfItemInLine * 3) + HexView->PositionInItem)),
                        (int)(NumberOfLine - HexView->VscrollPos) * HexView->HeightChar);
            break;

        case COLUMN_VALUE:

            SetCaretPos(HexView->Line2 + (HexView->WidthChar * NumberOfItemInLine),
                        (int)(NumberOfLine - HexView->VscrollPos) * HexView->HeightChar);
            break;
        }

        ShowCaret(hWnd);
        HexView->Flags |= HVF_CARETVISIBLE;
    }
    else if (HexView->Flags & HVF_CARETVISIBLE) {

        HideCaret(hWnd);
        HexView->Flags &= ~HVF_CARETVISIBLE;
    }
}

VOID
HexView_SetPositionOfColumns(
    _Inout_ PHEXVIEW HexView
    )
{
    if (HexView->ExStyle & HVS_ADDRESS64) {

        HexView->Line1 = HexView->WidthChar * (NUMBEROF_CHARS_IN_FIRST_COLUMN_64BIT - HexView->HscrollPos);
        HexView->Line2 = HexView->WidthChar * ((NUMBEROF_CHARS_IN_FIRST_COLUMN_64BIT + NUMBEROF_CHARS_IN_SECOND_COLUMN) - HexView->HscrollPos);
    }
    else {

        HexView->Line1 = HexView->WidthChar * (NUMBEROF_CHARS_IN_FIRST_COLUMN_32BIT - HexView->HscrollPos);
        HexView->Line2 = HexView->WidthChar * ((NUMBEROF_CHARS_IN_FIRST_COLUMN_32BIT + NUMBEROF_CHARS_IN_SECOND_COLUMN) - HexView->HscrollPos);
    }
}

_Check_return_
BOOL
HexView_SetNumberOfItem(
    _Inout_ PHEXVIEW HexView,
    _In_ int xPos,
    _In_ int yPos
    )
{
    LONG_PTR NumberOfLine;

    _ASSERTE(yPos >= 0);

    NumberOfLine = yPos / HexView->HeightChar;

    if (NumberOfLine == HexView->VisibleLines) {
        NumberOfLine--;
    }

    NumberOfLine += HexView->VscrollPos;

    if (NumberOfLine >= 0 && NumberOfLine < HexView->TotalLines) {

        SIZE_T NumberOfItem = NumberOfLine * 16;
        LONG_PTR NumberOfItemsInLine = HexView->TotalItems - NumberOfItem;

        if (NumberOfItemsInLine > 16) {
            NumberOfItemsInLine = 16;
        }

        if ((xPos > HexView->Line1 - HexView->WidthChar * 2) &&
            (xPos < (HexView->Line1 + ((NumberOfItemsInLine * 3) - 1) * HexView->WidthChar)))
        {
            xPos = (xPos - HexView->Line1) / HexView->WidthChar;

            HexView->NumberOfItem = NumberOfItem + ((xPos + 1) / 3);
            HexView->PositionInItem = ((xPos + 1) % 3) >> 1;
            HexView->ActiveColumn = COLUMN_DATA;
        }
        else if ((xPos > HexView->Line2 - HexView->WidthChar) &&
                 (xPos < (HexView->Line2 + NumberOfItemsInLine * HexView->WidthChar)))
        {
            xPos = (xPos - HexView->Line2) / HexView->WidthChar;

            HexView->NumberOfItem = NumberOfItem + xPos;
            HexView->PositionInItem = 0;
            HexView->ActiveColumn = COLUMN_VALUE;
        }

        return TRUE;
    }

    return FALSE;
}

LRESULT
CALLBACK
HexViewProc(
    _In_ HWND hWnd,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PHEXVIEW HexView;

    HexView = (PHEXVIEW)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (message) {

    case WM_CREATE:
    {
        HexView = (PHEXVIEW)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HEXVIEW));

        if (!HexView) {

            return -1;
        }

        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)HexView);

        HexView->ActiveColumn = COLUMN_DATA;

        HexView->clrText                   = RGB(0, 0, 0);
        HexView->clrTextBackground         = RGB(255, 255, 255);
        HexView->clrSelectedTextBackground = RGB(180, 220, 255);
        HexView->clrModifiedText           = RGB(255, 0, 0);

        SendMessage(hWnd, WM_SETFONT, 0, 0);
        break;
    }
    case WM_SETFONT:
    {
        HDC hdc;
        TEXTMETRIC TextMetric;
        HANDLE hOldFont;

        //
        // Delete the font, if we created it by yourself.
        //

        if ((HexView->Flags & HVF_FONTCREATED) && HexView->hFont) {

            DeleteObject(HexView->hFont);
            HexView->Flags &= ~HVF_FONTCREATED;
        }

        HexView->hFont = (HFONT)wParam;

        if (!HexView->hFont) {

            LOGFONT LogFont;

            LogFont.lfHeight         = -13;
            LogFont.lfWidth          = 0;
            LogFont.lfEscapement     = 0;
            LogFont.lfOrientation    = 0;
            LogFont.lfWeight         = FW_NORMAL;
            LogFont.lfItalic         = FALSE;
            LogFont.lfUnderline      = FALSE;
            LogFont.lfStrikeOut      = FALSE;
            LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
            LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
            LogFont.lfQuality        = DEFAULT_QUALITY;
            LogFont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
            LogFont.lfCharSet        = ANSI_CHARSET;

            _tcscpy_s(LogFont.lfFaceName, _countof(LogFont.lfFaceName), _T("Courier New"));

            HexView->hFont = CreateFontIndirect(&LogFont);

            if (HexView->hFont) {

                HexView->Flags |= HVF_FONTCREATED;
            }
            else {

                HexView->hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
            }
        }

        hdc = GetDC(hWnd);

        hOldFont = SelectObject(hdc, HexView->hFont);
        GetTextMetrics(hdc, &TextMetric);
        SelectObject(hdc, hOldFont);

        ReleaseDC(hWnd, hdc);

        HexView->WidthChar = TextMetric.tmAveCharWidth;
        HexView->HeightChar = TextMetric.tmHeight;

        HexView->VisibleLines = HexView->HeightView / HexView->HeightChar;
        HexView->VisibleChars = HexView->WidthView / HexView->WidthChar;

        if (hWnd == GetFocus()) {

            CreateCaret(hWnd, NULL, 1, HexView->HeightChar);
        }

        HexView_SetPositionOfColumns(HexView);

        HexView_SetScrollInfo(hWnd, HexView);
        HexView_SetCaret(hWnd, HexView);

        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_TIMER:
    {
        switch (wParam) {

        case IDT_TIMER:
        {
            if (hWnd == GetCapture()) {

                RECT rc;
                POINT Point;
                LONG Position;

                GetClientRect(hWnd, &rc);
                GetCursorPos(&Point);
                ScreenToClient(hWnd, &Point);

                if (Point.y < rc.top) {

                    SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, -1);

                    Position = MAKELPARAM(Point.x, 0);
                    SendMessage(hWnd, WM_MOUSEMOVE, 0, Position);
                }
                else if (Point.y > rc.bottom) {

                    SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, -1);

                    Position = MAKELPARAM(Point.x, rc.bottom - HexView->WidthChar);
                    SendMessage(hWnd, WM_MOUSEMOVE, 0, Position);
                }
            }
            break;
        }
        }

        break;
    }
    case WM_RBUTTONDOWN:
    {
        if (HexView->TotalItems) {

            NMHDR NmHdr = {0};

            SendMessage(hWnd, WM_SETFOCUS, 0, 0);

            HexView_SendNotify(hWnd, NM_RCLICK, &NmHdr);
        }
        break;
    }
    case WM_LBUTTONDOWN:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (HexView_SetNumberOfItem(HexView, xPos, yPos)) {

            HexView->SelectionStart = HexView->SelectionEnd = HexView->NumberOfItem;

            if (HexView->Flags & HVF_SELECTED) {

                HexView->Flags &= ~HVF_SELECTED;

                InvalidateRect(hWnd, NULL, FALSE);
            }

            HexView_SetCaret(hWnd, HexView);
            SetFocus(hWnd);

            SetCapture(hWnd);
            SetTimer(hWnd, IDT_TIMER, USER_TIMER_MINIMUM, (TIMERPROC)NULL);
        }

        break;
    }
    case WM_LBUTTONUP:
    {
        if (hWnd == GetCapture()) {

            KillTimer(hWnd, IDT_TIMER);
            ReleaseCapture();
        }

        break;
    }
    case WM_MOUSEMOVE:
    {
        RECT rc;
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        GetClientRect(hWnd, &rc);

        if (yPos < rc.top || yPos > rc.bottom) {
            break;
        }

        if (hWnd == GetCapture()) {

            if (HexView_SetNumberOfItem(HexView, xPos, yPos)) {

                HexView->SelectionEnd = HexView->NumberOfItem;
                HexView->Flags |= HVF_SELECTED;

                HexView_SetCaret(hWnd, HexView);

                InvalidateRect(hWnd, NULL, FALSE);
            }
        }

        break;
    }
    case WM_CHAR:
    {
        if (HexView->TotalItems && !(HexView->ExStyle & HVS_READONLY)) {

            switch (HexView->ActiveColumn) {

            case COLUMN_DATA:
            {
                if (_istxdigit((wint_t)wParam)) {

                    NMHVDISPINFO DispInfo = {0};
                    NMHEXVIEW NmHexView = {0};
                    TCHAR Buffer[4] = {0};
                    BYTE Value;

                    DispInfo.Item.Mask = HVIF_BYTE;
                    DispInfo.Item.NumberOfItem = HexView->NumberOfItem;
                    HexView_SendNotify(hWnd, HVN_GETDISPINFO, (LPNMHDR)&DispInfo);

                    Buffer[0] = (TCHAR)wParam;
                    Value = (BYTE)_tcstoul((PCTSTR)Buffer, 0, 16);

                    if (HexView->PositionInItem) {

                        DispInfo.Item.Value &= 0xf0;
                        DispInfo.Item.Value |= Value;
                    }
                    else {

                        DispInfo.Item.Value &= 0x0f;
                        DispInfo.Item.Value |= (Value << 4);
                    }

                    NmHexView.Item.NumberOfItem = HexView->NumberOfItem;
                    NmHexView.Item.Value = DispInfo.Item.Value;
                    HexView_SendNotify(hWnd, HVN_ITEMCHANGING, (LPNMHDR)&NmHexView);

                    SendMessage(hWnd, WM_KEYDOWN, VK_RIGHT, 0);
                }
                break;
            }
            case COLUMN_VALUE:
            {
                if (_istprint((wint_t)wParam) && wParam != VK_TAB) {

                    NMHEXVIEW NmHexView = {0};

                    NmHexView.Item.NumberOfItem = HexView->NumberOfItem;
                    NmHexView.Item.Value = (BYTE)wParam;
                    HexView_SendNotify(hWnd, HVN_ITEMCHANGING, (LPNMHDR)&NmHexView);

                    SendMessage(hWnd, WM_KEYDOWN, VK_RIGHT, 0);
                }
                break;
            }
            }
        }

        break;
    }
    case WM_KEYDOWN:
    {
        if (HexView->TotalItems) {

            BOOLEAN IsCtrlKeyDown = (GetKeyState(VK_CONTROL) & 0x8000) ? TRUE : FALSE;

            switch (wParam) {

            case VK_TAB:
            {
                switch (HexView->ActiveColumn) {

                case COLUMN_DATA:

                    HexView->ActiveColumn = COLUMN_VALUE;
                    break;

                case COLUMN_VALUE:

                    HexView->ActiveColumn = COLUMN_DATA;
                    break;
                }

                HexView->PositionInItem = 0;
                break;
            }
            case VK_LEFT:
            {
                if (IsCtrlKeyDown) {

                    SendMessage(hWnd, WM_HSCROLL, SB_LINELEFT, 0);
                    break;
                }

                switch (HexView->ActiveColumn) {

                case COLUMN_DATA:
                {
                    switch (HexView->PositionInItem) {

                    case 0:
                    {
                        if (HexView->NumberOfItem != 0) {

                            LONG_PTR NumberOfLine;

                            HexView->PositionInItem = 1;
                            HexView->NumberOfItem--;

                            NumberOfLine = HexView->NumberOfItem / 16;

                            if (NumberOfLine == HexView->VscrollPos - 1) {

                                SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
                            }
                        }
                        break;
                    }
                    case 1:
                    {
                        HexView->PositionInItem = 0;
                        break;
                    }
                    }
                    break;
                }
                case COLUMN_VALUE:
                {
                    if (HexView->NumberOfItem != 0) {

                        LONG_PTR NumberOfLine;

                        HexView->NumberOfItem--;

                        NumberOfLine = HexView->NumberOfItem / 16;

                        if (NumberOfLine == HexView->VscrollPos - 1) {

                            SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
                        }
                    }
                    break;
                }
                }
                break;
            }
            case VK_RIGHT:
            {
                if (IsCtrlKeyDown) {

                    SendMessage(hWnd, WM_HSCROLL, SB_LINERIGHT, 0);
                    break;
                }

                switch (HexView->ActiveColumn) {

                case COLUMN_DATA:
                {
                    switch (HexView->PositionInItem) {

                    case 0:
                    {
                        HexView->PositionInItem = 1;
                        break;
                    }
                    case 1:
                    {
                        if (HexView->NumberOfItem < HexView->TotalItems - 1) {

                            LONG_PTR NumberOfLine;

                            HexView->PositionInItem = 0;
                            HexView->NumberOfItem++;

                            NumberOfLine = HexView->NumberOfItem / 16;

                            if (NumberOfLine == HexView->VscrollPos + HexView->VisibleLines) {

                                SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
                            }
                        }
                        break;
                    }
                    }
                    break;
                }
                case COLUMN_VALUE:
                {
                    if (HexView->NumberOfItem < HexView->TotalItems - 1) {

                        LONG_PTR NumberOfLine;

                        HexView->NumberOfItem++;

                        NumberOfLine = HexView->NumberOfItem / 16;

                        if (NumberOfLine == HexView->VscrollPos + HexView->VisibleLines) {

                            SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
                        }
                    }
                    break;
                }
                }
                break;
            }
            case VK_UP:
            {
                if (IsCtrlKeyDown) {

                    SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
                }

                if (HexView->NumberOfItem >= 16) {

                    LONG_PTR NumberOfLine;

                    HexView->NumberOfItem -= 16;

                    NumberOfLine = HexView->NumberOfItem / 16;

                    if (NumberOfLine == HexView->VscrollPos - 1) {

                        SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
                    }
                }
                break;
            }
            case VK_DOWN:
            {
                if (IsCtrlKeyDown) {

                    SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
                }

                if ((HexView->NumberOfItem + 16) < HexView->TotalItems) {

                    LONG_PTR NumberOfLine;

                    HexView->NumberOfItem += 16;

                    NumberOfLine = HexView->NumberOfItem / 16;

                    if (NumberOfLine == HexView->VscrollPos + HexView->VisibleLines) {

                        SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
                    }
                }
                break;
            }
            case VK_PRIOR:
            {
                if ((HexView->NumberOfItem - 16 * HexView->VisibleLines) >= 0) {

                    HexView->NumberOfItem -= 16 * HexView->VisibleLines;
                }
                else {

                    SIZE_T NumberOfLines = HexView->NumberOfItem / 16;
                    HexView->NumberOfItem -= 16 * NumberOfLines;
                }

                SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0);
                break;
            }
            case VK_NEXT:
            {
                if ((HexView->NumberOfItem + 16 * HexView->VisibleLines) < HexView->TotalItems) {

                    HexView->NumberOfItem += 16 * HexView->VisibleLines;
                }
                else {

                    SIZE_T NumberOfItems = HexView->TotalItems - HexView->NumberOfItem - 1;
                    SIZE_T NumberOfLines = NumberOfItems / 16;

                    HexView->NumberOfItem += 16 * NumberOfLines;
                }

                SendMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
                break;
            }
            case VK_HOME:
            {
                if (IsCtrlKeyDown) {

                    HexView->NumberOfItem = 0;
                    HexView->PositionInItem = 0;

                    SendMessage(hWnd, WM_VSCROLL, SB_TOP, 0);
                }
                else {

                    HexView->NumberOfItem &= ~0xf;
                    HexView->PositionInItem = 0;

                    if (HexView->ActiveColumn == COLUMN_DATA) {

                        SendMessage(hWnd, WM_HSCROLL, SB_LEFT, 0);
                    }
                }
                break;
            }
            case VK_END:
            {
                if (IsCtrlKeyDown) {

                    HexView->NumberOfItem = HexView->TotalItems - 1;
                    HexView->PositionInItem = 0;

                    SendMessage(hWnd, WM_VSCROLL, SB_BOTTOM, 0);
                }
                else {

                    HexView->NumberOfItem = (HexView->NumberOfItem & ~0xf) + 15;
                    HexView->PositionInItem = 0;

                    if (HexView->NumberOfItem >= HexView->TotalItems) {

                        HexView->NumberOfItem = HexView->TotalItems - 1;
                    }

                    SendMessage(hWnd, WM_HSCROLL, SB_RIGHT, 0);
                }
                break;
            }
            }

            //
            // if lParam == 0 then WM_KEYDOWN came from WM_CHAR
            // and we don't need to select text.
            //

            if ((GetKeyState(VK_SHIFT) & 0x8000) && lParam) {

                if (HexView->PositionInItem || (HexView->SelectionStart != HexView->NumberOfItem)) {

                    HexView->SelectionEnd = HexView->NumberOfItem;
                    HexView->Flags |= HVF_SELECTED;
                }
                else {

                    HexView->Flags &= ~HVF_SELECTED;
                }
            }
            else if (!IsCtrlKeyDown && !(GetKeyState(VK_APPS) & 0x8000)) {

                HexView->SelectionStart = HexView->SelectionEnd = HexView->NumberOfItem;
                HexView->Flags &= ~HVF_SELECTED;
            }

            if (!(HexView->Flags & HVF_CARETVISIBLE)) {

                LONG_PTR NumberOfLine = HexView->NumberOfItem / 16;

                if (NumberOfLine > HexView->TotalLines - HexView->VisibleLines) {

                    HexView->VscrollPos = HexView->TotalLines - HexView->VisibleLines;
                }
                else {

                    HexView->VscrollPos = NumberOfLine;
                }

                HexView_SetScrollInfo(hWnd, HexView);
            }

            HexView_SetCaret(hWnd, HexView);

            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc;

        hdc = BeginPaint(hWnd, &ps);
        HexView_Paint(hWnd, hdc, HexView);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_SETFOCUS:
    {
        CreateCaret(hWnd, NULL, 1, HexView->HeightChar);

        if (HexView->Flags & HVF_CARETVISIBLE) {

            NMHDR NmHdr = {0};

            HexView_SetCaret(hWnd, HexView);
            ShowCaret(hWnd);

            HexView_SendNotify(hWnd, NM_SETFOCUS, &NmHdr);
        }
        break;
    }
    case WM_KILLFOCUS:
    {
        if (HexView->Flags & HVF_CARETVISIBLE) {

            HideCaret(hWnd);
        }

        DestroyCaret();
        break;
    }
    case WM_VSCROLL:
    {
        LONG_PTR OldPos = HexView->VscrollPos;

        switch (LOWORD(wParam)) {

        case SB_TOP:

            HexView->VscrollPos = 0;
            break;

        case SB_BOTTOM:

            HexView->VscrollPos = max(0, HexView->VscrollMax - HexView->VisibleLines + 1);
            break;

        case SB_LINEUP:

            HexView->VscrollPos = max(0, HexView->VscrollPos - 1);
            break;

        case SB_LINEDOWN:

            HexView->VscrollPos = min(HexView->VscrollPos + 1, max(0, HexView->VscrollMax - HexView->VisibleLines + 1));
            break;

        case SB_PAGEUP:

            HexView->VscrollPos = max(0, HexView->VscrollPos - HexView->VisibleLines);
            break;

        case SB_PAGEDOWN:

            HexView->VscrollPos = min(HexView->VscrollPos + HexView->VisibleLines, max(0, HexView->VscrollMax - HexView->VisibleLines + 1));
            break;

        case SB_THUMBTRACK:

            HexView->VscrollPos = HexView_GetTrackPos(hWnd, HexView, SB_VERT);
            break;
        }

        if (OldPos != HexView->VscrollPos) {

            HexView_SetScrollInfo(hWnd, HexView);

            //
            // If lParam == -1 then WM_VSCROLL came from WM_TIMER
            // and we don't need to set the caret because it will be set in WM_MOUSEMOVE.
            //

            if (-1 != lParam) {

                HexView_SetCaret(hWnd, HexView);
            }

            InvalidateRect(hWnd, NULL, FALSE);
        }

        break;
    }
    case WM_HSCROLL:
    {
        int OldPos = HexView->HscrollPos;

        switch (LOWORD(wParam)) {

        case SB_LEFT:

            HexView->HscrollPos = 0;
            break;

        case SB_RIGHT:

            HexView->HscrollPos = max(0, HexView->HscrollMax - HexView->VisibleChars + 1);
            break;

        case SB_LINELEFT:
        case SB_PAGELEFT:

            HexView->HscrollPos = max(0, HexView->HscrollPos - 1);
            break;

        case SB_LINERIGHT:
        case SB_PAGERIGHT:

            HexView->HscrollPos = min(HexView->HscrollPos + 1, max(0, HexView->HscrollMax - HexView->VisibleChars + 1));
            break;

        case SB_THUMBTRACK:

            HexView->HscrollPos = (int)HexView_GetTrackPos(hWnd, HexView, SB_HORZ);
            break;
        }

        if (OldPos != HexView->HscrollPos) {

            HexView_SetPositionOfColumns(HexView);

            HexView_SetScrollInfo(hWnd, HexView);
            HexView_SetCaret(hWnd, HexView);

            InvalidateRect(hWnd, NULL, FALSE);
        }

        break;
    }
    case WM_SIZE:
    {
        HexView->WidthView  = LOWORD(lParam);
        HexView->HeightView = HIWORD(lParam);

        HexView->VisibleLines = HexView->HeightView / HexView->HeightChar;
        HexView->VisibleChars = HexView->WidthView / HexView->WidthChar;

        if (HexView_PinToBottom(HexView)) {

            HexView_SetCaret(hWnd, HexView);

            InvalidateRect(hWnd, NULL, FALSE);
        }

        HexView_SetScrollInfo(hWnd, HexView);
        break;
    }
    case WM_MOUSEWHEEL:
    {
        INT ScrollLines = 0;
        SHORT Delta;

        Delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &ScrollLines, 0);

        if (Delta > 0) {

            HexView->VscrollPos = max(0, HexView->VscrollPos - (Delta * ScrollLines));
        }
        else if (Delta < 0) {

            HexView->VscrollPos = min(HexView->VscrollPos + (-Delta * ScrollLines), max(0, HexView->VscrollMax - HexView->VisibleLines + 1));
        }

        HexView_SetScrollInfo(hWnd, HexView);
        HexView_SetCaret(hWnd, HexView);

        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case HVM_SETEXTENDEDSTYLE:
    {
        HexView->ExStyle = (DWORD)lParam;
        break;
    }
    case HVM_SETITEMCOUNT:
    {
        HexView->TotalItems = 0;
        HexView->TotalLines = 0;
        HexView->LongestLine = 0;

        if (lParam) {

            HexView->TotalItems = (SIZE_T)lParam;

            HexView->TotalLines = HexView->TotalItems / 16;

            if (HexView->TotalItems % 16) {
                HexView->TotalLines += 1;
            }

            HexView->LongestLine = ((HexView->ExStyle & HVS_ADDRESS64) ? NUMBEROF_CHARS_IN_FIRST_COLUMN_64BIT : NUMBEROF_CHARS_IN_FIRST_COLUMN_32BIT) + NUMBEROF_CHARS_IN_SECOND_COLUMN + NUMBEROF_CHARS_IN_THIRD_COLUMN;
        }

        HexView->NumberOfItem = 0;
        HexView->PositionInItem = 0;

        HexView->SelectionStart = 0;
        HexView->SelectionEnd = 0;
        HexView->Flags &= ~HVF_SELECTED;

        HexView->ActiveColumn = COLUMN_DATA;

        HexView->VscrollPos = 0;
        HexView->VscrollMax = max(HexView->TotalLines - 1, 0);
        HexView->HscrollPos = 0;
        HexView->HscrollMax = max(HexView->LongestLine - 1, 0);

        HexView_SetPositionOfColumns(HexView);

        HexView_SetScrollInfo(hWnd, HexView);
        HexView_SetCaret(hWnd, HexView);

        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case HVM_GETSEL:
    {
        if (lParam) {

            ((PBYTERANGE)lParam)->Min = min(HexView->SelectionStart, HexView->SelectionEnd);
            ((PBYTERANGE)lParam)->Max = max(HexView->SelectionStart, HexView->SelectionEnd);

            if (HexView->Flags & HVF_SELECTED) {

                ((PBYTERANGE)lParam)->Max += 1;
            }
        }
        break;
    }
    case HVM_SETSEL:
    {
        if (lParam) {

            if ((((PBYTERANGE)lParam)->Min >= 0 && ((PBYTERANGE)lParam)->Min < HexView->TotalItems) &&
                (((PBYTERANGE)lParam)->Max >= 0 && ((PBYTERANGE)lParam)->Max <= HexView->TotalItems)) {

                HexView->SelectionStart = ((PBYTERANGE)lParam)->Min;
                HexView->SelectionEnd   = ((PBYTERANGE)lParam)->Max;

                if (((PBYTERANGE)lParam)->Min != ((PBYTERANGE)lParam)->Max) {

                    HexView->SelectionEnd -= 1;
                    HexView->Flags |= HVF_SELECTED;
                }
                else {

                    HexView->Flags &= ~HVF_SELECTED;
                }

                HexView->NumberOfItem = HexView->SelectionStart;
                HexView->PositionInItem = 0;

                HexView->VscrollPos = min((LONG_PTR)(HexView->NumberOfItem / 16), max(0, HexView->VscrollMax - HexView->VisibleLines + 1));

                HexView_SetScrollInfo(hWnd, HexView);
                HexView_SetCaret(hWnd, HexView);

                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        break;
    }
    case HVM_SETTEXTCOLOR:
    {
        HexView->clrText = (COLORREF)lParam;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case HVM_SETBKCOLOR:
    {
        HexView->clrTextBackground = (COLORREF)lParam;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case HVM_SETSELBKCOLOR:
    {
        HexView->clrSelectedTextBackground = (COLORREF)lParam;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case HVM_SETMODIFIEDCOLOR:
    {
        HexView->clrModifiedText = (COLORREF)lParam;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_DESTROY:
    {
        if (HexView) {

            if (HexView->Flags & HVF_FONTCREATED) {

                DeleteObject(HexView->hFont);
            }

            HeapFree(GetProcessHeap(), 0, HexView);
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

HWND
CreateHexView(
    _In_ HWND hWndParent
    )
{
    HWND hWnd;

    hWnd = CreateWindowEx(0,
                          _T("HexView"),
                          NULL,
                          WS_CHILD |
                          WS_VISIBLE,
                          0,
                          0,
                          0,
                          0,
                          hWndParent,
                          0,
                          GetModuleHandle(NULL),
                          NULL);

    return hWnd;
}

VOID
RegisterClassHexView(
    VOID
    )
{
    WNDCLASSEX wcex = {0};

    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc   = HexViewProc;
    wcex.hInstance     = GetModuleHandle(NULL);
    wcex.hCursor       = LoadCursor(NULL, IDC_IBEAM);
    wcex.lpszClassName = _T("HexView");

    RegisterClassEx(&wcex);
}