#include <windows.h>
#include <commctrl.h>    
#include "resource.h"
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>

#pragma comment(lib,"Comctl32.lib")

using namespace std;
using namespace nlohmann;

HINSTANCE hInst;
HWND hList;
HWND hDlgMain;
SYSTEMTIME stMain;
char* time_event;
char* text;
char* status;
int iSelect;
int gCount;
int lstId = 0;
json jsonObj;

BOOL CALLBACK DlgDiary(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgEdit(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgAdd(HWND, UINT, WPARAM, LPARAM);
int CreateColumn(HWND, int, LPSTR, int, int);
int AddItems(HWND, LPWSTR, LPSTR, LPSTR, LPSTR);
char* GetDate(SYSTEMTIME st);
void openFile();
string UTF8ToANSI(string);
string ANSItoUTF8(string);
void checkDate(string);
void saveToFile();

struct Record
{
    int id;
    string date;
    string time;
    string text;
    string status;
};

vector <Record> records;

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
    string id;

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
        CreateColumn(hList, 0, (LPSTR)"id", 0, LVCFMT_LEFT);
        CreateColumn(hList, 1, (LPSTR)"Время", 80, LVCFMT_LEFT);
        CreateColumn(hList, 2, (LPSTR)"Событие", rect.right - 180, LVCFMT_LEFT);
        CreateColumn(hList, 3, (LPSTR)"Статус", 100, LVCFMT_LEFT);

        openFile();

        if (!jsonObj.is_null())
        {
            gCount = jsonObj["count"];

            for (int i = 0; i < gCount; i++)
            {
                records.push_back(Record());
                records[i].id = jsonObj["data"][i]["id"];
                records[i].date = jsonObj["data"][i]["date"];
                records[i].time = jsonObj["data"][i]["time"];
                records[i].text = UTF8ToANSI(jsonObj["data"][i]["text"]);
                records[i].status = UTF8ToANSI(jsonObj["data"][i]["status"]);

                lstId = records[i].id + 1;
            }
        }

        MonthCal_GetCurSel(GetDlgItem(hWnd, IDC_CALENDAR), &stMain);
        checkDate(GetDate(stMain));

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
                {
                    ListView_GetItemText(hList, iSelect, 0, (LPSTR)id.c_str(), 256);

                    for (int i = 0; i < gCount; i++)
                    {
                        if (records[i].id == stoi(id))
                        {
                            records.erase(records.begin() + i);
                            gCount--;
                        }
                    }
                 
                    SendMessage(hList, LVM_DELETEITEM, iSelect, 0);
                    saveToFile();
                }
            }
            else
                MessageBox(hWnd, "   Выберите запись для удаления!", "Ошибка", MB_ICONERROR);

            ::SetFocus(hList);
            break;

        case IDC_BTN_DEL_ALL:
            checkPressBtn = MessageBox(hWnd, "   Вы действительно хотите удалить все записи?", "Подтверждение удаления", MB_ICONASTERISK | MB_YESNO);

            if (checkPressBtn == IDYES)
            {
                MonthCal_GetCurSel(GetDlgItem(hWnd, IDC_CALENDAR), &stMain);
                for (int i = gCount - 1; i >= 0; i--)
                {
                    if (records[i].date == GetDate(stMain))
                    {
                        records.erase(records.begin() + i);
                        gCount--;
                    }
                }
                SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);

                saveToFile();
            }

            ::SetFocus(hList);
            break;

        case IDC_BTN_DONE:
            iSelect = SendMessage(hList, LVM_GETNEXTITEM,
                -1, LVIS_SELECTED);

            if (iSelect >= 0)
            {
                ListView_GetItemText(hList, iSelect, 3, buffer, 256);
                
                if (strcmp(buffer, "Выполнено"))
                {
                    checkPressBtn = MessageBox(hWnd, "   Вы действительно выполнили цель?", "Подтверждение выполнения", MB_ICONASTERISK | MB_YESNO);

                    if (checkPressBtn == IDYES)
                    {
                        ListView_GetItemText(hList, iSelect, 0, (LPSTR)id.c_str(), 256);
                        for (int i = 0; i < gCount; i++)
                        {
                            if (records[i].id == stoi(id))
                            {
                                records[i].status = "Выполнено";
                                break;
                            }
                        }
                        ListView_SetItemText(hList, iSelect, 3, (LPSTR)"Выполнено");
                        saveToFile();
                    }
                }
                else
                    MessageBox(hWnd, "   Цель уже была выполнена!", "Ошибка", MB_ICONERROR | MB_OK);
            }
            else
                MessageBox(hWnd, "   Выберите запись для подтверждения!", "Ошибка", MB_ICONERROR);

            ::SetFocus(hList);
            break;

        case IDC_BTN_ADD:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_DLG_ADD), hWnd, DlgAdd);
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case MCN_SELECT:
            MonthCal_GetCurSel(GetDlgItem(hWnd, IDC_CALENDAR), &stMain);
            checkDate(GetDate(stMain));
            break;

        case NM_DBLCLK:
            iSelect = SendMessage(hList, LVM_GETNEXTITEM,
                -1, LVIS_SELECTED);

            if (iSelect >= 0)
            {
                time_event = new char[256];
                text = new char[256];
                status = new char[256];

                ListView_GetItemText(hList, iSelect, 1, time_event, 256);
                ListView_GetItemText(hList, iSelect, 2, text, 256);
                ListView_GetItemText(hList, iSelect, 3, status, 256);

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

void openFile()
{
    ifstream file;
    file.open("data.json");

    if (file.is_open())
    {
        file >> jsonObj;
    }
    file.close();
}

int AddItems(HWND hwndList, LPWSTR Text1, LPSTR Text2, LPSTR Text3, LPSTR Text4)
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
        ListView_SetItemText(hwndList, Ret, 3, Text4);
    }

    SetFocus(hwndList);
    return Ret;
}

