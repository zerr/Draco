//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   diagnostics/test/tstSwap.cc
 * \author Kent Budge
 * \date   Wed Apr 28 09:31:51 2010
 * \brief  Test runtime_error function.
 * \note   Copyright (C) 2016-2018 Los Alamos National Security, LLC.
 *         All rights reserved.
 */
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//

#include "../runtime_check.hh"
#include "c4/ParallelUnitTest.hh"
#include "c4/swap.hh"
#include "ds++/Release.hh"
#include <cmath>

using namespace std;
using namespace rtt_dsxx;
using namespace rtt_c4;
using namespace rtt_diagnostics;

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

void tstruntime_error(UnitTest &ut) {
  // Test passes
  try {
    runtime_check(true, "condition successful");
    ut.passes("check true on all processors");
  } catch (...) {
    ut.failure("check true on all processors");
  }

  try {
    runtime_check(false, "condition fails on all");
    ut.failure("check false on all processors");
  } catch (std::runtime_error &error) {
    ut.passes("check false on all processors throws on all as it should.");
  }

  try {
    runtime_check(rtt_c4::node() != 0, "condition fails on one processor");
    ut.failure("check false on one processors");
  } catch (std::runtime_error &error) {
    ut.passes("check false on one processor throws on all as it should.");
  }
}

//---------------------------------------------------------------------------//

int main(int argc, char *argv[]) {
  rtt_c4::ParallelUnitTest ut(argc, argv, release);
  try {
    tstruntime_error(ut);
  }
  UT_EPILOG(ut);
}

//---------------------------------------------------------------------------//
// end of tstSwap.cc
//---------------------------------------------------------------------------//
