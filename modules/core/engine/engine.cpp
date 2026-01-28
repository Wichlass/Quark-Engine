#include "engine.h"

#ifdef _WIN32
#include <Windows.h>
#endif

Engine::Engine()
    : m_isRunning(false)
    , m_engineMode(EngineMode::NONE)
    , m_runningPlatform(RunningPlatform::NONE)
    , m_renderingBackend(RenderingBackend::NONE)
    , m_pWindow(nullptr)
    , m_pModuleManager(new ModuleManager())
    , m_pRenderSystem(nullptr)
    , m_pRHI(nullptr)
#ifdef _WIN32
    , m_isWindowsDefaultCPPConsoleActive(true)
#endif
{
}

Engine::~Engine()
{
    delete m_pModuleManager;
    m_pModuleManager = nullptr;
}

void Engine::engineInit(
    EngineMode engineMode,
    RunningPlatform runningPlatform,
    RenderingBackend renderingBackend
)
{
#ifdef _WIN32
    if (m_isWindowsDefaultCPPConsoleActive)
        std::cout << "Engine initializing...\n";
#endif

    m_engineMode = engineMode;
    m_runningPlatform = runningPlatform;
    m_renderingBackend = renderingBackend;

    if (!m_pModuleManager->loadModules())
    {
        std::cerr << "Module loading failed!\n";
        return;
    }

    m_pRenderSystem = m_pModuleManager->fnCreateRenderSystem();
    if (!m_pRenderSystem)
    {
        std::cerr << "RenderSystem creation failed!\n";
        engineShutdown();
        return;
    }

    if (m_renderingBackend == RenderingBackend::RS_D3D11)
    {
        m_pRHI = m_pModuleManager->fnCreateRenderBackend_D3D11();
        if (!m_pRHI)
        {
            std::cerr << "D3D11 backend creation failed!\n";
            engineShutdown();
            return;
        }
    }

    if (m_engineMode == EngineMode::WITH_EDITOR)
    {
        m_pWindow = new Window();

        if (!m_pWindow->windowInit(1280, 720, "Quark Engine - Editor", false, false))
        {
            std::cerr << "Window init failed!\n";
            engineShutdown();
            return;
        }

#ifdef _WIN32
        m_pWindow->setWindowEventCallback(
            [this](UINT msg, WPARAM wParam, LPARAM lParam)
            {
                onWindowEvent(msg, wParam, lParam);
            }
        );

        m_pWindow->setWindowCloseEventCallback(
            [this]()
            {
                onWindowClose();
            }
        );
#endif

        //m_pRenderSystem->renderSystemInit(
        //    m_pRHI,
        //    m_pWindow->getWindowHandler(),
        //    m_pWindow->getWindowInstance()
        //);

        m_isRunning = true;
        engineRun();
    }
}

void Engine::engineRun()
{
    while (m_isRunning)
    {
        if (m_pWindow)
            m_pWindow->pollMessages();

        if (m_pRenderSystem)
            //m_pRenderSystem->renderFrame();

#ifdef _WIN32
        Sleep(16);
#endif
    }

    engineShutdown();
}

void Engine::engineShutdown()
{
    if (!m_pRenderSystem && !m_pRHI && !m_pWindow)
        return;

#ifdef _WIN32
    if (m_isWindowsDefaultCPPConsoleActive)
        std::cout << "Engine shutting down...\n";
#endif

    if (m_pRenderSystem)
    {
        m_pModuleManager->fnDestroyRenderSystem(m_pRenderSystem);
        m_pRenderSystem = nullptr;
    }

    if (m_pRHI)
    {
        m_pModuleManager->fnDestroyRenderBackend_D3D11(m_pRHI);
        m_pRHI = nullptr;
    }

    if (m_pWindow)
    {
        m_pWindow->windowShutdown();
        delete m_pWindow;
        m_pWindow = nullptr;
    }

    m_pModuleManager->unloadModules();
}

#ifdef _WIN32
void Engine::onWindowEvent(UINT, WPARAM, LPARAM)
{
}
#endif

void Engine::onWindowClose()
{
#ifdef _WIN32
    std::cout << "Window close requested\n";
#endif
    m_isRunning = false;
}

EngineAPI* createEngine()
{
    return new Engine();
}

void destroyEngine(EngineAPI* engine)
{
    delete engine;
}
