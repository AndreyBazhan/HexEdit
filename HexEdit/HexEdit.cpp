/*++

Copyright (c) Andrey Bazhan

--*/

#include "stdafx.h"
#include "HexEdit.h"
#include "HexView.h"

#define APPNAME                 _T("HexEdit")
#define WEBSITE                 _T("www.andreybazhan.com")
#define OPTIONS_KEY             _T("Software\\Andrey Bazhan\\") APPNAME
#define MAINWINDOW_VALUE        _T("MainWindow")

HWND            hMain;
HWND            hHexView;

OPTIONS         Options;

HANDLE          hFile;
HANDLE          hFileMap;
ULONG_PTR       FileBase;
LARGE_INTEGER   FileSize;

TCHAR           FilePath[MAX_PATH];


VOID
SetWindowTitleText(
    _In_z_ PCTSTR Text
    )
{
    TCHAR Title[MAX_PATH * 2];

    StringCchPrintf(Title,
                    _countof(Title),
                    _T("%s %s"),
                    APPNAME,
                    Text);

    SetWindowText(hMain, Title);
}

VOID
FileClose(
    VOID
    )
{
    if (FileBase && hFileMap && hFile) {

        SendMessage(hHexView, HVM_SETITEMCOUNT, 0, 0);

        UnmapViewOfFile((LPCVOID)FileBase);
        FileBase = NULL;

        CloseHandle(hFileMap);
        hFileMap = 0;

        CloseHandle(hFile);
        hFile = 0;

        SetWindowText(hMain, APPNAME);

        SetFocus(hMain);
    }
}

VOID
FileOpen(
    VOID
    )
{
    FileClose();

    hFile = CreateFile(FilePath,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       0,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       0);

    if (INVALID_HANDLE_VALUE != hFile) {

        GetFileSizeEx(hFile, &FileSize);

        hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

        if (hFileMap) {

            FileBase = (ULONG_PTR)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);

            if (FileBase) {

                DWORD Style = 0;

                Style |= FileSize.HighPart ? HVS_ADDRESS64 : 0;
                Style |= HVS_READONLY;

                SendMessage(hHexView, HVM_SETEXTENDEDSTYLE, 0, Style);
                SendMessage(hHexView, HVM_SETITEMCOUNT, 0, (LPARAM)FileSize.LowPart);

                SetWindowTitleText(FilePath);
            }
            else {

                CloseHandle(hFileMap);
                CloseHandle(hFile);
            }
        }
        else {

            CloseHandle(hFile);
        }
    }
}

VOID
GetSettings(
    VOID
    )
{
    HKEY hKey;
    DWORD cbData;

    //
    // Set default options
    //

    ZeroMemory(&Options, sizeof(OPTIONS));

    Options.WindowPlacement.rcNormalPosition.left = CW_USEDEFAULT;
    Options.WindowPlacement.rcNormalPosition.top = CW_USEDEFAULT;
    Options.WindowPlacement.rcNormalPosition.right = CW_USEDEFAULT;
    Options.WindowPlacement.rcNormalPosition.bottom = CW_USEDEFAULT;
    Options.WindowPlacement.showCmd = SW_SHOWNORMAL;

    //
    // Get options
    //

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, OPTIONS_KEY, 0, KEY_READ, &hKey)) {

        cbData = sizeof(Options.WindowPlacement);

        RegQueryValueEx(hKey, MAINWINDOW_VALUE, NULL, NULL, (LPBYTE)&Options.WindowPlacement, &cbData);

        RegCloseKey(hKey);
    }
}

