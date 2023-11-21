#include "gfs/filesystem.hpp"

#include <lz4.h>

#include <cassert>
#include <fstream>
#include <iostream>

namespace gfs
{
    constexpr char FS_FORMAT_MAGIC_NUM[4] = { 'g','f' ,'s' ,'f' }; // GFS Format
    constexpr auto FS_FORMAT_VERSION = 1;

    auto Filesystem::MountDir(const std::filesystem::path& rootDir, bool allowUnmount) -> MountID
    {
        if (!std::filesystem::exists(rootDir))
            return InvalidMountId;

        if (!std::filesystem::is_directory(rootDir))
            return InvalidMountId;

        auto& mount = m_mountMap[m_nextMountId];
        mount.RootDirPath = rootDir;
        mount.AllowUnmount = allowUnmount;
        mount.Id = m_nextMountId++;
        assert(mount.Id != InvalidMountId);

        GatherFilesInMount(mount);

        return mount.Id;
    }

    bool Filesystem::UnmountDir(MountID id)
    {
        const auto it = m_mountMap.find(id);
        if (it == m_mountMap.end())
            return false;

        if (!it->second.AllowUnmount)
            return false;

        m_mountMap.erase(it);
        return true;
    }

    auto Filesystem::GetMountId(const std::filesystem::path& rootDir) -> MountID
    {
        const auto rootDirAbs = std::filesystem::absolute(rootDir);
        for (const auto& [id, mount] : m_mountMap)
        {
            const auto mountDirAbs = std::filesystem::absolute(mount.RootDirPath);
            if (rootDirAbs == mountDirAbs)
                return mount.Id;
        }
        return InvalidMountId;
    }

    void Filesystem::ForEachMount(const std::function<void(const Mount&)>& func)
    {
        for (const auto& [id, mount] : m_mountMap)
            func(mount);
    }

    auto Filesystem::GetFile(FileID id) -> const File*
    {
        const auto it = m_files.find(id);
        if (it == m_files.end())
            return nullptr;

        return &it->second;
    }

    void Filesystem::ForEachFile(const std::function<void(const File& file)>& func)
    {
        for (const auto& [id, file] : m_files)
            func(file);
    }

    void Filesystem::SetImporter(const std::vector < std::string>& fileExts, const std::shared_ptr<FileImporter>& importer)
    {
        for (const auto& ext : fileExts)
        {
            const auto extHash = std::hash<std::string>{}(ext);
            m_extImporterMap[extHash] = importer;
        }
    }

    auto Filesystem::GetImporter(const std::string& fileExt) -> std::shared_ptr<FileImporter>
    {
        const auto extHash = std::hash<std::string>{}(fileExt);
        const auto it = m_extImporterMap.find(extHash);
        if (it == m_extImporterMap.end())
            return nullptr;

        return it->second;
    }

    bool Filesystem::IsPathInMount(const std::filesystem::path& path, MountID mountId)
    {
        auto* mount = GetMount(mountId);
        if (!mount)
            return false;

        std::filesystem::path finalPath;
        try
        {
            finalPath = std::filesystem::canonical(mount->RootDirPath / path);
        }
        catch (const std::exception& /*ex*/)
        {
            return false; // Path/File does not exists.
        }

        auto [rootEnd, nothing] = std::mismatch(mount->RootDirPath.begin(), mount->RootDirPath.end(), finalPath.begin());
        if (rootEnd == mount->RootDirPath.end())
            return false;

        return true;
    }

    bool Filesystem::IsPathInAnyMount(const std::filesystem::path& path)
    {
        for (const auto& [id, mount] : m_mountMap)
        {
            if (IsPathInMount(path, mount.Id))
                return true;
        }
        return false;
    }

    auto Filesystem::GetMount(MountID id) -> Mount*
    {
        const auto it = m_mountMap.find(id);
        if (it == m_mountMap.end())
            return nullptr;

        return &it->second;
    }

    auto Filesystem::GetMountPathIsIn(const std::filesystem::path& path) -> MountID
    {
        for (auto& [id, mount] : m_mountMap)
        {
            if (IsPathInMount(path, id))
                return id;
        }
        return InvalidMountId;
    }

