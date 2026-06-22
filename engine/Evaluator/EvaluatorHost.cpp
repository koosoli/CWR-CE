#include <Evaluator/EvaluatorHost.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include <Evaluator/SqsRunner.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

void EnsureEvaluatorRuntimeStubsLinked();

namespace
{
std::string readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return {};

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

const char* evalErrorName(EvalError err)
{
    switch (err)
    {
        case EvalOK:
            return "OK";
        case EvalGen:
            return "generic error";
        case EvalExpo:
            return "exponent out of range";
        case EvalNum:
            return "invalid number or unterminated string";
        case EvalVar:
            return "undefined variable";
        case EvalBadVar:
            return "bad variable name";
        case EvalDivZero:
            return "division by zero";
        case EvalTg90:
            return "tangent of 90 degrees";
        case EvalOpenB:
            return "missing '('";
        case EvalCloseB:
            return "missing ')'";
        case EvalEqu:
            return "missing '='";
        case EvalSemicolon:
            return "missing ';'";
        case EvalOper:
            return "unknown operator";
        case EvalLineLong:
            return "expression too long";
        case EvalType:
            return "type mismatch";
        case EvalNamespace:
            return "no local variable scope";
        case EvalDim:
            return "array dimension error";
        default:
            return "unknown error";
    }
}

GameArrayType makeArgsArray(const std::vector<std::string>& scriptArgs)
{
    GameArrayType args;
    for (const auto& arg : scriptArgs)
        args.Add(GameValue(arg.c_str()));
    return args;
}
} // namespace

void EvaluatorHost::EnsureInitialized()
{
    EnsureEvaluatorRuntimeStubsLinked();

    if (_initialized)
        return;

    _state.Init();
    _state.RegisterEvalCommands();
    _initialized = true;
}

EvalState& EvaluatorHost::State()
{
    EnsureInitialized();
    return _state;
}

EvaluatorRunResult EvaluatorHost::EvaluateExpression(const std::string& expr)
{
    EnsureInitialized();
    _state.clearOutput();

    EvaluatorRunResult result;
    if (_verbose)
        result.stderrText += "[VERBOSE] evaluating: " + expr + "\n";

    GameVarSpace local(_state.GetContext());
    _state.BeginContext(&local);

    GameValue value = _state.EvaluateMultiple(expr.c_str());
    EvalError err = _state.GetLastError();

    _state.EndContext();

    result.stdoutText = _state.getOutput();

    if (err != EvalOK)
    {
        RString errText = _state.GetLastErrorText();
        int errPos = _state.GetLastErrorPos();

        if (errText.GetLength() > 0)
            result.stderrText += std::string("Error: ") + static_cast<const char*>(errText) + "\n";
        else
            result.stderrText +=
                std::string("Error: ") + evalErrorName(err) + " (code " + std::to_string((int)err) + ")\n";

        if (errPos > 0 && errPos <= (int)expr.size())
        {
            result.stderrText += "  " + expr + "\n";
            result.stderrText += "  " + std::string(errPos - 1, ' ') + "^\n";
        }

        result.exitCode = 1;
        return result;
    }

    if (_verbose)
    {
        const char* typeName = value.GetData() ? value.GetData()->GetTypeName() : "nil";
        result.stderrText += std::string("[VERBOSE] result type: ") + typeName + "\n";
    }

    if (result.stdoutText.empty() && value.GetType() != GameNothing && !value.GetNil())
    {
        RString text = value.GetText();
        result.stdoutText += static_cast<const char*>(text);
        result.stdoutText += "\n";
    }

    return result;
}

EvaluatorRunResult EvaluatorHost::EvaluateInlineSqs(const std::string& script, const std::string& name)
{
    EnsureInitialized();
    _state.clearOutput();

    EvaluatorRunResult result;
    if (_verbose)
        result.stderrText += "[VERBOSE] running SQS file: " + name + "\n";

    SqsRunner runner(name);
    runner.parse(script);
    result.exitCode = runner.run(_state, GameValue());
    result.stdoutText = _state.getOutput();

    if (result.exitCode != 0)
    {
        RString errText = _state.GetLastErrorText();
        if (errText.GetLength() > 0)
            result.stderrText += std::string("Error: ") + static_cast<const char*>(errText) + "\n";
    }

    return result;
}

