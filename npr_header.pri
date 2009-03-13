CONFIG += debug_and_release
#CONFIG += release
CONFIG(debug, debug|release) {
	DBGNAME = debug
}
else {
	DBGNAME = release
}
