#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/World/Scene/Object.hpp>
#include <Poseidon/Game/Editor.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/Core/Progress.hpp>
#include <Poseidon/Foundation/Common/Filenames.hpp>

#include <stdarg.h>

#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_keyboard.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <sys/select.h>
#endif
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <cmath>
#include <utility>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/Array2D.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Types/LLinks.hpp>

// modes
#define MODE(x) (x & 0xff)
#define NOTMODE(x) (x & 0xff00)
#define MODE_NORMAL 0
#define MODE_SELECT 1
#define MODE_MOVE 2
#define MODE_ROTATE 3
// other flags
#define FLAG_POINTS 0x100 // magnetism for points
#define FLAG_PLANES 0x200 // magnetism for planes
#define FLAG_YFIXED 0x400 // magnetism in y axis forbidden

// speeds
#define SPEED_MOUSE 0.01f
#define SPEED_KEYB 1
#define SPEED_SCROLL 4

// other consts
#define INIT_SEL_ITEMS 64 // initial size of selection array
#define NEAR_OBJECT -1
#define MIN_MOVEMENT 0

using namespace Poseidon;
namespace Poseidon
{
} // namespace Poseidon

int CSelection::AddID(int nID)
{
    int nRet = m_data.Add(nID);
    m_pOwner->SendEvent(SELECTION_OBJECT_ADD, nID, true);
    return nRet;
}

bool CSelection::RemoveID(int nID)
{
    int index = m_data.Find(nID);
    if (index < 0)
    {
        return false;
    }
    m_data.DeleteAt(index);
    return true;
    // this function is used when object is destroyed
}

void CSelection::Remove(int index)
{
    int nObj = m_data[index];
    m_data.DeleteAt(index);
    m_pOwner->SendEvent(SELECTION_OBJECT_ADD, nObj, false);
}

void CSelection::RemoveAll()
{
    m_data.Init(INIT_SEL_ITEMS);
    m_pOwner->SendEvent(SELECTION_CLEAR);
}

const float DefCamDistance = 50.0;
const float DefCamHeight = 1;
const float DefDive = 0.5;

#ifndef _WIN32
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Common/NetGlobal.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <SDL3/SDL.h>

static bool TryToConnect(RString ip, SOCKET& socketSend)
{
    if (socketSend == INVALID_SOCKET)
    {
        socketSend = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    if (socketSend == INVALID_SOCKET)
    {
        return false;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(2500);
    return connect(socketSend, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}

void EditCursor::HandleError(bool send)
{
#ifdef _WIN32
    int error = WSAGetLastError();
#else
    int error = errno;
#endif

    if (send)
    {
        Foundation::WarningMessage("Sockets: Connection lost (cannot finish send, error %d)", error);
    }
    else
    {
        Foundation::WarningMessage("Sockets: Connection lost (cannot finish recv, error %d)", error);
    }

    if (_socketSend != INVALID_SOCKET)
    {
        closesocket(_socketSend);
        _socketSend = INVALID_SOCKET;
    }
    _connected = false;
}

EditCursor::EditCursor(RString ip)
    : _camDistance(DefCamDistance), _camHeight(DefCamHeight), _heading(0), _dive(DefDive), _bank(0), _visible(true),
      _cameraOnEdit(true), Vehicle(Shapes.New("data3d\\kursor.p3d", false, true), VehicleTypes.New("EditCursor"), -1)
{
    _animCameraMoved = Glob.time - 60;
    _animCameraUpdated = Glob.time;

    Matrix3Val rotY = Matrix3(MRotationY, _heading);
    Matrix3Val rotX = Matrix3(MRotationX, _dive);
    Matrix3Val rot = rotY * rotX;
    SetOrientation(rot);

    m_wFlags = MODE_NORMAL;
    m_bOldMouseL = false;
    m_bOldMouseR = false;
    m_bMagnetize = false;
    _showNode = false;

    m_Selection.m_pOwner = this;

    m_ptStart = VZero;
    m_ptCurrent = VZero;

#ifdef _WIN32
    WSADATA data;
    WSAStartup(0x0101, &data);
#endif

    _socketSend = INVALID_SOCKET;

    _ip = ip;
    _connected = ip.GetLength() > 0 ? TryToConnect(ip, _socketSend) : false;
}

EditCursor::~EditCursor()
{
    if (_socketSend != INVALID_SOCKET)
    {
        closesocket(_socketSend);
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

void EditCursor::LimitVirtual(CameraType camType, float& heading, float& dive, float& fov) const
{
    Vehicle::LimitVirtual(camType, heading, dive, fov);
    switch (camType)
    {
        case CamGunner:
        case CamInternal:
            heading = dive = 0;
            break;
    }
    fov = 0.7;
}

void EditCursor::InitVirtual(CameraType camType, float& heading, float& dive, float& fov) const
{
    Vehicle::InitVirtual(camType, heading, dive, fov);
}

void EditCursor::CheckMouseSelection(Point3& ptMin, Point3& ptMax)
{
    int i;

    m_MouseSelection.Init(INIT_SEL_ITEMS);
    if (m_bCtrl)
    {
        for (i = 0; i < m_Selection.Size(); i++)
        {
            m_MouseSelection.Add(m_Selection[i]);
        }
    }

    Landscape* land = GLOB_LAND;

    ptMin.Init();
    ptMax.Init();

    ptMin[0] = m_ptStart.X();
    ptMin[2] = m_ptStart.Z();
    ptMax[0] = m_ptCurrent.X();
    ptMax[2] = m_ptCurrent.Z();
    float pom;
    if (ptMin[0] > ptMax[0])
    {
        pom = ptMin[0];
        ptMin[0] = ptMax[0];
        ptMax[0] = pom;
    }
    if (ptMin[2] > ptMax[2])
    {
        pom = ptMin[2];
        ptMin[2] = ptMax[2];
        ptMax[2] = pom;
    }
    int nXMin = toIntFloor(ptMin.X() * InvObjGrid);
    int nXMax = toIntFloor(ptMax.X() * InvObjGrid);
    int nZMin = toIntFloor(ptMin.Z() * InvObjGrid);
    int nZMax = toIntFloor(ptMax.Z() * InvObjGrid);

    saturateMax(nXMin, 0);
    saturateMin(nXMax, land->GetLandRange() - 1);
    saturateMax(nZMin, 0);
    saturateMin(nZMax, land->GetLandRange() - 1);

    for (int nZ = nZMin; nZ <= nZMax; nZ++)
    {
        for (int nX = nXMin; nX <= nXMax; nX++)
        {
            ObjectList* pList = &land->_objects(nX, nZ);
            for (i = 0; i < pList->Size(); i++)
            {
                Ref<Object> pObj = (*pList)[i];
                if (pObj->GetType() == Primary)
                {
                    Point3 pos = pObj->Position();
                    if (pos.X() >= ptMin.X() && pos.X() <= ptMax.X() && pos.Z() >= ptMin.Z() && pos.Z() <= ptMax.Z())
                    {
                        int nObj = pObj->ID();
                        if (m_bCtrl)
                        {
                            int nFound = -1;
                            for (int j = 0; j < m_MouseSelection.Size(); j++)
                            {
                                if (m_MouseSelection[j] == nObj)
                                {
                                    nFound = j;
                                    break;
                                }
                            }
                            if (nFound < 0)
                            {
                                m_MouseSelection.Add(nObj);
                            }
                            else
                            {
                                m_MouseSelection.Delete(nFound);
                            }
                        }
                        else
                        {
                            m_MouseSelection.Add(nObj);
                        }
                    }
                }
            }
        }
    }
}

void CCALL EditCursor::SendEvent(int nMsgID, ...)
{
    if (!_connected)
    {
        return;
    }

    va_list args;
    va_start(args, nMsgID);

    SPosMessage event;
    event.nMsgID = nMsgID;
    switch (nMsgID)
    {
        case FILE_EXPORT:
        case FILE_IMPORT:
            strncpy(event.szFileName, va_arg(args, char*), LEN_FILENAME);
            break;
        case FILE_TRANSFER:
            strncpy(event.szPathFrom, va_arg(args, char*), LEN_PATHNAME);
            strncpy(event.szPathTo, va_arg(args, char*), LEN_PATHNAME);
            break;
        case CURSOR_POSITION_SET:
        {
            Matrix4* pPos = va_arg(args, Matrix4*);
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    event.Position[i][j] = pPos->Orientation()(i, j);
                }
            }
            for (int i = 0; i < 3; i++)
            {
                event.Position[i][3] = pPos->Position()[i];
            }
        }
        break;
        case SELECTION_OBJECT_ADD:
            event.nID = va_arg(args, int);
#ifdef _WIN32
            event.bState = va_arg(args, bool);
#else
            event.bState = va_arg(args, int) != 0;
#endif
            break;
        case REGISTER_LANDSCAPE_TEXTURE:
            event.nID = va_arg(args, int);
            strncpy(event.szName, va_arg(args, char*), LEN_NAME);
            break;
        case REGISTER_OBJECT_TYPE:
            strncpy(event.szName, va_arg(args, char*), LEN_NAME);
            break;
        case LAND_HEIGHT_CHANGE:
            event.nX = va_arg(args, int);
            event.nZ = va_arg(args, int);
            event.Y = va_arg(args, double);
            break;
        case LAND_TEXTURE_CHANGE:
            event.nX = va_arg(args, int);
            event.nZ = va_arg(args, int);
            event.nTextureID = va_arg(args, int);
            break;
        case OBJECT_CREATE:
            event.nID = va_arg(args, int);
            strncpy(event.szName, va_arg(args, char*), LEN_NAME);
            {
                Matrix4* pPos = va_arg(args, Matrix4*);
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        event.Position[i][j] = pPos->Orientation()(i, j);
                    }
                }
                for (int i = 0; i < 3; i++)
                {
                    event.Position[i][3] = pPos->Position()[i];
                }
            }
            break;
        case OBJECT_DESTROY:
            event.nID = va_arg(args, int);
            break;
        case OBJECT_MOVE:
            event.nID = va_arg(args, int);
            {
                Matrix4* pPos = va_arg(args, Matrix4*);
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        event.Position[i][j] = pPos->Orientation()(i, j);
                    }
                }
                for (int i = 0; i < 3; i++)
                {
                    event.Position[i][3] = pPos->Position()[i];
                }
            }
            break;
        case OBJECT_TYPE_CHANGE:
            event.nID = va_arg(args, int);
            strncpy(event.szName, va_arg(args, char*), LEN_NAME);
            break;
        case SYSTEM_QUIT:
        case SYSTEM_INIT:
        case SELECTION_CLEAR:
        case FILE_IMPORT_BEGIN:
        case FILE_IMPORT_END:
        case BLOCK_MOVE_BEGIN:
        case BLOCK_MOVE_END:
            // no params
            break;
    }

    fd_set set;
    FD_ZERO(&set);
    FD_SET(_socketSend, &set);
    timeval timeout = {30L, 0L};
    int result = select(FD_SETSIZE, nullptr, &set, nullptr, &timeout);
    if (result == 0)
    {
        Foundation::WarningMessage("Sockets: Timed out during send, message lost");
        return;
    }
    if (result == SOCKET_ERROR)
    {
        HandleError(true);
        return;
    }

    result = send(_socketSend, (const char*)&event, sizeof(event), 0);
    if (result == SOCKET_ERROR)
    {
        HandleError(true);
    }
    va_end(args);
}

