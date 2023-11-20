#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>

namespace gfs
{
    using MountID = uint32_t;

    class FileImporter;

    class Filesystem
    {
    public:
        Filesystem() = default;
        ~Filesystem() = default;

        //////////////////////////////////////////////////////////////////////////
        // Mounts
        //////////////////////////////////////////////////////////////////////////

        struct Mount
        {
            MountID Id;
            std::filesystem::path RootDirPath;
            bool AllowUnmount;
        };

        auto MountDir(const std::filesystem::path& rootDir, bool allowUnmount = true) -> MountID;
        bool UnmountDir(MountID id);

        auto GetMountId(const std::filesystem::path& rootDir) -> MountID;

        void ForEachMount(const std::function<void(const Mount&)>& func);

        //////////////////////////////////////////////////////////////////////////
        // Files
        //////////////////////////////////////////////////////////////////////////

        struct File
        {
            uint64_t Uuid;
            MountID MountId;
            std::filesystem::path MountRelPath;
        };

        auto GetFile(uint64_t id) -> const File*;

        void ForEachFile(const std::function<void(const File& file)>& func);

        //////////////////////////////////////////////////////////////////////////
        // Import
        //////////////////////////////////////////////////////////////////////////

        void SetImporter(const std::vector < std::string>& fileExts, const std::shared_ptr<FileImporter>& importer);
        auto GetImporter(const std::string& fileExt) -> std::shared_ptr<FileImporter>;

        //////////////////////////////////////////////////////////////////////////
        // Utility
        //////////////////////////////////////////////////////////////////////////

        bool IsPathInMount(const std::filesystem::path& path, MountID mountId);
        bool IsPathInAnyMount(const std::filesystem::path& path);

    private:
        auto GetMount(MountID id) -> Mount*;

    private:
        std::vector<Mount> m_mounts;
        std::unordered_map<MountID, Mount*> m_mountIdMap;
        std::unordered_map<size_t, Mount*> m_mountPathMap;
        MountID m_nextMountId = 1;

        std::vector<File> m_files;
        std::unordered_map<uint64_t, File*> m_fileIdMap;

        std::unordered_map < size_t, std::shared_ptr < FileImporter >> m_extImporterMap;
    };
}