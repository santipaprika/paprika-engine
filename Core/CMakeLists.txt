# ------------- Core.Static -------------
set(SRC_STATIC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/stdafx.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/stdafx.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/TransformComponent.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/TransformComponent.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/Logger.h" 
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/GLTFReader.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/GLTFReader.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/Timer.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/InputController.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/InputController.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/TransformUtils.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/TransformUtils.h"
)

add_library(Core.Static STATIC ${SRC_STATIC_FILES})

SET(AGILITY_PATH "${CMAKE_SOURCE_DIR}/Externals/AgilitySDK")

target_include_directories(Core.Static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${AGILITY_PATH}/include")
target_link_libraries(Core.Static PUBLIC GLTFSDK DirectXTK_SimpleMath DirectXTex)

if(MSVC)
 target_compile_options(Core.Static PUBLIC "/MP")
endif()

target_compile_definitions(Core.Static PUBLIC NOMINMAX)
target_compile_definitions(Core.Static
    PUBLIC
      $<$<CONFIG:Debug>:PPK_DEBUG>
      $<$<CONFIG:RelWithDebInfo>:PPK_DEBUG>
      $<$<CONFIG:Release>:PPK_RELEASE>
      $<$<CONFIG:MinSizeRel>:PPK_RELEASE>

      # Warning level log defines
      $<$<CONFIG:Debug>:PPK_BREAK_ON_WARNING>
      $<$<CONFIG:Debug>:PPK_LOG_INFO>
      PPK_LOG_WARNING
      PPK_LOG_ERROR
      
      # gpu debugging/profiling defines
      $<$<CONFIG:Debug>:PPK_DEBUG_GPU_RESOURCES>
      $<$<CONFIG:Debug>:PPK_DEBUG_SHADERS>
      $<$<CONFIG:RelWithDebInfo>:PPK_DEBUG_GPU_RESOURCES>
      $<$<CONFIG:RelWithDebInfo>:PPK_DEBUG_SHADERS>
      $<$<CONFIG:RelWithDebInfo>:PPK_PROFILE_SHADERS>
      $<$<CONFIG:RelWithDebInfo>:PROFILE_BUILD>
      
      # gpu validation layer defines
      $<$<CONFIG:Debug>:PPK_D3D_DEBUG_LAYER>
      
      # disable pix on release
      $<$<CONFIG:Debug>:PPK_USE_PIX>
      $<$<CONFIG:RelWithDebInfo>:PPK_USE_PIX>
)

# ------------- Core -------------
set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Application.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Application.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/ComponentManager.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/ComponentManager.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/ApplicationHelper.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/ImGuiHelper.h"
)

add_library(Core STATIC ${SRC_FILES})

target_include_directories(Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include")
target_link_libraries(Core PUBLIC Core.Static Renderer Scene ImGui)
