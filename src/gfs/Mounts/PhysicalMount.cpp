#include "gfs/Mounts/PhysicalMount.hpp"

#include <fstream>
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

		if (!CheckFileExists(filename))
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

		if (!CheckFileExists(filename))
			return false;

		if (CheckFileExists(newFilename))
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
		const auto newCwdRelativeFilename = m_rootPath / newFilename;
		rename(cwdRelativeFilename, newCwdRelativeFilename);

		return true;
	}

	bool PhysicalMount::CheckFileExists(const std::string& filename) const
	{
		if (std::filesystem::path(filename).empty())
			return false;

		const auto cwdRelativeFilename = m_rootPath / filename;
		return exists(cwdRelativeFilename) && is_regular_file(cwdRelativeFilename);
	}

} // namespace gfs