#
# LocalSetup.cmake
#
# This file contains further instructions to set up the DTOOL package
# when using CMake.
#

include(CheckCXXSourceCompiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)

# Check if we have getopt.
check_function_exists(getopt HAVE_GETOPT)
check_function_exists(getopt_long_only HAVE_GETOPT_LONG_ONLY)
check_include_file_cxx(getopt.h PHAVE_GETOPT_H)

# Define if the C++ iostream library defines ios::binary.
check_cxx_source_compiles("
#include <ios>
std::ios::openmode binary = std::ios::binary;
int main(int argc, char *argv[]) { return 0; }
" HAVE_IOS_BINARY)

# Do we have all these header files?
check_include_file_cxx(unistd.h PHAVE_UNISTD_H)
check_include_file_cxx(utime.h PHAVE_UTIME_H)
check_include_file_cxx(glob.h PHAVE_GLOB_H)
check_include_file_cxx(dirent.h PHAVE_DIRENT_H)

# Set LINK_ALL_STATIC if we're building everything as static libraries.
# Also set the library type used for "modules" appropriately.
if(BUILD_SHARED_LIBS)
  set(LINK_ALL_STATIC OFF)
  set(MODULE_TYPE "MODULE"
    CACHE INTERNAL "" FORCE)

else()
  set(LINK_ALL_STATIC ON)
  set(MODULE_TYPE "MODULE"
    CACHE INTERNAL "" FORCE)

endif()

# Now go through all the packages and report whether we have them.
show_packages()

message("")
if(HAVE_PYTHON AND NOT PYTHON_FOUND)
  message(SEND_ERROR "Configured interrogate with Python bindings, but no Python library found.  Disable HAVE_PYTHON to continue.")
elseif(BUILD_PYTHON_BINDINGS)
  message("Compilation will generate Python interfaces for Python ${PYTHON_VERSION_STRING}.")
else()
  message("Configuring interrogate WITHOUT Python interfaces.")
endif()

message("")
