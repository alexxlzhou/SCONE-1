/*
** CmaPoolOptimizer.h
**
** Copyright (C) 2013-2018 Thomas Geijtenbeek. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#pragma once

#include "Optimizer.h"
#include "spot/optimizer_pool.h"
#include "xo/system/log_sink.h"

namespace scone
{
	/// Multiple CMA-ES optimizations than run in a prioritized fashion, based on their predicted fitness.
	class CmaPoolOptimizer : public Optimizer, public spot::optimizer_pool
	{
	public:
		CmaPoolOptimizer( const PropNode& pn );
		virtual ~CmaPoolOptimizer() {}

		virtual void Run() override;
		virtual void SetOutputMode( OutputMode m ) override;

	protected:
		std::vector< PropNode > props_;
		size_t optimizations_;
		long random_seed_;
	};

	class SCONE_API CmaPoolOptimizerReporter : public spot::reporter
	{
	public:
		virtual void on_start( const spot::optimizer& opt ) override;
		virtual void on_stop( const spot::optimizer& opt, const spot::stop_condition& s ) override;
	};
}
