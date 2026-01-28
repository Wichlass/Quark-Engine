cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("quark_engine")

add_executable(quark_engine
    application/entrypoint.cpp
)

set_target_properties(quark_engine
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/win64"
)

target_include_directories(quark_engine PRIVATE
    modules
    editor
    thirdparty
)

target_link_libraries(quark_engine PRIVATE d3d11)

set_target_properties(quark_engine PROPERTIES 
    OUTPUT_NAME "Quark Editor"
)

# module engine.dll
add_library(engine SHARED
    modules/core/engine/engine.cpp
    modules/core/engine/modulemanager/modulemanager.cpp
    modules/core/engine/window/window.cpp
)

target_include_directories(engine PRIVATE
    modules
)

target_compile_definitions(engine PRIVATE
    ENGINE_EXPORTS
)

set_target_properties(engine
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/win64/modules"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/win64/modules"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/win64/modules"
)

# module rendersystem.dll
add_library(rendersystem SHARED
    modules/graphics/rendersystem/rendersystem.cpp
)

target_include_directories(rendersystem PRIVATE
    modules
)

target_compile_definitions(rendersystem PRIVATE
    RENDERSYSTEM_EXPORTS
)

set_target_properties(rendersystem
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/win64/modules"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/win64/modules"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/win64/modules"
)

# render system submodule rsd3d11.dll
add_library(rsd3d11 SHARED
    modules/graphics/rendersystem/backends/d3d11/rsd3d11.cpp
    modules/graphics/rendersystem/backends/d3d11/rsd3d11_device.cpp
    modules/graphics/rendersystem/backends/d3d11/rsd3d11_memory.cpp
    modules/graphics/rendersystem/backends/d3d11/rsd3d11_pipeline.cpp
    modules/graphics/rendersystem/backends/d3d11/rsd3d11_shaders.cpp
)

target_include_directories(rsd3d11 PRIVATE
    modules
)

target_compile_definitions(rsd3d11 PRIVATE
    RSD3D11_EXPORTS
)

set_target_properties(rsd3d11
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/win64/modules"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/win64/modules"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/win64/modules"
)

# devapp - Main Development Application
add_executable(devapp
    modules/graphics/devapp/devapp.cpp
    modules/tools/modelloader.cpp
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/backends/imgui_impl_win32.cpp
    thirdparty/imgui/backends/imgui_impl_dx11.cpp
    thirdparty/imguizmo/ImGuizmo.cpp
)

target_include_directories(devapp PRIVATE
    modules
    thirdparty
    thirdparty/imgui
    thirdparty/imguizmo
    ${CMAKE_SOURCE_DIR}/thirdparty/assimp/include
    ${CMAKE_BINARY_DIR}/thirdparty/assimp/include
    ${CMAKE_SOURCE_DIR}/thirdparty/assimp/build/include
)

# Link assimp library
find_library(ASSIMP_LIBRARY 
    NAMES assimp assimp-vc143-mt
    PATHS ${CMAKE_BINARY_DIR}/thirdparty/assimp/lib
          ${CMAKE_SOURCE_DIR}/thirdparty/assimp/build/lib/Release
)
if(ASSIMP_LIBRARY)
    target_link_libraries(devapp PRIVATE ${ASSIMP_LIBRARY})
else()
    message(WARNING "Assimp library not found, trying subdirectory...")
    add_subdirectory(thirdparty/assimp)
    target_link_libraries(devapp PRIVATE assimp)
endif()

set_target_properties(devapp
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/win64"
        OUTPUT_NAME "devapp"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET quark_engine PROPERTY CXX_STANDARD 20)
    set_property(TARGET engine PROPERTY CXX_STANDARD 20)
    set_property(TARGET rendersystem PROPERTY CXX_STANDARD 20)
    set_property(TARGET rsd3d11 PROPERTY CXX_STANDARD 20)
    set_property(TARGET devapp PROPERTY CXX_STANDARD 20)
endif()

# Bad Soldier Minigame
add_executable(badsoldier
    minigames/badsoldier.cpp
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/backends/imgui_impl_win32.cpp
    thirdparty/imgui/backends/imgui_impl_dx11.cpp
)

target_include_directories(badsoldier PRIVATE
    modules
    thirdparty
    thirdparty/imgui
)

target_link_libraries(badsoldier PRIVATE
    d3d11
    winmm
)

set_target_properties(badsoldier
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/win64/minigames"
        OUTPUT_NAME "badsoldier"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET badsoldier PROPERTY CXX_STANDARD 20)
endif()
