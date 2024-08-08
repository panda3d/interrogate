cmake_minimum_required(VERSION 3.13)
set(CMAKE_DISABLE_SOURCE_CHANGES ON) # Must go before project() below
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON) # Must go before project() below

if(POLICY CMP0091)
  # Needed for CMake to pass /MD flag properly with non-VC generators.
  cmake_policy(SET CMP0091 NEW)
endif()

# Determine whether we are using a multi-config generator.
get_property(IS_MULTICONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Set the default CMAKE_BUILD_TYPE before calling project().
if(IS_MULTICONFIG)
  message(STATUS "Using multi-configuration generator")
else()
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Standard CACHE STRING "Choose the type of build." FORCE)
    message(STATUS "Using default build type ${CMAKE_BUILD_TYPE}")
  else()
    message(STATUS "Using build type ${CMAKE_BUILD_TYPE}")
  endif()
endif()

# Set defaults for macOS, must be before project().
if(APPLE)
  # Needed for enable_language(OBJCXX)
  cmake_minimum_required(VERSION 3.16)

  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum macOS version to target")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

  if(CMAKE_VERSION VERSION_LESS "3.19" AND NOT CMAKE_OSX_SYSROOT)
    # Older CMake chose SDK based on deployment target, against Apple's recommendations.
    # However, we need to use the latest to be able to target arm64.
    if(IS_DIRECTORY "/Library/Developer/CommandLineTools/SDKs/MacOSX11.1.sdk")
      set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX11.1.sdk" CACHE STRING "")
    elseif(IS_DIRECTORY "/Library/Developer/CommandLineTools/SDKs/MacOSX11.0.sdk")
      set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX11.0.sdk" CACHE STRING "")
    endif()
  endif()
endif()

project(interrogate)

if(APPLE)
  # Allows separating out C++ flags from ObjC++ flags
  enable_language(OBJCXX)
endif()

# Determine the possible build types.  Must be *after* calling project().
set(_configs Standard Release RelWithDebInfo Debug MinSizeRel)
if(CMAKE_CXX_COMPILER_ID MATCHES "(AppleClang|Clang|GCC)")
  list(APPEND _configs Coverage)
endif()

if(IS_MULTICONFIG)
  set(CMAKE_CONFIGURATION_TYPES "${_configs}" CACHE STRING "" FORCE)
else()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${_configs})
endif()

string(REPLACE "$(EFFECTIVE_PLATFORM_NAME)" "" PANDA_CFG_INTDIR "${CMAKE_CFG_INTDIR}")

# Add generic modules to cmake module path,
# and add Interrogate specific modules to cmake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros/")

# When using the Xcode generator, don't append the platform name to the
# intermediate configuration directory.
set_property(GLOBAL PROPERTY XCODE_EMIT_EFFECTIVE_PLATFORM_NAME OFF)

# Build all static libraries with -fPIC.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include modules builtin to CMake
include(GNUInstallDirs)     # Defines CMAKE_INSTALL_<dir> variables

# Include global modules needed for configure scripts
include(PackageConfig)      # Defines package_option

# Configure Interrogate
include(src/CompilerFlags.cmake)
include(src/Config.cmake)

# Include global modules
include(AddBisonTarget)     # Defines add_bison_target function
include(AddFlexTarget)      # Defines add_flex_target function
include(CompositeSources)   # Defines composite_sources function
include(Python)             # Defines add_python_target
include(CTest)

add_subdirectory(src "${CMAKE_BINARY_DIR}/src")

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
