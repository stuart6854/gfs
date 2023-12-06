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

} // namespace gfs
