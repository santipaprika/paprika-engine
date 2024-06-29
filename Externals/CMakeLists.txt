set(ENABLE_UNIT_TESTS OFF)
set(ENABLE_SAMPLES OFF)

include(FetchContent)
FetchContent_Declare(
  glTF-SDK
  GIT_REPOSITORY https://github.com/microsoft/glTF-SDK.git
  GIT_TAG        382dea2c80737f9f3ae2b93e97263a593c2ea832 # r1.9.6.0
)
FetchContent_MakeAvailable(glTF-SDK)

FetchContent_Declare(
  DirectXTex
  GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git
  GIT_TAG        419e31491496d7af58ab862b50558668704abebb # Sep 2023
)
FetchContent_MakeAvailable(DirectXTex)

# add_subdirectory(DirectXTex)
add_subdirectory(DirectXTK)

set_property(TARGET
    GLTFSDK
	PROPERTY FOLDER "Externals"
)

set_property(TARGET
    DirectXTK_SimpleMath
	PROPERTY FOLDER "Externals"
)

set_property(TARGET
    DirectXTex
	PROPERTY FOLDER "Externals"
)

# IMGUI

file(GLOB IMGUI_ROOT_SOURCE CONFIGURE_DEPENDS "imgui/*.cpp" "imgui/*.h")
SET(IMGUI_BACKEND "imgui/backends/imgui_impl_dx12.h" "imgui/backends/imgui_impl_dx12.cpp" 
				  "imgui/backends/imgui_impl_win32.h" "imgui/backends/imgui_impl_win32.cpp"
)
SET(IMGUI_MISC "imgui/misc/cpp/imgui_stdlib.h" "imgui/misc/cpp/imgui_stdlib.cpp")
add_library(ImGui STATIC ${IMGUI_ROOT_SOURCE} ${IMGUI_BACKEND} ${IMGUI_MISC})
target_include_directories(ImGui PUBLIC "imgui")

set_property(TARGET
    ImGui
	PROPERTY FOLDER "Externals"
)