#include "stdafx.h"

#include "MuscleReflex.h"
#include "../sim/Muscle.h"
#include "../sim/Area.h"
#include "Tools.h"

//#define DEBUG_MUSCLE "vasti_r"

namespace scone
{
	namespace cs
	{
		MuscleReflex::MuscleReflex( const PropNode& props, opt::ParamSet& par, sim::Model& model, const sim::Area& area ) :
		Reflex( props, par, model, area ),
		m_Source( *FindByName( model.GetMuscles(), props.GetStr( "source", props.GetStr( "target" ) ) + GetSideName( area.side ) ) )
		{
			// init names
			String reflexname = GetReflexName( m_Target.GetName(), m_Source.GetName() );
			opt::ScopedParamSetPrefixer prefixer( par, reflexname + "." );

			INIT_PARAM_NAMED( props, par, length_gain, "KL", 0.0 );
			INIT_PARAM_NAMED( props, par, length_ofs, "L0", 1.0 );
			INIT_PARAM_NAMED( props, par, velocity_gain, "KV", 0.0 );
			INIT_PARAM_NAMED( props, par, force_gain, "KF", 0.0 );

			// create delayed sensors
			m_pForceSensor = &model.AcquireDelayedSensor< sim::MuscleForceSensor >( m_Source );
			m_pLengthSensor = &model.AcquireDelayedSensor< sim::MuscleLengthSensor >( m_Source );
			m_pVelocitySensor = &model.AcquireDelayedSensor< sim::MuscleVelocitySensor >( m_Source );
		}

		MuscleReflex::~MuscleReflex()
		{
		}

		void MuscleReflex::ComputeControls( double timestamp )
		{
			// add stretch reflex
			Real u_l = length_gain * std::max( 0.0, m_pLengthSensor->GetValue( delay ) - length_ofs );

			// add velocity reflex
			// TODO: should velocity gain be positive only?
			Real u_v = velocity_gain * std::max( 0.0, m_pVelocitySensor->GetValue( delay ) );

			// add force reflex
			Real u_f = force_gain * m_pForceSensor->GetValue( delay );

			m_Target.AddControlValue( u_l + u_v + u_f );

#ifdef DEBUG_MUSCLE
			if ( m_Target.GetName() == DEBUG_MUSCLE )
				log::TraceF( "u_l=%.3f u_v=%.3f u_f=%.3f", u_l, u_v, u_f );
#endif
		}
	}
}
