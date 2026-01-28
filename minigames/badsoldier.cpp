#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <cmath>
#include <functional>

#define NOMINMAX
#ifdef _WIN32
#include <Windows.h>
#endif

// Engine Includes
#include "../modules/graphics/rendersystem/rendersystemapi.h"
#include "../modules/graphics/rendersystem/renderobject.h"
#include "../modules/graphics/rendersystem/meshdata.h"
#include "../modules/graphics/rendersystem/material.h"
#include "../modules/graphics/rendersystem/lighting.h"
#include "../modules/graphics/rendersystem/camera.h"
#include "../modules/graphics/rendersystem/rhi.h"
#include "../modules/headeronly/globaltypes.h"

// ImGui Includes
#include "../thirdparty/imgui/imgui.h"
#include "../thirdparty/imgui/backends/imgui_impl_win32.h"
#include "../thirdparty/imgui/backends/imgui_impl_dx11.h"

// DLL functions
typedef RenderSystemAPI* (*CreateRenderSystemFn)();
typedef void (*DestroyRenderSystemFn)(RenderSystemAPI*);
typedef RHI* (*CreateRenderBackendFn)();
typedef void (*DestroyRenderBackendFn)(RHI*);

// Forward declare ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ==================== WINDOW CLASS ====================
class GameWindow {
    HWND m_hwnd;
    HINSTANCE m_hInstance;
    bool m_isRunning;
    UINT32 m_width, m_height;
    std::function<void(UINT32, UINT32)> m_resizeCallback;
    
    static GameWindow* s_instance;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (s_instance) return s_instance->handleMessage(hwnd, msg, wParam, lParam);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;
        
        switch (msg) {
            case WM_CLOSE: m_isRunning = false; return 0;
            case WM_DESTROY: PostQuitMessage(0); return 0;
            case WM_KEYDOWN: 
                break;
            case WM_SIZE:
                 if (m_resizeCallback) m_resizeCallback(LOWORD(lParam), HIWORD(lParam));
                 m_width = LOWORD(lParam);
                 m_height = HIWORD(lParam);
                 return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

public:
    GameWindow() : m_hwnd(nullptr), m_hInstance(GetModuleHandle(nullptr)), m_isRunning(false), m_width(0), m_height(0) { s_instance = this; }
    ~GameWindow() { if (m_hwnd) DestroyWindow(m_hwnd); s_instance = nullptr; }

    void setResizeCallback(std::function<void(UINT32, UINT32)> cb) { m_resizeCallback = cb; }

    bool create(const char* title) {
        m_width = GetSystemMetrics(SM_CXSCREEN);
        m_height = GetSystemMetrics(SM_CYSCREEN);
        
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = m_hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = "BadSoldierWindow";

        RegisterClassEx(&wc);
        
        m_hwnd = CreateWindowEx(
            0, "BadSoldierWindow", title, 
            WS_POPUP | WS_VISIBLE, // Fullscreen (Popup)
            0, 0, m_width, m_height, 
            nullptr, nullptr, m_hInstance, nullptr
        );

        if (!m_hwnd) return false;
        
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        m_isRunning = true;
        return true;
    }

    void pollMessages() {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void close() { m_isRunning = false; }
    HWND getHandle() const { return m_hwnd; }
    bool isRunning() const { return m_isRunning; }
    UINT32 getWidth() const { return m_width; }
    UINT32 getHeight() const { return m_height; }
};

GameWindow* GameWindow::s_instance = nullptr;

// ==================== MATH UTILS ====================
float randomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

// ==================== MESH GENERATORS ====================
MeshData CreateSphereMesh(float radius, int slices = 16, int stacks = 16) {
    MeshData mesh = {};
    std::vector<Vertex> vertices;
    std::vector<UINT32> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = 3.14159f * float(i) / float(stacks);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * 3.14159f * float(j) / float(slices);
            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);
            
            Vertex v;
            v.position = Quark::Vec3(x, y, z) * radius;
            v.normal = Quark::Vec3(x, y, z);
            v.texCoord = Quark::Vec2((float)j / slices, (float)i / stacks);
            v.tangent = Quark::Vec3(-sin(theta), 0, cos(theta)); // approx
            v.bitangent = Quark::Vec3(0,0,1); // placeholder
            vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = (i * (slices + 1)) + j;
            int second = first + slices + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    Vertex* vRaw = new Vertex[vertices.size()];
    std::copy(vertices.begin(), vertices.end(), vRaw);
    UINT32* iRaw = new UINT32[indices.size()];
    std::copy(indices.begin(), indices.end(), iRaw);

    mesh.vertices = vRaw;
    mesh.vertexCount = (UINT32)vertices.size();
    mesh.indices = iRaw;
    mesh.indexCount = (UINT32)indices.size();
    mesh.boundingBox.minBounds = Quark::Vec3(-radius, -radius, -radius);
    mesh.boundingBox.maxBounds = Quark::Vec3(radius, radius, radius);
    return mesh;
}

MeshData CreateCapsuleMesh(float radius, float height, int slices = 16) {
    MeshData mesh = {};
    std::vector<Vertex> vertices;
    std::vector<UINT32> indices;
    
    float halfHeight = height * 0.5f;

    // Top Cap (Center)
    vertices.push_back({Quark::Vec3(0, halfHeight, 0), Quark::Vec3(0,1,0), Quark::Vec2(0.5f, 0.5f), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1)});
    int topIndex = 0;

    // Top Ring
    for (int i = 0; i <= slices; ++i) {
        float theta = 2.0f * 3.14159f * float(i) / float(slices);
        float x = cos(theta) * radius;
        float z = sin(theta) * radius;
        vertices.push_back({Quark::Vec3(x, halfHeight, z), Quark::Vec3(x,0,z).Normalized(), Quark::Vec2((float)i/slices, 0), Quark::Vec3(-sin(theta), 0, cos(theta)), Quark::Vec3(0,1,0)});
    }

    // Bottom Ring
    for (int i = 0; i <= slices; ++i) {
        float theta = 2.0f * 3.14159f * float(i) / float(slices);
        float x = cos(theta) * radius;
        float z = sin(theta) * radius;
        vertices.push_back({Quark::Vec3(x, -halfHeight, z), Quark::Vec3(x,0,z).Normalized(), Quark::Vec2((float)i/slices, 1), Quark::Vec3(-sin(theta), 0, cos(theta)), Quark::Vec3(0,1,0)});
    }

