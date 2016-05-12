#pragma once

#include "cs.h"

#include "scone/sim/sim.h"
#include "scone/core/InitFromPropNode.h"
#include "scone/sim/Controller.h"
#include "scone/core/HasName.h"

namespace scone
{
	namespace cs
	{
		class SCONE_API Measure : public sim::Controller, public HasName
		{
		public:
			Measure( const PropNode& props, opt::ParamSet& par, sim::Model& model, const sim::Area& area );
			virtual ~Measure() { };

			virtual double GetResult( sim::Model& model ) = 0;
			PropNode& GetReport() { return report; }
			const PropNode& GetReport() const { return report; }
			virtual bool IsActive( const sim::Model& model, TimeInSeconds timestamp ) const { return timestamp >= start_time; }

			virtual const String& GetName() const override { return name; }
			Real GetWeight() { return weight; }
			Real GetThreshold() { return threshold; }
			Real GetOffset() { return offset; }

		private:
			TimeInSeconds start_time;
			PropNode report;

			String name;
			Real weight;
			Real threshold;
			Real offset;
		};
	}
}
