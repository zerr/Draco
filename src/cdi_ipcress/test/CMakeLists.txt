#-----------------------------*-cmake-*----------------------------------------#
# file   cdi_ipcress/test/CMakeLists.txt
# author Kelly Thompson <kgt@lanl.gov>
# date   2012 Aug 1
# brief  Generate build project files for cdi_ipcress/test.
# note   Copyright (C) 2016-2018, Los Alamos National Security, LLC.
#        All rights reserved.
#------------------------------------------------------------------------------#
project( cdi_ipcress_test CXX )

# ---------------------------------------------------------------------------- #
# Source files
# ---------------------------------------------------------------------------- #

set( test_sources
  tIpcressFile.cc
  tIpcressOpacity.cc
  tIpcressWithCDI.cc
  #   ReadOdfIpcressFile.cc # needs argument '--unittest'
  tIpcressOdfmgOpacity.cc
  )
file( GLOB ipcress_data *.ipcress )

##---------------------------------------------------------------------------##
## Copy data files
##---------------------------------------------------------------------------##

foreach( file ${ipcress_data} )
  get_filename_component( shortname ${file} NAME )
  configure_file( ${file}
    ${PROJECT_BINARY_DIR}/${shortname} COPYONLY )
endforeach()

# ---------------------------------------------------------------------------- #
# Build Unit tests
# ---------------------------------------------------------------------------- #

add_scalar_tests(
  SOURCES "${test_sources}"
  DEPS    "Lib_cdi_ipcress" )

add_scalar_tests(
  SOURCES "ReadOdfIpcressFile.cc"
  DEPS    "Lib_cdi_ipcress"
  TEST_ARGS "-u" )

include( ApplicationUnitTest )
add_app_unit_test(
  DRIVER    ${CMAKE_CURRENT_SOURCE_DIR}/tIpcress_Interpreter.py
  APP       $<TARGET_FILE_DIR:Exe_Ipcress_Interpreter>/$<TARGET_FILE_NAME:Exe_Ipcress_Interpreter>
  TEST_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/Al_BeCu.ipcress"
  STDINFILE "${PROJECT_SOURCE_DIR}/IpcressInterpreter.stdin"
  GOLDFILE  "${PROJECT_SOURCE_DIR}/Ipcress_Interpreter-Al_BeCu.ipcress.gold"
  )

#------------------------------------------------------------------------------#
# End cdi_ipcress/test/CMakeLists.txt
#------------------------------------------------------------------------------#
