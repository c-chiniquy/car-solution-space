cmake_minimum_required(VERSION 3.22)

set(name "car-solution-space")
project(${name} LANGUAGES CXX)

# requires C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# project
set(PROJECT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/Car.h
    ${CMAKE_SOURCE_DIR}/src/Car.cpp
    ${CMAKE_SOURCE_DIR}/src/Constants.h
    ${CMAKE_SOURCE_DIR}/src/FitnessTexture.h
    ${CMAKE_SOURCE_DIR}/src/FitnessTexture.cpp
    ${CMAKE_SOURCE_DIR}/src/GeneInterpreter.h
    ${CMAKE_SOURCE_DIR}/src/GeneInterpreter.cpp
    ${CMAKE_SOURCE_DIR}/src/GeneticAlgorithm.h
    ${CMAKE_SOURCE_DIR}/src/GeneticAlgorithm.cpp
    ${CMAKE_SOURCE_DIR}/src/Main.h
    ${CMAKE_SOURCE_DIR}/src/Main.cpp
    ${CMAKE_SOURCE_DIR}/src/Settings.h
    ${CMAKE_SOURCE_DIR}/src/Settings.cpp
    ${CMAKE_SOURCE_DIR}/src/Terrain.h
    ${CMAKE_SOURCE_DIR}/src/Terrain.cpp
)