void EditLog(const char* format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    // create or append to session log
    static bool notFirst;

    FILE* f;
    static const char logName[] = "buldozer.log";
    if (notFirst)
    {
        f = fopen(logName, "at");
    }
    else
    {
        f = fopen(logName, "wt"), notFirst = true;
    }
    if (!f)
    {
        return;
    }
    vfprintf(f, format, arglist);
    fputc('\n', f);
    fclose(f);

    va_end(arglist);
}

void EditCursor::SwitchCamera()
{
    if (!_animCamera)
    {
        // find camera object
        int x, z;
        for (x = 0; x < LandRange; x++)
        {
            for (z = 0; z < LandRange; z++)
            {
                const ObjectList& list = GLOB_LAND->GetObjects(z, x);
                for (int i = 0; i < list.Size(); i++)
                {
                    Object* obj = list[i];
                    if (!strcmpi(obj->GetShape()->Name(), "data3d\\camera.p3d"))
                    {
                        _animCamera = obj;
                        break;
                    }
                }
            }
        }
    }
    _cameraOnEdit = !_cameraOnEdit;
    if (!_animCamera)
    {
        _cameraOnEdit = true;
    }
    if (_cameraOnEdit)
    {
        GLOB_WORLD->SwitchCameraTo(this, CamInternal);
    }
    else
    {
        GLOB_WORLD->SwitchCameraTo(_animCamera, CamInternal);
    }
}

#include <Poseidon/Dev/Debug/DebugTrap.hpp>

using namespace Poseidon::Dev;

bool EditCursor::ReceiveMessage(SPosMessage& sMsg)
{
    if (_socketSend == INVALID_SOCKET)
    {
        return false;
    }

    fd_set set;
    FD_ZERO(&set);
    FD_SET(_socketSend, &set);
    timeval timeout = {0L, 0L};

    int result = select(FD_SETSIZE, &set, nullptr, nullptr, &timeout);
    if (result == 0)
    {
        return false; // no data
    }
    if (result == SOCKET_ERROR)
    {
        HandleError(false);
        return false;
    }

    int len = recv(_socketSend, (char*)&sMsg, sizeof(sMsg), 0);
    if (len == 0)
    {
        return false;
    }
    if (len == SOCKET_ERROR)
    {
        HandleError(false);
        return false;
    }
    while (len < sizeof(sMsg))
    {
        FD_ZERO(&set);
        FD_SET(_socketSend, &set);
        timeval timeout = {30L, 0L};
        result = select(FD_SETSIZE, &set, nullptr, nullptr, &timeout);
        if (result == 0)
        {
            Foundation::WarningMessage("Sockets: Timed out during receive, message lost");
            return false;
        }
        if (result == SOCKET_ERROR)
        {
            HandleError(false);
            return false;
        }

        int len2 = recv(_socketSend, (char*)&sMsg + len, sizeof(sMsg) - len, 0);
        if (len2 == SOCKET_ERROR)
        {
            HandleError(false);
            return false;
        }
        len += len2;
    }
    return true;
}

