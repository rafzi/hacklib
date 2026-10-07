# 3.15: CMP0091, the MSVC runtime is selected by CMAKE_MSVC_RUNTIME_LIBRARY instead of /MD in the flags
CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(hacklib_project)

# set the default configuration to debug
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

set(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(ARCH_64BIT TRUE)
    ADD_DEFINITIONS(-DARCH_64BIT)
    SET(PROJ_BIN ${PROJ_ROOT}/bin64)
    SET(PROJ_LIB ${PROJ_ROOT}/lib64)
ELSE()
    SET(PROJ_BIN ${PROJ_ROOT}/bin)
    SET(PROJ_LIB ${PROJ_ROOT}/lib)
ENDIF()

# default output directories
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY          ${PROJ_BIN})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG    ${PROJ_BIN})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${PROJ_BIN})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY          ${PROJ_BIN})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG    ${PROJ_BIN})
SET(CAMKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE  ${PROJ_BIN})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY          ${PROJ_LIB})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG    ${PROJ_LIB})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE  ${PROJ_LIB})

LINK_DIRECTORIES(${PROJ_LIB})
INCLUDE_DIRECTORIES(${PROJ_ROOT}/include)

# global configuration
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(CMAKE_DEBUG_POSTFIX "d")
SET(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static C/C++ runtime (/MT, /MTd) for everything: each module gets a private copy of the
# CRT. Injected DLLs then share no CRT state with the process they are loaded into, and
# cemu's dispatcher shares none with a guest that uses the ucrt DLL (locks, atexit table,
# stdio, errno). See src/cemu/docs/decisions/staticcrt.md.
SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# Build trees configured before this switch cache /MD in the flags, where it would fight the
# runtime selected above. Strip it from the cache once.
FOREACH(flags_var CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_MINSIZEREL)
    IF("${${flags_var}}" MATCHES "(^| )[/-]M[DT]d?( |$)")
        STRING(REGEX REPLACE "(^| )[/-]M[DT]d?( |$)" " " stripped "${${flags_var}}")
        STRING(STRIP "${stripped}" stripped)
        SET(${flags_var} "${stripped}" CACHE STRING "Flags used by the compiler." FORCE)
        MESSAGE(STATUS "Removed a stale runtime flag (/MD, /MT) from ${flags_var}")
    ENDIF()
ENDFOREACH()

IF(WIN32)
    ADD_DEFINITIONS(/MP)  # parallel compilation
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)  # microsofts useless warnings
    ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)  # microsofts useless warnings
    ADD_DEFINITIONS(-DNOMINMAX)  # microsofts wonderful macros for min and max
ELSEIF(UNIX)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
ENDIF()

# projects
FILE(GLOB subdirs RELATIVE ${PROJ_ROOT} ${PROJ_ROOT}/src/*)
FOREACH(subdir ${subdirs})
    IF(IS_DIRECTORY ${PROJ_ROOT}/${subdir})
        ADD_SUBDIRECTORY(${PROJ_ROOT}/${subdir})
    ENDIF()
ENDFOREACH()
