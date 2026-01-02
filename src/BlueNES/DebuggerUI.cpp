#include "DebuggerUI.h"
#include <Windows.h>
#include <commctrl.h>
#include "Core.h"
#include <string>
#include "resource.h"
#include "DebuggerContext.h"
#include "Bus.h"
#include "SharedContext.h"

#pragma comment(lib, "comctl32.lib")

DebuggerUI::DebuggerUI(HINSTANCE hInst, Core& core) : hInst(hInst), _core(core) {
	_bus = _core.emulator.GetBus();
	log = (uint8_t*)malloc(0x10000); // 64KB log buffer
    dbgCtx = _core.context.debugger_context;
	sharedCtx = &_core.context;
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

void DebuggerUI::RedrawVisibleRange() {
    // 1. Get the index of the first visible item
    int topIndex = ListView_GetTopIndex(hList);

    // 2. Get how many items fit in the current window height
    int countPerPage = ListView_GetCountPerPage(hList);

    // 3. Calculate the bottom index (plus a small buffer of 1 or 2 for safety)
    int bottomIndex = topIndex + countPerPage + 1;

    // 4. Tell Windows to invalidate only this specific range
    // This triggers LVN_GETDISPINFO for only these items.
    ListView_RedrawItems(hList, topIndex, bottomIndex);
}

std::wstring DebuggerUI::StringToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

void DebuggerUI::ComputeDisplayMap() {
    displayList.clear();
    displayMap.clear();

    for (int i = 0; i < 0x10000; ++i) {
        if (dbgCtx->memory_metadata[i] == META_CODE) {
            displayList.push_back(i);
			displayMap[i] = displayList.size() - 1;
        }
	}
    // Set the virtual count to the display map.
    ListView_SetItemCountEx(hList, displayList.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
}

void DebuggerUI::FocusPC(uint16_t pc) {
    // Find which index in our map corresponds to the current PC
    auto it = std::find(displayList.begin(), displayList.end(), pc);
    if (it != displayList.end()) {
        int index = std::distance(displayList.begin(), it);

        // Ensure the item is visible (scrolls if necessary)
        ListView_EnsureVisible(hList, index, FALSE);

        // Optionally redraw just this area
        RedrawVisibleRange();
    }
}

LRESULT DebuggerUI::HandleCustomDraw(LPNMLVCUSTOMDRAW lplvcd, const std::vector<uint16_t>& displayMap) {
    switch (lplvcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW; // We want to be notified per item

    case CDDS_ITEMPREPAINT: {
  //      uint16_t itemIdx = displayMap[lplvcd->nmcd.dwItemSpec];
		//uint16_t itemAddr = displayList[itemIdx];
        uint16_t itemAddr = displayList[lplvcd->nmcd.dwItemSpec];

        // If this is the current Program Counter, highlight it!
        if (itemAddr == dbgCtx->lastState.pc) {
            lplvcd->clrTextBk = RGB(255, 255, 0); // Yellow background
            lplvcd->clrText = RGB(0, 0, 0);       // Black text
        }
        // If there is a breakpoint here, color it red
        else if (dbgCtx->HasBreakpoint(itemAddr)) {
            lplvcd->clrTextBk = RGB(200, 0, 0);   // Dark Red background
            lplvcd->clrText = RGB(255, 255, 255); // White text
        }
        return CDRF_NEWFONT;
    }
    }
    return CDRF_DODEFAULT;
}

LRESULT CALLBACK DebuggerUI::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        DebuggerUI* pMain = (DebuggerUI*)pcs->lpCreateParams;

        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&icex);
        HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_WRAPABLE,
            0, 0, 0, 0, hwnd, (HMENU)IDR_TOOLBAR1, pMain->hInst, NULL);

        // Essential: Tell the toolbar how big the TBBUTTON structure is
        SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

        // iBitmap: The index of the icon in your bitmap strip (0, 1, 2...)
        // idCommand: The ID we defined in resource.h
        // fsState: TBSTATE_ENABLED makes it clickable
        // fsStyle: BTNS_BUTTON for standard, BTNS_SEP for a space between buttons

        TBBUTTON tbButtons[] = {
            { 0, IDB_PLAY,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Play" },
            { 1, IDB_PAUSE,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Pause" },
            //{ 2, ID_STOP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Stop" },
            { 0, 0,          TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0 }, // Separator gap
            { 3, IDB_FILLED_CIRCLE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Breakpoint" }
        };
        int numButtons = sizeof(tbButtons) / sizeof(TBBUTTON);
        SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)numButtons, (LPARAM)&tbButtons);
        SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);

        
        
        pMain->hList = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEW,
            nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SINGLESEL,
            10, 120, 860, 520,
            hwnd, (HMENU)1001, pMain->hInst, nullptr
        );

        LVCOLUMN col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        wchar_t bpText[] = L"BP";
        col.cx = 40; col.pszText = bpText;
        ListView_InsertColumn(pMain->hList, 0, &col);

        wchar_t lineText[] = L"Line";
        col.cx = 70; col.pszText = lineText;
        ListView_InsertColumn(pMain->hList, 1, &col);

        wchar_t addrText[] = L"Addr";
        col.cx = 80; col.pszText = addrText;
        ListView_InsertColumn(pMain->hList, 2, &col);

        wchar_t instText[] = L"Instruction";
        col.cx = 600; col.pszText = instText;
        ListView_InsertColumn(pMain->hList, 3, &col);

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
            switch (message) {
            case WM_SYSKEYDOWN:
                switch (wParam)
                {
                    case VK_F10: {
                        pMain->dbgCtx->step_requested.store(true);
                        // TODO Improve by using a conditional variable or event
                        //Sleep(.1);
                        pMain->FocusPC(pMain->dbgCtx->lastState.pc);
                        pMain->RedrawVisibleRange();

                        //CommandQueue::Command cmd;
                        //cmd.type = CommandQueue::CommandType::STEP_OVER;
                        //pMain->sharedCtx->command_queue.Push(cmd);
                        return 0; // Return 0 to tell Windows we handled it. Otherwise we never get it again.
                    } break;
                }
                return DefWindowProc(hwnd, message, wParam, lParam);
                break;
            case WM_KEYDOWN:
            {
                return DefWindowProc(hwnd, message, wParam, lParam);
            }
            break;
            case WM_NOTIFY: {
				// Only handle if paused so we don't slow down emulation
                if (!pMain->_core.isPlaying || !pMain->dbgCtx->is_paused) {
                    return 1;
                }
                LPNMHDR lpnmh = (LPNMHDR)lParam;
                if (lpnmh->code == NM_CUSTOMDRAW) {

                    // 3. Cast to the specific CustomDraw structure
                    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

                    // 4. Call your handler and return its result directly to Windows
                    return pMain->HandleCustomDraw(lplvcd, pMain->displayList);
                }
                else if (lpnmh->idFrom == 1001) // Our ListView control
                {
                    if (lpnmh->code == LVN_GETDISPINFO)
                    {
                        NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)lParam;
                        int index = pDispInfo->item.iItem;
                        int subItem = pDispInfo->item.iSubItem;
                        // Fill in the item text based on index and subItem
                        wchar_t buffer[256];
                        switch (subItem)
                        {
                        case 0: // BP column
                            swprintf_s(buffer, L""); // Placeholder for breakpoint indicator
                            //buffer[0] = L'\0';
                            break;
                        case 1: // Line column
                            //swprintf_s(buffer, L"%d", index + 1); // Line number (1-based)
                            swprintf_s(buffer, L""); // Address in hex
                            //buffer[0] = L'\0';
                            break;
                        case 2: { // Addr column
                            uint16_t displayAddr = pMain->displayList[index];
                            swprintf_s(buffer, L"%04X", displayAddr); // Address in hex
                            //swprintf_s(buffer, L""); // Address in hex
                        } break;
                        case 3: { // Instruction column
                            // Placeholder instruction text
                            //if (pMain->dbgCtx->memory_metadata[index] == META_CODE) {
                            uint16_t displayAddr = pMain->displayList[index];
                            swprintf_s(buffer, L"%S", pMain->_opcodeNames[pMain->_bus->peek(displayAddr)].c_str()); // Address in hex
                            //}
                            //else {
                                //swprintf_s(buffer, L""); // Address in hex
                                //buffer[0] = L'\0';
                            //}
                        } break;
                        default:
                            //buffer[0] = L'\0';
                            swprintf_s(buffer, L"");
                            break;
                        }
                        
                        pDispInfo->item.pszText = buffer;
                    }
				}
            } break;
            case WM_COMMAND: {
                switch (LOWORD(wParam))
                {
                    case IDB_PLAY:
                        pMain->dbgCtx->is_paused.store(false);
                    break;
                    case IDB_PAUSE:
                        pMain->dbgCtx->is_paused.store(true);
                        pMain->ComputeDisplayMap();
                        pMain->FocusPC(pMain->dbgCtx->lastState.pc);
                        pMain->RedrawVisibleRange();
                        // TODO Improve by using a conditional variable or event
                        Sleep(5);
                        break;
                }
            } break;
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