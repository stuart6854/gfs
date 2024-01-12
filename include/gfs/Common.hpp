#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace gfs
{
	using MemBuffer = std::vector<uint8_t>;

	class FileId
	{
	public:
		FileId() = default;
		FileId(const std::string& str) : m_id(std::hash<std::string>{}(str))
		{
			if (str.empty())
				m_id = 0;
		}
		FileId(const char* str) : FileId(std::string(str)) {}
		FileId(uint64_t id) : m_id(id) {}
		FileId(const FileId&) = default;
		FileId(FileId&&) = default;
		~FileId() = default;

		operator bool() const { return m_id != 0; }
		operator uint64_t() const { return m_id; }

		bool operator==(const FileId& rhs) const { return m_id == rhs.m_id; }
		bool operator!=(const FileId& rhs) const { return !(*this == rhs); }

		auto operator=(const FileId&) -> FileId& = default;
		auto operator=(FileId&&) -> FileId& = default;

	private:
		uint64_t m_id = 0;
	};

} // namespace gfs

namespace std
{
	template <>
	struct hash<gfs::FileId>
	{
		std::size_t operator()(const gfs::FileId& id) const noexcept { return id; }
	};
} // namespace std