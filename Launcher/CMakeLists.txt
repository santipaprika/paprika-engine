set(SRC_FILES 
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/main.cpp"
)

# Make an automatic library - will be static or dynamic based on user setting
add_executable(Launcher WIN32 ${SRC_FILES})

# We need this directory, and users of our library will need it too
#target_include_directories(Launcher PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include")

target_link_libraries(Launcher PUBLIC Core Core.Static Renderer Scene)

# Set Pix path
SET(PIX_PATH "${CMAKE_SOURCE_DIR}/Externals/WinPixEventRuntime")

# Copy Pix DLL next to executable
add_custom_command(TARGET Launcher POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PIX_PATH}/WinPixEventRuntime.dll"
    $<TARGET_FILE_DIR:Launcher>
)

# Set AgilitySDK path
SET(AGILITY_PATH "${CMAKE_SOURCE_DIR}/Externals/AgilitySDK")

# Copy AgilitySDK next to executable
add_custom_command(TARGET Launcher PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${AGILITY_PATH}/D3D12"
    $<TARGET_FILE_DIR:Launcher>/D3D12
)

# Set DXC path
set(DXC_ROOT "${CMAKE_SOURCE_DIR}/Externals/DirectXShaderCompiler")

# Copy DXC next to executable
add_custom_command(TARGET Launcher POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DXC_ROOT}/bin/x64/dxcompiler.dll"
    $<TARGET_FILE_DIR:Launcher>
)