#pragma once

#include <Poseidon/Graphics/Cursor/ICursorOverlay.hpp>

namespace Poseidon
{
class World;
}

namespace Poseidon
{
class GameCursorOverlay : public ICursorOverlay
{
  public:
    explicit GameCursorOverlay(World* world) : _world(world) {}

    void Draw(Engine* engine) override;

  private:
    World* _world; // raw — World owns the overlay, can't outlive it
};

} // namespace Poseidon
