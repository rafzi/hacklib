# Create imported target drdecode
add_library(drdecode STATIC IMPORTED)

set_target_properties(drdecode PROPERTIES
  INTERFACE_LINK_LIBRARIES "drlibc"
)


if (WIN32)
  set(decode_lib "${PROJ_LIB}/drdecode.lib")
elseif (UNIX)
  set(decode_lib "${PROJ_LIB}/libdrdecode.a")
endif ()


# Import target "drdecode" for configuration "RelWithDebInfo"
set_property(TARGET drdecode APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(drdecode PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO ${decode_lib}
  )



# helper function
function (_DR_get_lang target lang_var)
  # Note that HAS_CXX and LINKER_LANGUAGE are only defined it
  # explicitly set: can't be used to distinguish CXX vs C.
  get_target_property(sources ${target} SOURCES)
  foreach (src ${sources})
    # LANGUAGE, however, is set for us
    get_source_file_property(src_lang ${src} LANGUAGE)
    if (NOT DEFINED tgt_lang)
      set(tgt_lang ${src_lang})
    elseif (${src_lang} MATCHES CXX)
      # If any source file is cxx, mark as cxx
      set(tgt_lang ${src_lang})
    endif (NOT DEFINED tgt_lang)
  endforeach (src)

  set(${lang_var} ${tgt_lang} PARENT_SCOPE)
endfunction (_DR_get_lang)

# We'll be included at the same scope as the containing project, so use
# a prefixed var name for globals.
get_filename_component(DynamoRIO_cwd "${CMAKE_CURRENT_LIST_FILE}" PATH)

# Export variables in case client needs custom configuration that
# our exported functions do not provide.
# Additionally, only set if not already defined, to allow
# for further customization.
if (NOT DEFINED DynamoRIO_INCLUDE_DIRS)
  set(DynamoRIO_INCLUDE_DIRS "${DynamoRIO_cwd}/../include")
endif (NOT DEFINED DynamoRIO_INCLUDE_DIRS)

if (NOT DEFINED ARM AND NOT DEFINED AARCH64 AND NOT DEFINED RISCV64 AND
    NOT DEFINED X86)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^arm64")
    set(AARCH64 1)
  elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(ARM 1) # This means AArch32.
  elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
    set(AARCH64 1)
  elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^riscv64")
    set(RISCV64 1)
  else ()
    set(X86 1) # This means IA-32 or AMD64
  endif ()
endif ()
if (NOT DEFINED DR_HOST_ARM AND NOT DEFINED DR_HOST_AARCH64 AND NOT DEFINED DR_HOST_RISCV64 AND NOT DEFINED DR_HOST_X86)
  if (ARM)
    set(DR_HOST_ARM 1)
  elseif (AARCH64)
    set(DR_HOST_AARCH64 1)
  elseif (RISCV64)
    set(DR_HOST_RISCV64 1)
  else ()
    set(DR_HOST_X86 1)
  endif ()
endif ()

if (WIN32 OR ANDROID)
  set(USE_DRPATH ON)
else ()
  set(USE_DRPATH OFF)
endif ()

function (configure_DynamoRIO_main_headers)
  include_directories(${DynamoRIO_INCLUDE_DIRS})
endfunction ()

function (DynamoRIO_extra_cflags flags_out extra_cflags tgt_cxx)
  if (DynamoRIO_FAST_IR)
    if (NOT tgt_cxx AND CMAKE_COMPILER_IS_GNUCC)
      # we require C99 for our extern inline functions to work properly
      set(extra_cflags "${extra_cflags} -std=gnu99")
    endif ()
  endif (DynamoRIO_FAST_IR)
  set(${flags_out} "${extra_cflags}" PARENT_SCOPE)
endfunction (DynamoRIO_extra_cflags)

function (_DR_append_property_string type target name value)
  # XXX: if we require cmake 2.8.6 we can simply use APPEND_STRING
  get_property(cur ${type} ${target} PROPERTY ${name})
  if (cur)
    set(value "${cur} ${value}")
  endif (cur)
  set_property(${type} ${target} PROPERTY ${name} "${value}")
endfunction (_DR_append_property_string)

# helper function
function (_DR_get_size is_cxx x64_var)
  if (is_cxx)
    set(sizeof_void ${CMAKE_CXX_SIZEOF_DATA_PTR})
  else (is_cxx)
    set(sizeof_void ${CMAKE_C_SIZEOF_DATA_PTR})
  endif (is_cxx)

  if ("${sizeof_void}" STREQUAL "")
    message(FATAL_ERROR "unable to determine bitwidth: did earlier ABI tests fail?  check CMakeFiles/CMakeError.log")
  endif ("${sizeof_void}" STREQUAL "")

  if (${sizeof_void} EQUAL 8)
    set(${x64_var} ON PARENT_SCOPE)
  else (${sizeof_void} EQUAL 8)
    set(${x64_var} OFF PARENT_SCOPE)
  endif (${sizeof_void} EQUAL 8)
endfunction (_DR_get_size)

function (_DR_append_property_list type target name value)
  set_property(${type} ${target} PROPERTY ${name} ${value} APPEND)
endfunction (_DR_append_property_list)

function (DynamoRIO_extra_defines list_out tgt_cxx)
  _DR_get_size(${tgt_cxx} tgt_x64)
  set(defs_list "")
  if (tgt_x64)
    if (AARCH64)
      set(defs_list ${defs_list} "ARM_64")
    elseif (RISCV64)
      set(defs_list ${defs_list} "RISCV_64")
    else ()
      set(defs_list ${defs_list} "X86_64")
    endif ()
  else (tgt_x64)
    if (ARM)
      set(defs_list ${defs_list} "ARM_32")
    else ()
      set(defs_list ${defs_list} "X86_32")
    endif ()
  endif (tgt_x64)

  if (UNIX)
    if (APPLE)
      set(defs_list ${defs_list} "MACOS")
    else (APPLE)
      set(defs_list ${defs_list} "LINUX")
      if (ANDROID)
        set(defs_list ${defs_list} "ANDROID")
      endif (ANDROID)
    endif (APPLE)
    if (CMAKE_COMPILER_IS_CLANG)
      set(defs_list ${defs_list} "CLANG")
    endif (CMAKE_COMPILER_IS_CLANG)
  else (UNIX)
    set(defs_list ${defs_list} "WINDOWS")
  endif (UNIX)
  if (DynamoRIO_INTERNAL)
    if (DR_HOST_ARM)
      set(defs_list ${defs_list} "DR_HOST_ARM")
    elseif (DR_HOST_AARCH64)
      set(defs_list ${defs_list} "DR_HOST_AARCH64")
    elseif (DR_HOST_RISCV64)
      set(defs_list ${defs_list} "DR_HOST_RISCV64")
    elseif (DR_HOST_X86)
      set(defs_list ${defs_list} "DR_HOST_X86")
    endif ()
    if (DR_HOST_X64)
      set(defs_list ${defs_list} "DR_HOST_X64")
    endif ()
    if (NOT "${TARGET_ARCH}" STREQUAL "${CMAKE_SYSTEM_PROCESSOR}")
      set(defs_list ${defs_list} "DR_HOST_NOT_TARGET")
    endif ()
  endif ()

  if (DynamoRIO_REG_COMPATIBILITY)
    set(defs_list ${defs_list} "DR_REG_ENUM_COMPATIBILITY")
  endif (DynamoRIO_REG_COMPATIBILITY)

  if (DynamoRIO_PAGE_SIZE_COMPATIBILITY)
    set(defs_list ${defs_list} "DR_PAGE_SIZE_COMPATIBILITY")
  endif (DynamoRIO_PAGE_SIZE_COMPATIBILITY)

  if (DynamoRIO_NUM_SIMD_SLOTS_COMPATIBILITY)
    set(defs_list ${defs_list} "DR_NUM_SIMD_SLOTS_COMPATIBILITY")
  endif (DynamoRIO_NUM_SIMD_SLOTS_COMPATIBILITY)

  if (DynamoRIO_LOG_COMPATIBILITY)
    set(defs_list ${defs_list} "DR_LOG_DEFINE_COMPATIBILITY")
  endif ()

  if (DynamoRIO_FAST_IR)
    set(defs_list ${defs_list} "DR_FAST_IR")
  endif (DynamoRIO_FAST_IR)

  set(${list_out} "${defs_list}" PARENT_SCOPE)
endfunction (DynamoRIO_extra_defines)

# Unfortunately, CMake doesn't support removing flags on a per-target basis,
# or per-target include dirs or link dirs, so we have to make global changes.
# We don't want find_package() to incur those changes: only if a target
# is actually configured.
# The is_cxx parameter does not matter much: this routine can be called
# with is_cxx=OFF and C++ clients will still be configured properly,
# unless the C++ compiler and the C compiler are configured for
# different bitwidths.
function (configure_DynamoRIO_global is_cxx change_flags)
  # We need to perform some global config once per cmake directory.
  # We want it to work even if the caller puts code in a function
  # (=> no PARENT_SCOPE var) and we want to re-execute on each re-config
  # (=> no CACHE INTERNAL).  A global property w/ the listdir in the name
  # fits the bill.  Xref i#1052.
  # CMAKE_CURRENT_LIST_DIR wasn't added until CMake 2.8.3 (i#1056).
  get_filename_component(caller_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
  get_property(already_configured_listdir GLOBAL PROPERTY
    DynamoRIO_configured_globally_${caller_dir})
  if (NOT DEFINED already_configured_listdir)
    set_property(GLOBAL PROPERTY
      DynamoRIO_configured_globally_${caller_dir} ON)

    # If called from another function, indicate whether to propagate
    # with a variable that does not make it up to global scope
    if (nested_scope)
      set(just_configured ON PARENT_SCOPE)
    endif (nested_scope)

    configure_DynamoRIO_main_headers()

    if (change_flags)
      # Remove global C flags that are unsafe for a client library.
      # Since CMake does not support removing flags on a per-target basis,
      # we clear the base flags so we can add what we want to each target.
      foreach (config "" ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES})
        if ("${config}" STREQUAL "")
          set(config_upper "")
        else ("${config}" STREQUAL "")
          string(TOUPPER "_${config}" config_upper)
        endif ("${config}" STREQUAL "")
        foreach (var CMAKE_C_FLAGS${config_upper};CMAKE_CXX_FLAGS${config_upper})
          if ("${${var}}" STREQUAL "" OR NOT DEFINED ${var})
            # Empty string will trip the NOT DEFINED ORIG_CMAKE_C_FLAGS check below
            set(${var} " ")
          endif ()
          set(ORIG_${var} "${${var}}" PARENT_SCOPE)
          set(local_${var} "${${var}}")
          if (WIN32)
            # We could limit this global var changing to Windows,
            # but it simplifies cross-platform uses to be symmetric
            set(MT_base "/MT")
            if (DEBUG OR "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
              set(MT_flag "/MTd")
            else ()
              set(MT_flag "/MT")
            endif()
            if (local_${var} MATCHES "/M[TD]")
              string(REGEX REPLACE "/M[TD]" "${MT_base}" local_${var} "${local_${var}}")
            else ()
              set(local_${var} "${local_${var}} ${MT_flag}")
            endif ()
            string(REGEX REPLACE "/RTC." "" local_${var} "${local_${var}}")
          endif (WIN32)
          set(CLIENT_${var} "${CLIENT_${var}} ${local_${var}}" PARENT_SCOPE)
          if (UNIX AND X86 AND ${var} MATCHES "-m32")
            set(base_var_value "-m32")
          else ()
            # If we set to "", the default values come back
            set(base_var_value " ")
          endif ()
          set(${var} "${base_var_value}" PARENT_SCOPE)
        endforeach (var)
      endforeach (config)
    endif (change_flags)

  else (NOT DEFINED already_configured_listdir)
    # We can detect failure to propagate to global scope on the 2nd client
    # in the same listdir.
    # XXX: is there any way we can have better support for functions?
    # I spent a while trying to use CACHE INTERNAL FORCE to set the
    # global vars but it has all kinds of weird consequences for other
    # vars based on the original values of the now-cache vars.
    # This behavior varies by generator and I never found a solution
    # that worked for all generators.  Ninja was easy, but VS and Makefiles
    # ended up with ORIG_* set to the blank values, even when ORIG_*
    # was marked as cache.  Plus, Dr. Memory's SAVE_* values ended up
    # w/ the cache value as well.
    if (NOT DEFINED ORIG_CMAKE_C_FLAGS)
      message(FATAL_ERROR "When invoking configure_DynamoRIO_*() from a function, "
        "configure_DynamoRIO_global() must be called from global scope first.")
    endif (NOT DEFINED ORIG_CMAKE_C_FLAGS)
  endif (NOT DEFINED already_configured_listdir)
endfunction (configure_DynamoRIO_global)

function (configure_DynamoRIO_decoder target)
  _DR_get_lang(${target} tgt_lang)
  if (${tgt_lang} MATCHES CXX)
    set(tgt_cxx ON)
  else (${tgt_lang} MATCHES CXX)
    set(tgt_cxx OFF)
  endif (${tgt_lang} MATCHES CXX)

  # we do not need propagation so no need to set nested
  configure_DynamoRIO_global(${tgt_cxx} OFF)

  DynamoRIO_extra_cflags(extra_cflags "" ${tgt_cxx})
  _DR_append_property_string(TARGET ${target} COMPILE_FLAGS "${extra_cflags}")
  DynamoRIO_extra_defines(extra_defs ${tgt_cxx})
  message("Extra defs: ${extra_defs}")
  _DR_append_property_list(TARGET ${target} COMPILE_DEFINITIONS "${extra_defs}")

  # DynamoRIOTarget.cmake added the "drdecode" imported target
  target_link_libraries(${target} drdecode)

endfunction (configure_DynamoRIO_decoder)