VOID
SaveSettings(
    VOID
    )
{
    HKEY hKey;

    Options.WindowPlacement.length = sizeof(WINDOWPLACEMENT);

    GetWindowPlacement(hMain, &Options.WindowPlacement);

    Options.WindowPlacement.rcNormalPosition.right = Options.WindowPlacement.rcNormalPosition.right - Options.WindowPlacement.rcNormalPosition.left;
    Options.WindowPlacement.rcNormalPosition.bottom = Options.WindowPlacement.rcNormalPosition.bottom - Options.WindowPlacement.rcNormalPosition.top;

    if (Options.WindowPlacement.showCmd == SW_SHOWMINIMIZED) {

        Options.WindowPlacement.showCmd = SW_SHOWNORMAL;
    }

    //
    // Save options
    //

    if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, OPTIONS_KEY, &hKey)) {

        RegSetValueEx(hKey, MAINWINDOW_VALUE, 0, REG_BINARY, (LPBYTE)&Options.WindowPlacement, sizeof(Options.WindowPlacement));

        RegCloseKey(hKey);
    }
}

INT_PTR
CALLBACK
AboutProc(
    _In_ HWND hDlg,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (message) {

    case WM_INITDIALOG:
    {
        TCHAR FileName[MAX_PATH];
        TCHAR Text[MAX_PATH] = APPNAME _T(" ");
        DWORD Handle;
        PVOID Block = NULL;
        DWORD BlockSize;
        PVOID String = NULL;
        UINT Length;

        SetWindowText(hDlg, _T("About ") APPNAME);

        if (GetModuleFileName(NULL, FileName, _countof(FileName))) {

            BlockSize = GetFileVersionInfoSize(FileName, &Handle);

            if (BlockSize) {

                Block = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BlockSize);

                if (Block) {

                    GetFileVersionInfo(FileName, 0, BlockSize, Block);

                    if (VerQueryValue(Block,
                        _T("\\StringFileInfo\\040904b0\\FileVersion"),
                        &String,
                        &Length)) {

                        _tcsncpy_s(&Text[_tcslen(Text)], _countof(Text) - _tcslen(Text), (PCTSTR)String, Length);

                        SendMessage(GetDlgItem(hDlg, IDC_VERSION), WM_SETTEXT, 0, (LPARAM)Text);
                    }

                    if (VerQueryValue(Block,
                        _T("\\StringFileInfo\\040904b0\\LegalCopyright"),
                        &String,
                        &Length)) {

                        _tcsncpy_s(Text, _countof(Text), (PCTSTR)String, Length);

                        SendMessage(GetDlgItem(hDlg, IDC_COPYRIGHT), WM_SETTEXT, 0, (LPARAM)Text);
                    }

                    HeapFree(GetProcessHeap(), 0, Block);
                }
            }
        }

        SendMessage(GetDlgItem(hDlg, IDC_SYSLINK), WM_SETTEXT, 0, (LPARAM)_T("<a>") WEBSITE _T("</a>"));

        return TRUE;
    }
    case WM_NOTIFY:
    {
        LPNMHDR NmHdr = (LPNMHDR)lParam;

        if (NmHdr->hwndFrom == GetDlgItem(hDlg, IDC_SYSLINK)) {

            if (NmHdr->code == NM_CLICK) {

                ShellExecute(hDlg, _T("open"), WEBSITE, NULL, NULL, SW_SHOWNORMAL);
            }
        }

        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam)) {

        case IDOK:

            EndDialog(hDlg, 0);
            return TRUE;
        }

        break;
    }
    case WM_CLOSE:

        EndDialog(hDlg, 0);
        return TRUE;
    }

    return FALSE;
}

