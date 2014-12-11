#pragma once

#include "opt.h"
#include "Optimizer.h"

namespace scone
{
	namespace opt
	{
		class OPT_API CmaOptimizer : public Optimizer, public Factoryable< Optimizer, CmaOptimizer >
		{
		public:
			CmaOptimizer();
			virtual ~CmaOptimizer();

			virtual void ProcessProperties( const PropNode& props ) override;
			virtual void Run( ObjectiveSP m_Objective ) override;

		private:
			ObjectiveSP m_Objective;

			int m_Mu;
			int m_Lambda;
			double m_Sigma;
			size_t max_generations;

			struct Impl;
			std::unique_ptr< Impl > m_pImpl;
			size_t num_elitists;
			int max_attempts;
		};
	}
}
