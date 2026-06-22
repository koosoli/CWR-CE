#include <Evaluator/SqsRunner.hpp>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <Poseidon/Foundation/platform.hpp>

static std::string trim(const std::string& s)
{
    size_t beg = s.find_first_not_of(" \t\r\n");
    if (beg == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(beg, end - beg + 1);
}

void SqsRunner::parse(const std::string& content)
{
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        parseLine(line.c_str());
    }
}

// Mirrors Script::ProcessLine from Poseidon/Game/Scripting/Scripts.cpp
void SqsRunner::parseLine(const char* line)
{
    const char* pos = line;
    while (*pos && isspace((unsigned char)*pos))
        pos++;

    switch (*pos)
    {
        case 0:   // empty
        case ';': // comment
            break;
        case '#': // label
        {
            std::string lbl = trim(pos + 1);
            _labels.push_back({lbl, (int)_lines.size()});
        }
        break;
        case '&': // fork (treat as suspend in evaluator)
        {
            const char* time = pos + 1;
            SqsLine sl;
            sl.waitUntil = "";
            sl.suspendUntil = trim(time);
            sl.condition = "";
            sl.statement = "";
            _lines.push_back(sl);
        }
        break;
        case '~': // relative delay
        {
            const char* time = pos + 1;
            char buf[256];
            snprintf(buf, sizeof(buf), "__waituntil = _time+(%s)", trim(time).c_str());

            SqsLine sl1;
            sl1.waitUntil = "";
            sl1.suspendUntil = "";
            sl1.condition = "";
            sl1.statement = buf;
            _lines.push_back(sl1);

            SqsLine sl2;
            sl2.waitUntil = "";
            sl2.suspendUntil = "__waituntil";
            sl2.condition = "";
            sl2.statement = "";
            _lines.push_back(sl2);
        }
        break;
        case '@': // wait condition
        {
            SqsLine sl;
            sl.waitUntil = trim(pos + 1);
            sl.suspendUntil = "";
            sl.condition = "";
            sl.statement = "";
            _lines.push_back(sl);
        }
        break;
        case '?': // condition : statement
        {
            const char* field1 = pos + 1;
            while (isspace((unsigned char)*field1))
                field1++;
            const char* field2 = strchr(field1, ':');
            if (!field2)
                break;
            const char* field1End = field2;
            field2++;
            while (isspace((unsigned char)*field2))
                field2++;

            SqsLine sl;
            sl.waitUntil = "";
            sl.suspendUntil = "";
            sl.condition = *field1 ? std::string(field1, field1End - field1) : "";
            sl.statement = field2;
            _lines.push_back(sl);
        }
        break;
        default:
        {
            SqsLine sl;
            sl.waitUntil = "";
            sl.suspendUntil = "";
            sl.condition = "";
            sl.statement = trim(pos);
            _lines.push_back(sl);
        }
        break;
    }
}

int SqsRunner::run(GameState& state, GameValuePar argument)
{
    _currentLine = 0;
    _exited = false;

    GameVarSpace local(state.GetContext());
    state.BeginContext(&local);
    state.VarSetLocal("_this", argument, true);
    state.VarSet("_time", GameValue(0.0f));

    int maxIter = 10000;
    int iter = 0;

    while (!isTerminated() && iter++ < maxIter)
    {
        SqsLine& line = _lines[_currentLine];

        // Handle suspend (delay) - in non-realtime, skip immediately
        if (!line.suspendUntil.empty() && !_realtime)
        {
            if (!line.suspendUntil.empty())
            {
                // evaluate the suspend expression but skip waiting
                state.Evaluate(line.suspendUntil.c_str());
            }
        }

        // Handle wait condition - in non-realtime, evaluate once and continue
        if (!line.waitUntil.empty() && !_realtime)
        {
            state.EvaluateBool(line.waitUntil.c_str());
        }

        _currentLine++;

        bool condMet = line.condition.empty() || state.EvaluateBool(line.condition.c_str());

        if (condMet && !line.statement.empty())
        {
            // Check for goto
            std::string stmt = trim(line.statement);
            if (strnicmp(stmt.c_str(), "goto", 4) == 0 && (stmt.size() == 4 || isspace((unsigned char)stmt[4])))
            {
                std::string label = trim(stmt.substr(4));
                // strip quotes if present
                if (label.size() >= 2 && label.front() == '"' && label.back() == '"')
                    label = label.substr(1, label.size() - 2);
                goTo(label);
                continue;
            }

            // Check for exit
            if (stricmp(stmt.c_str(), "exit") == 0)
            {
                exit();
                break;
            }

            state.Execute(stmt.c_str());

            if (state.GetLastError() != EvalOK)
            {
                state.EndContext();
                return 1;
            }
        }
    }

    state.EndContext();
    return 0;
}

void SqsRunner::goTo(const std::string& label)
{
    for (auto& lbl : _labels)
    {
        if (stricmp(lbl.name.c_str(), label.c_str()) == 0)
        {
            _currentLine = lbl.line;
            return;
        }
    }
    // Unknown label - exit
    exit();
}
