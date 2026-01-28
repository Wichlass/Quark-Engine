#pragma once

// Engine init mode
enum class EngineMode
{
	WITH_EDITOR,
	NO_EDITOR,
	NONE
};

// Engine running platform
enum class RunningPlatform
{
	WINDOWS,
	LINUX,
	ANDROID,
	NONE
};

// Engine Rendering backend
enum class RenderingBackend
{
	RS_D3D11,
	RS_D3D12,
	RS_VULKAN,
	RS_OPENGL,
	RS_METAL,
	NONE
};