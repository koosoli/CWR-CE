#include <Poseidon/World/Entities/Vehicles/Plane.hpp>

namespace Poseidon
{
Plane::Plane(Vector3Par normal, Vector3Par point) : _normal(normal), _d(-normal * point) {}

void Plane::Transform(const Matrix4& trans, const Matrix4& iTrans)
{
    // note: iTrans transforms normal
    // it can be viewed as transforming plane (not trans)
    _d += trans.Position() * _normal;
    _normal.SetRotate(iTrans, _normal);
}

} // namespace Poseidon
