#pragma once

// ============================================================================
// Evaluator testing - shared test fixture
// ============================================================================
// Common fixture for all evaluator tests to ensure GGameState is initialized.
// ============================================================================

// Helper fixture to ensure GGameState is initialized
class EvaluatorFixture
{
    static bool s_initialized;
    
public:
    EvaluatorFixture()
    {
        // Initialize GameState with default operators/functions only once
        if (!s_initialized)
        {
            GGameState.Init();
            s_initialized = true;
        }
    }
};

// Initialize static member
inline bool EvaluatorFixture::s_initialized = false;
