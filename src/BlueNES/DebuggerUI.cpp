#include "DebuggerUI.h"
#include <Windows.h>
#include <commctrl.h>
#include "Core.h"
#include <string>
#include "resource.h"
#include "DebuggerContext.h"
#include "Bus.h"
#include "SharedContext.h"
#include <sstream>
#include "imgui.h"

#pragma comment(lib, "comctl32.lib")

DebuggerUI::DebuggerUI(HINSTANCE hInst, Core& core) : hInst(hInst), _core(core) {
	_bus = _core.emulator.GetBus();
	log = (uint8_t*)malloc(0x10000); // 64KB log buffer
    dbgCtx = _core.context.debugger_context;
	sharedCtx = &_core.context;

}

//void DebuggerUI::RedrawVisibleRange() {
//    // 1. Get the index of the first visible item
//    int topIndex = ListView_GetTopIndex(hList);
//
//    // 2. Get how many items fit in the current window height
//    int countPerPage = ListView_GetCountPerPage(hList);
//
//    // 3. Calculate the bottom index (plus a small buffer of 1 or 2 for safety)
//    int bottomIndex = topIndex + countPerPage + 1;
//
//    // 4. Tell Windows to invalidate only this specific range
//    // This triggers LVN_GETDISPINFO for only these items.
//    ListView_RedrawItems(hList, topIndex, bottomIndex);
//}
//
//void DebuggerUI::RedrawItem(int idx) {
//    ListView_RedrawItems(hList, idx, idx);
//}

std::wstring DebuggerUI::StringToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

//void DebuggerUI::ComputeDisplayMap() {
//    displayList.clear();
//    displayMap.clear();
//
//    for (int i = 0; i < 0x10000; ++i) {
//        if (dbgCtx->memory_metadata[i] == META_CODE) {
//            displayList.push_back(i);
//			displayMap[i] = displayList.size() - 1;
//        }
//	}
//    // Set the virtual count to the display map.
//    ListView_SetItemCountEx(hList, displayList.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
//}

//void DebuggerUI::FocusPC(uint16_t pc) {
//    // Find which index in our map corresponds to the current PC
//    //auto it = std::find(displayList.begin(), displayList.end(), pc);
//	//auto it = displayMap.find(pc);
//    auto it = displayMap[pc];
//    
//    //if (it != displayList.end()) {
//        //int index = std::distance(displayList.begin(), it);
//
//        // Ensure the item is visible (scrolls if necessary)
//        //ListView_EnsureVisible(hList, index, FALSE);
//        ListView_EnsureVisible(hList, it, FALSE);
//
//        // Optionally redraw just this area
//        
//    //}
//}

//LRESULT DebuggerUI::HandleCustomDraw(LPNMLVCUSTOMDRAW lplvcd, const std::vector<uint16_t>& displayMap) {
//    switch (lplvcd->nmcd.dwDrawStage) {
//    case CDDS_PREPAINT:
//        return CDRF_NOTIFYITEMDRAW; // We want to be notified per item
//
//    case CDDS_ITEMPREPAINT: {
//  //      uint16_t itemIdx = displayMap[lplvcd->nmcd.dwItemSpec];
//		//uint16_t itemAddr = displayList[itemIdx];
//        uint16_t itemAddr = displayList[lplvcd->nmcd.dwItemSpec];
//        bool isSelected = (lplvcd->nmcd.uItemState & CDIS_SELECTED);
//
//        // If this is the current Program Counter, highlight it!
//        if (itemAddr == dbgCtx->lastState.pc) {
//            lplvcd->clrTextBk = RGB(255, 255, 0); // Yellow background
//            lplvcd->clrText = RGB(0, 0, 0);       // Black text
//        }
//        else if (isSelected) {
//            // If you don't set these, Windows uses the default Blue
//            lplvcd->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
//            lplvcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
//        }
//        // If there is a breakpoint here, color it red
//        else if (dbgCtx->HasBreakpoint(itemAddr)) {
//            lplvcd->clrTextBk = RGB(200, 0, 0);   // Dark Red background
//            lplvcd->clrText = RGB(255, 255, 255); // White text
//        }
//        return CDRF_NEWFONT;
//    }
//    }
//    return CDRF_DODEFAULT;
//}

