#pragma once
#include <Evaluator/express.hpp>
#include <Evaluator/MockObjects.hpp>
#include <string>

class EvalState : public GameState
{
    std::string _outputBuffer;

  public:
    ObjectRegistry objects;
    GroupRegistry groups;
    int playerId = 0;

    EvalState() = default;

    void RegisterEvalCommands();
    void RegisterObjectCommands();
    void RegisterCreationCommands();
    void RegisterPropertyCommands();
    void RegisterVariableCommands();
    void RegisterScriptCommands();

    void appendOutput(const std::string& text) { _outputBuffer += text; }
    void clearOutput() { _outputBuffer.clear(); }
    const std::string& getOutput() const { return _outputBuffer; }
};
