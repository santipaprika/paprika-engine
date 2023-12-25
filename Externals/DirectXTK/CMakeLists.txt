set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/SimpleMath.h" "${CMAKE_CURRENT_SOURCE_DIR}/Include/SimpleMath.inl" "${CMAKE_CURRENT_SOURCE_DIR}/Src/SimpleMath.cpp"
)

add_library(DirectXTK_SimpleMath STATIC ${SRC_FILES})
target_include_directories(DirectXTK_SimpleMath PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include")
