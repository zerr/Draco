#-----------------------------*-cmake-*----------------------------------------#
# file   device/test/CMakeLists.txt
# brief  Instructions for building device/test Makefile.
# note   Copyright(C) 2016-2018 Los Alamos National Security, LLC.
#        All rights reserved.
#------------------------------------------------------------------------------#

project( device_test CXX )

# ---------------------------------------------------------------------------- #
# Build GPU_Device
# ---------------------------------------------------------------------------- #

if( USE_CUDA AND EXISTS "${CUDA_NVCC_EXECUTABLE}" )
  include( GPU_Device.cmake )
endif()

# ---------------------------------------------------------------------------- #
# End device/test/CMakeLists.txt
# ---------------------------------------------------------------------------- #
