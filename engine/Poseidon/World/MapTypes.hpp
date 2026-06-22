#pragma once


#include <Poseidon/Foundation/Types/EnumDecl.hpp>
namespace Poseidon
{
DEFINE_ENUM_BEG(MapType)
	MapTree,MapSmallTree,MapBush,
	MapBuilding,MapHouse,
	MapForestBorder,MapForestTriangle,MapForestSquare,
	MapChurch,MapChapel,MapCross,
	MapRock,
	MapBunker, MapFortress,
	MapFountain,
	MapViewTower, MapLighthouse, MapQuay,
	MapFuelstation, MapHospital,
	MapFence, MapWall,
	MapHide,
	MapBusStop,
	NMapTypes
DEFINE_ENUM_END(MapType)
} // namespace Poseidon
