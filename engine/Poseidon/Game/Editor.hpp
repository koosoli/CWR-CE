#pragma once

#include <Poseidon/World/Entities/Vehicles/Vehicle.hpp>

namespace Poseidon
{
} // namespace Poseidon


#ifdef _WIN32
  #include <winsock2.h>
#else
  #include <sys/socket.h>
  typedef int SOCKET;
  #define SOCKET_ERROR   -1
  #define INVALID_SOCKET -1
  #define closesocket(s) ::close(s)
#endif

using Poseidon::CameraType;
using Poseidon::SimulationImportance;

// Editor ↔ external tool message protocol
constexpr int LEN_FILENAME = 128;
constexpr int LEN_PATHNAME = 64;
constexpr int LEN_NAME = 32;

enum EditorMessageID
{
    SYSTEM_QUIT = 1,
    SYSTEM_INIT = 2,
    FILE_EXPORT = 3,
    FILE_IMPORT = 4,
    FILE_TRANSFER = 5,
    CURSOR_POSITION_SET = 6,
    SELECTION_CLEAR = 7,
    SELECTION_OBJECT_ADD = 8,
    REGISTER_LANDSCAPE_TEXTURE = 9,
    REGISTER_OBJECT_TYPE = 10,
    LAND_HEIGHT_CHANGE = 11,
    LAND_TEXTURE_CHANGE = 12,
    OBJECT_CREATE = 13,
    OBJECT_DESTROY = 14,
    OBJECT_MOVE = 15,
    OBJECT_TYPE_CHANGE = 16,
    FILE_IMPORT_BEGIN = 17,
    FILE_IMPORT_END = 18,
    BLOCK_MOVE_BEGIN = 19,
    BLOCK_MOVE_END = 20,
};

struct SPosMessage
{
    int nMsgID;
    union
    {
        struct { char szFileName[LEN_FILENAME]; };
        struct { char szPathFrom[LEN_PATHNAME]; char szPathTo[LEN_PATHNAME]; };
        struct { int nID; char szName[LEN_NAME]; bool bState; float Position[4][4]; };
        struct { int nX; int nZ; float Y; int nTextureID; };
        struct { int landRangeX, landRangeY; float landGridX, landGridY; };
    };
};

class EditCursor;

class CSelection
{
	public:
	FindArray< int > m_data;
	EditCursor *m_pOwner;
	
	public:
		CSelection() {m_pOwner = nullptr;}
	int AddObject (Object *pObj)
	{
		return AddID(pObj->ID());
	}
	int AddID(int nID);
	bool RemoveAddObject (Object *pObj)
	{
		return RemoveID(pObj->ID());
	}

	bool RemoveID(int nID);

	void Remove(int index);
	void RemoveAll();
	int Size() const {return m_data.Size();}
	int operator [] ( int i ) const {return m_data[i];}
	AutoArray< int > &GetData() {return m_data;}
};

class EditCursor: public Entity
{
	typedef Entity base;

private:

	// describe orientation (Euler parameters)
	float _heading,_dive,_bank;

	float _camDistance;
	float _camHeight;
	bool _visible;

	bool m_bCtrl;		// CTRL key was down when LBUTTON DOWN
	bool m_bOldMouseL;	// last state of left mouse button
	bool m_bOldMouseR;	// last state of right mouse button
	WORD m_wFlags;		// how move will be interpreted and other flags
	bool _showNode;
	
	bool _cameraOnEdit;
	OLink<Object> _animCamera;
	Poseidon::Foundation::Time _animCameraMoved;
	Poseidon::Foundation::Time _animCameraUpdated;

	CSelection m_Selection;
	Ref<Object> _drawRectangle;

	// State for moving, selecting and rotating
	Point3 m_ptStart;
	Point3 m_ptCurrent;
	int m_nPrimaryObject;
	bool m_bPrimarySelection;	// selection state of primary object
	AutoArray< int > m_MouseSelection;	// Objects in mouse selection cube

	Matrix4	m_posLast;

	SOCKET _socketSend;
	RString _ip;
	bool _connected;
	
	bool m_bMagnetize;
	bool m_bMoved;
	AutoArray< Matrix4 > m_transPure;
	
private:
	void CheckMouseSelection(Point3& ptMin, Point3& ptMax);

	void UpdateTerrain(Vector3 &position, float change);

	bool ReceiveMessage(SPosMessage	&sMsg);
	void HandleError(bool send);

public:
	bool ProcessEvents();
	void CCALL SendEvent(int nMsgID, ...);

	void Simulate( float deltaT, SimulationImportance prec ) override;
	void Draw( int forceLOD, ClipFlags clipFlags, const FrameBase &pos ) override;

	void Sound( bool inside, float deltaT ) override{}
	void UnloadSound() override{}
	Matrix4 InsideCamera( CameraType camType ) const override {return Matrix4(MTranslation,Vector3(0,0,-_camDistance));}
	float OutsideCameraDistance( CameraType camType ) const override {return _camDistance;}
	int InsideLOD( CameraType camType ) const override {return 0;}

	SimulationImportance WorstImportance() const override {return Poseidon::SimulateVisibleNear;}

	void LimitVirtual
	(
		CameraType camType, float &heading, float &dive, float &fov
	) const override;
	void InitVirtual
	(
		CameraType camType, float &heading, float &dive, float &fov
	) const override;
	bool IsContinuous( CameraType camType ) const override {return true;}

	void SwitchCamera();

	EditCursor(RString ip);
	~EditCursor() override;
};

