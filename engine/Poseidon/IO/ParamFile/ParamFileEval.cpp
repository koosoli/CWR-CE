
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>

#include <Poseidon/IO/Streams/SerializeBin.hpp>
#include <Evaluator/express.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{
// EvaluatorFunctions implementation backed by the game state. The *Internal
// variants skip the game-state context init/deinit that the plain variants do.
class GameStateEvaluatorFunctions : public EvaluatorFunctions
{
  public:
    GameVarSpace* CreateVariables() override;
    void DeleteVariables(GameVarSpace* vars) override;
    void InitEvaluator(GameVarSpace* vars) override;
    void DoneEvaluator() override;
    GameVarSpace* LoadVariables(SerializeBinStream& f) override;
    void SaveVariables(SerializeBinStream& f, GameVarSpace* vars) override;
    float EvaluateFloat(const char* expr, GameVarSpace* vars) override;
    float EvaluateFloatInternal(const char* expr) override;
    RString EvaluateStringInternal(const char* expr) override;
    void ExecuteInternal(const char* expr) override;
    void VarSetFloatInternal(const char* name, float value, bool readOnly, bool forceLocal) override;

    GameStateEvaluatorFunctions() { ParamFile::SetDefaultEvalFunctions(this); }
};

// Meyers singleton - no global constructor
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
static GameStateEvaluatorFunctions& GGameStateEvaluatorFunctions()
{
    static GameStateEvaluatorFunctions instance;
    return instance;
}
#pragma clang diagnostic pop

void InitParamFileEvaluator()
{
    // Force initialization of singleton
    (void)GGameStateEvaluatorFunctions();
}

GameVarSpace* GameStateEvaluatorFunctions::CreateVariables()
{
    return new GameVarSpace();
}

void GameStateEvaluatorFunctions::DeleteVariables(GameVarSpace* vars)
{
    delete vars;
}

// Must be paired with a DoneEvaluator() call.
void GameStateEvaluatorFunctions::InitEvaluator(GameVarSpace* vars)
{
    GGameState.BeginContext(vars);
}

void GameStateEvaluatorFunctions::DoneEvaluator()
{
    GGameState.EndContext();
}

GameVarSpace* GameStateEvaluatorFunctions::LoadVariables(SerializeBinStream& f)
{
    GameVarSpace* vars = new GameVarSpace();
    int n;
    f.Transfer(n);
    for (int i = 0; i < n; i++)
    {
        RString name;
        int value;
        f.Transfer(name);
        f.Transfer(value);
        GameVariable var(name, (float)value, true);
        vars->_vars.Add(var);
    }
    return vars;
}

void GameStateEvaluatorFunctions::SaveVariables(SerializeBinStream& f, GameVarSpace* vars)
{
    int n = vars ? vars->_vars.NItems() : 0;
    f.Transfer(n);
    if (n > 0)
    {
        for (int i = 0; i < vars->_vars.NTables(); i++)
        {
            AutoArray<GameVariable>& table = vars->_vars.GetTable(i);
            for (int j = 0; j < table.Size(); j++)
            {
                RString name = table[j]._name;
                int value = toInt((float)table[j]._value);
                f.Transfer(name);
                f.Transfer(value);
            }
        }
    }
}

float GameStateEvaluatorFunctions::EvaluateFloat(const char* expr, GameVarSpace* vars)
{
    GGameState.BeginContext(vars);
    GameValue result = GGameState.Evaluate(expr);
    GGameState.EndContext();
    return result;
}

float GameStateEvaluatorFunctions::EvaluateFloatInternal(const char* expr)
{
    GameValue result = GGameState.Evaluate(expr);
    return result;
}

RString GameStateEvaluatorFunctions::EvaluateStringInternal(const char* expr)
{
    return GGameState.Evaluate(expr).GetText();
}

void GameStateEvaluatorFunctions::ExecuteInternal(const char* expr)
{
    GGameState.Execute(expr);
}

void GameStateEvaluatorFunctions::VarSetFloatInternal(const char* name, float value, bool readOnly, bool forceLocal)
{
    GGameState.VarSet(name, GameValue(value), readOnly, forceLocal);
}

} // namespace Poseidon
