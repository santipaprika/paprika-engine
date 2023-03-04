set(ENABLE_UNIT_TESTS OFF)
set(ENABLE_SAMPLES OFF)

include(FetchContent)
FetchContent_Declare(
  glTF-SDK
  GIT_REPOSITORY https://github.com/microsoft/glTF-SDK.git
  GIT_TAG        382dea2c80737f9f3ae2b93e97263a593c2ea832 # r1.9.6.0
)
FetchContent_MakeAvailable(glTF-SDK)

set_property(TARGET
    GLTFSDK
	PROPERTY FOLDER "Externals"
)