# iglo
set(IGLO_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extern/iglo-0.2.0")
set(IGLO_SOURCES
    ${IGLO_INCLUDE_DIR}/iglo.h
    ${IGLO_INCLUDE_DIR}/iglo.cpp
    ${IGLO_INCLUDE_DIR}/igloBatchRenderer.h
    ${IGLO_INCLUDE_DIR}/igloBatchRenderer.cpp
    ${IGLO_INCLUDE_DIR}/igloCamera.h
    ${IGLO_INCLUDE_DIR}/igloCamera.cpp
    ${IGLO_INCLUDE_DIR}/igloFont.h
    ${IGLO_INCLUDE_DIR}/igloFont.cpp
    ${IGLO_INCLUDE_DIR}/igloMainLoop.h
    ${IGLO_INCLUDE_DIR}/igloMainLoop.cpp
    ${IGLO_INCLUDE_DIR}/igloUtility.h
    ${IGLO_INCLUDE_DIR}/igloUtility.cpp
    ${IGLO_INCLUDE_DIR}/igloConfig.h
)

# imgui
set(IMGUI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extern/imgui-1.90.4")
set(IMGUI_SOURCES
    ${IMGUI_INCLUDE_DIR}/imconfig.h
    ${IMGUI_INCLUDE_DIR}/imgui.cpp
    ${IMGUI_INCLUDE_DIR}/imgui.h
    ${IMGUI_INCLUDE_DIR}/imgui_demo.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_draw.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_internal.h
    ${IMGUI_INCLUDE_DIR}/imgui_tables.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_widgets.cpp
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_win32.h
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_dx12.h
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_dx12.cpp
)

# implot
set(IMPLOT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extern/implot")
set(IMPLOT_SOURCES
    ${IMPLOT_INCLUDE_DIR}/implot.h
    ${IMPLOT_INCLUDE_DIR}/implot.cpp
    ${IMPLOT_INCLUDE_DIR}/implot_items.cpp
    ${IMPLOT_INCLUDE_DIR}/implot_demo.cpp
    ${IMPLOT_INCLUDE_DIR}/implot_internal.h
)

# ini parser
set(INIPARSER_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extern/ini-parser")

# box2d
set(BOX2D_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extern/box2d-2.4.1/include")
set(BOX2D_DEBUG_LIB "${CMAKE_SOURCE_DIR}/extern/box2d-2.4.1/bin/Debug/box2d.lib")
set(BOX2D_RELEASE_LIB "${CMAKE_SOURCE_DIR}/extern/box2d-2.4.1/bin/Release/box2d.lib")

# set Agility SDK version
set(AGILITY_SDK_VERSION "1.715.1-preview")

# set download URL for Agility SDK
set(AGILITY_SDK_URL "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/${AGILITY_SDK_VERSION}")

# set the path where the SDK will be downloaded and extracted
set(AGILITY_SDK_PACKAGE_DIR "${CMAKE_BINARY_DIR}/packages/Microsoft.Direct3D.D3D12.${AGILITY_SDK_VERSION}")
set(AGILITY_SDK_NUPKG_FILE "${CMAKE_BINARY_DIR}/Microsoft.Direct3D.D3D12.${AGILITY_SDK_VERSION}.nupkg")

# download the .nupkg file if it doesn't exist
if(NOT EXISTS "${AGILITY_SDK_NUPKG_FILE}")
    message(STATUS "Downloading Agility SDK from NuGet...")
    file(DOWNLOAD
        ${AGILITY_SDK_URL}
        ${AGILITY_SDK_NUPKG_FILE}
        TLS_VERIFY ON
        STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download Agility SDK. Check your internet connection.")
    endif()
    message(STATUS "Successfully downloaded Agility SDK.")
endif()

# extract the .nupkg file if it hasn't been extracted yet
if(NOT EXISTS "${AGILITY_SDK_PACKAGE_DIR}")
    message(STATUS "Extracting Agility SDK...")
    file(MAKE_DIRECTORY ${AGILITY_SDK_PACKAGE_DIR})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${AGILITY_SDK_NUPKG_FILE}
        WORKING_DIRECTORY ${AGILITY_SDK_PACKAGE_DIR}
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to extract Agility SDK.")
    endif()
    message(STATUS "Successfully extracted Agility SDK.")
endif()

# set path to the extracted SDK files
set(AGILITY_SDK_INCLUDE_DIR "${AGILITY_SDK_PACKAGE_DIR}/build/native/include")
set(AGILITY_SDK_DLL64_DIR "${AGILITY_SDK_PACKAGE_DIR}/build/native/bin/x64")

# Agility SDK files that will be copied to D3D12 folder
set(AGILITY_SDK_DLLS
    "${AGILITY_SDK_DLL64_DIR}/D3D12Core.dll"
    "${AGILITY_SDK_DLL64_DIR}/D3D12SDKLayers.dll"
    "${AGILITY_SDK_DLL64_DIR}/DirectSR.dll"
)

if(MSVC)
    # set warning level
    add_compile_options(/W3 /WX)
endif()

# source files
add_executable(${name} ${PROJECT_SOURCES} ${IGLO_SOURCES} ${IMGUI_SOURCES} ${IMPLOT_SOURCES})

if(MSVC)
    # set linker subsystem
    target_link_options(${name} PRIVATE "/SUBSYSTEM:WINDOWS")
    # no filter for project source files
    source_group(" " FILES ${PROJECT_SOURCES})
endif()

# includes
target_include_directories(${name} PRIVATE 
    ${IGLO_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${IMPLOT_INCLUDE_DIR}
    ${INIPARSER_INCLUDE_DIR}
    ${BOX2D_INCLUDE_DIR}
    ${AGILITY_SDK_INCLUDE_DIR}
)

# link the correct Box2D library based on the build type
target_link_libraries(${name} PRIVATE
    $<$<CONFIG:Debug>:${BOX2D_DEBUG_LIB}>
    $<$<CONFIG:Release>:${BOX2D_RELEASE_LIB}>
)

# filters
source_group("iglo" FILES ${IGLO_SOURCES})
source_group("imgui" FILES ${IMGUI_SOURCES})
source_group("implot" FILES ${IMPLOT_SOURCES})
 
# create D3D12 folder
add_custom_command(
    TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${name}>/D3D12
)

# copy D3D12 files
foreach(d3d12file ${AGILITY_SDK_DLLS})
    add_custom_command(
        TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${d3d12file} $<TARGET_FILE_DIR:${name}>/D3D12
    )
endforeach()

# copy resources
if(EXISTS "resources")
  add_custom_command(
      TARGET ${name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${name}>/resources
  )
endif()

# set executable output directory
# and set working dir to 'name/Debug' or 'name/Release' whichever is active.
set_target_properties(${name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${name}
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${name}/${CMAKE_CFG_INTDIR}
)
