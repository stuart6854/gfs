#include <catch2/catch_test_macros.hpp>
#include <gfs/gfs.hpp>

#include <filesystem>
#include <iostream>

TEST_CASE("Mounts", "[mount]")
{
	gfs::Filesystem fs{};

	SECTION("General")
	{
		auto* mount = fs.GetMountByAlias("some_mount");
		REQUIRE(mount == nullptr);

		const auto* mountAlias = "alias";
		fs.AddMount<gfs::PhysicalMount>(mountAlias, "./data");
		mount = fs.GetMountByAlias(mountAlias);
		REQUIRE(mount != nullptr);
	}

	SECTION("Physical Mount")
	{
		auto* osMount = fs.AddMount<gfs::PhysicalMount>("data", "./data");
		REQUIRE(osMount != nullptr);

		SECTION("Files")
		{
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
				REQUIRE_FALSE(osMount->DeleteFile(""));
				REQUIRE_FALSE(osMount->DeleteFile("invalid_filename"));
				REQUIRE(osMount->DeleteFile("hello_world.txt"));
			}
			SECTION("MoveFile")
			{
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
				REQUIRE_FALSE(osMount->CheckFileExists(""));
				REQUIRE_FALSE(osMount->CheckFileExists("abc"));
				REQUIRE_FALSE(osMount->CheckFileExists("abc.txt"));
				REQUIRE_FALSE(osMount->CheckFileExists("hello_world.txt"));
				REQUIRE_FALSE(osMount->CheckFileExists("some_file.txt"));
				REQUIRE_FALSE(osMount->CheckFileExists("SoMe_fILe_moVEd.txt"));
				REQUIRE(osMount->CheckFileExists("some_file_moved.txt"));
				REQUIRE(osMount->CheckFileExists("some_dir/some_file.txt"));
			}
		}

		fs.RemoveMount(*osMount);

		// Clean up
		std::error_code error{};
		std::filesystem::remove_all(".data", error);
	}
}