    // Bottom Cap (Center)
    vertices.push_back({Quark::Vec3(0, -halfHeight, 0), Quark::Vec3(0,-1,0), Quark::Vec2(0.5f, 0.5f), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1)});
    int bottomIndex = (int)vertices.size() - 1;

    // Indices
    int topRingStart = 1;
    int bottomRingStart = topRingStart + slices + 1;

    for (int i = 0; i < slices; ++i) {
        // Top cap
        indices.push_back(topIndex);
        indices.push_back(topRingStart + i + 1);
        indices.push_back(topRingStart + i);

        // Cylinder body
        indices.push_back(topRingStart + i);
        indices.push_back(topRingStart + i + 1);
        indices.push_back(bottomRingStart + i);

        indices.push_back(bottomRingStart + i);
        indices.push_back(topRingStart + i + 1);
        indices.push_back(bottomRingStart + i + 1);

        // Bottom cap
        indices.push_back(bottomIndex);
        indices.push_back(bottomRingStart + i);
        indices.push_back(bottomRingStart + i + 1);
    }

    Vertex* vRaw = new Vertex[vertices.size()];
    std::copy(vertices.begin(), vertices.end(), vRaw);
    UINT32* iRaw = new UINT32[indices.size()];
    std::copy(indices.begin(), indices.end(), iRaw);

    mesh.vertices = vRaw;
    mesh.vertexCount = (UINT32)vertices.size();
    mesh.indices = iRaw;
    mesh.indexCount = (UINT32)indices.size();
    mesh.boundingBox.minBounds = Quark::Vec3(-radius, -halfHeight, -radius);
    mesh.boundingBox.maxBounds = Quark::Vec3(radius, halfHeight, radius);

    return mesh;
}



MeshData CreateBoxMesh(Quark::Vec3 size) {
    MeshData mesh = {};
    std::vector<Vertex> vertices;
    std::vector<UINT32> indices;

    Quark::Vec3 half = size * 0.5f;

    // Helper to add face
    auto addFace = [&](Quark::Vec3 p1, Quark::Vec3 p2, Quark::Vec3 p3, Quark::Vec3 p4, Quark::Vec3 n) {
         UINT32 base = (UINT32)vertices.size();
         vertices.push_back({p1, n, Quark::Vec2(0,0), Quark::Vec3(0,0,0), Quark::Vec3(0,0,0)});
         vertices.push_back({p2, n, Quark::Vec2(1,0), Quark::Vec3(0,0,0), Quark::Vec3(0,0,0)});
         vertices.push_back({p3, n, Quark::Vec2(1,1), Quark::Vec3(0,0,0), Quark::Vec3(0,0,0)});
         vertices.push_back({p4, n, Quark::Vec2(0,1), Quark::Vec3(0,0,0), Quark::Vec3(0,0,0)});
         indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
         indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);
    };

    float minX = -half.x, maxX = half.x;
    float minY = -half.y, maxY = half.y;
    float minZ = -half.z, maxZ = half.z;

    addFace(Quark::Vec3(minX, minY, maxZ), Quark::Vec3(maxX, minY, maxZ), Quark::Vec3(maxX, maxY, maxZ), Quark::Vec3(minX, maxY, maxZ), Quark::Vec3(0,0,1));
    addFace(Quark::Vec3(maxX, minY, minZ), Quark::Vec3(minX, minY, minZ), Quark::Vec3(minX, maxY, minZ), Quark::Vec3(maxX, maxY, minZ), Quark::Vec3(0,0,-1));
    addFace(Quark::Vec3(maxX, minY, maxZ), Quark::Vec3(maxX, minY, minZ), Quark::Vec3(maxX, maxY, minZ), Quark::Vec3(maxX, maxY, maxZ), Quark::Vec3(1,0,0));
    addFace(Quark::Vec3(minX, minY, minZ), Quark::Vec3(minX, minY, maxZ), Quark::Vec3(minX, maxY, maxZ), Quark::Vec3(minX, maxY, minZ), Quark::Vec3(-1,0,0));
    addFace(Quark::Vec3(minX, maxY, maxZ), Quark::Vec3(maxX, maxY, maxZ), Quark::Vec3(maxX, maxY, minZ), Quark::Vec3(minX, maxY, minZ), Quark::Vec3(0,1,0));
    addFace(Quark::Vec3(minX, minY, minZ), Quark::Vec3(maxX, minY, minZ), Quark::Vec3(maxX, minY, maxZ), Quark::Vec3(minX, minY, maxZ), Quark::Vec3(0,-1,0));

    Vertex* vRaw = new Vertex[vertices.size()];
    std::copy(vertices.begin(), vertices.end(), vRaw);
    UINT32* iRaw = new UINT32[indices.size()];
    std::copy(indices.begin(), indices.end(), iRaw);

    mesh.vertices = vRaw;
    mesh.vertexCount = (UINT32)vertices.size();
    mesh.indices = iRaw;
    mesh.indexCount = (UINT32)indices.size();
    mesh.boundingBox.minBounds = -half;
    mesh.boundingBox.maxBounds = half;
    return mesh;
}

MeshData CreateGunMesh() {
    MeshData mesh = {};
    std::vector<Vertex> vertices;
    std::vector<UINT32> indices;

    auto addBox = [&](Quark::Vec3 center, Quark::Vec3 size) {
         Quark::Vec3 half = size * 0.5f;
         UINT32 startIdx = (UINT32)vertices.size();
         
         // 8 corners
         Quark::Vec3 p[8];
         p[0] = center + Quark::Vec3(-half.x, -half.y,  half.z);
         p[1] = center + Quark::Vec3( half.x, -half.y,  half.z);
         // Let's do faces directly to get normals right
         
         // Helper macro/lambda for face
         auto addFace = [&](Quark::Vec3 p1, Quark::Vec3 p2, Quark::Vec3 p3, Quark::Vec3 p4, Quark::Vec3 n) {
             UINT32 base = (UINT32)vertices.size();
             vertices.push_back({p1, n, Quark::Vec2(0,0), Quark::Vec3(0,0,0), Quark::Vec3(0,0,0)});
             vertices.push_back({p2, n, Quark::Vec2(1,0), Quark::Vec3(0,0,0), Quark::Vec3(0,0,0)});
             vertices.push_back({p3, n, Quark::Vec2(1,1), Quark::Vec3(0,0,0), Quark::Vec3(0,0,0)});
             vertices.push_back({p4, n, Quark::Vec2(0,1), Quark::Vec3(0,0,0), Quark::Vec3(0,0,0)});
             indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
             indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);
         };

         // Bounds
         float minX = center.x - half.x, maxX = center.x + half.x;
         float minY = center.y - half.y, maxY = center.y + half.y;
         float minZ = center.z - half.z, maxZ = center.z + half.z;

         // Front (+Z)
         addFace(Quark::Vec3(minX, minY, maxZ), Quark::Vec3(maxX, minY, maxZ), Quark::Vec3(maxX, maxY, maxZ), Quark::Vec3(minX, maxY, maxZ), Quark::Vec3(0,0,1));
         // Back (-Z)
         addFace(Quark::Vec3(maxX, minY, minZ), Quark::Vec3(minX, minY, minZ), Quark::Vec3(minX, maxY, minZ), Quark::Vec3(maxX, maxY, minZ), Quark::Vec3(0,0,-1));
         // Right (+X)
         addFace(Quark::Vec3(maxX, minY, maxZ), Quark::Vec3(maxX, minY, minZ), Quark::Vec3(maxX, maxY, minZ), Quark::Vec3(maxX, maxY, maxZ), Quark::Vec3(1,0,0));
         // Left (-X)
         addFace(Quark::Vec3(minX, minY, minZ), Quark::Vec3(minX, minY, maxZ), Quark::Vec3(minX, maxY, maxZ), Quark::Vec3(minX, maxY, minZ), Quark::Vec3(-1,0,0));
         // Top (+Y)
         addFace(Quark::Vec3(minX, maxY, maxZ), Quark::Vec3(maxX, maxY, maxZ), Quark::Vec3(maxX, maxY, minZ), Quark::Vec3(minX, maxY, minZ), Quark::Vec3(0,1,0));
         // Bottom (-Y)
         addFace(Quark::Vec3(minX, minY, minZ), Quark::Vec3(maxX, minY, minZ), Quark::Vec3(maxX, minY, maxZ), Quark::Vec3(minX, minY, maxZ), Quark::Vec3(0,-1,0));
    };

    // Barrel
    addBox(Quark::Vec3(0, 0, 0.2f), Quark::Vec3(0.1f, 0.1f, 0.6f));
    // Handle
    addBox(Quark::Vec3(0, -0.15f, -0.05f), Quark::Vec3(0.08f, 0.3f, 0.1f));

    // Convert
    Vertex* vRaw = new Vertex[vertices.size()];
    std::copy(vertices.begin(), vertices.end(), vRaw);
    UINT32* iRaw = new UINT32[indices.size()];
    std::copy(indices.begin(), indices.end(), iRaw);

    mesh.vertices = vRaw;
    mesh.vertexCount = (UINT32)vertices.size();
    mesh.indices = iRaw;
    mesh.indexCount = (UINT32)indices.size();
    mesh.boundingBox.minBounds = Quark::Vec3(-1, -1, -1);
    mesh.boundingBox.maxBounds = Quark::Vec3(1, 1, 1);
    return mesh;
}

