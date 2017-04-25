#pragma once

#include <bitset>
#include "memory_tools.h"
#include "Side.h"

namespace scone
{
	class SCONE_API Locality
	{
	public:
		Locality( Side s = NoSide, bool m = false ) : side( s ), mirrored( m ) {}

		Side side;
		bool mirrored;
	};

	inline Locality MakeMirrored( Locality a ) { a.mirrored = !a.mirrored;	return a; }
}
