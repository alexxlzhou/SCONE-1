/*
** PieceWiseLinearFunction.cpp
**
** Copyright (C) 2013-2018 Thomas Geijtenbeek. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "PieceWiseLinearFunction.h"
#include "OpenSim/Common/PiecewiseLinearFunction.h"

namespace scone
{
	struct PieceWiseLinearFunction::Impl {
		OpenSim::PiecewiseLinearFunction m_osFunc;
	};

	PieceWiseLinearFunction::PieceWiseLinearFunction( const PropNode& props, Params& par ) :
	m_pImpl( new Impl ),
	control_point_y( props.get_child( "control_point_y" ) ),
	control_point_dt( props.get_child( "control_point_dt" ) )
	{
		INIT_PROP( props, control_points, size_t( 0 ) );
		INIT_PROP( props, flat_extrapolation, false );

		for ( int cpidx = 0; cpidx < control_points; ++cpidx )
		{
			Real xVal = 0.0;
			if ( cpidx > 0 )
			{
				double dt = par.get( stringf( "DT%d", cpidx - 1 ), control_point_dt );
				xVal = m_pImpl->m_osFunc.getX( cpidx - 1 ) + dt;
			}
			Real yVal = par.get( stringf( "Y%d", cpidx ), control_point_y );
			m_pImpl->m_osFunc.addPoint( xVal, yVal );
		}
	}

	PieceWiseLinearFunction::~PieceWiseLinearFunction()
	{
	}

	scone::Real PieceWiseLinearFunction::GetValue( Real x )
	{
		SimTK::Vector xval( 1 );
		xval[ 0 ] = flat_extrapolation ? std::min( x, m_pImpl->m_osFunc.getX( m_pImpl->m_osFunc.getNumberOfPoints() - 1) ) : x;

		return m_pImpl->m_osFunc.calcValue( xval );
	}

	String PieceWiseLinearFunction::GetSignature()
	{
		return stringf( "L%d", m_pImpl->m_osFunc.getSize() );
	}
}
