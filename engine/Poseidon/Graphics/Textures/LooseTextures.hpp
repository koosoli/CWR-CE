#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{
namespace Graphics
{

// If --loose-textures is enabled and the requested name has a known
// "baked" extension (.paa / .pac / .jpg / .jpeg), check for a sibling
// .png or .tga at the same VFS path. Returns the loose sibling path
// if one exists, otherwise the original name.
//
// The texture cache key (Texture::Name()) stays as the original name
// so models continue to reference textures by their canonical filename;
// only the actual file read swaps to the loose copy.
//
// Powers the artist workflow: drop foo.png next to foo.paa (or
// foo.jpg) and the engine picks it up live, no PAA repack required.
RString ResolveLooseTexturePath(const char* name);

} // namespace Graphics
} // namespace Poseidon
