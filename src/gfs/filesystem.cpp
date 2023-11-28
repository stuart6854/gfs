#include "gfs/filesystem.hpp"
#include "gfs/binary_streams.hpp"
#include "gfs/file_importer.hpp"

#include <filesystem>
#include <functional>
#include <lz4.h>

#include <cstdint>
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>

namespace gfs
{
	constexpr char FS_FORMAT_MAGIC_NUM[4] = { 'g', 'f', 's', 'f' }; // GFS Format
	constexpr auto FS_FORMAT_VERSION = 1;

	constexpr uint32_t FS_FORMAT_PATH_LENGTH = 255;

	auto Filesystem::MountDir(const std::filesystem::path& rootDir, bool allowUnmount) -> MountID
	{
		if (!std::filesystem::exists(rootDir))
			return InvalidMountId;

		if (!std::filesystem::is_directory(rootDir))
			return InvalidMountId;

		auto& mount = m_mountMap[m_nextMountId];
		mount.RootDirPath = rootDir;
		mount.AllowUnmount = allowUnmount;
		mount.Id = m_nextMountId++;
		assert(mount.Id != InvalidMountId);

		GatherFilesInMount(mount);

		return mount.Id;
	}

	bool Filesystem::UnmountDir(MountID id)
	{
		const auto it = m_mountMap.find(id);
		if (it == m_mountMap.end())
			return false;

		if (!it->second.AllowUnmount)
			return false;

		m_mountMap.erase(it);
		return true;
	}

	auto Filesystem::GetMountId(const std::filesystem::path& rootDir) -> MountID
	{
		const auto rootDirAbs = std::filesystem::absolute(rootDir);
		for (const auto& [id, mount] : m_mountMap)
		{
			const auto mountDirAbs = std::filesystem::absolute(mount.RootDirPath);
			if (rootDirAbs == mountDirAbs)
				return mount.Id;
		}
		return InvalidMountId;
	}

	void Filesystem::ForEachMount(const std::function<void(const Mount&)>& func)
	{
		for (const auto& [id, mount] : m_mountMap)
			func(mount);
	}

	auto Filesystem::GetFile(FileID id) const -> const File*
	{
		const auto it = m_files.find(id);
		if (it == m_files.end())
			return nullptr;

		return &it->second;
	}

	void Filesystem::ForEachFile(const std::function<void(const File& file)>& func)
	{
		for (const auto& [id, file] : m_files)
			func(file);
	}

	bool Filesystem::WriteFile(MountID mountId,
		const std::filesystem::path& filename,
		FileID fileId,
		const std::vector<FileID>& fileDependencies,
		const BinaryStreamable& dataObject,
		bool compress,
		const std::filesystem::path& sourceFilename)
	{
		auto* mount = GetMount(mountId);
		if (!mount)
			return false;

		WriteOnlyByteBuffer uncompressedDataBuffer;
		dataObject.Write(uncompressedDataBuffer);

		bool shouldCompress = compress && uncompressedDataBuffer.GetSize() >= FS_COMPRESS_MIN_FILE_SIZE_BYTES;
		WriteOnlyByteBuffer compressedDataBuffer;
		if (shouldCompress)
		{
			compressedDataBuffer.SetSize(uncompressedDataBuffer.GetSize());
			const uint32_t compressedSize = LZ4_compress_default(reinterpret_cast<const char*>(uncompressedDataBuffer.GetData()),
				reinterpret_cast<char*>(compressedDataBuffer.GetData()),
				int32_t(uncompressedDataBuffer.GetSize()),
				int32_t(compressedDataBuffer.GetSize()));
			compressedDataBuffer.SetSize(compressedSize);
		}
		else
		{
			compressedDataBuffer.SetSize(uncompressedDataBuffer.GetSize());
			compressedDataBuffer.Write(uncompressedDataBuffer.GetSize(), uncompressedDataBuffer.GetData());
		}

		FormatHeader header{};
		std::memcpy(header.MagicNumber, FS_FORMAT_MAGIC_NUM, sizeof(FS_FORMAT_MAGIC_NUM));
		header.FormatVersion = FS_FORMAT_VERSION;
		header.FileCount = 1;

		File file{};
		file.FileId = fileId;
		file.MountId = mountId;
		file.MountRelPath = filename;
		file.SourceFilename = sourceFilename;
		file.FileDependencies = fileDependencies;
		file.UncompressedSize = uint32_t(uncompressedDataBuffer.GetSize());
		file.CompressedSize = uint32_t(compressedDataBuffer.GetSize());

		std::ofstream stream(mount->RootDirPath / filename, std::ios::binary);
		if (!stream)
			return false;

		stream.unsetf(std::ios::skipws);

		stream << header;
		stream << file;
		file.Offset = uint32_t(stream.tellp());
		const uint32_t offsetPos = file.Offset - sizeof(file.Offset);

		stream.write(reinterpret_cast<const char*>(compressedDataBuffer.GetData()), compressedDataBuffer.GetSize());

		// Go back and write data offset
		stream.seekp(offsetPos, std::ios::beg);
		stream.write(reinterpret_cast<const char*>(&file.Offset), sizeof(file.Offset));

		m_files[file.FileId] = file; // Register new file.

		return true;
	}

