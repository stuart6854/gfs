# GFS - Game (Virtual) Filesystem

## Introduction

A (virtual) filesystem designed to be used by games and game engines.

### Features
- Mount & Unmount directories
- Create files under mounts with data
- Read files inside of mounts using file ids
- Iterate mounts & files
- Optionally compress file data.

## Requirements

- C++17
- LZ4 v1.9.4 (https://github.com/lz4/lz4)

## Usage

Clone project and run the following to generate project files:
```console
// Windows
./GenerateProj_VS2022.bat

// Linux
./GenerateProj_GMake2.sh
```

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
FileID newFileId = 98475845; // Could be random uint64 or hash of filepath.
std::vector<FileId> fileDependencies{};
SomeGameData dataObj{};
bool compressData = false; // File data can optionally be compressed using LZ4.
bool wasWritten = fs.WriteFile(dataMount, newFileId, fileDependencies, dataObj, compressData);

/* Read file */
// Reads the files data from the disk and writes to the passed `BinaryStreamable` object.
// Compressed data will also be decompressed automatically.
bool wasRead = fs.ReadFile(newFileId, dataObj);

``` 

## Planned Features

- Add data compression threshold eg. only compress data greater than ~0.5MB
- Currently write data to temp file to get binary size. Neeed to see if it is faster to write binary data to a vector instead
- Archive files - Combine multiple (existing) files into a single archive file
- File importing - Call Filesystem::ImportFile(filename) which gets a FileImporter assigned to file ext and reads, processes and writes a new file
    - Files that were imported track the source filename - allows for reimport, which would potentially allow for asset/resource hot-reloading