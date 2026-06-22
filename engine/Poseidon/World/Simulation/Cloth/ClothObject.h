#pragma once

#include <Poseidon/World/Entities/Infantry/Head.hpp>


namespace Poseidon
{
class ClothKnot
{
	public:
	Vector3 _pos,_vel,_norm;
};

template <class Type>
class Array2DCloth
{
	Temp<Type> _data;
	int _w,_h;

	public:
	void Dim( int w, int h )
	{
		_w=w, _h=h;
		_data.Realloc(w*h);
	}
	void Clear() {_w=0, _h=0, _data.Free();}
	int W() const {return _w;}
	int H() const {return _h;}
	const Type &Get( int x, int y ) const {return _data[y*_w+x];}
	Type &Set( int x, int y ) {return _data[y*_w+x];}

	const Type &operator ()( int x, int y ) const {return _data[y*_w+x];}
	Type &operator ()( int x, int y ) {return _data[y*_w+x];}

	const Type &operator []( int offset ) const {return _data[offset];}
	Type &operator []( int offset ) {return _data[offset];}

	int CoordOffset( int x, int y ) const {return x+y*_w;}
	int XFromOffset( int id ) const {return id%_w;}
	int YFromOffset( int id ) const {return id/_w;}
};

class ClothObject
{
private:

	Array2DCloth<ClothKnot> _knots;
	float _xMin,_yMin;
	float _xSize, _ySize;
	float _maxStep;

	float _stretchCoef;
	float _fricCoef;
	float _windCoef;
	float _gravCoef;

public:
	ClothObject();
	~ClothObject();

	void Init
	(
		const ParamEntry &config,
		Matrix4Val pos,
		float xMin, float yMin,
		float sizeX, float sizeY
	);
	void InitPos( Matrix4Val pos, Vector3Val vel );

	// get results
	Vector3Val GetPosition( float x, float y ) const;
	Vector3Val GetNormal( float x, float y ) const;

	float GetSizeX() const {return _xSize;}
	float GetSizeY() const {return _ySize;}
	float GetMinX() const {return _xMin;}
	float GetMinY() const {return _yMin;}

	void Simulate
	(
		Matrix4Val pos,
		Vector3Val velocity,
		float time, Vector3Par wind, Vector3Par inertia,
		SimulationImportance importance
	);
	void SetKnot
	(
		float x, float y,
		Vector3Par pos, Vector3Par vel, Vector3Par norm
	);

	bool IsConstraint(int x, int y);

};
}  // namespace Poseidon
