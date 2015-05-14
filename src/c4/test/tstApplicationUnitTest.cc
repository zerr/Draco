//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   c4/test/tstApplicationUnitTest.cc
 * \author Kelly Thompson
 * \date   Tue Jun  6 15:03:08 2006
 * \brief  Test the Draco class ApplicationUnitTest
 * \note   Copyright (C) 2006-2015 Los Alamos National Security, LLC.
 *         All rights reserved.
 */
//---------------------------------------------------------------------------//
// $Id$
//---------------------------------------------------------------------------//

#include "c4/ApplicationUnitTest.hh"
#include "ds++/Release.hh"
#include "ds++/path.hh"
#include "ds++/to_string.hh"
#include <fstream>

using namespace std;
using namespace rtt_c4;

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

void tstOne( ApplicationUnitTest &unitTest, string const & appPath )
{
    string const extraArg( "hello" );
    unitTest.addCommandLineArgument( extraArg );

    {
        // This call should report failure since we have listOfArgs.size() = 1
        // and numPasses=0.
        if( unitTest.allTestsPass() )
            unitTest.failure("Didn't expect all tests to pass.");
        else
            unitTest.passes("Some tests did not pass (as expected).");
    }

    cout << ">>> Executing unitTest.runTests()..." << endl;
    unitTest.runTests();

    //! \bug Consider using Boost or other 3rd party library to aid with
    // file path manipulation, including finding the cwd.

    string const logFilename( unitTest.logFileName() );
    std::ostringstream msg;
    msg << appPath << "phw_hello-" << unitTest.nodes() <<".out";
    string const expLogFilename( msg.str() );
    if( expLogFilename == logFilename )
        unitTest.passes( "Found expected log filename." );
    else
    {
        ostringstream msg2;
        msg2 << "Did not find expected log filename.  Looking for \""
             << expLogFilename << "\" but found \"" << logFilename
             << "\" instead.";
        unitTest.failure( msg2.str() );
    }
    cout << endl;

    std::cout << "Should we report timings? ";
    if( unitTest.reportTimingsI() ) std::cout << "yes";
    else std::cout << "no";
    std::cout << std::endl;

    return;
}

//---------------------------------------------------------------------------//

void tstTwo( ApplicationUnitTest &unitTest )
{
    // This test is designed to fail.

    std::string const extraArg;
    std::cout << ">>> Executing unitTest.runTest( extraArg )..." << std::endl;
    if( unitTest.runTest( extraArg ) )
        unitTest.passes("Successfully ran phw.");
    else
        unitTest.failure("Found problems when running phw.");  // expected path.
    if( unitTest.allTestsPass() )
        unitTest.failure("Didn't expect all tests to pass.");
    else
        unitTest.passes("Some tests failed as expected.");

    // Kill fail flag (we expected this failure).
    unitTest.reset();
    // We need at least one pass.
    unitTest.passes("Done with tstTwo.");
    if( unitTest.allTestsPass() )
        unitTest.passes("All tests pass.");
    else
        unitTest.failure("Some tests failed.");
    return;
}

//---------------------------------------------------------------------------//

void tstThree( ApplicationUnitTest &unitTest)
{
    unitTest.setNodes("1");
    if (unitTest.runTest("hello"))
    {
        unitTest.passes("Successfully ran phw with overriding proc count.");

        ostringstream data;
        {
            // open and parse log file
            ifstream file( unitTest.logFileName().c_str() );
            Check( file );
            data << file.rdbuf();
        }

        // Check expected word counts.

        bool verbose(false);
        std::map< std::string, unsigned > word_count(
            ApplicationUnitTest::get_word_count( data, verbose ) );

        if( word_count[ string("world!") ] == 1 )
        {
            unitTest.passes("Found single occurance of \"world!\"");
        }
        else
        {
            unitTest.failure("Did NOT find single occurance of \"world!\" (count is " +
                             rtt_dsxx::to_string(word_count[string("world!")]) + ')');
            // print the word_list
            for( auto it = word_count.begin(); it != word_count.end(); ++it)
                cout << it->first << ": " << it->second << endl;
        }
    }
    else
    {
        unitTest.failure("Did NOT successfully run phw with overriding proc count.");
    }
}

//---------------------------------------------------------------------------//

void tstTwoCheck( ApplicationUnitTest &unitTest, std::ostringstream & msg )
{
    using rtt_dsxx::UnitTest;

    std::map<string,unsigned> word_list( UnitTest::get_word_count( msg ) );

    // Check the list of occurances against the expected values
    if( word_list[ string("Test") ] == 5 )
        unitTest.passes("Found 5 occurances of \"Test\"");
    if( word_list[ string("failed") ] != 3 )
        unitTest.failure("Did not find 3 occurances of failure.");
    if( word_list[ string("passed") ] == 2 )
        unitTest.passes("Found 2 occurances of \"working\"");

    return;
}

//---------------------------------------------------------------------------//
int main(int argc, char *argv[])
{
    try
    {
        // build the application path + name
        string const appPath( rtt_dsxx::getFilenameComponent( argv[0], rtt_dsxx::FC_PATH ) );
        string const appName( appPath + string("phw") );

        // Test ctor for ApplicationUnitTest
        ApplicationUnitTest ut( argc, argv, rtt_dsxx::release, appName );
        tstOne( ut, appPath );

        // Silent version.
        std::ostringstream messages;
        ApplicationUnitTest sut(
            argc, argv, rtt_dsxx::release, appName, std::list<std::string>(), messages );
        tstTwo(sut);
        tstTwoCheck( ut, messages );

        // Overriding processor count. This should probably be run last since
        // it modifies ut.
        tstThree(ut);

        ut.status();
    }
    catch( rtt_dsxx::assertion &err )
    {
        std::string msg = err.what();
        if( msg != std::string( "Success" ) )
        { cout << "ERROR: While testing " << argv[0] << ", "
               << err.what() << endl;
            return 1;
        }
        return 0;
    }
    catch (exception &err)
    {
        cout << "ERROR: While testing " << argv[0] << ", "
             << err.what() << endl;
        return 1;
    }
    catch( ... )
    {
        cout << "ERROR: While testing " << argv[0] << ", "
             << "An unknown exception was thrown" << endl;
        return 1;
    }
    return 0;
}

//---------------------------------------------------------------------------//
// end of tstApplicationUnitTest.cc.cc
//---------------------------------------------------------------------------//
