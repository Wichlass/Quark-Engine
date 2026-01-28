#pragma once

#include "enginetypes.h"
#include "../../graphics/rendersystem/rstypes.h"

class EngineAPI
{
public:
	virtual ~EngineAPI() = default;
	virtual void engineInit(EngineMode engineMode, RunningPlatform runningPlatform, RenderingBackend renderingBackend) = 0;
	virtual void engineShutdown() = 0;
};