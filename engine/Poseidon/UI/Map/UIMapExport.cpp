#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/World/MapTypes.hpp>

#include <Poseidon/UI/Map/UIMap.hpp>
#include <Poseidon/Game/OperMap.hpp>

#include <Poseidon/IO/ParamFile/ParamFile.hpp>

#include <Poseidon/Dev/Debug/DebugTrap.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

using namespace Poseidon::Dev;

#define DRAW_BITMAPS 0
#define FLIP_VERTICAL 1

#define DRAW_TEXTS 0

static float Coef = 0.5f;
// note: LandSize is in this source always used as float
// #define LandSize toInt(LandRange * LandGrid)

#if defined _WIN32

static COLORREF colorSea = RGB(200, 230, 253);
static HBRUSH brushSea;
static COLORREF colorLand = RGB(255, 255, 255);
static HBRUSH brushLand;
static COLORREF colorForest = RGB(205, 230, 154);
static HBRUSH brushForest;
static COLORREF colorForestBorder = RGB(102, 205, 0);
static HPEN penForest;
static COLORREF colorRoads = RGB(123, 92, 72);
static HPEN penRoads;
static COLORREF colorCountlines = RGB(211, 186, 163);
static HPEN penCountlines;
static COLORREF colorCountlinesWater = RGB(128, 196, 255);
static HPEN penCountlinesWater;
static COLORREF colorGrid = RGB(112, 112, 83);
static HPEN penGrid;
static COLORREF colorSpot = RGB(0, 0, 0);
static HPEN penSpot;
#if DRAW_BITMAPS
static MapTypeInfo infoTree;
static MapTypeInfo infoSmallTree;
static MapTypeInfo infoBush;
static MapTypeInfo infoChurch;
static MapTypeInfo infoChapel;
static MapTypeInfo infoCross;
static MapTypeInfo infoRock;
static MapTypeInfo infoBunker;
static MapTypeInfo infoFortress;
static MapTypeInfo infoFountain;
static MapTypeInfo infoViewTower;
static MapTypeInfo infoLighthouse;
static MapTypeInfo infoQuay;
static MapTypeInfo infoFuelstation;
static MapTypeInfo infoHospital;
static MapTypeInfo infoBusStop;
static HBITMAP maskTree = nullptr;
static HBITMAP maskSmallTree = nullptr;
static HBITMAP maskBush = nullptr;
static HBITMAP maskChurch = nullptr;
static HBITMAP maskChapel = nullptr;
static HBITMAP maskCross = nullptr;
static HBITMAP maskRock = nullptr;
static HBITMAP maskBunker = nullptr;
static HBITMAP maskFortress = nullptr;
static HBITMAP maskFountain = nullptr;
static HBITMAP maskViewTower = nullptr;
static HBITMAP maskLighthouse = nullptr;
static HBITMAP maskQuay = nullptr;
static HBITMAP maskFuelstation = nullptr;
static HBITMAP maskHospital = nullptr;
static HBITMAP bmpTree = nullptr;
static HBITMAP bmpSmallTree = nullptr;
static HBITMAP bmpBush = nullptr;
static HBITMAP bmpChurch = nullptr;
static HBITMAP bmpChapel = nullptr;
static HBITMAP bmpCross = nullptr;
static HBITMAP bmpRock = nullptr;
static HBITMAP bmpBunker = nullptr;
static HBITMAP bmpFortress = nullptr;
static HBITMAP bmpFountain = nullptr;
static HBITMAP bmpViewTower = nullptr;
static HBITMAP bmpLighthouse = nullptr;
static HBITMAP bmpQuay = nullptr;
static HBITMAP bmpFuelstation = nullptr;
static HBITMAP bmpHospital = nullptr;
static HBITMAP bmpBusStop = nullptr;

#define Compose(r, g, b) \
    ((r) >> (8 - sRBits) << sRShift) | ((g) >> (8 - sGBits) << sGShift) | ((b) >> (8 - sBBits) << sBShift)

void CreateBitmap(HDC hDC, MapTypeInfo& info, HBITMAP& bmp, HBITMAP& mask)
{
    int sRShift = 10;
    int sGShift = 5;
    int sBShift = 0;
    int sRBits = 5;
    int sGBits = 5;
    int sBBits = 5;

    WORD white = Compose(255, 255, 255);
    WORD black = Compose(0, 0, 0);
    WORD color = Compose(info.color.R8(), info.color.G8(), info.color.B8());

    MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(info.icon, 0, 0);
    int size = toInt(info.size);
    float invSize = 1.0 / size;

    WORD* dataBmp = new WORD[Square(size)];
    WORD* dataMask = new WORD[Square(size)];

    for (int y = 0; y < size; y++)
    {
        float v = (y + 0.5) * invSize;
        for (int x = 0; x < size; x++)
        {
            float u = (x + 0.5) * invSize;
            Color src = info.icon->GetPixel(0, u, v);
            if (src.A() > 0)
            {
                dataMask[y * size + x] = white;
                dataBmp[y * size + x] = color;
            }
            else
            {
                dataMask[y * size + x] = black;
                dataBmp[y * size + x] = white;
            }
        }
    }

    struct Info : public BITMAPINFOHEADER
    {
        DWORD masks[3];
    } bmInfo;
    bmInfo.biSize = sizeof(BITMAPINFOHEADER);
    bmInfo.biWidth = size;
    bmInfo.biHeight = size;
    bmInfo.biPlanes = 1;
    bmInfo.biBitCount = 16;
    bmInfo.biCompression = BI_RGB;
    bmInfo.biSizeImage = size * size * sizeof(WORD);
    bmInfo.biXPelsPerMeter = 5000;
    bmInfo.biYPelsPerMeter = 5000;
    bmInfo.biClrUsed = 0;
    bmInfo.biClrImportant = 0;
    bmInfo.masks[0] = 0x1f << 10;
    bmInfo.masks[1] = 0x1f << 5;
    bmInfo.masks[2] = 0x1f << 0;

    // create bitmap object from prepared data
    bmp = CreateDIBitmap(hDC, &bmInfo, CBM_INIT, dataBmp, (BITMAPINFO*)&bmInfo, DIB_RGB_COLORS);
    if (bmp == 0)
    {
        LOG_DEBUG(UI, "Cannot create bitmap");
    }

    mask = CreateDIBitmap(hDC, &bmInfo, CBM_INIT, dataMask, (BITMAPINFO*)&bmInfo, DIB_RGB_COLORS);
    if (mask == 0)
    {
        LOG_DEBUG(UI, "Cannot create mask");
    }

    delete[] dataBmp;
    delete[] dataMask;
}

