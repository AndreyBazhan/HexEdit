/*++

Copyright (c) Andrey Bazhan

--*/

#pragma once
#include "stdafx.h"


#define HVM_SETTEXTCOLOR        (WM_USER + 0)
#define HVM_SETBKCOLOR          (WM_USER + 1)
#define HVM_SETSELBKCOLOR       (WM_USER + 2)
#define HVM_SETMODIFIEDCOLOR    (WM_USER + 3)
#define HVM_SETEXTENDEDSTYLE    (WM_USER + 4)
#define HVM_SETITEMCOUNT        (WM_USER + 5)
#define HVM_GETSEL              (WM_USER + 6)
#define HVM_SETSEL              (WM_USER + 7)


#define HVN_GETDISPINFO         (WMN_FIRST - 0)
#define HVN_ITEMCHANGING        (WMN_FIRST - 1)


//
// Mask of an item
//

#define HVIF_ADDRESS            0x0001
#define HVIF_BYTE               0x0002

//
// State of an item
//

#define HVIS_MODIFIED           0x0001

//
// Extended styles
//

#define HVS_ADDRESS64           0x0001
#define HVS_READONLY            0x0002

//
// States of the control
//

#define HVF_FONTCREATED         0x0001
#define HVF_CARETVISIBLE        0x0002
#define HVF_SELECTED            0x0004


enum Column {
    COLUMN_DATA = 1,
    COLUMN_VALUE
};

typedef struct _HEXVIEW {
    int Line1; // Start of the COLUMN_DATA
    int Line2; // Start of the COLUMN_VALUE

    int VisibleLines;
    LONG_PTR TotalLines;
    int VisibleChars;
    int LongestLine;

    LONG_PTR VscrollPos;
    LONG_PTR VscrollMax;
    int HscrollPos;
    int HscrollMax;

    int HeightChar;
    int WidthChar;
    int HeightView;
    int WidthView;

    int ActiveColumn; // COLUMN_DATA or COLUMN_VALUE

    SIZE_T TotalItems;
    SIZE_T NumberOfItem;
    UINT PositionInItem; // 0 = HINIBBLE; 1 = LONIBBLE

    SIZE_T SelectionStart;
    SIZE_T SelectionEnd;

    DWORD Flags;
    DWORD ExStyle;

    HFONT hFont;

    COLORREF clrText;
    COLORREF clrTextBackground;
    COLORREF clrSelectedTextBackground;
    COLORREF clrModifiedText;
} HEXVIEW, *PHEXVIEW;

typedef struct _HVITEM {
    UINT Mask;
    UINT State;
    ULONG64 Address;
    SIZE_T NumberOfItem;
    BYTE Value;
} HVITEM, *PHVITEM;

typedef struct _NMHVDISPINFO {
    NMHDR NmHdr;
    HVITEM Item;
} NMHVDISPINFO, *PNMHVDISPINFO;

typedef struct _NMHEXVIEW {
    NMHDR NmHdr;
    HVITEM Item;
} NMHEXVIEW, *PNMHEXVIEW;

typedef struct _BYTERANGE {
    SIZE_T Min;
    SIZE_T Max;
} BYTERANGE, *PBYTERANGE;

HWND
CreateHexView(
    _In_ HWND hWndParent
    );

VOID
RegisterClassHexView(
    VOID
    );
