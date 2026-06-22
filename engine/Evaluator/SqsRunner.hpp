#pragma once
#include <Evaluator/express.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <string>
#include <vector>
#include <functional>

struct SqsLine
{
    std::string waitUntil;
    std::string suspendUntil;
    std::string condition;
    std::string statement;
};

struct SqsLabel
{
    std::string name;
    int line;
};

class SqsRunner
{
    std::vector<SqsLine> _lines;
    std::vector<SqsLabel> _labels;
    int _currentLine = 0;
    bool _exited = false;
    bool _realtime = false;
    std::string _name;

  public:
    SqsRunner() = default;
    SqsRunner(const std::string& name, bool realtime = false) : _realtime(realtime), _name(name) {}

    void parse(const std::string& content);
    void parseLine(const char* line);
    int run(GameState& state, GameValuePar argument);

    bool isTerminated() const { return _exited || _currentLine >= (int)_lines.size(); }
    void goTo(const std::string& label);
    void exit() { _exited = true; }

    const std::vector<SqsLine>& lines() const { return _lines; }
    const std::vector<SqsLabel>& labels() const { return _labels; }
    const std::string& name() const { return _name; }
};