void DeleteBitmap(HBITMAP& bmp, HBITMAP& mask)
{
    if (bmp)
        DeleteObject(bmp);
    if (mask)
        DeleteObject(mask);
}

void DrawSign(HDC hDC, HBITMAP bitmap, HBITMAP mask, Vector3 pos, int size)
{
    if (bitmap == nullptr)
        return;
    int x = toInt(Coef * pos.X());
    int y = toInt(Coef * (-LandSize + pos.Z()));
    int newSize = size;

    HDC hDCBmp = CreateCompatibleDC(hDC);
    if (hDCBmp)
    {
        HGDIOBJ bitmapOld = SelectObject(hDCBmp, mask);
        StretchBlt(hDC, x - newSize / 2, y - newSize / 2, newSize, newSize, hDCBmp, 0, 0, size, size, SRCPAINT);
        SelectObject(hDCBmp, bitmap);
        StretchBlt(hDC, x - newSize / 2, y - newSize / 2, newSize, newSize, hDCBmp, 0, 0, size, size, SRCAND);
        SelectObject(hDCBmp, bitmapOld);
        DeleteDC(hDCBmp);
    }
}

#endif

float GetHeight(int x, int z)
{
    saturate(x, 1, LandRange - 2);
    saturate(z, 1, LandRange - 2);
    return GLOB_LAND->GetHeight(z, x);
}

void DrawSea(HDC hDC, int x, int z)
{
    RECT rect;
    rect.left = toInt(Coef * x * LandGrid);
    rect.right = toInt(Coef * (x + 1) * LandGrid);
    rect.top = toInt(Coef * (-LandSize + z * LandGrid));
    rect.bottom = toInt(Coef * (-LandSize + (z + 1) * LandGrid));

    float hTL = GetHeight(x, z);
    float hTR = GetHeight(x + 1, z);
    float hBL = GetHeight(x, z + 1);
    float hBR = GetHeight(x + 1, z + 1);

    if (hTL <= 0 && hTR <= 0 && hBL <= 0 && hBR <= 0)
    {
        FillRect(hDC, &rect, brushSea);
    }
    else if (hTL >= 0 && hTR >= 0 && hBL >= 0 && hBR >= 0)
    {
        FillRect(hDC, &rect, brushLand);
    }
    else
    {
        POINT ptSea[5];
        POINT ptLand[5];
        int nSea = 0;
        int nLand = 0;
        if (hTL <= 0)
        {
            ptSea[nSea].x = rect.left;
            ptSea[nSea].y = rect.top;
            nSea++;
            if (hTR > 0)
            {
                goto TLTR;
            }
        }
        else
        {
            ptLand[nLand].x = rect.left;
            ptLand[nLand].y = rect.top;
            nLand++;
            if (hTR <= 0)
            {
            TLTR:
                int d = -toInt(Coef * LandGrid * hTL / (hTR - hTL));
                ptLand[nLand].x = rect.left + d;
                ptLand[nLand].y = rect.top;
                nLand++;
                ptSea[nSea].x = rect.left + d;
                ptSea[nSea].y = rect.top;
                nSea++;
            }
        }
        if (hTR <= 0)
        {
            ptSea[nSea].x = rect.right;
            ptSea[nSea].y = rect.top;
            nSea++;
            if (hBR > 0)
            {
                goto TRBR;
            }
        }
        else
        {
            ptLand[nLand].x = rect.right;
            ptLand[nLand].y = rect.top;
            nLand++;
            if (hBR <= 0)
            {
            TRBR:
                int d = -toInt(Coef * LandGrid * hTR / (hBR - hTR));
                ptLand[nLand].x = rect.right;
                ptLand[nLand].y = rect.top + d;
                nLand++;
                ptSea[nSea].x = rect.right;
                ptSea[nSea].y = rect.top + d;
                nSea++;
            }
        }
        if (hBR <= 0)
        {
            ptSea[nSea].x = rect.right;
            ptSea[nSea].y = rect.bottom;
            nSea++;
            if (hTL > 0)
            {
                goto BRTL;
            }
        }
        else
        {
            ptLand[nLand].x = rect.right;
            ptLand[nLand].y = rect.bottom;
            nLand++;
            if (hTL <= 0)
            {
            BRTL:
                int d = -toInt(Coef * LandGrid * hBR / (hTL - hBR));
                ptLand[nLand].x = rect.right - d;
                ptLand[nLand].y = rect.bottom - d;
                nLand++;
                ptSea[nSea].x = rect.right - d;
                ptSea[nSea].y = rect.bottom - d;
                nSea++;
            }
        }
        if (nSea >= 2)
        {
            ptSea[nSea].x = ptSea[0].x;
            ptSea[nSea].y = ptSea[0].y;
            nSea++;
            SelectObject(hDC, brushSea);
            Polygon(hDC, ptSea, nSea);
        }
        if (nLand >= 2)
        {
            ptLand[nLand].x = ptLand[0].x;
            ptLand[nLand].y = ptLand[0].y;
            nLand++;
            SelectObject(hDC, brushLand);
            Polygon(hDC, ptLand, nLand);
        }

        nSea = 0;
        nLand = 0;
        if (hTL <= 0)
        {
            ptSea[nSea].x = rect.left;
            ptSea[nSea].y = rect.top;
            nSea++;
            if (hBR > 0)
            {
                goto TLBR;
            }
        }
        else
        {
            ptLand[nLand].x = rect.left;
            ptLand[nLand].y = rect.top;
            nLand++;
            if (hBR <= 0)
            {
            TLBR:
                int d = -toInt(Coef * LandGrid * hTL / (hBR - hTL));
                ptLand[nLand].x = rect.left + d;
                ptLand[nLand].y = rect.top + d;
                nLand++;
                ptSea[nSea].x = rect.left + d;
                ptSea[nSea].y = rect.top + d;
                nSea++;
            }
        }
        if (hBR <= 0)
        {
            ptSea[nSea].x = rect.right;
            ptSea[nSea].y = rect.bottom;
            nSea++;
            if (hBL > 0)
            {
                goto BRBL;
            }
        }
        else
        {
            ptLand[nLand].x = rect.right;
            ptLand[nLand].y = rect.bottom;
            nLand++;
            if (hBL <= 0)
            {
            BRBL:
                int d = -toInt(Coef * LandGrid * hBR / (hBL - hBR));
                ptLand[nLand].x = rect.right - d;
                ptLand[nLand].y = rect.bottom;
                nLand++;
                ptSea[nSea].x = rect.right - d;
                ptSea[nSea].y = rect.bottom;
                nSea++;
            }
        }
        if (hBL <= 0)
        {
            ptSea[nSea].x = rect.left;
            ptSea[nSea].y = rect.bottom;
            nSea++;
            if (hTL > 0)
            {
                goto BLTL;
            }
        }
        else
        {
            ptLand[nLand].x = rect.left;
            ptLand[nLand].y = rect.bottom;
            nLand++;
            if (hTL <= 0)
            {
            BLTL:
                int d = -toInt(Coef * LandGrid * hBL / (hTL - hBL));
                ptLand[nLand].x = rect.left;
                ptLand[nLand].y = rect.bottom - d;
                nLand++;
                ptSea[nSea].x = rect.left;
                ptSea[nSea].y = rect.bottom - d;
                nSea++;
            }
        }
        if (nSea >= 2)
        {
            ptSea[nSea].x = ptSea[0].x;
            ptSea[nSea].y = ptSea[0].y;
            nSea++;
            SelectObject(hDC, brushSea);
            Polygon(hDC, ptSea, nSea);
        }
        if (nLand >= 2)
        {
            ptLand[nLand].x = ptLand[0].x;
            ptLand[nLand].y = ptLand[0].y;
            nLand++;
            SelectObject(hDC, brushLand);
            Polygon(hDC, ptLand, nLand);
        }
    }
}

