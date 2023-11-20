#pragma once

#include "filesystem.hpp"

#include <filesystem>

namespace gfs
{
    class FileImporter
    {
    public:
        virtual ~FileImporter() = default;

        virtual bool Import(const std::filesystem::path& importFilename, const std::filesystem::path& outputDir) = 0;
        virtual bool Reimport(const Filesystem::File& file) = 0;
    };
}