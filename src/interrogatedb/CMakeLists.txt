set(INTERROGATEDB_HEADERS
  config_interrogatedb.h indexRemapper.h interrogateComponent.I
  interrogateComponent.h interrogateDatabase.I
  interrogateDatabase.h interrogateElement.I
  interrogateElement.h interrogateFunction.I
  interrogateFunction.h interrogateFunctionWrapper.I
  interrogateFunctionWrapper.h
  interrogateMakeSeq.I interrogateMakeSeq.h
  interrogateManifest.I interrogateManifest.h
  interrogateType.I interrogateType.h
  interrogate_datafile.I interrogate_datafile.h
  interrogate_interface.h interrogate_request.h
)

set(INTERROGATEDB_SOURCES
  config_interrogatedb.cxx
  indexRemapper.cxx
  interrogateComponent.cxx interrogateDatabase.cxx
  interrogateElement.cxx interrogateFunction.cxx
  interrogateFunctionWrapper.cxx
  interrogateMakeSeq.cxx
  interrogateManifest.cxx
  interrogateType.cxx interrogate_datafile.cxx
  interrogate_interface.cxx interrogate_request.cxx
)

set(INTERROGATEDB_IGATE
  interrogate_interface.h
  interrogate_request.h
)

set(IGATERUNTIME_HEADERS
  extension.h py_compat.h py_panda.h py_panda.I py_wrappers.h
)

composite_sources(interrogatedb INTERROGATEDB_SOURCES)
add_library(interrogatedb
  ${INTERROGATEDB_HEADERS} ${INTERROGATEDB_SOURCES})
set_target_properties(interrogatedb PROPERTIES DEFINE_SYMBOL BUILDING_INTERROGATEDB)
target_link_libraries(interrogatedb dtoolutil)

install(TARGETS interrogatedb
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/interrogatedb)
install(FILES ${INTERROGATEDB_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/interrogatedb)
install(FILES ${IGATERUNTIME_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/interrogatedb)

# ALSO: This has an Interrogate binding! Take care of that if we want it.
# Note we don't use the regular Interrogate macros; this has some custom flags
# that would make it not worthwhile.

if(NOT BUILD_PYTHON_BINDINGS)
  return()
endif()

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/interrogatedb_module.cxx"
  COMMAND host_interrogate
    -D EXPCL_INTERROGATEDB=
    -nodb -python -promiscuous
    -module interrogatedb
    -library interrogatedb
    -string -true-names -do-module
    -srcdir "${CMAKE_CURRENT_SOURCE_DIR}"
    -oc "${CMAKE_CURRENT_BINARY_DIR}/interrogatedb_module.cxx"
    ${INTERROGATEDB_IGATE}

  DEPENDS host_interrogate ${INTERROGATEDB_IGATE}
  COMMENT "Interrogating interrogatedb")

add_python_target(interrogatedb_module
  MODULE_NAME interrogatedb
  "${CMAKE_CURRENT_BINARY_DIR}/interrogatedb_module.cxx")
target_link_libraries(interrogatedb_module PRIVATE interrogatedb)
