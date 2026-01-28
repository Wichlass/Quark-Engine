#pragma once
#include "../../headeronly/globaltypes.h"

// Native OS render target (for cross-platform support)
struct NativeRenderTarget
{
    void* surface = nullptr;  // HWND, wl_surface, CAMetalLayer, etc.
    void* context = nullptr;  // HINSTANCE, Display*, wl_display, etc.
};

// Resource handles
using hMesh = UINT32;
using hMaterial = UINT32;
using hTexture = UINT32;
using hLight = UINT32;