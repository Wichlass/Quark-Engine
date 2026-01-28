#include <iostream>
#include <chrono>
#include <algorithm>
#include <functional>
#include <vector>
#include <string>
#include "../rendersystem/rendersystemapi.h"
#include "../rendersystem/renderobject.h"
#include "../rendersystem/meshdata.h"
#include "../rendersystem/material.h"
#include "../rendersystem/lighting.h"
#include "../rendersystem/renderstats.h"
#include "../rendersystem/camera.h"
#include "../rendersystem/rhi.h"
#include "../rendersystem/sky.h"
#include "../../headeronly/globaltypes.h"
#include "../../tools/modelloader.h"

// ImGui includes
#include "../../../thirdparty/imgui/imgui.h"
#include "../../../thirdparty/imgui/backends/imgui_impl_win32.h"
#include "../../../thirdparty/imgui/backends/imgui_impl_dx11.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include "../../../thirdparty/imguizmo/ImGuizmo.h"

// DLL function types for RenderSystem factory
typedef RenderSystemAPI* (*CreateRenderSystemFn)();
typedef void (*DestroyRenderSystemFn)(RenderSystemAPI*);

// DLL function types for RHI factory
typedef RHI* (*CreateRenderBackendFn)();
typedef void (*DestroyRenderBackendFn)(RHI*);

// Forward declare ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ==================== WIN32 WINDOW WRAPPER ====================

class SimpleWindow
{
private:
    HWND m_hwnd;
    HINSTANCE m_hInstance;
    bool m_isRunning;
    UINT32 m_width;
    UINT32 m_height;
    std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> m_messageCallback;

    static SimpleWindow* s_instance;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (s_instance)
            return s_instance->handleMessage(hwnd, msg, wParam, lParam);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;

        if (m_messageCallback)
        {
            LRESULT result = m_messageCallback(hwnd, msg, wParam, lParam);
            if (result != 0)
                return result;
        }

        switch (msg)
        {
        case WM_CLOSE:
            m_isRunning = false;
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

public:
    SimpleWindow()
        : m_hwnd(nullptr)
        , m_hInstance(GetModuleHandle(nullptr))
        , m_isRunning(false)
        , m_width(0)
        , m_height(0)
    {
        s_instance = this;
    }

    ~SimpleWindow()
    {
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
        s_instance = nullptr;
    }

    bool create(UINT32 width, UINT32 height, const char* title)
    {
        m_width = width;
        m_height = height;

        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = m_hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = "QRETAW";

        if (!RegisterClassEx(&wc))
        {
            DWORD error = GetLastError();
            if (error != 1410)
            {
                std::cerr << "Failed to register window class! Error: " << error << "\n";
                return false;
            }
        }

        RECT rect = { 0, 0, (LONG)width, (LONG)height };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        m_hwnd = CreateWindowEx(
            0, "QRETAW", title, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, m_hInstance, nullptr
        );

        if (!m_hwnd)
        {
            std::cerr << "Failed to create window!\n";
            return false;
        }

        ShowWindow(m_hwnd, SW_MAXIMIZE);
        UpdateWindow(m_hwnd);
        m_isRunning = true;
        return true;
    }

    void pollMessages()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    HWND getHandle() const { return m_hwnd; }
    HINSTANCE getInstance() const { return m_hInstance; }
    bool isRunning() const { return m_isRunning; }
    void close() { m_isRunning = false; }
    UINT32 getWidth() const { return m_width; }
    UINT32 getHeight() const { return m_height; }

    void setMessageCallback(std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> callback)
    {
        m_messageCallback = callback;
    }
};

SimpleWindow* SimpleWindow::s_instance = nullptr;

// ==================== MESH CREATION HELPERS ====================

MeshData CreateCubeMesh()
{
    MeshData meshData = {};

    static Vertex cubeVertices[] = {
        // Front face
        { Quark::Vec3(-1.0f, -1.0f,  1.0f), Quark::Vec3(0, 0, 1), Quark::Vec2(0, 1), Quark::Vec3(1,0,0), Quark::Vec3(0,1,0) },
        { Quark::Vec3( 1.0f, -1.0f,  1.0f), Quark::Vec3(0, 0, 1), Quark::Vec2(1, 1), Quark::Vec3(1,0,0), Quark::Vec3(0,1,0) },
        { Quark::Vec3( 1.0f,  1.0f,  1.0f), Quark::Vec3(0, 0, 1), Quark::Vec2(1, 0), Quark::Vec3(1,0,0), Quark::Vec3(0,1,0) },
        { Quark::Vec3(-1.0f,  1.0f,  1.0f), Quark::Vec3(0, 0, 1), Quark::Vec2(0, 0), Quark::Vec3(1,0,0), Quark::Vec3(0,1,0) },
        // Back face
        { Quark::Vec3( 1.0f, -1.0f, -1.0f), Quark::Vec3(0, 0, -1), Quark::Vec2(0, 1), Quark::Vec3(-1,0,0), Quark::Vec3(0,1,0) },
        { Quark::Vec3(-1.0f, -1.0f, -1.0f), Quark::Vec3(0, 0, -1), Quark::Vec2(1, 1), Quark::Vec3(-1,0,0), Quark::Vec3(0,1,0) },
        { Quark::Vec3(-1.0f,  1.0f, -1.0f), Quark::Vec3(0, 0, -1), Quark::Vec2(1, 0), Quark::Vec3(-1,0,0), Quark::Vec3(0,1,0) },
        { Quark::Vec3( 1.0f,  1.0f, -1.0f), Quark::Vec3(0, 0, -1), Quark::Vec2(0, 0), Quark::Vec3(-1,0,0), Quark::Vec3(0,1,0) },
        // Top face
        { Quark::Vec3(-1.0f,  1.0f,  1.0f), Quark::Vec3(0, 1, 0), Quark::Vec2(0, 1), Quark::Vec3(1,0,0), Quark::Vec3(0,0,-1) },
        { Quark::Vec3( 1.0f,  1.0f,  1.0f), Quark::Vec3(0, 1, 0), Quark::Vec2(1, 1), Quark::Vec3(1,0,0), Quark::Vec3(0,0,-1) },
        { Quark::Vec3( 1.0f,  1.0f, -1.0f), Quark::Vec3(0, 1, 0), Quark::Vec2(1, 0), Quark::Vec3(1,0,0), Quark::Vec3(0,0,-1) },
        { Quark::Vec3(-1.0f,  1.0f, -1.0f), Quark::Vec3(0, 1, 0), Quark::Vec2(0, 0), Quark::Vec3(1,0,0), Quark::Vec3(0,0,-1) },
        // Bottom face
        { Quark::Vec3(-1.0f, -1.0f, -1.0f), Quark::Vec3(0, -1, 0), Quark::Vec2(0, 1), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1) },
        { Quark::Vec3( 1.0f, -1.0f, -1.0f), Quark::Vec3(0, -1, 0), Quark::Vec2(1, 1), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1) },
        { Quark::Vec3( 1.0f, -1.0f,  1.0f), Quark::Vec3(0, -1, 0), Quark::Vec2(1, 0), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1) },
        { Quark::Vec3(-1.0f, -1.0f,  1.0f), Quark::Vec3(0, -1, 0), Quark::Vec2(0, 0), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1) },
        // Right face
        { Quark::Vec3( 1.0f, -1.0f,  1.0f), Quark::Vec3(1, 0, 0), Quark::Vec2(0, 1), Quark::Vec3(0,0,-1), Quark::Vec3(0,1,0) },
        { Quark::Vec3( 1.0f, -1.0f, -1.0f), Quark::Vec3(1, 0, 0), Quark::Vec2(1, 1), Quark::Vec3(0,0,-1), Quark::Vec3(0,1,0) },
        { Quark::Vec3( 1.0f,  1.0f, -1.0f), Quark::Vec3(1, 0, 0), Quark::Vec2(1, 0), Quark::Vec3(0,0,-1), Quark::Vec3(0,1,0) },
        { Quark::Vec3( 1.0f,  1.0f,  1.0f), Quark::Vec3(1, 0, 0), Quark::Vec2(0, 0), Quark::Vec3(0,0,-1), Quark::Vec3(0,1,0) },
        // Left face
        { Quark::Vec3(-1.0f, -1.0f, -1.0f), Quark::Vec3(-1, 0, 0), Quark::Vec2(0, 1), Quark::Vec3(0,0,1), Quark::Vec3(0,1,0) },
        { Quark::Vec3(-1.0f, -1.0f,  1.0f), Quark::Vec3(-1, 0, 0), Quark::Vec2(1, 1), Quark::Vec3(0,0,1), Quark::Vec3(0,1,0) },
        { Quark::Vec3(-1.0f,  1.0f,  1.0f), Quark::Vec3(-1, 0, 0), Quark::Vec2(1, 0), Quark::Vec3(0,0,1), Quark::Vec3(0,1,0) },
        { Quark::Vec3(-1.0f,  1.0f, -1.0f), Quark::Vec3(-1, 0, 0), Quark::Vec2(0, 0), Quark::Vec3(0,0,1), Quark::Vec3(0,1,0) }
    };

    static UINT32 cubeIndices[] = {
        0,1,2, 0,2,3,       // Front
        4,5,6, 4,6,7,       // Back
        8,9,10, 8,10,11,    // Top
        12,13,14, 12,14,15, // Bottom
        16,17,18, 16,18,19, // Right
        20,21,22, 20,22,23  // Left
    };

    meshData.vertices = cubeVertices;
    meshData.vertexCount = 24;
    meshData.indices = cubeIndices;
    meshData.indexCount = 36;
    meshData.boundingBox.minBounds = Quark::Vec3(-1.0f, -1.0f, -1.0f);
    meshData.boundingBox.maxBounds = Quark::Vec3(1.0f, 1.0f, 1.0f);


    return meshData;
}

MeshData CreatePlaneMesh()
{
    MeshData meshData = {};

    static Vertex planeVertices[] = {
        { Quark::Vec3(-10.0f, 0.0f, -10.0f), Quark::Vec3(0, 1, 0), Quark::Vec2(0, 10), Quark::Vec3(1, 0, 0), Quark::Vec3(0, 0, 1) },
        { Quark::Vec3( 10.0f, 0.0f, -10.0f), Quark::Vec3(0, 1, 0), Quark::Vec2(10, 10), Quark::Vec3(1, 0, 0), Quark::Vec3(0, 0, 1) },
        { Quark::Vec3( 10.0f, 0.0f,  10.0f), Quark::Vec3(0, 1, 0), Quark::Vec2(10, 0), Quark::Vec3(1, 0, 0), Quark::Vec3(0, 0, 1) },
        { Quark::Vec3(-10.0f, 0.0f,  10.0f), Quark::Vec3(0, 1, 0), Quark::Vec2(0, 0), Quark::Vec3(1, 0, 0), Quark::Vec3(0, 0, 1) }
    };

    // Indices wound CCW when viewed from +Y (matching the +Y normal)
    static UINT32 planeIndices[] = { 0, 2, 1, 0, 3, 2 };

    meshData.vertices = planeVertices;
    meshData.vertexCount = 4;
    meshData.indices = planeIndices;
    meshData.indexCount = 6;
    meshData.boundingBox.minBounds = Quark::Vec3(-10.0f, 0.0f, -10.0f);
    meshData.boundingBox.maxBounds = Quark::Vec3(10.0f, 0.0f, 10.0f);


    return meshData;
}

