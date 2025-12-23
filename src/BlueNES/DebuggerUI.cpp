#include "DebuggerUI.h"
#include <Windows.h>
#include <commctrl.h>
#include "Core.h"
#include <string>

DebuggerUI::DebuggerUI(HINSTANCE hInst, Core& core) : hInst(hInst), _core(core) {
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = DebuggerUI::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = hInst;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
    wcex.lpszClassName = L"BlueDebugger";

    RegisterClassEx(&wcex);

    hDebuggerWnd = CreateWindowEx(
        0,
        L"BlueDebugger",
        L"Blue NES Debugger",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 900, 700,
        nullptr, nullptr, hInst, this
    );

    ShowWindow(hDebuggerWnd, SW_SHOW);
}

LRESULT CALLBACK DebuggerUI::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        DebuggerUI* pMain = (DebuggerUI*)pcs->lpCreateParams;
        HWND hList = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEW,
            nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL,
            10, 120, 860, 520,
            hwnd, (HMENU)1001, pMain->hInst, nullptr
        );

        LVCOLUMN col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        wchar_t bpText[] = L"BP";
        col.cx = 40; col.pszText = bpText;
        ListView_InsertColumn(hList, 0, &col);

        wchar_t lineText[] = L"Line";
        col.cx = 70; col.pszText = lineText;
        ListView_InsertColumn(hList, 1, &col);

        wchar_t addrText[] = L"Addr";
        col.cx = 80; col.pszText = addrText;
        ListView_InsertColumn(hList, 2, &col);

        wchar_t instText[] = L"Instruction";
        col.cx = 600; col.pszText = instText;
        ListView_InsertColumn(hList, 3, &col);

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pMain)
        );
        result = 1;
    }
    else
    {
        DebuggerUI* pMain = reinterpret_cast<DebuggerUI*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (pMain)
        {
            switch (message)
            {
            case WM_KEYDOWN:
            {
                switch (wParam)
                {
                case VK_F10:
                    
                    break;
                }
                break;
            }

            case WM_DISPLAYCHANGE:
            {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            result = 0;
            wasHandled = true;
            break;

            //case WM_PAINT:
            //{
            //    PAINTSTRUCT ps;
            //    HDC hdc = BeginPaint(hwnd, &ps);
            //    //pMain->RenderFrame();
            //    EndPaint(hwnd, &ps);
            //    ValidateRect(hwnd, NULL);
            //}
            //result = 0;
            //wasHandled = true;
            //break;

            case WM_DESTROY:
            {
                //PostQuitMessage(0);
				ShowWindow(hwnd, SW_HIDE);
            }
            result = 1;
            wasHandled = true;
            break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}