MeshData CreatePlaneMesh(float size) {
     static std::vector<Vertex> v;
     static std::vector<UINT32> i;
     v = {
        { Quark::Vec3(-size, 0, -size), Quark::Vec3(0,1,0), Quark::Vec2(0,0), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1) },
        { Quark::Vec3( size, 0, -size), Quark::Vec3(0,1,0), Quark::Vec2(1,0), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1) },
        { Quark::Vec3( size, 0,  size), Quark::Vec3(0,1,0), Quark::Vec2(1,1), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1) },
        { Quark::Vec3(-size, 0,  size), Quark::Vec3(0,1,0), Quark::Vec2(0,1), Quark::Vec3(1,0,0), Quark::Vec3(0,0,1) }
     };
     i = { 0,2,1, 0,3,2 }; // CCW
     
     MeshData m = {};
     m.vertices = v.data();
     m.vertexCount = 4;
     m.indices = i.data();
     m.indexCount = 6;
     m.boundingBox.minBounds = Quark::Vec3(-size, 0, -size);
     m.boundingBox.maxBounds = Quark::Vec3(size, 0, size);
     return m;
}

// ==================== GAME ENTITIES ====================

struct Projectile {
    Quark::Vec3 position;
    Quark::Vec3 velocity;
    float lifeTime;
    bool isEnemy;
    bool active;
    hMesh mesh;
    hMaterial material;
};

struct Enemy {
    Enemy() : position(0,0,0), rotation(0), hp(100), shootTimer(0), active(false), meshBody(0), meshWeapon(0), matBody(0) {}
    Quark::Vec3 position;
    float rotation;
    float hp;
    float shootTimer;
    bool active;
    hMesh meshBody;
    hMesh meshWeapon;
    hMaterial matBody;
};

struct Player {
    Quark::Vec3 position;
    float hp;
    int ammo;
    float shootCooldown;
    hMesh mesh;
    hMaterial material;
};

struct Pickup {
    Quark::Vec3 position;
    bool active;
    float rotation;
    int type; // 0 = Ammo, 1 = Health
    hMesh mesh;
    hMaterial material;
};

// ==================== GAME CLASS ====================
class BadSoldierGame {
    GameWindow* m_window;
    RenderSystemAPI* m_rs;
    RHI* m_rhi;
    Camera m_camera;
    
    // Resources
    hMesh m_capsuleMesh;
    hMesh m_sphereMesh;
    hMesh m_cubeMesh;
    hMesh m_planeMesh;
    hMesh m_gunMesh;

    hMaterial m_matPlayer;
    hMaterial m_matEnemy;
    hMaterial m_matBulletPlayer;
    hMaterial m_matBulletEnemy;
    hMaterial m_matGround;
    hMaterial m_matPickup;
    hMaterial m_matPickupHealth;
    hMaterial m_matObstacle;

    // Lighting
    hLight m_hSun;
    hLight m_hFlashlight;
    hLight m_hCenterLight;
    
    // Night Mode State
    bool m_nightMode = false;
    bool m_flashlightOn = true;
    bool m_fPressed = false;

    // Cheats
    bool m_cheatInfiniteAmmo = false;
    bool m_cheatInfiniteHealth = false;
    bool m_cheatLeftPressed = false;
    bool m_cheatRightPressed = false;

    // Splash Screen
    bool m_showSplash = true;
    float m_splashTimer = 0.0f;

    // Game State
    struct Obstacle {
        Quark::Vec3 position;
        Quark::Vec3 size; // Half-size for AABB
        hMesh mesh;
        hMaterial material;
    };
    std::vector<Obstacle> m_obstacles;

    Player m_player;
    std::vector<Enemy> m_enemies;
    std::vector<Projectile> m_projectiles;
    std::vector<Pickup> m_pickups;
    
    float m_enemySpawnTimer = 0.0f;
    float m_score = 0.0f;
    bool m_gameOver = false;
    bool m_gameStarted = false;

    // Controls
    bool m_keys[256] = {};
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    bool m_firstMouse = true;
    bool m_mouseCaptured = false;
    
    // Settings
    float m_mouseSensitivity = 0.1f;
    bool m_paused = false;
    bool m_escPressed = false; // Debounce

    void resolveCollision(Quark::Vec3& pos, float radius) {
        for (const auto& o : m_obstacles) {
            Quark::Vec3 minB = o.position - o.size;
            Quark::Vec3 maxB = o.position + o.size;
            
            // Closest point on AABB to sphere center
            Quark::Vec3 closest;
            closest.x = std::max(minB.x, std::min(pos.x, maxB.x));
            closest.y = std::max(minB.y, std::min(pos.y, maxB.y));
            closest.z = std::max(minB.z, std::min(pos.z, maxB.z));
            
            Quark::Vec3 diff = pos - closest;
            float dist = diff.Length();
            
            if (dist < radius) {
                // Collision
                if (dist > 0.0001f) {
                    Quark::Vec3 n = diff.Normalized();
                    float pen = radius - dist;
                    pos = pos + n * pen;
                }
            }
        }
    }

public:
    BadSoldierGame(GameWindow* win, RenderSystemAPI* rs, RHI* rhi) 
        : m_window(win), m_rs(rs), m_rhi(rhi) {}