enum TransferState
{
    TSNone,
    TSTexturesRegistration,
    TSHeights,
    TSTextures,
    TSObjectTypes,
    TSObjects
};

bool EditCursor::ProcessEvents()
{
    if (_ip.GetLength() == 0)
    {
        return false;
    }
    if (!_connected)
    {
        if (TryToConnect(_ip, _socketSend))
        {
            _connected = true;
        }
        else
        {
            return false;
        }
    }

    GDebugger.NextAliveExpected(15 * 60 * 1000);

    static TransferState lanscapeTransfer = TSNone;
    static int objRegistered, objInstances, texRegistered;

    SPosMessage event;
    Landscape* land = GLOB_LAND;
    bool retVal = false;
    bool dataReady = false;

    while ((dataReady = ReceiveMessage(event)) || lanscapeTransfer != TSNone)
    {
        if (!dataReady)
        {
            Sleep(10);
            continue;
        }

        retVal = true;
        GApp->m_forceRender = true;
        switch (event.nMsgID)
        {
            case SYSTEM_QUIT:
                land->Quit();
                break;
            case SYSTEM_INIT:
            {
                land->Dim(event.landRangeX, event.landRangeY, event.landRangeX, event.landRangeY, event.landGridX);
                land->Init();
                lanscapeTransfer = TSNone;
                break;
            }
            case FILE_EXPORT:
                land->SaveData(event.szFileName);
                break;
            case FILE_IMPORT:
                land->Init();
                land->LoadData(event.szFileName, 50.0);
                {
                    SendEvent(FILE_IMPORT_BEGIN);
                    SendEvent(SYSTEM_INIT);
                    int x, z;
                    // Set height
                    for (x = 0; x < LandRange; x++)
                    {
                        for (z = 0; z < LandRange; z++)
                        {
                            SendEvent(LAND_HEIGHT_CHANGE, x, z, land->GetHeight(z, x));
                        }
                    }
                    // Register textures
                    for (x = 0; x < land->_texture.Size(); x++)
                    {
                        Texture* tex = land->_texture[x];
                        if (tex)
                        {
                            SendEvent(REGISTER_LANDSCAPE_TEXTURE, x, tex->Name());
                        }
                    }
                    // Set textures
                    for (x = 0; x < LandRange; x++)
                    {
                        for (z = 0; z < LandRange; z++)
                        {
                            SendEvent(LAND_TEXTURE_CHANGE, x, z, land->GetTexture(z, x));
                        }
                    }
                    // Register object types
                    Shapes.ForEach([this](LODShapeWithShadow& shape)
                                   { SendEvent(REGISTER_OBJECT_TYPE, shape.Name()); });
                    // Create objects
                    AutoArray<int> objects = land->GetObjectIDList();
                    for (x = 0; x < objects.Size(); x++)
                    {
                        Ref<Object> pObj = land->GetObject(objects[x]);
                        SendEvent(OBJECT_CREATE, objects[x], pObj->GetShape()->Name(), &pObj->Transform());
                    }
                    SendEvent(FILE_IMPORT_END);
                }
                break;
            case FILE_TRANSFER:
                break;
            case CURSOR_POSITION_SET:
            {
                Matrix4 trans;
                Point3 pos;
                Matrix3Val ori = M3Identity;
                for (int i = 0; i < 3; i++)
                {
                    pos[i] = event.Position[i][3];
                }
                trans.SetOrientation(ori);
                trans.SetPosition(pos);
                Move(trans);
            }
            break;
            case SELECTION_CLEAR:
                m_Selection.m_data.Init(INIT_SEL_ITEMS);
                break;
            case SELECTION_OBJECT_ADD:
            {
                int nFound = -1;
                for (int i = 0; i < m_Selection.Size(); i++)
                {
                    if (m_Selection[i] == event.nID)
                    {
                        nFound = i;
                        break;
                    }
                }
                if (event.bState)
                {
                    if (nFound < 0)
                    {
                        m_Selection.m_data.Add(event.nID);
                    }
                }
                else
                {
                    if (nFound >= 0)
                    {
                        m_Selection.m_data.DeleteAt(nFound);
                    }
                }
            }
            break;
            case FILE_IMPORT_BEGIN:
                // begin landscape transfer from Visitor
                lanscapeTransfer = TSTexturesRegistration;
                objRegistered = event.nX;
                objInstances = event.nZ;
                texRegistered = event.nTextureID;

                ProgressReset();
                ProgressStart("Registering terrain textures");
                ProgressAdd(texRegistered);
                ProgressFrame();
                break;
            case FILE_IMPORT_END:
                // finish landscape transfer from Visitor
                lanscapeTransfer = TSNone;
                land->Recalculate();
                if (objInstances != 0)
                {
                    LOG_DEBUG(Script, "Missing {} object instances.", objInstances);
                }
                ProgressFrame();
                ProgressFinish();
                break;
            case REGISTER_LANDSCAPE_TEXTURE:
                EditLog("Register texture %d:%s", event.nID, event.szName);
                land->RegisterTexture(event.nID, event.szName);
                if (lanscapeTransfer != TSNone)
                {
                    ProgressSetRest(--texRegistered);
                    ProgressRefresh();
                }
                break;
            case REGISTER_OBJECT_TYPE:
            {
                // extend partial name if necessary
                const char* ext = strrchr(event.szName, '.');
                if (ext != nullptr && strlen(ext) >= 4 || strlen(event.szName) < LEN_NAME - 1)
                {
                    land->RegisterObjectType(event.szName);
                }
                else
                {
                    char name[128];
                    int len = ext - event.szName;
                    strncpy(name, event.szName, len);
                    name[len] = 0;
                    land->RegisterObjectType(name);
                }

                if (lanscapeTransfer != TSNone)
                {
                    if (lanscapeTransfer == TSTextures)
                    {
                        ProgressFinish();

                        lanscapeTransfer = TSObjectTypes;
                        ProgressReset();
                        ProgressStart("Receiving object types");
                        ProgressAdd(objRegistered);
                        ProgressFrame();
                    }
                    ProgressSetRest(--objRegistered);
                    ProgressRefresh();
                }
                break;
            }
            case LAND_HEIGHT_CHANGE:
                land->HeightChange(event.nX, event.nZ, event.Y);

                if (lanscapeTransfer != TSNone)
                {
                    if (lanscapeTransfer == TSTexturesRegistration)
                    {
                        if (texRegistered != 0)
                        {
                            LOG_DEBUG(Script, "Missing {} textures to register.", texRegistered);
                        }
                        ProgressFinish();

                        lanscapeTransfer = TSHeights;
                        ProgressReset();
                        ProgressStart("Receiving terrain height map");
                        ProgressAdd(Square(land->GetLandRange()));
                        ProgressFrame();
                    }
                    ProgressAdvance(1);
                    ProgressRefresh();
                }
                break;
            case LAND_TEXTURE_CHANGE:
                land->TextureChange(event.nX, event.nZ, event.nTextureID);

                if (lanscapeTransfer != TSNone)
                {
                    if (lanscapeTransfer == TSHeights)
                    {
                        ProgressFinish();

                        lanscapeTransfer = TSTextures;
                        ProgressReset();
                        ProgressStart("Receiving terrain textures");
                        ProgressAdd(Square(land->GetLandRange()));
                        ProgressFrame();
                    }
                    ProgressAdvance(1);
                    ProgressRefresh();
                }
                break;
            case OBJECT_CREATE:
            {
                Matrix4 trans;
                Point3 pos;
                Matrix3 ori;
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        ori(i, j) = event.Position[i][j];
                    }
                }
                for (int i = 0; i < 3; i++)
                {
                    pos[i] = event.Position[i][3];
                }
                trans.SetOrientation(ori);
                trans.SetPosition(pos);
                SetTransform(trans);
                land->ObjectCreate(event.nID, event.szName, trans, nullptr, nullptr, lanscapeTransfer != TSNone);
            }
                if (lanscapeTransfer != TSNone)
                {
                    if (lanscapeTransfer == TSObjectTypes)
                    {
                        if (objRegistered != 0)
                        {
                            LOG_DEBUG(Script, "Missing {} objects to register.", objRegistered);
                        }
                        ProgressFinish();

                        lanscapeTransfer = TSObjects;
                        ProgressReset();
                        ProgressStart("Receiving objects");
                        ProgressAdd(objInstances);
                        ProgressFrame();
                    }
                    ProgressSetRest(--objInstances);
                    ProgressRefresh();
                }
                break;
            case OBJECT_DESTROY:
                // make sure object is no longer is selection
                m_Selection.RemoveID(event.nID);
                land->ObjectDestroy(event.nID);
                break;
            case OBJECT_MOVE:
            {
                Matrix4 trans;
                Point3 pos;
                Matrix3 ori;
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        ori(i, j) = event.Position[i][j];
                    }
                }
                for (int i = 0; i < 3; i++)
                {
                    pos[i] = event.Position[i][3];
                }
                trans.SetOrientation(ori);
                trans.SetPosition(pos);
                SetTransform(trans);
                land->ObjectMove(event.nID, trans);
                m_bMoved = true; // for Magnetize
            }
            break;
            case OBJECT_TYPE_CHANGE:
                land->ObjectTypeChange(event.nID, event.szName);
                break;
        }
    }
    return retVal;
}