LRESULT
CALLBACK
WndProc(
    _In_ HWND hWnd,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    RECT rc;

    switch (message) {

    case WM_CREATE:
    {
        RegisterClassHexView();

        hHexView = CreateHexView(hWnd);

        break;
    }
    case WM_NOTIFY:
    {
        LPNMHDR NmHdr = (LPNMHDR)lParam;

        if (NmHdr->hwndFrom == hHexView) {

            if (NmHdr->code == HVN_GETDISPINFO) {

                PNMHVDISPINFO DispInfo = (PNMHVDISPINFO)lParam;

                if (DispInfo->Item.Mask & HVIF_ADDRESS) {

                    DispInfo->Item.Address = DispInfo->Item.NumberOfItem;
                }
                else if (DispInfo->Item.Mask & HVIF_BYTE) {

                    PBYTE Base = (PBYTE)(FileBase + DispInfo->Item.NumberOfItem);

                    DispInfo->Item.Value = *Base;

                    //
                    // Set state of the item.
                    //

                    if (DispInfo->Item.NumberOfItem >= 0 && DispInfo->Item.NumberOfItem <= 255) {

                        DispInfo->Item.State = HVIS_MODIFIED;
                    }
                }
            }
            else if (NmHdr->code == HVN_ITEMCHANGING) {

                PNMHEXVIEW HexView = (PNMHEXVIEW)lParam;

                PBYTE Base = (PBYTE)(FileBase + HexView->Item.NumberOfItem);

                *Base = HexView->Item.Value;
            }
        }

        break;
    }
    case WM_SIZE:
    {
        GetClientRect(hWnd, &rc);

        MoveWindow(hHexView,
                   rc.left,
                   rc.top,
                   rc.right - rc.left,
                   rc.bottom - rc.top,
                   TRUE);
        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam)) {

        case IDM_FILE_OPEN:
        {
            OPENFILENAME ofn = {0};

            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hWnd;
            ofn.hInstance = GetModuleHandle(NULL);
            ofn.lpstrFilter = _T("All Files(*.*)\0*.*\0Executable Files(*.exe)\0*.exe\0Dynamic Link Library(*.dll)\0*.dll\0System Files(*.sys)\0*.sys\0\0");
            ofn.lpstrFile = FilePath;
            ofn.nMaxFile = _countof(FilePath);
            ofn.lpstrTitle = _T("Open File");
            ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

            if (GetOpenFileName(&ofn)) {

                FileOpen();
            }

            break;
        }
        case IDM_FILE_CLOSE:

            FileClose();
            break;

        case IDM_ABOUT:

            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutProc);
            break;

        case IDM_EXIT:

            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        default:

            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
        }

        break;
    }
    case WM_DROPFILES:
    {
        if (DragQueryFile((HDROP)wParam, 0, FilePath, _countof(FilePath))) {

            FileOpen();
        }

        break;
    }
    case WM_SETFOCUS:

        SetFocus(hHexView);
        break;

    case WM_CLOSE:

        FileClose();

        SaveSettings();

        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:

        PostQuitMessage(0);
        break;

    default:

        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

int
APIENTRY
_tWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR lpCmdLine,
    _In_ int nCmdShow
    )
{
    MSG msg;
    WNDCLASSEX wcex = {0};
    HACCEL hAccel;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = 0;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSAPPLICATION));
    wcex.hCursor       = 0;
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName  = MAKEINTRESOURCE(IDC_WINDOWSAPPLICATION);
    wcex.lpszClassName = APPNAME;
    wcex.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSAPPLICATION));

    if (!RegisterClassEx(&wcex)) {

        MessageBox(0, _T("RegisterClassEx failed!"), APPNAME, MB_ICONERROR);
        return 0;
    }

    GetSettings();

    hMain = CreateWindowEx(WS_EX_ACCEPTFILES,
                           APPNAME,
                           APPNAME,
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                           Options.WindowPlacement.rcNormalPosition.left,
                           Options.WindowPlacement.rcNormalPosition.top,
                           Options.WindowPlacement.rcNormalPosition.right,
                           Options.WindowPlacement.rcNormalPosition.bottom,
                           0,
                           0,
                           hInstance,
                           NULL);

    if (!hMain) {

        MessageBox(0, _T("CreateWindowEx failed!"), APPNAME, MB_ICONERROR);
        return 0;
    }

    ShowWindow(hMain, Options.WindowPlacement.showCmd);
    UpdateWindow(hMain);

    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSAPPLICATION));

    while (GetMessage(&msg, NULL, 0, 0) > 0) {

        if (!TranslateAccelerator(hMain, hAccel, &msg)) {

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}