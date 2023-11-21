#include "gfs/filesystem.hpp"

#include <cassert>
#include <fstream>

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
        return mount.Id;
    }

    bool Filesystem::UnmountDir(MountID id)
    {
        const auto it = m_mountMap.find(id);
        if (it == m_mountMap.end())
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
        const auto it = m_fileIdMap.find(id);
        if (it == m_fileIdMap.end())
            return nullptr;

        return it->second;
    }

    void Filesystem::ForEachFile(const std::function<void(const File& file)>& func)
    {
        for (const auto& file : m_files)
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

    bool Filesystem::WriteFile(const std::filesystem::path& filename, File file, const std::filesystem::path& dataFilename, bool compress)
    {
        FormatHeader header{};
        std::memcpy(header.MagicNumber, FS_FORMAT_MAGIC_NUM, sizeof(FS_FORMAT_MAGIC_NUM));
        header.FormatVersion = FS_FORMAT_VERSION;
        header.FileCount = 1;

        std::ofstream stream(filename, std::ios::binary);
        if (!stream)
            return false;

        std::ifstream dataStream(dataFilename, std::ios::binary | std::ios::ate);
        if (!dataStream)
            return false;

        dataStream.unsetf(std::ios_base::skipws); // Do not ignore whitespace.

        const auto dataSize = uint32_t(dataStream.tellg());
        dataStream.seekg(0, std::ios::beg);

        std::vector<uint8_t> dataBuffer;
        dataBuffer.reserve(dataSize);
        dataBuffer.insert(dataBuffer.begin(),
            std::istream_iterator<uint8_t>(dataStream),
            std::istream_iterator<uint8_t>());

        file.UncompressedSize = uint32_t(dataBuffer.size());
        if (compress)
        {
            // #TODO: Compress data.
        }
        else
        {
            file.CompressedSize = file.UncompressedSize;
        }

        stream << header;
        stream << file;
        const uint32_t dataOffset = uint32_t(stream.tellp());
        const uint32_t offsetPos = dataOffset - sizeof(file.Offset);

        stream.write(reinterpret_cast<const char*>(dataBuffer.data()), sizeof(uint8_t) * dataBuffer.size());

        // Go back and write data offset
        stream.seekp(offsetPos, std::ios::beg);
        stream.write(reinterpret_cast<const char*>(&dataOffset), sizeof(offsetPos));

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
        uint16_t count = uint16_t(file.FileDependencies.size());
        stream.write(reinterpret_cast<const char*>(&count), sizeof(count));
        stream.write(reinterpret_cast<const char*>(file.FileDependencies.data()), sizeof(FileID) * count);
        stream.write(reinterpret_cast<const char*>(&file.UncompressedSize), sizeof(file.UncompressedSize));
        stream.write(reinterpret_cast<const char*>(&file.CompressedSize), sizeof(file.CompressedSize));
        stream.write(reinterpret_cast<const char*>(&file.Offset), sizeof(file.Offset));
        return stream;
    }

    auto operator>>(std::istream& stream, Filesystem::File& file) -> std::istream&
    {
        stream.read(reinterpret_cast<char*>(&file.FileId), sizeof(file.FileId));
        uint16_t count = 0;
        stream.read(reinterpret_cast<char*>(&count), sizeof(count));
        file.FileDependencies.resize(count);
        stream.read(reinterpret_cast<char*>(file.FileDependencies.data()), sizeof(FileID) * count);
        stream.read(reinterpret_cast<char*>(&file.UncompressedSize), sizeof(file.UncompressedSize));
        stream.read(reinterpret_cast<char*>(&file.CompressedSize), sizeof(file.CompressedSize));
        stream.read(reinterpret_cast<char*>(&file.Offset), sizeof(file.Offset));
        return stream;
    }

}