// ==================== RENDER EDITOR APPLICATION ====================

class RenderEditor
{
private:
    SimpleWindow* m_pWindow;
    RenderSystemAPI* m_pRenderSystem;
    RHI* m_pRHI;

    // DLL handles
    HMODULE m_hRenderSystemDLL;
    HMODULE m_hRHIDLL;

    // Factory function pointers
    CreateRenderSystemFn m_pfnCreateRenderSystem;
    DestroyRenderSystemFn m_pfnDestroyRenderSystem;
    CreateRenderBackendFn m_pfnCreateRenderBackend;
    DestroyRenderBackendFn m_pfnDestroyRenderBackend;

    // Editor structs
    struct EditorMesh
    {
        std::string name;
        hMesh handle;
        bool isPlane;
        Quark::AABB boundingBox;
    };

    struct MaterialResource
    {
        std::string name;
        hMaterial handle;
        MaterialData data;
        std::string albedoPath;
        std::string normalPath;
    };

    struct SceneObject
    {
        std::string name;
        int meshIndex;
        int materialIndex;
        Quark::Vec3 position;
        Quark::Vec3 rotation;
        Quark::Vec3 scale;
        
        // Active = object is submitted to render system (false = completely ignored)
        bool active;
        
        // Render flags
        bool visible;           // Rendered in main pass
        bool castsShadows;      // Casts shadows (even if not visible)
        bool receivesShadows;   // Receives shadows from lights
        bool frustumCull;       // Participates in frustum culling
        
        bool animate;
        float animationSpeed;

        SceneObject()
            : position(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1)
            , active(true), visible(true), castsShadows(true), receivesShadows(true), frustumCull(true)
            , animate(false), animationSpeed(1.0f)
            , meshIndex(-1), materialIndex(-1) {}
    };


    Camera m_camera;
    std::vector<EditorMesh> m_meshes;

    std::vector<MaterialResource> m_materials;
    std::vector<SceneObject> m_sceneObjects;

    int m_selectedObject = -1;
    int m_selectedMaterial = -1;
    int m_selectedMesh = -1;

    bool m_keys[256] = {};
    float m_cameraMoveSpeed = 15.0f;
    float m_cameraRotateSpeed = 0.1f;
    bool m_firstMouse = true;
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    bool m_mouseControlEnabled = false;

    float m_time = 0.0f;
    float m_lastFrameTime = 0.0f;
    bool m_isRunning = false;

    bool m_showDemoWindow = false;
    bool m_showStatsWindow = true;
    bool m_showResourcesWindow = true;
    bool m_showSceneWindow = true;
    bool m_showInspectorWindow = true;
    bool m_showEnvironmentWindow = true;
    bool m_showCameraWindow = true;
    bool m_showLightingWindow = true;
    bool m_showSkyWindow = true;

    float m_clearColor[4] = { 0.87843f, 1.0f, 1.0f, 1.0f };
    char m_newObjectName[64] = "New Object";
    char m_newMaterialName[64] = "New Material";
    char m_texturePath[260] = {};

    // ==================== LIGHTING ====================
    struct SceneLight
    {
        std::string name;
        LightType type;
        bool enabled;
        hLight handle;  // GPU handle
        Quark::Vec3 position;
        Quark::Vec3 direction;
        Quark::Color color;
        float intensity;
        float range;
        float innerCutoff;  // degrees
        float outerCutoff;  // degrees
        bool castsShadows;
        int shadowQuality;  // 0=None, 1=Hard, 2=Medium, 3=Soft

        SceneLight(const std::string& n, LightType t)
            : name(n), type(t), enabled(true), handle(0)
            , position(0, 5, 0), direction(0, -1, 0)
            , color(Quark::Color::White()), intensity(1.0f)
            , range(10.0f), innerCutoff(25.0f), outerCutoff(35.0f)
            , castsShadows(false), shadowQuality(2) {}
    };
    std::vector<SceneLight> m_lights;
    int m_selectedLight = -1;

    // Gizmo state
    ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE m_gizmoMode = ImGuizmo::WORLD;
    bool m_pickingRequested = false;
    bool m_gizmoIsUsing = false;
    bool m_gizmoIsOver = false;
    
    // Snap values
    float m_snapTranslation = 0.5f;
    float m_snapRotation = 15.0f;
    float m_snapScale = 0.1f;
    bool m_useSnap = false;

    static const int MAX_UNDO_STEPS = 50;
    struct UndoState
    {
        std::vector<SceneObject> objects;
        int selectedObject = -1;
    };

    std::vector<UndoState> m_undoStack;
    std::vector<UndoState> m_redoStack;
    bool m_wasGizmoUsing = false;  // Track when gizmo manipulation ends

    // Copy/Paste clipboard
    SceneObject m_clipboard;
    bool m_hasClipboard = false;


public:
    RenderEditor() 
        : m_pWindow(nullptr)
        , m_pRenderSystem(nullptr)
        , m_pRHI(nullptr)
        , m_hRenderSystemDLL(nullptr)
        , m_hRHIDLL(nullptr)
        , m_pfnCreateRenderSystem(nullptr)
        , m_pfnDestroyRenderSystem(nullptr)
        , m_pfnCreateRenderBackend(nullptr)
        , m_pfnDestroyRenderBackend(nullptr)
    {}
    ~RenderEditor() { shutdown(); }

    bool openFileDialog(const char* filter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0")
    {
        OPENFILENAMEA ofn;
        char szFile[260] = {};
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_pWindow->getHandle();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            strncpy_s(m_texturePath, ofn.lpstrFile, _TRUNCATE);
            return true;
        }
        return false;
    }

    bool init()
    {
        std::cout << "=== Quark Render Engine Editor ===\n\n";

        m_pWindow = new SimpleWindow();
        if (!m_pWindow->create(1920, 1080, "Quark Render Engine - Editor"))
        {
            std::cerr << "Failed to create window!\n";
            return false;
        }

        m_pWindow->setMessageCallback([this](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            return handleWindowMessage(hwnd, msg, wParam, lParam);
        });

        // ==================== DYNAMIC DLL LOADING ====================
        
        // Load RHI DLL (rsd3d11.dll)
        m_hRHIDLL = LoadLibraryA("modules/rsd3d11.dll");
        if (!m_hRHIDLL)
        {
            std::cerr << "Failed to load rsd3d11.dll! Error: " << GetLastError() << "\n";
            return false;
        }

        // Get RHI factory functions
        m_pfnCreateRenderBackend = (CreateRenderBackendFn)GetProcAddress(m_hRHIDLL, "createRenderBackend");
        m_pfnDestroyRenderBackend = (DestroyRenderBackendFn)GetProcAddress(m_hRHIDLL, "destroyRenderBackend");
        if (!m_pfnCreateRenderBackend || !m_pfnDestroyRenderBackend)
        {
            std::cerr << "Failed to get RHI factory functions!\n";
            return false;
        }

        // Load RenderSystem DLL
        m_hRenderSystemDLL = LoadLibraryA("modules/rendersystem.dll");
        if (!m_hRenderSystemDLL)
        {
            std::cerr << "Failed to load rendersystem.dll! Error: " << GetLastError() << "\n";
            return false;
        }

        // Get RenderSystem factory functions
        m_pfnCreateRenderSystem = (CreateRenderSystemFn)GetProcAddress(m_hRenderSystemDLL, "createRenderSystem");
        m_pfnDestroyRenderSystem = (DestroyRenderSystemFn)GetProcAddress(m_hRenderSystemDLL, "destroyRenderSystem");
        if (!m_pfnCreateRenderSystem || !m_pfnDestroyRenderSystem)
        {
            std::cerr << "Failed to get RenderSystem factory functions!\n";
            return false;
        }

        std::cout << "DLLs loaded successfully!\n";

        // Create RHI and RenderSystem using factory functions
        m_pRHI = m_pfnCreateRenderBackend();
        if (!m_pRHI)
        {
            std::cerr << "Failed to create RHI!\n";
            return false;
        }

        m_pRenderSystem = m_pfnCreateRenderSystem();
        if (!m_pRenderSystem)
        {
            std::cerr << "Failed to create RenderSystem!\n";
            return false;
        }

        // Initialize with new API
        m_pRenderSystem->init(m_pRHI, static_cast<qWndh>(m_pWindow->getHandle()));
        // m_pRenderSystem->setAmbientLight(Quark::Color(0.0f, 0.0f, 0.0f));

        if (!initImGui())
        {
            std::cerr << "Failed to initialize ImGui!\n";
            return false;
        }

        // Setup camera
        m_camera.setPosition(Quark::Vec3(0.0f, 5.0f, -15.0f));
        m_camera.lookAt(Quark::Vec3(0.0f, 0.0f, 0.0f), Quark::Vec3::Up());
        m_camera.setPerspective(Quark::Radians(60.0f), 1920.0f / 1080.0f, 0.1f, 1000.0f);
        m_pRenderSystem->setActiveCamera(&m_camera);

        initScene();

        m_isRunning = true;
        m_lastFrameTime = static_cast<float>(std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count());

        std::cout << "Editor initialized successfully!\n";
        return true;
    }

    bool initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
        
        // Docking options
        io.ConfigDockingWithShift = false;  // Dock without holding shift
        
        ImGui::StyleColorsDark();

        void* device = m_pRHI->getDevice();
        void* context = m_pRHI->getContext();

