
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Game/Scripting/Scripts.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/Game/Commands/GameStateExt.hpp>
#include <Poseidon/AI/AI.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/World/World.hpp>
#include <ctype.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/LLinks.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{
RString FindScript(RString name);
}

namespace Poseidon
{
#define DIAG_DEBUG 0 // script debug level (0..90)

#if _MSC_VER >= 1300
#define SCRIPTS_DEBUGGING 0
#else
#define SCRIPTS_DEBUGGING 0
#endif

#if SCRIPTS_DEBUGGING
#import "dte.olb" raw_interfaces_only named_guids
static EnvDTE::DebuggerPtr GDebugger = nullptr;
static int nScripts = 0;

static EnvDTE::DebuggerPtr CreateDebugger()
{
    IUnknownPtr pUnk;
    GetActiveObject(EnvDTE::CLSID_DTE, nullptr, &pUnk);

    if (pUnk == nullptr)
        return nullptr;
    EnvDTE::_DTEPtr pDTE = pUnk;
    if (pDTE == nullptr)
        return nullptr;

    EnvDTE::DebuggerPtr debugger;
    if (!SUCCEEDED(pDTE->get_Debugger(&debugger)))
        return nullptr;

    EnvDTE::dbgDebugMode mode;
    if (!SUCCEEDED(debugger->get_CurrentMode(&mode)))
        return nullptr;
    if (mode == EnvDTE::dbgDesignMode)
        return nullptr;

    return debugger;
}

void InitDebugger()
{
    if (nScripts++ == 0)
    {
        CoInitialize(nullptr);
        GDebugger = CreateDebugger();
    }
}
void DoneDebugger()
{
    if (--nScripts == 0)
    {
        GDebugger = nullptr;
        CoUninitialize();
    }
}
#endif

Script::Script()
{
    _baseTime = Glob.time;
    _currentLine = 0;

    _time = 0;
    _suspendedUntil = 0;
    _maxLinesAtOnce = 100;

#if SCRIPTS_DEBUGGING
    InitDebugger();
#endif
}

static void ReadLine(QIStream& in, char* buffer, int bufferSize)
{
    char* p = buffer;
    char* endP = buffer + bufferSize - 1;
    char c = in.get();
    while (!in.eof() && c == ' ')
    {
        c = in.get();
    }
    while (!in.eof() && c != '\r' && c != '\n')
    {
        if (p < endP)
        {
            *(p++) = c;
        }
        c = in.get();
    }
    *p = 0;
    while (!in.eof() && (c == '\r' || c == '\n'))
    {
        c = in.get();
    }
    if (!in.eof())
    {
        in.unget();
    }
}

} // namespace Poseidon
bool OpenScript(QIFStreamB& in, RString name)
{
    using namespace Poseidon;
    RString fullname;
    if (name[0] == '\\')
    {
        fullname = (const char*)name + 1;
        if (!QIFStreamB::FileExist(fullname))
        {
            Foundation::WarningMessage("Script %s not found", (const char*)name);
            fullname = "";
        }
    }
    else
    {
        fullname = Poseidon::FindScript(name);
    }

#if DIAG_DEBUG >= 20
    LOG_DEBUG(Script, "Load {}", (const char*)name);
#endif

    in.AutoOpen(fullname);
    return !in.fail();
}
namespace Poseidon
{

Script::Script(RString name, GameValuePar argument, int maxLines)
{
    _argument = argument;
    _baseTime = Glob.time;
    _currentLine = 0;
    _maxLinesAtOnce = maxLines;

    _time = 0;
    _suspendedUntil = 0;
    _name = name;

    QIFStreamB in;
    OpenScript(in, name);
    while (!in.eof() && !in.fail())
    {
        char buffer[4096];
        ReadLine(in, buffer, sizeof(buffer));
        ProcessLine(buffer);
    }
#if SCRIPTS_DEBUGGING
    InitDebugger();
#endif
}

Script::Script(const AutoArray<RString>& lines, GameValuePar argument, int maxLines)
{
    _argument = argument;
    _baseTime = Glob.time;
    _currentLine = 0;
    _maxLinesAtOnce = maxLines;

    _time = 0;
    _suspendedUntil = 0;
    _name = "Constructed";

    for (int i = 0; i < lines.Size(); i++)
    {
        ProcessLine(lines[i]);
    }
#if SCRIPTS_DEBUGGING
    InitDebugger();
#endif
}

Script::~Script()
{
#if SCRIPTS_DEBUGGING
    DoneDebugger();
#endif
}

static void Trim(RString& str)
{
    int n = str.GetLength();
    int beg = 0, end = n;
    for (int i = 0; i < n; i++)
    {
        if (!isspace(str[i]))
        {
            beg = i;
            break;
        }
    }
    for (int i = n - 1; i >= 0; i--)
    {
        if (!isspace(str[i]))
        {
            end = i + 1;
            break;
        }
    }
    str = str.Substring(beg, end);
}

void Script::ProcessLine(const char* line)
{
    const char* pos = line;
    while (*pos > 0 && isspace(*pos))
    {
        pos++;
    }

    static RString trueCond("");
    static RString emptyCommand("");
    static RString noSuspend("");

    switch (*pos)
    {
        case 0:   // empty line
        case ';': // comment
            break;
        case '#': // label
        {
            int i = _labels.Add();
            _labels[i].name = pos + 1;
            _labels[i].line = _lines.Size();
            Trim(_labels[i].name);
        }
        break;
        case '&': // time wait condition (absolute)
        {
            const char* time = pos + 1;
            int i = _lines.Add();
            _lines[i].waitUntil = trueCond;
            _lines[i].suspendUntil = time;
            _lines[i].condition = trueCond;
            _lines[i].statement = emptyCommand;
        }
        break;
        case '~': // time wait condition (relative)
        {
            const char* time = pos + 1;
            char buf[256];
            snprintf(buf, sizeof(buf), "__waituntil = _time+(%s)", time);

            int i = _lines.Add();
            _lines[i].waitUntil = trueCond;
            _lines[i].suspendUntil = noSuspend;
            _lines[i].condition = trueCond;
            _lines[i].statement = buf;

            i = _lines.Add();
            _lines[i].waitUntil = trueCond;
            _lines[i].suspendUntil = "__waituntil";
            _lines[i].condition = trueCond;
            _lines[i].statement = emptyCommand;
        }
        break;
        case '@': // wait condition
        {
            int i = _lines.Add();
            const char* until = pos + 1;
            _lines[i].waitUntil = until;
            _lines[i].suspendUntil = noSuspend;
            _lines[i].condition = trueCond;
            _lines[i].statement = emptyCommand;
        }
        break;
        case '?': // condition : statement
        {
            const char* field1 = pos + 1;
            while (isspace(*field1))
            {
                field1++;
            }
            const char* field2 = strchr(field1, ':');
            if (!field2)
            {
                RptF("Only one field in line \"%s\".", line);
                break;
            }
            const char* field1End = field2;
            field2++;

            while (isspace(*field2))
            {
                field2++;
            }
            int i = _lines.Add();
            _lines[i].waitUntil = trueCond;
            _lines[i].suspendUntil = noSuspend;
            _lines[i].condition = *field1 ? RString(field1, field1End - field1) : RString((const char*)trueCond);
            _lines[i].statement = field2;
        }
        break;
        default:
        {
            const char* field1 = pos;
            while (isspace(*field1))
            {
                field1++;
            }
            int i = _lines.Add();
            _lines[i].waitUntil = trueCond;
            _lines[i].suspendUntil = noSuspend;
            _lines[i].condition = trueCond;
            _lines[i].statement = field1;
        }
        break;
    }
}

LSError ScriptLine::Serialize(ParamArchive& ar)
{
    PARAM_CHECK(ar.Serialize("waitUntil", waitUntil, 1, "true"))
    PARAM_CHECK(ar.Serialize("suspendUntil", suspendUntil, 1, "0"))
    PARAM_CHECK(ar.Serialize("condition", condition, 1, "true"))
    PARAM_CHECK(ar.Serialize("statement", statement, 1))
    return LSOK;
}

LSError ScriptLabel::Serialize(ParamArchive& ar)
{
    PARAM_CHECK(ar.Serialize("name", name, 1))
    PARAM_CHECK(ar.Serialize("line", line, 1, 0))
    return LSOK;
}

LSError Script::Serialize(ParamArchive& ar)
{
    if (!GWorld->GetGameState())
    {
        LOG_ERROR(Script, "Error: cannot load script - GWorld->GetGameState() == nullptr");
        return LSStructure;
    }

    if (ar.GetArVersion() >= 4)
    {
        void* old = ar.GetParams();
        ar.SetParams(GWorld->GetGameState());
        PARAM_CHECK(ar.Serialize("argument", _argument, 4))
        ar.SetParams(old);
    }
    else
    {
        if (ar.IsLoading())
        {
            OLink<Object> veh;
            PARAM_CHECK(ar.SerializeRef("Vehicle", veh, 1))
            _argument = GameValueExt(veh.GetLink());
        }
    }
    PARAM_CHECK(ar.Serialize("baseTime", _baseTime, 1))
    PARAM_CHECK(ar.Serialize("currentLine", _currentLine, 1, 0))
    PARAM_CHECK(ar.Serialize("Lines", _lines, 1))
    PARAM_CHECK(ar.Serialize("Labels", _labels, 1))
    PARAM_CHECK(ar.Serialize("Name", _name, 1, RString()))
    PARAM_CHECK(ar.Serialize("maxLinesAtOnce", _maxLinesAtOnce, 1, -1))

    void* old = ar.GetParams();
    ar.SetParams(GWorld->GetGameState());
    PARAM_CHECK(ar.Serialize("Vars", _vars._vars, 1))
    ar.SetParams(old);

    LSError err = ar.Serialize("time", _time, 1);
    if (err != LSOK)
    {
        if (ar.IsSaving())
        {
            return err;
        }
        else
        {
            _time = VarGet("_time");
        }
    }
    PARAM_CHECK(ar.Serialize("suspendedUntil", _suspendedUntil, 1, -FLT_MAX))

    return LSOK;
}

static Script* CurrentScript = nullptr;

bool Script::IsTerminated() const
{
    return _currentLine >= _lines.Size();
}

bool Script::SimulateBody()
{
#if SCRIPTS_DEBUGGING
    if (GDebugger)
    {
        EnvDTE::Breakpoints* breakpoints = nullptr;
        GDebugger->get_Breakpoints(&breakpoints);
        long n = 0;
        breakpoints->get_Count(&n);
        for (long i = 0; i < n; i++)
        {
            VARIANT index;
            index.vt = VT_I4;
            index.lVal = i;

            EnvDTE::Breakpoint* breakpoint = nullptr;
            breakpoints->Item(index, &breakpoint);
            BSTR filename;
            breakpoint->get_File(&filename);
        }
    }
#endif

    GameState* gstate = GWorld->GetGameState();
    gstate->VarSetLocal("_this", _argument, true);
// if simulate body takes too long, interrupt it
#if DIAG_DEBUG >= 10
    DWORD timeStart = Foundation::GlobalTickCount();
#endif
    int cntExecute = 0;
    int maxExecute = _maxLinesAtOnce > 0 ? _maxLinesAtOnce : 100;
    while (_currentLine < _lines.Size())
    {
        ScriptLine& line = _lines[_currentLine];
        if (line.suspendUntil.GetLength() > 0)
        {
            _suspendedUntil = gstate->Evaluate(line.suspendUntil);
            if (_time < _suspendedUntil)
            {
                break;
            }
        }
        if (line.waitUntil.GetLength() > 0 && !gstate->EvaluateBool(line.waitUntil))
        {
            break;
        }
        _currentLine++;
        if (
            // empty condition is considered true
            (line.condition.GetLength() == 0 || gstate->EvaluateBool(line.condition))
            // empty command is not executed
            && line.statement.GetLength() > 0)
        {
#if DIAG_DEBUG >= 20
            LOG_DEBUG(Script, "Execute {}", (const char*)line.statement);
#endif
            gstate->Execute(line.statement);
            _time = gstate->VarGet("_time");
        }
        if (++cntExecute >= maxExecute)
        {
#if DIAG_DEBUG >= 10
            DWORD time = Foundation::GlobalTickCount();
            LOG_DEBUG(Script, "Script interrupted after {} ms", time - timeStart);
#endif
            break;
        }
    }
#if DIAG_DEBUG >= 10
    if (cntExecute > 0)
    {
        DWORD time = Foundation::GlobalTickCount();
        LOG_DEBUG(Script, "Script paused after {} ms", time - timeStart);
    }
#endif
    return _currentLine >= _lines.Size();
}

bool Script::Simulate(float deltaT)
{
    if (CurrentScript)
    {
        return false;
    }
    bool ret = false;
    if (_time >= _suspendedUntil)
    {
        CurrentScript = this;
        GameState* gstate = GWorld->GetGameState();
        gstate->BeginContext(&_vars);
        gstate->VarSet("_time", _time); // make sure there is such variable
        ret = SimulateBody();
        _time = gstate->VarGet("_time");
        gstate->EndContext();
        CurrentScript = nullptr;
    }
    _time += deltaT;
    return ret;
}

bool Script::OnSimulate()
{
    if (CurrentScript)
    {
        return false;
    }
    bool ret = false;
    _time = Glob.time - _baseTime;
    if (_time >= _suspendedUntil)
    {
        CurrentScript = this;
        GameState* gstate = GWorld->GetGameState();
        gstate->BeginContext(&_vars);
        gstate->VarSet("_time", _time);
        ret = SimulateBody();
        _time = gstate->VarGet("_time");
        _baseTime = Glob.time - _time;
        gstate->EndContext();
        CurrentScript = nullptr;
    }
    return ret;
}

GameValue Script::VarGet(const char* name) const
{
    GameState* gstate = GWorld->GetGameState();
    gstate->BeginContext(const_cast<GameVarSpace*>(&_vars));
    GameValuePar retval = gstate->VarGet(name);
    gstate->EndContext();
    return retval;
}

void Script::VarSet(const char* name, GameValuePar value, bool readOnly)
{
    GameState* gstate = GWorld->GetGameState();
    gstate->BeginContext(&_vars);
    gstate->VarSet(name, value, readOnly);
    gstate->EndContext();
}

void Script::GoTo(RString label)
{
    for (int i = 0; i < _labels.Size(); i++)
    {
        if (stricmp(_labels[i].name, label) == 0)
        {
            _currentLine = _labels[i].line;
            return;
        }
    }
    LOG_DEBUG(Script, "Unknown label {}.", (const char*)label);
    Exit();
}

void Script::Exit()
{
    _currentLine = _lines.Size();
}

#define NOTHING GameValue()

} // namespace Poseidon
GameValue ScriptExecute(const GameState* state, GameValuePar oper1, GameValuePar oper2)
{
    using namespace Poseidon;
    DoAssert(oper2.GetType() == GameString) Script* script = new Script(oper2, oper1);
    GWorld->AddScript(script);
    return NOTHING;
}
namespace Poseidon
{

} // namespace Poseidon
GameValue ScriptExit(const GameState* state)
{
    using namespace Poseidon;
    if (CurrentScript)
    {
        CurrentScript->Exit();
    }
    return NOTHING;
}
namespace Poseidon
{

} // namespace Poseidon
GameValue ScriptGoto(const GameState* state, GameValuePar oper1)
{
    using namespace Poseidon;
    DoAssert(oper1.GetType() == GameString) if (CurrentScript)
    {
        CurrentScript->GoTo(oper1);
    }
    return NOTHING;
}
