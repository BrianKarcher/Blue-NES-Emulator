#include <SDL.h>
#include "main.h"
#include "Core.h"

int main(int argc, char* argv[]) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        // Handle error
    }
    Core core;
 
    if (core.Initialize() == S_OK && core.CreateWindows() == S_OK)
    {
        core.RunMessageLoop();
    }

    return 0;
}