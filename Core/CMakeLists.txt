# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
# file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ModernCMakeExample_SOURCE_DIR}/include/modern/*.hpp")
set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Application.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Application.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/ApplicationHelper.h"

)

set(SRC_STATIC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/stdafx.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/stdafx.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/Logger.h" "${CMAKE_CURRENT_SOURCE_DIR}/Include/StreamReader.h"
)

# Make an automatic library - will be static or dynamic based on user setting
add_library(Core STATIC ${SRC_FILES})
add_library(Core.Static STATIC ${SRC_STATIC_FILES})

SET(GLTF_PATH "${CMAKE_SOURCE_DIR}/Externals/glTF-SDK/GLTFSDK")

# We need this directory, and users of our library will need it too
target_include_directories(Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${GLTF_PATH}/Inc")
target_include_directories(Core.Static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${GLTF_PATH}/Inc")
target_link_libraries(Core.Static PUBLIC GLTFSDK)
target_link_libraries(Core PUBLIC Core.Static Renderer)
