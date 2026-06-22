#pragma once

#include <Poseidon/Graphics/Rendering/Colors.hpp>
#include <Poseidon/Foundation/Types/RemoveLinks.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon::Dev
{

class OnPaintContext;

class DebugWindow: public RemoveLinks
{
public:
	DebugWindow( const char *title );
	~DebugWindow() override;

	virtual void OnPaint( const OnPaintContext &dc ) = 0;
	virtual void Update();
	virtual void Close();
	virtual bool IsOpen();
};

void WaitForClose(DebugWindow *win);

typedef short DebugPixel;


class DebugMemWindow: public DebugWindow
{
	typedef DebugWindow base;

private:
	Temp<DebugPixel> _data;
	int _w,_h;

public:
	DebugMemWindow( const char *title, int w, int h );
	~DebugMemWindow() override;

	void OnPaint( const OnPaintContext &dc ) override;

	DebugPixel Get( int x, int y ) const;
	DebugPixel &Set( int x, int y );

	DebugPixel &operator () ( int x, int y ) {return Set(x,y);}
	DebugPixel operator () ( int x, int y ) const {return Get(x,y);}

	void Plot( int x, int y, DebugPixel color );
	void Line( int xb, int yb, int xe, int ye, DebugPixel color );

	static DebugPixel DColorGray( float val )
	{
		int gray=toIntFloor(val*31);
		saturate(gray,0,31);
		return (gray<<10)|(gray<<5)|gray;
	}
	static DebugPixel DColor( ColorVal color )
	{
		int r=toIntFloor(color.R()*31);
		int g=toIntFloor(color.G()*31);
		int b=toIntFloor(color.B()*31);
		saturate(r,0,31);
		saturate(g,0,31);
		saturate(b,0,31);
		return (r<<10)|(g<<5)|b;
	}
};

} // namespace Poseidon::Dev
