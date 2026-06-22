#include <Poseidon/Foundation/Framework/AppFrame.hpp>

// CurrentAppFrameFunctions must be provided by the host app. This default is used
// only for ES_STANDALONE_BUILD, where no app supplies one.

#ifdef ES_STANDALONE_BUILD

namespace Poseidon::Foundation
{
static AppFrameFunctions defaultAppFrameFunctions;
AppFrameFunctions* CurrentAppFrameFunctions = &defaultAppFrameFunctions;
} // namespace Poseidon::Foundation

#endif
