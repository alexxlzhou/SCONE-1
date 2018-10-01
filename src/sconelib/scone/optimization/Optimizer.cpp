/*
** Optimizer.cpp
**
** Copyright (C) 2013-2018 Thomas Geijtenbeek. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "Optimizer.h"

#include "scone/core/Log.h"
#include "scone/core/system_tools.h"
#include "scone/core/string_tools.h"
#include "scone/core/Factories.h"
#include "scone/core/math.h"
#include "scone/optimization/Objective.h"

#include "xo/filesystem/filesystem.h"

#if defined(_MSC_VER)
#	define NOMINMAX
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif
#include "xo/system/system_tools.h"

namespace scone
{
	Optimizer::Optimizer( const PropNode& props ) :
	HasSignature( props ),
	max_threads( 1 ),
	thread_priority( (int)xo::thread_priority::lowest ),
	m_ObjectiveProps( props.get_child( "Objective" ) ),
	m_Objective( CreateObjective( m_ObjectiveProps ) ),
	m_BestFitness( m_Objective->info().worst_fitness() ),
	output_mode_( no_output ),
	m_LastFileOutputGen( 0 )
	{
		INIT_PROP( props, output_root, GetFolder( SCONE_RESULTS_FOLDER ) );
		INIT_PROP( props, log_level_, xo::log::info_level );

		INIT_PROP( props, max_threads, size_t( 32 ) );
		INIT_PROP( props, thread_priority, (int)xo::thread_priority::lowest );
		INIT_PROP( props, show_optimization_time, false );

		INIT_PROP( props, init_file, path( "" ) );
		INIT_PROP( props, use_init_file, true );
		INIT_PROP( props, init_file_std_factor, 1.0 );
		INIT_PROP( props, init_file_std_offset, 0.0 );
		INIT_PROP( props, use_init_file_std, true );

		INIT_PROP( props, output_objective_result_files, false );
		INIT_PROP( props, min_improvement_for_file_output, 0.05 );
		INIT_PROP( props, max_generations_without_file_output, 1000 );

		INIT_PROP( props, max_generations, 10000 );
		INIT_PROP( props, min_progress, 1e-6 );
		INIT_PROP( props, min_progress_samples, 200 );

		// initialize parameters from file
		if ( use_init_file && !init_file.empty() )
		{
			auto result = GetObjective().info().import_mean_std( init_file, use_init_file_std, init_file_std_factor, init_file_std_offset );
			log::info( "Imported ", result.first, ", skipped ", result.second, " parameters from ", init_file );
		}
	}

	Optimizer::~Optimizer()
	{}

	const path& Optimizer::GetOutputFolder() const
	{
		SCONE_ASSERT( !output_folder_.empty() );
		return output_folder_;
	}

	scone::String Optimizer::GetClassSignature() const
	{
		String s = GetObjective().GetSignature();
		if ( use_init_file && !init_file.empty() )
			s += ".I";

		return s;
	}

	void Optimizer::CreateOutputFolder( const PropNode& props )
	{
		SCONE_ASSERT( output_folder_.empty() );

		output_folder_ = xo::create_unique_folder( output_root / GetSignature() );
		id_ = output_folder_.filename().string();

		// create log sink if enabled
		if ( log_level_ < xo::log::never_log_level )
			log_sink_ = std::make_unique< xo::log::file_sink >( log_level_, GetOutputFolder() / "optimization.log" );

		// prepare output folder, and initialize
		auto outdir = GetOutputFolder();
		PropNode pn;
		pn[ "Optimizer" ] = props;
		xo::save_file( pn, GetOutputFolder() / "config.scone" );
		if ( use_init_file && !init_file.empty() )
			xo::copy_file( init_file, GetOutputFolder() / init_file.filename(), true );

		// copy all objective resources to output folder
		for ( auto& f : GetObjective().GetExternalResources() )
			xo::copy_file( f, outdir / f.filename(), true );
	}

	void Optimizer::ManageFileOutput( double fitness, const std::vector< path >& files ) const
	{
		m_OutputFiles.push_back( std::make_pair( fitness, files ) );
		if ( m_OutputFiles.size() >= 3 )
		{
			// see if we should delete the second last file
			auto testIt = m_OutputFiles.end() - 2;
			double imp1 = testIt->first / ( testIt - 1 )->first;
			double imp2 = ( testIt + 1 )->first / testIt->first;
			if ( IsMinimizing() ) { imp1 = 1.0 / imp1; imp2 = 1.0 / imp2; }

			if ( imp1 < min_improvement_for_file_output && imp2 < min_improvement_for_file_output )
			{
				// delete the file(s)
				bool ok = true;
				for ( auto& file : testIt->second )
					ok &= xo::remove( file );

				if ( ok )
					m_OutputFiles.erase( testIt );
			}
		}
	}

	void Optimizer::SetThreadPriority( int priority )
	{
#ifdef _MSC_VER
		::SetThreadPriority( ::GetCurrentThread(), priority );
#elif __APPLE__
		// TODO setschedprio unavailable; maybe use getschedparam?
#else
		pthread_setschedprio( pthread_self(), priority );
#endif
	}
}