        if (!ImGui_ImplWin32_Init(m_pWindow->getHandle()))
            return false;
        if (!ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(device), static_cast<ID3D11DeviceContext*>(context)))
            return false;

        std::cout << "ImGui initialized with Docking!\n";
        return true;
    }


    int createCubeMesh(const std::string& name)
    {
        MeshData cubeData = CreateCubeMesh();
        // New API: createMesh(MeshData, bool isDynamic)
        hMesh handle = m_pRenderSystem->createMesh(cubeData, false);
        m_meshes.push_back({ name, handle, false, cubeData.boundingBox });
        return (int)m_meshes.size() - 1;
    }

    int createPlaneMesh(const std::string& name)
    {
        MeshData planeData = CreatePlaneMesh();
        hMesh handle = m_pRenderSystem->createMesh(planeData, false);
        m_meshes.push_back({ name, handle, true, planeData.boundingBox });
        return (int)m_meshes.size() - 1;
    }

    int createMaterial(const std::string& name, const Quark::Color& albedo = Quark::Color::White())
    {
        MaterialData matData;
        matData.albedo = albedo;
        matData.metallic = 0.0f;
        matData.roughness = 0.5f;
        matData.cullMode = static_cast<UINT32>(CullMode::BACK);

        hMaterial handle = m_pRenderSystem->createMaterial(matData);
        m_materials.push_back({ name, handle, matData, "", "" });
        return (int)m_materials.size() - 1;
    }

    void createObject(const std::string& name, int meshIdx, int matIdx,
                     const Quark::Vec3& pos = Quark::Vec3::Zero())
    {
        SceneObject obj;
        obj.name = name;
        obj.meshIndex = meshIdx;
        obj.materialIndex = matIdx;
        obj.position = pos;
        m_sceneObjects.push_back(obj);
    }

    void deleteSelectedObject()
    {
        if (m_selectedObject >= 0 && m_selectedObject < (int)m_sceneObjects.size())
        {
            m_sceneObjects.erase(m_sceneObjects.begin() + m_selectedObject);
            m_selectedObject = -1;
        }
    }

    void deleteSelectedMaterial()
    {
        if (m_selectedMaterial >= 0 && m_selectedMaterial < (int)m_materials.size())
        {
            m_pRenderSystem->destroyMaterial(m_materials[m_selectedMaterial].handle);
            m_materials.erase(m_materials.begin() + m_selectedMaterial);
            m_selectedMaterial = -1;
        }
    }

    // ==================== UNDO/REDO SYSTEM ====================
    void saveUndoState()
    {
        UndoState state;
        state.objects = m_sceneObjects;
        state.selectedObject = m_selectedObject;
        
        // Add to undo stack
        m_undoStack.push_back(state);
        
        // Limit stack size
        if (m_undoStack.size() > MAX_UNDO_STEPS)
        {
            m_undoStack.erase(m_undoStack.begin());
        }
        
        // Clear redo stack when new action is performed
        m_redoStack.clear();
    }

    void undo()
    {
        if (m_undoStack.empty()) return;
        
        // Save current state to redo stack
        UndoState currentState;
        currentState.objects = m_sceneObjects;
        currentState.selectedObject = m_selectedObject;
        m_redoStack.push_back(currentState);
        
        // Restore from undo stack
        UndoState& prevState = m_undoStack.back();
        m_sceneObjects = prevState.objects;
        m_selectedObject = prevState.selectedObject;
        m_undoStack.pop_back();
        
        std::cout << "[Editor] Undo - " << m_undoStack.size() << " steps remaining\n";
    }

    void redo()
    {
        if (m_redoStack.empty()) return;
        
        // Save current state to undo stack
        UndoState currentState;
        currentState.objects = m_sceneObjects;
        currentState.selectedObject = m_selectedObject;
        m_undoStack.push_back(currentState);
        
        // Restore from redo stack
        UndoState& nextState = m_redoStack.back();
        m_sceneObjects = nextState.objects;
        m_selectedObject = nextState.selectedObject;
        m_redoStack.pop_back();
        
        std::cout << "[Editor] Redo - " << m_redoStack.size() << " steps remaining\n";
    }

    // ==================== COPY/PASTE SYSTEM ====================
    void copySelectedObject()
    {
        if (m_selectedObject >= 0 && m_selectedObject < (int)m_sceneObjects.size())
        {
            m_clipboard = m_sceneObjects[m_selectedObject];
            m_hasClipboard = true;
            std::cout << "[Editor] Copied: " << m_clipboard.name << "\n";
        }
    }

    void pasteObject()
    {
        if (!m_hasClipboard) return;
        
        saveUndoState();  // Save before paste
        
        SceneObject newObj = m_clipboard;
        // Generate unique name
        newObj.name = m_clipboard.name + "_copy";
        
        // Calculate paste position from mouse ray intersection with Y=0 plane
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_pWindow->getHandle(), &pt);
        
        RECT clientRect;
        GetClientRect(m_pWindow->getHandle(), &clientRect);
        float clientWidth = (float)(clientRect.right - clientRect.left);
        float clientHeight = (float)(clientRect.bottom - clientRect.top);
        
        Quark::Vec2 mousePos((float)pt.x, (float)pt.y);
        Quark::Vec2 screenSize(clientWidth, clientHeight);
        
        m_camera.update();
        Quark::Ray ray = m_camera.screenPointToRay(mousePos, screenSize);
        
        // Intersect with Y=clipboard.position.y plane (horizontal plane at original object height)
        float planeY = m_clipboard.position.y;
        float t;
        if (ray.IntersectPlane(Quark::Vec3::Up(), Quark::Vec3(0, planeY, 0), t))
        {
            Quark::Vec3 hitPoint = ray.PointAt(t);
            newObj.position = Quark::Vec3(hitPoint.x, planeY, hitPoint.z);
        }
        else
        {
            // Fallback: offset from original position
            newObj.position = m_clipboard.position + Quark::Vec3(1.0f, 0.0f, 1.0f);
        }
        
        m_sceneObjects.push_back(newObj);
        m_selectedObject = (int)m_sceneObjects.size() - 1;
        
        std::cout << "[Editor] Pasted: " << newObj.name << " at (" 
                  << newObj.position.x << ", " << newObj.position.y << ", " << newObj.position.z << ")\n";
    }


    void duplicateSelectedObject()
    {
        if (m_selectedObject >= 0 && m_selectedObject < (int)m_sceneObjects.size())
        {
            copySelectedObject();
            pasteObject();
        }
    }

    void submitLights()
    {
        // Update or create lights using handle-based API
        for (auto& light : m_lights)
        {
            switch (light.type)
            {
                case LightType::DIRECTIONAL:
                {
                    DirectionalLight dirLight;
                    dirLight.direction = light.direction.Normalized();
                    dirLight.intensity = light.enabled ? light.intensity : 0.0f;
                    dirLight.color = light.color;
                    dirLight.flags = static_cast<UINT32>(LightFlags::LIGHT_ENABLED);
                    if (light.castsShadows)
                        dirLight.flags = dirLight.flags | static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS);
                    dirLight.shadowQuality = static_cast<ShadowQuality>(light.shadowQuality);
                    
                    if (light.handle == 0)
                        light.handle = m_pRenderSystem->createDirectionalLight(dirLight);
                    else
                        m_pRenderSystem->updateLight(light.handle, dirLight);
                    break;
                }
                case LightType::POINT:
                {
                    PointLight pointLight;
                    pointLight.position = light.position;
                    pointLight.intensity = light.enabled ? light.intensity : 0.0f;
                    pointLight.color = light.color;
                    pointLight.range = light.range;
                    pointLight.flags = static_cast<UINT32>(LightFlags::LIGHT_ENABLED);
                    if (light.castsShadows)
                        pointLight.flags = pointLight.flags | static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS);
                    pointLight.shadowQuality = static_cast<ShadowQuality>(light.shadowQuality);
                    
                    if (light.handle == 0)
                        light.handle = m_pRenderSystem->createPointLight(pointLight);
                    else
                        m_pRenderSystem->updateLight(light.handle, pointLight);
                    break;
                }
                case LightType::SPOT:
                {
                    SpotLight spotLight;
                    spotLight.position = light.position;
                    spotLight.direction = light.direction.Normalized();
                    spotLight.intensity = light.enabled ? light.intensity : 0.0f;
                    spotLight.color = light.color;
                    spotLight.range = light.range;
                    spotLight.innerCutoff = std::cos(Quark::Radians(light.innerCutoff));
                    spotLight.outerCutoff = std::cos(Quark::Radians(light.outerCutoff));
                    spotLight.flags = static_cast<UINT32>(LightFlags::LIGHT_ENABLED);
                    if (light.castsShadows)
                        spotLight.flags = spotLight.flags | static_cast<UINT32>(LightFlags::LIGHT_CAST_SHADOWS);
                    spotLight.shadowQuality = static_cast<ShadowQuality>(light.shadowQuality);
                    
                    if (light.handle == 0)
                        light.handle = m_pRenderSystem->createSpotLight(spotLight);
                    else
                        m_pRenderSystem->updateLight(light.handle, spotLight);
                    break;
                }
            } // switch
        } // for
    }

    void pickObject()
    {
        // Get mouse position in client coordinates
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_pWindow->getHandle(), &pt);
        
        // Get actual client rect size (important for maximized windows)
        RECT clientRect;
        GetClientRect(m_pWindow->getHandle(), &clientRect);
        float clientWidth = (float)(clientRect.right - clientRect.left);
        float clientHeight = (float)(clientRect.bottom - clientRect.top);
        
        Quark::Vec2 mousePos((float)pt.x, (float)pt.y);
        Quark::Vec2 screenSize(clientWidth, clientHeight);
        
        m_camera.update();
        Quark::Ray ray = m_camera.screenPointToRay(mousePos, screenSize);
        
        int closestIdx = -1;
        float minT = 100000.0f;
        
        for (int i = 0; i < (int)m_sceneObjects.size(); ++i)
        {
            SceneObject& obj = m_sceneObjects[i];
            if (!obj.visible || obj.meshIndex < 0) continue;
            
            bool isPlane = m_meshes[obj.meshIndex].isPlane;
            float t = -1.0f;
            bool hit = false;
            
            if (isPlane)
            {
                // For plane meshes, use AABB intersection instead of sphere
                // The plane mesh is 20x20 units (-10 to 10 on X and Z)
                Quark::Vec3 halfSize = obj.scale * 10.0f;
                Quark::AABB bounds(
                    obj.position - Quark::Vec3(halfSize.x, 0.1f, halfSize.z),
                    obj.position + Quark::Vec3(halfSize.x, 0.1f, halfSize.z)
                );
                float tMin, tMax;
                hit = ray.IntersectAABB(bounds, tMin, tMax);
                if (hit) t = tMin;
            }
            else
            {
                // For other meshes, use bounding box
                const Quark::AABB& localBounds = m_meshes[obj.meshIndex].boundingBox;
                Quark::Vec3 halfExtent = (localBounds.maxBounds - localBounds.minBounds) * 0.5f;
                Quark::Vec3 scaledExtent = Quark::Vec3(
                    halfExtent.x * obj.scale.x,
                    halfExtent.y * obj.scale.y,
                    halfExtent.z * obj.scale.z
                );
                Quark::AABB worldBounds(
                    obj.position - scaledExtent,
                    obj.position + scaledExtent
                );
                float tMin, tMax;
                hit = ray.IntersectAABB(worldBounds, tMin, tMax);
                if (hit) t = tMin;
            }

            
            if (hit && t > 0 && t < minT)
            {
                minT = t;
                closestIdx = i;
            }
        }
        
        if (closestIdx != -1)
        {
            m_selectedObject = closestIdx;
            m_selectedMaterial = -1;
            m_selectedMesh = -1;
        }
        else
        {
            // Clicked on empty space - deselect
            m_selectedObject = -1;
        }
    }



    void loadModelDialog()
    {
        OPENFILENAMEA ofn;
        char szFile[260] = {};
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_pWindow->getHandle();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = ModelLoader::GetSupportedExtensions();
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            LoadedModel model;
            if (ModelLoader::Load(ofn.lpstrFile, model))
            {
                // Create a default material for loaded meshes if none exists
                int defaultMatIdx = 0;
                if (m_materials.empty())
                {
                    defaultMatIdx = createMaterial("Default Material", Quark::Color(0.8f, 0.8f, 0.8f, 1.0f));
                }
                
                // Create meshes and objects from loaded model
                for (size_t i = 0; i < model.meshes.size(); i++)
                {
                    LoadedMesh& lm = model.meshes[i];
                    
                    // Create GPU mesh
                    hMesh meshHandle = m_pRenderSystem->createMesh(lm.data, false);
                    
                    std::string meshName = model.name + "_" + lm.name;
                    if (meshName.empty() || meshName == "_")
                        meshName = "Mesh_" + std::to_string(m_meshes.size());
                    
                    m_meshes.push_back({ meshName, meshHandle, false, lm.data.boundingBox });
                    
                    // Create scene object
                    SceneObject obj;
                    obj.name = meshName;
                    obj.meshIndex = (int)m_meshes.size() - 1;
                    obj.materialIndex = defaultMatIdx;
                    obj.position = Quark::Vec3::Zero();
                    m_sceneObjects.push_back(obj);
                }
                
                std::cout << "[Editor] Loaded " << model.meshes.size() << " meshes from " << model.name << "\n";
            }
        }
    }

    void initScene()
    {
        int cubeMeshIdx = createCubeMesh("Cube");
        int planeMeshIdx = createPlaneMesh("Plane");

        int redMat = createMaterial("Red Material", Quark::Color::Red());
        int greenMat = createMaterial("Green Material", Quark::Color::Green());
        int blueMat = createMaterial("Blue Material", Quark::Color::Blue());

        int planeMat = createMaterial("Ground Material", Quark::Color(1.0f, 1.0f, 1.0f, 1.0f));
        m_materials[planeMat].data.roughness = 1.0f;
        m_materials[planeMat].data.cullMode = static_cast<UINT32>(CullMode::BACK);
        m_pRenderSystem->updateMaterial(m_materials[planeMat].handle, m_materials[planeMat].data);

        createObject("Ground", planeMeshIdx, planeMat, Quark::Vec3::Zero());
        createObject("Red Cube", cubeMeshIdx, redMat, Quark::Vec3(-4.0f, 2.0f, 0.0f));
        createObject("Green Cube", cubeMeshIdx, greenMat, Quark::Vec3(0.0f, 2.0f, 0.0f));
        createObject("Blue Cube", cubeMeshIdx, blueMat, Quark::Vec3(4.0f, 2.0f, 0.0f));

        // Create default lights
        SceneLight sun("Sun", LightType::DIRECTIONAL);
        sun.direction = Quark::Vec3(0.5f, -0.7f, 0.3f).Normalized();
        sun.intensity = 1.5f;
        sun.castsShadows = true;
        sun.color = Quark::Color(1.0f, 0.95f, 0.9f, 1.0f);
        m_lights.push_back(sun);

        SceneLight pointLight("Point Light", LightType::POINT);
        pointLight.position = Quark::Vec3(0, 5, 0);
        pointLight.intensity = 2.0f;
        pointLight.range = 15.0f;
        pointLight.color = Quark::Color(1.0f, 0.8f, 0.6f, 1.0f);
        pointLight.enabled = false;
        m_lights.push_back(pointLight);

        SceneLight spotLight("Spot Light", LightType::SPOT);
        spotLight.position = Quark::Vec3(0, 8, -5);
        spotLight.direction = Quark::Vec3(0, -0.7f, 0.3f).Normalized();
        spotLight.intensity = 3.0f;
        spotLight.range = 20.0f;
        spotLight.innerCutoff = 15.0f;
        spotLight.outerCutoff = 25.0f;
        spotLight.enabled = false; // Disabled by default
        m_lights.push_back(spotLight);

        m_pRenderSystem->setClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
    }

    void renderImGui()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Create DockSpace over the entire viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags dockspaceFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        ImGui::Begin("DockSpace", nullptr, dockspaceFlags);
        ImGui::PopStyleVar(3);
        
        // Create the DockSpace
        ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGuizmo::BeginFrame();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        float width = (float)m_pWindow->getWidth();
        float height = (float)m_pWindow->getHeight();
        ImGuizmo::SetRect(0, 0, width, height);


        // Reset gizmo states at frame start
        m_gizmoIsOver = false;
        m_gizmoIsUsing = false;

        // Gizmo logic
        if (m_selectedObject >= 0 && m_selectedObject < (int)m_sceneObjects.size())
        {
            SceneObject& obj = m_sceneObjects[m_selectedObject];
            
            m_camera.update();
            
            // Get view and projection matrices (Quark uses column-major, same as ImGuizmo)
            const float* viewPtr = m_camera.view.m;
            const float* projPtr = m_camera.projection.m;
            
            // Build transform matrix using ImGuizmo's RecomposeMatrixFromComponents
            // This ensures we use the exact same convention ImGuizmo expects
            float matrixComponents[16];
            float pos[3] = { obj.position.x, obj.position.y, obj.position.z };
            float rot[3] = { obj.rotation.x, obj.rotation.y, obj.rotation.z };
            float scl[3] = { obj.scale.x, obj.scale.y, obj.scale.z };
            ImGuizmo::RecomposeMatrixFromComponents(pos, rot, scl, matrixComponents);

            // Prepare snap values
            float* snapPtr = nullptr;
            float snapValues[3] = {0, 0, 0};
            if (m_useSnap)
            {
                switch (m_gizmoOperation)
                {
                case ImGuizmo::TRANSLATE:
                    snapValues[0] = snapValues[1] = snapValues[2] = m_snapTranslation;
                    break;
                case ImGuizmo::ROTATE:
                    snapValues[0] = snapValues[1] = snapValues[2] = m_snapRotation;
                    break;
                case ImGuizmo::SCALE:
                    snapValues[0] = snapValues[1] = snapValues[2] = m_snapScale;
                    break;
                default:
                    break;
                }
                snapPtr = snapValues;
            }

            // Manipulate with optional snap
            if (ImGuizmo::Manipulate(viewPtr, projPtr, 
                                     m_gizmoOperation, m_gizmoMode, matrixComponents, 
                                     nullptr, snapPtr))
            {
                // Decompose the modified matrix back to components
                float newPos[3], newRot[3], newScl[3];
                ImGuizmo::DecomposeMatrixToComponents(matrixComponents, newPos, newRot, newScl);
                
                obj.position = Quark::Vec3(newPos[0], newPos[1], newPos[2]);
                obj.rotation = Quark::Vec3(newRot[0], newRot[1], newRot[2]);
                obj.scale = Quark::Vec3(newScl[0], newScl[1], newScl[2]);
            }
            
            // Update gizmo states for input handling
            m_gizmoIsOver = ImGuizmo::IsOver();
            bool currentlyUsing = ImGuizmo::IsUsing();
            
            // Save undo state when gizmo manipulation starts
            if (currentlyUsing && !m_wasGizmoUsing)
            {
                saveUndoState();
            }
            m_wasGizmoUsing = currentlyUsing;
            m_gizmoIsUsing = currentlyUsing;

        }



        // Handle picking request (after gizmo states are updated)
        if (m_pickingRequested)
        {
            // Only pick if gizmo is not being interacted with
            if (!m_gizmoIsUsing && !m_gizmoIsOver)
            {
                pickObject();
            }
            m_pickingRequested = false;
        }


        // Menu Bar
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Load Model..."))
                {
                    loadModelDialog();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) m_pWindow->close();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !m_undoStack.empty()))
                    undo();
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !m_redoStack.empty()))
                    redo();
                ImGui::Separator();
                if (ImGui::MenuItem("Copy", "Ctrl+C", false, m_selectedObject >= 0))
                    copySelectedObject();
                if (ImGui::MenuItem("Paste", "Ctrl+V", false, m_hasClipboard))
                    pasteObject();
                if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, m_selectedObject >= 0))
                    duplicateSelectedObject();
                ImGui::Separator();
                if (ImGui::MenuItem("Delete", "Del", false, m_selectedObject >= 0))
                {
                    saveUndoState();
                    deleteSelectedObject();
                }
                ImGui::EndMenu();
            }


            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Resources", nullptr, &m_showResourcesWindow);
                ImGui::MenuItem("Scene Graph", nullptr, &m_showSceneWindow);
                ImGui::MenuItem("Inspector", nullptr, &m_showInspectorWindow);
                ImGui::MenuItem("Lighting", nullptr, &m_showLightingWindow);
                ImGui::MenuItem("Environment", nullptr, &m_showEnvironmentWindow);
                ImGui::MenuItem("Camera", nullptr, &m_showCameraWindow);
                ImGui::Separator();
                ImGui::MenuItem("Statistics", nullptr, &m_showStatsWindow);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                ImGui::Text("Camera Controls:");
                ImGui::BulletText("Hold Right Mouse + WASD: Move camera");
                ImGui::BulletText("Q/E: Move camera up/down");
                ImGui::BulletText("Shift: Faster movement");
                ImGui::Separator();
                ImGui::Text("Gizmo Controls:");
                ImGui::BulletText("W: Translate mode");
                ImGui::BulletText("E: Rotate mode");
                ImGui::BulletText("R: Scale mode");
                ImGui::BulletText("T: Toggle World/Local space");
                ImGui::BulletText("Hold Ctrl: Enable snap");
                ImGui::Separator();
                ImGui::Text("Edit Controls:");
                ImGui::BulletText("Ctrl+Z: Undo");
                ImGui::BulletText("Ctrl+Y: Redo");
                ImGui::BulletText("Ctrl+C: Copy object");
                ImGui::BulletText("Ctrl+V: Paste object");
                ImGui::BulletText("Ctrl+D: Duplicate object");
                ImGui::BulletText("Delete: Delete object");
                ImGui::BulletText("Escape: Deselect all");
                ImGui::EndMenu();
            }


            ImGui::EndMainMenuBar();
        }

        // Statistics Window
        if (m_showStatsWindow)
        {
            ImGui::Begin("Statistics", &m_showStatsWindow);
            const RenderStats& stats = m_pRenderSystem->getStats();
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Objects Rendered: %d", stats.objectsRendered);
            ImGui::Text("Objects Culled: %d", stats.objectsCulled);
            ImGui::Text("Draw Calls: %d", stats.drawCalls);
            ImGui::Text("Triangles: %d", stats.trianglesRendered);
            ImGui::End();
        }

        // Resources Window
        if (m_showResourcesWindow)
        {
            ImGui::Begin("Resources", &m_showResourcesWindow);
            if (ImGui::BeginTabBar("ResourceTabs"))
            {
                if (ImGui::BeginTabItem("Meshes"))
                {
                    if (ImGui::Button("+ Cube")) createCubeMesh("Cube");
                    ImGui::SameLine();
                    if (ImGui::Button("+ Plane")) createPlaneMesh("Plane");
                    ImGui::Separator();
                    for (int i = 0; i < (int)m_meshes.size(); ++i)
                    {
                        ImGui::PushID(i);
                        if (ImGui::Selectable(m_meshes[i].name.c_str(), m_selectedMesh == i)) {
                            m_selectedMesh = i;
                            m_selectedObject = -1;
                            m_selectedMaterial = -1;
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Materials"))
                {
                    ImGui::InputText("##NewMatName", m_newMaterialName, 64);
                    ImGui::SameLine();
                    if (ImGui::Button("+ New Material"))
                        createMaterial(m_newMaterialName);
                    ImGui::Separator();
                    for (int i = 0; i < (int)m_materials.size(); ++i)
                    {
                        ImGui::PushID(i);
                        if (ImGui::Selectable(m_materials[i].name.c_str(), m_selectedMaterial == i)) {
                            m_selectedMaterial = i;
                            m_selectedObject = -1;
                            m_selectedMesh = -1;
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }

        // Scene Graph Window
        if (m_showSceneWindow)
        {
            ImGui::Begin("Scene Graph", &m_showSceneWindow);
            ImGui::InputText("##NewObjName", m_newObjectName, 64);
            ImGui::SameLine();
            if (ImGui::Button("+ New Object") && !m_meshes.empty() && !m_materials.empty())
                createObject(m_newObjectName, 0, 0);
            ImGui::Separator();
            for (int i = 0; i < (int)m_sceneObjects.size(); ++i)
            {
                ImGui::PushID(i);
                // Use ##ID in label to ensure uniqueness
                std::string label = m_sceneObjects[i].name + "##" + std::to_string(i);
                
                // Gray out inactive objects in the list
                if (!m_sceneObjects[i].active)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                
                if (ImGui::Selectable(label.c_str(), m_selectedObject == i)) {
                    m_selectedObject = i;
                    m_selectedMaterial = -1;
                    m_selectedMesh = -1;
                }
                
                if (!m_sceneObjects[i].active)
                    ImGui::PopStyleColor();
                    
                ImGui::PopID();
            }

            ImGui::End();
        }

        // Inspector Window
        if (m_showInspectorWindow)
        {
            ImGui::Begin("Inspector", &m_showInspectorWindow);
            if (m_selectedObject >= 0 && m_selectedObject < (int)m_sceneObjects.size())
            {
            SceneObject& obj = m_sceneObjects[m_selectedObject];
                ImGui::Text("Object: %s", obj.name.c_str());
                ImGui::SameLine();
                ImGui::Checkbox("Active", &obj.active);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Object is submitted to render system (disabled = completely ignored)");
                ImGui::Separator();
                
                // Gizmo Controls
                ImGui::Text("Gizmo Mode:");
                ImGui::SameLine();
                if (ImGui::RadioButton("W", m_gizmoOperation == ImGuizmo::TRANSLATE))
                    m_gizmoOperation = ImGuizmo::TRANSLATE;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (W)");
                ImGui::SameLine();
                if (ImGui::RadioButton("E", m_gizmoOperation == ImGuizmo::ROTATE))
                    m_gizmoOperation = ImGuizmo::ROTATE;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");
                ImGui::SameLine();
                if (ImGui::RadioButton("R", m_gizmoOperation == ImGuizmo::SCALE))
                    m_gizmoOperation = ImGuizmo::SCALE;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");
                
                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::SameLine();
                
                if (ImGui::RadioButton("World", m_gizmoMode == ImGuizmo::WORLD))
                    m_gizmoMode = ImGuizmo::WORLD;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("World Space (T to toggle)");
                ImGui::SameLine();
                if (ImGui::RadioButton("Local", m_gizmoMode == ImGuizmo::LOCAL))
                    m_gizmoMode = ImGuizmo::LOCAL;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Local Space (T to toggle)");
                
                // Snap controls
                ImGui::Checkbox("Snap (hold Ctrl)", &m_useSnap);
                if (m_useSnap || ImGui::IsItemHovered())
                {
                    ImGui::DragFloat("Snap Translation", &m_snapTranslation, 0.1f, 0.1f, 10.0f);
                    ImGui::DragFloat("Snap Rotation", &m_snapRotation, 1.0f, 1.0f, 90.0f);
                    ImGui::DragFloat("Snap Scale", &m_snapScale, 0.01f, 0.01f, 1.0f);
                }
                
                ImGui::Separator();
                
                // Transform
                ImGui::DragFloat3("Position", &obj.position.x, 0.1f);
                ImGui::DragFloat3("Rotation", &obj.rotation.x, 1.0f);
                ImGui::DragFloat3("Scale", &obj.scale.x, 0.05f, 0.001f, 100.0f);
                ImGui::Checkbox("Animate", &obj.animate);
                if (obj.animate)
                    ImGui::SliderFloat("Speed", &obj.animationSpeed, 0.0f, 500.0f);

                ImGui::Separator();
                
                // Render Flags
                ImGui::Text("Render Flags:");
                ImGui::Checkbox("Visible", &obj.visible);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Object is rendered in main pass");
                ImGui::SameLine();
                ImGui::Checkbox("Frustum Cull", &obj.frustumCull);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Object participates in frustum culling");
                
                ImGui::Checkbox("Cast Shadows", &obj.castsShadows);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Object casts shadows (even if not visible)");
                ImGui::SameLine();
                ImGui::Checkbox("Receive Shadows", &obj.receivesShadows);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Object receives shadows from lights");

                
                ImGui::Separator();
                

                if (!m_meshes.empty())
                {
                    const char* currentMesh = (obj.meshIndex >= 0 && obj.meshIndex < (int)m_meshes.size()) 
                        ? m_meshes[obj.meshIndex].name.c_str() : "None";
                    if (ImGui::BeginCombo("Mesh", currentMesh))
                    {
                        for (int i = 0; i < (int)m_meshes.size(); i++)
                        {
                            bool isSelected = (obj.meshIndex == i);
                            if (ImGui::Selectable(m_meshes[i].name.c_str(), isSelected))
                                obj.meshIndex = i;
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                
                // Material dropdown
                if (!m_materials.empty())
                {
                    const char* currentMat = (obj.materialIndex >= 0 && obj.materialIndex < (int)m_materials.size()) 
                        ? m_materials[obj.materialIndex].name.c_str() : "None";
                    if (ImGui::BeginCombo("Material", currentMat))
                    {
                        for (int i = 0; i < (int)m_materials.size(); i++)
                        {
                            bool isSelected = (obj.materialIndex == i);
                            if (ImGui::Selectable(m_materials[i].name.c_str(), isSelected))
                                obj.materialIndex = i;
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                
                ImGui::Separator();
                if (ImGui::Button("Delete Object"))
                    deleteSelectedObject();
            }
            else if (m_selectedMaterial >= 0 && m_selectedMaterial < (int)m_materials.size())
            {
                MaterialResource& mat = m_materials[m_selectedMaterial];
                ImGui::Text("Material: %s", mat.name.c_str());
                ImGui::Separator();
                
                bool changed = false;
                changed |= ImGui::ColorEdit4("Albedo", &mat.data.albedo.r);
                changed |= ImGui::SliderFloat("Metallic", &mat.data.metallic, 0.0f, 1.0f);
                changed |= ImGui::SliderFloat("Roughness", &mat.data.roughness, 0.0f, 1.0f);
                changed |= ImGui::SliderFloat("AO", &mat.data.ao, 0.0f, 1.0f);
                
                ImGui::Separator();
                ImGui::Text("Textures:");
                
                // Albedo texture
                if (ImGui::Button("Load Albedo Texture"))
                {
                    if (openFileDialog())
                    {
                        hTexture tex = m_pRenderSystem->loadTexture(m_texturePath);
                        if (tex != 0)
                        {
                            m_pRenderSystem->setMaterialTexture(mat.handle, tex, 0);
                            mat.data.flags |= static_cast<UINT32>(MaterialFlags::ALBEDO_MAP);
                            mat.albedoPath = m_texturePath;
                            changed = true;
                        }
                    }
                }
                if (!mat.albedoPath.empty())
                    ImGui::TextWrapped("  -> %s", mat.albedoPath.c_str());
                
                // Normal texture
                if (ImGui::Button("Load Normal Texture"))
                {
                    if (openFileDialog())
                    {
                        hTexture tex = m_pRenderSystem->loadTexture(m_texturePath);
                        if (tex != 0)
                        {
                            m_pRenderSystem->setMaterialTexture(mat.handle, tex, 1);
                            mat.data.flags |= static_cast<UINT32>(MaterialFlags::NORMAL_MAP);
                            mat.normalPath = m_texturePath;
                            changed = true;
                        }
                    }
                }
                if (!mat.normalPath.empty())
                    ImGui::TextWrapped("  -> %s", mat.normalPath.c_str());
                
                if (changed)
                    m_pRenderSystem->updateMaterial(mat.handle, mat.data);
                
                ImGui::Separator();
                if (ImGui::Button("Delete Material"))
                    deleteSelectedMaterial();
            }
            else
            {
                ImGui::TextDisabled("Select an object or material");
            }
            ImGui::End();
        }

        // Environment Window
        if (m_showEnvironmentWindow)
        {
            ImGui::Begin("Environment", &m_showEnvironmentWindow);
            if (ImGui::ColorEdit3("Clear Color", m_clearColor))
                m_pRenderSystem->setClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
            ImGui::End();
        }

        // Camera Window
        if (m_showCameraWindow)
        {
            ImGui::Begin("Camera", &m_showCameraWindow);
            Quark::Vec3 pos = m_camera.position;
            if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
                m_camera.setPosition(pos);
            ImGui::DragFloat("Yaw", &m_yaw, 0.5f);
            ImGui::DragFloat("Pitch", &m_pitch, 0.5f, -89.0f, 89.0f);
            ImGui::SliderFloat("Move Speed", &m_cameraMoveSpeed, 0.1f, 50.0f);
            if (ImGui::Button("Reset Camera"))
            {
                m_camera.setPosition(Quark::Vec3(0.0f, 5.0f, -15.0f));
                m_yaw = 0.0f;
                m_pitch = 0.0f;
            }
            ImGui::End();
        }

        // Lighting Window
        if (m_showLightingWindow)
        {
            ImGui::Begin("Lighting", &m_showLightingWindow);
            
            // Light type buttons
            if (ImGui::Button("+ Directional"))
            {
                SceneLight light("Directional " + std::to_string(m_lights.size()), LightType::DIRECTIONAL);
                m_lights.push_back(light);
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Point"))
            {
                SceneLight light("Point " + std::to_string(m_lights.size()), LightType::POINT);
                light.position = m_camera.position;
                m_lights.push_back(light);
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Spot"))
            {
                SceneLight light("Spot " + std::to_string(m_lights.size()), LightType::SPOT);
                light.position = m_camera.position;
                light.direction = m_camera.forward();
                m_lights.push_back(light);
            }
            
            ImGui::Separator();
            ImGui::Text("Lights: %d / %d", (int)m_lights.size(), MAX_LIGHTS);
            ImGui::Separator();
            
            // Light list
            for (int i = 0; i < (int)m_lights.size(); ++i)
            {
                ImGui::PushID(i);
                SceneLight& light = m_lights[i];
                
                const char* typeIcon = "";
                switch (light.type)
                {
                    case LightType::DIRECTIONAL: typeIcon = "[D]"; break;
                    case LightType::POINT: typeIcon = "[P]"; break;
                    case LightType::SPOT: typeIcon = "[S]"; break;
                }
                
                ImGui::Checkbox("##Enabled", &light.enabled);
                ImGui::SameLine();
                ImGui::Text("%s", typeIcon);
                ImGui::SameLine();
                if (ImGui::Selectable(light.name.c_str(), m_selectedLight == i))
                    m_selectedLight = i;
                ImGui::PopID();
            }
            
            ImGui::Separator();
            
            // Selected light properties
            if (m_selectedLight >= 0 && m_selectedLight < (int)m_lights.size())
            {
                SceneLight& light = m_lights[m_selectedLight];
                
                ImGui::Text("Type: %s", 
                    light.type == LightType::DIRECTIONAL ? "Directional" :
                    light.type == LightType::POINT ? "Point" : "Spot");
                
                float col[3] = { light.color.r, light.color.g, light.color.b };
                if (ImGui::ColorEdit3("Color", col))
                    light.color = Quark::Color(col[0], col[1], col[2], 1.0f);
                
                ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
                
                if (light.type != LightType::DIRECTIONAL)
                {
                    ImGui::DragFloat3("Position", &light.position.x, 0.1f);
                    ImGui::SliderFloat("Range", &light.range, 1.0f, 100.0f);
                }
                
                if (light.type != LightType::POINT)
                {
                    ImGui::DragFloat3("Direction", &light.direction.x, 0.01f);
                    if (ImGui::Button("Normalize Dir"))
                        light.direction.Normalize();
                }
                
                if (light.type == LightType::SPOT)
                {
                    ImGui::SliderFloat("Inner Cutoff", &light.innerCutoff, 1.0f, 90.0f);
                    ImGui::SliderFloat("Outer Cutoff", &light.outerCutoff, 1.0f, 90.0f);
                }
                
                ImGui::Separator();
                ImGui::Checkbox("Cast Shadows", &light.castsShadows);
                if (light.castsShadows)
                {
                    const char* qualityItems[] = { "Hard (1x1)", "Medium (3x3)", "Soft (5x5)" };
                    int qualityIndex = light.shadowQuality - 1;  // 1-3 -> 0-2
                    if (ImGui::Combo("Shadow Quality", &qualityIndex, qualityItems, 3))
                        light.shadowQuality = qualityIndex + 1;  // 0-2 -> 1-3
                }
                
                ImGui::Separator();
                if (ImGui::Button("Delete Light"))
                {
                    m_lights.erase(m_lights.begin() + m_selectedLight);
                    m_selectedLight = -1;
                }
            }
            else
            {
                ImGui::TextDisabled("Select a light to edit");
            }
            
            ImGui::End();
        }

        // ==================== SKY SETTINGS WINDOW ====================
        if (m_showSkyWindow)
        {
            ImGui::Begin("Sky Settings", &m_showSkyWindow, ImGuiWindowFlags_AlwaysAutoResize);
            
            SkySettings sky = m_pRenderSystem->getSkySettings();
            bool skyChanged = false;
            
            // Helper macro for flag toggle
            #define SKY_FLAG_TOGGLE(flagName, label, tooltip) \
            { \
                bool flagValue = (static_cast<UINT32>(sky.flags) & static_cast<UINT32>(SkyFlags::flagName)) != 0; \
                if (ImGui::Checkbox(label, &flagValue)) \
                { \
                    if (flagValue) \
                        sky.flags = static_cast<SkyFlags>(static_cast<UINT32>(sky.flags) | static_cast<UINT32>(SkyFlags::flagName)); \
                    else \
                        sky.flags = static_cast<SkyFlags>(static_cast<UINT32>(sky.flags) & ~static_cast<UINT32>(SkyFlags::flagName)); \
                    skyChanged = true; \
                } \
                if (tooltip[0] && ImGui::IsItemHovered()) ImGui::SetTooltip(tooltip); \
            }
            
            // ======================== SUN ========================
            if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::TextDisabled("Direction & Intensity synced from Directional Light");
                
                SKY_FLAG_TOGGLE(SUN_ENABLED, "Sun Enabled", "Enable sun in atmosphere calculations");
                ImGui::SameLine();
                SKY_FLAG_TOGGLE(SUN_DISK_ENABLED, "Show Disk", "Render visible sun disk");
                
                ImGui::Separator();
                ImGui::Text("Sun Disk:");
                skyChanged |= ImGui::SliderFloat("Size##sun", &sky.sunDiskSize, 0.5f, 10.0f);
                skyChanged |= ImGui::SliderFloat("Softness##sun", &sky.sunDiskSoftness, 0.1f, 5.0f);
                skyChanged |= ImGui::SliderFloat("Bloom Strength##sun", &sky.sunBloomStrength, 0.0f, 2.0f);
                
                ImGui::Separator();
                ImGui::Text("Sun Colors:");
                skyChanged |= ImGui::ColorEdit3("Zenith Color##sun", &sky.sunColorZenith.x);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sun color when high in sky");
                skyChanged |= ImGui::ColorEdit3("Horizon Color##sun", &sky.sunColorHorizon.x);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sun color at sunrise/sunset");
                skyChanged |= ImGui::ColorEdit3("Average Color##sun", &sky.sunAverageColor.x);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("General sun tint for lighting");
            }
            
            // ======================== ATMOSPHERE ========================
            if (ImGui::CollapsingHeader("Atmosphere", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Scattering:");
                skyChanged |= ImGui::SliderFloat("Rayleigh Strength", &sky.rayleighStrength, 0.0f, 5.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Blue sky scattering - higher = more blue");
                skyChanged |= ImGui::SliderFloat("Mie Strength", &sky.mieStrength, 0.0f, 2.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sun glow/haze - higher = larger glow");
                skyChanged |= ImGui::SliderFloat("Mie Anisotropy", &sky.mieAnisotropy, 0.0f, 0.99f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Forward scattering bias (0.8 typical)");
                
                ImGui::Separator();
                ImGui::Text("Density:");
                skyChanged |= ImGui::SliderFloat("Density Falloff", &sky.densityFalloff, 0.5f, 10.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("How fast atmosphere thins with height");
                skyChanged |= ImGui::SliderFloat("Density Scale", &sky.atmosphereDensityScale, 0.1f, 3.0f);
                skyChanged |= ImGui::DragFloat("Atmosphere Height", &sky.atmosphereHeight, 100.0f, 1000.0f, 100000.0f);
                skyChanged |= ImGui::DragFloat("Planet Radius", &sky.atmospherePlanetRadius, 10000.0f, 1000000.0f, 10000000.0f, "%.0f");
                
                ImGui::Separator();
                ImGui::Text("Ozone Layer:");
                skyChanged |= ImGui::SliderFloat("Ozone Strength", &sky.ozoneStrength, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Subtle blue/orange tint effect");
                skyChanged |= ImGui::ColorEdit3("Ozone Absorption", &sky.ozoneAbsorption.x);
                
                ImGui::Separator();
                ImGui::Text("Sky Colors (Override):");
                SKY_FLAG_TOGGLE(FORCE_HORIZON_COLOR, "Custom Horizon", "");
                bool forceHorizon = (static_cast<UINT32>(sky.flags) & static_cast<UINT32>(SkyFlags::FORCE_HORIZON_COLOR)) != 0;
                if (forceHorizon)
                    skyChanged |= ImGui::ColorEdit3("Horizon Color", &sky.horizonColor.x);
                
                SKY_FLAG_TOGGLE(FORCE_ZENITH_COLOR, "Custom Zenith", "");
                bool forceZenith = (static_cast<UINT32>(sky.flags) & static_cast<UINT32>(SkyFlags::FORCE_ZENITH_COLOR)) != 0;
                if (forceZenith)
                    skyChanged |= ImGui::ColorEdit3("Zenith Color", &sky.zenithColor.x);
            }
            
            // ======================== NIGHT SKY ========================
            if (ImGui::CollapsingHeader("Night Sky"))
            {
                skyChanged |= ImGui::ColorEdit3("Night Color", &sky.nightSkyColor.x);
                skyChanged |= ImGui::SliderFloat("Night Strength", &sky.nightSkyStrength, 0.0f, 0.2f);
                
                ImGui::Separator();
                skyChanged |= ImGui::SliderFloat("Star Intensity", &sky.starIntensity, 0.0f, 3.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Procedural star brightness");
                skyChanged |= ImGui::SliderFloat("Moon Intensity", &sky.moonIntensity, 0.0f, 2.0f);
            }
            
            // ======================== CLOUDS ========================
            if (ImGui::CollapsingHeader("Clouds", ImGuiTreeNodeFlags_DefaultOpen))
            {
                SKY_FLAG_TOGGLE(CLOUDS_ENABLED, "Clouds Enabled", "Enable cloud rendering");
                
                bool cloudsEnabled = (static_cast<UINT32>(sky.flags) & static_cast<UINT32>(SkyFlags::CLOUDS_ENABLED)) != 0;
                if (cloudsEnabled)
                {
                    SKY_FLAG_TOGGLE(VOLUMETRIC_CLOUDS, "Volumetric Mode", "3D ray marching (high quality)");
                    SKY_FLAG_TOGGLE(CLOUDS_USE_SUN_LIGHT, "Use Sun Color", "Sunlight affects cloud color");
                    SKY_FLAG_TOGGLE(CLOUDS_CAST_SHADOWS, "Cast Shadows", "Clouds cast shadows on scene");
                    
                    bool volumetric = (static_cast<UINT32>(sky.flags) & static_cast<UINT32>(SkyFlags::VOLUMETRIC_CLOUDS)) != 0;
                    
                    // Quality preset
                    ImGui::Separator();
                    const char* qualityItems[] = { "Low (32)", "Medium (64)", "High (96)", "Ultra (128)", "Cinematic (192)" };
                    int volQualityIndex = static_cast<int>(sky.volumetricQuality);
                    if (ImGui::Combo("Volumetric Quality", &volQualityIndex, qualityItems, 5))
                    {
                        sky.volumetricQuality = static_cast<VolumetricCloudQuality>(volQualityIndex);
                        skyChanged = true;
                    }
                    
                    // ========== BASIC ==========
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Basic Settings");
                    skyChanged |= ImGui::SliderFloat("Coverage", &sky.cloudCoverage, 0.0f, 1.0f);
                    skyChanged |= ImGui::SliderFloat("Density", &sky.cloudDensity, 0.0f, 2.0f);
                    skyChanged |= ImGui::DragFloat("Height (2D)", &sky.cloudHeight, 50.0f, 500.0f, 10000.0f);
                    skyChanged |= ImGui::SliderFloat("Thickness (2D)", &sky.cloudThickness, 100.0f, 2000.0f);
                    
                    if (volumetric)
                    {
                        // ========== SHAPE ==========
                        if (ImGui::TreeNode("Shape"))
                        {
                            skyChanged |= ImGui::DragFloat("Layer Bottom", &sky.cloudLayerBottom, 50.0f, 500.0f, 10000.0f);
                            skyChanged |= ImGui::DragFloat("Layer Top", &sky.cloudLayerTop, 50.0f, 1000.0f, 15000.0f);
                            ImGui::Separator();
                            skyChanged |= ImGui::SliderFloat("Base Scale", &sky.cloudScale, 0.00005f, 0.001f, "%.5f");
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lower = larger cloud formations");
                            skyChanged |= ImGui::SliderFloat("Detail Scale", &sky.cloudDetailScale, 0.0005f, 0.01f, "%.4f");
                            skyChanged |= ImGui::SliderFloat("Erosion", &sky.cloudErosion, 0.0f, 1.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Detail noise erosion strength");
                            skyChanged |= ImGui::SliderFloat("Curliness", &sky.cloudCurliness, 0.0f, 1.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Wispy curl noise effect");
                            skyChanged |= ImGui::SliderFloat("Anvil Bias", &sky.cloudAnvilBias, -0.5f, 0.5f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cumulonimbus anvil shape at top");
                            ImGui::TreePop();
                        }
                        
                        // ========== DENSITY ==========
                        if (ImGui::TreeNode("Density"))
                        {
                            skyChanged |= ImGui::SliderFloat("Multiplier", &sky.cloudDensityMultiplier, 0.1f, 3.0f);
                            skyChanged |= ImGui::SliderFloat("Bottom Density", &sky.cloudDensityBottom, 0.0f, 2.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Density at cloud base");
                            skyChanged |= ImGui::SliderFloat("Top Density", &sky.cloudDensityTop, 0.0f, 1.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Density at cloud top (wispy)");
                            skyChanged |= ImGui::SliderFloat("Height Gradient", &sky.cloudHeightGradient, 0.0f, 1.0f);
                            skyChanged |= ImGui::SliderFloat("Edge Fade", &sky.cloudEdgeFade, 0.0f, 1.0f);
                            skyChanged |= ImGui::SliderFloat("Coverage Remap", &sky.cloudCoverageRemapping, 0.1f, 2.0f);
                            ImGui::TreePop();
                        }
                        
                        // ========== LIGHTING ==========
                        if (ImGui::TreeNode("Lighting"))
                        {
                            ImGui::Text("Features:");
                            SKY_FLAG_TOGGLE(CLOUD_LIGHTING_ENABLED, "Light Scattering", "Calculate sun light through clouds");
                            SKY_FLAG_TOGGLE(CLOUD_AMBIENT_ENABLED, "Ambient Light", "Add sky ambient to clouds");
                            SKY_FLAG_TOGGLE(CLOUD_POWDER_EFFECT, "Powder Effect", "Dark edges (Beer-Powder)");
                            SKY_FLAG_TOGGLE(CLOUD_SILVER_LINING, "Silver Lining", "Bright edges when backlit");
                            SKY_FLAG_TOGGLE(CLOUD_DETAIL_NOISE, "Detail Noise", "High-frequency erosion");
                            SKY_FLAG_TOGGLE(CLOUD_ATMOSPHERIC_PERSPECTIVE, "Atm. Perspective", "Distance fog fade");
                            
                            ImGui::Separator();
                            ImGui::Text("Coefficients:");
                            skyChanged |= ImGui::SliderFloat("Scattering", &sky.cloudScatteringCoeff, 0.01f, 0.2f);
                            skyChanged |= ImGui::SliderFloat("Extinction", &sky.cloudExtinctionCoeff, 0.01f, 0.2f);
                            skyChanged |= ImGui::SliderFloat("Phase G", &sky.cloudPhaseFunctionG, 0.0f, 0.99f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Henyey-Greenstein: 0=isotropic, 0.8+=forward scatter");
                            
                            ImGui::Separator();
                            ImGui::Text("Effects:");
                            skyChanged |= ImGui::SliderFloat("Powder Strength", &sky.cloudPowderCoeff, 0.0f, 5.0f);
                            skyChanged |= ImGui::SliderFloat("Multi-Scatter", &sky.cloudMultiScatterStrength, 0.0f, 1.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fake bounced light approximation");
                            skyChanged |= ImGui::SliderFloat("Ambient Strength", &sky.cloudAmbientStrength, 0.0f, 1.0f);
                            skyChanged |= ImGui::ColorEdit3("Ambient Color", &sky.cloudAmbientColor.x);
                            
                            ImGui::Separator();
                            ImGui::Text("Silver Lining:");
                            skyChanged |= ImGui::SliderFloat("Silver Intensity", &sky.cloudSilverIntensity, 0.0f, 2.0f);
                            skyChanged |= ImGui::SliderFloat("Silver Spread", &sky.cloudSilverSpread, 0.01f, 0.5f);
                            ImGui::TreePop();
                        }
                        
                        // ========== ANIMATION ==========
                        if (ImGui::TreeNode("Animation"))
                        {
                            skyChanged |= ImGui::SliderFloat("Wind Speed", &sky.cloudSpeed, 0.0f, 0.5f);
                            skyChanged |= ImGui::SliderFloat2("Wind Direction", &sky.cloudDirection.x, -1.0f, 1.0f);
                            skyChanged |= ImGui::DragFloat3("World Offset", &sky.cloudOffset.x, 10.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Manual cloud position offset");
                            ImGui::TreePop();
                        }
                        
                        // ========== WEATHER ==========
                        if (ImGui::TreeNode("Weather"))
                        {
                            skyChanged |= ImGui::SliderFloat("Weather Scale", &sky.cloudWeatherScale, 0.00001f, 0.0002f, "%.6f");
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Large-scale coverage variation");
                            skyChanged |= ImGui::SliderFloat2("Weather Offset", &sky.cloudWeatherOffset.x, -100.0f, 100.0f);
                            ImGui::Separator();
                            skyChanged |= ImGui::SliderFloat("Storminess", &sky.cloudStorminess, 0.0f, 1.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Darker, denser storm clouds");
                            skyChanged |= ImGui::SliderFloat("Rain Absorption", &sky.cloudRainAbsorption, 0.0f, 1.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Light absorption in rain clouds");
                            ImGui::TreePop();
                        }
                        
                        // ========== PERFORMANCE ==========
                        if (ImGui::TreeNode("Performance"))
                        {
                            ImGui::TextDisabled("0 = auto from quality preset");
                            skyChanged |= ImGui::SliderInt("Max Steps", &sky.cloudMaxSteps, 0, 256);
                            skyChanged |= ImGui::SliderInt("Light Steps", &sky.cloudMaxLightSteps, 0, 16);
                            ImGui::Separator();
                            skyChanged |= ImGui::SliderFloat("Step Size", &sky.cloudStepSize, 10.0f, 500.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Minimum step size in meters");
                            skyChanged |= ImGui::DragFloat("Max Distance", &sky.cloudMaxRenderDistance, 1000.0f, 10000.0f, 500000.0f);
                            skyChanged |= ImGui::SliderFloat("LOD Multiplier", &sky.cloudLodDistanceMultiplier, 0.5f, 2.0f);
                            skyChanged |= ImGui::SliderFloat("Blue Noise", &sky.cloudBlueNoiseScale, 0.0f, 2.0f);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Jitter to reduce banding artifacts");
                            ImGui::TreePop();
                        }
                    }
                    
                    // ========== COLOR ==========
                    if (ImGui::TreeNode("Color"))
                    {
                        SKY_FLAG_TOGGLE(FORCE_CLOUD_COLOR, "Custom Cloud Color", "");
                        bool forceCloudColor = (static_cast<UINT32>(sky.flags) & static_cast<UINT32>(SkyFlags::FORCE_CLOUD_COLOR)) != 0;
                        if (forceCloudColor)
                            skyChanged |= ImGui::ColorEdit3("Cloud Color", &sky.cloudColor.x);
                        
                        skyChanged |= ImGui::SliderFloat("Sun Influence", &sky.cloudSunLightInfluence, 0.0f, 1.0f);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How much sun color affects clouds");
                        skyChanged |= ImGui::SliderFloat("Light Absorption", &sky.cloudLightAbsorption, 0.0f, 1.0f);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shadow darkness on clouds");
                        ImGui::TreePop();
                    }
                }
            }
            
            // ======================== QUALITY ========================
            if (ImGui::CollapsingHeader("General Quality"))
            {
                const char* qualityItems[] = { "Low", "Medium", "High", "Ultra" };
                int qualityIndex = static_cast<int>(sky.quality);
                if (ImGui::Combo("Sky Quality", &qualityIndex, qualityItems, 4))
                {
                    sky.quality = static_cast<SkyQuality>(qualityIndex);
                    skyChanged = true;
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Overall sky rendering quality");
            }
            
            // ======================== PRESETS ========================
            if (ImGui::CollapsingHeader("Presets"))
            {
                ImGui::TextDisabled("Quick environment setups");
                
                if (ImGui::Button("Earth Default", ImVec2(120, 0)))
                {
                    sky = SkySettings(); // Default values
                    skyChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Mars", ImVec2(120, 0)))
                {
                    sky.horizonColor = Quark::Vec3(0.9f, 0.5f, 0.3f);
                    sky.zenithColor = Quark::Vec3(0.6f, 0.3f, 0.2f);
                    sky.rayleighStrength = 0.1f;
                    sky.mieStrength = 3.0f;
                    sky.atmosphereHeight = 11000.0f;
                    sky.atmospherePlanetRadius = 3389500.0f;
                    sky.flags = static_cast<SkyFlags>(static_cast<UINT32>(sky.flags) | 
                        static_cast<UINT32>(SkyFlags::FORCE_HORIZON_COLOR) |
                        static_cast<UINT32>(SkyFlags::FORCE_ZENITH_COLOR));
                    skyChanged = true;
                }
                
                if (ImGui::Button("Sunset", ImVec2(120, 0)))
                {
                    sky.sunColorHorizon = Quark::Vec3(1.0f, 0.4f, 0.1f);
                    sky.horizonColor = Quark::Vec3(1.0f, 0.5f, 0.2f);
                    sky.mieStrength = 1.5f;
                    sky.flags = static_cast<SkyFlags>(static_cast<UINT32>(sky.flags) | static_cast<UINT32>(SkyFlags::FORCE_HORIZON_COLOR));
                    skyChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Overcast", ImVec2(120, 0)))
                {
                    sky.cloudCoverage = 0.9f;
                    sky.cloudDensity = 1.5f;
                    sky.cloudStorminess = 0.3f;
                    sky.sunIntensity = 0.5f;
                    skyChanged = true;
                }
                
                if (ImGui::Button("Night", ImVec2(120, 0)))
                {
                    sky.sunIntensity = 0.0f;
                    sky.nightSkyStrength = 0.1f;
                    sky.starIntensity = 2.0f;
                    sky.moonIntensity = 0.8f;
                    skyChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear Sky", ImVec2(120, 0)))
                {
                    sky.cloudCoverage = 0.0f;
                    sky.flags = static_cast<SkyFlags>(static_cast<UINT32>(sky.flags) & ~static_cast<UINT32>(SkyFlags::CLOUDS_ENABLED));
                    skyChanged = true;
                }
            }
            
            #undef SKY_FLAG_TOGGLE
            
            if (skyChanged)
            {
                m_pRenderSystem->setSkySettings(sky);
            }
            
            ImGui::End();
        }
        // End DockSpace window
        ImGui::End();

        ImGui::Render();

    }

    void run()
    {
        while (m_pWindow && m_pWindow->isRunning())
        {
            float currentTime = static_cast<float>(std::chrono::duration<double>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count());
            float deltaTime = currentTime - m_lastFrameTime;
            m_lastFrameTime = currentTime;
            m_time += deltaTime;
            deltaTime = (std::min)(deltaTime, 0.1f);

            updateCamera(deltaTime);
            updateScene(deltaTime);
            submitLights();

            m_pWindow->pollMessages();

            m_pRenderSystem->setTime(m_time);
            m_pRenderSystem->setDeltaTime(deltaTime);

            m_pRenderSystem->renderFrame();

            // Render ImGui overlay
            renderImGui();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            m_pRenderSystem->endFrame();
        }
    }

    LRESULT handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_KEYDOWN:
            if (wParam < 256) m_keys[wParam] = true;
            
            // Gizmo shortcuts - Only if not controlling camera AND not typing in ImGui
            if (!ImGui::GetIO().WantCaptureKeyboard && !m_mouseControlEnabled)
            {
                bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                
                // Ctrl + Key combinations
                if (ctrlPressed)
                {
                    switch (wParam)
                    {
                    case 'Z':
                        undo();
                        break;
                    case 'Y':
                        redo();
                        break;
                    case 'C':
                        copySelectedObject();
                        break;
                    case 'V':
                        pasteObject();
                        break;
                    case 'D':
                        duplicateSelectedObject();
                        break;
                    }
                }
                else
                {
                    // Normal key shortcuts (without Ctrl)
                    switch (wParam)
                    {
                    case 'W':
                        m_gizmoOperation = ImGuizmo::TRANSLATE;
                        break;
                    case 'E':
                        m_gizmoOperation = ImGuizmo::ROTATE;
                        break;
                    case 'R':
                        m_gizmoOperation = ImGuizmo::SCALE;
                        break;
                    case 'T':
                        m_gizmoMode = (m_gizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
                        break;
                    case 'Q':
                        // Deselect current object
                        m_selectedObject = -1;
                        break;
                    case VK_DELETE:
                        // Delete selected object
                        if (m_selectedObject >= 0)
                        {
                            saveUndoState();
                            deleteSelectedObject();
                        }
                        break;
                    case VK_ESCAPE:
                        // Deselect everything
                        m_selectedObject = -1;
                        m_selectedMaterial = -1;
                        m_selectedMesh = -1;
                        m_selectedLight = -1;
                        break;
                    }
                }
                
                // Ctrl key for snap toggle
                if (wParam == VK_CONTROL)
                    m_useSnap = true;
            }

            break;
        case WM_KEYUP:
            if (wParam < 256) m_keys[wParam] = false;
            if (wParam == VK_CONTROL) m_useSnap = false;
            break;
        case WM_LBUTTONDOWN:
            // Only request picking if:
            // 1. ImGui doesn't want the mouse
            // 2. Gizmo is not being used
            // 3. Gizmo is not hovered
            if (!ImGui::GetIO().WantCaptureMouse && !m_gizmoIsUsing && !m_gizmoIsOver)
            {
                m_pickingRequested = true;
            }
            break;
        case WM_RBUTTONDOWN:
            if (!ImGui::GetIO().WantCaptureMouse)
            {
                m_mouseControlEnabled = true;
                m_firstMouse = true;
                ShowCursor(FALSE);
                SetCapture(hwnd);
            }
            break;
        case WM_RBUTTONUP:
            // Only restore cursor if we were controlling the camera
            if (m_mouseControlEnabled)
            {
                m_mouseControlEnabled = false;
                ShowCursor(TRUE);
                ReleaseCapture();
            }
            break;

        case WM_MOUSEMOVE:
            if (m_mouseControlEnabled)
            {
                float xpos = static_cast<float>(LOWORD(lParam));
                float ypos = static_cast<float>(HIWORD(lParam));
                if (m_firstMouse)
                {
                    m_lastMouseX = xpos;
                    m_lastMouseY = ypos;
                    m_firstMouse = false;
                }
                m_yaw -= (xpos - m_lastMouseX) * m_cameraRotateSpeed;
                m_pitch -= (m_lastMouseY - ypos) * m_cameraRotateSpeed;
                m_pitch = (std::max)(-89.0f, (std::min)(89.0f, m_pitch));
                m_lastMouseX = xpos;
                m_lastMouseY = ypos;

                RECT rect;
                GetClientRect(hwnd, &rect);
                POINT center = { rect.right / 2, rect.bottom / 2 };
                ClientToScreen(hwnd, &center);
                SetCursorPos(center.x, center.y);
                m_lastMouseX = static_cast<float>(rect.right / 2);
                m_lastMouseY = static_cast<float>(rect.bottom / 2);
            }
            break;
        case WM_SIZE:
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                if (width > 0 && height > 0 && m_pRenderSystem)
                {
                    m_pRenderSystem->onResize(width, height);
                    m_camera.setAspect(static_cast<float>(width) / static_cast<float>(height));
                }
            }
            break;
        }
        return 0;
    }

    void updateCamera(float dt)
    {
        if (!m_mouseControlEnabled) return;

        float moveSpeed = m_cameraMoveSpeed * dt;
        if (m_keys[VK_SHIFT]) moveSpeed *= 2.0f;

        if (m_keys['W']) m_camera.translateLocal(Quark::Vec3::Forward() * moveSpeed);
        if (m_keys['S']) m_camera.translateLocal(Quark::Vec3::Back() * moveSpeed);
        if (m_keys['A']) m_camera.translateLocal(Quark::Vec3::Right() * moveSpeed);
        if (m_keys['D']) m_camera.translateLocal(Quark::Vec3::Left() * moveSpeed);
        if (m_keys['E']) m_camera.translate(Quark::Vec3::Up() * moveSpeed);
        if (m_keys['Q']) m_camera.translate(Quark::Vec3::Down() * moveSpeed);

        Quark::Vec3 eulerAngles(Quark::Radians(m_pitch), Quark::Radians(m_yaw), 0.0f);
        m_camera.setEulerAngles(eulerAngles);
    }

    void updateScene(float dt)
    {
        // New API: No need to clearRenderQueue, submit does it automatically per frame

        for (auto& obj : m_sceneObjects)
        {
            // Skip if not active (completely ignored by render system)
            if (!obj.active)
                continue;
                
            // Skip if no valid mesh/material
            if (obj.meshIndex < 0 || obj.materialIndex < 0)
                continue;
            if (obj.meshIndex >= (int)m_meshes.size() || obj.materialIndex >= (int)m_materials.size())
                continue;

            if (obj.animate)
            {
                obj.rotation.y += dt * obj.animationSpeed * 50.0f;
                if (obj.rotation.y > 360.0f) obj.rotation.y -= 360.0f;
            }

            // Build transform
            Quark::Mat4 translation = Quark::Mat4::Translation(obj.position);
            Quark::Mat4 rotation = Quark::Mat4::RotationX(Quark::Radians(obj.rotation.x)) *
                                   Quark::Mat4::RotationY(Quark::Radians(obj.rotation.y)) *
                                   Quark::Mat4::RotationZ(Quark::Radians(obj.rotation.z));
            Quark::Mat4 scale = Quark::Mat4::Scaling(obj.scale);
            Quark::Mat4 worldMatrix = translation * rotation * scale;

            // Calculate world bounds from local mesh bounds
            const Quark::AABB& localBounds = m_meshes[obj.meshIndex].boundingBox;
            Quark::Vec3 halfExtent = (localBounds.maxBounds - localBounds.minBounds) * 0.5f;
            Quark::Vec3 scaledExtent = Quark::Vec3(
                halfExtent.x * obj.scale.x,
                halfExtent.y * obj.scale.y,
                halfExtent.z * obj.scale.z
            );
            Quark::AABB worldBounds;
            worldBounds.minBounds = obj.position - scaledExtent;
            worldBounds.maxBounds = obj.position + scaledExtent;


            // New API: submit(RenderObject) with flags based on SceneObject settings
            RenderObject renderObj;
            renderObj.mesh = m_meshes[obj.meshIndex].handle;
            renderObj.material = m_materials[obj.materialIndex].handle;
            renderObj.worldMatrix = worldMatrix;
            renderObj.worldAABB = worldBounds;
            
            // Build flags from SceneObject properties
            renderObj.flags = RenderObjectFlags::NONE;
            if (obj.visible)
                renderObj.flags |= RenderObjectFlags::VISIBLE;
            if (obj.frustumCull)
                renderObj.flags |= RenderObjectFlags::FRUSTUM_CULL;
            if (obj.castsShadows)
                renderObj.flags |= RenderObjectFlags::CAST_SHADOW;
            if (obj.receivesShadows)
                renderObj.flags |= RenderObjectFlags::RECEIVE_SHADOW;
            
            m_pRenderSystem->submit(renderObj);


        }

    }

    void shutdown()
    {
        if (m_pRenderSystem)
        {
            for (auto& mesh : m_meshes)
                m_pRenderSystem->destroyMesh(mesh.handle);
            m_meshes.clear();

            for (auto& mat : m_materials)
                m_pRenderSystem->destroyMaterial(mat.handle);
            m_materials.clear();

            m_sceneObjects.clear();

            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            // Shutdown and destroy using factory function
            m_pRenderSystem->shutdown();
            if (m_pfnDestroyRenderSystem)
            {
                m_pfnDestroyRenderSystem(m_pRenderSystem);
            }
            m_pRenderSystem = nullptr;
        }

        if (m_pRHI)
        {
            // Destroy RHI using factory function
            if (m_pfnDestroyRenderBackend)
            {
                m_pfnDestroyRenderBackend(m_pRHI);
            }
            m_pRHI = nullptr;
        }

        // Unload DLLs
        if (m_hRenderSystemDLL)
        {
            FreeLibrary(m_hRenderSystemDLL);
            m_hRenderSystemDLL = nullptr;
        }

        if (m_hRHIDLL)
        {
            FreeLibrary(m_hRHIDLL);
            m_hRHIDLL = nullptr;
        }

        if (m_pWindow)
        {
            delete m_pWindow;
            m_pWindow = nullptr;
        }
    }
};

// ==================== MAIN ====================

int main()
{
    RenderEditor app;

    if (!app.init())
    {
        std::cerr << "Failed to initialize render editor!\n";
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return -1;
    }

    app.run();

    std::cout << "\nRender editor closed successfully!\n";
    // std::cin.get();
    return 0;
}