#pragma once

#include <Poseidon/World/Model/Model.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace Poseidon::Model
{
namespace ModelComputation {

inline BoundingBox computeBoundingBox(const std::vector<Vertex>& vertices) {
    if (vertices.empty()) {
        return BoundingBox();
    }
    
    BoundingBox box;
    box.min = Vector3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    box.max = Vector3(
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    );
    
    for (const auto& v : vertices) {
        box.min.x = std::min(box.min.x, v.position.x);
        box.min.y = std::min(box.min.y, v.position.y);
        box.min.z = std::min(box.min.z, v.position.z);
        
        box.max.x = std::max(box.max.x, v.position.x);
        box.max.y = std::max(box.max.y, v.position.y);
        box.max.z = std::max(box.max.z, v.position.z);
    }
    
    return box;
}

inline BoundingSphere computeBoundingSphere(const BoundingBox& box) {
    BoundingSphere sphere;
    sphere.center = box.GetCenter();
    
    Vector3 extent = box.GetExtent();
    sphere.radius = std::sqrt(
        extent.x * extent.x + 
        extent.y * extent.y + 
        extent.z * extent.z
    );
    
    return sphere;
}

inline BoundingSphere computeBoundingSphere(const std::vector<Vertex>& vertices) {
    if (vertices.empty()) {
        return BoundingSphere();
    }

    BoundingBox box = computeBoundingBox(vertices);
    return computeBoundingSphere(box);
}

inline std::vector<uint32_t> extractUniqueMaterialIndices(const std::vector<Triangle>& triangles) {
    std::vector<uint32_t> uniqueIndices;
    
    for (const auto& tri : triangles) {
        if (std::find(uniqueIndices.begin(), uniqueIndices.end(), tri.materialIndex) == uniqueIndices.end()) {
            uniqueIndices.push_back(tri.materialIndex);
        }
    }
    
    std::sort(uniqueIndices.begin(), uniqueIndices.end());
    return uniqueIndices;
}

inline std::vector<Section> generateSections(const std::vector<Triangle>& triangles) {
    if (triangles.empty()) {
        return {};
    }
    
    std::vector<Section> sections;
    
    uint32_t currentMaterial = triangles[0].materialIndex;
    uint32_t sectionStart = 0;
    uint32_t sectionCount = 0;
    
    for (size_t i = 0; i < triangles.size(); ++i) {
        if (triangles[i].materialIndex != currentMaterial) {
            Section sec;
            sec.materialIndex = currentMaterial;
            sec.startTriangle = sectionStart;
            sec.triangleCount = sectionCount;
            sec.hints = RenderHints::None;
            sections.push_back(sec);

            currentMaterial = triangles[i].materialIndex;
            sectionStart = static_cast<uint32_t>(i);
            sectionCount = 1;
        } else {
            ++sectionCount;
        }
    }
    
    // Add final section
    Section sec;
    sec.materialIndex = currentMaterial;
    sec.startTriangle = sectionStart;
    sec.triangleCount = sectionCount;
    sec.hints = RenderHints::None;
    sections.push_back(sec);
    
    return sections;
}

inline Matrix4x3 computeProxyTransform(
    const std::vector<Vertex>& vertices,
    const NamedSelection& selection
) {
    Matrix4x3 transform;

    // Proxy selection must have exactly 3 vertices
    if (selection.vertexIndices.size() != 3) {
        return transform;  // identity
    }

    uint32_t i0 = selection.vertexIndices[0];
    uint32_t i1 = selection.vertexIndices[1];
    uint32_t i2 = selection.vertexIndices[2];
    
    if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
        return transform;  // identity
    }

    const Vector3& v0 = vertices[i0].position;
    const Vector3& v1 = vertices[i1].position;
    const Vector3& v2 = vertices[i2].position;
    
    // Build local coordinate system from the proxy triangle
    Vector3 right = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    float rightLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    if (rightLen > 0.0001f) {
        right.x /= rightLen;
        right.y /= rightLen;
        right.z /= rightLen;
    } else {
        right = {1, 0, 0};
    }
    
    Vector3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vector3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    Vector3 forward = {
        edge1.y * edge2.z - edge1.z * edge2.y,
        edge1.z * edge2.x - edge1.x * edge2.z,
        edge1.x * edge2.y - edge1.y * edge2.x
    };
    float forwardLen = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (forwardLen > 0.0001f) {
        forward.x /= forwardLen;
        forward.y /= forwardLen;
        forward.z /= forwardLen;
    } else {
        forward = {0, 0, 1};
    }
    
    Vector3 up = {
        forward.y * right.z - forward.z * right.y,
        forward.z * right.x - forward.x * right.z,
        forward.x * right.y - forward.y * right.x
    };
    
    transform.Set(
        right.x, right.y, right.z, v0.x,
        up.x, up.y, up.z, v0.y,
        forward.x, forward.y, forward.z, v0.z
    );
    
    return transform;
}

inline std::vector<Proxy> generateProxiesFromSelections(
    const std::vector<Vertex>& vertices,
    const std::vector<NamedSelection>& selections
) {
    std::vector<Proxy> proxies;
    
    for (size_t i = 0; i < selections.size(); ++i) {
        const auto& sel = selections[i];
        
        if (sel.name.find("proxy:") == 0 && sel.vertexIndices.size() == 3) {
            Proxy proxy;
            proxy.name = sel.name;
            proxy.selectionIndex = static_cast<uint32_t>(i);
            proxy.transform = computeProxyTransform(vertices, sel);
            proxies.push_back(proxy);
        }
    }
    
    return proxies;
}

inline bool validateMesh(const Mesh& mesh) {
    if (mesh.vertices.empty() || mesh.triangles.empty()) {
        return false;
    }
    if (mesh.boundingSphere.radius <= 0.0f) {
        return false;
    }
    for (const auto& tri : mesh.triangles) {
        for (int i = 0; i < 3; ++i) {
            if (tri.indices[i] >= mesh.vertices.size()) {
                return false;
            }
        }
    }
    
    return true;
}

} // namespace ModelComputation
} // namespace Poseidon::Model
