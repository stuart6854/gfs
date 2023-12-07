#pragma once

#include "Common.hpp"
#include "Mounts/Mount.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace gfs
{
	class Filesystem
	{
	public:
		////////////////////////////////////////
		/// Mounts
		////////////////////////////////////////

		template <typename T, typename... Args>
		auto AddMount(const std::string& alias, Args&&... args) -> Mount*;
		void RemoveMount(Mount& mount);

		auto GetMountByAlias(const std::string& alias) const -> Mount*;
		auto GetMountByFile(const std::string& filename) const -> Mount*;

		////////////////////////////////////////
		/// Files
		////////////////////////////////////////

		bool ReadFileIntoString(const std::string& filename, std::string& outString) const;
		bool ReadFileIntoMemory(const std::string& filename, MemBuffer& outMemBuffer) const;

	private:
		std::vector<std::unique_ptr<Mount>> m_mounts;
	};

	template <typename T, typename... Args>
	auto Filesystem::AddMount(const std::string& alias, Args&&... args) -> Mount*
	{
		static_assert(std::is_base_of_v<Mount, T>, "T parameter must be derive from Mount");

		auto ownedMount = std::make_unique<T>(alias, std::forward<Args>(args)...);
		if (!ownedMount->OnMount())
			return nullptr;

		auto* mountPtr = ownedMount.get();
		m_mounts.push_back(std::move(ownedMount));

		return mountPtr;
	}

} // namespace gfs