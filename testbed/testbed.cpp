
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

struct DataType
{
    uint32_t value_a;
    float value_b;
    bool value_c;

    friend auto operator<<(std::ostream& stream, const DataType& value)->std::ostream&
    {
        stream.write(reinterpret_cast<const char*>(&value.value_a), sizeof(value.value_a));
        stream.write(reinterpret_cast<const char*>(&value.value_b), sizeof(value.value_b));
        stream.write(reinterpret_cast<const char*>(&value.value_c), sizeof(value.value_c));
        return stream;
    }
};

struct TextResource
{
    std::string Text;

    friend auto operator<<(std::ostream& stream, const TextResource& value)->std::ostream&
    {
        uint32_t size = uint32_t(value.Text.size());
        stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
        stream.write(reinterpret_cast<const char*>(value.Text.data()), sizeof(std::string::value_type) * size);
        return stream;
    }
    friend auto operator>>(std::istream& stream, TextResource& value)->std::istream&
    {
        uint32_t size = 0;
        stream.read(reinterpret_cast<char*>(&size), sizeof(size));
        value.Text.resize(size);
        stream.read(reinterpret_cast<char*>(value.Text.data()), sizeof(std::string::value_type) * size);
        return stream;
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
    assert(fs.WriteFile("mount_a/file.rbin", 234598753, data, false));

    TextResource texResource{};
    texResource.Text = ReadTextFile("external_files/txt_file.txt");
    assert(fs.WriteFile("mount_a/aa/txt_file.rbin", 67236784, texResource, false));

    TextResource texResourceBigger{};
    for (auto i = 0; i < 1000; ++i)
        texResourceBigger.Text += texResource.Text;
    assert(fs.WriteFile("mount_b_locked/txt_file_bigger.rbin", 68923789324, texResourceBigger, false));
    assert(fs.WriteFile("mount_b_locked/txt_file_bigger_compressed.rbin", 8367428478, texResourceBigger, true));

    std::cout << "Files" << std::endl;
    fs.ForEachFile([](const gfs::Filesystem::File& file) {
        std::cout << "- " << file.FileId << " - " << file.MountRelPath << std::endl;
        });

    return 0;
}