	bool Filesystem::ReadFile(FileID fileId, BinaryStreamable& dataObject)
	{
		auto* file = GetFile(fileId);
		if (!file)
			return false;

		auto* mount = GetMount(file->MountId);
		if (!mount)
			return false;

		std::ifstream stream(mount->RootDirPath / file->MountRelPath, std::ios::binary);
		stream.unsetf(std::ios::skipws);
		stream.seekg(file->Offset);

		const bool isCompressed = file->CompressedSize != file->UncompressedSize;

		ReadOnlyByteBuffer decompressedBuffer(file->UncompressedSize);
		ReadOnlyByteBuffer compressedBuffer(isCompressed ? file->CompressedSize : 0);

		stream.read(reinterpret_cast<char*>(isCompressed ? compressedBuffer.GetData() : decompressedBuffer.GetData()), file->CompressedSize);
		if (isCompressed)
		{
			const auto* srcPtr = reinterpret_cast<const char*>(compressedBuffer.GetData());
			auto* dstPtr = reinterpret_cast<char*>(decompressedBuffer.GetData());
			int bytes = LZ4_decompress_safe(srcPtr, dstPtr, int32_t(compressedBuffer.GetSize()), int32_t(decompressedBuffer.GetSize()));

			if (uint32_t(bytes) != file->UncompressedSize)
				return false; // Did not decompress to original size.
		}

		dataObject.Read(decompressedBuffer);

		return true;
	}

	bool Filesystem::CreateArchive(MountID mountId, const std::filesystem::path& filename, const std::vector<FileID>& files)
	{
		auto* mount = GetMount(mountId);
		if (!mount)
			return false;

		// Calculate total data size & data offsets
		uint64_t totalDataSize = 0;
		std::vector<uint64_t> fileDataOffsets(files.size());
		for (auto i = 0; i < files.size(); ++i)
		{
			const auto fileId = files[i];
			const auto* file = GetFile(fileId);
			if (!file)
				return false;

			fileDataOffsets[i] = totalDataSize;
			totalDataSize += file->CompressedSize;
		}

		// Gather file data
		ReadOnlyByteBuffer dataBuffer(totalDataSize);
		for (auto i = 0; i < files.size(); ++i)
		{
			const auto fileId = files[i];
			const auto* file = GetFile(fileId);
			if (!file)
				return false;

			auto* dataWriteOffset = static_cast<uint8_t*>(dataBuffer.GetData()) + fileDataOffsets[i];

			std::ifstream stream(mount->RootDirPath / file->MountRelPath, std::ios::binary);
			stream.unsetf(std::ios::skipws);
			stream.seekg(file->Offset);
			stream.read(reinterpret_cast<char*>(dataWriteOffset), file->CompressedSize);
		}

		FormatHeader header{};
		std::memcpy(header.MagicNumber, FS_FORMAT_MAGIC_NUM, sizeof(FS_FORMAT_MAGIC_NUM));
		header.FormatVersion = FS_FORMAT_VERSION;
		header.FileCount = files.size();

		std::ofstream stream(mount->RootDirPath / filename, std::ios::binary);
		if (!stream)
			return false;

		stream.unsetf(std::ios::skipws);

		stream << header;

		std::vector<uint64_t> fileOffsetWritePositions(files.size());
		for (auto i = 0; i < files.size(); ++i)
		{
			const auto fileId = files[i];
			auto* file = GetFile(fileId);
			if (!file)
				return false;

			file->MountId = mountId;	   // Update mount id.
			file->MountRelPath = filename; // Update filename to archive file.

			stream << *file;
			fileOffsetWritePositions[i] = uint32_t(stream.tellp()) - sizeof(file->Offset);
		}
		const uint32_t dataStartOffset = stream.tellp();

		stream.write(reinterpret_cast<const char*>(dataBuffer.GetData()), dataBuffer.GetSize());

		for (auto i = 0; i < files.size(); ++i)
		{
			const auto fileId = files[i];
			auto* file = GetFile(fileId);

			const auto fileOffsetWritePos = fileOffsetWritePositions[i];
			file->Offset = dataStartOffset + fileDataOffsets[i];

			// Go back and write data offset
			stream.seekp(fileOffsetWritePos, std::ios::beg);
			stream.write(reinterpret_cast<const char*>(&file->Offset), sizeof(file->Offset));
		}

		return true;
	}

