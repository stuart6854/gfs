#include "gfs/Filesystem.hpp"

#include <bitsery/bitsery.h>
#include <lz4.h>
#include <nlohmann/json.hpp>

namespace gfs
{
	void Filesystem::RemoveMount(Mount& mount)
	{
		mount.OnUnmount();

		m_mounts.erase(std::remove_if(m_mounts.begin(), m_mounts.end(), [&](const auto& mountPtr) { return &mount == mountPtr.get(); }), m_mounts.end());
	}

	auto Filesystem::GetMountByAlias(const std::string& alias) const -> Mount*
	{
		const auto it = std::find_if(m_mounts.begin(), m_mounts.end(), [=](const auto& mountPtr) { return mountPtr->GetAlias() == alias; });
		if (it != m_mounts.end())
			return it->get();
		return nullptr;
	}

	auto Filesystem::GetMountByFile(const std::string& filename) const -> Mount*
	{
		const auto it = std::find_if(m_mounts.begin(), m_mounts.end(), [=](const auto& mountPtr) { return mountPtr->HasFile(filename); });
		if (it != m_mounts.end())
			return it->get();
		return nullptr;
	}

	bool Filesystem::ReadFileIntoString(const std::string& filename, std::string& outString) const
	{
		auto* mount = GetMountByFile(filename);
		if (mount == nullptr)
			return false;

		return mount->ReadFileIntoString(filename, outString);
	}

	bool Filesystem::ReadFileIntoMemory(const std::string& filename, MemBuffer& outMemBuffer) const
	{
		auto* mount = GetMountByFile(filename);
		if (mount == nullptr)
			return false;

		return mount->ReadFileIntoMemory(filename, outMemBuffer);
	}

} // namespace gfs