void EditCursor::UpdateTerrain(Vector3& position, float change)
{
    float xs = m_ptStart.X();
    float zs = m_ptStart.Z();
    float xe = m_ptCurrent.X();
    float ze = m_ptCurrent.Z();

    if (xs > xe)
    {
        swap(xs, xe);
    }
    if (zs > ze)
    {
        swap(zs, ze);
    }

    float landGrid = GLandscape->GetLandGrid();
    float invLandGrid = GLandscape->GetInvLandGrid();

    int xxs = toIntCeil(xs * invLandGrid);
    int zzs = toIntCeil(zs * invLandGrid);
    int xxe = toIntFloor(xe * invLandGrid);
    int zze = toIntFloor(ze * invLandGrid);

    if (xxs > xxe || zzs > zze)
    {
        // no node in selection - change nearest
        int xx = toInt(position.X() * InvLandGrid);
        int zz = toInt(position.Z() * InvLandGrid);
        if (InRange(zz, xx))
        {
            GLandscape->HeightChange(xx, zz, GLandscape->GetData(xx, zz) + change);
        }
        float x = xx * landGrid;
        float z = zz * landGrid;
        float height = GLandscape->SurfaceY(x, z);
        position[0] = x;
        position[2] = z;
        position[1] = height;
        GLandscape->TerrainChanged(x, z, LandGrid * 2);
        SendEvent(LAND_HEIGHT_CHANGE, xx, zz, height);
    }
    else
    {
        float dy = position[1] - GLandscape->SurfaceY(position[0], position[1]);
        for (int zz = zzs; zz <= zze; zz++)
        {
            for (int xx = xxs; xx <= xxe; xx++)
            {
                if (InRange(zz, xx))
                {
                    GLandscape->HeightChange(xx, zz, GLandscape->GetData(xx, zz) + change);
                }
                float x = xx * landGrid;
                float z = zz * landGrid;
                float height = GLandscape->SurfaceY(x, z);
                SendEvent(LAND_HEIGHT_CHANGE, xx, zz, height);
            }
        }
        position[1] = GLandscape->SurfaceY(position[0], position[1]) + dy;

        GLandscape->TerrainChanged(0.5 * (xs + xe), 0.5 * (zs + ze), 0.5 * floatMax(xe - xs, ze - zs) + 2 * landGrid);
    }
}

