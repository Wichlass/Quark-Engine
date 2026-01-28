#include <iostream>

#define NOMINMAX

#ifdef _WIN32
#include <Windows.h>
#endif

#include "core/engine/engineapi.h"

int main()
{
#ifdef _WIN32
    HMODULE engineModule = LoadLibraryA("modules/engine.dll");

    if (!engineModule) return 1;

    typedef EngineAPI* (*t_fnCreateEngine)();
    typedef void (*t_fnDestroyEngine)(EngineAPI*);

    t_fnCreateEngine fnCreateEngine =
        reinterpret_cast<t_fnCreateEngine>(
            GetProcAddress(engineModule, "createEngine"));

    t_fnDestroyEngine fnDestroyEngine =
        reinterpret_cast<t_fnDestroyEngine>(
            GetProcAddress(engineModule, "destroyEngine"));

    EngineAPI* engine = fnCreateEngine();
    engine->engineInit(
        EngineMode::WITH_EDITOR,
        RunningPlatform::WINDOWS,
        RenderingBackend::RS_D3D11
    );

    fnDestroyEngine(engine);
    engine = nullptr;

    FreeLibrary(engineModule);
#endif

    return 0;
}