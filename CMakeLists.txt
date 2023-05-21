cmake_minimum_required(VERSION 3.16)
project(dsbmc LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/${CMAKE_PROJECT_NAME})
set(TRANSLATIONS locale/dsbmc_de.ts)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

qt_standard_project_setup()

add_executable(dsbmc
  src/main.cpp
  src/model.cpp
  src/thread.cpp
  src/mainwin.cpp
  src/preferences.cpp
  lib/qt-helper/qt-helper.cpp
  lib/config.c
  lib/dsbcfg/dsbcfg.c
  lib/libdsbmc/libdsbmc.c)

target_link_libraries(dsbmc PRIVATE Qt6::Widgets)

target_include_directories(dsbmc PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/libdsbmc
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/dsbcfg
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/qt-helper)

add_compile_definitions(
  PROGRAM="${CMAKE_PROJECT_NAME}"
  LOCALE_PATH="${DATADIR}")

set_source_files_properties(${TRANSLATIONS}
    PROPERTIES OUTPUT_LOCATION "locale")
qt_add_translations(dsbmc TS_FILES ${TRANSLATIONS}
  QM_FILES_OUTPUT_VARIABLE qm_files)

install(TARGETS dsbmc DESTINATION bin)
install(FILES ${qm_files} DESTINATION "${DATADIR}")
install(FILES "${CMAKE_PROJECT_NAME}.desktop"
  DESTINATION share/applications)