void DrawForests(HDC hDC, int x, int z)
{
    RECT rect;
    rect.left = toInt(Coef * x * LandGrid);
    rect.right = toInt(Coef * (x + 1) * LandGrid);
    rect.top = toInt(Coef * (-LandSize + z * LandGrid));
    rect.bottom = toInt(Coef * (-LandSize + (z + 1) * LandGrid));

    const ObjectList& list = GLOB_LAND->GetObjects(z, x);
    for (int o = 0; o < list.Size(); o++)
    {
        Object* obj = list[o];
        if (obj == nullptr)
        {
            continue;
        }
        if (obj->GetType() == Primary)
        {
            switch (obj->GetShape()->GetMapType())
            {
                case MapForestTriangle:
                {
                    const int n = 3;
                    POINT vs[n];
                    Vector3 dir = obj->Direction();
                    float angle = atan2(dir.X(), dir.Z());
                    switch (toInt(angle * 2.0 / H_PI))
                    {
                        case -1:
                            vs[0].x = rect.left;
                            vs[0].y = rect.top;
                            vs[1].x = rect.left;
                            vs[1].y = rect.bottom;
                            vs[2].x = rect.right;
                            vs[2].y = rect.top;
                            break;
                        case 0:
                            vs[0].x = rect.left;
                            vs[0].y = rect.bottom;
                            vs[1].x = rect.right;
                            vs[1].y = rect.bottom;
                            vs[2].x = rect.left;
                            vs[2].y = rect.top;
                            break;
                        case 1:
                            vs[0].x = rect.left;
                            vs[0].y = rect.bottom;
                            vs[1].x = rect.right;
                            vs[1].y = rect.bottom;
                            vs[2].x = rect.right;
                            vs[2].y = rect.top;
                            break;
                        case -2:
                        case 2:
                            vs[0].x = rect.left;
                            vs[0].y = rect.top;
                            vs[1].x = rect.right;
                            vs[1].y = rect.bottom;
                            vs[2].x = rect.right;
                            vs[2].y = rect.top;
                            break;
                    }
                    Polygon(hDC, vs, n);
                }
                    return;
                case MapForestSquare:
                {
                    FillRect(hDC, &rect, brushForest);
                }
                    return; // only one forest in square is enabled
            }
        }
    }
}

