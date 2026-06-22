#ifndef POSEIDON_TEST_HARNESS_PROTOCOL_HPP
#define POSEIDON_TEST_HARNESS_PROTOCOL_HPP

// Harness protocol: JSON command/event builders and parsers using cJSON.

#include <cjson/cJSON.h>
#include <string>
#include <vector>
#include <cstdint>

namespace Poseidon::Dev {
namespace HarnessProtocol
{

/// Protocol version — must match engine/Trident/protocol/harness.schema.json
constexpr int VERSION = 1;

// ── Response Builders ──────────────────────────────────────────────────────

/// Build {"ok":true} + optional extra fields. Caller must cJSON_Delete result.
inline std::string OkResponse()
{
    return "{\"ok\":true}\n";
}

/// Build {"ok":false,"error":"msg"}.
inline std::string ErrorResponse(const char* msg)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddFalseToObject(root, "ok");
    cJSON_AddStringToObject(root, "error", msg);
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

/// Build a response with arbitrary data fields.
inline std::string JsonResponse(cJSON* root)
{
    cJSON_AddTrueToObject(root, "ok");
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

// ── Event Builders ─────────────────────────────────────────────────────────

inline std::string ReadyEvent(int idd)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", "ready");
    cJSON_AddNumberToObject(root, "idd", idd);
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

inline std::string DisplayEvent(int idd, const char* name)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", "display");
    cJSON_AddNumberToObject(root, "idd", idd);
    if (name && name[0])
        cJSON_AddStringToObject(root, "name", name);
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

inline std::string ExitEvent(int code)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", "exit");
    cJSON_AddNumberToObject(root, "code", code);
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

inline std::string PlayerJoinedEvent(int dpid, const char* name)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", "player_joined");
    cJSON_AddNumberToObject(root, "dpid", dpid);
    cJSON_AddStringToObject(root, "name", name ? name : "");
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

inline std::string PlayerLeftEvent(int dpid, const char* name)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", "player_left");
    cJSON_AddNumberToObject(root, "dpid", dpid);
    cJSON_AddStringToObject(root, "name", name ? name : "");
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

inline std::string MissionStateEvent(const char* prev, const char* state)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", "mission_state");
    cJSON_AddStringToObject(root, "prev", prev ? prev : "");
    cJSON_AddStringToObject(root, "state", state ? state : "");
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

inline std::string VonReceivedEvent(uint32_t channel, int frames)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", "von_received");
    cJSON_AddNumberToObject(root, "channel", channel);
    cJSON_AddNumberToObject(root, "frames", frames);
    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

// ── Command Info for describe ──────────────────────────────────────────────

struct ParamInfo
{
    const char* name;
    const char* type;
    bool required;
};

struct CommandRegistration
{
    const char* name;
    const char* description;
    std::vector<ParamInfo> params;
};

struct FieldInfo
{
    const char* name;
    const char* type;
};

struct EventRegistration
{
    const char* name;
    const char* description;
    std::vector<FieldInfo> fields;
};

/// Build the describe response from registered commands and events.
inline std::string DescribeResponse(const std::vector<CommandRegistration>& commands,
                                    const std::vector<EventRegistration>& events)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddTrueToObject(root, "ok");
    cJSON_AddNumberToObject(root, "version", VERSION);

    cJSON* cmds = cJSON_AddArrayToObject(root, "commands");
    for (const auto& cmd : commands)
    {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", cmd.name);
        cJSON_AddStringToObject(item, "description", cmd.description);
        cJSON* params = cJSON_AddArrayToObject(item, "params");
        for (const auto& p : cmd.params)
        {
            cJSON* pi = cJSON_CreateObject();
            cJSON_AddStringToObject(pi, "name", p.name);
            cJSON_AddStringToObject(pi, "type", p.type);
            cJSON_AddBoolToObject(pi, "required", p.required);
            cJSON_AddItemToArray(params, pi);
        }
        cJSON_AddItemToArray(cmds, item);
    }

    cJSON* evts = cJSON_AddArrayToObject(root, "events");
    for (const auto& evt : events)
    {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", evt.name);
        cJSON_AddStringToObject(item, "description", evt.description);
        cJSON* fields = cJSON_AddArrayToObject(item, "fields");
        for (const auto& f : evt.fields)
        {
            cJSON* fi = cJSON_CreateObject();
            cJSON_AddStringToObject(fi, "name", f.name);
            cJSON_AddStringToObject(fi, "type", f.type);
            cJSON_AddItemToArray(fields, fi);
        }
        cJSON_AddItemToArray(evts, item);
    }

    char* out = cJSON_PrintUnformatted(root);
    std::string result(out);
    result += '\n';
    cJSON_free(out);
    cJSON_Delete(root);
    return result;
}

// ── Command Parsing Helpers ────────────────────────────────────────────────

/// Get string field from parsed cJSON object, or empty string.
inline const char* GetString(cJSON* root, const char* field)
{
    cJSON* item = cJSON_GetObjectItemCaseSensitive(root, field);
    if (item && cJSON_IsString(item))
        return item->valuestring;
    return "";
}

/// Get int field from parsed cJSON object, or default value.
inline int GetInt(cJSON* root, const char* field, int defaultVal = 0)
{
    cJSON* item = cJSON_GetObjectItemCaseSensitive(root, field);
    if (item && cJSON_IsNumber(item))
        return item->valueint;
    return defaultVal;
}

/// Get bool field from parsed cJSON object, or default value.
inline bool GetBool(cJSON* root, const char* field, bool defaultVal = false)
{
    cJSON* item = cJSON_GetObjectItemCaseSensitive(root, field);
    if (item)
    {
        if (cJSON_IsTrue(item))
            return true;
        if (cJSON_IsFalse(item))
            return false;
    }
    return defaultVal;
}

} // namespace HarnessProtocol
} // namespace Poseidon::Dev

#endif // POSEIDON_TEST_HARNESS_PROTOCOL_HPP
