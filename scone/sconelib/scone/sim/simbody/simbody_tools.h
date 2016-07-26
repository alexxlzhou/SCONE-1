#pragma once

#include "sim_simbody.h"

#include <OpenSim/OpenSim.h>

#include "scone/core/math.h"
#include "scone/core/Vec3.h"

namespace scone
{
	namespace sim
	{
		inline Vec3 ToVec3( const SimTK::Vec3& vec ) { return Vec3( vec[0], vec[1], vec[2] ); }
		inline Vec3f ToVec3f( const SimTK::Vec3& vec ) { return Vec3f( float( vec[0] ), float( vec[1] ), float( vec[2] ) ); }
		inline Vec3d ToVec3d( const SimTK::Vec3& vec ) { return Vec3d( vec[0], vec[1], vec[2] ); }
	}
}