//void DebuggerUI::StepInto() {
//	if (!dbgCtx->is_paused.load(std::memory_order_relaxed)) return;
//	uint16_t oldpc = dbgCtx->lastState.pc;
//    dbgCtx->step_requested.store(true);
//    // 1. Wait for the CPU to actually move
//    int timeout = 1000;
//    while (dbgCtx->lastState.pc == oldpc && timeout-- > 0) {
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }
//    FocusPC(dbgCtx->lastState.pc);
//	RedrawItem(displayMap[oldpc]);
//	RedrawItem(displayMap[dbgCtx->lastState.pc]);
//
//    //CommandQueue::Command cmd;
//    //cmd.type = CommandQueue::CommandType::STEP_OVER;
//    //pMain->sharedCtx->command_queue.Push(cmd);
//    // return 0; // Return 0 to tell Windows we handled it. Otherwise we never get it again.
//}

std::string DebuggerUI::Disassemble(uint16_t address) {
    uint8_t opcode = _bus->peek(address);
	std::stringstream ss;
	ss << _opcodeNames[opcode];
    switch (instMode[opcode]) {
    case IMM: {
        uint8_t value = _bus->peek(address + 1);
        ss << " #$" << std::uppercase << std::hex << (int)value;
    } break;
    case ZP: {
        uint8_t addr = _bus->peek(address + 1);
		ss << " $" << std::uppercase << std::hex << (int)addr;
	} break;
    case ZPX: {
        uint8_t addr = _bus->peek(address + 1);
		ss << " $" << std::uppercase << std::hex << (int)addr << ",X ($" << std::uppercase << std::hex << (int)dbgCtx->lastState.x << ")";
	} break;
    case ZPY: {
		uint8_t addr = _bus->peek(address + 1);
		ss << " $" << std::uppercase << std::hex << (int)addr << ",Y ($" << std::uppercase << std::hex << (int)dbgCtx->lastState.y << ")";
	} break;
	case ABS: {
		uint16_t addr = _bus->peek(address + 1) | (_bus->peek(address + 2) << 8);
		uint8_t value = _bus->peek(addr);
		ss << " $" << std::uppercase << std::hex << (int)addr << " = ($" << std::uppercase << std::hex << (int)value << ")";
	} break;
	case ABSX: {
		uint16_t addr = _bus->peek(address + 1) | (_bus->peek(address + 2) << 8);
		uint8_t value = _bus->peek(addr + dbgCtx->lastState.x);
		ss << " $" << std::uppercase << std::hex << (int)addr << ",X ($" << std::uppercase << std::hex << (int)dbgCtx->lastState.x << ")" << " = ($" << std::uppercase << std::hex << (int)value << ")";
	} break;
	case ABSY: {
		uint16_t addr = _bus->peek(address + 1) | (_bus->peek(address + 2) << 8);
		uint8_t value = _bus->peek(addr + dbgCtx->lastState.y);
		ss << " $" << std::uppercase << std::hex << (int)addr << ",Y ($" << std::uppercase << std::hex << (int)dbgCtx->lastState.y << ")" << " = ($" << std::uppercase << std::hex << (int)value << ")";
	} break;
	case IND: {
		uint16_t addr = _bus->peek(address + 1) | (_bus->peek(address + 2) << 8);
		uint16_t ptr = (_bus->peek((addr & 0xFF00) | ((addr + 1) & 0x00FF)) << 8);
		ss << " ($" << std::uppercase << std::hex << (int)addr << ")" << " = ($" << std::uppercase << std::hex << (int)ptr << ")";
	} break;
	case INDX: {
		uint8_t addr = _bus->peek(address + 1);
		uint8_t ptr = (uint8_t)(addr + dbgCtx->lastState.x);
		uint8_t ptrAddr = (_bus->peek((ptr & 0xFF00) | ((ptr + 1) & 0x00FF)) << 8);
		ss << " ($" << std::uppercase << std::hex << (int)addr << ",X $" << std::uppercase << std::hex << (int)dbgCtx->lastState.x << ") $" << std::uppercase << std::hex << (int)ptr << " = (" << (int)ptrAddr << ")";
	} break;
	case INDY: {
		uint8_t addr = _bus->peek(address + 1);
		uint8_t ptrAddr = (_bus->peek((addr & 0xFF00) | ((addr + 1) & 0x00FF)) << 8);
		ss << " ($" << std::uppercase << std::hex << (int)addr << "),Y( $" << std::uppercase << std::hex << (int)dbgCtx->lastState.y << ") = $" << std::uppercase << std::hex << (int)ptrAddr;
	} break;
	case REL: {
		int8_t offset = (int8_t)_bus->peek(address + 1);
		uint16_t target = address + 2 + offset;
		ss << " $" << std::uppercase << std::hex << target;
	} break;
    case ACC:
        ss << " A ($" << std::uppercase << std::hex << (int)dbgCtx->lastState.a << ")";
		break;
    }
    return ss.str();
}

