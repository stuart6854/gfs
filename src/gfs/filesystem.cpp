#include "gfs/filesystem.hpp"

#include <lz4.h>

#include <cassert>
#include <fstream>
#include <iostream>

namespace gfs
{
    constexpr char FS_FORMAT_MAGIC_NUM[4] = { 'g','f' ,'s' ,'f' }; // GFS Format
    constexpr auto FS_FORMAT_VERSION = 1;

    constexpr uint32_t FS_FORMAT_PATH_LENGTH = 255;

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
        std::ifstream stream(filename, std::ios::binary);
        if (!stream)
            return;

        FormatHeader header{};
        stream >> header;

        if (std::memcmp(header.MagicNumber, FS_FORMAT_MAGIC_NUM, sizeof(header.MagicNumber)) != 0)
            return;

        File file{};
        stream >> file;

        file.MountId = mountId;
        file.MountRelPath = filename;
        m_files[file.FileId] = file;
    }

    bool Filesystem::WriteFile(MountID mountId, const std::filesystem::path& filename, FileID fileId, const BinaryStreamable& dataObject, bool compress)
    {
        auto* mount = GetMount(mountId);
        if (!mount)
            return false;

        const auto tempBinaryFile = mount->RootDirPath / (filename.string() + ".tmp");

        BinaryStreamWrite tempFileStream(tempBinaryFile);
        if (!tempFileStream)
            return false;

        tempFileStream.Write(dataObject);
        tempFileStream.Close();

        BinaryStreamWrite stream(mount->RootDirPath / filename);
        if (!stream)
            return false;

        std::ifstream dataStream(tempBinaryFile, std::ios::binary | std::ios::ate);
        if (!dataStream)
            return false;

        const auto dataSize = uint32_t(dataStream.tellg());
        dataStream.seekg(0, std::ios::beg);
        std::vector<uint8_t> uncompressedDataBuffer(dataSize);
        dataStream.read(reinterpret_cast<char*>(uncompressedDataBuffer.data()), dataSize);
        dataStream.close();

        File file{};
        file.FileId = fileId;
        file.MountId = mountId;
        file.MountRelPath = filename;
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
        file.Offset = uint32_t(stream.TellP());
        const uint32_t offsetPos = file.Offset - sizeof(file.Offset);

        stream.Write(sizeof(uint8_t) * compressedDataBuffer.size(), compressedDataBuffer.data());

        // Go back and write data offset
        stream.SeekP(offsetPos, false);
        stream.Write(file.Offset);

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

    bool Filesystem::ReadFile(FileID fileId, BinaryStreamable& dataObject)
    {
        auto* file = GetFile(fileId);
        if (!file)
            return false;

        auto* mount = GetMount(file->MountId);
        if (!mount)
            return false;

        std::ifstream stream(mount->RootDirPath / file->MountRelPath, std::ios::binary);
        stream.unsetf(std::ios_base::skipws);
        stream.seekg(file->Offset);

        const bool isCompressed = file->CompressedSize != file->UncompressedSize;

        std::vector<uint8_t> bufferUncompressed(file->UncompressedSize);
        std::vector<uint8_t> bufferCompressed(isCompressed ? file->CompressedSize : 0);

        stream.read(reinterpret_cast<char*>(isCompressed ? bufferCompressed.data() : bufferUncompressed.data()), file->CompressedSize);
        if (file->CompressedSize != file->UncompressedSize)
        {
            // Data is compressed.
            int bytes = LZ4_decompress_safe(reinterpret_cast<const char*>(bufferCompressed.data()), reinterpret_cast<char*>(bufferUncompressed.data()), file->CompressedSize, file->UncompressedSize);

            if (uint32_t(bytes) != file->UncompressedSize)
                return false; // Did not decompress to original size.
        }
        else
        {
        }

        BinaryStreamRead dataStream(bufferUncompressed.data(), bufferUncompressed.size());
        dataObject.Read(dataStream);

        return true;
    }

    auto operator<<(std::ostream& stream, const FormatHeader& header) -> std::ostream&
    {
        stream.write(reinterpret_cast<const char*>(&header.MagicNumber), sizeof(header.MagicNumber));
        stream.write(reinterpret_cast<const char*>(&header.FormatVersion), sizeof(header.FormatVersion));
        stream.write(reinterpret_cast<const char*>(&header.FileCount), sizeof(header.FileCount));
        return stream;
    }

    auto operator>>(std::istream& stream, FormatHeader& header) -> std::istream&
    {
        stream.read(reinterpret_cast<char*>(&header.MagicNumber), sizeof(header.MagicNumber));
        stream.read(reinterpret_cast<char*>(&header.FormatVersion), sizeof(header.FormatVersion));
        stream.read(reinterpret_cast<char*>(&header.FileCount), sizeof(header.FileCount));
        return stream;
    }

    auto operator<<(std::ostream& stream, const Filesystem::File& file) -> std::ostream&
    {
        stream.write(reinterpret_cast<const char*>(&file.FileId), sizeof(file.FileId));

        // Mount relative path
        auto pathStr = file.MountRelPath.string();
        pathStr.resize(FS_FORMAT_PATH_LENGTH);
        stream.write(reinterpret_cast<const char*>(pathStr.data()), pathStr.size());

        // File dependencies
        const auto count = uint32_t(file.FileDependencies.size());
        stream.write(reinterpret_cast<const char*>(&count), sizeof(count));
        if (!file.FileDependencies.empty())
            stream.write(reinterpret_cast<const char*>(file.FileDependencies.data()), sizeof(file.FileDependencies[0]) * count);

        stream.write(reinterpret_cast<const char*>(&file.UncompressedSize), sizeof(file.UncompressedSize));
        stream.write(reinterpret_cast<const char*>(&file.CompressedSize), sizeof(file.CompressedSize));
        stream.write(reinterpret_cast<const char*>(&file.Offset), sizeof(file.Offset));
        return stream;
    }

    auto operator>>(std::istream& stream, Filesystem::File& file) -> std::istream&
    {
        stream.read(reinterpret_cast<char*>(&file.FileId), sizeof(file.FileId));

        // Mount relative path
        std::string pathStr(FS_FORMAT_PATH_LENGTH, 0);
        stream.read(reinterpret_cast<char*>(pathStr.data()), FS_FORMAT_PATH_LENGTH);
        pathStr.erase(pathStr.find_last_not_of(char(0)) + 1, std::string::npos);
        file.MountRelPath = pathStr;

        // File dependencies
        uint32_t count = 0;
        stream.read(reinterpret_cast<char*>(&count), sizeof(count));
        file.FileDependencies.resize(count);
        if (!file.FileDependencies.empty())
            stream.read(reinterpret_cast<char*>(file.FileDependencies.data()), sizeof(file.FileDependencies[0]) * count);

        stream.read(reinterpret_cast<char*>(&file.UncompressedSize), sizeof(file.UncompressedSize));
        stream.read(reinterpret_cast<char*>(&file.CompressedSize), sizeof(file.CompressedSize));
        stream.read(reinterpret_cast<char*>(&file.Offset), sizeof(file.Offset));
        return stream;
    }

}
