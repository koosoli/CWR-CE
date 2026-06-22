// Client-local mute (VoN) / ignore (chat) store.
// The VoN speaker feed and ChatList::Add both gate on this store; the live-MP
// tests (tests/integration/mp/von_mute.test, chat_ignore.test) cover the seams
// end-to-end. This unit test pins the store's toggle/set/clear semantics and
// that voice and chat sets are independent and keyed per dpnid.
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Core/PlayerMuteIgnore.hpp>

using namespace Poseidon;

TEST_CASE("PlayerMuteIgnore: voice mute toggle/set/clear", "[network][mute]")
{
    ClearMuteIgnore();
    CHECK_FALSE(IsVoiceMuted(7));

    CHECK(ToggleVoiceMute(7)); // returns new state: muted
    CHECK(IsVoiceMuted(7));
    CHECK_FALSE(ToggleVoiceMute(7)); // back to unmuted
    CHECK_FALSE(IsVoiceMuted(7));

    SetVoiceMuted(7, true);
    CHECK(IsVoiceMuted(7));
    SetVoiceMuted(7, false);
    CHECK_FALSE(IsVoiceMuted(7));

    SetVoiceMuted(3, true);
    SetVoiceMuted(5, true);
    CHECK(IsVoiceMuted(3));
    CHECK(IsVoiceMuted(5));
    CHECK_FALSE(IsVoiceMuted(4));

    ClearMuteIgnore();
    CHECK_FALSE(IsVoiceMuted(3));
    CHECK_FALSE(IsVoiceMuted(5));
}

TEST_CASE("PlayerMuteIgnore: chat ignore is independent of voice mute", "[network][mute]")
{
    ClearMuteIgnore();

    CHECK(ToggleChatIgnore(9));
    CHECK(IsChatIgnored(9));
    CHECK_FALSE(IsVoiceMuted(9)); // muting voice and ignoring chat are separate

    SetVoiceMuted(9, true);
    CHECK(IsChatIgnored(9));
    CHECK(IsVoiceMuted(9));

    SetChatIgnored(9, false);
    CHECK_FALSE(IsChatIgnored(9));
    CHECK(IsVoiceMuted(9)); // unignoring chat leaves voice mute intact

    ClearMuteIgnore();
    CHECK_FALSE(IsChatIgnored(9));
    CHECK_FALSE(IsVoiceMuted(9));
}

// SetVoiceMutedAll / SetChatIgnoredAll cover all dpnids, including those not yet
// in the identity list at call time.  Broken-state delta: without these sentinels
// triMuteVoN -1 / triIgnoreChat -1 iterated GetIdentities() at call time and
// missed players whose identity exchange had not yet completed.
TEST_CASE("PlayerMuteIgnore: SetVoiceMutedAll covers any dpnid", "[network][mute]")
{
    ClearMuteIgnore();
    CHECK_FALSE(IsVoiceMuted(42));

    SetVoiceMutedAll(true);
    CHECK(IsVoiceMuted(42));
    CHECK(IsVoiceMuted(99));

    SetVoiceMutedAll(false);
    CHECK_FALSE(IsVoiceMuted(42));

    ClearMuteIgnore();
}

TEST_CASE("PlayerMuteIgnore: SetChatIgnoredAll covers any dpnid", "[network][mute]")
{
    ClearMuteIgnore();
    CHECK_FALSE(IsChatIgnored(55));

    SetChatIgnoredAll(true);
    CHECK(IsChatIgnored(55));
    CHECK(IsChatIgnored(100));

    ClearMuteIgnore();
    CHECK_FALSE(IsChatIgnored(55)); // ClearMuteIgnore resets the all-flag
}
