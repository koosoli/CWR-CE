// Regression test: Briefing coordination with 2 auto-ready clients
// Both clients must reach briefing state, then auto-launch to play state
// when server advances to NGSPlay. Tests for briefing lockup regression
// (dc08b8b1b changes to GetServerState priority).
//
// Timeline:
// 1. Server loads mission, waits for both clients to reach briefing (NGS 13)
// 2. Clients auto-assign roles, reach briefing with auto-ready countdown
// 3. Clients auto-launch when server signals NGSPlay (NGS 14)
// 4. Test completes when all reach play state
//
// FAILURE MODE: If clients get stuck in briefing (all green but no progress),
// test timeout will catch it as a regression.

triAssertNgs 14
