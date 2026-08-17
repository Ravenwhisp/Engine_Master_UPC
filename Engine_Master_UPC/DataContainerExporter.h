#pragma once

#include <filesystem>

class DataContainerExporter
{
public:
    // Scans ASSETS_FOLDER for all .datacontainer files and writes them as a single
    // JSON object keyed by filename stem (e.g. "MyConfig" -> { ... }).
    static bool exportToJson(const std::filesystem::path& outputPath);

    // Reads a combined JSON file and writes each entry back as an individual
    // .datacontainer file in ASSETS_FOLDER. Existing files are overwritten.
    static bool importFromJson(const std::filesystem::path& inputPath);
};