    void init() {
        // Init Resources (Meshes)
        m_capsuleMesh = m_rs->createMesh(CreateCapsuleMesh(0.5f, 1.8f), false);
        m_sphereMesh = m_rs->createMesh(CreateSphereMesh(0.2f), false);
        m_planeMesh = m_rs->createMesh(CreatePlaneMesh(50.0f), false);
        m_gunMesh = m_rs->createMesh(CreateGunMesh(), false);
        m_cubeMesh = m_rs->createMesh(CreateBoxMesh(Quark::Vec3(1,1,1)), false);
        
        // Init Materials
        MaterialData mat;
        mat.roughness = 0.5f;
        mat.cullMode = static_cast<UINT32>(CullMode::DOUBLE_SIDED);

        mat.albedo = Quark::Color(0.2f, 0.8f, 0.2f); // Green Player
        m_matPlayer = m_rs->createMaterial(mat);

        mat.albedo = Quark::Color(0.8f, 0.1f, 0.1f); // Red Enemy
        m_matEnemy = m_rs->createMaterial(mat);

        mat.albedo = Quark::Color(1.0f, 1.0f, 0.0f); // Yellow Bullet
        m_matBulletPlayer = m_rs->createMaterial(mat);

        mat.albedo = Quark::Color(1.0f, 0.0f, 1.0f); // Purple Enemy Bullet
        mat.emissive = Quark::Color(1.0f, 0.0f, 1.0f) * 2.0f;
        m_matBulletEnemy = m_rs->createMaterial(mat);

        mat.emissive = Quark::Color(0,0,0); // Reset emissive
        mat.albedo = Quark::Color(0.0f, 0.0f, 1.0f); // Blue Ammo Pickup
        m_matPickup = m_rs->createMaterial(mat);

        mat.albedo = Quark::Color(1.0f, 0.2f, 0.2f); // Red Health Pickup
        m_matPickupHealth = m_rs->createMaterial(mat);

        mat.albedo = Quark::Color(0.3f, 0.3f, 0.3f); // Gray Ground
        mat.emissive = Quark::Color::Black();
        m_matGround = m_rs->createMaterial(mat);
        
        mat.albedo = Quark::Color(0.0f, 0.8f, 0.8f); // Cyan Pickup
        m_matPickup = m_rs->createMaterial(mat);

        mat.albedo = Quark::Color(0.5f, 0.3f, 0.1f); // Brown Obstacle
        m_matObstacle = m_rs->createMaterial(mat);

        // Setup Light
        // Setup Light
        // 1. Sun (Directional)
        DirectionalLight dirLight;
        dirLight.direction = Quark::Vec3(-0.5f, -1.0f, -0.3f).Normalized();
        dirLight.intensity = 1.5f;
        dirLight.color = Quark::Color::White();
        dirLight.flags = (UINT32)LightFlags::LIGHT_ENABLED | (UINT32)LightFlags::LIGHT_CAST_SHADOWS;
        dirLight.shadowQuality = ShadowQuality::MEDIUM;
        m_hSun = m_rs->createDirectionalLight(dirLight);

        // 2. Flashlight (Spot) - Initially OFF (unless Night Mode starts ON)
        SpotLight spotLight;
        spotLight.position = Quark::Vec3(0,0,0);
        spotLight.direction = Quark::Vec3(0,0,1);
        spotLight.color = Quark::Color(1.0f, 0.9f, 0.8f);
        spotLight.intensity = 5.0f;
        spotLight.range = 30.0f;
        spotLight.innerCutoff = cos(Quark::Radians(15.0f));
        spotLight.outerCutoff = cos(Quark::Radians(25.0f));
        spotLight.flags = (UINT32)LightFlags::NONE; // Start OFF
        spotLight.shadowQuality = ShadowQuality::MEDIUM;
        m_hFlashlight = m_rs->createSpotLight(spotLight);

        // 3. Center Light (Point) - Initially OFF
        PointLight pointLight;
        pointLight.position = Quark::Vec3(0, 10.0f, 0); // High up in center
        pointLight.color = Quark::Color(0.8f, 0.8f, 1.0f); // Blueish
        pointLight.intensity = 3.0f;
        pointLight.range = 40.0f;
        pointLight.flags = (UINT32)LightFlags::NONE; // Start OFF
        pointLight.shadowQuality = ShadowQuality::MEDIUM;
        m_hCenterLight = m_rs->createPointLight(pointLight);

        // Camera
        m_camera.setPerspective(Quark::Radians(60.0f), (float)m_window->getWidth()/(float)m_window->getHeight(), 0.1f, 1000.0f);
        m_camera.lookAt(Quark::Vec3(0.0f, 1.0f, 0.0f));
        m_rs->setActiveCamera(&m_camera);

        resetGame();
    }

    void resetGame() {
        m_player.position = Quark::Vec3(0, 1, 0);
        m_player.hp = 100.0f;
        m_player.ammo = 50;
        m_player.shootCooldown = 0.0f;
        
        m_enemies.clear();
        m_projectiles.clear();
        m_pickups.clear();
        m_obstacles.clear();
        
        // Create Map (Barriers)
        // 4 Big Cubes
        auto addBlock = [&](Quark::Vec3 pos, Quark::Vec3 size) {
            Obstacle o;
            o.position = pos;
            o.size = size; // Half-Extents
            o.mesh = m_cubeMesh;
            o.material = m_matObstacle;
            m_obstacles.push_back(o);
        };
        
        // North (+Z) at 50.
        addBlock(Quark::Vec3(0, 1.0f, 51), Quark::Vec3(52, 7.0f, 1)); 
        // South (-Z) at -50
        addBlock(Quark::Vec3(0, 1.0f, -51), Quark::Vec3(52, 7.0f, 1));
        // East (+X) at 50
        addBlock(Quark::Vec3(51, 1.0f, 0), Quark::Vec3(1, 7.0f, 50));
        // West (-X) at -50
        addBlock(Quark::Vec3(-51, 1.0f, 0), Quark::Vec3(1, 7.0f, 50));
        
        // Random "Houses" / Blocks
        // Create a grid of possible positions
        for (int x = -40; x <= 40; x += 20) {
            for (int z = -40; z <= 40; z += 20) {
                if (abs(x) < 10 && abs(z) < 10) continue; // Clear center
                
                // Add a "House" (Large block)
                if (randomFloat(0,1) > 0.3f) {
                    // Lowered height range: 1.5 to 3.0 (Total 3m to 6m)
                    float h = randomFloat(2.0f, 3.0f);
                    float w = randomFloat(3, 6);
                    float d = randomFloat(3, 6);
                    addBlock(Quark::Vec3((float)x, h/2, (float)z), Quark::Vec3(w, h, d));
                }
            }
        }

        m_score = 0;
        m_gameOver = false;
        m_mouseCaptured = false;
        m_enemySpawnTimer = 0.0f;
    }

