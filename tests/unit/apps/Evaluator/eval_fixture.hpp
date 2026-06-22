#pragma once
#include <Evaluator/EvaluatorHost.hpp>

class EvalFixture
{
    static bool s_initialized;
    static EvaluatorHost s_host;

  public:
    EvalFixture()
    {
        if (!s_initialized)
        {
            s_host.State();
            s_initialized = true;
        }
        s_host.State().clearOutput();
    }

    EvalState& state() { return s_host.State(); }
    EvaluatorHost& host() { return s_host; }
};

inline bool EvalFixture::s_initialized = false;
inline EvaluatorHost EvalFixture::s_host;
