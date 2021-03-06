#-----------------------------*-cmake-*----------------------------------------#
# file   config/unix-clang.cmake
# brief  Establish flags for Unix clang
# note   Copyright (C) 2010-2018 Los Alamos National Security, LLC.
#        All rights reserved.
#------------------------------------------------------------------------------#

# Note: In config/compilerEnv.cmake, the build system sets flags for
# 1) the language standard (C++14, C99, etc)
# 2) interprocedural optimization.

#
# Compiler flag checks
#
include(platform_checks)
query_openmp_availability()

# Debug flags to consider adding:
# http://clang.llvm.org/docs/UsersManual.html#options-to-control-error-and-warning-messages
# -fdiagnostics-show-hotness
#
# valgrind like options - https://clang.llvm.org/docs/AddressSanitizer.html
#      '-g -fsanitize=address -fno-omit-frame-pointer'
#      must use clang++ for linking
#      suppressions: LSAN_OPTIONS=suppressions=MyLSan.supp
#      human readable: ASAN_SYMBOLIZER_PATH=/usr/local/bin/llvm-symbolizer ./a.out

#
# Compiler Flags
#

if( NOT CXX_FLAGS_INITIALIZED )
   set( CXX_FLAGS_INITIALIZED "yes" CACHE INTERNAL "using draco settings." )

   set( CMAKE_C_FLAGS                "-Wcast-align -Wpointer-arith -Wall -Wno-long-long -pedantic" )
   if (NOT ${CMAKE_GENERATOR} MATCHES Xcode AND HAS_MARCH_NATIVE)
      set( CMAKE_C_FLAGS             "${CMAKE_C_FLAGS} -march=native" )
   endif()
   set( CMAKE_C_FLAGS_DEBUG          "-g -fno-inline -O0 -Wextra -DDEBUG")
   set( CMAKE_C_FLAGS_RELEASE        "-O3 -funroll-loops -DNDEBUG" )
   set( CMAKE_C_FLAGS_MINSIZEREL     "${CMAKE_C_FLAGS_RELEASE}" )
   set( CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -Wextra -funroll-loops" )

# Suppress warnings about typeid() called with function as an argument. In this
# case, the function might not be called if the type can be deduced.
   set( CMAKE_CXX_FLAGS                "${CMAKE_C_FLAGS} -stdlib=libc++ -Wno-potentially-evaluated-expression" )
   if( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.8 )
     set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
   endif()
   set( CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG} -Woverloaded-virtual")
   set( CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}")
   set( CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_RELEASE}")
   set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}" )

   # Use C99 standard.
   set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

endif()

##---------------------------------------------------------------------------##
# Ensure cache values always match current selection
##---------------------------------------------------------------------------##
set( CMAKE_C_FLAGS                "${CMAKE_C_FLAGS}"                CACHE STRING "compiler flags" FORCE )
set( CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}"          CACHE STRING "compiler flags" FORCE )
set( CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}"        CACHE STRING "compiler flags" FORCE )
set( CMAKE_C_FLAGS_MINSIZEREL     "${CMAKE_C_FLAGS_MINSIZEREL}"     CACHE STRING "compiler flags" FORCE )
set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}" CACHE STRING "compiler flags" FORCE )

set( CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS}"                CACHE STRING "compiler flags" FORCE )
set( CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}"          CACHE STRING "compiler flags" FORCE )
set( CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}"        CACHE STRING "compiler flags" FORCE )
set( CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL}"     CACHE STRING "compiler flags" FORCE )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" CACHE STRING "compiler flags" FORCE )

# Toggle for C++11 support
if( OpenMP_C_FLAGS )
  toggle_compiler_flag( OPENMP_FOUND "${OpenMP_C_FLAGS}" "C;CXX;EXE_LINKER" "" )
endif()

#------------------------------------------------------------------------------#
# End config/unix-clang.cmake
#------------------------------------------------------------------------------#
