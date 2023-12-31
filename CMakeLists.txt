cmake_minimum_required(VERSION 3.5)

#project(eclChat LANGUAGES CXX)
project(eclChat)

option(SERVER "build server" OFF)


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/imgui")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends")

set(IMGUI_HOME ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
if(SERVER)
    set(SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/mainserver.cpp)
else()
    set(SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/mainclient.cpp)
    set(SRC_LIST ${SRC_LIST} ${IMGUI_HOME}/imgui.cpp)
    set(SRC_LIST ${SRC_LIST} ${IMGUI_HOME}/imgui_demo.cpp)
    set(SRC_LIST ${SRC_LIST} ${IMGUI_HOME}/imgui_draw.cpp)
    set(SRC_LIST ${SRC_LIST} ${IMGUI_HOME}/imgui_tables.cpp)
    set(SRC_LIST ${SRC_LIST} ${IMGUI_HOME}/imgui_widgets.cpp)
    set(SRC_LIST ${SRC_LIST} ${IMGUI_HOME}/backends/imgui_impl_opengl3.cpp)
    set(SRC_LIST ${SRC_LIST} ${IMGUI_HOME}/backends/imgui_impl_sdl2.cpp)
endif()
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/base64/base64.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/add_scalar.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/fe.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/ge.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/key_exchange.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/keypair.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/sc.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/seed.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/sha512.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/sign.c)
set(SRC_LIST ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/ed25519/src/verify.c)

# message("${SRC_LIST}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -L/usr/lib/x86_64-linux-gnu")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -L/usr/lib/x86_64-linux-gnu")

# Create an option to switch between a system sdl library and a vendored sdl library
option(eclChat_VENDORED "Use vendored libraries" OFF)

if(eclChat_VENDORED)
    add_subdirectory(vendored/sdl EXCLUDE_FROM_ALL)
else()
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
    find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
endif()

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS})

if(SERVER)
    add_executable(server ${SRC_LIST})
    target_link_libraries(server PRIVATE SDL2::SDL2)
    target_link_libraries(server PRIVATE ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
    if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
        target_link_libraries(server PRIVATE SDL2::SDL2main)
    endif()
else()
    add_executable(client ${SRC_LIST})
    target_link_libraries(client PRIVATE SDL2::SDL2)
    target_link_libraries(client PRIVATE ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
    if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
        target_link_libraries(client PRIVATE SDL2::SDL2main)
    endif()
endif()







