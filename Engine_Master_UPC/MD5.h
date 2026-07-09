#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include "MD5Fwd.h"

std::string           to_hex_string(const std::vector<int8_t>& bytes);
std::vector<int8_t>   to_byte_vector(const std::string& text);
std::vector<int8_t>   computeMD5(const std::vector<int8_t>& message);


inline bool isValidAsset(const MD5Hash& id) { return !id.empty(); }


inline MD5Hash computeMD5(const std::filesystem::path& filePath) 
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        return "";
    }

    std::vector<int8_t> contents(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return to_hex_string(computeMD5(contents));
}

inline uint64_t hashToUID(const MD5Hash& hash)
{
    if (hash.size() < 16)
    {
        return 0;
    }


    for (size_t i = 0; i < 16; ++i)
    {
        const char c = hash[i];
        const bool valid = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!valid)
        {
            return 0;
        }
    }

    return std::stoull(hash.substr(0, 16), nullptr, 16);
}
