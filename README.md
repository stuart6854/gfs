# GFS - Game (Virtual) Filesystem

[![CMake Build](https://github.com/stuart6854/gfs/actions/workflows/cmake_build.yml/badge.svg?branch=main)](https://github.com/stuart6854/gfs/actions/workflows/cmake_build.yml)

## Introduction

A (virtual) filesystem designed to be used by games and game engines.

### Features
- Mount & Unmount directories
- Create files under mounts with data
- Read files inside of mounts using file ids
- Iterate mounts & files
- Optionally compress file data.
- Combine multiple files into single archive files.

## Requirements

- C++17 capable compiler (MSVC, Clang++, G++)
- CMake 3.15+

## Usage

This project can either be compiled on its own or consumed as a submodule (CMake ```add_subdirectory```).

## Example

See the `testbed` project for for an runnable example.

```c++
gfs::Filesystem fs;

// Mount a directory that cannot be unmounted.
MountID dataMount = fs.MountDir("data"), false);
if(dataMount == gfs::InvalidMountId)
    std::cout << "Failed to mount data dir." << std::endl;

// Mount a directory that can be unmounted.
MountID modsMount = fs.MountDir("mods"), true);
if(dataMount == gfs::InvalidMountId)
    std::cout << "Failed to mount mods dir." << std::endl;

// Ummount a directory.
bool wasUmounted = fs.Unmount(modsMount);

// Iterate mounts & files.
fs.ForeachMount([](const gfs::Filesystem::Mount& mount){});
fs.ForeachFile([](const gfs::Filesystem::File& file){});

/* Write new file */
struct SomeGameData : gfx::BinaryStreamable
{
    float time;
    uint32_t x;
    uint32_t y;

    void Read(gfs::BinaryStreamRead& stream) override;
    void Write(gfs::BinaryStreamWrite& stream) const override;
}
std::filesystem::path filename = "some_file.bin";
FileID newFileId = 98475845; // Could be random uint64 or hash of filepath.
std::vector<FileId> fileDependencies{};
SomeGameData dataObj{};
bool compressData = false; // File data can optionally be compressed using LZ4.
bool wasWritten = fs.WriteFile(dataMount, filename, newFileId, fileDependencies, dataObj, compressData);

/* Read file */
// Reads the files data from the disk and writes to the passed `BinaryStreamable` object.
// Compressed data will also be decompressed automatically.
bool wasRead = fs.ReadFile(newFileId, dataObj);

/* Create archive */
gfs::MountID mountId = dataMount;
std::filesystem::path filename = "archive_file.pbin";
std::vector<gfs::FileID> files{ 98475845, 111, 222, 666 };
bool wasCreated = fs.CreateArchive(mountId, filename, files);

/* Import files */
struct MyImporter : gfs::FileImporter
{
    bool Import(gfs::Filesystem& fs, const std::filesystem::path& importFilename, gfs::MountID outputMount, const std::filesystem::path& outputDir) override
	{ 
        ...
	}

	bool Reimport(gfs::Filesystem& fs, const gfs::Filesystem::File& file) override 
    {
        ...
    }
};
fs.SetImporter({ ".txt", ".ext" }, std::make_shared<MyImporter>());
bool wasImported = fs.Import("path/to/external/file.txt", mountId, "mount/rel/output/dir/");
bool wasReimported = fs.Reimport(fileId);

``` 

## Planned Features

- Add data compression threshold eg. only compress data greater than ~0.5MB