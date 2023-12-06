#pragma once

#include <string>
#include <utility>

namespace gfs
{
	class Mount
	{
	public:
		explicit Mount(std::string alias) : m_alias(std::move(alias)) {}
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

		virtual bool CreateFile(const std::string& filename) = 0;
		virtual bool DeleteFile(const std::string& filename) = 0;

		/**
		 * \brief Used to both move and rename files.
		 * \param filename The file to target.
		 * \param newFilename The filename to move the target to.
		 * \return TRUE if successful.
		 */
		virtual bool MoveFile(const std::string& filename, const std::string& newFilename) = 0;

		virtual bool CheckFileExists(const std::string& filename) const = 0;

		////////////////////////////////////////
		/// Getters
		////////////////////////////////////////

		auto GetAlias() const -> const auto& { return m_alias; }

	private:
		std::string m_alias;
	};

} // namespace gfs