set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Scene.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Scene.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/Transform.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Transform.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/Transform.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Transform.h"

        # Entity classes
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/Entities/MeshEntity.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Entities/MeshEntity.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/Entities/CameraEntity.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Entities/CameraEntity.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Src/Entities/LightEntity.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Entities/LightEntity.h"
)

add_library(Scene STATIC ${SRC_FILES})

SET(GLTF_PATH "${CMAKE_SOURCE_DIR}/Externals/glTF-SDK/GLTFSDK")

target_include_directories(Scene PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${GLTF_PATH}/Inc")
target_link_libraries(Scene PUBLIC Core.Static Renderer)