    void update(float dt) {
        if (m_showSplash) {
            m_splashTimer += dt;
            if (m_splashTimer > 3.5f) {
                m_showSplash = false;
            }
            return; 
        }

        if (!m_gameStarted) return;
        if (m_gameOver) {
            if (ImGui::IsKeyPressed(ImGuiKey_Enter)) resetGame();
            return;
        }

        handleInput(dt);
        if (m_paused) return; // Stop update logic
        
        // Spawn Enemies
        m_enemySpawnTimer += dt;
        if (m_enemySpawnTimer > 2.0f) { // Spawn every 2 seconds
            m_enemySpawnTimer = 0;
            spawnEnemy();
        }

        // Spawn Pickups (Randomly)
        if (randomFloat(0, 1000) < 5) { // Small chance
             // Check limit for Ammo (Type 0)
             int ammoCount = 0;
             for(const auto& p : m_pickups) if(p.active && p.type == 0) ammoCount++;

             if (ammoCount < 15) {
                 Pickup p;
                 p.position = Quark::Vec3(randomFloat(-40, 40), 0.5f, randomFloat(-40, 40));
                 p.active = true;
                 p.rotation = 0;
                 p.type = 0; // Ammo
                 p.mesh = m_sphereMesh; // Sphere
                 p.material = m_matPickup; // Blue
                 m_pickups.push_back(p);
             }
        }

        // Update Projectiles
        for (auto& p : m_projectiles) {
            if (!p.active) continue;
            p.position = p.position + p.velocity * dt;
            p.lifeTime -= dt;
            if (p.lifeTime <= 0) p.active = false;
            
            // Floor collision
            // Floor collision
            if (p.position.y < 0) p.active = false;
            
            // Obstacle Collision
            for (const auto& o : m_obstacles) {
                Quark::Vec3 minB = o.position - o.size;
                Quark::Vec3 maxB = o.position + o.size;
                if (p.position.x > minB.x && p.position.x < maxB.x &&
                    p.position.y > minB.y && p.position.y < maxB.y &&
                    p.position.z > minB.z && p.position.z < maxB.z) {
                    p.active = false;
                    break;
                }
            }
        }
        
        // Helper for Sphere-AABB resolution (Slide)
        auto resolveCollision = [&](Quark::Vec3& pos, float radius) {
            for (const auto& o : m_obstacles) {
                Quark::Vec3 minB = o.position - o.size;
                Quark::Vec3 maxB = o.position + o.size;
                
                // Closest point on AABB to sphere center
                Quark::Vec3 closest;
                closest.x = std::max(minB.x, std::min(pos.x, maxB.x));
                closest.y = std::max(minB.y, std::min(pos.y, maxB.y));
                closest.z = std::max(minB.z, std::min(pos.z, maxB.z));
                
                Quark::Vec3 diff = pos - closest;
                float dist = diff.Length();
                
                if (dist < radius) {
                    // Collision
                    if (dist > 0.0001f) {
                        Quark::Vec3 n = diff.Normalized();
                        float pen = radius - dist;
                        pos = pos + n * pen;
                    } else {
                        // Center is inside AABB? Push out X or Z
                         // Simple Push: Determine min axis overlap
                         // For now, just push up? No, slide.
                         // Simplest fallback: Just push back along -dir? No dir known here.
                         // Just ignore deep penetration for now or push Y.
                    }
                }
            }
        };

        // Resolve Player Collision
        resolveCollision(m_player.position, 0.5f); // 0.5 radius

        // Update Enemies
        for (auto& e : m_enemies) {
            if (!e.active) continue;
            
            // Calculate distance to player
            Quark::Vec3 diff = m_player.position - e.position;
            float distToPlayer = diff.Length();

            // Stealth Logic:
            // If Night Mode is ON and Flashlight is OFF, enemies have reduced vision range.
            if (m_nightMode && !m_flashlightOn) {
                 float darkVisionRange = 10.0f; 
                 if (distToPlayer > darkVisionRange) {
                     continue; // Player is hidden in the dark
                 }
            }

            // Move towards player
            Quark::Vec3 dirVec = diff;
            dirVec.y = 0; // Stay on ground
            float hDist = dirVec.Length();

            if (hDist > 0.1f) {
                Quark::Vec3 d = dirVec.Normalized();
                
                // Stop if within 3.0f units
                if (hDist > 3.0f) {
                    e.position = e.position + d * 3.0f * dt;
                    
                    // Obstacle Collision (Slide)
                    resolveCollision(e.position, 0.5f);
                }
                
                // Rotation (Always face player)
                e.rotation = atan2(d.x, d.z);
            }

            // Shoot at player
            e.shootTimer -= dt;
            if (e.shootTimer <= 0) {
                e.shootTimer = 2.0f;
                
                // Gun Offset (0.5, 0.2, 0.4) + Barrel Length Z (roughly 0.5 from origin)
                // Total local offset from enemy center to muzzle
                Quark::Vec3 localMuzzle(0.5f, 0.2f, 0.9f);
                
                float c = cos(e.rotation), s = sin(e.rotation);
                
                // Rotate offset (Corrected Rotation Matrix logic to match render)
                // Assuming standard [ c 0 s; 0 1 0; -s 0 c ] or similar.
                // If previous mismatch was sign, flipping s terms usually fixes it.
                // Trying: x' = xc + zs, z' = -xs + zc
                Quark::Vec3 rotMuzzle;
                rotMuzzle.x = localMuzzle.x * c + localMuzzle.z * s;
                rotMuzzle.y = localMuzzle.y;
                rotMuzzle.z = -localMuzzle.x * s + localMuzzle.z * c;
                
                Quark::Vec3 spawnPos = e.position + rotMuzzle;
                
                shoot(spawnPos, (m_player.position + Quark::Vec3(0,1,0) - spawnPos).Normalized(), true);
            }
        }

        // Update Camera (Post-Collision)
        Quark::Vec3 camFront;
        camFront.x = cos(Quark::Radians(m_yaw)) * cos(Quark::Radians(m_pitch));
        camFront.y = sin(Quark::Radians(m_pitch));
        camFront.z = sin(Quark::Radians(m_yaw)) * cos(Quark::Radians(m_pitch));
        camFront = camFront.Normalized();

        m_camera.setPosition(m_player.position + Quark::Vec3(0, 1.5f, 0)); // FPS view
        m_camera.lookAt(m_player.position + camFront * 10.0f, Quark::Vec3::Up());
        
        // --- LIGHTING UPDATE ---
        // 1. Toggle Flashlight
        if (ImGui::IsKeyPressed(ImGuiKey_F)) {
            m_flashlightOn = !m_flashlightOn;
        }

        // 2. Sun Logic
        if (m_nightMode) {
             // Disable Sun
             DirectionalLight sun;
             sun.flags = (UINT32)LightFlags::NONE;
             m_rs->updateLight(m_hSun, sun);

             // Enable/Disable Center Light
             PointLight center;
             center.position = Quark::Vec3(0, 10.0f, 0);
             center.color = Quark::Color(0.8f, 0.8f, 1.0f);
             center.intensity = 3.0f;
             center.range = 25.0f;
             center.flags = (UINT32)LightFlags::LIGHT_ENABLED;
             center.shadowQuality = ShadowQuality::MEDIUM;
             m_rs->updateLight(m_hCenterLight, center);

             // Enable/Disable Flashlight
             SpotLight flash;
             flash.position = m_camera.position;
             flash.direction = m_camera.forward();
             flash.color = Quark::Color(1.0f, 0.9f, 0.8f);
             flash.intensity = 20.0f;
             flash.range = 50.0f;
             flash.innerCutoff = cos(Quark::Radians(10.0f));
             flash.outerCutoff = cos(Quark::Radians(17.0f));
             flash.flags = m_flashlightOn ? (UINT32)LightFlags::LIGHT_ENABLED : (UINT32)LightFlags::NONE;
             flash.shadowQuality = ShadowQuality::MEDIUM;
             m_rs->updateLight(m_hFlashlight, flash);

             m_rs->setClearColor(0.05f, 0.05f, 0.05f, 1.0f); // Night Sky
             m_rs->setAmbientLight(Quark::Color(0.002f, 0.002f, 0.002f));

        } else {
             // Day Mode: Enable Sun, Disable Others
             DirectionalLight sun;
             sun.direction = Quark::Vec3(-0.5f, -1.0f, -0.3f).Normalized();
             sun.intensity = 1.5f;
             sun.color = Quark::Color::White();
             sun.flags = (UINT32)LightFlags::LIGHT_ENABLED | (UINT32)LightFlags::LIGHT_CAST_SHADOWS;
             sun.shadowQuality = ShadowQuality::MEDIUM;
             m_rs->updateLight(m_hSun, sun);

             PointLight center;
             center.flags = (UINT32)LightFlags::NONE;
             m_rs->updateLight(m_hCenterLight, center);

             SpotLight flash;
             flash.flags = (UINT32)LightFlags::NONE;
             m_rs->updateLight(m_hFlashlight, flash);

             m_rs->setClearColor(0.8f, 0.9f, 0.9f, 1.0f); // Day Sky
             m_rs->setAmbientLight(Quark::Color(0.2f, 0.2f, 0.2f));
        }
        
        // Collisions
        // 1. Projectiles vs Enemies/Player
        float hitRadius = 0.6f;
        for (auto& p : m_projectiles) {
            if (!p.active) continue;

            if (p.isEnemy) {
                // Fix Hit Reg: Check against center of player capsule (approx 0.9m height)
                Quark::Vec3 playerCenter = m_player.position + Quark::Vec3(0, 0.9f, 0);
                if ((p.position - playerCenter).Length() < hitRadius) {
                    if (!m_cheatInfiniteHealth) {
                        m_player.hp -= 10.0f;
                    }
                    p.active = false;
                    if (m_player.hp <= 0) m_gameOver = true;
                }
            } else {
                for (auto& e : m_enemies) {
                    if (!e.active) continue;
                    
                    // Body Hit
                    bool hit = false;
                    float dmg = 0;

                    // Check horizontal distance
                    float dist = (Quark::Vec3(p.position.x, 0, p.position.z) - Quark::Vec3(e.position.x, 0, e.position.z)).Length();
                    if (dist < 0.6f) { // Radius 0.5f + margin
                        float heightInfo = p.position.y - e.position.y;
                        // Capsule is centered at e.position with height 1.8 (range -0.9 to +0.9)
                        // Head is sphere radius 0.2 at +1.1 center (range +0.9 to +1.3)
                        // Total valid Y range: -0.9 to +1.4 (margin)
                        if (heightInfo > -0.9f && heightInfo < 1.4f) {
                             hit = true;
                             dmg = 25.0f; // Body shot
                             
                             // Head starts at 0.9. Give slight leniency (0.8)
                             if (heightInfo > 0.8f) {
                                 dmg = 50.0f; // Headshot
                             }
                        }
                    }

                    if (hit) {
                        e.hp -= dmg;
                        p.active = false;
                        if (e.hp <= 0) {
                            e.active = false;
                            m_score += (dmg > 25 ? 500 : 100);
                            
                            // Spawn Pickup (Health Only)
                            if (randomFloat(0,1) < 0.25f) { // 25% chance
                                // Check limit for Health (Type 1)
                                int healthCount = 0;
                                for(const auto& p : m_pickups) if(p.active && p.type == 1) healthCount++;
                                
                                if (healthCount < 5) {
                                    Pickup pick;
                                    pick.position = e.position;
                                    pick.active = true;
                                    pick.rotation = 0;
                                    
                                    // Always Health (Red Sphere)
                                    pick.type = 1;
                                    pick.mesh = m_sphereMesh; 
                                    pick.material = m_matPickupHealth;
                                    m_pickups.push_back(pick);
                                }
                            }
                        }
                    }
                }
            }
        }

        for (auto& pick : m_pickups) {
            if (!pick.active) continue;
            pick.rotation += dt * 2.0f;
            if ((pick.position - m_player.position).Length() < 1.0f) {
                if (pick.type == 0) {
                    m_player.ammo += 20;
                    // std::cout << "Picked up Ammo: " << m_player.ammo << std::endl;
                } else {
                    m_player.hp += 50;
                    if (m_player.hp > 100) m_player.hp = 100;
                    // std::cout << "Picked up Health: " << m_player.hp << std::endl;
                }
                pick.active = false;
            }
        }

        // Cleanup inactive
        m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), [](const Projectile& p){ return !p.active; }), m_projectiles.end());
        m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [](const Enemy& e){ return !e.active; }), m_enemies.end());
        m_pickups.erase(std::remove_if(m_pickups.begin(), m_pickups.end(), [](const Pickup& p){ return !p.active; }), m_pickups.end());
    }

    void handleInput(float dt) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
             if (!m_escPressed) {
                 m_escPressed = true;
                 m_paused = !m_paused;
                 if (m_paused) {
                     m_mouseCaptured = false;
                     while(ShowCursor(TRUE) < 0); // Force Show
                 } else {
                     m_mouseCaptured = true;
                     m_firstMouse = true;
                     while(ShowCursor(FALSE) >= 0); // Force Hide
                     
                     // Reset cursor to center to prevent jump
                     POINT centerPt = { (LONG)m_window->getWidth() / 2, (LONG)m_window->getHeight() / 2 };
                     ClientToScreen(m_window->getHandle(), &centerPt);
                     SetCursorPos(centerPt.x, centerPt.y);
                 }
             }
        } else {
             m_escPressed = false;
        }
        
        if (m_paused) return; // Don't process other inputs if paused

        // --- CHEAT INPUT ---
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
        if (ctrl) {
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
                if (!m_cheatLeftPressed) {
                     m_cheatInfiniteAmmo = !m_cheatInfiniteAmmo;
                     m_cheatLeftPressed = true;
                }
            } else {
                m_cheatLeftPressed = false;
            }

            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
                 if (!m_cheatRightPressed) {
                     m_cheatInfiniteHealth = !m_cheatInfiniteHealth;
                     m_cheatRightPressed = true;
                 }
            } else {
                m_cheatRightPressed = false;
            }
        } else {
            m_cheatLeftPressed = false;
            m_cheatRightPressed = false;
        }

        if (!m_mouseCaptured) {
            if (ImGui::IsMouseClicked(0) && !ImGui::GetIO().WantCaptureMouse) {
                 m_mouseCaptured = true;
                 m_firstMouse = true;
                 ShowCursor(FALSE);
            }
            return;
        }

        // Mouse Look with Center Locking
        POINT centerPt = { (LONG)m_window->getWidth() / 2, (LONG)m_window->getHeight() / 2 };
        ClientToScreen(m_window->getHandle(), &centerPt);

        POINT pt; GetCursorPos(&pt);
        
        if (m_firstMouse) {
            SetCursorPos(centerPt.x, centerPt.y);
            m_firstMouse = false;
            return; // Skip first frame to avoid large jump
        }

        float xoffset = (float)(pt.x - centerPt.x);
        float yoffset = (float)(centerPt.y - pt.y); // Y is reversed
        
        // Reset to center
        SetCursorPos(centerPt.x, centerPt.y);

        m_yaw += xoffset * m_mouseSensitivity; // User sensitivity
        m_pitch += yoffset * m_mouseSensitivity;

        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;

        // WASD
        Quark::Vec3 front;
        front.x = cos(Quark::Radians(m_yaw)) * cos(Quark::Radians(m_pitch));
        front.y = sin(Quark::Radians(m_pitch));
        front.z = sin(Quark::Radians(m_yaw)) * cos(Quark::Radians(m_pitch));
        front = front.Normalized();
        
        Quark::Vec3 right = front.Cross(Quark::Vec3::Up()).Normalized();

        float speed = 5.0f * dt;
        Quark::Vec3 moveDir(0,0,0);
        if (GetAsyncKeyState('W') & 0x8000) moveDir = moveDir + front;
        if (GetAsyncKeyState('S') & 0x8000) moveDir = moveDir - front;
        if (GetAsyncKeyState('A') & 0x8000) moveDir = moveDir - right;
        if (GetAsyncKeyState('D') & 0x8000) moveDir = moveDir + right;
        
        if (moveDir.Length() > 0.001f) {
            moveDir = moveDir.Normalized();
            m_player.position = m_player.position + moveDir * speed;
        }
        
        m_player.position.y = 1.0f; // Keep on ground

        // Shooting
        m_player.shootCooldown -= dt;
        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && m_player.shootCooldown <= 0 && (m_player.ammo > 0 || m_cheatInfiniteAmmo)) {
            
            // Raycast for accuracy
            Quark::Vec2 screenCenter((float)m_window->getWidth()/2, (float)m_window->getHeight()/2);
            Quark::Vec2 screenSize((float)m_window->getWidth(), (float)m_window->getHeight());
            Quark::Ray ray = m_camera.screenPointToRay(screenCenter, screenSize);
            
            // Aim at a point far away
            Quark::Vec3 target = ray.origin + ray.direction * 100.0f;
            
            // Spawn position: Below camera center
            // Camera is at player.pos + (0, 1.5, 0)
            Quark::Vec3 spawnPos = m_player.position + Quark::Vec3(0, 1.3f, 0); 

            shoot(spawnPos, (target - spawnPos).Normalized(), false);
            if (!m_cheatInfiniteAmmo) {
                m_player.ammo--;
            }
            m_player.shootCooldown = 0.15f;

            PlaySound(TEXT("gunpop.wav"), NULL, SND_FILENAME | SND_ASYNC);
        }

    }

    void shoot(Quark::Vec3 pos, Quark::Vec3 dir, bool isEnemy) {
        Projectile p;
        p.position = pos;
        p.velocity = dir * (isEnemy ? 30.0f : 60.0f); // Higher speed
        p.lifeTime = 3.0f;
        p.isEnemy = isEnemy;
        p.active = true;
        p.mesh = m_sphereMesh;
        p.material = isEnemy ? m_matBulletEnemy : m_matBulletPlayer;
        m_projectiles.push_back(p);
    }

    void spawnEnemy() {
        Enemy e;
        float angle = randomFloat(0, 6.28f);
        float dist = randomFloat(20, 30);
        e.position = Quark::Vec3(sin(angle)*dist, 1.0f, cos(angle)*dist);
        e.hp = 100;
        e.shootTimer = randomFloat(0, 2);
        e.active = true;
        e.meshBody = m_capsuleMesh;
        e.matBody = m_matEnemy;
        m_enemies.push_back(e);
    }

    void render() {
        RenderObjectFlags noCull = RenderObjectFlags::VISIBLE | RenderObjectFlags::CAST_SHADOW | RenderObjectFlags::RECEIVE_SHADOW;

        // Render Ground
        RenderObject ro;
        ro.mesh = m_planeMesh;
        ro.material = m_matGround;
        ro.worldMatrix = Quark::Mat4::Translation(Quark::Vec3(0,-0.1f,0));
        ro.flags = noCull; 
        m_rs->submit(ro);

        // Render Enemies
        for (const auto& e : m_enemies) {
            ro.mesh = e.meshBody;
            ro.material = e.matBody;
            ro.worldMatrix = Quark::Mat4::Translation(e.position) * Quark::Mat4::RotationY(e.rotation);
            ro.flags = noCull;
            m_rs->submit(ro);
            
            // Head
            ro.mesh = m_sphereMesh;
            ro.material = e.matBody;
            ro.worldMatrix = Quark::Mat4::Translation(e.position + Quark::Vec3(0, 1.8f * 0.5f + 0.2f, 0)); // Capsule half height + radius ? Capsule centered at y=0?
            // Capsule mesh created min -0.9, max 0.9. Center 0.
            // e.position is at ground (y=1.0 set in spawn?).
            // If e.position is center of feet/base, we need to adjust.
            // createCapsule: center is (0,0,0). So if e.position is feet, we should raise it 0.9.
            // but in spawnEnemy: e.position.y = 1.0f. Caps height 1.8 -> half 0.9.
            // So e.position is center of capsule.
            // Head should be at +0.9 (top) + 0.2 (radius) = +1.1.
            ro.worldMatrix = Quark::Mat4::Translation(e.position + Quark::Vec3(0, 1.1f, 0));
            ro.flags = noCull;
            m_rs->submit(ro);

            // Gun
            ro.mesh = m_gunMesh; 
            ro.material = m_matGround; // Gray
            
            // Matrix Composition: Parent(Body) * Local(Offset)
            // Body = T(pos) * R(rot). Local = T(offset).
            // Result = T(pos) * R(rot) * T(offset).
            ro.worldMatrix = Quark::Mat4::Translation(e.position) * Quark::Mat4::RotationY(e.rotation) * Quark::Mat4::Translation(Quark::Vec3(0.5f, 0.2f, 0.4f));
            ro.flags = noCull;
            m_rs->submit(ro);
        }

        // Render Projectiles
        for (const auto& p : m_projectiles) {
            ro.mesh = p.mesh;
            ro.material = p.material;
            ro.worldMatrix = Quark::Mat4::Translation(p.position) * Quark::Mat4::Scaling(Quark::Vec3(0.5f));
            ro.flags = noCull;
            m_rs->submit(ro);
        }

        // Render Pickups
        for (const auto& p : m_pickups) {
            ro.mesh = p.mesh;
            ro.material = p.material;
            ro.worldMatrix = Quark::Mat4::Translation(p.position) * Quark::Mat4::RotationY(Quark::Radians(p.rotation));
            ro.flags = noCull;
            m_rs->submit(ro);
        }

        // Render Obstacles
        for (const auto& o : m_obstacles) {
            ro.mesh = o.mesh;
            ro.material = o.material;
            ro.worldMatrix = Quark::Mat4::Translation(o.position) * Quark::Mat4::Scaling(o.size * 2.0f); // Scaling is full size, o.size is half-extent? Check logic.
            // CreateBoxMesh(1,1,1) -> Bounds -0.5 to 0.5. Size 1.
            // If o.size is half-extent (e.g., 2,2,2), full size is 4. So Scale(4).
            // o.size * 2.0f.
            ro.flags = noCull;
            m_rs->submit(ro);
        }
    }

    void renderUI() {
        // Start ImGui
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (m_showSplash) {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2((float)m_window->getWidth(), (float)m_window->getHeight()));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));
            ImGui::Begin("Splash", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
            
            float midX = (float)m_window->getWidth() / 2.0f;
            float midY = (float)m_window->getHeight() / 2.0f;

            ImGui::SetWindowFontScale(2.0f);
            std::string t1 = "Made with Quark Engine";
            float t1W = ImGui::CalcTextSize(t1.c_str()).x;
            ImGui::SetCursorPos(ImVec2(midX - t1W/2, midY - 30));
            ImGui::Text(t1.c_str());

            std::string t2 = "";
            float t2W = ImGui::CalcTextSize(t2.c_str()).x; // CalcTextSize doesn't account for FontScale usually, need to multiply?
            // Actually, scaling CalcTextSize results is safer.
            ImGui::SetCursorPos(ImVec2(midX - t2W, midY + 10)); // t2W * 1.0 roughly, center it manually
            ImGui::Text(t2.c_str());
            ImGui::SetWindowFontScale(1.0f);

            ImGui::End();
            ImGui::PopStyleColor();

        } else if (!m_gameStarted) {
            ImGui::SetNextWindowPos(ImVec2((float)m_window->getWidth()/2 - 100, (float)m_window->getHeight()/2 - 100));
            ImGui::SetNextWindowSize(ImVec2(200, 200));
            ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            ImGui::SetWindowFontScale(2.0f);
            ImGui::Text("BAD SOLDIER");
            ImGui::SetWindowFontScale(1.0f);
            
            ImGui::SliderFloat("Sensitivity", &m_mouseSensitivity, 0.05f, 1.5f);
            ImGui::Checkbox("Night Mode (Hard)", &m_nightMode);
            
            if (ImGui::Button("START GAME", ImVec2(180, 50))) {
                m_gameStarted = true;
                resetGame();
                
                // Force Capture
                m_mouseCaptured = true;
                m_firstMouse = true;
                m_paused = false;
                while(ShowCursor(FALSE) >= 0); // Force Hide
                
                POINT centerPt = { (LONG)m_window->getWidth() / 2, (LONG)m_window->getHeight() / 2 };
                ClientToScreen(m_window->getHandle(), &centerPt);
                SetCursorPos(centerPt.x, centerPt.y);
            }
            if (ImGui::Button("EXIT", ImVec2(180, 50))) {
                m_window->close();
            }
            ImGui::End();
        } else if (m_paused) {
            ImGui::SetNextWindowPos(ImVec2((float)m_window->getWidth()/2 - 100, (float)m_window->getHeight()/2 - 100));
            ImGui::SetNextWindowSize(ImVec2(200, 200));
            ImGui::Begin("Pause Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            ImGui::SetWindowFontScale(2.0f);
            ImGui::Text("PAUSED");
            ImGui::SetWindowFontScale(1.0f);
            
            if (ImGui::Button("RESUME", ImVec2(180, 50))) {
                m_paused = false;
                m_mouseCaptured = true;
                m_firstMouse = true;
                while(ShowCursor(FALSE) >= 0); // Force Hide
                POINT centerPt = { (LONG)m_window->getWidth() / 2, (LONG)m_window->getHeight() / 2 };
                ClientToScreen(m_window->getHandle(), &centerPt);
                SetCursorPos(centerPt.x, centerPt.y);
            }
            if (ImGui::Button("MAIN MENU", ImVec2(180, 50))) {
                m_gameStarted = false;
                m_paused = false;
                m_mouseCaptured = false;
                while(ShowCursor(TRUE) < 0); // Force Show
            }
            ImGui::End();
        } else if (m_gameOver) {
            ImGui::SetNextWindowPos(ImVec2((float)m_window->getWidth()/2 - 150, (float)m_window->getHeight()/2 - 50));
            ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            ImGui::TextColored(ImVec4(1,0,0,1), "GAME OVER");
            ImGui::Text("Score: %.0f", m_score);
            ImGui::Text("Press ENTER to Restart");
            ImGui::End();
        } else {
            // FPS Counter (Top Scale)
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowBgAlpha(0.3f);
            ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::End();

            // HUD (Bottom Left)
            ImGui::SetNextWindowPos(ImVec2(10, 50));
            ImGui::SetNextWindowBgAlpha(0.3f);
            ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
            
            if (m_cheatInfiniteHealth) ImGui::Text("HP: INF");
            else ImGui::Text("HP: %.0f", m_player.hp);
            
            if (m_cheatInfiniteAmmo) ImGui::Text("Ammo: INF");
            else ImGui::Text("Ammo: %d", m_player.ammo);
            
            ImGui::Text("Score: %.0f", m_score);
            ImGui::End();
            
            // Crosshair
            ImGui::GetForegroundDrawList()->AddCircle(ImVec2((float)m_window->getWidth()/2, (float)m_window->getHeight()/2), 4.0f, IM_COL32(0, 255, 0, 200));
        }

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
};