void EditCursor::Simulate(float deltaT, SimulationImportance prec)
{
    int i, j, x, z;
    auto& input = InputSubsystem::Instance();
    Landscape* land = GLOB_LAND;

    // restore unmagnetized positions
    if (m_bMagnetize)
    {
        for (i = 0; i < m_Selection.Size(); i++)
        {
            land->GetObject(m_Selection[i])->SetTransform(m_transPure[i]);
        }
        m_transPure.Init(INIT_SEL_ITEMS);
        m_bMagnetize = false;
    }

    // process events from editor
    m_bMoved = false;
    bool busy = ProcessEvents();

    if (input.ConsumeKeyPress(SDL_SCANCODE_INSERT))
    {
        SwitchCamera();
    }

    // init
    Vector3 position = Position();
    float rotYAngle = 0, rotXAngle = 0;

    const bool* keystate = SDL_GetKeyboardState(nullptr);
    bool bFaster = keystate[SDL_SCANCODE_SCROLLLOCK];
    float speedKeyb = deltaT * SPEED_KEYB;

    bool bReset = false; // Reset camera position
    Point3 ptMin, ptMax;

    // translate cursor keys to mouse moving
    float speedX = input.GetMouseDeltaX() * SPEED_MOUSE;
    float speedZ = -input.GetMouseDeltaY() * SPEED_MOUSE;
    if (bFaster)
    {
        speedX *= SPEED_SCROLL;
        speedZ *= SPEED_SCROLL;
        speedKeyb *= SPEED_SCROLL;
    }

    Frame moveTrans;
    moveTrans.SetTransform(*this);

    // process mouse moving
    if (_cameraOnEdit)
    {
        // edit cursor movement

        switch (MODE(m_wFlags))
        {
            case MODE_NORMAL:
                if (input.IsKeyDown(SDL_SCANCODE_KP_5))
                {
                    rotXAngle += speedZ;
                    rotYAngle -= speedX;
                }
                else
                {
                    Vector3 offset(speedX * _camDistance, 0, speedZ * _camDistance);
                    Matrix4Val rotY = Matrix4(MRotationY, _heading);
                    offset.SetRotate(rotY, offset);
                    position += offset;
                }
                break;
            case MODE_SELECT:
            {
                float xOffset = speedX * _camDistance / 3;
                float zOffset = speedZ * _camDistance / 3;
                Vector3 offset(xOffset, 0, zOffset);
                Matrix4Val rotY = Matrix4(MRotationY, _heading);
                offset.SetRotate(rotY, offset);
                m_ptCurrent += offset;
                CheckMouseSelection(ptMin, ptMax);
                position = m_ptCurrent;
            }
            break;
            case MODE_MOVE:
            {
                float xOffset = speedX * _camDistance / 3;
                float zOffset = speedZ * _camDistance / 3;
                if (xOffset == 0 && zOffset == 0)
                {
                    break;
                }
                Vector3 offset(xOffset, 0, zOffset);
                Matrix4Val rotY = Matrix4(MRotationY, _heading);
                offset.SetRotate(rotY, offset);
                for (i = 0; i < m_Selection.Size(); i++)
                {
                    Ref<Object> pObj = land->GetObject(m_Selection[i]);
                    Point3 ptObj = pObj->Position();
                    float yRel = ptObj.Y() - land->SurfaceY(ptObj.X(), ptObj.Z());
                    ptObj += offset;
                    ptObj[1] = land->SurfaceY(ptObj.X(), ptObj.Z()) + yRel;
                    land->MoveObject(pObj, ptObj);
                }
                m_ptStart += offset;
                m_ptCurrent += offset;
                position = m_ptCurrent;
            }
            break;
            case MODE_ROTATE:
            {
                float yAngle = speedX;
                if (yAngle == 0)
                {
                    break;
                }
                Matrix3 rot(MRotationY, yAngle); // rotation is identical for all objects
                for (i = 0; i < m_Selection.Size(); i++)
                {
                    Ref<Object> pObj = land->GetObject(m_Selection[i]);
                    Point3 ptObj = pObj->Position();
                    float yRel = ptObj.Y() - land->SurfaceY(ptObj.X(), ptObj.Z());

                    Matrix3Val orient = pObj->Transform().Orientation() * rot; // rotate orientation
                    Vector3Val vector = rot * (ptObj - m_ptStart);             // rotate position
                    ptObj = m_ptStart + vector;
                    ptObj[1] = land->SurfaceY(ptObj.X(), ptObj.Z()) + yRel;

                    pObj->SetOrientation(orient);
                    land->MoveObject(pObj, ptObj);
                }
            }
            break;
        }

        // process keys
        Vector3 offset;
        for (int key = 0; key < 256; key++)
        {
            if (input.ConsumeKeyPress(static_cast<SDL_Scancode>(key)))
            {
                switch (key)
                {
                    case SDL_SCANCODE_SPACE:
                    {
                        // Invert selection of nearest object
                        Ref<Object> pObj = land->NearestObject(position, NEAR_OBJECT, Primary);
                        if (pObj && pObj->ID() >= 0)
                        {
                            int nObj = pObj->ID();
                            int nFound = -1;
                            for (i = 0; i < m_Selection.Size(); i++)
                            {
                                if (m_Selection[i] == nObj)
                                {
                                    nFound = i;
                                    break;
                                }
                            }
                            if (nFound < 0)
                            {
                                m_Selection.AddID(nObj);
                            }
                            else
                            {
                                m_Selection.Remove(nFound);
                            }
                        }
                    }
                    break;
                    case SDL_SCANCODE_KP_0:
                        // Reset camera position
                        bReset = true;
                        break;
                    case SDL_SCANCODE_F5:
                        m_wFlags ^= FLAG_POINTS;
                        break;
                    case SDL_SCANCODE_F6:
                        m_wFlags ^= FLAG_PLANES;
                        break;
                    case SDL_SCANCODE_F7:
                        m_wFlags ^= FLAG_YFIXED;
                        break;
                    case SDL_SCANCODE_U:
                        UpdateTerrain(position, +1.0);
                        break;
                    case SDL_SCANCODE_J:
                        UpdateTerrain(position, -1.0);
                        break;
                    case SDL_SCANCODE_I:
                        UpdateTerrain(position, +5.0);
                        break;
                    case SDL_SCANCODE_K:
                        UpdateTerrain(position, -5.0);
                        break;
                    case SDL_SCANCODE_H:
                        _showNode = !_showNode;
                        break;
                    case SDL_SCANCODE_LEFT:
                        offset = Vector3(-0.1, 0, 0);
                        goto MoveObject;
                    case SDL_SCANCODE_RIGHT:
                        offset = Vector3(0.1, 0, 0);
                        goto MoveObject;
                    case SDL_SCANCODE_DOWN:
                        offset = Vector3(0, 0, -0.1);
                        goto MoveObject;
                    case SDL_SCANCODE_UP:
                        offset = Vector3(0, 0, 0.1);
                        goto MoveObject;
                    MoveObject:
                        Matrix4Val rotY = Matrix4(MRotationY, _heading);
                        offset.SetRotate(rotY, offset);
                        for (i = 0; i < m_Selection.Size(); i++)
                        {
                            Ref<Object> pObj = land->GetObject(m_Selection[i]);
                            Point3 ptObj = pObj->Position();
                            float yRel = ptObj.Y() - land->SurfaceY(ptObj.X(), ptObj.Z());
                            ptObj += offset;
                            ptObj[1] = land->SurfaceY(ptObj.X(), ptObj.Z()) + yRel;
                            land->MoveObject(pObj, ptObj);
                            SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
                        }
                        m_ptCurrent += offset;
                        position += offset;
                        break;
                }
            }
        }

        rotXAngle -= input.GetKeyValue(SDL_SCANCODE_KP_2) * speedKeyb;
        rotYAngle += input.GetKeyValue(SDL_SCANCODE_KP_4) * speedKeyb;
        rotYAngle -= input.GetKeyValue(SDL_SCANCODE_KP_6) * speedKeyb;
        rotXAngle += input.GetKeyValue(SDL_SCANCODE_KP_8) * speedKeyb;
        _camDistance += input.GetKeyValue(SDL_SCANCODE_KP_MINUS) * speedKeyb * 20;
        _camDistance -= input.GetKeyValue(SDL_SCANCODE_KP_PLUS) * speedKeyb * 20;
        saturate(_camDistance, 0, 500);
        if (input.IsKeyDown(SDL_SCANCODE_PAGEDOWN))
        {
            if (MODE(m_wFlags) == MODE_MOVE || MODE(m_wFlags) == MODE_ROTATE)
            {
                for (i = 0; i < m_Selection.Size(); i++)
                {
                    Ref<Object> pObj = land->GetObject(m_Selection[i]);
                    Point3 ptObj = pObj->Position();
                    ptObj[1] -= input.GetKeyValue(SDL_SCANCODE_PAGEDOWN) * speedKeyb * _camDistance / 3;
                    pObj->SetPosition(ptObj);
                    SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
                }
            }
            else
            {
                _camHeight -= input.GetKeyValue(SDL_SCANCODE_PAGEDOWN) * speedKeyb * 20;
                _camHeight = floatMax(_camHeight, -5);
            }
        }
        if (input.IsKeyDown(SDL_SCANCODE_PAGEUP))
        {
            if (MODE(m_wFlags) == MODE_MOVE || MODE(m_wFlags) == MODE_ROTATE)
            {
                for (i = 0; i < m_Selection.Size(); i++)
                {
                    Ref<Object> pObj = land->GetObject(m_Selection[i]);
                    Point3 ptObj = pObj->Position();
                    ptObj[1] += input.GetKeyValue(SDL_SCANCODE_PAGEUP) * speedKeyb * _camDistance / 3;
                    pObj->SetPosition(ptObj);
                    SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
                }
            }
            else
            {
                _camHeight += input.GetKeyValue(SDL_SCANCODE_PAGEUP) * speedKeyb * 20;
                _camHeight = floatMin(_camHeight, +500);
            }
        }

        // process mouse buttons

        if (input.IsMouseLeftDown() && !m_bOldMouseL)
        {
            // mouseL down
            if (MODE(m_wFlags) == MODE_NORMAL)
            {
                m_bCtrl = (input.IsKeyDown(SDL_SCANCODE_LCTRL) || input.IsKeyDown(SDL_SCANCODE_RCTRL));
                m_ptCurrent = m_ptStart = position;
                Ref<Object> pObj = land->NearestObject(position, NEAR_OBJECT, Primary);
                if (pObj && pObj->ID() >= 0)
                {
                    int nObj = pObj->ID();
                    int nFound = -1;
                    for (i = 0; i < m_Selection.Size(); i++)
                    {
                        if (m_Selection[i] == nObj)
                        {
                            nFound = i;
                            break;
                        }
                    }
                    m_nPrimaryObject = nObj;
                    if (m_bCtrl)
                    {
                        m_bPrimarySelection = (nFound >= 0);
                        if (!m_bPrimarySelection)
                        {
                            m_Selection.AddID(nObj);
                        }
                    }
                    else
                    {
                        if (nFound < 0)
                        {
                            m_Selection.RemoveAll();
                            m_Selection.AddID(nObj);
                        }
                    }
                    m_wFlags = NOTMODE(m_wFlags) | MODE_MOVE;
                }
                else
                {
                    m_wFlags = NOTMODE(m_wFlags) | MODE_SELECT;
                    CheckMouseSelection(ptMin, ptMax);
                }
            }
        }
        else if (!input.IsMouseLeftDown() && m_bOldMouseL)
        {
            // mouseL up
            if (MODE(m_wFlags) == MODE_SELECT)
            {
                m_Selection.RemoveAll();
                for (i = 0; i < m_MouseSelection.Size(); i++)
                {
                    m_Selection.AddID(m_MouseSelection[i]);
                }
                m_MouseSelection.Init(INIT_SEL_ITEMS);
                m_wFlags = NOTMODE(m_wFlags) | MODE_NORMAL;
            }
            else if (MODE(m_wFlags) == MODE_MOVE)
            {
                Vector3 offset = m_ptStart - m_ptCurrent;
                float minMovement = MIN_MOVEMENT * _camDistance * (1 / DefCamDistance);
                if (offset.Size() < minMovement)
                {
                    // Move back to starting position
                    for (i = 0; i < m_Selection.Size(); i++)
                    {
                        Ref<Object> pObj = land->GetObject(m_Selection[i]);
                        Point3 ptObj = pObj->Position();
                        float yRel = ptObj.Y() - land->SurfaceY(ptObj.X(), ptObj.Z());
                        ptObj += offset;
                        ptObj[1] = land->SurfaceY(ptObj.X(), ptObj.Z()) + yRel;
                        land->MoveObject(pObj, ptObj);
                    }
                    if (m_bCtrl)
                    {
                        if (m_bPrimarySelection)
                        {
                            int nFound = -1;
                            for (i = 0; i < m_Selection.Size(); i++)
                            {
                                if (m_Selection[i] == m_nPrimaryObject)
                                {
                                    nFound = i;
                                    break;
                                }
                            }
                            if (nFound >= 0)
                            {
                                m_Selection.Remove(nFound);
                            }
                        }
                    }
                    else
                    {
                        m_Selection.RemoveAll();
                        m_Selection.AddID(m_nPrimaryObject);
                    }
                    position = m_ptStart;
                }
                else
                {
                    SendEvent(BLOCK_MOVE_BEGIN);
                    for (i = 0; i < m_Selection.Size(); i++)
                    {
                        Ref<Object> pObj = land->GetObject(m_Selection[i]);
                        SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
                    }
                    SendEvent(BLOCK_MOVE_END);
                    m_bMoved = true;
                }
                m_nPrimaryObject = -1;
                m_wFlags = NOTMODE(m_wFlags) | MODE_NORMAL;
            }
        }
        if (input.IsMouseRightDown() && !m_bOldMouseR)
        {
            // mouseR down
            if (MODE(m_wFlags) == MODE_NORMAL && m_Selection.Size() > 0)
            {
                m_ptStart = position;
                m_wFlags = NOTMODE(m_wFlags) | MODE_ROTATE;
            }
        }
        else if (!input.IsMouseRightDown() && m_bOldMouseR)
        {
            // mouseR up
            if (MODE(m_wFlags) == MODE_ROTATE)
            {
                m_wFlags = NOTMODE(m_wFlags) | MODE_NORMAL;
                SendEvent(BLOCK_MOVE_BEGIN);
                for (i = 0; i < m_Selection.Size(); i++)
                {
                    Ref<Object> pObj = land->GetObject(m_Selection[i]);
                    SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
                }
                SendEvent(BLOCK_MOVE_END);
                m_bMoved = true;
            }
        }
        m_bOldMouseL = input.IsMouseLeftDown();
        m_bOldMouseR = input.IsMouseRightDown();

        // magnetism
        if (MODE(m_wFlags) == MODE_ROTATE || MODE(m_wFlags) == MODE_MOVE)
        {
            for (i = 0; i < m_Selection.Size(); i++)
            {
                m_transPure.Add(land->GetObject(m_Selection[i])->Transform());
            }
            if (land->Magnetize((m_wFlags & FLAG_POINTS) != 0, (m_wFlags & FLAG_PLANES) != 0,
                                (m_wFlags & FLAG_YFIXED) != 0, m_Selection.GetData()))
            {
                m_bMagnetize = true;
            }
            else
            {
                m_transPure.Init(INIT_SEL_ITEMS);
            }
        }
        else if (m_bMoved)
        {
            if (land->Magnetize((m_wFlags & FLAG_POINTS) != 0, (m_wFlags & FLAG_PLANES) != 0,
                                (m_wFlags & FLAG_YFIXED) != 0, m_Selection.GetData()))
            {
                SendEvent(BLOCK_MOVE_BEGIN);
                for (i = 0; i < m_Selection.Size(); i++)
                {
                    Ref<Object> pObj = land->GetObject(m_Selection[i]);
                    SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
                }
                SendEvent(BLOCK_MOVE_END);
            }
        }

        // change position of cursor
        if (bReset)
        {
            _camDistance = DefCamDistance;
            _camHeight = DefCamHeight;
            _dive = DefDive;
        }
        else
        {
            _dive += rotXAngle;
        }
        _heading += rotYAngle;

        Matrix3Val rotY = Matrix3(MRotationY, _heading);
        Matrix3Val rotX = Matrix3(MRotationX, _dive);
        Matrix3Val rot = rotY * rotX;

        position[1] = GLOB_LAND->SurfaceY(position[0], position[2]) + _camHeight;
        saturateMax(position[1], 0.1); // never allow under water camera

        moveTrans.SetOrientation(rot);
        moveTrans.SetPosition(position);

        // Send message CURSOR_POSITION_SET if position changed
        bool bChanged = false;
        for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 4; j++)
            {
                if (Transform()(i, j) != m_posLast(i, j))
                {
                    bChanged = true;
                    break;
                }
            }
            if (bChanged)
            {
                break;
            }
        }
        if (bChanged)
        {
            SendEvent(CURSOR_POSITION_SET, &Transform());
            m_posLast = Transform();
        }
    }
    else
    { // camera movement
        // rotate
        rotXAngle -= input.GetKeyValue(SDL_SCANCODE_KP_2) * speedKeyb * 0.2;
        rotYAngle += input.GetKeyValue(SDL_SCANCODE_KP_4) * speedKeyb * 0.2;
        rotYAngle -= input.GetKeyValue(SDL_SCANCODE_KP_6) * speedKeyb * 0.2;
        rotXAngle += input.GetKeyValue(SDL_SCANCODE_KP_8) * speedKeyb * 0.2;
        float zoom = 0;
        zoom += input.GetKeyValue(SDL_SCANCODE_KP_MINUS) * speedKeyb * 20;
        zoom -= input.GetKeyValue(SDL_SCANCODE_KP_PLUS) * speedKeyb * 20;

        float speedY = 0;
        speedY += input.GetKeyValue(SDL_SCANCODE_PAGEUP) * speedKeyb;
        speedY -= input.GetKeyValue(SDL_SCANCODE_PAGEDOWN) * speedKeyb;

        bool reset = false;
        if (input.ConsumeKeyPress(SDL_SCANCODE_KP_5))
        {
            reset = true;
        }
        // move - speedX,speedY,moveUp

        if (speedX || speedY || speedZ || rotYAngle || rotXAngle || reset)
        {
            // change position of cursor
            Vector3 position = _animCamera->Position();
            Matrix3 rot = _animCamera->Orientation();
            float scale = _animCamera->Scale();

            float dive = -atan2(rot.Direction().Y(), rot.Direction().SizeXZ());
            float heading = -atan2(rot.Direction().X(), rot.Direction().Z());
            dive += rotXAngle;

            float logScale = log(scale);
            logScale += zoom;
            scale = exp(logScale);

            if (reset)
            {
                dive = 0, scale = 1;
            }

            heading += rotYAngle;

            Matrix3Val rotY = Matrix3(MRotationY, heading);
            Matrix3Val rotX = Matrix3(MRotationX, dive);

            rot = rotY * rotX;

            Vector3 offset(speedX, speedY, speedZ);
            position += 10 * (rot * offset);

            float minY = GLOB_LAND->SurfaceY(position[0], position[2]);
            saturateMax(position[1], 0); // never allow under water camera
            if (position[1] < minY)
            {
                position[1] = minY;
            }

            _animCamera->SetScale(scale);
            _animCamera->SetOrientation(rot);
            _animCamera->SetPosition(position);

            _animCameraMoved = Glob.time;
        }
    }

    if (_animCamera && Glob.time - _animCameraMoved > 0.3 && _animCameraMoved >= _animCameraUpdated)
    {
        _animCameraUpdated = Glob.time;
        SendEvent(OBJECT_MOVE, _animCamera->ID(), &_animCamera->Transform());
    }

    if (_showNode)
    {
        x = toInt(position.X() * InvLandGrid);
        z = toInt(position.Z() * InvLandGrid);
        Point3 pos(x * LandGrid, land->SurfaceY(x * LandGrid, z * LandGrid), z * LandGrid);
        land->ShowArrow(pos);
    }

    char szComm[5];
    if (busy)
    {
        snprintf(szComm, sizeof(szComm), "%s", (const char*)"BUSY");
    }
    else if (_connected)
    {
        snprintf(szComm, sizeof(szComm), "%s", (const char*)"CONN");
    }
    else
    {
        snprintf(szComm, sizeof(szComm), "%s", (const char*)"DISC");
    }

    // Send selection to landscape
    if (MODE(m_wFlags) == MODE_SELECT)
    {
        land->SetSelection(m_MouseSelection);
        ptMin[1] = GLOB_LAND->SurfaceY(ptMin[0], ptMin[2]);
        ptMax[1] = GLOB_LAND->SurfaceY(ptMax[0], ptMax[2]);
        land->SetSelRectangle(ptMin, ptMax);
    }
    else
    {
        land->SetSelection(m_Selection.GetData());
    }

    // Display status line
    if (_cameraOnEdit)
    {
        Ref<Object> pNearest = land->NearestObject(position, NEAR_OBJECT, ObjectType(Primary | Network));
        int nNearest;
        char szNearest[64];
        if (pNearest)
        {
            nNearest = pNearest->ID();
            const char* nameFull = pNearest->GetShape()->Name();
            const char* name = findLastSep(nameFull);
            if (!name)
            {
                name = nameFull;
            }
            else
            {
                name++;
            }
            snprintf(szNearest, sizeof(szNearest), "%s", (const char*)name);
            char* ext = strrchr(szNearest, '.');
            if (ext)
            {
                *ext = 0;
            }
        }
        else
        {
            nNearest = 0;
            snprintf(szNearest, sizeof(szNearest), "%s", (const char*)"<none>");
        }
        char szMode[64];
        switch (MODE(m_wFlags))
        {
            case MODE_NORMAL:
                snprintf(szMode, sizeof(szMode), "%s", (const char*)"Normal");
                break;
            case MODE_MOVE:
                snprintf(szMode, sizeof(szMode), "%s", (const char*)"Move");
                break;
            case MODE_ROTATE:
                snprintf(szMode, sizeof(szMode), "%s", (const char*)"Rotate");
                break;
            case MODE_SELECT:
                snprintf(szMode, sizeof(szMode), "%s", (const char*)"Select");
                break;
        }
        if (m_wFlags & FLAG_POINTS)
        {
            strncat(szMode, " Pts", sizeof(szMode) - strlen(szMode) - 1);
        }
        if (m_wFlags & FLAG_PLANES)
        {
            strncat(szMode, " Plns", sizeof(szMode) - strlen(szMode) - 1);
        }
        if (m_wFlags & FLAG_YFIXED)
        {
            strncat(szMode, " YFix", sizeof(szMode) - strlen(szMode) - 1);
        }
        int nSelected = m_Selection.Size();
        GLOB_ENGINE->ShowMessage(INT_MAX, "%s Pos=(%.1f, %.1f, %.1f) Obj=%s(%d) Mode=%s x %d", szComm, position.X(),
                                 position.Y(), position.Z(), szNearest, nNearest, szMode, nSelected);
    }
    else
    {
        GLOB_ENGINE->ShowMessage(INT_MAX, "%s Camera=(%.1f, %.1f, %.1f)", szComm, _animCamera->Position().X(),
                                 _animCamera->Position().Y(), _animCamera->Position().Z());
    }

    Glob.dropDown = 0;

    Move(moveTrans); // finally apply move
    DirectionWorldToModel(_modelSpeed, _speed);
}

