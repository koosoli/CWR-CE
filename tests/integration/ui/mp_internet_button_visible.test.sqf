triSetLanguage "English"
triWaitFrames 5
triAssertEq [(triDisplay), 0]
triClick 105
triAssertEq [(triDisplay), 8]
triAssert [(triGetControlVisible 122)]
triEndTest
