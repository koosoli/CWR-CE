#pragma once

#include <Poseidon/Input/ViewerControls.hpp>
#include <SDL3/SDL_scancode.h>

namespace Poseidon
{

// IViewerInputProvider — abstract interface over the input methods the
// viewer cares about.  Lets ViewerController::Poll be unit-tested with
// a mock provider that doesn't need an SDL window or InputSubsystem.
//
// Production wiring uses InputSubsystemViewerProvider in
// ViewerController.cpp; tests use a plain struct with public setters.
class IViewerInputProvider
{
  public:
    virtual ~IViewerInputProvider() = default;
    virtual bool  MouseLeft() const   = 0;
    virtual bool  MouseRight() const  = 0;
    virtual bool  MouseMiddle() const = 0;
    virtual float MouseDeltaX() const = 0;
    virtual float MouseDeltaY() const = 0;
    //! One-shot read: returns wheel delta accumulated since last call,
    //! and clears the accumulator.
    virtual float ConsumeWheel() = 0;
    virtual bool  KeyDown(SDL_Scancode) const     = 0;
    virtual bool  KeyPressed(SDL_Scancode) const  = 0;
    virtual void  ConsumeKeyPress(SDL_Scancode)   = 0;
};

// ViewerController — produces ViewerControls from input each frame.
//
// Lives in the same shape as PilotController / InfantryController: the
// engine-side adapter calls Poll(provider) once per frame, the consumer
// (World::TickViewerControls) applies the result.  Kept off the
// VehicleControls / PilotController hierarchy on purpose — the viewer
// is not a vehicle, and forcing it into that contract entangles viewer
// logic with the consumer (ObjectViewer::Simulate).
//
// Anything that needs one-shot semantics (Esc, F5, Space, R, O, ?)
// consumes its key inside Poll so the corresponding scancode never
// leaks into game-side input processing for the same frame.
class ViewerController
{
  public:
    //! Poll input and produce a ViewerControls snapshot for this frame.
    //! `provider` is queried for raw state; pass an
    //! InputSubsystemViewerProvider in production or a mock in tests.
    ViewerControls Poll(IViewerInputProvider& provider);

    //! Convenience for production code: poll the live InputSubsystem.
    //! Inline wrapper around the provider-taking overload.
    ViewerControls Poll();

  private:
    bool _slashLatched = false; // edge-detect for Shift+/ (toggle help)
};
} // namespace Poseidon

