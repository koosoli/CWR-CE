// Dedicated E2E browser visibility smoke (run from the Pester orchestrator).
// Preconditions are provided by the caller:
// - local master-server service is running
// - dedicated server has published and was observer-marked as verified
// - game is launched with --master-server pointing at that local service
//
// Open the multiplayer browser (display 8), switch to the Internet source
// (IDC_MULTI_INTERNET = 122 — selecting it kicks a master-server list fetch),
// let the synchronous fetch + list rebuild settle, then assert the master
// server's servers populate the session list (IDC_MULTI_SESSIONS = 102).
// Each statement is a single line: tri sends one statement per frame, so a
// multi-line `if (...)` would be split into incomplete fragments.
//
// We assert a row well past index 0 on purpose: the local dedicated server is
// also LAN-discoverable, so index 0 alone would pass even if the internet
// browser were unwired (a stale LAN row leaks into the list). The --dev master
// server seeds 16 servers (DEV_SEED_SERVER_COUNT), so a selectable 6th row
// (index 5) can only come from the master-server fetch — LAN yields just the
// local host. triSelectList returns false for an out-of-range index, so an
// unwired browser fails here with FAIL:no_internet_rows.

triClick 105
triAssertEq [(triDisplay), 8]
triClick 122
triWaitFrames 60
triWaitFrames 180
if (triSelectList [102, 5]) then {"OK"} else {"FAIL:no_internet_rows"}
triEndTest
