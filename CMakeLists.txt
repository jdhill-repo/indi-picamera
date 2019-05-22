cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_picamera CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")

include(GNUInstallDirs)

set (GENERIC_VERSION_MAJOR 0)
set (GENERIC_VERSION_MINOR 1)

find_package(Threads REQUIRED)
find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_library(wiringPi_LIB wiringPi)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_picamera.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_picamera_ccd.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})

include(CMakeCommon)

############# INDI PICAMERA ###############
set(indipicamera_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/indi_picamera.cpp
)

add_executable(indi_picamera_ccd ${indipicamera_SRCS})

target_link_libraries(indi_picamera_ccd ${INDI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CFITSIO_LIBRARIES} m ${ZLIB_LIBRARY} ${wiringPi_LIB})

install(TARGETS indi_picamera_ccd RUNTIME DESTINATION bin)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_picamera_ccd.xml DESTINATION ${INDI_DATA_DIR})
