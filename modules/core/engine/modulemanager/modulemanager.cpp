#include <iostream>
#include "modulemanager.h"

ModuleManager::ModuleManager()
	: m_renderSystemModule(nullptr)
	, fnCreateRenderSystem(nullptr)
	, fnDestroyRenderSystem(nullptr)

    , m_rsD3D11BackendModule(nullptr)
    , fnCreateRenderBackend_D3D11(nullptr)
    , fnDestroyRenderBackend_D3D11(nullptr)
{
}

bool ModuleManager::loadModules()
{
#ifdef _WIN32
    // RenderSystem
    m_renderSystemModule = LoadLibraryA("modules/rendersystem.dll");
    if (!m_renderSystemModule)
        return false;

    fnCreateRenderSystem =
        reinterpret_cast<t_fnCreateRenderSystem>(
            GetProcAddress(m_renderSystemModule, "createRenderSystem"));

    fnDestroyRenderSystem =
        reinterpret_cast<t_fnDestroyRenderSystem>(
            GetProcAddress(m_renderSystemModule, "destroyRenderSystem"));

    if (!fnCreateRenderSystem || !fnDestroyRenderSystem)
    {
        std::cout << "rendersystem.dll function load failed!\n";
        FreeLibrary(m_renderSystemModule);
        m_renderSystemModule = nullptr;
        return false;
    }

    // D3D11 backend
    m_rsD3D11BackendModule = LoadLibraryA("modules/rsd3d11.dll");
    if (!m_rsD3D11BackendModule)
        return false;

    fnCreateRenderBackend_D3D11 =
        reinterpret_cast<t_fnCreateRenderBackend_D3D11>(
            GetProcAddress(m_rsD3D11BackendModule, "createRenderBackend"));

    fnDestroyRenderBackend_D3D11 =
        reinterpret_cast<t_fnDestroyRenderBackend_D3D11>(
            GetProcAddress(m_rsD3D11BackendModule, "destroyRenderBackend"));

    if (!fnCreateRenderBackend_D3D11 || !fnDestroyRenderBackend_D3D11)
    {
        std::cout << "rsd3d11.dll function load failed!\n";
        FreeLibrary(m_rsD3D11BackendModule);
        m_rsD3D11BackendModule = nullptr;
        return false;
    }

    std::cout << "rendersystem.dll loaded.\n";
    std::cout << "rsd3d11.dll loaded.\n";
    return true;
#endif
}

void ModuleManager::unloadModules()
{
#ifdef _WIN32
    // RenderSystem
    if (m_renderSystemModule)
    {
        FreeLibrary(m_renderSystemModule);
        m_renderSystemModule = nullptr;
    }

    fnCreateRenderSystem = nullptr;
    fnDestroyRenderSystem = nullptr;

    // D3D11 backend
    if (m_rsD3D11BackendModule)
    {
        FreeLibrary(m_rsD3D11BackendModule);
        m_rsD3D11BackendModule = nullptr;
    }

    fnCreateRenderBackend_D3D11 = nullptr;
    fnDestroyRenderBackend_D3D11 = nullptr;
#endif
}

ModuleManager::~ModuleManager()
{
    unloadModules();
}