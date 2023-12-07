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
		return create_directories(m_rootPath, error);
	}

	void PhysicalMount::OnUnmount()
	{
		Flush();
	}

	void PhysicalMount::Flush() {}

	bool PhysicalMount::CreateFile(const std::string& filename)
	{
		if (std::filesystem::path(filename).empty())
			return false;
		if (std::filesystem::path(filename).extension().empty())
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
		if (exists(cwdRelativeFilename))
			return true;

		if (is_directory(cwdRelativeFilename))
			return false;

		std::error_code error{};
		create_directories(cwdRelativeFilename.parent_path(), error);
		if (!exists(cwdRelativeFilename.parent_path()))
			return false;

		std::ofstream stream(cwdRelativeFilename);
		if (stream.fail())
			return false;

		stream.close();
		return true;
	}

	bool PhysicalMount::DeleteFile(const std::string& filename)
	{
		if (std::filesystem::path(filename).empty())
			return false;
		if (std::filesystem::path(filename).extension().empty())
			return false;

		if (!HasFile(filename))
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
		std::error_code error{};
		return remove(cwdRelativeFilename, error);
	}

	bool PhysicalMount::MoveFile(const std::string& filename, const std::string& newFilename)
	{
		if (std::filesystem::path(filename).empty() || std::filesystem::path(newFilename).empty())
			return false;
		if (std::filesystem::path(filename).extension().empty() || std::filesystem::path(newFilename).extension().empty())
			return false;

		if (!HasFile(filename))
			return false;

		if (HasFile(newFilename))
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
		const auto newCwdRelativeFilename = m_rootPath / newFilename;
		rename(cwdRelativeFilename, newCwdRelativeFilename);

		return true;
	}

	bool PhysicalMount::HasFile(const std::string& filename) const
	{
		if (std::filesystem::path(filename).empty())
			return false;
		if (std::filesystem::path(filename).extension().empty())
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
		return exists(cwdRelativeFilename) && is_regular_file(cwdRelativeFilename);
	}

	bool PhysicalMount::WriteStringToFile(const std::string& filename, const std::string& text)
	{
		if (std::filesystem::path(filename).empty())
			return false;
		if (std::filesystem::path(filename).extension().empty())
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
		std::ofstream stream(cwdRelativeFilename, std::ios::out);
		if (stream.fail())
			return false;

		stream.write(text.data(), static_cast<std::streamsize>(text.size()));
		return !stream.fail();
	}

	bool PhysicalMount::WriteMemoryToFile(const std::string& filename, const MemBuffer& memBuffer)
	{
		if (std::filesystem::path(filename).empty())
			return false;
		if (std::filesystem::path(filename).extension().empty())
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
		std::ofstream stream(cwdRelativeFilename, std::ios::out | std::ios::binary);
		if (stream.fail())
			return false;

		stream.write(reinterpret_cast<const char*>(memBuffer.data()), static_cast<std::streamsize>(memBuffer.size()));

		return !stream.fail();
	}

	bool PhysicalMount::ReadFileIntoString(const std::string& filename, std::string& outString)
	{
		if (!HasFile(filename))
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
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

	bool PhysicalMount::ReadFileIntoMemory(const std::string& filename, MemBuffer& outMemBuffer)
	{
		if (!HasFile(filename))
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
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

} // namespace gfs