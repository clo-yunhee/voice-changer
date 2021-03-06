cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0087 NEW)
project(VoiceChanger)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(wxWidgets_ROOT_DIR "E:/wxWidgets-3.1.5")
    if(MSVC)
        set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/vc14x_x64_dll")
    elseif(MINGW)
        set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/gcc920TDM_x64_dll")
    endif()
endif()

set(SOURCES
        "AberthRoots.cpp"
        "AberthRoots.h"
        "AboutDialog.cpp"
        "AboutDialog.h"
        "AudioTrack.cpp"
        "AudioTrack.h"
        "AudioTrackPlayer.cpp"
        "AudioTrackPlayer.h"
        "Exception.h"
        "FilterDesign.cpp"
        "FilterDesign.h"
        "GlottalModel.cpp"
        "GlottalModel.h"
        "GlottalSource.cpp"
        "GlottalSource.h"
        "Lpc.cpp"
        "Lpc.h"
        "LpcTrack.cpp"
        "LpcTrack.h"
        "Noise.cpp"
        "Noise.h"
        "VoiceChangerApp.cpp"
        "VoiceChangerApp.h"
        "VoiceChangerController.cpp"
        "VoiceChangerController.h"
        "reaper/core/file_resource.cc"
        "reaper/core/file_resource.h"
        "reaper/core/float_matrix-inl.h"
        "reaper/core/float_matrix.cc"
        "reaper/core/float_matrix.h"
        "reaper/core/track.cc"
        "reaper/core/track.h"
        "reaper/epoch_tracker/epoch_tracker.cc"
        "reaper/epoch_tracker/epoch_tracker.h"
        "reaper/epoch_tracker/fd_filter.cc"
        "reaper/epoch_tracker/fd_filter.h"
        "reaper/epoch_tracker/fft.cc"
        "reaper/epoch_tracker/fft.h"
        "reaper/epoch_tracker/lpc_analyzer.cc"
        "reaper/epoch_tracker/lpc_analyzer.h"
        "reaper/reaper.cc"
        "reaper/reaper.h"
)

set(IMAGES
        "Patreon.png"
)

find_package(wxWidgets REQUIRED COMPONENTS richtext html net core base)
include(${wxWidgets_USE_FILE})

set(PA_BUILD_STATIC ON)
add_subdirectory(portaudio EXCLUDE_FROM_ALL)

add_executable(VoiceChanger WIN32 ${SOURCES})
target_link_libraries(VoiceChanger PUBLIC ${wxWidgets_LIBRARIES} portaudio_static)
target_include_directories(VoiceChanger SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/header_libs)

include(png2c/Functions.cmake)
target_embed_images(VoiceChanger ${IMAGES})

if(CMAKE_BUILD_TYPE STREQUAL "Debug"
        OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(VoiceChanger PUBLIC DEBUG)
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(VoiceChanger PUBLIC avrt)

    install(TARGETS VoiceChanger RUNTIME DESTINATION .)

    set(DEPENDENCY_PATHS ${wxWidgets_LIB_DIR})

    install(CODE "set(DEPENDENCY_PATHS \"${DEPENDENCY_PATHS}\")")

    install(CODE [[
        list(APPEND pre_exclude_regexes "api-ms-.*") # windows API
        list(APPEND pre_exclude_regexes "ext-ms-.*") # windows API
        list(APPEND pre_exclude_regexes "ieshims\\.dll") # windows API
        list(APPEND pre_exclude_regexes "emclient\\.dll") # windows API
        list(APPEND pre_exclude_regexes "devicelockhelpers\\.dll") # windows API
        
        list(APPEND post_exclude_regexes ".*windows[\\/]system32.*") # windows system dlls
    
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES $<TARGET_FILE:VoiceChanger>
            RESOLVED_DEPENDENCIES_VAR _r_deps
            UNRESOLVED_DEPENDENCIES_VAR _u_deps 
            DIRECTORIES ${DEPENDENCY_PATHS}
            PRE_EXCLUDE_REGEXES ${pre_exclude_regexes}
            POST_EXCLUDE_REGEXES ${post_exclude_regexes}
        )
        foreach(_file ${_r_deps})
            message(STATUS "${_file}")
            file(INSTALL
                DESTINATION "${CMAKE_INSTALL_PREFIX}"
                TYPE SHARED_LIBRARY
                FOLLOW_SYMLINK_CHAIN
                FILES "${_file}"
            )
        endforeach()
        list(LENGTH _u_deps _u_length)
        if("${_u_length}" GREATER 0)
            message(WARNING "Unresolved dependencies detected!")
        endif()
    ]])
else()
    install(TARGETS VoiceChanger RUNTIME DESTINATION bin)
endif()