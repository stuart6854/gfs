#pragma once

#include "gfs/Common.hpp"

#include <string>
#include <utility>

#undef DeleteFile
#undef MoveFile

namespace gfs
{
	class Mount
	{
	public:
		explicit Mount(std::string alias) : m_alias(std::move(alias)), m_aliasHash(std::hash<std::string>{}(alias)) {}
		virtual ~Mount() = default;

		////////////////////////////////////////
		/// Mount
		////////////////////////////////////////

		virtual bool OnMount() = 0;
		virtual void OnUnmount() = 0;

		virtual void Flush() = 0;

		////////////////////////////////////////
		/// Files
		////////////////////////////////////////

		// virtual bool CreateFile(const FileId& fileId) = 0;
		virtual bool DeleteFile(const FileId& fileId) = 0;

		/**
		 * \brief Used to both move and rename files.
		 * \param filename The file to target.
		 * \param newFilename The filename to move the target to.
		 * \return TRUE if successful.
		 */
		virtual bool MoveFile(const FileId& fileId, const std::string& newFilename) = 0;

		virtual bool HasFile(const FileId& fileId) const = 0;

		virtual bool WriteStringToFile(const std::string& filename, const std::string& text) = 0;
		virtual bool WriteMemoryToFile(const std::string& filename, const MemBuffer& memBuffer) = 0;

		virtual bool ReadFileIntoString(const FileId& fileId, std::string& outString) = 0;
		virtual bool ReadFileIntoMemory(const FileId& fileId, MemBuffer& outMemBuffer) = 0;

		////////////////////////////////////////
		/// Getters
		////////////////////////////////////////

		auto GetAlias() const -> const auto& { return m_alias; }
		auto GetAliasHash() const -> const auto& { return m_aliasHash; }

	private:
		std::string m_alias;
		uint64_t m_aliasHash;
	};

} // namespace gfs