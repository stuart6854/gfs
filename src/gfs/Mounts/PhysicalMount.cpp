#include "gfs/Mounts/PhysicalMount.hpp"

#include <fstream>
#include <iterator>
#include <utility>

namespace gfs
{
	PhysicalMount::PhysicalMount(const std::string& alias, std::filesystem::path rootPath) : Mount(alias), m_rootPath(std::move(rootPath)) {}

	bool PhysicalMount::OnMount()
	{
		if (exists(m_rootPath))
			return true;

		if (is_directory(m_rootPath))
			return true;

		std::error_code error{};
		if (!create_directories(m_rootPath, error))
			return false;

		DiscoverExistingFiles();
		SetupFilewatch();

		return true;
	}

	void PhysicalMount::OnUnmount()
	{
		m_filewatch = nullptr;

		Flush();
	}

	void PhysicalMount::Flush() {}

	/*bool PhysicalMount::CreateFile(const FileId& fileId)
	{
		if (!fileId)
			return false;

		const auto* file = GetFile(fileId);
		if (!file)
			return false;

		const auto cwdRelativeFilename = m_rootPath / file->filename;
		assert(exists(cwdRelativeFilename));

		std::error_code error{};
		create_directories(cwdRelativeFilename.parent_path(), error);
		if (!exists(cwdRelativeFilename.parent_path()))
			return false;

		std::ofstream stream(cwdRelativeFilename);
		if (stream.fail())
			return false;

		stream.close();
		return true;
	}*/

	bool PhysicalMount::DeleteFile(const StrId& fileId)
	{
		if (!fileId)
			return false;

		const auto* file = GetFile(fileId);
		if (!file)
			return false;

		const auto cwdRelativeFilename = m_rootPath / file->filename;
		std::error_code error{};
		if (!remove(cwdRelativeFilename, error))
			return false;

		RemoveFile(file->filename);

		return true;
	}

	bool PhysicalMount::MoveFile(const StrId& fileId, const std::string& newFilename)
	{
		if (!fileId)
			return false;
		if (newFilename.empty() || std::filesystem::path(newFilename).extension().empty())
			return false;

		const auto* file = GetFile(fileId);
		if (!file)
			return false;

		if (HasFile(newFilename))
			return false;

		const auto cwdRelativeFilename = m_rootPath / file->filename;
		const auto newCwdRelativeFilename = m_rootPath / newFilename;
		rename(cwdRelativeFilename, newCwdRelativeFilename);

		RemoveFile(file->filename);
		AddFile(newFilename);

		return true;
	}

	bool PhysicalMount::HasFile(const StrId& fileId) const
	{
		if (!fileId)
			return false;

		const auto* file = GetFile(fileId);
		return file != nullptr;
	}

	bool PhysicalMount::WriteStringToFile(const std::string& filename, const std::string& text)
	{
		if (filename.empty() || std::filesystem::path(filename).extension().empty())
			return false;

		StrId fileId(filename);
		const auto* file = GetFile(fileId);
		if (file)
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;

		std::error_code error{};
		create_directories(cwdRelativeFilename.parent_path(), error);
		if (!exists(cwdRelativeFilename.parent_path()))
			return false;

		std::ofstream stream(cwdRelativeFilename, std::ios::out);
		if (stream.fail())
			return false;

		stream.write(text.data(), static_cast<std::streamsize>(text.size()));
		if (stream.fail())
			return false;

		AddFile(filename);

		return true;
	}

	bool PhysicalMount::WriteMemoryToFile(const std::string& filename, const MemBuffer& memBuffer)
	{
		if (filename.empty() || std::filesystem::path(filename).extension().empty())
			return false;

		StrId fileId(filename);
		const auto* file = GetFile(fileId);
		if (file)
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;

		std::error_code error{};
		create_directories(cwdRelativeFilename.parent_path(), error);
		if (!exists(cwdRelativeFilename.parent_path()))
			return false;

		std::ofstream stream(cwdRelativeFilename, std::ios::out | std::ios::binary);
		if (stream.fail())
			return false;

		stream.write(reinterpret_cast<const char*>(memBuffer.data()), static_cast<std::streamsize>(memBuffer.size()));

		if (stream.fail())
			return false;

		AddFile(filename);

		return true;
	}

	bool PhysicalMount::ReadFileIntoString(const StrId& fileId, std::string& outString)
	{
		if (!HasFile(fileId))
			return false;

		const auto* file = GetFile(fileId);
		if (!file)
			return false;

		const auto cwdRelativeFilename = m_rootPath / file->filename;
		std::ifstream stream(cwdRelativeFilename, std::ios::in | std::ios::binary | std::ios::ate);
		if (stream.fail())
			return false;

		stream.unsetf(std::ios::skipws);

		const size_t fileSize = stream.tellg();
		stream.seekg(0, std::ios::beg);

		outString.reserve(fileSize);
		outString.insert(outString.begin(), std::istream_iterator<char>(stream), std::istream_iterator<char>());

		return outString.size() == fileSize;
	}

	bool PhysicalMount::ReadFileIntoMemory(const StrId& fileId, MemBuffer& outMemBuffer)
	{
		if (!HasFile(fileId))
			return false;

		const auto* file = GetFile(fileId);
		if (!file)
			return false;

		const auto cwdRelativeFilename = m_rootPath / file->filename;
		std::ifstream stream(cwdRelativeFilename, std::ios::in | std::ios::binary | std::ios::ate);
		if (stream.fail())
			return false;

		stream.unsetf(std::ios::skipws);

		const size_t fileSize = stream.tellg();
		stream.seekg(0, std::ios::beg);

		outMemBuffer.reserve(fileSize);
		outMemBuffer.insert(outMemBuffer.begin(), std::istream_iterator<uint8_t>(stream), std::istream_iterator<uint8_t>());

		return outMemBuffer.size() == fileSize;
	}

	void PhysicalMount::AddFile(const std::string& filename)
	{
		const std::lock_guard lock(m_mutex);
		const File file = { StrId(filename), filename };
		m_fileMap[file.id] = file;
	}

	void PhysicalMount::RemoveFile(const std::string& filename)
	{
		const std::lock_guard lock(m_mutex);
		const StrId fileId(filename);
		m_fileMap.erase(fileId);
	}

	auto PhysicalMount::GetFile(const StrId& fileId) const -> const File*
	{
		const std::lock_guard lock(m_mutex);
		const auto it = m_fileMap.find(fileId);
		if (it == m_fileMap.end())
			return nullptr;
		return &it->second;
	}

	void PhysicalMount::DiscoverExistingFiles()
	{
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(m_rootPath))
		{
			if (!dirEntry.is_regular_file())
				continue;

			const auto& path = dirEntry.path();
			const auto fileId = StrId(path.string());
			m_fileMap[fileId] = File{ fileId, path.string() };
		}
	}

	void PhysicalMount::SetupFilewatch()
	{
		m_filewatch = std::make_unique<filewatch::FileWatch<std::string>>(m_rootPath.string(), [this](const std::string& file, const filewatch::Event event) {
			switch (event)
			{
				case filewatch::Event::added:
					AddFile(file);
					break;
				case filewatch::Event::removed:
					RemoveFile(file);
					break;
				case filewatch::Event::modified:
					break;
				case filewatch::Event::renamed_old:
					RemoveFile(file);
					break;
				case filewatch::Event::renamed_new:
					AddFile(file);
					break;
			}
		});
	}

} // namespace gfs