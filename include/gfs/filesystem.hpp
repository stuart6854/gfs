#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>

namespace gfs
{
    using MountID = uint32_t;
    using FileID = uint64_t;

    constexpr MountID InvalidMountId = 0;

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

        /**
         * @brief
         * @param rootDir
         * @param allowUnmount
         * @return 0 if failed to mount directory. >0 if successful.
        */
        auto MountDir(const std::filesystem::path& rootDir, bool allowUnmount = true) -> MountID;

        /**
         * @brief
         * @param id
         * @return True if a was unmounted.
        */
        bool UnmountDir(MountID id);

        /**
         * @brief
         * @param rootDir
         * @return 0 if mount does not exist. Otherwise, returns the mounts id.
        */
        auto GetMountId(const std::filesystem::path& rootDir) -> MountID;

        /**
         * @brief Runs the given function for each mounted directory.
         * @param func
        */
        void ForEachMount(const std::function<void(const Mount&)>& func);

        //////////////////////////////////////////////////////////////////////////
        // Files
        //////////////////////////////////////////////////////////////////////////

        struct File
        {
            FileID Uuid; // The Unique Identifier of the file.
            MountID MountId; // The Id of the mount this file is in.
            std::filesystem::path MountRelPath;// Path to this file relative to the mount it is in.
            std::vector<FileID> FileDependencies; // Files this file references.
        };

        /**
         * @brief
         * @param id
         * @return
        */
        auto GetFile(FileID id) -> const File*;

        /**
         * @brief
         * @param func
        */
        void ForEachFile(const std::function<void(const File& file)>& func);

        //////////////////////////////////////////////////////////////////////////
        // Import
        //////////////////////////////////////////////////////////////////////////

        /**
         * @brief
         * @param fileExts
         * @param importer
        */
        void SetImporter(const std::vector < std::string>& fileExts, const std::shared_ptr<FileImporter>& importer);

        /**
         * @brief
         * @param fileExt
         * @return
        */
        auto GetImporter(const std::string& fileExt) -> std::shared_ptr<FileImporter>;

        //////////////////////////////////////////////////////////////////////////
        // Utility
        //////////////////////////////////////////////////////////////////////////

        /**
         * @brief
         * @param path
         * @param mountId
         * @return
         * @attention Unsure if this works as intended. Taken from https://stackoverflow.com/questions/61123627/check-if-an-stdfilesystempath-is-inside-a-directory
        */
        bool IsPathInMount(const std::filesystem::path& path, MountID mountId);

        /**
         * @brief
         * @param path
         * @return
         * @attention Unsure if this works as intended. See `IsPathInMount()`.
        */
        bool IsPathInAnyMount(const std::filesystem::path& path);

    private:
        auto GetMount(MountID id) -> Mount*;

    private:
        std::vector<Mount> m_mounts;
        std::unordered_map<MountID, Mount*> m_mountIdMap;
        MountID m_nextMountId = 1;

        std::vector<File> m_files;
        std::unordered_map<uint64_t, File*> m_fileIdMap;

        std::unordered_map < size_t, std::shared_ptr < FileImporter >> m_extImporterMap;
    };
}