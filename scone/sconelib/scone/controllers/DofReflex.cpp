#include "DofReflex.h"
#include "scone/model/Dof.h"
#include "scone/model/Actuator.h"
#include "scone/model/Model.h"
#include "scone/model/SensorDelayAdapter.h"
#include "scone/model/Sensors.h"

//#define DEBUG_MUSCLE "glut_max_r"

namespace scone
{
	DofReflex::DofReflex( const PropNode& props, ParamSet& par, Model& model, const Locality& area ) :
	Reflex( props, par, model, area ),
	m_DelayedPos( model.AcquireDelayedSensor< DofPositionSensor >( *FindByName( model.GetDofs(), props.get< String >( "source" ) ) ) ),
	m_DelayedVel( model.AcquireDelayedSensor< DofVelocitySensor >( *FindByName( model.GetDofs(), props.get< String >( "source" ) ) ) ),
	m_DelayedRootPos( model.AcquireDelayedSensor< DofPositionSensor >( *FindByName( model.GetDofs(), "pelvis_tilt" ) ) ),
	m_DelayedRootVel( model.AcquireDelayedSensor< DofVelocitySensor >( *FindByName( model.GetDofs(), "pelvis_tilt" ) ) ),
	m_bUseRoot( m_DelayedRootPos.GetName() != m_DelayedPos.GetName() )
	{
		auto source = props.get< String >( "source" );
		String reflexname = GetReflexName( m_Target.GetName(), FindByName( model.GetDofs(), source )->GetName() );
		ScopedParamSetPrefixer prefixer( par, reflexname + "." );

		INIT_PARAM_NAMED( props, par, target_pos, "P0", 0.0 );
		INIT_PARAM_NAMED( props, par, target_vel, "V0", 0.0 );
		INIT_PARAM_NAMED( props, par, pos_gain, "KP", 0.0 );
		INIT_PARAM_NAMED( props, par, vel_gain, "KV", 0.0 );
		INIT_PARAM_NAMED( props, par, constant_u, "C0", 0.0 );
	}

	DofReflex::~DofReflex()
	{
	}

	void DofReflex::ComputeControls( double timestamp )
	{
		// TODO: Add world coordinate option to Body
		Real root_pos = m_DelayedRootPos.GetValue( delay );
		Real root_vel = m_DelayedRootVel.GetValue( delay );
		Real pos = m_DelayedPos.GetValue( delay );
		Real vel = m_DelayedVel.GetValue( delay );

		if ( m_bUseRoot )
		{
			pos += root_pos;
			vel += root_vel;
		}

		Real u_p = pos_gain * ( target_pos - pos );
		Real u_d = vel_gain * ( target_vel - vel );

		AddTargetControlValue( constant_u + u_p + u_d );

#ifdef DEBUG_MUSCLE
		if ( m_Target.GetName() == DEBUG_MUSCLE )
			log::TraceF( "pos=%.3f vel=%.3f root_pos=%.3f root_vel=%.3f u_p=%.3f u_d=%.3f", pos, vel, root_pos, root_vel, u_p, u_d );
#endif
	}
}