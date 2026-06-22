#pragma once

#include <string>
#include <map>
#include <memory>


namespace Poseidon::Foundation { class AppConfig; }
using AppConfig = Poseidon::Foundation::AppConfig;

namespace Poseidon
{
class EngineConfig;
class UserConfig;
struct RuntimeFlags;
struct EngineState;

/// Unified configuration system with source tracking
class ConfigurationSystem
{
  public:
    ConfigurationSystem();
    ~ConfigurationSystem();

    /// Initialize from all sources in priority order; CLI args win.
    void Initialize(const AppConfig& cliArgs);

    /// Flushes any pending changes.
    void Shutdown();

    /// Initialize language, stringtable, modules, and config files: consolidates
    /// ReadRegistry + language setup + ParseStringtable/Config/Resource. An empty
    /// language reads it from config. Returns true on success.
    bool InitializeGameConfiguration(const char* language = nullptr);

    /// Push the loaded configuration into the global Config struct.
    void ApplyToLegacyGlobConfig();

    int GetInt(const char* key, int defaultValue = 0) const;
    bool GetBool(const char* key, bool defaultValue = false) const;
    std::string GetString(const char* key, const std::string& defaultValue = "") const;
    float GetFloat(const char* key, float defaultValue = 0.0f) const;
    bool HasKey(const char* key) const;

    /// Source of a value: "default", "registry", or "CLI".
    std::string GetSource(const char* key) const;

    EngineConfig& GetEngineConfig() { return *m_engineConfig; }
    const EngineConfig& GetEngineConfig() const { return *m_engineConfig; }

    UserConfig& GetUserConfig() { return *m_userConfig; }
    const UserConfig& GetUserConfig() const { return *m_userConfig; }

    /// Engine-internal runtime flags (CLI args, never serialized).
    RuntimeFlags& GetRuntimeFlags() { return *m_runtimeFlags; }
    const RuntimeFlags& GetRuntimeFlags() const { return *m_runtimeFlags; }

    /// Engine runtime state (visibility distances, computed fields).
    EngineState& GetEngineState() { return *m_engineState; }
    const EngineState& GetEngineState() const { return *m_engineState; }

  private:
    struct ConfigValue
    {
        std::string value;
        std::string source;
    };

    /// Set a config value (used during loading).
    void SetValue(const char* key, const std::string& value, const char* source);

    /// Load configuration layers in priority order.
    void LoadDefaults();
    void ApplyCommandLine(const AppConfig& cliArgs);

    std::unique_ptr<RuntimeFlags> m_runtimeFlags;
    std::unique_ptr<EngineState> m_engineState;
    std::unique_ptr<EngineConfig> m_engineConfig;
    std::unique_ptr<UserConfig> m_userConfig;

    std::map<std::string, ConfigValue> m_config;
    bool m_initialized;
};
} // namespace Poseidon
