#include <Poseidon/Graphics/Rendering/Draw/RandomShape.hpp>
#include <Poseidon/World/Entities/Vehicles/Vehicle.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Random/randomGen.hpp>

using namespace Poseidon;
ObjectTyped::ObjectTyped(LODShapeWithShadow* shape, const EntityType* type, int id)
    : base(shape, id), _type(const_cast<EntityType*>(type))
{
    _type->VehicleAddRef();
}

ObjectTyped::~ObjectTyped()
{
    _type->VehicleRelease();
}

#if SUPPORT_RANDOM_SHAPES

namespace Poseidon
{

RandomShapeType::RandomShapeType(const ParamEntry* param) : base(param) {}

RandomShapeType::~RandomShapeType() {}

void RandomShapeType::Load(const ParamEntry& cfg)
{
    base::Load(cfg);
}

void RandomShapeType::InitShape()
{
    // load shapes
    const ParamEntry& shapes = (*_par) >> "models";
    for (int i = 0; i < shapes.GetSize() - 1; i += 2)
    {
        RString modelName = ::GetShapeName(shapes[i]);
        float modelProbab = shapes[i + 1];

        RandomShapeInfo& info = _shapes.Append();
        info._shape = Shapes.New(modelName, false, true);
        info._probab = modelProbab;
    }
}

void RandomShapeType::DeinitShape()
{
    // release shapes
    _shapes.Clear();
}

LODShapeWithShadow* RandomShapeType::SelectShape(float x) const
{
    for (int i = 0; i < _shapes.Size(); i++)
    {
        const RandomShapeInfo& info = _shapes[i];
        x -= info._probab;
        if (x > 0)
            continue;
        return info._shape;
    }
    if (_shapes.Size() <= 0)
        return 0;
    return _shapes[_shapes.Size() - 1]._shape;
}

RandomShape::RandomShape(const RandomShapeType* type, int id) : base(nullptr, type, id) // select any shape
{
}

LODShapeWithShadow* RandomShape::SelectShape(Vector3Val pos) const
{
    // select random shape (based on position)
    float random = GRandGen.RandomValue(toIntFloor(pos.X()), toIntFloor(pos.Z()), toIntFloor(pos.Y()));
    return Type()->SelectShape(random);
}

LODShapeWithShadow* RandomShape::GetShapeOnPos(Vector3Val pos) const
{
    // virtual function of Object
    return SelectShape(pos);
}

void RandomShape::Draw(int forceLOD, ClipFlags clipFlags, const FrameBase& pos)
{
    LODShapeWithShadow* shape = SelectShape(pos.Position());
    if (!shape)
        return;

    // temporarily override shape
    _shape = shape;
    base::Draw(forceLOD, clipFlags, pos);
    // reset shape to nullptr
    _shape = nullptr;
}

} // namespace Poseidon

#endif
