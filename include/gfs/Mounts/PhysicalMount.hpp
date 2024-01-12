#pragma once

#include "Mount.hpp"

#include <FileWatch.hpp>

#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>

#undef DeleteFile
#undef MoveFile

namespace gfs
{
	class PhysicalMount : public Mount
	{
	public:
		explicit PhysicalMount(const std::string& alias, std::filesystem::path rootPath);
		~PhysicalMount() override = default;

		bool OnMount() override;
		void OnUnmount() override;

		void Flush() override;

		bool DeleteFile(const FileId& fileId) override;
		bool MoveFile(const FileId& fileId, const std::string& newFilename) override;

		bool HasFile(const FileId& fileId) const override;

		bool WriteStringToFile(const std::string& filename, const std::string& text) override;
		bool WriteMemoryToFile(const std::string& filename, const MemBuffer& memBuffer) override;

		bool ReadFileIntoString(const FileId& fileId, std::string& outString) override;
		bool ReadFileIntoMemory(const FileId& fileId, MemBuffer& outMemBuffer) override;

	private:
		struct File
		{
			FileId id;
			std::string filename;
		};
		void AddFile(const std::string& filename);
		void RemoveFile(const std::string& filename);
		auto GetFile(const FileId& fileId) const -> const File*;

		void DiscoverExistingFiles();
		void SetupFilewatch();

	private:
		std::filesystem::path m_rootPath;
		std::unique_ptr<filewatch::FileWatch<std::string>> m_filewatch;
		mutable std::mutex m_mutex;
		std::unordered_map<FileId, File> m_fileMap{};
	};

} // namespace gfs