EvaluatorRunResult EvaluatorHost::EvaluateSqfFile(const std::string& path, bool testMode,
                                                  const std::vector<std::string>& scriptArgs)
{
    std::string content = readFile(path);
    if (content.empty())
    {
        std::ifstream check(path);
        if (!check.is_open())
        {
            EvaluatorRunResult result;
            result.exitCode = 1;
            result.stderrText = "Error: cannot open file '" + path + "'\n";
            return result;
        }
        return {};
    }

    EnsureInitialized();

    GameArrayType args = makeArgsArray(scriptArgs);

    GameVarSpace local(_state.GetContext());
    _state.BeginContext(&local);
    _state.VarSetLocal("_this", GameValue(args), true);

    _state.clearOutput();

    EvaluatorRunResult result;
    if (_verbose)
        result.stderrText += "[VERBOSE] evaluating file: " + path + "\n";

    GameValue value = _state.EvaluateMultiple(content.c_str());
    EvalError err = _state.GetLastError();
    _state.EndContext();

    result.stdoutText = _state.getOutput();

    if (err != EvalOK)
    {
        RString errText = _state.GetLastErrorText();
        if (errText.GetLength() > 0)
            result.stderrText += std::string("Error: ") + static_cast<const char*>(errText) + "\n";
        else
            result.stderrText +=
                std::string("Error: ") + evalErrorName(err) + " (code " + std::to_string((int)err) + ")\n";

        if (!testMode)
        {
            int errPos = _state.GetLastErrorPos();
            if (errPos > 0 && errPos <= (int)content.size())
            {
                result.stderrText += "  " + content + "\n";
                result.stderrText += "  " + std::string(errPos - 1, ' ') + "^\n";
            }
        }

        result.exitCode = 1;
        return result;
    }

    if (result.stdoutText.empty() && value.GetType() != GameNothing && !value.GetNil())
    {
        RString text = value.GetText();
        result.stdoutText += static_cast<const char*>(text);
        result.stdoutText += "\n";
    }

    return result;
}

EvaluatorRunResult EvaluatorHost::EvaluateSqsFile(const std::string& path, const std::vector<std::string>& scriptArgs)
{
    std::string content = readFile(path);
    if (content.empty())
    {
        std::ifstream check(path);
        if (!check.is_open())
        {
            EvaluatorRunResult result;
            result.exitCode = 1;
            result.stderrText = "Error: cannot open file '" + path + "'\n";
            return result;
        }
        return {};
    }

    EnsureInitialized();
    _state.clearOutput();

    EvaluatorRunResult result;
    if (_verbose)
        result.stderrText += "[VERBOSE] running SQS file: " + path + "\n";

    GameValue argument;
    if (!scriptArgs.empty())
        argument = GameValue(makeArgsArray(scriptArgs));

    SqsRunner runner(path);
    runner.parse(content);
    result.exitCode = runner.run(_state, argument);
    result.stdoutText = _state.getOutput();

    if (result.exitCode != 0)
    {
        RString errText = _state.GetLastErrorText();
        if (errText.GetLength() > 0)
            result.stderrText += std::string("Error: ") + static_cast<const char*>(errText) + "\n";
    }

    return result;
}

ValidationSummary EvaluatorHost::ValidateBook(const std::string& bookDir)
{
    EnsureInitialized();
    return validateBook(bookDir, _state, _verbose);
}

ValidationSummary EvaluatorHost::ValidateRef(const std::string& refDir)
{
    EnsureInitialized();
    return validateRef(refDir, _state, _verbose);
}

bool EvaluatorHost::IsSqsFile(const std::string& path)
{
    if (path.size() < 4)
        return false;

    std::string ext = path.substr(path.size() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return ext == ".sqs";
}

std::string EvaluatorHost::FormatValidationSummary(const ValidationSummary& sum, bool showDetails)
{
    std::ostringstream out;
    out << "Results: " << sum.passed << " passed, " << sum.failed << " failed, " << sum.skipped << " skipped\n";
    if (showDetails)
    {
        for (const auto& r : sum.results)
        {
            if (r.status == "fail")
                out << "  FAIL " << r.source << "[" << r.index << "]: " << r.error << "\n";
            else if (r.status == "skip")
                out << "  SKIP " << r.source << "[" << r.index << "]: " << r.error << "\n";
        }
    }
    return out.str();
}

bool EvaluatorHost::WriteValidateReport(const ValidationSummary& sum, const std::string& path)
{
    FILE* file = fopen(path.c_str(), "w");
    if (!file)
        return false;

    fprintf(file, "[\n");
    for (int i = 0; i < (int)sum.results.size(); i++)
    {
        const auto& r = sum.results[i];
        auto esc = [](const std::string& s)
        {
            std::string out;
            for (char c : s)
            {
                if (c == '"')
                    out += "\\\"";
                else if (c == '\\')
                    out += "\\\\";
                else if (c == '\n')
                    out += "\\n";
                else if (c == '\r')
                    out += "\\r";
                else
                    out += c;
            }
            return out;
        };

        fprintf(file, "  {\"function\": \"%s\", \"example_index\": %d, \"status\": \"%s\", \"error\": \"%s\"}%s\n",
                esc(r.source).c_str(), r.index, r.status.c_str(), esc(r.error).c_str(),
                i + 1 < (int)sum.results.size() ? "," : "");
    }
    fprintf(file, "]\n");
    fclose(file);
    return true;
}
