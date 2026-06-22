#pragma once

#include <string>
#include <vector>

#include <Evaluator/EvalState.hpp>
#include <Evaluator/Validate.hpp>

struct EvaluatorRunResult
{
    int exitCode = 0;
    std::string stdoutText;
    std::string stderrText;
};

class EvaluatorHost
{
    EvalState _state;
    bool _initialized = false;
    bool _verbose = false;

    void EnsureInitialized();

  public:
    explicit EvaluatorHost(bool verbose = false) : _verbose(verbose) {}

    void SetVerbose(bool verbose) { _verbose = verbose; }
    bool GetVerbose() const { return _verbose; }

    EvalState& State();

    EvaluatorRunResult EvaluateExpression(const std::string& expr);
    EvaluatorRunResult EvaluateInlineSqs(const std::string& script, const std::string& name = "eval");
    EvaluatorRunResult EvaluateSqfFile(const std::string& path, bool testMode = false,
                                       const std::vector<std::string>& scriptArgs = {});
    EvaluatorRunResult EvaluateSqsFile(const std::string& path, const std::vector<std::string>& scriptArgs = {});

    ValidationSummary ValidateBook(const std::string& bookDir);
    ValidationSummary ValidateRef(const std::string& refDir);

    static bool IsSqsFile(const std::string& path);
    static std::string FormatValidationSummary(const ValidationSummary& sum, bool showDetails);
    static bool WriteValidateReport(const ValidationSummary& sum, const std::string& path);
};