void DrawForestBorders(HDC hDC, int x, int z)
{
    RECT rect;
    rect.left = toInt(Coef * x * LandGrid);
    rect.right = toInt(Coef * (x + 1) * LandGrid);
    rect.top = toInt(Coef * (-LandSize + z * LandGrid));
    rect.bottom = toInt(Coef * (-LandSize + (z + 1) * LandGrid));

    const ObjectList& list = GLOB_LAND->GetObjects(z, x);
    for (int o = 0; o < list.Size(); o++)
    {
        Object* obj = list[o];
        if (obj == nullptr)
        {
            continue;
        }
        if (obj->GetType() == Primary)
        {
            switch (obj->GetShape()->GetMapType())
            {
                case MapForestTriangle:
                {
                    Vector3 dir = obj->Direction();
                    float angle = atan2(dir.X(), dir.Z());
                    switch (toInt(angle * 2.0 / H_PI))
                    {
                        case -1:
                        case 1:
                            MoveToEx(hDC, rect.left, rect.bottom, nullptr);
                            LineTo(hDC, rect.right, rect.top);
                            break;
                        case -2:
                        case 0:
                        case 2:
                            MoveToEx(hDC, rect.left, rect.top, nullptr);
                            LineTo(hDC, rect.right, rect.bottom);
                            break;
                    }
                }
                break;
                case MapForestSquare:
                {
                    GeographyInfo geogr = GLOB_LAND->GetGeography(x, z + 1);
                    if (!geogr.u.forestInner && !geogr.u.forestOuter)
                    {
                        MoveToEx(hDC, rect.left, rect.bottom, nullptr);
                        LineTo(hDC, rect.right, rect.bottom);
                    }
                    geogr = GLOB_LAND->GetGeography(x, z - 1);
                    if (!geogr.u.forestInner && !geogr.u.forestOuter)
                    {
                        MoveToEx(hDC, rect.left, rect.top, nullptr);
                        LineTo(hDC, rect.right, rect.top);
                    }
                    geogr = GLOB_LAND->GetGeography(x - 1, z);
                    if (!geogr.u.forestInner && !geogr.u.forestOuter)
                    {
                        MoveToEx(hDC, rect.left, rect.bottom, nullptr);
                        LineTo(hDC, rect.left, rect.top);
                    }
                    geogr = GLOB_LAND->GetGeography(x + 1, z);
                    if (!geogr.u.forestInner && !geogr.u.forestOuter)
                    {
                        MoveToEx(hDC, rect.right, rect.bottom, nullptr);
                        LineTo(hDC, rect.right, rect.top);
                    }
                }
            }
        }
    }
}

void DrawRoads(HDC hDC, int x, int z)
{
    const ObjectList& list = GLOB_LAND->GetObjects(z, x);
    for (int o = 0; o < list.Size(); o++)
    {
        Object* obj = list[o];
        if (obj == nullptr)
        {
            continue;
        }
        if (obj->GetType() == Network)
        {
            Vector3 ptTL = obj->GetShape()->MemoryPoint("LB");
            Vector3 ptTR = obj->GetShape()->MemoryPoint("PB");
            Vector3 ptBL = obj->GetShape()->MemoryPoint("LE");
            Vector3 ptBR = obj->GetShape()->MemoryPoint("PE");
            Vector3 mapTL = obj->PositionModelToWorld(ptTL);
            Vector3 mapTR = obj->PositionModelToWorld(ptTR);
            Vector3 mapBL = obj->PositionModelToWorld(ptBL);
            Vector3 mapBR = obj->PositionModelToWorld(ptBR);
            MoveToEx(hDC, toInt(Coef * mapTL.X()), toInt(Coef * (-LandSize + mapTL.Z())), nullptr);
            LineTo(hDC, toInt(Coef * mapBL.X()), toInt(Coef * (-LandSize + mapBL.Z())));
            MoveToEx(hDC, toInt(Coef * mapTR.X()), toInt(Coef * (-LandSize + mapTR.Z())), nullptr);
            LineTo(hDC, toInt(Coef * mapBR.X()), toInt(Coef * (-LandSize + mapBR.Z())));
        }
    }
}

void DrawObjects(HDC hDC, int x, int z)
{
    const ObjectList& list = GLOB_LAND->GetObjects(z, x);
    for (int o = 0; o < list.Size(); o++)
    {
        Object* obj = list[o];
        if (obj == nullptr)
        {
            continue;
        }
        if (obj->GetType() == Primary)
        {
            switch (obj->GetShape()->GetMapType())
            {
#if DRAW_BITMAPS
                case MapTree:
                    DrawSign(hDC, bmpTree, maskTree, obj->Position(), toInt(infoTree.size));
                    break;
                case MapSmallTree:
                    DrawSign(hDC, bmpSmallTree, maskSmallTree, obj->Position(), toInt(infoSmallTree.size));
                    break;
                case MapBush:
                    DrawSign(hDC, bmpBush, maskBush, obj->Position(), toInt(infoBush.size));
                    break;
                case MapChurch:
                    DrawSign(hDC, bmpChurch, maskChurch, obj->Position(), toInt(infoChurch.size));
                    break;
                case MapChapel:
                    DrawSign(hDC, bmpChapel, maskChapel, obj->Position(), toInt(infoChapel.size));
                    break;
                case MapCross:
                    DrawSign(hDC, bmpCross, maskCross, obj->Position(), toInt(infoCross.size));
                    break;
                case MapRock:
                    DrawSign(hDC, bmpRock, maskRock, obj->Position(), toInt(infoRock.size));
                    break;
                case MapBunker:
                    DrawSign(hDC, bmpBunker, maskBunker, obj->Position(), toInt(infoBunker.size));
                    break;
                case MapFortress:
                    DrawSign(hDC, bmpFortress, maskFortress, obj->Position(), toInt(infoFortress.size));
                    break;
                case MapFountain:
                    DrawSign(hDC, bmpFountain, maskFountain, obj->Position(), toInt(infoFountain.size));
                    break;
                case MapViewTower:
                    DrawSign(hDC, bmpViewTower, maskViewTower, obj->Position(), toInt(infoViewTower.size));
                    break;
                case MapLighthouse:
                    DrawSign(hDC, bmpLighthouse, maskLighthouse, obj->Position(), toInt(infoLighthouse.size));
                    break;
                case MapQuay:
                    DrawSign(hDC, bmpQuay, maskQuay, obj->Position(), toInt(infoQuay.size));
                    break;
                case MapFuelstation:
                    DrawSign(hDC, bmpFuelstation, maskFuelstation, obj->Position(), toInt(infoFuelstation.size));
                    break;
                case MapHospital:
                    DrawSign(hDC, bmpHospital, maskHospital, obj->Position(), toInt(infoHospital.size));
                    break;
                case MapBusStop:
                    DrawSign(hDC, bmpBusStop, maskBusStop, obj->Position(), toInt(infoBusStop.size));
                    break;
#else
                case MapTree:
                case MapSmallTree:
                case MapBush:
                case MapChurch:
                case MapChapel:
                case MapCross:
                case MapRock:
                case MapBunker:
                case MapFortress:
                case MapFountain:
                case MapViewTower:
                case MapLighthouse:
                case MapQuay:
                case MapFuelstation:
                case MapHospital:
#endif
                case MapBuilding:
                case MapHouse:
                case MapFence:
                case MapWall:
                {
                    const Vector3* minmax = obj->GetShape()->MinMax();
                    Vector3 ptTL(minmax[0].X(), 0, minmax[0].Z());
                    Vector3 ptTR(minmax[1].X(), 0, minmax[0].Z());
                    Vector3 ptBL(minmax[0].X(), 0, minmax[1].Z());
                    Vector3 ptBR(minmax[1].X(), 0, minmax[1].Z());
                    Vector3 mapTL = obj->PositionModelToWorld(ptTL);
                    Vector3 mapTR = obj->PositionModelToWorld(ptTR);
                    Vector3 mapBL = obj->PositionModelToWorld(ptBL);
                    Vector3 mapBR = obj->PositionModelToWorld(ptBR);

                    const int n = 4;
                    POINT vs[n];
                    // 0
                    vs[0].x = toInt(Coef * mapTL.X());
                    vs[0].y = toInt(Coef * (-LandSize + mapTL.Z()));
                    // 1
                    vs[1].x = toInt(Coef * mapBL.X());
                    vs[1].y = toInt(Coef * (-LandSize + mapBL.Z()));
                    // 2
                    vs[2].x = toInt(Coef * mapBR.X());
                    vs[2].y = toInt(Coef * (-LandSize + mapBR.Z()));
                    // 3
                    vs[3].x = toInt(Coef * mapTR.X());
                    vs[3].y = toInt(Coef * (-LandSize + mapTR.Z()));

                    PackedColor color = obj->GetShape()->Color();
                    HBRUSH brush = CreateSolidBrush(RGB(color.R8(), color.G8(), color.B8()));
                    HGDIOBJ brushOld = SelectObject(hDC, brush);
                    Polygon(hDC, vs, n);
                    SelectObject(hDC, brushOld);
                    DeleteObject(brush);
                }
                break;
            }
        }
    }
}

