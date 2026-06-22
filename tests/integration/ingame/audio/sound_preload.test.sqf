// Asset-loader plan Phase B: the mission audio manifest (CfgSounds) is
// decoded into the backend PCM cache during the loading screen, and the
// first say is served from that cache instead of reading + decoding the
// file on the main thread.
//
// Broken state (no preload hook in World::InitVehicles): cache is empty at
// mission start -- triAssertGe [(triGetAudioCacheEntries), 1] fails with entries=0.
// Broken state (no cache in WaveOAL::DecodePcm): the say decodes from the
// stream -- triAssertGe [(triGetAudioCacheHits), 1] fails with hits=0.

triSimFrames 30
triAssertGe [(triGetAudioCacheEntries), 1]
player say "testsay"
triSimFrames 30
triAssertGe [(triGetAudioCacheHits), 1]
triEndTest
