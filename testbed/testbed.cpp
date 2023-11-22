
#include <gfs/gfs.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

auto ReadTextFile(const std::filesystem::path& filename) -> std::string
{
    std::ifstream stream(filename);
    if (!stream)return "";

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

    void Read(gfs::BinaryStreamRead& stream) override
    {
        stream.Read(value_a);
        stream.Read(value_b);
        stream.Read(value_c);
    }

    void Write(gfs::BinaryStreamWrite& stream) const override
    {
        stream.Write(value_a);
        stream.Write(value_b);
        stream.Write(value_c);
    }
};

struct TextResource : gfs::BinaryStreamable
{
    std::string Text;

    void Read(gfs::BinaryStreamRead& stream) override
    {
        stream.Read(Text);
    }

    void Write(gfs::BinaryStreamWrite& stream) const override
    {
        stream.Write(Text);
    }
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
    fs.ForEachMount([](const gfs::Filesystem::Mount& mount) {
        std::cout << "- " << mount.RootDirPath << std::endl;
        });

    assert(fs.IsPathInAnyMount("aa/ab"));
    assert(!fs.IsPathInAnyMount("some_file.txt"));
    assert(!fs.IsPathInAnyMount(".././some_other_file.txt"));

    DataType data{ 5, 3.1415f, true };
    if (!fs.WriteFile(mountA, "file.rbin", 234598753, {}, data, false))
        assert(false);

    TextResource texResource{};
    texResource.Text = ReadTextFile("external_files/txt_file.txt");
    if (!fs.WriteFile(mountA, "aa/txt_file.rbin", 67236784, {}, texResource, false))
        assert(false);

    TextResource texResourceBigger{};
    for (auto i = 0; i < 1000; ++i)
        texResourceBigger.Text += texResource.Text;
    if (!fs.WriteFile(mountB, "txt_file_bigger.rbin", 68923789324, {}, texResourceBigger, false))
        assert(false);

    if (!fs.WriteFile(mountB, "txt_file_bigger_compressed.rbin", 8367428478, {}, texResourceBigger, true))
        assert(false);

    std::cout << "Files" << std::endl;
    fs.ForEachFile([](const gfs::Filesystem::File& file) {
        std::cout << "- " << file.FileId << " - " << file.MountRelPath << std::endl;
        });

    DataType readData{};
    if (!fs.ReadFile(234598753, readData))
        assert(false);

    assert(data.value_a == readData.value_a && data.value_b && readData.value_b && data.value_c == readData.value_c);

    TextResource readTexResourceUncompressed{};
    if (!fs.ReadFile(68923789324, readTexResourceUncompressed))
        assert(false);
    assert(readTexResourceUncompressed.Text == texResourceBigger.Text);

    TextResource readTexResourceCompressed{};
    if (!fs.ReadFile(8367428478, readTexResourceCompressed))
        assert(false);
    assert(readTexResourceCompressed.Text == texResourceBigger.Text);

    return 0;
}