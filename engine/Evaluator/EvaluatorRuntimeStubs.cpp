#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>

static MemFunctions defaultMemFunctions;
MemFunctions* GMemFunctionsPtr = &defaultMemFunctions;
MemFunctions* GSafeMemFunctionsPtr = &defaultMemFunctions;

class EvaluatorAppFrameFunctions : public AppFrameFunctions
{
};

static EvaluatorAppFrameFunctions evaluatorAppFrame;
AppFrameFunctions* CurrentAppFrameFunctions = &evaluatorAppFrame;

#ifndef _STATISTICS_H
class StatisticsByName
{
  public:
    StatisticsByName() {}
    ~StatisticsByName() {}
    void Count(const char*, int) {}
};
#endif

void EnsureEvaluatorRuntimeStubsLinked() {}