void DebuggerUI::DrawScrollableDisassembler() {
    ImGui::Begin("Disassembler");

    // Use a table for easy alignment
    static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;

    // Set the height of the scrollable area (0 = use all available space)
    if (ImGui::BeginTable("DisasTable", 3, flags, ImVec2(0, 400))) {
        ImGui::TableSetupColumn("BP", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Instruction", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        // 65536 possible addresses (NES memory space)
        ImGuiListClipper clipper;
        clipper.Begin(65536);

        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                ImGui::TableNextRow();

                // --- COLUMN 0: Breakpoint Icon ---
                ImGui::TableSetColumnIndex(0);
                bool hasBreakpoint = dbgCtx->HasBreakpoint(i);
                if (hasBreakpoint) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "  O"); // Red circle for BP
                }

                // --- COLUMN 1: Address ---
                ImGui::TableSetColumnIndex(1);
                char addrStr[8];
                sprintf_s(addrStr, "$%04X", i);

                // Selectable makes the whole row clickable and easy to target for right-click
                if (ImGui::Selectable(addrStr, (dbgCtx->lastState.pc == i), ImGuiSelectableFlags_SpanAllColumns)) {
                    // Jump logic or selecting logic here
                }

                // --- RIGHT CLICK CONTEXT MENU ---
                // This targets the Selectable we just created
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Add Breakpoint", nullptr, hasBreakpoint)) {
                        if (hasBreakpoint) dbgCtx->breakpoints[i].store(false);
                        else dbgCtx->breakpoints[i].store(true);
                    }
                    //if (ImGui::MenuItem("Run to here")) {
                    //    runToAddress = i;
                    //    isPaused = false;
                    //}
                    if (ImGui::MenuItem("Copy Address")) {
                        ImGui::SetClipboardText(addrStr);
                    }
                    ImGui::EndPopup();
                }

                // --- COLUMN 2: Instruction ---
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(Disassemble(i).c_str());
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