BOOL CALLBACK DlgEdit(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    string hour;
    string min;
    string id;
    int check;
    string txt_s;

    switch(msg)
    {
    case WM_INITDIALOG:
        hour = time_event[0];
        min = time_event[3];
        hour += time_event[1];
        min += time_event[4];
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
            time_event = &hour[0];

            GetDlgItemText(hWnd, IDC_TEXT, text, 256);

            check = IsDlgButtonChecked(hWnd, IDC_STATUS);

            if (check)
                status = (char*)"Выполнено";
            else
                status = (char*)"Не выполнено";

            txt_s = text;
            if (txt_s != "")
            {
                ListView_GetItemText(hList, iSelect, 0, (LPSTR)id.c_str(), 256);
                for (int i = 0; i < gCount; i++)
                {
                    if (records[i].id == stoi(id))
                    {
                        records[i].time = time_event;
                        records[i].text = text;
                        records[i].status = status;
                        break;
                    }
                }

                ListView_SetItemText(hList, iSelect, 1, time_event);
                ListView_SetItemText(hList, iSelect, 2, text);
                ListView_SetItemText(hList, iSelect, 3, status);

                /*SendMessage(hList, LVM_SORTITEMS, NULL, NULL);*/

                EndDialog(hWnd, NULL);
                ::SetFocus(hList);

                saveToFile();
            }
            else
                MessageBox(hWnd, "   Введите данные в поле \"Событие\"!", "Ошибка", MB_ICONERROR | MB_OK);

            return TRUE;

        case IDC_BTN_CNCL:
            EndDialog(hWnd, NULL);
            ::SetFocus(hList);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hWnd, NULL);
        return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK DlgAdd(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    string hour;
    string min;
    char *txt = new char[256];
    string txt_s;
    string date;

    switch (msg)
    {
    case WM_INITDIALOG:
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_ACPT_ADD:
            date = GetDate(stMain);
            DateTime_GetSystemtime(GetDlgItem(hWnd, IDC_TIME_ADD), &stMain);
            GetDlgItemText(hWnd, IDC_TEXT_ADD, txt, 256);

            hour = to_string(stMain.wHour);
            min = to_string(stMain.wMinute);

            if (hour.size() == 1)
                hour = '0' + hour;

            if (min.size() == 1)
                min = '0' + min;

            hour += ":" + min;
            time_event = &hour[0];

            txt_s = txt;
            if (txt_s != "")
            {
                records.push_back(Record());
                records[gCount].id = lstId++;
                records[gCount].date = date;
                records[gCount].time = time_event;
                records[gCount].text = txt;
                records[gCount].status = "Не выполнено";

                AddItems(hList, (LPWSTR)to_string(records[gCount].id).c_str(), (LPSTR)records[gCount].time.c_str(), (LPSTR)records[gCount].text.c_str(), (LPSTR)records[gCount].status.c_str());

                //SendMessage(hList, LVM_SORTITEMS, NULL, NULL);

                gCount++;
                EndDialog(hWnd, NULL);
                ::SetFocus(hList);

                saveToFile();
            }
            else
                MessageBox(hWnd, "   Введите данные в поле \"Событие\"!", "Ошибка", MB_ICONERROR | MB_OK);

            return TRUE;

        case IDC_BTN_CNCL_ADD:
            EndDialog(hWnd, NULL);
            ::SetFocus(hList);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hWnd, NULL);
        return TRUE;
    }
    return FALSE;
}

string UTF8ToANSI(string s)
{
    BSTR    bstrWide;
    char* pszAnsi;
    int     nLength;
    const char* pszCode = s.c_str();

    nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, NULL, NULL);
    bstrWide = SysAllocStringLen(NULL, nLength);

    MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);

    nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
    pszAnsi = new char[nLength];

    WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
    SysFreeString(bstrWide);

    string r(pszAnsi);
    delete[] pszAnsi;
    return r;
}

string ANSItoUTF8(string s)
{
    BSTR    bstrWide;
    char* pszAnsi;
    int     nLength;
    const char* pszCode = s.c_str();

    nLength = MultiByteToWideChar(CP_ACP, 0, pszCode, strlen(pszCode) + 1, NULL, NULL);
    bstrWide = SysAllocStringLen(NULL, nLength);

    MultiByteToWideChar(CP_ACP, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);

    nLength = WideCharToMultiByte(CP_UTF8, 0, bstrWide, -1, NULL, 0, NULL, NULL);
    pszAnsi = new char[nLength];

    WideCharToMultiByte(CP_UTF8, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
    SysFreeString(bstrWide);

    string r(pszAnsi);
    delete[] pszAnsi;
    return r;
}

void checkDate(string date)
{
    SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);

    for (int i = 0; i < gCount; i++)
    {
        if (records[i].date == date)
            AddItems(hList, (LPWSTR)to_string(records[i].id).c_str(), (LPSTR)records[i].time.c_str(), (LPSTR)records[i].text.c_str(), (LPSTR)records[i].status.c_str());
    }
}

void saveToFile()
{
    ofstream file;

    jsonObj.clear();
    jsonObj["count"] = gCount;

    for (int i = 0; i < gCount; i++)
    {
        jsonObj["data"][i] = { {"id", records[i].id}, {"date", records[i].date}, {"time", records[i].time}, {"text", ANSItoUTF8(records[i].text)}, {"status", ANSItoUTF8(records[i].status)} };
    }

    file.open("data.json");

    if (file.is_open())
    {
        file << jsonObj;
    }
    file.close();
}
