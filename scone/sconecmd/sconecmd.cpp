#include "scone/core/Log.h"
#include "scone/optimization/opt_tools.h"
#include "scone/controllers/cs_tools.h"
#include "scone/model/simbody/sim_simbody.h"
#include "flut/system_tools.hpp"
#include <boost/filesystem.hpp>
#include <tclap/CmdLine.h>
#include <thread>
#include "flut/system/log_sink.hpp"
#include "flut/prop_node_tools.hpp"
#include "spot/optimization_pool.h"
#include <xutility>

using namespace scone;
using namespace std;

int main(int argc, char* argv[])
{
	flut::log::stream_sink console_sink( flut::log::info_level, std::cout );

	try
	{
		TCLAP::CmdLine cmd( "SCONE Command Line Utility", ' ', "0.1", true );
		TCLAP::ValueArg< string > optArg( "o", "optimize", "Scenario to optimize", true, "", "Scenario file" );
		TCLAP::ValueArg< string > resArg( "e", "evaluate", "Evaluate result from an optimization", false, "", "Result file" );
		TCLAP::ValueArg< int > logArg( "l", "log", "Set the log level", false, 1, "1-7", cmd );
		TCLAP::ValueArg< int > multiArg( "p", "pool", "The number of optimizations to run in parallel", false, 1, "1-99", cmd );
		TCLAP::ValueArg< int > promiseWindowArg( "w", "window", "Window size to determine most promising optimization of pool", false, 400, "2-...", cmd );
		TCLAP::SwitchArg statusOutput( "s", "status", "Output status updates for use in external tools", cmd, false );
		TCLAP::SwitchArg quietOutput( "q", "quiet", "Do not output simulation progress", cmd, false );
		TCLAP::UnlabeledMultiArg< string > propArg( "property", "Override specific scenario property, using <key>=<value>", false, "<key>=<value>", cmd, true );
		cmd.xorAdd( optArg, resArg );
		cmd.parse( argc, argv );

		if ( optArg.isSet() )
		{
			// set log level
			console_sink.set_log_level( flut::log::level( logArg.isSet() ? log::Level( logArg.getValue() ) : log::InfoLevel ) );
			auto scenario_file = path( optArg.getValue() );

			// load properties
			PropNode props = flut::load_file_with_include( scenario_file, "INCLUDE" );

			// apply command line settings (parameter 2 and further)
			for ( auto kvstring : propArg )
			{
				auto kvp = flut::make_key_value_str( kvstring );
				props.set_delimited( kvp.first, kvp.second, '.' );
			}

			// create optimizer
			if ( multiArg.isSet() )
			{
				// pool optimization, REQUIRES scone::Optimizer to be derived from spot::optimizer
				FLUT_NOT_IMPLEMENTED;
				spot::optimization_pool op( promiseWindowArg.getValue() );
				for ( int i = 0; i < multiArg.getValue(); ++i )
				{
					OptimizerUP o = PrepareOptimization( props, scenario_file );
					o->SetConsoleOutput( !quietOutput.getValue() );
					o->SetStatusOutput( statusOutput.getValue() );
					if ( o->GetStatusOutput() ) o->OutputStatus( "scenario", optArg.getValue() );
					// TODO: op.push_back( std::move( o ) );
				}
				op.run();
			}
			else
			{
				OptimizerUP o = PrepareOptimization( props, scenario_file );
				o->SetConsoleOutput( !quietOutput.getValue() );
				o->SetStatusOutput( statusOutput.getValue() );
				if ( o->GetStatusOutput() ) o->OutputStatus( "scenario", optArg.getValue() );
				o->Run();
			}
		}
		else if ( resArg.isSet() )
		{
			// set log level
			console_sink.set_log_level( flut::log::level( logArg.isSet() ? log::Level( logArg.getValue() ) : log::TraceLevel ) );
			SimulateObjective( path( resArg.getValue() ) );
		}
		else SCONE_THROW( "Unexpected error parsing program arguments" ); // This should never happen
	}
	catch (std::exception& e)
	{
		log::Critical( e.what() );
		cout << "*error=" << e.what() << endl;
		cout.flush();

		// sleep some time for the error message to sing in...
		std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
	}
	catch (TCLAP::ExitException& e )
	{
		return e.getExitStatus();
	}

	return 0;
}
