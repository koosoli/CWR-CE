#pragma once

#include <Poseidon/Foundation/Types/Memtype.h>

namespace Poseidon
{
struct DirectionInfo
{
	int dx;
	int dy;
	float angle;
};

static const int nDirections = 20;

static const DirectionInfo directions[nDirections] =
{
	{0,		-1, 0.0f},
	{-1,	-2, 26.565f},
	{-1,	-1, 45.0f},
	{-2,	-1, 63.435f},
	{-1,	0,	90.0f},
	{-2,	1,	116.565f},
	{-1,	1,	135.0f},
	{-1,	2,	153.435f},
	{0,		1,	180.0f},
	{1,		2,	-153.435f},
	{1,		1,	-135.0f},
	{2,		1,	-116.565f},
	{1,		0,	-90.0f},
	{2,		-1, -63.435f},
	{1,		-1, -45.0f},
	{1,		-2, -26.565f},
	{0,		-2, 0.0f},
	{-2,	0,	90.0f},
	{0,		2,	180.0f},
	{2,		0,	-90.0f}
};

struct Node
{
	// position / key
	union
	{
		int key;
		struct
		{
			int x:16;
			int y:16;
		};
	};
	// sort fields
	float h;	// heuristic
	float f;	// heuristic + cost
	// other
	BYTE dir;
};

bool operator <(const Node& a, const Node& b)
{
	return a.f < b.f;
}

bool operator ==(const Node& a, const Node& b)
{
	return (a.key == b.key);
}

template <class Appraisement, class OpenList, class ClosedList>
class AStar
{
protected:
	Appraisement _app;
	OpenList _open;
	ClosedList _closed;

	OpenList _openWorking;
	ClosedList _closedWorking;
	// goal position
	int _xg;
	int _yg;
	// robot position
	int _xr;
	int _yr;
	// flags
	bool _goalValid;
	bool _pathValid;
	bool _searching;
	// other
	float _heuristicCoef;

public:
	AStar(Appraisement app);
	void FindPath(int xfrom, int yfrom, int xto, int yto);
	void UpdatePath(int xfrom, int yfrom);

	void Simulate(int steps);
	bool GetTarget(int xfrom, int yfrom, int &xtarget, int &ytarget, bool aggregation = false);

// implementation
protected:
	void Step();
	void EstimateHeuristicCoef();
};

template <class Appraisement, class OpenList, class ClosedList>
class DStar
{
};

template <class Appraisement, class OpenList, class ClosedList>
AStar<Appraisement, OpenList, ClosedList>::AStar(Appraisement app)
{
	_app = app;
	_open.Clear();
	_closed.Clear();

	_xg = -1;
	_yg = -1;
	_xr = -1;
	_yr = -1;
	
	_goalValid = false;
	_pathValid = false;
	_searching = false;
}

template <class Appraisement, class OpenList, class ClosedList>
void AStar<Appraisement, OpenList, ClosedList>::FindPath(int xfrom, int yfrom, int xto, int yto)
{
	_xr = xfrom;
	_yr = yfrom;
	_xg = xto;
	_yg = yto;

	_goalValid = true;
	_pathValid = false;
	_searching = true;

	_openWorking.Clear();
	_closedWorking.Clear();

	EstimateHeuristicCoef();

	// add goal into _openWorking list
	Node node;
	node.x = xto;
	node.y = yto;
	float heur = _heuristicCoef * _app.Distance(xfrom, yfrom, xto, yto);
	node.h = heur;
	node.f = heur;
	node.dir = 0xff;
	_openWorking.Insert(node);
}

template <class Appraisement, class OpenList, class ClosedList>
void AStar<Appraisement, OpenList, ClosedList>::UpdatePath(int xfrom, int yfrom)
{
	if (!_goalValid) return;

	_xr = xfrom;
	_yr = yfrom;

	_searching = true;

	_openWorking.Clear();
	_closedWorking.Clear();

	EstimateHeuristicCoef();

	// add goal into _openWorking list
	Node node;
	node.x = _xg;
	node.y = _yg;
	float heur = _heuristicCoef * _app.Distance(xfrom, yfrom, _xg, _yg);
	node.h = heur;
	node.f = heur;
	node.dir = 0xff;
	_openWorking.Insert(node);
}

template <class Appraisement, class OpenList, class ClosedList>
void AStar<Appraisement, OpenList, ClosedList>::Simulate(int steps)
{
	if (!_searching) return;

}

template <class Appraisement, class OpenList, class ClosedList>
bool AStar<Appraisement, OpenList, ClosedList>::GetTarget(int xfrom, int yfrom, int &xtarget, int &ytarget, bool aggregation)
{
}

template <class Appraisement, class OpenList, class ClosedList>
void AStar<Appraisement, OpenList, ClosedList>::Step()
{
}

template <class Appraisement, class OpenList, class ClosedList>
void AStar<Appraisement, OpenList, ClosedList>::EstimateHeuristicCoef()
{
}

}  // namespace Poseidon
