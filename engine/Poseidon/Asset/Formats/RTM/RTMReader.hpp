#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace Poseidon::Asset::Formats
{

static constexpr int MAGIC_LEN     = 8;
static constexpr int BONE_NAME_LEN = 32;
static constexpr int MATRIX_FLOATS = 12; // 3x4 stored as 12 floats

enum class RTMVersion { Unknown, V100, V101 };

struct BoneTransform
{
    // [aside(3) up(3) dir(3) pos(3)] = Matrix3T + Vector3T
    float m[12];
    void  setIdentity() { std::memset(m, 0, sizeof(m)); m[0] = m[4] = m[8] = 1.0f; }
    float tx() const { return m[9]; }
    float ty() const { return m[10]; }
    float tz() const { return m[11]; }
};

struct RTMPhase
{
    float                    time = 0.0f;
    std::vector<BoneTransform> transforms; // one per bone
};

struct RTMAnimation
{
    RTMVersion               version = RTMVersion::Unknown;
    float                    stepX = 0.0f, stepY = 0.0f, stepZ = 0.0f;
    std::vector<std::string> boneNames;
    std::vector<RTMPhase>    phases;
    int                      boneCount() const { return static_cast<int>(boneNames.size()); }
    int                      phaseCount() const { return static_cast<int>(phases.size()); }
};

// Read header only (magic, step, bone names, phase/bone counts) — no phase data.
// Useful for lazy loading: get metadata without reading all keyframes.
template<typename Stream>
bool readRTMHeader(Stream& in, RTMAnimation& anim)
{
    anim = RTMAnimation{};
    char magic[MAGIC_LEN + 1] = {};
    in.read(magic, MAGIC_LEN);
    if (in.fail())
        return false;

    if (std::strcmp(magic, "RTM_0100") == 0)
    {
        anim.version = RTMVersion::V100;
        in.read((char*)&anim.stepZ, sizeof(float));
    }
    else if (std::strcmp(magic, "RTM_0101") == 0)
    {
        anim.version = RTMVersion::V101;
        in.read((char*)&anim.stepX, sizeof(float));
        in.read((char*)&anim.stepY, sizeof(float));
        in.read((char*)&anim.stepZ, sizeof(float));
    }
    else
        return false;

    int32_t nAnim = 0, nSel = 0;
    in.read((char*)&nAnim, sizeof(int32_t));
    in.read((char*)&nSel, sizeof(int32_t));
    if (in.fail() || nAnim < 0 || nSel < 0)
        return false;
    if (nAnim > 100000 || nSel > 1000)
        return false;

    anim.boneNames.resize(nSel);
    for (int i = 0; i < nSel; i++)
    {
        // One extra byte the read never touches guarantees a NUL terminator: the
        // wire name is exactly BONE_NAME_LEN bytes with no required terminator, so
        // the tolower scan and the std::string assignment below would otherwise run
        // off the buffer when all BONE_NAME_LEN bytes are non-zero.
        char name[BONE_NAME_LEN + 1] = {};
        in.read(name, BONE_NAME_LEN);
        if (in.fail())
            return false;
        for (char* p = name; *p; ++p)
            *p = static_cast<char>(std::tolower(static_cast<unsigned char>(*p)));
        anim.boneNames[i] = name;
    }
    anim.phases.resize(nAnim);
    return true;
}

// Read phase data from stream (call after readRTMHeader, stream positioned at first phase).
template<typename Stream>
bool readRTMPhases(Stream& in, RTMAnimation& anim)
{
    int nSel  = anim.boneCount();
    int nAnim = anim.phaseCount();
    for (int p = 0; p < nAnim; p++)
    {
        auto& phase = anim.phases[p];
        in.read((char*)&phase.time, sizeof(float));
        if (in.fail())
            return false;
        phase.transforms.resize(nSel);
        for (int s = 0; s < nSel; s++)
        {
            char name[BONE_NAME_LEN]; // per-phase bone name (redundant in format, skip)
            in.read(name, BONE_NAME_LEN);
            in.read((char*)phase.transforms[s].m, sizeof(float) * MATRIX_FLOATS);
            if (in.fail())
                return false;
        }
    }
    return true;
}

template<typename Stream>
bool readRTM(Stream& in, RTMAnimation& anim)
{
    if (!readRTMHeader(in, anim))
        return false;
    return readRTMPhases(in, anim);
}

inline bool readRTMFromMemory(const uint8_t* data, size_t size, RTMAnimation& anim)
{
    std::string       buf(reinterpret_cast<const char*>(data), size);
    std::istringstream iss(std::move(buf), std::ios::binary);
    return readRTM(iss, anim);
}

inline bool readRTMFromFile(const char* path, RTMAnimation& anim)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.good())
        return false;
    return readRTM(file, anim);
}

} // namespace Poseidon::Asset::Formats