void DrawLines(HDC hDC, POINT* pt, float* height, float step, float minLevel, float maxLevel)
{
    float invStep = 1.0 / step;

    int n0, n1, n2, nt;
    int xs, ys, xe, ye;
    for (int t = 0; t < 2; t++)
    {
        // t = 0, 1
        // draw triangel <t, t+1, t+2>
        n0 = t;
        n1 = t + 1;
        n2 = t + 2;
        // sort vertices by height
        if (height[n0] > height[n1])
        { // swap n0, n1
            nt = n0;
            n0 = n1;
            n1 = nt;
        }
        if (height[n0] > height[n2])
        { // swap n0, n2
            nt = n0;
            n0 = n2;
            n2 = nt;
        }
        if (height[n1] > height[n2])
        { // swap n1, n2
            nt = n1;
            n1 = n2;
            n2 = nt;
        }

        float level = step * toIntCeil(height[n0] * invStep);
        saturateMax(level, minLevel);
        float toLevel = floatMin(height[n2], maxLevel);
        for (; level < toLevel; level += step)
        {
            // draw one line (at level <level>)
            float coef = (level - height[n0]) * (1.0 / (height[n2] - height[n0]));
            xe = pt[n0].x + toInt(coef * (pt[n2].x - pt[n0].x));
            ye = pt[n0].y + toInt(coef * (pt[n2].y - pt[n0].y));
            if (level == height[n1])
            {
                xs = pt[n1].x;
                ys = pt[n1].y;
            }
            else if (level < height[n1])
            {
                float coef = (level - height[n0]) * (1.0 / (height[n1] - height[n0]));
                xs = pt[n0].x + coef * (pt[n1].x - pt[n0].x);
                ys = pt[n0].y + coef * (pt[n1].y - pt[n0].y);
            }
            else
            {
                float coef = (level - height[n1]) * (1.0 / (height[n2] - height[n1]));
                xs = pt[n1].x + coef * (pt[n2].x - pt[n1].x);
                ys = pt[n1].y + coef * (pt[n2].y - pt[n1].y);
            }
            // draw line from <xs, ys> to <xe, ye>
            MoveToEx(hDC, toInt(xs), toInt(ys), nullptr);
            LineTo(hDC, toInt(xe), toInt(ye));
        }
    }
}

void DrawCountlines(HDC hDC, int x, int z)
{
    RECT rect;
    rect.left = toInt(Coef * x * LandGrid);
    rect.right = toInt(Coef * (x + 1) * LandGrid);
    rect.top = toInt(Coef * (-LandSize + z * LandGrid));
    rect.bottom = toInt(Coef * (-LandSize + (z + 1) * LandGrid));
    POINT pt[4];
    pt[0].x = rect.left;
    pt[0].y = rect.bottom;
    pt[1].x = rect.right;
    pt[1].y = rect.bottom;
    pt[2].x = rect.left;
    pt[2].y = rect.top;
    pt[3].x = rect.right;
    pt[3].y = rect.top;

    float height[4];
    height[0] = GLOB_LAND->GetHeight(z + 1, x);
    height[1] = GLOB_LAND->GetHeight(z + 1, x + 1);
    height[2] = GLOB_LAND->GetHeight(z, x);
    height[3] = GLOB_LAND->GetHeight(z, x + 1);

    // step, minLevel, maxLevel
    SelectObject(hDC, penCountlinesWater);
    DrawLines(hDC, pt, height, 10, -10000, 5);
    SelectObject(hDC, penCountlines);
    DrawLines(hDC, pt, height, 10, 10, 10000);
}

