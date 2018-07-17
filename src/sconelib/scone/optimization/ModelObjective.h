#pragma once

#include "../optimization/Objective.h"
#include "../model/Model.h"

namespace scone
{
	class SCONE_API ModelObjective : public Objective
	{
	public:
		ModelObjective( const PropNode& props );
		virtual ~ModelObjective() {}

		virtual fitness_t evaluate( const ParamInstance& point ) const override;

		virtual fitness_t EvaluateModel( Model& m ) const;
		virtual void AdvanceModel( Model& m, TimeInSeconds t ) const = 0;

		virtual fitness_t GetResult( Model& m ) const = 0;
		virtual PropNode GetReport( Model& m ) const = 0;

		virtual TimeInSeconds GetDuration() const = 0;
		virtual ModelUP CreateModelFromParams( Params& point ) const;
		ModelUP CreateModelFromParFile( const path& parfile ) const;
		
	protected:
		PropNode m_ModelPropsCopy;
	};
}