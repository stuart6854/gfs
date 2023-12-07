#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace gfs
{
	using MemBuffer = std::vector<uint8_t>;

	class StrId
	{
	public:
		StrId() = default;
		StrId(const std::string& str) : m_id(std::hash<std::string>{}(str))
		{
			if (str.empty())
				m_id = 0;
		}
		StrId(const char* str) : StrId(std::string(str)) {}
		StrId(const StrId&) = default;
		StrId(StrId&&) = default;
		~StrId() = default;

		operator bool() const { return m_id != 0; }
		operator uint64_t() const { return m_id; }

		bool operator==(const StrId& rhs) const { return m_id == rhs.m_id; }
		bool operator!=(const StrId& rhs) const { return !(*this == rhs); }

		auto operator=(const StrId&) -> StrId& = default;
		auto operator=(StrId&&) -> StrId& = default;

	private:
		uint64_t m_id = 0;
	};

} // namespace gfs

namespace std
{
	template <>
	struct hash<gfs::StrId>
	{
		std::size_t operator()(const gfs::StrId& id) const noexcept { return id; }
	};
} // namespace std