    void Filesystem::GatherFilesInMount(const Mount& mount)
    {
        for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(mount.RootDirPath))
        {
            if (!dirEntry.is_regular_file())
                continue;

            const auto filePath = dirEntry.path();

            const auto fileSize = std::filesystem::file_size(filePath);
            if (fileSize < sizeof(FormatHeader))
                continue;

            ValidateAndRegisterFile(filePath, mount.Id);
        }
    }

    void Filesystem::ValidateAndRegisterFile(const std::filesystem::path& filename, MountID mountId)
    {
        BinaryStreamRead stream(filename);
        if (!stream)
            return;

        FormatHeader header{};
        stream.Read(header);

        if (std::memcmp(header.MagicNumber, FS_FORMAT_MAGIC_NUM, sizeof(header.MagicNumber)) != 0)
            return;

        File file{};
        stream.Read(file);

        file.MountId = mountId;
        file.MountRelPath = filename;
        m_files[file.FileId] = file;
    }

    bool Filesystem::WriteFile(const std::filesystem::path& filename, FileID fileId, const BinaryStreamable& dataObject, bool compress)
    {
        const auto tempBinaryFile = filename.string() + ".tmp";

        BinaryStreamWrite tempFileStream(tempBinaryFile);
        if (!tempFileStream)
            return false;

        tempFileStream.Write(dataObject);
        const auto binaryDataSize = tempFileStream.TellP();
        tempFileStream.Close();

        BinaryStreamWrite stream(filename);
        if (!stream)
            return false;

        BinaryStreamRead dataStream(tempBinaryFile);
        if (!dataStream)
            return false;

        auto uncompressedDataBuffer = dataStream.Read(binaryDataSize);
        dataStream.Close();

        File file{};
        file.FileId = fileId;
        file.MountId = GetMountPathIsIn(filename);
        file.FileDependencies = {};

        std::vector<uint8_t> compressedDataBuffer;
        file.UncompressedSize = uint32_t(uncompressedDataBuffer.size());
        if (compress)
        {
            compressedDataBuffer.resize(uncompressedDataBuffer.size());
            const uint32_t compressedSize = LZ4_compress_default(
                reinterpret_cast<const char*>(uncompressedDataBuffer.data()),
                reinterpret_cast<char*>(compressedDataBuffer.data()),
                int32_t(uncompressedDataBuffer.size()),
                int32_t(compressedDataBuffer.size())
            );
            compressedDataBuffer.resize(compressedSize);
            file.CompressedSize = compressedSize;
        }
        else
        {
            compressedDataBuffer = uncompressedDataBuffer;
            file.CompressedSize = file.UncompressedSize;
        }
        uncompressedDataBuffer.clear();

        FormatHeader header{};
        std::memcpy(header.MagicNumber, FS_FORMAT_MAGIC_NUM, sizeof(FS_FORMAT_MAGIC_NUM));
        header.FormatVersion = FS_FORMAT_VERSION;
        header.FileCount = 1;

        stream.Write(header);
        stream.Write(file);
        const uint32_t dataOffset = uint32_t(stream.TellP());
        const uint32_t offsetPos = dataOffset - sizeof(file.Offset);

        stream.Write(sizeof(uint8_t) * compressedDataBuffer.size(), compressedDataBuffer.data());

        // Go back and write data offset
        stream.SeekP(offsetPos, false);
        stream.Write(dataOffset);

        bool exists = std::filesystem::exists(tempBinaryFile);
        assert(exists);

        std::filesystem::remove(tempBinaryFile); // #ERROR: This is throwing for some reason.
        //try
        //{
        //    std::filesystem::remove(tempBinaryFile); // #ERROR: This is throwing for some reason.
        //}
        //catch (std::filesystem::filesystem_error const& ex)
        //{
        //    std::cout << "what():  " << ex.what() << '\n'
        //        << "path1(): " << ex.path1() << '\n'
        //        << "path2(): " << ex.path2() << '\n'
        //        << "code().value():    " << ex.code().value() << '\n'
        //        << "code().message():  " << ex.code().message() << '\n'
        //        << "code().category(): " << ex.code().category().name() << '\n';
        //}

        m_files[file.FileId] = file; // Register new file.

        return true;
    }

    void FormatHeader::Read(BinaryStreamRead& stream)
    {
        stream.Read(sizeof(MagicNumber), MagicNumber);
        stream.Read(FormatVersion);
        stream.Read(FileCount);
    }

    void FormatHeader::Write(BinaryStreamWrite& stream) const
    {
        stream.Write(sizeof(MagicNumber), MagicNumber);
        stream.Write(FormatVersion);
        stream.Write(FileCount);
    }

    void Filesystem::File::Read(BinaryStreamRead& stream)
    {
        stream.Read(FileId);
        stream.ReadVector(FileDependencies);
        stream.Read(UncompressedSize);
        stream.Read(CompressedSize);
        stream.Read(Offset);
    }

    void Filesystem::File::Write(BinaryStreamWrite& stream) const
    {
        stream.Write(FileId);
        stream.WriteVector(FileDependencies);
        stream.Write(UncompressedSize);
        stream.Write(CompressedSize);
        stream.Write(Offset);
    }

}