static void DrawRectangle(float xs, float zs, float xe, float ze)
{
    PackedColor color(Color(0, 1, 0, 0.3));

    Vector3 ss(xs, GLandscape->SurfaceYAboveWater(xs, zs) + 0.5, zs);
    Vector3 es(xe, GLandscape->SurfaceYAboveWater(xe, zs) + 0.5, zs);
    Vector3 se(xs, GLandscape->SurfaceYAboveWater(xs, ze) + 0.5, ze);
    Vector3 ee(xe, GLandscape->SurfaceYAboveWater(xe, ze) + 0.5, ze);

    static Ref<ObjectColored> obj;
    if (!obj)
    {
        Ref<LODShapeWithShadow> lShape = new LODShapeWithShadow();
        Shape* shape = new Shape;
        lShape->AddShape(shape, 0);

        // initalize lod level
        shape->ReallocTable(6);
        for (int i = 0; i < 6; i++)
        {
            shape->SetPos(i) = VZero;
            shape->SetClip(i, ClipAll);
            shape->SetNorm(i) = VUp;
        }
        shape->SetUV(0, 0, 0);
        shape->SetUV(1, 1, 0);
        shape->SetUV(2, 0, 1);
        shape->SetUV(3, 1, 0);
        shape->SetUV(4, 1, 1);
        shape->SetUV(5, 0, 1);

        // precalculate hints for possible optimizations
        shape->CalculateHints();
        lShape->CalculateHints();

        // change faces parameters
        Poly face;
        face.Init();
        face.SetN(3);
        face.SetSpecial(ClampU | ClampV);
        face.SetTexture(nullptr);
        face.Set(0, 0);
        face.Set(1, 1);
        face.Set(2, 2);
        shape->AddFace(face);
        face.Set(0, 3);
        face.Set(1, 4);
        face.Set(2, 5);
        shape->AddFace(face);

        shape->OrSpecial(IsAlpha | IsAlphaFog | BestMipmap);
        lShape->OrSpecial(IsColored | IsOnSurface);

        obj = new ObjectColored(lShape, -1);
        obj->SetOrientation(M3Identity);
        obj->SetConstantColor(color);
    }

    // use global object
    LODShape* lShape = obj->GetShape();
    Shape* shape = lShape->Level(0);

    shape->SetPos(0) = VZero;
    shape->SetPos(1) = se - ss;
    shape->SetPos(2) = ee - ss;
    shape->SetPos(3) = VZero;
    shape->SetPos(4) = ee - ss;
    shape->SetPos(5) = es - ss;
    Vector3 normal = (se - ss).CrossProduct(ee - ss).Normalized();
    shape->SetNorm(0) = normal;
    shape->SetNorm(1) = normal;
    shape->SetNorm(2) = normal;
    normal = (ee - es).CrossProduct(ee - se).Normalized();
    shape->SetNorm(3) = normal;
    shape->SetNorm(4) = normal;
    shape->SetNorm(5) = normal;

    lShape->SetAutoCenter(false);
    lShape->CalculateMinMax(true);

    shape->FindSections();

    obj->SetPosition(ss);
    obj->Draw(0, ClipAll, *obj);
}

