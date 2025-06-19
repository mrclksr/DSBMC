cmake_minimum_required(VERSION 3.16)
project(dsbmc LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/${CMAKE_PROJECT_NAME})
set(SOURCE_FILES
  src/main.cpp
  src/model.cpp
  src/thread.cpp
  src/mainwin.cpp
  src/preferences.cpp
  lib/qt-helper/qt-helper.cpp
  lib/config.c
  lib/dsbcfg/dsbcfg.c
  lib/libdsbmc/libdsbmc.c)

option(USE_QT6 "Build with Qt6." OFF)
if (USE_QT6)
  set(QT_MAJOR_VERSION 6)
else()
  set(QT_MAJOR_VERSION 5)
endif()

find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS Widgets LinguistTools)
if (${QT_MAJOR_VERSION} GREATER OR EQUAL 6)
  qt_standard_project_setup()
else()
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
endif()

set(TS_FILES locale/dsbmc_ar.ts
             locale/dsbmc_cs.ts
             locale/dsbmc_de.ts
             locale/dsbmc_es.ts
             locale/dsbmc_hi.ts
             locale/dsbmc_hu.ts
             locale/dsbmc_id.ts
             locale/dsbmc_it.ts
             locale/dsbmc_ja.ts
             locale/dsbmc_ko.ts
             locale/dsbmc_pl.ts
             locale/dsbmc_pt.ts
             locale/dsbmc_ru.ts
             locale/dsbmc_tr.ts
             locale/dsbmc_uk.ts
             locale/dsbmc_zh.ts)


if (NOT USE_QT6)
  qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
  list(APPEND SOURCE_FILES ${QM_FILES})
endif()

add_executable(dsbmc ${SOURCE_FILES})

if (USE_QT6)
  qt_add_translations(dsbmc
   TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)
endif()

target_include_directories(dsbmc PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/libdsbmc
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/dsbcfg
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/qt-helper)

add_compile_definitions(
  PROGRAM="${CMAKE_PROJECT_NAME}"
  LOCALE_PATH="${DATADIR}")

target_link_libraries(dsbmc PRIVATE Qt${QT_MAJOR_VERSION}::Widgets)

install(TARGETS dsbmc DESTINATION bin)
install(FILES "${CMAKE_PROJECT_NAME}.desktop"
  DESTINATION share/applications)
install(FILES ${QM_FILES} DESTINATION "${DATADIR}")
