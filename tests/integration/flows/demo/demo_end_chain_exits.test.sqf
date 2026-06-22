// GL-21 synthetic demo-ending regression.
//
// Real QA reports are demo-only, but the live Demo.Demo mission is expensive and
// noisy to drive. This stripped-down mission keeps only the essential demo end
// path: a startup trigger runs `outro.sqs`, which flips `uplkonec`, and the
// mission must then resolve to `END1`.
//
// In autotest mode the intended behaviour is not "stay on IDD 58".  The mission
// should:
// 1. leave `CONTINUE`
// 2. reach `END1`
// 3. exit cleanly on its own
//
// triAssertEq auto-polls (Capybara sync), so the CONTINUE→END1 hop
// converges without an explicit triSimFrames pad between assertions.

triSceneReady
triAssertEq [(triGetEndMode), "CONTINUE"]
triAssertEq [(triGetEndMode), "END1"]
triEndTest
