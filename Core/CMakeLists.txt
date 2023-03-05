# ------------- Core.Static -------------
set(SRC_STATIC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/stdafx.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/stdafx.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/Logger.h" "${CMAKE_CURRENT_SOURCE_DIR}/Include/GLTFReader.h"
)

add_library(Core.Static STATIC ${SRC_STATIC_FILES})

SET(GLTF_PATH "${CMAKE_SOURCE_DIR}/Externals/glTF-SDK/GLTFSDK")

target_include_directories(Core.Static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${GLTF_PATH}/Inc")
target_link_libraries(Core.Static PUBLIC GLTFSDK)

# ------------- Core -------------
set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Application.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Application.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/ApplicationHelper.h"
)

add_library(Core STATIC ${SRC_FILES})

target_include_directories(Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${GLTF_PATH}/Inc")
target_link_libraries(Core PUBLIC Core.Static Renderer Scene)