void EditCursor::Draw(int forceLOD, ClipFlags clipFlags, const FrameBase& pos)
{
    base::Draw(forceLOD, clipFlags, pos);

    float xs = m_ptStart.X();
    float zs = m_ptStart.Z();
    float xe = m_ptCurrent.X();
    float ze = m_ptCurrent.Z();

    if (xs > xe)
    {
        swap(xs, xe);
    }
    if (zs > ze)
    {
        swap(zs, ze);
    }

    if (xe - xs < 0.01 || ze - zs < 0.01)
    {
        return;
    }

    float landRange = GLandscape->GetLandGrid();
    float invLandRange = GLandscape->GetInvLandGrid();

    int xxs = toIntCeil(xs * invLandRange);
    int zzs = toIntCeil(zs * invLandRange);
    int xxe = toIntFloor(xe * invLandRange);
    int zze = toIntFloor(ze * invLandRange);

    if (xxs > xxe)
    {
        if (zzs > zze)
        {
            DrawRectangle(xs, zs, xe, ze);
        }
        else
        {
            DrawRectangle(xs, zs, xe, zzs * landRange);
            for (int zz = zzs; zz < zze; zz++)
            {
                DrawRectangle(xs, zz * landRange, xe, (zz + 1) * landRange);
            }
            DrawRectangle(xs, zze * landRange, xe, ze);
        }
    }
    else
    {
        if (zzs > zze)
        {
            DrawRectangle(xs, zs, xxs * landRange, ze);
            for (int xx = xxs; xx < xxe; xx++)
            {
                DrawRectangle(xx * landRange, zs, (xx + 1) * landRange, ze);
            }
            DrawRectangle(xxe * landRange, zs, xe, ze);
        }
        else
        {
            float x = xxs * landRange;
            DrawRectangle(xs, zs, x, zzs * landRange);
            for (int zz = zzs; zz < zze; zz++)
            {
                DrawRectangle(xs, zz * landRange, x, (zz + 1) * landRange);
            }
            DrawRectangle(xs, zze * landRange, x, ze);

            for (int xx = xxs; xx < xxe; xx++)
            {
                float x = xx * landRange;
                float x1 = x + landRange;
                DrawRectangle(x, zs, x1, zzs * landRange);
                for (int zz = zzs; zz < zze; zz++)
                {
                    DrawRectangle(x, zz * landRange, x1, (zz + 1) * landRange);
                }
                DrawRectangle(x, zze * landRange, x1, ze);
            }

            x = xxe * landRange;
            DrawRectangle(x, zs, xe, zzs * landRange);
            for (int zz = zzs; zz < zze; zz++)
            {
                DrawRectangle(x, zz * landRange, xe, (zz + 1) * landRange);
            }
            DrawRectangle(x, zze * landRange, xe, ze);
        }
    }
}
