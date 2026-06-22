#pragma once

namespace Poseidon
{
// Per-session, client-local sets of players the local user has muted (VoN
// voice) or ignored (text chat), keyed by network player id (dpnid — the same
// id the MP player list, chat sender Person::GetRemotePlayer(), and VoN packet
// channel all use).  Purely local: no network sync.  Reset on session teardown
// via ClearMuteIgnore().  Thread-safe — the VoN voice receive path may query
// IsVoiceMuted off the main thread.
bool IsVoiceMuted(int playerId);
bool IsChatIgnored(int playerId);

void SetVoiceMuted(int playerId, bool muted);
void SetChatIgnored(int playerId, bool ignored);

// Mute/ignore every remote player, including those not yet in the identity list.
// Used by triMuteVoN -1 / triIgnoreChat -1.  Reset by ClearMuteIgnore().
void SetVoiceMutedAll(bool muted);
void SetChatIgnoredAll(bool ignored);

// Flip the current state; returns the new state (true = now muted/ignored).
bool ToggleVoiceMute(int playerId);
bool ToggleChatIgnore(int playerId);

void ClearMuteIgnore();
} // namespace Poseidon
