cmake_minimum_required(VERSION 3.10)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

project("WebEngine") #set engine name

#C++ 20, and set the standard on, making sure it builds.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#create a storage area to hold the .cpp files
set(ENGINE_SOURCES
    WebEngine.cpp
    GUI.cpp
    DOMTree.cpp
    Layout.cpp
    Parser.cpp
    ConnectSocket.cpp
)

#create a storage area to hold the .h files
set(ENGINE_HEADERS
    GUI.h
    DOMTree.h
    Layout.h
    Parser.h
    ConnectSocket.h
    Profiler.h
    ThreadPool.h
    config.h
)

#add the .h and .cpp files to the executable
add_executable(WebEngine ${ENGINE_SOURCES} ${ENGINE_HEADERS})
set_target_properties(WebEngine PROPERTIES FOLDER "Core")

#folder names groups
source_group("Source Files\\Source Files" FILES ${ENGINE_SOURCES})
source_group("Header Files\\Header Files" FILES ${ENGINE_HEADERS})

#if win 32, use the main stuff, and some ms libs (ws2_32)
if(WIN32)
    target_include_directories(WebEngine PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL3/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL_ttf/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL3_image/include"
    )
    target_link_directories(WebEngine PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL3/lib/x64"
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL_ttf/lib/x64"
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL3_image/lib/x64"
    )
    target_link_libraries(WebEngine PRIVATE
        SDL3
        SDL3_ttf
        SDL3_image
        ws2_32
    )
else() #if its macos or linux...
    find_package(PkgConfig REQUIRED) #find the CMake thing
    pkg_check_modules(SDL3 REQUIRED sdl3)
    pkg_check_modules(SDL3_TTF REQUIRED sdl3-ttf)
    pkg_check_modules(SDL3_IMAGE REQUIRED sdl3-image)

    target_include_directories(WebEngine PRIVATE
        ${SDL3_INCLUDE_DIRS}
        ${SDL3_TTF_INCLUDE_DIRS}
        ${SDL3_IMAGE_INCLUDE_DIRS}
    )
    target_link_libraries(WebEngine PRIVATE
        ${SDL3_LIBRARIES}
        ${SDL3_TTF_LIBRARIES}
        ${SDL3_IMAGE_LIBRARIES}
    )
endif()

#move the fonts to wherever its running. so that we dont have errors.
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/fonts" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")