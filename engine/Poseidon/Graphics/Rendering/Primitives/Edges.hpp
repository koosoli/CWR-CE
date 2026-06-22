#pragma once

namespace Poseidon
{
class Shape;
class FVConnections;

} // namespace Poseidon
#include <Poseidon/Graphics/Rendering/Shape/ClipShape.hpp>
namespace Poseidon
{

class FaceEdges
{
	// this is like normal face array
	// but vertices have different meaning
	// they are indices to neighbourgh faces
	protected:
	FaceArray _data;
	AutoArray<Offset> _offsets; // face offsets
	bool _error; // error notification

	public:
	FaceEdges();

	const Poly &GetEdges( int i ) const {return _data[_offsets[i]];}
	const Poly &operator [] ( int i ) const {return _data[_offsets[i]];}
	int NFaces() const {return _offsets.Size();}

	void Build( Shape *shape, const FVConnections &con );
	void Build( Shape *shape );

	bool GetError() const {return _error;}
};

class ConvexComponent;

class ComponentEdges: public FaceEdges
{
	// information from FaceEdges parsed
	// so that indices point into Faces() member of ConvexComponent
	public:
	void Build( const FaceEdges &edges, const ConvexComponent &component );
};
} // namespace Poseidon

using Poseidon::ComponentEdges;
