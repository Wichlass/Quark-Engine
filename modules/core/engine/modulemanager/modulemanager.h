#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include "../../../graphics/rendersystem/rendersystemapi.h"

class ModuleManager
{
private:
#ifdef _WIN32
	using QMODULE = HMODULE;
#else
	using QMODULE = void*;
#endif // _WIN32

	QMODULE m_renderSystemModule;
	typedef RenderSystemAPI* (*t_fnCreateRenderSystem)();
	typedef void (*t_fnDestroyRenderSystem)(RenderSystemAPI*);

	QMODULE m_rsD3D11BackendModule;
	typedef RHI* (*t_fnCreateRenderBackend_D3D11)();
	typedef void  (*t_fnDestroyRenderBackend_D3D11)(RHI*);

public:
	ModuleManager();
	~ModuleManager();

	bool loadModules();
	void unloadModules();

	t_fnCreateRenderSystem fnCreateRenderSystem;
	t_fnDestroyRenderSystem fnDestroyRenderSystem;

	t_fnCreateRenderBackend_D3D11 fnCreateRenderBackend_D3D11;
	t_fnDestroyRenderBackend_D3D11 fnDestroyRenderBackend_D3D11;
};