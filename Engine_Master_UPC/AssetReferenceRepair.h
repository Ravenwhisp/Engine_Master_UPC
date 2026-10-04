#pragma once

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

// Works on the source document, without constructing components or scripts.
namespace AssetReferenceRepair
{
    inline std::string escapePath(const std::string& key)
    {
        std::string escaped;
        for (char c : key)
            escaped += c == '~' ? "~0" : c == '/' ? "~1" : std::string(1, c);
        return escaped;
    }

    template<typename Value, typename Visitor>
    void visitReferences(Value& value, const Visitor& visitor, const std::string& location = "")
    {
        if (value.IsObject())
        {
            auto uid = value.FindMember("uid");
            auto hash = value.FindMember("libId");
            auto type = value.FindMember("type");
            if (uid != value.MemberEnd() && uid->value.IsUint64() && uid->value.GetUint64() != 0 &&
                hash != value.MemberEnd() && hash->value.IsString() &&
                type != value.MemberEnd() && type->value.IsString())
                visitor(value, uid->value.GetUint64(), location.empty() ? "/" : location);
            for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
                visitReferences(it->value, visitor, location + "/" +
                    escapePath(std::string(it->name.GetString(), it->name.GetStringLength())));
        }
        else if (value.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
                visitReferences(value[i], visitor, location + "/" + std::to_string(i));
        }
    }

    struct RepairCounts
    {
        size_t repaired = 0;
        size_t unresolved = 0;
    };

    template<typename Lookup, typename ReportUnresolved>
    RepairCounts repairReferences(rapidjson::Document& document, const Lookup& lookup,
                                  const ReportUnresolved& reportUnresolved)
    {
        RepairCounts counts;
        visitReferences(document, [&](rapidjson::Value& reference, uint64_t uid, const std::string& location)
        {
            const std::string* currentHash = lookup(uid);
            if (!currentHash || currentHash->empty())
            {
                ++counts.unresolved;
                reportUnresolved(uid, location);
                return;
            }
            auto& hash = reference["libId"];
            if (*currentHash == std::string(hash.GetString(), hash.GetStringLength())) return;
            hash.SetString(currentHash->c_str(), static_cast<rapidjson::SizeType>(currentHash->size()), document.GetAllocator());
            ++counts.repaired;
        });
        return counts;
    }

    struct DependencyOrder
    {
        std::vector<uint64_t> ordered;
        // Includes cycles and everything that depends on a cycle.
        std::vector<uint64_t> blocked;
    };

    inline DependencyOrder orderDependencies(const std::map<uint64_t, std::set<uint64_t>>& graph)
    {
        std::map<uint64_t, size_t> remaining;
        std::map<uint64_t, std::vector<uint64_t>> dependents;
        std::set<uint64_t> ready;
        for (const auto& [uid, dependencies] : graph)
        {
            remaining[uid] = 0;
            for (uint64_t dependency : dependencies)
            {
                if (graph.find(dependency) == graph.end()) continue;
                ++remaining[uid];
                dependents[dependency].push_back(uid);
            }
            if (remaining[uid] == 0) ready.insert(uid);
        }
        DependencyOrder result;
        while (!ready.empty())
        {
            const uint64_t uid = *ready.begin();
            ready.erase(ready.begin());
            result.ordered.push_back(uid);
            for (uint64_t dependent : dependents[uid])
                if (--remaining[dependent] == 0) ready.insert(dependent);
        }
        for (const auto& [uid, count] : remaining)
            if (count != 0) result.blocked.push_back(uid);
        return result;
    }

    inline bool writeAtomically(const std::filesystem::path& path,
        const char* data, size_t size, std::string& error)
    {
        if (!data || size == 0)
        {
            error = "Empty serialized data";
            return false;
        }
        std::filesystem::path temporary = path;
        temporary += ".references.tmp";
        std::error_code ec;
        if (std::filesystem::exists(temporary, ec) || ec)
        {
            error = "Temporary repair file already exists or cannot be inspected";
            return false;
        }
        {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            file.write(data, static_cast<std::streamsize>(size));
            file.close();
            if (!file)
            {
                error = "Could not write temporary repair file";
                std::filesystem::remove(temporary, ec);
                return false;
            }
        }
#ifdef _WIN32
        // std::filesystem::rename cannot replace an existing file on Windows.
        const bool replaced = MoveFileExW(temporary.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
        if (!replaced) error = "Could not replace source file (Windows error " + std::to_string(GetLastError()) + ")";
#else
        std::filesystem::rename(temporary, path, ec);
        const bool replaced = !ec;
        if (!replaced) error = ec.message();
#endif
        if (!replaced) std::filesystem::remove(temporary, ec);
        return replaced;
    }

    inline bool saveAtomically(const rapidjson::Document& document,
        const std::filesystem::path& path, std::string& error)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        if (!document.Accept(writer))
        {
            error = "JSON serialization failed";
            return false;
        }
        return writeAtomically(path, buffer.GetString(), buffer.GetSize(), error);
    }
}
