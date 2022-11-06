set(SRC_FILES 
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/main.cpp"
)

# Make an automatic library - will be static or dynamic based on user setting
add_executable(Launcher WIN32 ${SRC_FILES})

# We need this directory, and users of our library will need it too
#target_include_directories(Launcher PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include")

target_link_libraries(Launcher PUBLIC Core)
