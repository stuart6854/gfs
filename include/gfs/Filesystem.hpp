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

		/**
		 * \brief Get the first mount that has the file.
		 * \param fileId
		 * \return
		 */
		auto GetMountByFile(const StrId& fileId) const -> Mount*;

		////////////////////////////////////////
		/// Files
		////////////////////////////////////////

		bool ReadFileIntoString(std::string& outString, const StrId& fileId) const;
		bool ReadFileIntoMemory(MemBuffer& outMemBuffer, const StrId& fileId) const;

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