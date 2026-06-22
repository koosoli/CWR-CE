// End-to-end persistence: open Audio, change a volume via the engine,
// Esc back to Index so AudioPage::Unmount fires SaveConfig, exit.
// The accompanying Pester wrapper (tests/smoke/audio_volume_persist.tests.ps1)
// runs this scenario against an ephemeral POSEIDON_USER_DIR and asserts
// audio.cfg on disk has the new value.
//
// triSetVolume bypasses the slider UI (which would need pixel-precise
// drag math through the 3D notebook surface) and writes the runtime
// volume directly via GSoundsys.  AudioPage::Unmount → SaveConfig
// reads back from GSoundsys, so the change still flows through the
// save path the same way a manual slider drag would.

triSimFrames 5;
triSetLanguage "English";
triSimFrames 5;
triClickText "OPTIONS";
triSimFrames 5;
triClickText "Audio";
triSimFrames 5;

triSetVolume ["music", 33];
triSetVolume ["effects", 44];
triSetVolume ["speech", 55];

// Esc → ScrollListPage::OnKeyDown → shell.PopPage() →
// AudioPage::Unmount → SaveConfig.
triSendKey 41;
triSimFrames 5;

triEndTest;
