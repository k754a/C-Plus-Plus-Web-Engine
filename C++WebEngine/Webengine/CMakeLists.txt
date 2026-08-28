cmake_minimum_required(VERSION 3.21)

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
if(WIN32)
    add_executable(WebEngine WIN32 ${ENGINE_SOURCES} ${ENGINE_HEADERS})
else()
    add_executable(WebEngine ${ENGINE_SOURCES} ${ENGINE_HEADERS})
endif()

if(MSVC)
     set_property(TARGET WebEngine PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set_target_properties(WebEngine PROPERTIES FOLDER "Core")

#folder names groups
source_group("Source Files\\Source Files" FILES ${ENGINE_SOURCES})
source_group("Header Files\\Header Files" FILES ${ENGINE_HEADERS})

#if win 32, use the main stuff, and some ms libs (ws2_32)
if(WIN32)

    set(SDL3_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL3")

    set(SDL3_TTF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL_ttf")

    set(SDL3_IMAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL3_image")


    target_include_directories(WebEngine PRIVATE "${SDL3_PATH}/include" "${SDL3_TTF_PATH}/include" "${SDL3_IMAGE_PATH}/include" )

    target_link_directories(WebEngine PRIVATE "${SDL3_PATH}/lib/x64" "${SDL3_TTF_PATH}/lib/x64" "${SDL3_IMAGE_PATH}/lib/x64" )

    target_link_libraries(WebEngine PRIVATE
        SDL3
        SDL3_ttf
        SDL3_image
        ws2_32
    )
    #move the dll's to the main running, to prevent errors.
    add_custom_command(TARGET WebEngine POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_PATH}/lib/x64/SDL3.dll"
        "$<TARGET_FILE_DIR:WebEngine>/SDL3.dll"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_TTF_PATH}/lib/x64/SDL3_ttf.dll"
        "$<TARGET_FILE_DIR:WebEngine>/SDL3_ttf.dll"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_IMAGE_PATH}/lib/x64/SDL3_image.dll"
        "$<TARGET_FILE_DIR:WebEngine>/SDL3_image.dll"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts" 
        "$<TARGET_FILE_DIR:WebEngine>/fonts"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/logo" 
        "$<TARGET_FILE_DIR:WebEngine>/logo"

    )
else() #if its macos or linux...


    find_package(PkgConfig REQUIRED) #find the CMake thing
    find_package(CURL REQUIRED) #find the Curl thing

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
        CURL::libcurl
    )

    target_link_options(WebEngine PRIVATE -static-libgcc -static-libstdc++)

    set_target_properties(WebEngine PROPERTIES 
        BUILD_RPATH "$ORIGIN" 
        INSTALL_RPATH "$ORIGIN"
    )



   # Copy fonts and logos post-build using standard Linux commands
    add_custom_command(TARGET WebEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:WebEngine>/fonts"
        COMMAND cp -rf "${CMAKE_CURRENT_SOURCE_DIR}/fonts/." "$<TARGET_FILE_DIR:WebEngine>/fonts/"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:WebEngine>/logo"
        COMMAND cp -rf "${CMAKE_CURRENT_SOURCE_DIR}/logo/." "$<TARGET_FILE_DIR:WebEngine>/logo/"
    )
endif()