#ifndef _CONSOLE_BASE_H
#define _CONSOLE_BASE_H


namespace Poseidon::Foundation
{
extern bool wasError; // use this to indicate error
// note that when error is indicated via return value, wasError need not be set

int consoleMain( int argc, const char *argv[] );

} // namespace Poseidon::Foundation

#endif
