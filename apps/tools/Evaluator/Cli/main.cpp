#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

#include <Evaluator/EvaluatorHost.hpp>
#include <CLI/App.hpp>
#include <CLI/Error.hpp>
#include <CLI/Option.hpp>

namespace
{
void printRunResult(const EvaluatorRunResult& result)
{
    if (!result.stdoutText.empty())
        std::fwrite(result.stdoutText.data(), 1, result.stdoutText.size(), stdout);
    if (!result.stderrText.empty())
        std::fwrite(result.stderrText.data(), 1, result.stderrText.size(), stderr);
}
} // namespace

int main(int argc, char* argv[])
{
    CLI::App app{"PoseidonEvaluator - OFP SQF/SQS script evaluator"};

    std::string evalExpr;
    std::string testFile;
    std::string scriptFile;
    std::string flavor;
    std::string validateBookDir;
    std::string validateRefDir;
    std::vector<std::string> scriptArgs;
    bool verbose = false;

    app.add_flag("-v,--verbose", verbose, "Show evaluation trace on stderr");
    app.add_option("--eval", evalExpr, "Evaluate an inline SQF expression");
    app.add_option("--test", testFile, "Run script in test mode (exit 0/1)");
    app.add_option("--flavor", flavor, "Force script type: sqf or sqs (auto-detect from extension by default)");
    app.add_option("--validate-book", validateBookDir, "Validate code examples in book markdown files");
    app.add_option("--validate-ref", validateRefDir, "Validate function reference examples from JSON files");
    app.add_option("script", scriptFile, "Script file to run (.sqf or .sqs)");
    app.add_option("args", scriptArgs, "Arguments passed to script as _this");

    app.get_option("--eval")->excludes(app.get_option("--test"));
    app.get_option("--eval")->excludes(app.get_option("script"));
    app.get_option("--test")->excludes(app.get_option("script"));

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e)
    {
        return app.exit(e);
    }

    EvaluatorHost host(verbose);

    if (!validateBookDir.empty())
    {
        auto sum = host.ValidateBook(validateBookDir);
        std::cout << EvaluatorHost::FormatValidationSummary(sum, true);
        return sum.failed > 0 ? 1 : 0;
    }

    if (!validateRefDir.empty())
    {
        auto sum = host.ValidateRef(validateRefDir);
        std::cout << EvaluatorHost::FormatValidationSummary(sum, true);
        if (EvaluatorHost::WriteValidateReport(sum, "validate-report.json"))
            fprintf(stderr, "Report written to validate-report.json\n");
        return sum.failed > 0 ? 1 : 0;
    }

    if (!evalExpr.empty())
    {
        EvaluatorRunResult result;
        if (flavor == "sqs")
            result = host.EvaluateInlineSqs(evalExpr);
        else
            result = host.EvaluateExpression(evalExpr);
        printRunResult(result);
        return result.exitCode;
    }

    if (!testFile.empty())
    {
        EvaluatorRunResult result;
        bool useSqs = (flavor == "sqs") || (flavor.empty() && EvaluatorHost::IsSqsFile(testFile));
        if (useSqs)
            result = host.EvaluateSqsFile(testFile);
        else
            result = host.EvaluateSqfFile(testFile, true);
        printRunResult(result);
        return result.exitCode;
    }

    if (!scriptFile.empty())
    {
        EvaluatorRunResult result;
        bool useSqs = (flavor == "sqs") || (flavor.empty() && EvaluatorHost::IsSqsFile(scriptFile));
        if (useSqs)
            result = host.EvaluateSqsFile(scriptFile, scriptArgs);
        else
            result = host.EvaluateSqfFile(scriptFile, false, scriptArgs);
        printRunResult(result);
        return result.exitCode;
    }

    std::cout << app.help();
    return 0;
}
