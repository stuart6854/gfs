#pragma once

#include "filesystem.hpp"

#include <filesystem>

namespace gfs
{
	class FileImporter
	{
	public:
		virtual ~FileImporter() = default;

		virtual bool Import(Filesystem& fs,
			const std::filesystem::path& importFilename,
			MountID outputMount,
			const std::filesystem::path& outputDir,
			const std::string& metadata) = 0;
		virtual bool Reimport(Filesystem& fs, const Filesystem::File& file) = 0;
	};

} // namespace gfs