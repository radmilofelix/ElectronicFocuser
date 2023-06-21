cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi-regulusfocuser CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

include(GNUInstallDirs)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

find_package(INDI REQUIRED)
find_package(MODBUS REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_regulusfocuser.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_regulusfocuser.xml)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})

include(CMakeCommon)

#set(GPIO_LIBRARIES "libgpiod.so")

################ ROR System ################
set(indi_regulusfocuser_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/regulusfocuser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/modbusMaster.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indi_regulusfocuser indi_regulusfocuser_SRCS 6 cpp)
ENDIF ()

add_executable(indi_regulusfocuser ${indi_regulusfocuser_SRCS})
target_link_libraries(indi_regulusfocuser ${INDI_LIBRARIES} modbus)
install(TARGETS indi_regulusfocuser RUNTIME DESTINATION bin )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_regulusfocuser.xml DESTINATION ${INDI_DATA_DIR})

