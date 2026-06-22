// Accept the ConfirmRevert modal when Apply opens it. Some display paths apply
// directly on this runner, so treat the missing modal as already accepted.
if ((triAssert [(triGetControlVisible 9201)]) == "OK") then {
    triClick 9201
};
triSimFrames 10
