#pragma once

#include "sim.h"
#include "../core/Propertyable.h"
#include <functional>
#include "../opt/ParamSet.h"

namespace scone
{
	namespace sim
	{
		class SCONE_SIM_API Controller : public Propertyable, public opt::Parameterizable
		{
		public:
			Controller( const PropNode& props );
			virtual ~Controller();

			virtual void InitFromModel( sim::Model& model ) { };
			virtual bool UpdateControls( sim::Model& model, double timestamp ) = 0;
		};
	}
}