//LRESULT CALLBACK DebuggerUI::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    LRESULT result = 0;
//
//    if (message == WM_CREATE)
//    {
//        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
//        DebuggerUI* pMain = (DebuggerUI*)pcs->lpCreateParams;
//
//        INITCOMMONCONTROLSEX icex;
//        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
//        icex.dwICC = ICC_BAR_CLASSES;
//        InitCommonControlsEx(&icex);
//        HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
//            WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_WRAPABLE,
//            0, 0, 0, 0, hwnd, (HMENU)IDR_TOOLBAR1, pMain->hInst, NULL);
//
//        // Essential: Tell the toolbar how big the TBBUTTON structure is
//        SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
//
//        // iBitmap: The index of the icon in your bitmap strip (0, 1, 2...)
//        // idCommand: The ID we defined in resource.h
//        // fsState: TBSTATE_ENABLED makes it clickable
//        // fsStyle: BTNS_BUTTON for standard, BTNS_SEP for a space between buttons
//
//        TBBUTTON tbButtons[] = {
//            { 0, IDB_PLAY,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Play" },
//            { 1, IDB_PAUSE,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Pause" },
//            //{ 2, ID_STOP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Stop" },
//            { 0, 0,          TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0 }, // Separator gap
//            { 3, IDB_FILLED_CIRCLE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Breakpoint" }
//        };
//        int numButtons = sizeof(tbButtons) / sizeof(TBBUTTON);
//        SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)numButtons, (LPARAM)&tbButtons);
//        SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
//
//        
//        
//        pMain->hList = CreateWindowEx(
//            WS_EX_CLIENTEDGE,
//            WC_LISTVIEW,
//            nullptr,
//            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SINGLESEL,
//            10, 120, 300, 520,
//            hwnd, (HMENU)1001, pMain->hInst, nullptr
//        );
//
//        // LVS_EX_FULLROWSELECT ensures the highlight spans all columns
//        // LVS_EX_GRIDLINES is also nice for debuggers if you want visible borders
//        ListView_SetExtendedListViewStyle(pMain->hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
//
//        LVCOLUMN col{};
//        col.mask = LVCF_TEXT | LVCF_WIDTH;
//
//        wchar_t bpText[] = L"BP";
//        col.cx = 40; col.pszText = bpText;
//        ListView_InsertColumn(pMain->hList, 0, &col);
//
//        wchar_t lineText[] = L"Line";
//        col.cx = 70; col.pszText = lineText;
//        ListView_InsertColumn(pMain->hList, 1, &col);
//
//        wchar_t addrText[] = L"Addr";
//        col.cx = 80; col.pszText = addrText;
//        ListView_InsertColumn(pMain->hList, 2, &col);
//
//        wchar_t instText[] = L"Instruction";
//        col.cx = 100; col.pszText = instText;
//        ListView_InsertColumn(pMain->hList, 3, &col);
//
//        RECT rect;
//		GetClientRect(hWndToolbar, &rect);
//
//        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
//        HWND hLabel = CreateWindowEx(
//            0,                      // Optional styles
//            L"STATIC",              // Predefined class
//            L"A:",         // Text to display
//            WS_CHILD | WS_VISIBLE | SS_LEFT, // Styles
//            320, rect.bottom + 20, 80, 20,         // x, y, width, height
//            hwnd,                   // Parent window handle
//            NULL,                   // No menu
//            pMain->hInst,              // Instance handle
//            NULL                    // Additional data
//        );
//        //SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
//        SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
//
//        ::SetWindowLongPtrW(
//            hwnd,
//            GWLP_USERDATA,
//            reinterpret_cast<LONG_PTR>(pMain)
//        );
//        result = 1;
//    }
//    else
//    {
//        DebuggerUI* pMain = reinterpret_cast<DebuggerUI*>(static_cast<LONG_PTR>(
//            ::GetWindowLongPtrW(
//                hwnd,
//                GWLP_USERDATA
//            )));
//
//        bool wasHandled = false;
//
//        if (pMain)
//        {
//            switch (message) {
//            case WM_USER_BREAKPOINT_HIT: {
//                // 1. Force the window to the foreground so the user sees the hit
//                SetForegroundWindow(hwnd);
//
//                // 2. Update the UI state
//                pMain->ComputeDisplayMap();
//                pMain->FocusPC(pMain->dbgCtx->lastState.pc);
//
//                // 3. Refresh the Listview to show the yellow highlight
//                InvalidateRect(pMain->hList, NULL, FALSE);
//
//                //LOG(L"Breakpoint Hit at %04X\n", pMain->dbgCtx->lastState.pc);
//                return 0;
//            }
//            case WM_NOTIFY: {
//				// Only handle if paused so we don't slow down emulation
//                if (!pMain->_core.isPlaying || !pMain->dbgCtx->is_paused) {
//                    return 1;
//                }
//                LPNMHDR lpnmh = (LPNMHDR)lParam;
//                if (lpnmh->code == NM_RCLICK) {
//                    LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
//
//                    // 1. Load the menu resource
//                    HMENU hMenuLoad = LoadMenu(pMain->hInst, MAKEINTRESOURCE(IDR_DBG_CONTEXT_MENU));
//                    HMENU hPopupMenu = GetSubMenu(hMenuLoad, 0); // Get the "Dummy" popup
//
//                    // 2. Get mouse position
//                    POINT pt;
//                    GetCursorPos(&pt); // Gets absolute screen coordinates
//
//                    // 3. Show the menu
//                    // TPM_RETURNCMD makes the function return the ID of the clicked item
//                    // TPM_RIGHTBUTTON allows right-click selection
//                    TrackPopupMenu(hPopupMenu, TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN,
//                        pt.x, pt.y, 0, hwnd, NULL);
//
//                    int iItem = lpnmia->iItem; // Index of the row clicked
//                    if (iItem != -1) {
//                        uint16_t addr = pMain->displayList[iItem];
//                        // You can store this 'addr' in a temporary variable to use
//                        // when the WM_COMMAND for the menu item eventually fires.
//                        pMain->contextMenuAddr = addr;
//                    }
//
//                    DestroyMenu(hMenuLoad);
//                    return 0;
//                }
//                else if (lpnmh->code == NM_CUSTOMDRAW) {
//
//                    // 3. Cast to the specific CustomDraw structure
//                    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
//
//                    return pMain->HandleCustomDraw(lplvcd, pMain->displayList);
//                }
//                else if (lpnmh->idFrom == 1001) // Our ListView control
//                {
//                    if (lpnmh->code == LVN_GETDISPINFO)
//                    {
//                        NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)lParam;
//                        int index = pDispInfo->item.iItem;
//                        int subItem = pDispInfo->item.iSubItem;
//                        if (pDispInfo->item.mask & LVIF_TEXT) {
//
//                            // Fill in the item text based on index and subItem
//                            switch (subItem)
//                            {
//                            case 0: { // BP column
//                                uint16_t addr = pMain->displayList[index];
//                                if (pMain->dbgCtx->HasBreakpoint(addr))
//                                    wcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"*");
//                                else
//                                    wcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L" ");
//                            } break;
//                            case 1: { // Line column
//                                //swprintf_s(buffer, L"%d", index + 1); // Line number (1-based)
//                                swprintf_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L" "); // Address in hex
//                                //buffer[0] = L'\0';
//                            } break;
//                            case 2: { // Addr column
//                                uint16_t displayAddr = pMain->displayList[index];
//                                swprintf_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%04X", displayAddr); // Address in hex
//                                //swprintf_s(buffer, L""); // Address in hex
//                            } break;
//                            case 3: { // Instruction column
//                                // Placeholder instruction text
//                                //if (pMain->dbgCtx->memory_metadata[index] == META_CODE) {
//                                uint16_t displayAddr = pMain->displayList[index];
//                                std::string inst = pMain->Disassemble(displayAddr);
//                                swprintf_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%S", inst.c_str()); // Address in hex
//                                //}
//                                //else {
//                                    //swprintf_s(buffer, L""); // Address in hex
//                                    //buffer[0] = L'\0';
//                                //}
//                            } break;
//                            default: {
//                                //buffer[0] = L'\0';
//                                swprintf_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L" ");
//                            } break;
//                            }
//                        }
//                    }
//				}
//            } break;
//            case WM_COMMAND: {
//                switch (LOWORD(wParam))
//                {
//                case ID_POPUP_TOGGLEBREAKPOINT: {
//                    uint16_t addr = pMain->contextMenuAddr;
//                    pMain->dbgCtx->ToggleBreakpoint(addr);
//					pMain->RedrawItem(pMain->displayMap[addr]);
//                } break;
//                    case IDB_PLAY:
//                        pMain->dbgCtx->is_paused.store(false);
//                        SetFocus(pMain->hDebuggerWnd);
//                    break;
//                    case IDB_PAUSE:
//                        pMain->dbgCtx->is_paused.store(true);
//                        pMain->ComputeDisplayMap();
//                        pMain->FocusPC(pMain->dbgCtx->lastState.pc);
//                        pMain->RedrawVisibleRange();
//                        SetFocus(pMain->hDebuggerWnd);
//                        // TODO Improve by using a conditional variable or event
//                        Sleep(5);
//                        break;
//                }
//            } break;
//            case WM_DISPLAYCHANGE:
//            {
//                InvalidateRect(hwnd, NULL, FALSE);
//            }
//            result = 0;
//            wasHandled = true;
//            break;
//
//            //case WM_PAINT:
//            //{
//            //    PAINTSTRUCT ps;
//            //    HDC hdc = BeginPaint(hwnd, &ps);
//            //    //pMain->RenderFrame();
//            //    EndPaint(hwnd, &ps);
//            //    ValidateRect(hwnd, NULL);
//            //}
//            //result = 0;
//            //wasHandled = true;
//            //break;
//
//            case WM_DESTROY:
//            {
//                //PostQuitMessage(0);
//				ShowWindow(hwnd, SW_HIDE);
//            }
//            result = 1;
//            wasHandled = true;
//            break;
//            }
//        }
//
//        if (!wasHandled)
//        {
//            result = DefWindowProc(hwnd, message, wParam, lParam);
//        }
//    }
//
//    return result;
//}