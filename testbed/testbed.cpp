#include "gfs/filesystem.hpp"
#include <gfs/gfs.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <unordered_map>

auto ReadTextFile(const std::filesystem::path& filename) -> std::string
{
	std::ifstream stream(filename);
	if (!stream)
		return "";

	std::stringstream ss;
	ss << stream.rdbuf();
	return ss.str();
}

struct DataType : gfs::BinaryStreamable
{
	uint32_t value_a = 0;
	float value_b = 0.0f;
	bool value_c = false;

	DataType() = default;
	DataType(uint32_t a, float b, bool c) : value_a(a), value_b(b), value_c(c) {}

	void Read(gfs::ReadOnlyByteBuffer& buffer) override
	{
		buffer.Read(value_a);
		buffer.Read(value_b);
		buffer.Read(value_c);
	}

	void Write(gfs::WriteOnlyByteBuffer& buffer) const override
	{
		buffer.Write(value_a);
		buffer.Write(value_b);
		buffer.Write(value_c);
	}
};

struct TextResource : gfs::BinaryStreamable
{
	std::string Text;

	void Read(gfs::ReadOnlyByteBuffer& buffer) override { buffer.Read(Text); }

	void Write(gfs::WriteOnlyByteBuffer& buffer) const override { buffer.Write(Text); }
};

int main()
{
	std::cout << "GFS Testbed\n";

	std::filesystem::create_directories("mount_a/aa/ab");
	std::filesystem::create_directories("mount_b_locked");

	gfs::Filesystem fs;

	auto mountA = fs.MountDir("mount_a", true);
	assert(mountA != gfs::InvalidMountId);
	auto mountB = fs.MountDir("mount_b_locked", false);
	assert(mountB != gfs::InvalidMountId);

	std::cout << "Mounts" << std::endl;
	fs.ForEachMount([](const gfs::Filesystem::Mount& mount) { std::cout << "- " << mount.RootDirPath << std::endl; });

	// assert(fs.IsPathInAnyMount("aa/ab"));
	// assert(!fs.IsPathInAnyMount("some_file.txt"));
	// assert(!fs.IsPathInAnyMount(".././some_other_file.txt"));

	DataType data{ 5, 3.1415f, true };
	if (!fs.WriteFile(mountA, "file.rbin", 234598753, {}, data, false))
		assert(false);

	TextResource shortText{};
	shortText.Text = ReadTextFile("external_files/txt_file.txt");
	if (!fs.WriteFile(mountA, "aa/txt_file.rbin", 67236784, {}, shortText, false))
		assert(false);

	TextResource texResourceBigger{};
	for (auto i = 0; i < 1000; ++i)
		texResourceBigger.Text += shortText.Text;
	if (!fs.WriteFile(mountB, "txt_file_bigger.rbin", 68923789324, {}, texResourceBigger, false))
		assert(false);

	if (!fs.WriteFile(mountB, "txt_file_bigger_compressed.rbin", 8367428478, {}, texResourceBigger, true))
		assert(false);

	DataType readData{};
	if (!fs.ReadFile(234598753, readData))
		assert(false);
	assert(data.value_a == readData.value_a && data.value_b && readData.value_b && data.value_c == readData.value_c);

	TextResource readTextShort{};
	if (!fs.ReadFile(67236784, readTextShort))
		assert(false);
	assert(strcmp(readTextShort.Text.c_str(), shortText.Text.c_str()) == 0);

	TextResource readTexResourceUncompressed{};
	if (!fs.ReadFile(68923789324, readTexResourceUncompressed))
		assert(false);
	assert(strcmp(readTexResourceUncompressed.Text.c_str(), texResourceBigger.Text.c_str()) == 0);

	TextResource readTexResourceCompressed{};
	if (!fs.ReadFile(8367428478, readTexResourceCompressed))
		assert(false);
	assert(readTexResourceCompressed.Text == texResourceBigger.Text);

	{
		// Archive Test
		std::vector<gfs::FileID> fileIds;
		std::unordered_map<gfs::FileID, TextResource> origFileDataMap;

		auto createArchiveFile = [&](uint32_t fileId) {
			TextResource data{};
			data.Text = "I am file " + std::to_string(fileId) + "!";
			std::string filename = "archive_file_" + std::to_string(fileId) + ".rbin";
			if (!fs.WriteFile(mountA, filename, fileId, {}, data, false))
				assert(false);

			fileIds.push_back(fileId);
			origFileDataMap[fileId] = data;
		};

		createArchiveFile(1111);
		createArchiveFile(2222);
		createArchiveFile(3333);
		createArchiveFile(4444);

		if (!fs.CreateArchive(mountA, "archive.rpak", fileIds))
			assert(false);

		for (auto fileId : fileIds)
		{
			TextResource readData{};
			if (!fs.ReadFile(fileId, readData))
				assert(false);

			assert(readData.Text == origFileDataMap[fileId].Text);
		}
	}

	std::cout << "Files" << std::endl;
	fs.ForEachFile([](const gfs::Filesystem::File& file) { std::cout << "- " << file.FileId << " - " << file.MountRelPath << std::endl; });

	return 0;
}