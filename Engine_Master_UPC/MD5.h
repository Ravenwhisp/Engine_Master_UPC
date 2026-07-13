#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include "MD5Fwd.h"

std::string           to_hex_string(const std::vector<int8_t>& bytes);
std::vector<int8_t>   to_byte_vector(const std::string& text);
std::vector<int8_t>   computeMD5(const std::vector<int8_t>& message);


inline bool isValidAsset(const MD5Hash& id) { return !id.empty(); }


MD5Hash computeMD5(const std::filesystem::path& filePath);

uint64_t hashToUID(const MD5Hash& hash);
