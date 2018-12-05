/*
** StudioModel.cpp
**
** Copyright (C) 2013-2018 Thomas Geijtenbeek. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "StudioModel.h"

#include "scone/optimization/opt_tools.h"
#include "scone/core/StorageIo.h"
#include "scone/core/Profiler.h"
#include "scone/core/Factories.h"
#include "scone/core/system_tools.h"
#include "scone/core/Settings.h"
#include "scone/core/math.h"
#include "scone/model/Muscle.h"

#include "xo/time/timer.h"
#include "xo/filesystem/filesystem.h"
#include "simvis/color.h"
#include "xo/geometry/path_alg.h"
#include "StudioSettings.h"

namespace scone
{
	StudioModel::StudioModel( vis::scene& s, const path& file, bool force_evaluation ) :
	scene_( s ),
	specular_( GetStudioSetting < float > ( "viewer.specular" ) ),
	shininess_( GetStudioSetting< float >( "viewer.shininess" ) ),
	ambient_( GetStudioSetting< float >( "viewer.ambient" ) ),
	emissive_( GetStudioSetting< float >( "viewer.emissive" ) ),
	bone_mat( GetStudioSetting< vis::color >( "viewer.bone" ), specular_, shininess_, ambient_, emissive_ ),
	muscle_mat( GetStudioSetting< vis::color >( "viewer.muscle_0" ), specular_, shininess_, ambient_, emissive_ ),
	tendon_mat( GetStudioSetting< vis::color >( "viewer.tendon" ), specular_, shininess_, ambient_, emissive_ ),
	arrow_mat( GetStudioSetting< vis::color >( "viewer.force" ), specular_, shininess_, ambient_, emissive_ ),
	contact_mat( GetStudioSetting< vis::color >( "viewer.contact" ), specular_, shininess_, ambient_, emissive_ ),
	muscle_gradient( { { 0.0f, GetStudioSetting< vis::color >( "viewer.muscle_0" ) }, { 0.5f, GetStudioSetting< vis::color >( "viewer.muscle_50" ) }, { 1.0f, GetStudioSetting< vis::color >( "viewer.muscle_100" ) } } ),
	is_evaluating( false )
	{
		// TODO: don't reset this every time, perhaps keep view_flags outside StudioModel
		view_flags.set( ShowForces ).set( ShowMuscles ).set( ShowTendons ).set( ShowGeometry ).set( EnableShadows );

		// create the objective form par file or config file
		model_objective = CreateModelObjective( file );
		log::info( "Created objective ", model_objective->GetSignature(), " from ", file );

		SearchPoint par( model_objective->info() );
		if ( file.extension() == "par" )
		{
			auto result = par.import_values( file );
			log::info( "Read ", result.first, " of ", model_objective->info().dim(), " parameters, skipped ", result.second, " from ", file.filename() );
		}
		model_ = model_objective->CreateModelFromParams( par );

		// accept filename and clear data
		filename_ = file;

		// load results if the file is an sto
		if ( file.extension() == "sto" && !force_evaluation )
		{
			xo::timer t;
			log::info( "Reading ", file );
			ReadStorageSto( data, file );
			InitStateDataIndices();
			log::trace( "Read ", file, " in ", t.seconds(), " seconds" );
		}
		else
		{
			// start evaluation
			is_evaluating = true;
			model_->SetStoreData( true );

			model_->SetSimulationEndTime( model_objective->GetDuration() );
			log::info( "Evaluating ", filename_ );
			EvaluateTo( 0 ); // evaluate one step so we can init vis
		}

		InitVis( s );
		UpdateVis( 0 );
	}

	StudioModel::~StudioModel()
	{}

	void StudioModel::InitStateDataIndices()
	{
		// setup state_data_index (lazy init)
		SCONE_ASSERT( state_data_index.empty() );
		model_state = model_->GetState();
		state_data_index.resize( model_state.GetSize() );
		for ( size_t state_idx = 0; state_idx < state_data_index.size(); state_idx++ )
		{
			auto data_idx = data.GetChannelIndex( model_state.GetName( state_idx ) );
			SCONE_ASSERT_MSG( data_idx != NoIndex, "Could not find state channel " + model_state.GetName( state_idx ) );
			state_data_index[ state_idx ] = data_idx;
		}
	}

	void StudioModel::InitVis( vis::scene& scone_scene )
	{
		scone_scene.attach( root );

		xo::timer t;
		for ( auto& body : model_->GetBodies() )
		{
			bodies.push_back( root.add_group() );
			body_axes.push_back( bodies.back().add_axes( vis::vec3f( 0.1, 0.1, 0.1 ), 0.5f ) );

			auto geoms = body->GetDisplayGeometries();
			for ( auto geom : geoms )
			{
				//log::trace( "Loading geometry for body ", body->GetName(), ": ", geom_file );
				try
				{
					auto geom_file = xo::try_find_file( { geom.filename, path( "./geometry" ) / geom.filename, scone::GetFolder( scone::SCONE_GEOMETRY_FOLDER ) / geom.filename } );
					if ( geom_file )
					{
						body_meshes.push_back( bodies.back().add_mesh( *geom_file ) );
						body_meshes.back().set_material( bone_mat );
						body_meshes.back().pos_ori( geom.pos, geom.ori );
						body_meshes.back().scale( geom.scale );
					}
					else log::warning( "Could not find ", geom.filename );
				}
				catch ( std::exception& e )
				{
					log::warning( "Could not load ", geom.filename, ": ", e.what() );
				}
			}
		}
		//log::debug( "Meshes loaded in ", t.seconds(), " seconds" );

		for ( auto& cg : model_->GetContactGeometries() )
		{
			auto idx = FindIndexByName( model_->GetBodies(), cg.m_Body.GetName() );
			auto& parent = idx != NoIndex ? bodies[ idx ] : root;
			contact_geoms.push_back( parent.add_sphere( cg.m_Scale.x, GetStudioSetting< vis::color >( "viewer.contact" ), 0.75f ) );
			contact_geoms.back().set_material( contact_mat );
			contact_geoms.back().pos( cg.m_Pos );
		}

		for ( auto& muscle : model_->GetMuscles() )
		{
			// add path
			MuscleVis mv;
			mv.ten1 = root.add< vis::trail >( 1, GetStudioSetting<float>( "viewer.tendon_width" ), vis::make_yellow(), 0.3f );
			mv.ten2 = root.add< vis::trail >( 1, GetStudioSetting<float>( "viewer.tendon_width" ), vis::make_yellow(), 0.3f );
			mv.ten1.set_material( tendon_mat );
			mv.ten2.set_material( tendon_mat );

			mv.ce = root.add< vis::trail >( 1, GetStudioSetting<float>( "viewer.muscle_width" ), vis::make_red(), 0.5f );
			mv.mat = muscle_mat.clone();
			mv.ce.set_material( mv.mat );
			muscles.push_back( mv );
		}

		ApplyViewSettings( view_flags );
	}

	void StudioModel::UpdateVis( TimeInSeconds time )
	{
		SCONE_PROFILE_FUNCTION;

		index_t force_count = 0;

		if ( !is_evaluating )
		{
			// update model state from data
			SCONE_ASSERT( !state_data_index.empty() );
			for ( index_t i = 0; i < model_state.GetSize(); ++i )
				model_state[ i ] = data.GetInterpolatedValue( time, state_data_index[ i ] );
			model_->SetState( model_state, time );
		}

		// update com
		//com.pos( model->GetComPos() );

		// update bodies
		auto& model_bodies = model_->GetBodies();
		for ( index_t i = 0; i < model_bodies.size(); ++i )
		{
			auto& b = model_bodies[ i ];
			vis::transformf trans( b->GetOriginPos(), b->GetOrientation() );
			bodies[ i ].transform( trans );

			// external forces / moments
			auto f = b->GetExternalForce();
			if ( !f.is_null() )
				UpdateForceVis( force_count++, b->GetPosOfPointOnBody( b->GetExternalForcePoint() ), f );
			auto m = b->GetExternalMoment();
			if ( !m.is_null() )
				UpdateForceVis( force_count++, b->GetComPos(), m );
		}

		// update muscle paths
		auto &model_muscles = model_->GetMuscles();
		for ( index_t i = 0; i < model_muscles.size(); ++i )
			UpdateMuscleVis( *model_muscles[ i ], muscles[ i ] );

		// update ground reaction forces on legs
		for ( index_t i = 0; i < model_->GetLegCount(); ++i )
		{
			Vec3 force, moment, cop;
			model_->GetLeg( i ).GetContactForceMomentCop( force, moment, cop );

			if ( force.squared_length() > REAL_WIDE_EPSILON && view_flags.get< ShowForces >() )
				UpdateForceVis( force_count++, cop, force );
		}

		if ( force_count < forces.size() )
			forces.resize( force_count );
	}

	void StudioModel::UpdateForceVis( index_t force_idx, Vec3 cop, Vec3 force )
	{
		while ( forces.size() <= force_idx )
		{
			forces.push_back( root.add_arrow( 0.01f, 0.02f, vis::make_yellow(), 0.3f ) );
			forces.back().set_material( arrow_mat );
			forces.back().show( view_flags.get< ShowForces >() );
		}
		forces[ force_idx ].pos( cop, cop + 0.001 * force );
	}

	void StudioModel::UpdateMuscleVis( const class Muscle& mus, MuscleVis& vis )
	{
		auto mp = mus.GetMusclePath();
		auto len = mus.GetLength();
		auto tlen = mus.GetTendonLength() / 2;
		auto a = mus.GetActivation();
		auto p = mus.GetMusclePath();

		vis::color c = muscle_gradient( float( a ) );
		vis.mat.diffuse( c );
		vis.mat.emissive( c );

		if ( view_flags.get<ShowTendons>() )
		{
			auto i1 = insert_path_point( p, tlen );
			auto i2 = insert_path_point( p, len - tlen );
			vis.ten1.set_points( p.begin(), p.begin() + i1 + 1 );
			vis.ce.set_points( p.begin() + i1, p.begin() + i2 + 1 );
			vis.ten2.set_points( p.begin() + i2, p.end() );
		}
		else
		{
			vis.ce.set_points( p.begin(), p.end() );
		}
	}

	void StudioModel::EvaluateTo( TimeInSeconds t )
	{
		SCONE_ASSERT( IsEvaluating() );
		try
		{
			model_objective->AdvanceSimulationTo( *model_, t );
			if ( model_->GetTerminationRequest() || t >= model_->GetSimulationEndTime() )
				FinalizeEvaluation( true );
		}
		catch ( std::exception& e )
		{
			log::error( "Error evaluating model at time ", model_->GetTime(), ": ", e.what() );
			FinalizeEvaluation( false );
		}
	}

	void StudioModel::FinalizeEvaluation( bool output_results )
	{
		// copy data and init data
		data = model_->GetData();
		if ( !data.IsEmpty() )
			InitStateDataIndices();

		if ( output_results )
		{
			auto fitness = model_objective->GetResult( *model_ );
			log::info( "fitness = ", fitness );
			PropNode results;
			results.push_back( "result", model_objective->GetReport( *model_ ) );
			model_->WriteResults( filename_ );

			log::info( "Results written to ", path( filename_ ).replace_extension( "sto" ) );
			log::info( results );
		}

		// reset this stuff
		is_evaluating = false;
	}

	void StudioModel::ApplyViewSettings( const ViewFlags& flags )
	{
		view_flags = flags;
		for ( auto& f : forces )
			f.show( view_flags.get< ShowForces >() );

		for ( auto& m : muscles )
		{
			m.ce.show( view_flags.get< ShowMuscles >() );
			m.ten1.show( view_flags.get< ShowMuscles >() && view_flags.get< ShowTendons >() );
			m.ten2.show( view_flags.get< ShowMuscles >() && view_flags.get< ShowTendons >() );
		}

		for ( auto& e : body_meshes )
			e.show( view_flags.get< ShowGeometry >() );

		for ( auto& e : body_axes )
			e.show( view_flags.get< ShowAxes >() );

		for ( auto& e : contact_geoms )
			e.show( view_flags.get< ShowContactGeom >() );

		if ( model_ )
			UpdateVis( model_->GetTime() );
	}
}
