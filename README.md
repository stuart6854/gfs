# GFS - Game (Virtual) Filesystem

[![CMake Build](https://github.com/stuart6854/gfs/actions/workflows/cmake_build.yml/badge.svg?branch=main)](https://github.com/stuart6854/gfs/actions/workflows/cmake_build.yml)

## Introduction

A (virtual) filesystem designed to be used by games and game engines.

### Features

- Add/Remove mounts

[//]: # (- Iterate mounts & files)
[//]: # (- Optionally compress file data.)
[//]: # (- Combine multiple files into single archive files.)

## Requirements

- C++17 capable compiler (MSVC, Clang, GCC)
- CMake 3.15+

## Usage

This project can either be compiled on its own or consumed as a submodule (CMake ```add_subdirectory```).

## Example

*TODO*

[//]: # (See the `testbed` project for for an runnable example.)

[//]: # ()

[//]: # (```c++)

[//]: # (gfs::Filesystem fs;)

[//]: # ()

[//]: # (// Mount a directory that cannot be unmounted.)

[//]: # (MountID dataMount = fs.MountDir&#40;"data"&#41;, false&#41;;)

[//]: # (if&#40;dataMount == gfs::InvalidMountId&#41;)

[//]: # (    std::cout << "Failed to mount data dir." << std::endl;)

[//]: # ()

[//]: # (// Mount a directory that can be unmounted.)

[//]: # (MountID modsMount = fs.MountDir&#40;"mods"&#41;, true&#41;;)

[//]: # (if&#40;dataMount == gfs::InvalidMountId&#41;)

[//]: # (    std::cout << "Failed to mount mods dir." << std::endl;)

[//]: # ()

[//]: # (// Ummount a directory.)

[//]: # (bool wasUmounted = fs.Unmount&#40;modsMount&#41;;)

[//]: # ()

[//]: # (// Iterate mounts & files.)

[//]: # (fs.ForeachMount&#40;[]&#40;const gfs::Filesystem::Mount& mount&#41;{}&#41;;)

[//]: # (fs.ForeachFile&#40;[]&#40;const gfs::Filesystem::File& file&#41;{}&#41;;)

[//]: # ()

[//]: # (/* Write new file */)

[//]: # (struct SomeGameData : gfx::BinaryStreamable)

[//]: # ({)

[//]: # (    float time;)

[//]: # (    uint32_t x;)

[//]: # (    uint32_t y;)

[//]: # ()

[//]: # (    void Read&#40;gfs::BinaryStreamRead& stream&#41; override;)

[//]: # (    void Write&#40;gfs::BinaryStreamWrite& stream&#41; const override;)

[//]: # (})

[//]: # (std::filesystem::path filename = "some_file.bin";)

[//]: # (FileID newFileId = 98475845; // Could be random uint64 or hash of filepath.)

[//]: # (std::vector<FileId> fileDependencies{};)

[//]: # (SomeGameData dataObj{};)

[//]: # (bool compressData = false; // File data can optionally be compressed using LZ4.)

[//]: # (bool wasWritten = fs.WriteFile&#40;dataMount, filename, newFileId, fileDependencies, dataObj, compressData&#41;;)

[//]: # ()

[//]: # (/* Read file */)

[//]: # (// Reads the files data from the disk and writes to the passed `BinaryStreamable` object.)

[//]: # (// Compressed data will also be decompressed automatically.)

[//]: # (bool wasRead = fs.ReadFile&#40;newFileId, dataObj&#41;;)

[//]: # ()

[//]: # (/* Create archive */)

[//]: # (gfs::MountID mountId = dataMount;)

[//]: # (std::filesystem::path filename = "archive_file.pbin";)

[//]: # (std::vector<gfs::FileID> files{ 98475845, 111, 222, 666 };)

[//]: # (bool wasCreated = fs.CreateArchive&#40;mountId, filename, files&#41;;)

[//]: # ()

[//]: # (/* Import files */)

[//]: # (struct MyImporter : gfs::FileImporter)

[//]: # ({)

[//]: # (    bool Import&#40;gfs::Filesystem& fs, const std::filesystem::path& importFilename, gfs::MountID outputMount, const std::filesystem::path& outputDir&#41; override)

[//]: # (	{ )

[//]: # (        ...)

[//]: # (	})

[//]: # ()

[//]: # (	bool Reimport&#40;gfs::Filesystem& fs, const gfs::Filesystem::File& file&#41; override )

[//]: # (    {)

[//]: # (        ...)

[//]: # (    })

[//]: # (};)

[//]: # (fs.SetImporter&#40;{ ".txt", ".ext" }, std::make_shared<MyImporter>&#40;&#41;&#41;;)

[//]: # (bool wasImported = fs.Import&#40;"path/to/external/file.txt", mountId, "mount/rel/output/dir/"&#41;;)

[//]: # (bool wasReimported = fs.Reimport&#40;fileId&#41;;)

[//]: # ()

[//]: # (``` )

## Planned Features

- Allow user to specify specific mount for file-related methods on Filesystem e.g.
  filename=`"mount_alias:some_dir/some_file.txt" `