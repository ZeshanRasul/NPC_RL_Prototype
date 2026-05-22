cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)  # Let MSVC_RUNTIME_LIBRARY target property control /MD vs /MDd
project(NPC_RL_Prototype LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# User-facing options
# ---------------------------------------------------------------------------
option(NPC_RL_QLEARNING "Enable Q-learning / reinforcement-learning system" OFF)
option(NPC_RL_TRACY     "Enable Tracy profiler (Profile builds)"             OFF)

# ---------------------------------------------------------------------------
# FMOD – default to the standard Windows installer path; override via cmake -D
# ---------------------------------------------------------------------------
set(FMOD_ROOT "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows"
    CACHE PATH "Root of the FMOD Studio API Windows installation")

# ---------------------------------------------------------------------------
# Game source files
# ---------------------------------------------------------------------------
set(GAME_SOURCES
    src/main.cpp
    src/App.cpp
    src/GameManager.cpp
    src/Camera.cpp
    src/InputManager.cpp
    src/Tunables.cpp
    src/Window/Window.cpp
    # OpenGL
    src/OpenGL/Renderer.cpp
    src/OpenGL/Shader.cpp
    src/OpenGL/Texture.cpp
    src/OpenGL/VertexBuffer.cpp
    src/OpenGL/UniformBuffer.cpp
    src/OpenGL/ShaderStorageBuffer.cpp
    src/OpenGL/Cubemap.cpp
    # GameObjects
    src/GameObjects/Player.cpp
    src/GameObjects/Enemy.cpp
    src/GameObjects/Ground.cpp
    src/GameObjects/Crosshair.cpp
    src/GameObjects/Line.cpp
    # Model / animation
    src/Model/GltfModel.cpp
    src/Model/GltfNode.cpp
    src/Model/GltfAnimationClip.cpp
    src/Model/GltfAnimationChannel.cpp
    # Components
    src/Components/Component.cpp
    src/Components/AudioComponent.cpp
    # Physics
    src/Physics/AABB.cpp
    src/Physics/PhysicsWorld.cpp
    # Audio
    src/Audio/AudioSystem.cpp
    src/Audio/AudioManager.cpp
    src/Audio/SoundEvent.cpp
    # Tools
    src/Tools/Logger.cpp
    # C sources
    src/glad.c
    # Third-party (compiled inline)
    src/tinygltf/tiny_gltf.cc
)

if(NPC_RL_TRACY)
    list(APPEND GAME_SOURCES src/tracy/TracyClient.cpp)
endif()

# ---------------------------------------------------------------------------
# Third-party sources
# ---------------------------------------------------------------------------
set(IMGUI_SOURCES
    include/imgui/imgui.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/backend/imgui_impl_glfw.cpp
    include/imgui/backend/imgui_impl_opengl3.cpp
)

set(RECAST_SOURCES
    include/Recast/Source/Recast.cpp
    include/Recast/Source/RecastAlloc.cpp
    include/Recast/Source/RecastArea.cpp
    include/Recast/Source/RecastAssert.cpp
    include/Recast/Source/RecastContour.cpp
    include/Recast/Source/RecastFilter.cpp
    include/Recast/Source/RecastLayers.cpp
    include/Recast/Source/RecastMesh.cpp
    include/Recast/Source/RecastMeshDetail.cpp
    include/Recast/Source/RecastRasterization.cpp
    include/Recast/Source/RecastRegion.cpp
)

set(DETOUR_SOURCES
    include/Detour/Source/DetourAlloc.cpp
    include/Detour/Source/DetourAssert.cpp
    include/Detour/Source/DetourCommon.cpp
    include/Detour/Source/DetourNavMesh.cpp
    include/Detour/Source/DetourNavMeshBuilder.cpp
    include/Detour/Source/DetourNavMeshQuery.cpp
    include/Detour/Source/DetourNode.cpp
)

set(DETOURCROWD_SOURCES
    include/DetourCrowd/Source/DetourCrowd.cpp
    include/DetourCrowd/Source/DetourLocalBoundary.cpp
    include/DetourCrowd/Source/DetourObstacleAvoidance.cpp
    include/DetourCrowd/Source/DetourPathCorridor.cpp
    include/DetourCrowd/Source/DetourPathQueue.cpp
    include/DetourCrowd/Source/DetourProximityGrid.cpp
)

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
add_executable(NPC_RL_Prototype
    ${GAME_SOURCES}
    ${IMGUI_SOURCES}
    ${RECAST_SOURCES}
    ${DETOUR_SOURCES}
    ${DETOURCROWD_SOURCES}
)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(NPC_RL_Prototype PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/src/tinygltf
    ${CMAKE_SOURCE_DIR}/src/OpenGL
    ${CMAKE_SOURCE_DIR}/src/Tools
    ${CMAKE_SOURCE_DIR}/include/Recast/Include
    ${CMAKE_SOURCE_DIR}/include/Detour/Include
    ${CMAKE_SOURCE_DIR}/include/DetourCrowd/Include
    "${FMOD_ROOT}/api/studio/inc"
    "${FMOD_ROOT}/api/lowlevel/inc"
    "${FMOD_ROOT}/api/core/inc"
)

if(NPC_RL_TRACY)
    target_include_directories(NPC_RL_Prototype PRIVATE ${CMAKE_SOURCE_DIR}/src/tracy)
endif()

# ---------------------------------------------------------------------------
# Preprocessor definitions
# ---------------------------------------------------------------------------
target_compile_definitions(NPC_RL_Prototype PRIVATE
    _CRT_SECURE_NO_WARNINGS
    GLM_ENABLE_EXPERIMENTAL
    UNICODE
    _UNICODE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Profile>:NDEBUG>
)

if(NPC_RL_QLEARNING)
    target_compile_definitions(NPC_RL_Prototype PRIVATE NPC_RL_QLEARNING)
endif()

if(NPC_RL_TRACY)
    target_compile_definitions(NPC_RL_Prototype PRIVATE TRACY_ENABLE)
endif()

# ---------------------------------------------------------------------------
# FMOD imported targets (separate debug/release variants)
# ---------------------------------------------------------------------------
add_library(fmod_core STATIC IMPORTED)
set_target_properties(fmod_core PROPERTIES
    IMPORTED_LOCATION_DEBUG          "${FMOD_ROOT}/api/lowlevel/lib/fmodL64_vc.lib"
    IMPORTED_LOCATION_RELEASE        "${FMOD_ROOT}/api/lowlevel/lib/fmod64_vc.lib"
    IMPORTED_LOCATION_PROFILE        "${FMOD_ROOT}/api/lowlevel/lib/fmod64_vc.lib"
    IMPORTED_LOCATION_MINSIZEREL     "${FMOD_ROOT}/api/lowlevel/lib/fmod64_vc.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${FMOD_ROOT}/api/lowlevel/lib/fmod64_vc.lib"
)

add_library(fmod_studio STATIC IMPORTED)
set_target_properties(fmod_studio PROPERTIES
    IMPORTED_LOCATION_DEBUG          "${FMOD_ROOT}/api/studio/lib/fmodstudioL64_vc.lib"
    IMPORTED_LOCATION_RELEASE        "${FMOD_ROOT}/api/studio/lib/fmodstudio64_vc.lib"
    IMPORTED_LOCATION_PROFILE        "${FMOD_ROOT}/api/studio/lib/fmodstudio64_vc.lib"
    IMPORTED_LOCATION_MINSIZEREL     "${FMOD_ROOT}/api/studio/lib/fmodstudio64_vc.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${FMOD_ROOT}/api/studio/lib/fmodstudio64_vc.lib"
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(NPC_RL_Prototype PRIVATE
    opengl32
    "${CMAKE_SOURCE_DIR}/lib/glfw3.lib"
    fmod_core
    fmod_studio
)

# ---------------------------------------------------------------------------
# Compiler / linker options (MSVC)
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(NPC_RL_Prototype PRIVATE
        /W3
        /MP                  # parallel compilation
        /permissive-         # standards conformance
        $<$<CONFIG:Debug>:/Zi /sdl>
        $<$<CONFIG:Release>:/O2 /Oi /Gy>
        $<$<CONFIG:Profile>:/O2 /Oi /Gy>
    )
    target_link_options(NPC_RL_Prototype PRIVATE
        /DEBUG:FULL
        $<$<NOT:$<CONFIG:Debug>>:/OPT:NOREF /OPT:NOICF>
    )
endif()

# ---------------------------------------------------------------------------
# Working directory — all asset/shader paths are relative to the project root,
# so point both the VS debugger and standalone launches there.
# ---------------------------------------------------------------------------
set_target_properties(NPC_RL_Prototype PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# ---------------------------------------------------------------------------
# FMOD runtime DLLs — copy the correct variant next to the exe after each build
# so the exe can find them whether launched from VS or the file system.
# ---------------------------------------------------------------------------
set(FMOD_DLL_CORE_DEBUG    "${FMOD_ROOT}/api/lowlevel/lib/fmodL64.dll")
set(FMOD_DLL_STUDIO_DEBUG  "${FMOD_ROOT}/api/studio/lib/fmodstudioL64.dll")
set(FMOD_DLL_CORE_RELEASE  "${FMOD_ROOT}/api/lowlevel/lib/fmod64.dll")
set(FMOD_DLL_STUDIO_RELEASE "${FMOD_ROOT}/api/studio/lib/fmodstudio64.dll")

add_custom_command(TARGET NPC_RL_Prototype POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${FMOD_DLL_CORE_DEBUG},${FMOD_DLL_CORE_RELEASE}>"
        "$<TARGET_FILE_DIR:NPC_RL_Prototype>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${FMOD_DLL_STUDIO_DEBUG},${FMOD_DLL_STUDIO_RELEASE}>"
        "$<TARGET_FILE_DIR:NPC_RL_Prototype>"
    COMMENT "Copying FMOD DLLs to output directory"
)

# ---------------------------------------------------------------------------
# IDE source grouping (keeps VS Solution Explorer tidy)
# ---------------------------------------------------------------------------
source_group(TREE "${CMAKE_SOURCE_DIR}/src"     PREFIX "Source"     FILES ${GAME_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}/include" PREFIX "ThirdParty" FILES
    ${IMGUI_SOURCES} ${RECAST_SOURCES} ${DETOUR_SOURCES} ${DETOURCROWD_SOURCES})