// ==================== MAIN ====================
int main() {
    GameWindow window;
    if (!window.create("Bad Soldier")) return -1;

    // Load DLLs
    HMODULE hRHI = LoadLibraryA("modules/rsd3d11.dll");
    HMODULE hSys = LoadLibraryA("modules/rendersystem.dll");

    if (!hRHI || !hSys) {
        MessageBoxA(nullptr, "Failed to load engine DLLs!", "Error", MB_ICONERROR);
        return 1;
    }

    CreateRenderBackendFn createRHI = (CreateRenderBackendFn)GetProcAddress(hRHI, "createRenderBackend");
    CreateRenderSystemFn createRS   = (CreateRenderSystemFn)GetProcAddress(hSys, "createRenderSystem");

    RHI* rhi = createRHI();
    RenderSystemAPI* rs = createRS();

    rs->init(rhi, (qWndh)window.getHandle());
    rs->setClearColor(0.8f, 0.9f, 0.9f, 1.0f);

    // Hook Resize
    window.setResizeCallback([&](UINT32 w, UINT32 h){
        rs->onResize(w, h);
    });
    // Force initial
    rs->onResize(window.getWidth(), window.getHeight());

    // Init ImGui Utils
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplDX11_Init((ID3D11Device*)rhi->getDevice(), (ID3D11DeviceContext*)rhi->getContext());

    BadSoldierGame game(&window, rs, rhi);
    game.init();

    // Loop
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (window.isRunning()) {
        window.pollMessages();
        
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        // No explicit beginFrame found in inspection, but it might be handled internally or not exposed.
        // Assuming renderFrame does the clear and setup based on common engine patterns if beginFrame doesn't exist.
        // But wait, user's devapp had rs->init().
        
        game.update(dt);
        
        // This part needs to match engine API. 
        // If rendersystemapi.h allows custom clear?
        // Checking rendersystemapi.h again... it has:
        // virtual void renderFrame() = 0;
        // virtual void endFrame() = 0;
        
        // renderFrame() probably clears and renders submitted objects.
        game.render(); // Submits objects
        
        rs->renderFrame(); // Renders scene
        
        game.renderUI();   // Renders ImGui on top
        
        rs->endFrame(); // Swaps buffers
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    // delete rs; // Factory destroy
    // delete rhi;
    return 0;
}
