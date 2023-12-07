#pragma once

#include "Mount.hpp"

#include <FileWatch.hpp>

#include <filesystem>
#include <mutex>
#include <unordered_map>

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

		bool DeleteFile(const StrId& fileId) override;
		bool MoveFile(const StrId& fileId, const std::string& newFilename) override;

		bool HasFile(const StrId& fileId) const override;

		bool WriteStringToFile(const std::string& filename, const std::string& text) override;
		bool WriteMemoryToFile(const std::string& filename, const MemBuffer& memBuffer) override;

		bool ReadFileIntoString(const StrId& fileId, std::string& outString) override;
		bool ReadFileIntoMemory(const StrId& fileId, MemBuffer& outMemBuffer) override;

	private:
		struct File
		{
			StrId id;
			std::string filename;
		};
		void AddFile(const std::string& filename);
		void RemoveFile(const std::string& filename);
		auto GetFile(const StrId& fileId) const -> const File*;

		void DiscoverExistingFiles();
		void SetupFilewatch();

	private:
		std::filesystem::path m_rootPath;
		std::unique_ptr<filewatch::FileWatch<std::string>> m_filewatch;
		mutable std::mutex m_mutex;
		std::unordered_map<StrId, File> m_fileMap{};
	};

} // namespace gfs