	void Filesystem::SetImporter(const std::vector<std::string>& fileExts, const std::shared_ptr<FileImporter>& importer)
	{
		for (const auto& ext : fileExts)
		{
			const auto extHash = std::hash<std::string>{}(ext);
			m_extImporterMap[extHash] = importer;
		}
	}

	auto Filesystem::GetImporter(const std::string& fileExt) -> std::shared_ptr<FileImporter>
	{
		const auto extHash = std::hash<std::string>{}(fileExt);
		const auto it = m_extImporterMap.find(extHash);
		if (it == m_extImporterMap.end())
			return nullptr;

		return it->second;
	}

	bool Filesystem::Import(const std::filesystem::path& filename, MountID outputMount, const std::filesystem::path& outputDir)
	{
		if (filename.empty() || !std::filesystem::exists(filename) || !std::filesystem::is_regular_file(filename))
			return false;

		const auto fileExt = filename.extension().string();
		auto importer = GetImporter(fileExt);
		if (!importer)
			return false;

		return importer->Import(*this, filename, outputMount, outputDir);
	}

	bool Filesystem::Reimport(FileID fileId)
	{
		auto* file = GetFile(fileId);
		if (!file)
			return false;

		if (file->SourceFilename.empty() || !std::filesystem::exists(file->SourceFilename) || !std::filesystem::is_regular_file(file->SourceFilename))
			return false;

		const auto fileExt = file->SourceFilename.extension().string();
		auto importer = GetImporter(fileExt);
		if (!importer)
			return false;

		return importer->Reimport(*this, *file);
	}

	bool Filesystem::IsPathInMount(const std::filesystem::path& path, MountID mountId)
	{
		auto* mount = GetMount(mountId);
		if (!mount)
			return false;

		std::filesystem::path finalPath;
		try
		{
			finalPath = std::filesystem::canonical(mount->RootDirPath / path);
		}
		catch (const std::exception& /*ex*/)
		{
			return false; // Path/File does not exists.
		}

		auto [rootEnd, nothing] = std::mismatch(mount->RootDirPath.begin(), mount->RootDirPath.end(), finalPath.begin());
		if (rootEnd == mount->RootDirPath.end())
			return false;

		return true;
	}

	bool Filesystem::IsPathInAnyMount(const std::filesystem::path& path)
	{
		for (const auto& [id, mount] : m_mountMap)
		{
			if (IsPathInMount(path, mount.Id))
				return true;
		}
		return false;
	}

	auto Filesystem::GetMount(MountID id) -> Mount*
	{
		const auto it = m_mountMap.find(id);
		if (it == m_mountMap.end())
			return nullptr;

		return &it->second;
	}

	auto Filesystem::GetFile(FileID id) -> File*
	{
		const auto it = m_files.find(id);
		if (it == m_files.end())
			return nullptr;

		return &it->second;
	}

	auto Filesystem::GetMountPathIsIn(const std::filesystem::path& path) -> MountID
	{
		for (auto& [id, mount] : m_mountMap)
		{
			if (IsPathInMount(path, id))
				return id;
		}
		return InvalidMountId;
	}

