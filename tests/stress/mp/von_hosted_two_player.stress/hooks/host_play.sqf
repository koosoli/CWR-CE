triAssertEq [(triDisplay), 70]
triClick 1
triAssertNgs 13
triAssertEq [triMpClientReady 14, "OK"]
triAssertNgs 14
