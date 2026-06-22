#pragma once
#include <string>
#include <vector>

struct ValidationResult
{
    std::string source;
    int index;
    std::string status; // "pass", "fail", "skip"
    std::string error;
    std::string code;
};

struct ValidationSummary
{
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    std::vector<ValidationResult> results;
};

class GameState;

// Extract code blocks from markdown files and validate them
ValidationSummary validateBook(const std::string& bookDir, GameState& state, bool verbose = false);

// Validate function reference examples from JSON files
ValidationSummary validateRef(const std::string& refDir, GameState& state, bool verbose = false);