	void Filesystem::GatherFilesInMount(const Mount& mount)
	{
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(mount.RootDirPath))
		{
			if (!dirEntry.is_regular_file())
				continue;

			const auto filePath = dirEntry.path();

			const auto fileSize = std::filesystem::file_size(filePath);
			if (fileSize < sizeof(FormatHeader))
				continue;

			ValidateAndRegisterFile(filePath, mount.Id);
		}
	}

	void Filesystem::ValidateAndRegisterFile(const std::filesystem::path& filename, MountID mountId)
	{
		std::ifstream stream(filename, std::ios::binary);
		if (!stream)
			return;

		FormatHeader header{};
		stream >> header;

		if (std::memcmp(header.MagicNumber, FS_FORMAT_MAGIC_NUM, sizeof(header.MagicNumber)) != 0)
			return;

		File file{};
		stream >> file;

		file.MountId = mountId;
		file.MountRelPath = filename;
		m_files[file.FileId] = file;
	}

	auto operator<<(std::ostream& stream, const FormatHeader& header) -> std::ostream&
	{
		stream.write(reinterpret_cast<const char*>(&header.MagicNumber), sizeof(header.MagicNumber));
		stream.write(reinterpret_cast<const char*>(&header.FormatVersion), sizeof(header.FormatVersion));
		stream.write(reinterpret_cast<const char*>(&header.FileCount), sizeof(header.FileCount));
		return stream;
	}

	auto operator>>(std::istream& stream, FormatHeader& header) -> std::istream&
	{
		stream.read(reinterpret_cast<char*>(&header.MagicNumber), sizeof(header.MagicNumber));
		stream.read(reinterpret_cast<char*>(&header.FormatVersion), sizeof(header.FormatVersion));
		stream.read(reinterpret_cast<char*>(&header.FileCount), sizeof(header.FileCount));
		return stream;
	}

	auto operator<<(std::ostream& stream, const Filesystem::File& file) -> std::ostream&
	{
		stream.write(reinterpret_cast<const char*>(&file.FileId), sizeof(file.FileId));

		// Mount relative path
		uint16_t strLen = file.MountRelPath.string().size();
		stream.write(reinterpret_cast<const char*>(&strLen), sizeof(strLen));
		auto pathStr = file.MountRelPath.string();
		stream.write(reinterpret_cast<const char*>(pathStr.data()), strLen);

		// Source filename
		strLen = file.SourceFilename.string().size();
		stream.write(reinterpret_cast<const char*>(&strLen), sizeof(strLen));
		pathStr = file.SourceFilename.string();
		stream.write(reinterpret_cast<const char*>(pathStr.data()), strLen);

		// File dependencies
		const auto count = uint16_t(file.FileDependencies.size());
		stream.write(reinterpret_cast<const char*>(&count), sizeof(count));
		if (!file.FileDependencies.empty())
			stream.write(reinterpret_cast<const char*>(file.FileDependencies.data()), sizeof(file.FileDependencies[0]) * count);

		stream.write(reinterpret_cast<const char*>(&file.UncompressedSize), sizeof(file.UncompressedSize));
		stream.write(reinterpret_cast<const char*>(&file.CompressedSize), sizeof(file.CompressedSize));
		stream.write(reinterpret_cast<const char*>(&file.Offset), sizeof(file.Offset));
		return stream;
	}

	auto operator>>(std::istream& stream, Filesystem::File& file) -> std::istream&
	{
		stream.read(reinterpret_cast<char*>(&file.FileId), sizeof(file.FileId));

		// Mount relative path
		uint16_t strLen = 0;
		stream.read(reinterpret_cast<char*>(&strLen), sizeof(strLen));
		std::string pathStr(strLen, 0);
		stream.read(reinterpret_cast<char*>(pathStr.data()), strLen);
		file.MountRelPath = pathStr;

		// Source filename
		strLen = 0;
		stream.read(reinterpret_cast<char*>(&strLen), sizeof(strLen));
		pathStr = std::string(strLen, 0);
		stream.read(reinterpret_cast<char*>(pathStr.data()), strLen);
		file.SourceFilename = pathStr;

		// File dependencies
		uint16_t count = 0;
		stream.read(reinterpret_cast<char*>(&count), sizeof(count));
		file.FileDependencies.resize(count);
		if (!file.FileDependencies.empty())
			stream.read(reinterpret_cast<char*>(file.FileDependencies.data()), sizeof(file.FileDependencies[0]) * count);

		stream.read(reinterpret_cast<char*>(&file.UncompressedSize), sizeof(file.UncompressedSize));
		stream.read(reinterpret_cast<char*>(&file.CompressedSize), sizeof(file.CompressedSize));
		stream.read(reinterpret_cast<char*>(&file.Offset), sizeof(file.Offset));
		return stream;
	}

} // namespace gfs
