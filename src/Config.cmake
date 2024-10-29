#
# dtool/Config.cmake
#
# This file defines certain configuration variables that are written
# into the various make scripts.  It is processed by CMake to
# generate build scripts appropriate to each environment.
#

include(CMakeDependentOption)

# Define the type of build we are setting up.

set(_configs Standard Release RelWithDebInfo Debug MinSizeRel)
if(DEFINED CMAKE_CXX_FLAGS_COVERAGE)
  list(APPEND _configs Coverage)
endif()

# Are we building with static or dynamic linking?
if(EMSCRIPTEN OR WASI)
  set(_default_shared OFF)
else()
  set(_default_shared ON)
endif()
option(BUILD_SHARED_LIBS
  "Causes subpackages to be built separately -- setup for dynamic linking.
Utilities/tools/binaries/etc are then dynamically linked to the
libraries instead of being statically linked." ${_default_shared})


# The following options relate to interrogate, the tool that is
# used to generate bindings for non-C++ languages.

cmake_dependent_option(BUILD_PYTHON_BINDINGS
  "Do you want to generate a Python-callable interrogate interface?
This is only necessary if you plan to make calls into interrogatedb
from a program written in Python.  This is done only if HAVE_PYTHON
is also true." ON "HAVE_PYTHON" OFF)

#
# Python
#

set(WANT_PYTHON_VERSION ""
  CACHE STRING "Which Python version to seek out for building the Python bindings against.")

if(DEFINED _PREV_WANT_PYTHON_VERSION
    AND NOT _PREV_WANT_PYTHON_VERSION STREQUAL WANT_PYTHON_VERSION)
  # The user changed WANT_PYTHON_VERSION. We need to force FindPython to start
  # anew, deleting any variable that was autodetected last time
  foreach(_prev_var ${_PREV_PYTHON_VALUES})
    string(REPLACE "=" ";" _prev_var "${_prev_var}")
    list(GET _prev_var 0 _prev_var_name)
    list(GET _prev_var 1 _prev_var_sha1)
    string(SHA1 _current_var_sha1 "${${_prev_var_name}}")

    if(_prev_var_sha1 STREQUAL _current_var_sha1)
      unset(${_prev_var_name} CACHE)
    endif()

  endforeach(_prev_var)

  unset(_PREV_PYTHON_VALUES CACHE)

endif()

if(WANT_PYTHON_VERSION)
  # A specific version is requested; ensure we get that specific version
  list(APPEND WANT_PYTHON_VERSION "EXACT")
endif()

get_directory_property(_old_cache_vars CACHE_VARIABLES)
if(CMAKE_VERSION VERSION_LESS "3.26")
  find_package(Python ${WANT_PYTHON_VERSION} QUIET COMPONENTS Interpreter Development.Module)
  set(_IMPORTED_AS Python::Module)
else()
  find_package(Python ${WANT_PYTHON_VERSION} QUIET COMPONENTS Interpreter Development.Module Development.SABIModule)
  set(_IMPORTED_AS Python::SABIModule)
endif()

if(Python_FOUND)
  set(PYTHON_FOUND ON)
  set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
  set(PYTHON_INCLUDE_DIRS ${Python_INCLUDE_DIRS})
  set(PYTHON_LIBRARY_DIRS ${Python_LIBRARY_DIRS})
  set(PYTHON_VERSION_STRING ${Python_VERSION})

  execute_process(COMMAND ${Python_EXECUTABLE}
    -c "import sysconfig;print(sysconfig.get_config_var('Py_GIL_DISABLED') or 0)"
    OUTPUT_VARIABLE PYTHON_FREETHREADED
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(PYTHON_FREETHREADED)
    # The free-threaded build has no stable ABI (yet).
    set(_IMPORTED_AS Python::Module)
  endif()
endif()

if(CMAKE_VERSION VERSION_LESS "3.15")
  # CMake versions this old don't provide Python::Module, so we need to hack up
  # the variables to ensure no explicit linkage against libpython occurs

  if(WIN32)
    # Nothing needed here; explicit linkage is appropriate
    set(PYTHON_LIBRARY "${Python_LIBRARY}")
    set(PYTHON_LIBRARIES ${Python_LIBRARIES})

  elseif(APPLE OR UNIX)
    # Just unset and let the implicit linkage take over
    set(PYTHON_LIBRARY "")
    set(PYTHON_LIBRARIES "")

    if(APPLE)
      # macOS requires this explicit flag on the linker command line to allow the
      # references to the Python symbols to resolve at dynamic link time
      string(APPEND CMAKE_MODULE_LINKER_FLAGS " -undefined dynamic_lookup")

    endif()

  else()
    # On every other platform, guessing is a bad idea - insist the user upgrade
    # their CMake instead.
    message(WARNING "For Python support on this platform, please use CMake >= 3.15!")
    set(PYTHON_FOUND OFF)

  endif()

endif()

package_option(Python
  DEFAULT ON
  "Enables support for Python.  If BUILD_PYTHON_BINDINGS is also
enabled, Python bindings for interrogatedb will be generated."
  IMPORTED_AS ${_IMPORTED_AS}})

# Also detect the optimal install paths:
if(HAVE_PYTHON)
  if(WIN32 AND NOT CYGWIN)
    set(_LIB_DIR ".")
    set(_ARCH_DIR ".")

  elseif(PYTHON_EXECUTABLE)
    # Python 3.12 drops the distutils module, so we have to use the newer
    # sysconfig module instead.  Earlier versions of Python had the newer
    # module too, but it was broken in Debian/Ubuntu, see #1230
    if(PYTHON_VERSION_STRING VERSION_LESS "3.12")
      execute_process(
        COMMAND ${PYTHON_EXECUTABLE}
          -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(False))"
        OUTPUT_VARIABLE _LIB_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      execute_process(
        COMMAND ${PYTHON_EXECUTABLE}
          -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(True))"
        OUTPUT_VARIABLE _ARCH_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
      execute_process(
        COMMAND ${PYTHON_EXECUTABLE}
          -c "import sysconfig; print(sysconfig.get_path('purelib'))"
        OUTPUT_VARIABLE _LIB_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      execute_process(
        COMMAND ${PYTHON_EXECUTABLE}
          -c "import sysconfig; print(sysconfig.get_path('platlib'))"
        OUTPUT_VARIABLE _ARCH_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()

  else()
    set(_LIB_DIR "")
    set(_ARCH_DIR "")

  endif()

  if(PYTHON_FREETHREADED)
    if(CYGWIN)
      set(_EXT_SUFFIX ".${Python_SOABI}.dll")
    elseif(WIN32)
      set(_EXT_SUFFIX ".${Python_SOABI}.pyd")
    else()
      set(_EXT_SUFFIX ".${Python_SOABI}.so")
    endif()
  elseif(CYGWIN)
    set(_EXT_SUFFIX ".dll")
  elseif(WIN32)
    set(_EXT_SUFFIX ".pyd")
  elseif(Python_SOSABI)
    set(_EXT_SUFFIX ".${Python_SOSABI}.so")
  else()
    set(_EXT_SUFFIX ".abi3.so")
  endif()

  set(PYTHON_LIB_INSTALL_DIR "${_LIB_DIR}" CACHE STRING
    "Path to the Python architecture-independent package directory.")

  set(PYTHON_ARCH_INSTALL_DIR "${_ARCH_DIR}" CACHE STRING
    "Path to the Python architecture-dependent package directory.")

  set(PYTHON_EXTENSION_SUFFIX "${_EXT_SUFFIX}" CACHE STRING
    "Suffix for Python binary extension modules.")

endif()

if(NOT DEFINED _PREV_PYTHON_VALUES)
  # We need to make note of all auto-defined Python variables
  set(_prev_python_values)

  get_directory_property(_new_cache_vars CACHE_VARIABLES)
  foreach(_cache_var ${_new_cache_vars})
    if(_cache_var MATCHES "^(Python|PYTHON)_" AND NOT _old_cache_vars MATCHES ";${_cache_var};")
      string(SHA1 _cache_var_sha1 "${${_cache_var}}")
      list(APPEND _prev_python_values "${_cache_var}=${_cache_var_sha1}")
    endif()
  endforeach(_cache_var)

  set(_PREV_PYTHON_VALUES "${_prev_python_values}" CACHE INTERNAL "Internal." FORCE)
endif()

set(_PREV_WANT_PYTHON_VERSION "${WANT_PYTHON_VERSION}" CACHE INTERNAL "Internal." FORCE)


# How to invoke bison and flex.  Panda takes advantage of some
# bison/flex features, and therefore specifically requires bison and
# flex, not some other versions of yacc and lex.  However, you only
# need to have these programs if you need to make changes to the
# bison or flex sources (see the next point, below).

find_package(BISON QUIET)
find_package(FLEX QUIET)

# You may not even have bison and flex installed.  If you don't, no
# sweat; Panda ships with the pre-generated output of these programs,
# so you don't need them unless you want to make changes to the
# grammars themselves (files named *.yxx or *.lxx).

set(HAVE_BISON ${BISON_FOUND})
set(HAVE_FLEX ${FLEX_FOUND})
