# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
# file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ModernCMakeExample_SOURCE_DIR}/include/modern/*.hpp")
set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/stdafx.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/stdafx.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Application.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Application.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/ApplicationHelper.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/Logger.h")

# Make an automatic library - will be static or dynamic based on user setting
add_library(Core STATIC ${SRC_FILES})

# Set Pix path
SET(PIX_PATH "${CMAKE_SOURCE_DIR}/Externals/WinPixEventRuntime")

# We need this directory, and users of our library will need it too
target_include_directories(Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${PIX_PATH}/Include")
target_link_libraries(Core PUBLIC Renderer d3d12.lib dxgi.lib d3dcompiler.lib "${PIX_PATH}/WinPixEventRuntime.lib")

# IDEs should put the headers in a nice place
#source_group(
#  TREE "${PROJECT_SOURCE_DIR}/include"
#  PREFIX "Header Files"
#  FILES ${HEADER_LIST})
