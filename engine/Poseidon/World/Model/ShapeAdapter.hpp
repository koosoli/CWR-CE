#pragma once

namespace Poseidon { class LODShapeWithShadow; }
using Poseidon::LODShapeWithShadow;

namespace Poseidon {
namespace Model {

struct Model;

namespace ShapeAdapter
{
    LODShapeWithShadow* convertToLODShape(const Model& model, bool reversed = false);
}

} // namespace Model
} // namespace Poseidon
