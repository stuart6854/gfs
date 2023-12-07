#include <catch2/catch_test_macros.hpp>
#include <gfs/gfs.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
	/*bool CompareFiles(const std::string& p1, const std::string& p2)
	{
		std::ifstream f1(p1, std::ifstream::binary | std::ifstream::ate);
		std::ifstream f2(p2, std::ifstream::binary | std::ifstream::ate);

		if (f1.fail() || f2.fail())
		{
			return false; // file problem
		}

		if (f1.tellg() != f2.tellg())
		{
			return false; // size mismatch
		}

		// seek back to beginning and use std::equal to compare contents
		f1.seekg(0, std::ifstream::beg);
		f2.seekg(0, std::ifstream::beg);
		return std::equal(std::istreambuf_iterator(f1.rdbuf()), std::istreambuf_iterator<char>(), std::istreambuf_iterator<char>(f2.rdbuf()));
	}*/

} // namespace

TEST_CASE("Physical Mount", "[mount][physical]")
{
	// Pre-Test Clean up
	std::error_code error{};
	std::filesystem::remove_all("./data", error);

	gfs::Filesystem fs{};
	auto* osMount = fs.AddMount<gfs::PhysicalMount>("data", "./data");
	REQUIRE(osMount != nullptr);

	SECTION("CreateFile")
	{
		REQUIRE_FALSE(osMount->CreateFile(""));
		REQUIRE_FALSE(osMount->CreateFile("invalid_filename"));
		REQUIRE(osMount->CreateFile("hello_world.txt"));
		REQUIRE(osMount->CreateFile("some_file.txt"));
		REQUIRE(osMount->CreateFile("some_dir/some_file.txt"));
	}
	SECTION("DeleteFile")
	{
		REQUIRE(osMount->CreateFile("hello_world.txt"));

		REQUIRE_FALSE(osMount->DeleteFile(""));
		REQUIRE_FALSE(osMount->DeleteFile("invalid_filename"));
		REQUIRE(osMount->DeleteFile("hello_world.txt"));
	}
	SECTION("MoveFile")
	{
		REQUIRE(osMount->CreateFile("some_file.txt"));

		REQUIRE_FALSE(osMount->MoveFile("", ""));
		REQUIRE_FALSE(osMount->MoveFile("abc", ""));
		REQUIRE_FALSE(osMount->MoveFile("abc", "def"));
		REQUIRE_FALSE(osMount->MoveFile("abc.txt", "def"));
		REQUIRE_FALSE(osMount->MoveFile("abc", "def.txt"));
		REQUIRE_FALSE(osMount->MoveFile("some_file.txt", ""));
		REQUIRE_FALSE(osMount->MoveFile("some_file.txt", "invalid_filename"));
		REQUIRE(osMount->MoveFile("some_file.txt", "some_file_moved.txt"));
	}
	SECTION("CheckFileExists")
	{
		REQUIRE(osMount->CreateFile("some_dir/some_file.txt"));
		REQUIRE(osMount->CreateFile("some_file.txt"));
		REQUIRE(osMount->MoveFile("some_file.txt", "some_file_moved.txt"));

		REQUIRE_FALSE(osMount->HasFile(""));
		REQUIRE_FALSE(osMount->HasFile("abc"));
		REQUIRE_FALSE(osMount->HasFile("abc.txt"));
		REQUIRE_FALSE(osMount->HasFile("hello_world.txt"));
		REQUIRE_FALSE(osMount->HasFile("some_file.txt"));
		REQUIRE_FALSE(osMount->HasFile("SoMe_fILe_moVEd.txt"));
		REQUIRE(osMount->HasFile("some_file_moved.txt"));
		REQUIRE(osMount->HasFile("some_dir/some_file.txt"));
	}
	std::string txtData = R"(
Lorem ipsum dolor sit amet, consectetur adipiscing elit,
sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris
nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in
reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla
pariatur. Excepteur sint occaecat cupidatat non proident, sunt in
culpa qui officia deserunt mollit anim id est laborum.
)";
	SECTION("WriteMemoryToFile/ReadFileIntoMemory")
	{
		gfs::MemBuffer memBuffer{};
		memBuffer.reserve(txtData.size());
		memBuffer.insert(memBuffer.begin(), txtData.begin(), txtData.end());

		REQUIRE_FALSE(osMount->WriteMemoryToFile("", memBuffer));
		REQUIRE_FALSE(osMount->WriteMemoryToFile("some_file", memBuffer));
		REQUIRE(osMount->WriteMemoryToFile("some_file.bin", memBuffer));
		REQUIRE(std::filesystem::file_size("./data/some_file.bin") == memBuffer.size());

		gfs::MemBuffer readMemBuffer{};
		REQUIRE(osMount->ReadFileIntoMemory("some_file.bin", readMemBuffer));
		REQUIRE(readMemBuffer.size() == memBuffer.size());
		REQUIRE(std::memcmp(readMemBuffer.data(), memBuffer.data(), readMemBuffer.size()) == 0);
	}
	SECTION("WriteTextToFile/ReadFileIntoString")
	{
		REQUIRE_FALSE(osMount->WriteStringToFile("", txtData));
		REQUIRE_FALSE(osMount->WriteStringToFile("some_file", txtData));
		REQUIRE(osMount->WriteStringToFile("some_file.txt", txtData));
		REQUIRE(std::filesystem::file_size("./data/some_file.txt") == txtData.size());

		std::string readString{};
		REQUIRE(osMount->ReadFileIntoString("some_file.txt", readString));
		REQUIRE(readString.size() == txtData.size());
		REQUIRE(std::strcmp(readString.c_str(), txtData.c_str()) == 0);
	}

	fs.RemoveMount(*osMount);
}

TEST_CASE("General", "[general][mount]")
{
	// Pre-Test Clean up
	std::error_code error{};
	std::filesystem::remove_all("./data", error);

	gfs::Filesystem fs{};

	SECTION("add mount")
	{
		auto* mount = fs.AddMount<gfs::PhysicalMount>("alias", "./data");
		REQUIRE(mount != nullptr);
		fs.RemoveMount(*mount);
	}

	SECTION("get mount by alias")
	{
		auto* mount = fs.GetMountByAlias("some_mount");
		REQUIRE(mount == nullptr);

		const auto* mountAlias = "alias";
		fs.AddMount<gfs::PhysicalMount>(mountAlias, "./data");
		mount = fs.GetMountByAlias(mountAlias);
		REQUIRE(mount != nullptr);
	}
}