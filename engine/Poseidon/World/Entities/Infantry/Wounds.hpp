#pragma once

#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>


namespace Poseidon
{

struct WoundPair
{
	Ref<Texture> _healthy;
	Ref<Texture> _wounded;
};

class WoundInfo: public AutoArray<WoundPair>
{
	public:
	void Load(LODShape *shape, const ParamEntry &cfg);
	void Register(LODShape *shape);
	void LoadAndRegister( LODShape *shape, const ParamEntry &cfg );
	void Unload();
};

class WoundTextureSelection
{
	FaceSelection _faces;
	Ref<Texture> _tex,_oldTex;
	// Note: use section selections instead of face selections (currently scans both face list and section list)

	void SetTexture( Shape *shape, Texture *tex ) const;

	public:
	WoundTextureSelection();
	void Init
	(
		Shape *shape,
		const Offset *offsets, int nOffsets, Texture *tex, Texture *oldTex
	);
	void Apply(Shape *shape, Texture *tex) const; // override wounded texture
	void Apply(Shape *shape) const;
	void Restore( Shape *shape ) const;

	Texture *GetOrigTexture() const {return _oldTex;}
};

typedef AutoArray<WoundTextureSelection> WoundTextureSelectionArray;
class WoundTextureSelections
{
	WoundTextureSelectionArray _selection[MAX_LOD_LEVELS];

	public:
	WoundTextureSelections();
	void Init( LODShape *shape, const WoundInfo &info );
	void Init
	(
		LODShape *shape, const Animation &anim, const WoundInfo &info
	);
	void Init
	(
		LODShape *shape, const ParamEntry &cfg,
		const char *name, const char *altName
	); 
	void Init
	(
		LODShape *shape, const WoundInfo &info, const char *name, const char *altName
	); 
	void Unload();

	void Apply( LODShape *shape, int level ) const;
	void Restore( LODShape *shape, int level ) const;
	void ApplyModified
	(
		LODShape *shape, int level,
		Texture *orig, Texture *origWounded
	) const;
};

}  // namespace Poseidon
