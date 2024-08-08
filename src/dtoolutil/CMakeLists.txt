set(DTOOLUTIL_HEADERS
  dSearchPath.I dSearchPath.h
  filename.I filename.h
  globPattern.I globPattern.h
  lineStream.I lineStream.h
  lineStreamBuf.I lineStreamBuf.h
  load_dso.h
  panda_getopt.h panda_getopt_long.h panda_getopt_impl.h
  preprocess_argv.h
  string_utils.h string_utils.I
  stringDecoder.h stringDecoder.I
  textEncoder.h textEncoder.I
  unicodeLatinMap.h
  vector_string.h
  vector_uchar.h
  win32ArgParser.h
)

if(APPLE)
  set(DTOOLUTIL_HEADERS ${DTOOLUTIL_HEADERS}
    filename_assist.mm filename_assist.h)

  set_source_files_properties(
    filename_assist.mm filename_assist.h PROPERTIES
    WRAP_EXCLUDE YES
    SKIP_UNITY_BUILD_INCLUSION YES)
endif()

set(DTOOLUTIL_SOURCES
  dSearchPath.cxx
  filename.cxx
  globPattern.cxx
  lineStream.cxx lineStreamBuf.cxx
  load_dso.cxx
  panda_getopt_impl.cxx
  preprocess_argv.cxx
  string_utils.cxx
  stringDecoder.cxx
  textEncoder.cxx
  unicodeLatinMap.cxx
  vector_string.cxx
  vector_uchar.cxx
  win32ArgParser.cxx
)

composite_sources(dtoolutil DTOOLUTIL_SOURCES)
add_library(dtoolutil STATIC
  ${DTOOLUTIL_HEADERS} ${DTOOLUTIL_SOURCES})
target_compile_definitions(dtoolutil PRIVATE BUILDING_DTOOL_DTOOLUTIL)
target_link_libraries(dtoolutil dtoolbase ${CMAKE_DL_LIBS})

if(APPLE)
  find_library(FOUNDATION_LIBRARY Foundation)
  find_library(APPKIT_LIBRARY AppKit)
  target_link_libraries(dtoolutil ${FOUNDATION_LIBRARY} ${APPKIT_LIBRARY})

  mark_as_advanced(FOUNDATION_LIBRARY APPKIT_LIBRARY)
endif()

# These are all used by filename.cxx
foreach(var
    # filename.cxx:
    HAVE_IOS_BINARY
    PHAVE_DIRENT_H
    PHAVE_GLOB_H
    PHAVE_UTIME_H
    PHAVE_UNISTD_H
)

  if(${var})
    target_compile_definitions(dtoolutil PRIVATE "${var}=${${var}}")
  endif()

endforeach(var)

# Public definitions for getopt.
foreach(var
    HAVE_GETOPT
    HAVE_GETOPT_LONG_ONLY
    PHAVE_GETOPT_H
)

  if(${var})
    target_compile_definitions(dtoolutil PUBLIC "${var}=${${var}}")
  endif()

endforeach(var)
