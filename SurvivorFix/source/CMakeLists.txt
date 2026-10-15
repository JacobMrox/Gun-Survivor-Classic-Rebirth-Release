cmake_minimum_required(VERSION 3.20)
project(survivor_fix C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SF_VERSION "dev" CACHE STRING "Versione scritta nel log")

# Runtime statico: niente redistributable da installare. Va impostato prima di
# ogni target, ps1_logic compreso, altrimenti il link della DLL fallisce.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Logica PS1 pura: compila ovunque, testata in CI su Linux.
add_library(ps1_logic STATIC src/ps1/stumble_logic.cpp src/ps1/backfall_logic.cpp
            src/ps1/nav_logic.cpp)

include(CTest)
if(BUILD_TESTING)
  add_executable(test_stumble_logic tests/test_stumble_logic.cpp)
  target_link_libraries(test_stumble_logic PRIVATE ps1_logic)
  add_test(NAME stumble_logic COMMAND test_stumble_logic)
  add_executable(test_backfall_logic tests/test_backfall_logic.cpp)
  target_link_libraries(test_backfall_logic PRIVATE ps1_logic)
  add_test(NAME backfall_logic COMMAND test_backfall_logic)
  add_executable(test_nav_logic tests/test_nav_logic.cpp)
  target_link_libraries(test_nav_logic PRIVATE ps1_logic)
  add_test(NAME nav_logic COMMAND test_nav_logic)
endif()

if(NOT WIN32)
  return()
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "gun_bio.exe e' a 32 bit: configurare con -A Win32 (MSVC) o un toolchain i686.")
endif()

include(FetchContent)
FetchContent_Declare(minhook
  GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
  GIT_TAG v1.3.3)
FetchContent_MakeAvailable(minhook)

add_library(minhook STATIC
  ${minhook_SOURCE_DIR}/src/buffer.c
  ${minhook_SOURCE_DIR}/src/hook.c
  ${minhook_SOURCE_DIR}/src/trampoline.c
  ${minhook_SOURCE_DIR}/src/hde/hde32.c)
target_include_directories(minhook PUBLIC ${minhook_SOURCE_DIR}/include)

set(SF_SOURCES
  src/dllmain.cpp
  src/config.cpp
  src/exe_info.cpp
  src/game.cpp
  src/hooks.cpp
  src/log.cpp
  src/features/anim_observer.cpp
  src/features/bite_from_behind.cpp
  src/features/room_observer.cpp
  src/features/ps1_zombie_stumble.cpp)

# survivor_fix.dll: caricata dal launcher.
add_library(survivor_fix SHARED ${SF_SOURCES})
target_compile_definitions(survivor_fix PRIVATE SF_VERSION="${SF_VERSION}" UNICODE _UNICODE)
target_link_libraries(survivor_fix PRIVATE ps1_logic minhook bcrypt)
set_target_properties(survivor_fix PROPERTIES PREFIX "")

# dsound.dll: stesso codice, caricata dal gioco come proxy di DirectSound (niente launcher).
add_library(survivor_fix_dsound SHARED ${SF_SOURCES} src/proxy/dsound_proxy.cpp
            src/proxy/dsound_proxy.def)
target_compile_definitions(survivor_fix_dsound PRIVATE SF_VERSION="${SF_VERSION}" UNICODE _UNICODE)
target_link_libraries(survivor_fix_dsound PRIVATE ps1_logic minhook bcrypt)
set_target_properties(survivor_fix_dsound PROPERTIES PREFIX "" OUTPUT_NAME dsound)

add_executable(survivor_fix_launcher WIN32 launcher/launcher.cpp)
target_compile_definitions(survivor_fix_launcher PRIVATE UNICODE _UNICODE)

if(MINGW)
  target_link_options(survivor_fix PRIVATE -static)
  target_link_options(survivor_fix_dsound PRIVATE -static)
  target_link_options(survivor_fix_launcher PRIVATE -static -municode)
endif()
