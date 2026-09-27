#pragma once
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

/**
 * SkyrimNet Public API — typed memory query (v10+).
 *
 * Companion header to PublicAPI.h. Copy both into your SKSE plugin project;
 * PublicAPI.h includes this one.
 *
 * The DLL export itself takes a JSON string, which keeps the ABI stable as new
 * filter fields are added. This header gives you a typed struct and does the
 * serialization inline, so you write ordinary C++ and never touch the JSON:
 *
 * @code
 *   MemoryQuery q;
 *   q.excludeTags        = {"mymod_writeback"};
 *   q.excludeMemoryTypes = {"KNOWLEDGE"};
 *   q.minImportance      = 0.4f;
 *   q.orderBy            = MemoryOrder::ImportanceDesc;
 *   q.maxCount           = 10;
 *
 *   std::string json = QueryMemoriesForActor(formId, q);
 * @endcode
 *
 * No JSON library is required — the serializer below is self-contained.
 */

/**
 * Sort order for QueryMemoriesForActor.
 *
 * Relevance requires a non-empty contextQuery and ranks by semantic
 * similarity, equivalent to PublicGetMemoriesForActor's semantic mode — except
 * that the filters are applied before truncation, not after. With an empty
 * contextQuery it falls back to GameTimeDesc.
 *
 * Like that semantic mode, Relevance can only return memories that are present
 * in SkyrimNet's vector index: a memory whose embedding has not been generated
 * yet, or one past the internal candidate cap on a very large store, is not
 * returned. Use one of the other orders when completeness matters more than
 * ranking — those run entirely in SQL and see every eligible row.
 */
enum class MemoryOrder : uint32_t {
    GameTimeDesc = 0,  // newest first
    GameTimeAsc,
    ImportanceDesc,
    ImportanceAsc,
    IdDesc,
    IdAsc,
    Relevance,
};

/**
 * Filter and ordering for an actor's memories.
 *
 * Every include list is OR (a row matches if it has any listed value); every
 * exclude list drops a row that has any listed value. Exclude wins over
 * include where both would apply. An empty list is no constraint.
 *
 * A default-constructed MemoryQuery behaves close to
 * PublicGetMemoriesForActor(formId, 50, ""), so it is cheap to adopt
 * incrementally.
 */
struct MemoryQuery {
    int maxCount = 50;

    std::vector<std::string> includeTags;
    std::vector<std::string> excludeTags;
    bool matchAllIncludeTags = false;  // true = row must carry ALL of includeTags

    std::vector<std::string> includeMemoryTypes;  // "EXPERIENCE", "KNOWLEDGE", ...
    std::vector<std::string> excludeMemoryTypes;

    std::vector<std::string> includeEmotions;
    std::vector<std::string> excludeEmotions;

    std::vector<std::string> includeLocations;
    std::vector<std::string> excludeLocations;

    // Unset = unbounded on that side. Ranges are inclusive.
    std::optional<float> minImportance;
    std::optional<float> maxImportance;
    std::optional<double> gameTimeAfter;   // game-seconds, as reported in the "game_time" field
    std::optional<double> gameTimeBefore;

    // Unset = no constraint; true = active only; false = inactive only.
    std::optional<bool> isActive = true;

    std::string contextQuery;  // empty = non-semantic
    MemoryOrder orderBy = MemoryOrder::GameTimeDesc;
};

namespace SkyrimNetMemoryQueryDetail {

inline std::string EscapeJSON(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 2);
    for (unsigned char c : value) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    static const char* kHex = "0123456789abcdef";
                    out += "\\u00";
                    out += kHex[(c >> 4) & 0xF];
                    out += kHex[c & 0xF];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

inline void AppendStringArray(std::string& json, const char* key, const std::vector<std::string>& values) {
    if (values.empty()) return;
    json += ",\"";
    json += key;
    json += "\":[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) json += ",";
        json += "\"" + EscapeJSON(values[i]) + "\"";
    }
    json += "]";
}

// std::to_string formats floating point through the C locale, so a mod that
// called setlocale(LC_NUMERIC, ...) anywhere in the process would make this
// emit "0,4" instead of "0.4" — SkyrimNet would reject the payload as invalid
// JSON and the call would return "[]" from then on. std::to_chars always emits
// '.' regardless of locale, and round-trips exactly.
template <typename T>
inline std::string NumberToJSON(T value) {
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc{}) return "0";
    return std::string(buffer, result.ptr);
}

template <typename T>
inline void AppendOptionalNumber(std::string& json, const char* key, const std::optional<T>& value) {
    if (!value.has_value()) return;
    json += ",\"";
    json += key;
    json += "\":";
    json += NumberToJSON(*value);
}

inline const char* OrderName(MemoryOrder order) {
    switch (order) {
        case MemoryOrder::GameTimeAsc:    return "GameTimeAsc";
        case MemoryOrder::ImportanceDesc: return "ImportanceDesc";
        case MemoryOrder::ImportanceAsc:  return "ImportanceAsc";
        case MemoryOrder::IdDesc:         return "IdDesc";
        case MemoryOrder::IdAsc:          return "IdAsc";
        case MemoryOrder::Relevance:      return "Relevance";
        case MemoryOrder::GameTimeDesc:
        default:                          return "GameTimeDesc";
    }
}

} // namespace SkyrimNetMemoryQueryDetail

/**
 * Serialize a MemoryQuery to the JSON object the DLL export expects.
 *
 * Exposed so you can log or cache the payload; QueryMemoriesForActor calls it
 * for you. Fields left at their defaults are omitted — SkyrimNet applies the
 * same defaults on its side.
 */
inline std::string MemoryQueryToJSON(const MemoryQuery& q) {
    using namespace SkyrimNetMemoryQueryDetail;

    std::string json = "{\"maxCount\":" + NumberToJSON(q.maxCount);

    AppendStringArray(json, "includeTags", q.includeTags);
    AppendStringArray(json, "excludeTags", q.excludeTags);
    if (q.matchAllIncludeTags) json += ",\"matchAllIncludeTags\":true";

    AppendStringArray(json, "includeMemoryTypes", q.includeMemoryTypes);
    AppendStringArray(json, "excludeMemoryTypes", q.excludeMemoryTypes);
    AppendStringArray(json, "includeEmotions", q.includeEmotions);
    AppendStringArray(json, "excludeEmotions", q.excludeEmotions);
    AppendStringArray(json, "includeLocations", q.includeLocations);
    AppendStringArray(json, "excludeLocations", q.excludeLocations);

    AppendOptionalNumber(json, "minImportance", q.minImportance);
    AppendOptionalNumber(json, "maxImportance", q.maxImportance);
    AppendOptionalNumber(json, "gameTimeAfter", q.gameTimeAfter);
    AppendOptionalNumber(json, "gameTimeBefore", q.gameTimeBefore);

    // Explicit null distinguishes "no constraint" from the default of
    // active-only, so it is always emitted.
    json += ",\"isActive\":";
    json += q.isActive.has_value() ? (*q.isActive ? "true" : "false") : "null";

    if (!q.contextQuery.empty()) {
        json += ",\"contextQuery\":\"" + EscapeJSON(q.contextQuery) + "\"";
    }
    json += ",\"orderBy\":\"";
    json += OrderName(q.orderBy);
    json += "\"}";

    return json;
}
