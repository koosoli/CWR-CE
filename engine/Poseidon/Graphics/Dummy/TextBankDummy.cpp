#include <Poseidon/Graphics/Dummy/TextBankDummy.hpp>

namespace Poseidon
{

TextBankDummy::TextBankDummy() = default;

TextBankDummy::~TextBankDummy()
{
    UnlockAllTextures();
    DeleteAllAnimated();
}

int TextBankDummy::Find(RStringB name1, TextureDummy* interpolate)
{
    int i;
    for (i = 0; i < _texture.Size(); i++)
    {
        TextureDummy* texture = _texture[i];
        if (texture)
        {
            if (texture->GetName() != name1)
            {
                continue;
            }
            return i;
        }
    }
    return -1;
}

Ref<Texture> TextBankDummy::Load(RStringB name)
{
    int index = Find(name);
    if (index >= 0)
    {
        return (Texture*)_texture[index];
    }

    int iFree = _texture.Add();
    TextureDummy* texture = new TextureDummy;

    texture->SetName(name);
    _texture[iFree] = texture;
    texture->Init();
    return texture;
}

} // namespace Poseidon
