cmake_minimum_required(VERSION 3.1...3.18) #require a minimum cmake version to build
# If the version is less than tested, make a warning about it, else try to use 3.18
if(${CMAKE_VERSION} VERSION_LESS 3.18)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.18)
endif()
#------------------------------------------------------------------------------------------------
# HELPER MODULES
#------------------------------------------------------------------------------------------------
# Instead of Using CMAKE_MODULE_PATH, generate a variable to go into it and use .cmake extension
# That way we don't overwrite other projects' CMAKE_MODULE_PATH and keep it cleaner to read
set(TE_cmake ${CMAKE_CURRENT_SOURCE_DIR}/FlyLib/cmake_modules)
include(${TE_cmake}/Helpers.cmake)
#------------------------------------------------------------------------------------------------
# ADD SUBPROJECTS
#------------------------------------------------------------------------------------------------
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FlyLib)
#------------------------------------------------------------------------------------------------
# FETCH SUBPROJECTS
#------------------------------------------------------------------------------------------------
include(FetchContent)
set(EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/external/TempEngine_deps)
set(FETCHCONTENT_BASE_DIR ${EXTERNAL})
execute_process(COMMAND mkdir -p ${EXTERNAL})
if(USE_IMGUI)
    add_compile_definitions(USE_IMGUI)
    # DearIMGUI--------------------------------------------------------------------------------------
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG "v1.79"
    )
    FetchContent_GetProperties(imgui imgui_SOURCE_DIR imgui_include) # Generates imgui_SOURCE_DIR and imgui_BINARY_DIR
    FetchContent_MakeAvailable(imgui)   
    set(imgui_include ${imgui_SOURCE_DIR})
    set(imgui_src
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_include}/imgui_draw.cpp
        ${imgui_include}/imgui_widgets.cpp
        ${imgui_include}/imgui.cpp
        # Implementation Files
        ${imgui_include}/examples/imgui_impl_opengl3.cpp
    )
    if(ToWindows OR ToLinux OR ToDarwing OR ToMacOS OR ToDesktop)
        list(APPEND imgui_src ${imgui_include}/examples/imgui_impl_glfw.cpp)
    elseif(ToAndroid)
        list(APPEND imgui_src ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui_android/imgui_impl_android.cpp)
        set(imgui_android ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui_android)
    endif()
    
endif(USE_IMGUI)
#------------------------------------------------------------------------------------------------
# SETUP CURRENT PROJECT
#------------------------------------------------------------------------------------------------
project(TempEngine     VERSION 2020.1
                        DESCRIPTION "Multiplatform C++ Engine"
                        LANGUAGES C CXX)
# C/CXX Requirements
set(CMAKE_CXX_STANDARD 17) # Currently using <filesystem> for easy output and folder creation
# Probably I can downgrade to C++11 because most new things I don't really use with thought into it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON) # Android Turns it off (i still don't know why but it should)

# The Engine will be provided as is, no options initally to select
# Maybe in future yes for installing submodules of the engine
file(GLOB_RECURSE ENGINE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/EngineCode/main.cpp")
#------------------------------------------------------------------------------------------------
# SETUP BUILD PATHS
#------------------------------------------------------------------------------------------------
execute_process(COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/builds)
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/builds)
#------------------------------------------------------------------------------------------------
# CUSTOM BUILD MODULES PER PLATFORM
#------------------------------------------------------------------------------------------------
if(ToAndroid) 
    #------------------------------------------------------------------------------------------------
    # SETUP PROJECT OUTPUT
    #------------------------------------------------------------------------------------------------
    set(TE_OutputFolder ${CMAKE_BINARY_DIR}/makecapk/lib/arm64-v8a)
    execute_process(COMMAND mkdir -p ${TE_OutputFolder})
    add_library(TempEngine SHARED "${ENGINE_SRC}")
    #------------------------------------------------------------------------------------------------
    # PLATFORM SPECIFIC INCLUDE/LINKS
    #------------------------------------------------------------------------------------------------
    include(${TE_cmake}/FindAndroidSDKVars.cmake)  
    include(${TE_cmake}/SetupAndroidEnv.cmake)
    set_app_properties(TempEngine MarcTorresJimenez 29)
    set_android_compile_flags()
    set_android_link_flags()
    set_android_compiler(${NDK} ${OS_NAME} ${ANDROIDVERSION})
    link_android_libc(TempEngine ${NDK} ${OS_NAME} ${ANDROIDVERSION})
    if(WIN32)
        #include_android(TempEngine ${NDK} ${OS_NAME})
    endif()
    #------------------------------------------------------------------------------------------------
    # PLATFORM INSTALL TARGETS
    #------------------------------------------------------------------------------------------------
    include(${TE_cmake}/AndroidInstallTargets.cmake)

#------------------------------------------------------------------------------------------------
# DESKTOP PLATFORMS
#------------------------------------------------------------------------------------------------
elseif(ToWindows OR ToLinux OR ToDarwin OR ToMacOS OR ToDesktop)
    #------------------------------------------------------------------------------------------------
    # SET MINIMAL C/CXX FLAGS
    #------------------------------------------------------------------------------------------------
    set(CMAKE_C_FLAGS "-v ")
    set(CMAKE_CXX_FLAGS "-v ")
    #------------------------------------------------------------------------------------------------
    # CROSS-COMPILING Windows-Linux
    #------------------------------------------------------------------------------------------------
    set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
    # Change System Name to desired, then setup compiler and linkers and whatever is needed for crosscompiling
    set(TE_OutputFolder "${CMAKE_BINARY_DIR}/${CMAKE_SYSTEM_NAME}")
    #------------------------------------------------------------------------------------------------
    # SETUP PROJECT OUTPUT
    #------------------------------------------------------------------------------------------------
    add_executable(TempEngine "${ENGINE_SRC}")
    #------------------------------------------------------------------------------------------------
    # PLATFORM SPECIFIC INCLUDE/LINKS
    #------------------------------------------------------------------------------------------------
endif()

#------------------------------------------------------------------------------------------------
# SETTING FINAL PROPERTIES FOR THE PROJECT
#------------------------------------------------------------------------------------------------
set_target_properties(TempEngine
    PROPERTIES 
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/archive"
        LIBRARY_OUTPUT_DIRECTORY "${TE_OutputFolder}"
        RUNTIME_OUTPUT_DIRECTORY "${TE_OutputFolder}"
)
# INCLUDE AND LINK TO FLYLIB
target_link_libraries(TempEngine 
    PUBLIC 
        FlyLib)

target_include_directories(TempEngine 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/FlyLib/flylib_src
        ${CMAKE_CURRENT_SOURCE_DIR}/EngineCode)

# INCLUDE AND LINK TO OTHER SUBPROJECTS
target_sources(TempEngine PUBLIC
    "${imgui_src}"
)
target_include_directories(TempEngine   PUBLIC
    ${imgui_include} ${imgui_include}/examples ${imgui_android}
)