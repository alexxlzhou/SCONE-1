#include "QSconeStorageDataModel.h"

QSconeStorageDataModel::QSconeStorageDataModel( const scone::Storage<>* s ) : storage( s )
{

}

void QSconeStorageDataModel::setStorage( const scone::Storage<>* s )
{
	storage = s;
}

size_t QSconeStorageDataModel::getVariableCount() const
{
	if ( storage ) return storage->GetChannelCount();
	else return 0;
}

QString QSconeStorageDataModel::getLabel( int idx ) const
{
	SCONE_ASSERT( storage );
	return QString( storage->GetLables()[ idx ].c_str() );
}

double QSconeStorageDataModel::getValue( int idx, double time ) const
{
	SCONE_ASSERT( storage ); return storage->GetInterpolatedFrame( time ).value( idx );
}

std::vector< std::pair< float, float > > QSconeStorageDataModel::getSeries( int idx, double min_interval ) const
{
	SCONE_ASSERT( storage );
	std::vector< std::pair< float, float > > series;
	series.reserve( storage->GetFrameCount() ); // this may be a little much, but ensures no reallocation
	double last_time = getTimeStart() - 2 * min_interval;
	for ( size_t i = 0; i < storage->GetFrameCount(); ++i )
	{
		auto& f = storage->GetFrame( i );
		if ( f.GetTime() - last_time >= min_interval )
		{
			series.emplace_back( static_cast< float >( f.GetTime() ), static_cast< float >( f[ idx ] ) );
			last_time = f.GetTime();
		}
	}
	return series;
}

double QSconeStorageDataModel::getTimeFinish() const
{
	return storage ? storage->Back().GetTime() : 0.0;
}

double QSconeStorageDataModel::getTimeStart() const
{
	return 0.0;
}
