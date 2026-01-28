#pragma once

#ifdef _WIN32
#ifdef ENGINE_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif
#endif

#include <iostream>
#include "engineapi.h"
#include "modulemanager/modulemanager.h"
#include "window/window.h"
#include "enginetypes.h"
#include "../../graphics/rendersystem/rendersystemapi.h"

class Engine : public EngineAPI
{
public:
    Engine();
    ~Engine() override;

    void engineInit(
        EngineMode engineMode,
        RunningPlatform runningPlatform,
        RenderingBackend renderingBackend
    ) override;

    void engineShutdown() override;

private:
    void engineRun();

#ifdef _WIN32
    void onWindowEvent(UINT msg, WPARAM wParam, LPARAM lParam);
#endif
    void onWindowClose();

private:
#ifdef _WIN32
    bool m_isWindowsDefaultCPPConsoleActive;
#endif

    bool m_isRunning;
    EngineMode m_engineMode;
    RunningPlatform m_runningPlatform;
    RenderingBackend m_renderingBackend;

    Window* m_pWindow;
    ModuleManager* m_pModuleManager;

    RenderSystemAPI* m_pRenderSystem;
    RHI* m_pRHI;
};

// C API
extern "C"
{
    ENGINE_API EngineAPI* createEngine();
    ENGINE_API void destroyEngine(EngineAPI* engine);
}