#pragma once


#define GEOMETRY_SPEC ( 1e13 )

#define SPEC_LOD 1e15
#define MEMORY_SPEC ( SPEC_LOD*1 )
#define LANDCONTACT_SPEC ( SPEC_LOD*2 )
#define ROADWAY_SPEC ( SPEC_LOD*3 )
#define PATHS_SPEC ( SPEC_LOD*4 )
#define HITPOINTS_SPEC ( SPEC_LOD*5 )
#define VIEW_GEOM_SPEC ( SPEC_LOD*6 )
#define FIRE_GEOM_SPEC ( SPEC_LOD*7 )

#define VIEW_CARGO_GEOM_SPEC ( SPEC_LOD*8 )
//#define FIRE_CARGO_GEOM_SPEC ( SPEC_LOD*9 )
#define VIEW_COMMANDER_SPEC ( SPEC_LOD*10 )
#define VIEW_COMMANDER_GEOM_SPEC ( SPEC_LOD*11 )
//#define FIRE_COMMANDER_GEOM_SPEC ( SPEC_LOD*12 )
#define VIEW_PILOT_GEOM_SPEC ( SPEC_LOD*13 )
//#define FIRE_PILOT_GEOM_SPEC ( SPEC_LOD*14 )
#define VIEW_GUNNER_GEOM_SPEC ( SPEC_LOD*15 )
#define FIRE_GUNNER_GEOM_SPEC ( SPEC_LOD*16 )

#define VIEW_COMMANDER VIEW_COMMANDER_SPEC
#define VIEW_GUNNER 1000
#define VIEW_PILOT 1100
#define VIEW_CARGO 1200


namespace Poseidon
{
enum MaterialSection
{
	// predefined materials
	MSShining=200,
	MSInShadow,
	MSHalfLighted,
	MSFullLighted,
	MSInside,
	MSInShadow75,
	MSInside75,
	MSInShadow50,
	MSInside50,
};

} // namespace Poseidon
