cmake_minimum_required(VERSION 3.21)

project(SkyrimNet_Leashed VERSION 0.1.0 LANGUAGES CXX RC)

set(MOD_FOLDER_NAME "SkyrimNet_Leashed")

if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()

if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${MOD_FOLDER_NAME}")
endif()

find_package(CommonLibSSE CONFIG REQUIRED)
# CommonLibSSE's exported link interface names Microsoft::DirectXTK, so this has to be
# found here even though nothing under src/ includes it.
find_package(DirectXTK CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)

file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

configure_file("src/Version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/Version.rc" @ONLY)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${SOURCE_FILES} "${CMAKE_CURRENT_BINARY_DIR}/Version.rc")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/PrismaUI"
)
target_link_libraries(${PROJECT_NAME} PRIVATE glaze::glaze)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/Zi")
    target_link_options(${PROJECT_NAME} PRIVATE "/DEBUG" "/OPT:REF" "/OPT:ICF")
endif()

set(REPO_DLL_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../SKSE/Plugins")
set(POST_BUILD_COPY
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${REPO_DLL_FOLDER}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${REPO_DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
)

if(DEFINED OUTPUT_FOLDER)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")
    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")
    list(APPEND POST_BUILD_COPY
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../PrismaUI" "${OUTPUT_FOLDER}/PrismaUI"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}/SkyrimNet/config/plugins/SkyrimNet_Leashed"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml" "${DLL_FOLDER}/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml"
    )
endif()

add_custom_command(
    TARGET "${PROJECT_NAME}"
    POST_BUILD
    ${POST_BUILD_COPY}
    VERBATIM
)
