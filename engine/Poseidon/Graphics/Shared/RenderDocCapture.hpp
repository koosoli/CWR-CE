#pragma once

// In-application RenderDoc capture trigger.  RenderDoc's API table is
// loaded at engine init if `renderdoc.dll` is already in the process
// (typical when launched via RenderDoc's UI).  Without RenderDoc loaded,
// every entry point silently no-ops — the absence is logged once.
//
// Use to bracket a problematic frame from inside the game:
//   if (RdcCapture::Available()) RdcCapture::Trigger();
//
// The .rdc file lands wherever RenderDoc's path template points
// (usually `tmp/` or RenderDoc's session dir).  Path of the most
// recent capture is logged to the Graphics channel.

namespace Poseidon
{
namespace RdcCapture
{
// Attempt to bind to renderdoc.dll's RENDERDOC_GetAPI.  Safe to call
// many times (only the first does the LoadLibrary + symbol lookup).
// Returns true if the API table is now available.
bool Init();

// True after a successful Init() — convenience for call sites.
bool Available();

// Set the path template RenderDoc uses for capture filenames.
// E.g. "tmp/rdc/cwa" → "tmp/rdc/cwa_frame123.rdc".
// Created path is whatever the template + RenderDoc's frame counter
// produces.  Safe no-op if RenderDoc isn't loaded.
void SetPathTemplate(const char* prefix);

// Schedule a capture of the next swap.  Equivalent to pressing F12
// in the RenderDoc UI.  Safe no-op if RenderDoc isn't loaded.
void Trigger();

// Returns the path of the most recent .rdc file written, or empty if
// none / RenderDoc not loaded.  Useful for tests to surface the path
// in test output so the user can open the capture.
const char* LastCapturePath();
} // namespace RdcCapture

} // namespace Poseidon
