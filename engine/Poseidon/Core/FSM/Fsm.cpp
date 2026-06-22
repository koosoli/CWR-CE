
#include <Poseidon/Core/FSM/Fsm.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Serialization/ParamArchive.hpp>
#include <stdio.h>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{

FSM::FSM() : _curState(FinalState)
{
    _state = nullptr;
    _special = nullptr;
    _nStates = 0;
    for (int i = 0; i < MaxVar; i++)
    {
        Var(i) = 0;
    }
    for (int i = 0; i < MaxVar; i++)
    {
        VarTime(i) = Foundation::Time(0);
    }
}

FSM::FSM(const StateInfo* states, int n, const pStateFunction* functions, int nFunc) : _curState(FinalState)
{
    Init(states, n, functions, nFunc);
}

void FSM::Init(const StateInfo* states, int n, const pStateFunction* functions, int nFunc)
{
    _state = states;
    _nStates = n;
    _special = functions;
    _nSpecials = nFunc;
    for (int i = 0; i < MaxVar; i++)
    {
        Var(i) = 0;
    }
    for (int i = 0; i < MaxVar; i++)
    {
        VarTime(i) = Foundation::Time(0);
    }
    _timeOut = TIME_MAX; // default - no timeout
}

FSM::~FSM() = default;

void FSM::SetState(State state, Context* context)
{
    _curState = state;
    if (_curState == FinalState)
    {
        return;
    }
    const StateInfo& curState = _state[_curState];
    curState.Init(context);
}

bool FSM::Update(Context* context)
{
    if (_curState == FinalState)
    {
        return true;
    }
    const StateInfo& curState = CurState();
    curState.Check(context); // this function may change state
    return _curState == FinalState;
}

void FSM::Refresh(Context* context)
{
    if (_curState == FinalState)
    {
        return;
    }
    const StateInfo& curState = CurState();
    curState.Init(context);
}

// _special[] stores typed state functions as void(*)(void*); the ABI is
// identical — same suppression as StateInfo::Init/Check above.
__attribute__((no_sanitize("function"))) void FSM::Enter(Context* context)
{
    if (_nSpecials < 1)
    {
        return;
    }
    _special[0](context);
}

__attribute__((no_sanitize("function"))) void FSM::Exit(Context* context)
{
    if (_nSpecials < 2)
    {
        return;
    }
    _special[1](context);
}

void FSM::SetTimeOut(float sec)
{
    _timeOut = Glob.time + sec;
}

float FSM::GetTimeOut() const
{
    return _timeOut - Glob.time;
}

bool FSM::TimedOut() const
{
    return Glob.time > _timeOut;
}

LSError FSM::Serialize(ParamArchive& ar)
{
    if (ar.IsSaving())
    {
        RString str = CurState().GetName();
        PARAM_CHECK(ar.Serialize("curState", str, 1))
    }
    else if (ar.GetPass() == ParamArchive::PassFirst)
    {
        RString str;
        PARAM_CHECK(ar.Serialize("curState", str, 1))
        _curState = -1;
        for (int i = 0; i < _nStates; i++)
        {
            if (stricmp(_state[i].GetName(), str) == 0)
            {
                _curState = i;
            }
        }
        if (_curState < 0)
        {
            return LSStructure;
        }
    }
    PARAM_CHECK(ar.Serialize("timeOut", _timeOut, 1))
    for (int i = 0; i < MaxVar; i++)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Var%d", i);
        PARAM_CHECK(ar.Serialize(buffer, _iVar[i], 1, 0))
    }
    for (int i = 0; i < MaxVar; i++)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "VarTime%d", i);
        PARAM_CHECK(ar.Serialize(buffer, _tVar[i], 1, Foundation::Time(0)))
    }
    return LSOK;
}
} // namespace Poseidon
