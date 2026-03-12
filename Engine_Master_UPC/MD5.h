#pragma once
#include <string>
#include <filesystem>
#include <fstream>

using MD5Hash = std::string;

inline constexpr const char* INVALID_ASSET_ID = "";

std::string           to_hex_string(const std::vector<int8_t>& bytes);
std::vector<int8_t>   to_byte_vector(const std::string& text);
std::vector<int8_t>   computeMD5(const std::vector<int8_t>& message);

inline bool isValidAsset(const MD5Hash& id) { return !id.empty(); }

MD5Hash computeMD5(const std::string& filePath) {
	return to_hex_string(computeMD5(to_byte_vector(filePath)));
}

MD5Hash computeMD5(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        DEBUG_ERROR("[MD5] Could not open file for hashing: %s", filePath.string().c_str());
        return "";
    }

    std::vector<int8_t> contents(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return to_hex_string(computeMD5(contents));
}
