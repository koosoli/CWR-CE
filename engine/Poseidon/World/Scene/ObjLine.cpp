
#include <Poseidon/World/Scene/ObjLine.hpp>
#include <Poseidon/Graphics/Textures/TexturePreload.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>

namespace Poseidon
{
LODShapeWithShadow* ObjectLine::CreateShape()
{
    LODShapeWithShadow* lShape = new LODShapeWithShadow;

    lShape->SetAutoCenter(false);
    Shape* shape = new Shape;
    lShape->AddShape(shape, 0);
    const int special = IsAlpha | NoShadow | IsAlphaFog | IsColored;
    lShape->SetSpecial(special);

    shape->ReallocTable(2);
    const int clip = ClipAll | ClipLightLine;
    shape->SetPos(0) = VZero;
    shape->SetPos(1) = VForward;
    shape->SetClip(0, clip);
    shape->SetClip(1, clip);
    shape->SetNorm(0) = VUp;
    shape->SetNorm(1) = VUp;
    Poly face;
    face.Init();
    face.SetN(4); // degenerate square
    face.Set(0, 1);
    face.Set(1, 0);
    face.Set(2, 0);
    face.Set(3, 1);
    face.SetTexture(GScene->Preloaded(TextureWhite));
    face.SetSpecial(special);
    shape->AddFace(face);
    shape->SetSpecial(special);
    shape->FindSections();
    shape->CalculateHints();
    lShape->CalculateMinMax(true);
    return lShape;
}

void ObjectLine::SetPos(LODShapeWithShadow* lShape, Vector3Par beg, Vector3Par end)
{
#if ALPHA_SPLIT
    Shape* shape = lShape->LevelAlpha(0);
#else
    Shape* shape = lShape->LevelOpaque(0);
#endif
    PoseidonAssert(shape->NPos() == 2);
    PoseidonAssert(shape->NFaces() == 1);
    shape->SetPos(0) = beg;
    shape->SetPos(1) = end;
    lShape->CalculateMinMax(true);
}

DEFINE_FAST_ALLOCATOR(ObjectLineDiag)

ObjectLineDiag::ObjectLineDiag(LODShapeWithShadow* shape) : base(shape, -1) {}
void ObjectLineDiag::Draw(int level, ClipFlags clipFlags, const FrameBase& pos)
{
    DrawLines(level, clipFlags, pos);
}

} // namespace Poseidon
