set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/SimpleMath.h" "${CMAKE_CURRENT_SOURCE_DIR}/SimpleMath.inl" "${CMAKE_CURRENT_SOURCE_DIR}/SimpleMath.cpp"
)

add_library(DirectXTK_SimpleMath STATIC ${SRC_FILES})
target_include_directories(DirectXTK_SimpleMath PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/SimpleMath.h")
