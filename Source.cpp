#include <windows.h>
#include <commctrl.h>    
#pragma comment(lib,"Comctl32.lib")
#include "resource.h"
#include <string>

using namespace std;

HINSTANCE hInst;
HWND hList;
HWND hDlgMain;
SYSTEMTIME stMain;
char* time;
char* text;
char* status;
int iSelect;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgDiary(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgEdit(HWND, UINT, WPARAM, LPARAM);
int CreateColumn(HWND, int, LPSTR, int, int);
int AddItems(HWND, LPWSTR, LPSTR, LPSTR);
char* GetDate(SYSTEMTIME st);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    hDlgMain = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIARY), NULL, DlgDiary);
    ShowWindow(hDlgMain, nCmdShow);
    UpdateWindow(hDlgMain);

    while (GetMessage(&msg, NULL, NULL, NULL)) 
    {
        hInst = hInstance;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

BOOL CALLBACK DlgDiary(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    HICON hIcon;
    char* buffer = new char[256];
    int checkPressBtn;

    switch (msg)
    {
    case WM_INITDIALOG:
        hList = GetDlgItem(hWnd, IDC_LIST);
        MonthCal_GetCurSel(GetDlgItem(hWnd, IDC_CALENDAR), &stMain);
        SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE,
            0, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES);

        hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
            MAKEINTRESOURCEW(IDI_ICON),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            0);
        if (hIcon)
        {
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }

        GetClientRect(hList, &rect);
        CreateColumn(hList, 1, (LPSTR)"Время", 80, LVCFMT_LEFT);
        CreateColumn(hList, 2, (LPSTR)"Событие", rect.right - 180, LVCFMT_LEFT);
        CreateColumn(hList, 3, (LPSTR)"Статус", 100, LVCFMT_LEFT);

        AddItems(hList, (LPWSTR)"11:47", (LPSTR)"Встреча с деловым партнёром", (LPSTR)"Не выполнено");
        AddItems(hList, (LPWSTR)"09:30", (LPSTR)"Утренняя пробежка", (LPSTR)"Выполнено");
        AddItems(hList, (LPWSTR)"16:00", (LPSTR)"Выгулять собаку", (LPSTR)"Не выполнено");
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_DEL:
            iSelect = SendMessage(hList, LVM_GETNEXTITEM,
                -1, LVIS_SELECTED);

            if (iSelect >= 0)
            {
                checkPressBtn = MessageBox(hWnd, "   Вы действительно хотите удалить запись?", "Подтверждение удаления", MB_ICONASTERISK | MB_YESNO);

                if (checkPressBtn == IDYES)
                    SendMessage(hList, LVM_DELETEITEM, iSelect, 0);
            }
            else
                MessageBox(hWnd, "   Выберите запись для удаления!", "Ошибка", MB_ICONERROR);

            break;

        case IDC_BTN_DEL_ALL:
            checkPressBtn = MessageBox(hWnd, "   Вы действительно хотите удалить все записи?", "Подтверждение удаления", MB_ICONASTERISK | MB_YESNO);

            if (checkPressBtn == IDYES)
                SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);
            break;

        case IDC_BTN_DONE:
            iSelect = SendMessage(hList, LVM_GETNEXTITEM,
                -1, LVIS_SELECTED);

            if (iSelect >= 0)
            {
                ListView_GetItemText(hList, iSelect, 2, buffer, 256);
                
                if (strcmp(buffer, "Выполнено"))
                {
                    checkPressBtn = MessageBox(hWnd, "   Вы действительно выполнили цель?", "Подтверждение выполнения", MB_ICONASTERISK | MB_YESNO);

                    if (checkPressBtn == IDYES)
                        ListView_SetItemText(hList, iSelect, 2, (LPSTR)"Выполнено");
                }
                else
                    MessageBox(hWnd, "   Цель уже была выполнена!", "Ошибка", MB_ICONERROR | MB_OK);
            }
            else
                MessageBox(hWnd, "   Выберите запись для подтверждения!", "Ошибка", MB_ICONERROR);
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case MCN_SELECT:
            MonthCal_GetCurSel(GetDlgItem(hWnd, IDC_CALENDAR), &stMain);
            OutputDebugString(GetDate(stMain));
            OutputDebugString("\n");
            break;

        case NM_DBLCLK:
            iSelect = SendMessage(hList, LVM_GETNEXTITEM,
                -1, LVIS_SELECTED);

            if (iSelect >= 0)
            {
                time = new char[256];
                text = new char[256];
                status = new char[256];

                ListView_GetItemText(hList, iSelect, 0, time, 256);
                ListView_GetItemText(hList, iSelect, 1, text, 256);
                ListView_GetItemText(hList, iSelect, 2, status, 256);

                DialogBox(hInst, MAKEINTRESOURCE(IDD_DLG_EDIT), hWnd, DlgEdit);
            }
        }
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        return TRUE;
    }
    return FALSE;
}

