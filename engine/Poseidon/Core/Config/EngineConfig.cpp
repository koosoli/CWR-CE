#include <Poseidon/Core/Config/EngineConfig.hpp>

namespace Poseidon
{
EngineConfig::EngineConfig(RuntimeFlags& rf, EngineState& es)
    : horizontZ(es.horizontZ), objectsZ(es.objectsZ), tacticalZ(es.tacticalZ), radarZ(es.radarZ), shadowsZ(es.shadowsZ),
      trackTimeToLive(es.trackTimeToLive), invTrackTimeToLive(es.invTrackTimeToLive), blood(rf.blood)
#if _ENABLE_CHEATS
      ,
      super(rf.super)
#endif
      ,
      checkInitAndExit(rf.checkInitAndExit), noLandscape(rf.noLandscape), noTerrainCache(rf.noTerrainCache),
      doCreateServer(rf.hostMultiplayer), doCreateDedicatedServer(rf.dedicatedServer), showFps(rf.showFps),
      landEditor(rf.landEditor), hideCursor(rf.hideCursor), noRedrawWindow(rf.noRedrawWindow),
      gUseFileBanks(rf.gUseFileBanks), gEnableCaching(rf.gEnableCaching), gReplaceProxies(rf.gReplaceProxies),
      gMergeTextures(rf.gMergeTextures)
{
}
} // namespace Poseidon
