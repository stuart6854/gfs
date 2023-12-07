#pragma once

#include "Mount.hpp"

#include <filesystem>

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

		bool CreateFile(const std::string& filename) override;
		bool DeleteFile(const std::string& filename) override;
		bool MoveFile(const std::string& filename, const std::string& newFilename) override;

		bool HasFile(const std::string& filename) const override;

		bool WriteStringToFile(const std::string& filename, const std::string& text) override;
		bool WriteMemoryToFile(const std::string& filename, const MemBuffer& memBuffer) override;

		bool ReadFileIntoString(const std::string& filename, std::string& outString) override;
		bool ReadFileIntoMemory(const std::string& filename, MemBuffer& outMemBuffer) override;

	private:
		std::filesystem::path m_rootPath;
	};

} // namespace gfs