int CreateColumn(HWND hwndLV, int iCol, LPSTR text, int width, int pos)
{
    LVCOLUMN lvc;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = pos;
    lvc.cx = width;
    lvc.pszText = text;
    lvc.iSubItem = iCol;
    return ListView_InsertColumn(hwndLV, iCol, &lvc);
}

char* GetDate(SYSTEMTIME st)
{
    char* date = new char[255];
    GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, "dd.MM.yyyy", date, 255);

    return date;
}

int AddItems(HWND hwndList, LPWSTR Text1, LPSTR Text2, LPSTR Text3)
{
    LVITEMW lvi = { 0 };
    int Ret;
    lvi.mask = LVIF_TEXT;
    lvi.pszText = Text1;
    Ret = ListView_InsertItem(hwndList, &lvi);
    if (Ret >= 0)
    {
        ListView_SetItemText(hwndList, Ret, 1, Text2);
        ListView_SetItemText(hwndList, Ret, 2, Text3);
    }

    SetFocus(hwndList);
    return Ret;
}

BOOL CALLBACK DlgEdit(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    string hour;
    string min;
    int check;

    switch(msg)
    {
    case WM_INITDIALOG:
        hour = time[0];
        min = time[3];
        hour += time[1];
        min += time[4];
        stMain.wHour = stoi(hour);
        stMain.wMinute = stoi(min);
        stMain.wSecond = 0;
        stMain.wMilliseconds = 0;
        DateTime_SetSystemtime(GetDlgItem(hWnd, IDC_TIME), GDT_VALID, &stMain);

        SetDlgItemText(hWnd, IDC_TEXT, text);

        if (!strcmp(status, "Выполнено"))
            SendMessage(GetDlgItem(hWnd, IDC_STATUS), BM_SETCHECK, 1, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_ACPT:
            DateTime_GetSystemtime(GetDlgItem(hWnd, IDC_TIME), &stMain);
            hour = to_string(stMain.wHour);
            min = to_string(stMain.wMinute);

            if (hour.size() == 1)
                hour = '0' + hour;

            if (min.size() == 1)
                min = '0' + min;

            hour += ":" + min;
            time = &hour[0];

            GetDlgItemText(hWnd, IDC_TEXT, text, 256);

            check = IsDlgButtonChecked(hWnd, IDC_STATUS);

            if (check)
                status = (char*)"Выполнено";
            else
                status = (char*)"Не выполнено";

            ListView_SetItemText(hList, iSelect, 0, time);
            ListView_SetItemText(hList, iSelect, 1, text);
            ListView_SetItemText(hList, iSelect, 2, status);

            SendMessage(hList, LVM_SORTITEMS, NULL, NULL);

            EndDialog(hWnd, NULL);
            return TRUE;

        case IDC_BTN_CNCL:
            EndDialog(hWnd, NULL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hWnd, NULL);
        return TRUE;
    }
    return FALSE;
}
