#include "gfs/filesystem.hpp"

#include <cassert>

namespace gfs
{
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

}