void DrawName(HDC hDC, const ParamEntry& cls)
{
    float xx = (cls >> "position")[0];
    float yy = (cls >> "position")[1];
    int x = toInt(Coef * xx);
    int y = toInt(Coef * (-LandSize + yy));

    RString text = cls >> "name";
    SIZE size;
    GetTextExtentPoint32(hDC, text, text.GetLength(), &size);
    y += size.cy / 2;

    TextOut(hDC, x, y, text, text.GetLength());
}

#if DRAW_TEXTS
void TextOutCenter(HDC hDC, int x, int y, const char* buffer, float div)
{
    int width = 0;
    for (const char* p = buffer; *p != 0; p++)
    {
        int wChar;
        GetCharWidth32(hDC, *p, *p, &wChar);
        width += toIntFloor(wChar / div);
    }

    x -= width / 2;

    for (const char* p = buffer; *p != 0; p++)
    {
        TextOut(hDC, x, y, p, 1);
        int wChar;
        GetCharWidth32(hDC, *p, *p, &wChar);
        x += toIntFloor(wChar / div);
    }
}
#endif

void DrawMount(HDC hDC, Vector3Par pos)
{
    int x = toInt(Coef * pos.X());
    int y = toInt(Coef * (-LandSize + pos.Z()));

    SelectObject(hDC, penSpot);

    MoveToEx(hDC, toInt(x), toInt(y - 1), nullptr);
    LineTo(hDC, toInt(x + 1), toInt(y - 1));
    LineTo(hDC, toInt(x + 1), toInt(y + 1));
    LineTo(hDC, toInt(x), toInt(y + 1));
    LineTo(hDC, toInt(x), toInt(y - 1));

#if DRAW_TEXTS
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%.0f", pos.Y());

    TextOutCenter(hDC, x, y + 15, buffer, 5);
#endif
}

void DrawGrid(HDC hDC)
{
    for (int i = 0; i < 100; i++)
    {
        int x = toInt(Coef * 0.01 * i * LandSize);
        MoveToEx(hDC, x, toInt(Coef * -LandSize), nullptr);
        LineTo(hDC, x, 0);
#if DRAW_TEXTS
        char buffer[3];
        buffer[0] = 'A' + i / 10;
        buffer[1] = 'a' + i % 10;
        buffer[2] = 0;
        x += toInt(Coef * 0.005 * LandSize);
        TextOutCenter(hDC, x, 0, buffer, 2.5);
#endif
    }

    for (int i = 0; i < 100; i++)
    {
        int y = toInt(Coef * (-LandSize + 0.01 * i * LandSize));
        MoveToEx(hDC, 0, y, nullptr);
        LineTo(hDC, toInt(Coef * LandSize), y);
#if DRAW_TEXTS
        char buffer[3];
        buffer[0] = '0' + (99 - i) / 10;
        buffer[1] = '0' + (99 - i) % 10;
        buffer[2] = 0;
        y += toInt(Coef * 0.005 * LandSize);
        TextOutCenter(hDC, 15, y + 10, buffer, 3);
#endif
    }
}

