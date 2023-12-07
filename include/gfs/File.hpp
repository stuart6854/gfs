#pragma once

// #include <cstdint>
// #include <string>
// #include <vector>

namespace gfs
{
	class Mount;

	/*struct FileHandle
	{
		Mount* mount = nullptr;
		uint64_t filenameHash;
	};*/

	/*using FileID = uint64_t;
	constexpr FileID InvalidFileId = 0;

	constexpr uint8_t FileFormatVersion = 0;

	struct File
	{
		FileID id;
		std::string name;
		std::string path;
		std::vector<FileID> references;
		std::string metadata; // Can be used to hold import settings etc.
		uint64_t uncompressedSize;
		uint64_t compressedSize;
		uint64_t offset;
	};*/

} // namespace gfs