void ExportWMF(const char* name, bool grid)
{
    GDebugger.PauseCheckingAlive();

    RECT rect;
    rect.left = 0;
    rect.right = toInt(Coef * LandSize);
    rect.top = 0;
    rect.bottom = toInt(Coef * LandSize);
    RString description = "Cold War Assault Map";
    HDC hDC = CreateEnhMetaFile(nullptr, name, &rect, description);
    SetMapMode(hDC, MM_HIMETRIC);
    SetBkMode(hDC, TRANSPARENT);

    brushSea = CreateSolidBrush(colorSea);
    brushLand = CreateSolidBrush(colorLand);
    brushForest = CreateSolidBrush(colorForest);
    penForest = CreatePen(PS_SOLID, 1, colorForestBorder);
    penRoads = CreatePen(PS_SOLID, 1, colorRoads);
    penCountlines = CreatePen(PS_SOLID, 1, colorCountlines);
    penCountlinesWater = CreatePen(PS_SOLID, 1, colorCountlinesWater);
    penGrid = CreatePen(PS_SOLID, 1, colorGrid);
    penSpot = CreatePen(PS_SOLID, 1, colorSpot);

#if DRAW_BITMAPS
    const ParamEntry& cls = Res >> "RscMapControl";
    infoTree.Load(cls >> "Tree");
    infoSmallTree.Load(cls >> "SmallTree");
    infoBush.Load(cls >> "Bush");
    infoChurch.Load(cls >> "Church");
    infoChapel.Load(cls >> "Chapel");
    infoCross.Load(cls >> "Cross");
    infoRock.Load(cls >> "Rock");
    infoBunker.Load(cls >> "Bunker");
    infoFortress.Load(cls >> "Fortress");
    infoFountain.Load(cls >> "Fountain");
    infoViewTower.Load(cls >> "ViewTower");
    infoLighthouse.Load(cls >> "Lighthouse");
    infoQuay.Load(cls >> "Quay");
    infoFuelstation.Load(cls >> "Fuelstation");
    infoHospital.Load(cls >> "Hospital");
    CreateBitmap(hDC, infoTree, bmpTree, maskTree);
    CreateBitmap(hDC, infoSmallTree, bmpSmallTree, maskSmallTree);
    CreateBitmap(hDC, infoBush, bmpBush, maskBush);
    CreateBitmap(hDC, infoChurch, bmpChurch, maskChurch);
    CreateBitmap(hDC, infoChapel, bmpChapel, maskChapel);
    CreateBitmap(hDC, infoCross, bmpCross, maskCross);
    CreateBitmap(hDC, infoRock, bmpRock, maskRock);
    CreateBitmap(hDC, infoBunker, bmpBunker, maskBunker);
    CreateBitmap(hDC, infoFortress, bmpFortress, maskFortress);
    CreateBitmap(hDC, infoFountain, bmpFountain, maskFountain);
    CreateBitmap(hDC, infoViewTower, bmpViewTower, maskViewTower);
    CreateBitmap(hDC, infoLighthouse, bmpLighthouse, maskLighthouse);
    CreateBitmap(hDC, infoQuay, bmpQuay, maskQuay);
    CreateBitmap(hDC, infoFuelstation, bmpFuelstation, maskFuelstation);
    CreateBitmap(hDC, infoHospital, bmpHospital, maskHospital);
    CreateBitmap(hDC, infoBusStop, bmpBusStop, maskBusStop);
#endif

    HGDIOBJ brushOld = SelectObject(hDC, GetStockObject(NULL_BRUSH));
    HGDIOBJ penOld = SelectObject(hDC, GetStockObject(NULL_PEN));

    for (int z = 0; z < LandRange; z++)
    {
        for (int x = 0; x < LandRange; x++)
        {
            DrawSea(hDC, x, z);
        }
    }

    SelectObject(hDC, brushForest);
    for (int z = 0; z < LandRange; z++)
    {
        for (int x = 0; x < LandRange; x++)
        {
            DrawForests(hDC, x, z);
        }
    }

    SelectObject(hDC, penForest);
    for (int z = 0; z < LandRange; z++)
    {
        for (int x = 0; x < LandRange; x++)
        {
            DrawForestBorders(hDC, x, z);
        }
    }

    for (int z = 0; z < LandRange; z++)
    {
        for (int x = 0; x < LandRange; x++)
        {
            DrawCountlines(hDC, x, z);
        }
    }

    SelectObject(hDC, penRoads);
    for (int z = 0; z < LandRange; z++)
    {
        for (int x = 0; x < LandRange; x++)
        {
            DrawRoads(hDC, x, z);
        }
    }

    SelectObject(hDC, GetStockObject(NULL_PEN));
    for (int z = 0; z < LandRange; z++)
    {
        for (int x = 0; x < LandRange; x++)
        {
            DrawObjects(hDC, x, z);
        }
    }

    HFONT font =
        CreateFont(toInt(Coef * 30), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, "Courier New");
    HGDIOBJ fontOld = SelectObject(hDC, font);

    // mountains
    const AutoArray<Vector3>& mountains = GLandscape->GetMountains();
    float minDist2 = Square(25);
    for (int i = 0; i < mountains.Size(); i++)
    {
        const Vector3& pos = mountains[i];
        bool skip = false;
        for (int j = 0; j < i; j++)
        {
            if (pos.DistanceXZ2(mountains[j]) < minDist2)
            {
                skip = true;
                break;
            }
        }
        if (!skip)
        {
            DrawMount(hDC, pos);
        }
    }

    HFONT fontGrid =
        CreateFont(toInt(Coef * 50), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    if (grid)
    {
        SetTextColor(hDC, colorGrid);
        SelectObject(hDC, fontGrid);
        SelectObject(hDC, penGrid);
        DrawGrid(hDC);
    }

    SelectObject(hDC, brushOld);
    SelectObject(hDC, penOld);
    SelectObject(hDC, fontOld);

    DeleteObject(brushSea);
    DeleteObject(brushLand);
    DeleteObject(brushForest);
    DeleteObject(penForest);
    DeleteObject(penRoads);
    DeleteObject(penCountlines);
    DeleteObject(penCountlinesWater);
    DeleteObject(penGrid);
    DeleteObject(penSpot);
    DeleteObject(font);
    DeleteObject(fontGrid);

#if DRAW_BITMAPS
    DeleteBitmap(bmpTree, maskTree);
    DeleteBitmap(bmpSmallTree, maskSmallTree);
    DeleteBitmap(bmpBush, maskBush);
    DeleteBitmap(bmpChurch, maskChurch);
    DeleteBitmap(bmpChapel, maskChapel);
    DeleteBitmap(bmpCross, maskCross);
    DeleteBitmap(bmpRock, maskRock);
    DeleteBitmap(bmpBunker, maskBunker);
    DeleteBitmap(bmpFortress, maskFortress);
    DeleteBitmap(bmpFountain, maskFountain);
    DeleteBitmap(bmpViewTower, maskViewTower);
    DeleteBitmap(bmpLighthouse, maskLighthouse);
    DeleteBitmap(bmpQuay, maskQuay);
    DeleteBitmap(bmpFuelstation, maskFuelstation);
    DeleteBitmap(bmpHospital, maskHospital);
    DeleteBitmap(bmpBusStop, maskBusStop);
#endif

    CloseEnhMetaFile(hDC);

    GDebugger.ResumeCheckingAlive();
}

#define BUF_OPT (64L * 1024)
#define BUF_MIN (1024)

static int fputiw(int W, FILE* f)
{
    if (fputc((byte)W, f) < 0)
    {
        return EOF;
    }
    return fputc(W >> 8, f);
}
static int fputi24(long W, FILE* f)
{
    if (fputc((byte)W, f) < 0)
    {
        return EOF;
    }
    if (fputc((byte)(W >> 8), f) < 0)
    {
        return EOF;
    }
    if (fputc((byte)(W >> 16), f) < 0)
    {
        return EOF;
    }
    return 0;
}

enum
{
    MaxRep = 128
};

static int UlozBBlok(word* Blok, int* LBlok, FILE* f)
{
    if (*LBlok > 0)
    {
        int L = *LBlok;
        if (fputc(L - 1, f) < 0)
        {
            return EOF;
        }
        while (--L >= 0)
        {
            if (fputc(*Blok++, f) < 0)
            {
                return EOF;
            }
        }
    }
    *LBlok = 0;
    return 0;
}

static int PridejBBlok(word* Blok, int* LBlok, FILE* f, word W)
{
    Blok[*LBlok] = W;
    (*LBlok)++;
    if (*LBlok >= MaxRep)
    {
        return UlozBBlok(Blok, LBlok, f);
    }
    return 0;
}

static int PridejBRep(word* Blok, int* LBlok, FILE* f, word LW, int rep)
{
    if (rep > 0)
    {
        if (rep < 3)
        {
            while (--rep >= 0)
            {
                if (PridejBBlok(Blok, LBlok, f, LW) < 0)
                {
                    return EOF;
                }
            }
        }
        else
        {
            if (*LBlok > 0)
            {
                if (UlozBBlok(Blok, LBlok, f) < 0)
                {
                    return EOF;
                }
            }
            if (fputc(rep - 1 + 0x80, f) < 0)
            {
                return EOF;
            }
            if (fputc(LW, f) < 0)
            {
                return EOF;
            }
        }
    }
    return 0;
}

static int SavePACB(FILE* f, byte* Buf, long L)
{
    int LBloku = 0;
    word LW = 0;
    int rep = 0;
    word* Blok = new word[MaxRep];
    if (!Blok)
    {
        return -1;
    }

    while (L > 0)
    {
        word A = *Buf++;
        L--;
        if (rep < MaxRep && A == LW)
        {
            rep++;
        }
        else
        {
            if (PridejBRep(Blok, &LBloku, f, LW, rep) < 0)
            {
                goto Error;
            }
            LW = A, rep = 1;
        }
    }
    if (PridejBRep(Blok, &LBloku, f, LW, rep) < 0)
    {
        goto Error;
    }
    if (UlozBBlok(Blok, &LBloku, f) < 0)
    {
        goto Error;
    }

    delete[] Blok;
    return 0;
Error:
    delete[] Blok;
    return -1;
}

int SavePAC256(const char* N, int W, int H, void* _Buf, unsigned long* RGB, int NC) /* paleta - NC barev */
{                                                                                   /* run-length compress. */
    byte* Buf = (byte*)_Buf;
    // word *Blok=mallocSpc(MaxRep*sizeof(*Blok),'PACS');
    FILE* f;
    int r;
    // if( !Blok ) return -1;
    f = fopen(N, "wb");
    r = -1;
    if (f)
    {
        long L = (long)W * H;
        int I;
        if (setvbuf(f, nullptr, _IOFBF, BUF_OPT) < 0)
        {
            if (setvbuf(f, nullptr, _IOFBF, BUF_MIN) < 0)
            {
                goto Error;
            }
        }
        fputc(0, f); /*  Number of Characters in Identification Field. */
        fputc(1, f); /*  Color Map Type. */
        fputc(9, f); /*  Image Type Code. 1 (index) + 8 (compressed) */
        /*   3   : Color Map Specification. */
        fputiw(0, f);  /* beg */
        fputiw(NC, f); /* count */
        fputc(24, f);  /* bit RGBA */
        /*   8   : Image Specification. */
        fputiw(0, f);   /*  X Origin of Image. */
        fputiw(0, f);   /*  Y Origin of Image. */
        fputiw(W, f);   /*  Width Image. */
        fputiw(H, f);   /*  Height Image. */
        fputc(8, f);    /*  Image Pixel Size. */
        fputc(0x20, f); /*  Image Descriptor Byte. */
        /* color map */
        for (I = 0; I < NC; I++)
        {
            fputi24(RGB[I], f);
        }
        /* 18 */
        if (SavePACB(f, Buf, L) < 0)
        {
            goto Error;
        }

        r = 0;
    Error:
        fclose(f);
    }
    // freeSpc(Blok);
    return r;
}

#define XRGB(r, g, b) RGB(b, g, r)

void ExportOperMaps(RString prefix)
{
#if _ENABLE_CHEATS
    GDebugger.PauseCheckingAlive();

    COLORREF palette[] = {
        XRGB(255, 255, 255), // OITNormal
        XRGB(192, 192, 255), // OITAvoidBush
        XRGB(192, 255, 192), // OITAvoidTree
        XRGB(255, 192, 192), // OITAvoid
        XRGB(0, 255, 255),   // OITWater
        XRGB(255, 0, 255),   // OITSpaceRoad
        XRGB(255, 255, 0),   // OITRoad
        XRGB(0, 0, 255),     // OITSpaceBush
        XRGB(0, 255, 0),     // OITSpaceTree
        XRGB(255, 0, 0),     // OITSpace
        XRGB(255, 255, 0)    // OITRoadForced
    };

    const int size = LandRange * OperItemRange;

    char* items = new char[size * size];
    char* itemsSoldier = new char[size * size];

    int oz = 0;
    for (int zz = 0; zz < LandRange; zz++)
    {
        int ox = 0;
        for (int xx = 0; xx < LandRange; xx++)
        {
            OperField field(xx, zz, MASK_AVOID_OBJECTS);
            for (int z = 0; z < OperItemRange; z++)
                for (int x = 0; x < OperItemRange; x++)
                {
                    OperItem& item = field._items[z][x];
                    int index = size * (size - 1 - oz - z) + ox + x;
                    items[index] = item._type;
                    itemsSoldier[index] = item._typeSoldier;
                }
            ox += OperItemRange;
        }
        oz += OperItemRange;
    }

    RString name;
    name = prefix + RString("Veh.tga");
    SavePAC256(name, size, size, items, palette, sizeof(palette) / sizeof(COLORREF));

    name = prefix + RString("Sol.tga");
    SavePAC256(name, size, size, itemsSoldier, palette, sizeof(palette) / sizeof(COLORREF));

    delete[] items;
    delete[] itemsSoldier;

    GDebugger.ResumeCheckingAlive();
#endif
}

#else
void ExportOperMaps(RString prefix) {}
void ExportWMF(const char* name, bool grid) {}

#endif
} // namespace Poseidon
