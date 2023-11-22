#pragma once

#include "binary_streams.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

// #TODO: Create binary stream wrapper (to help force binary input/output): https://cplusplus.com/forum/general/97851/

namespace gfs
{
    class FileImporter;

    template<typename S, typename T, typename = void>
    struct is_to_stream_writable : std::false_type {};

    template<typename S, typename T>
    struct is_to_stream_writable<S, T, std::void_t<decltype(std::declval<S&>() << std::declval<T>())>>
        : std::true_type {};

    using MountID = uint32_t;
    using FileID = uint64_t;

    constexpr MountID InvalidMountId = 0;

    struct FormatHeader
    {
        char MagicNumber[4];
        uint16_t FormatVersion;
        uint32_t FileCount;

        friend auto operator<<(std::ostream& stream, const FormatHeader& header)->std::ostream&;
        friend auto operator>>(std::istream& stream, FormatHeader& header)->std::istream&;
    };

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
            FileID FileId; // The Unique Identifier of the file.
            MountID MountId; // The Id of the mount this file is in.
            std::filesystem::path MountRelPath;// Path to this file relative to the mount it is in.
            std::vector<FileID> FileDependencies; // Files this file references.
            uint32_t UncompressedSize;
            uint32_t CompressedSize;
            uint32_t Offset;

            friend auto operator<<(std::ostream& stream, const File& header)->std::ostream&;
            friend auto operator>>(std::istream& stream, File& header)->std::istream&;
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

        /**
         * @brief
         * @param filename
         * @param fileId
         * @param dataObject
         * @param compress
         * @return
        */
        bool WriteFile(MountID mountId, const std::filesystem::path& filename, FileID fileId, const BinaryStreamable& dataObject, bool compress);

        /**
         * @brief
         * @param fileId
         * @param dataObject
         * @return
        */
        bool ReadFile(FileID fileId, BinaryStreamable& dataObject);

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

        auto GetMountPathIsIn(const std::filesystem::path& path) -> MountID;

        void GatherFilesInMount(const Mount& mount);
        void ValidateAndRegisterFile(const std::filesystem::path& filename, MountID mountId);

    private:
        std::unordered_map<MountID, Mount> m_mountMap;
        MountID m_nextMountId = 1;

        std::unordered_map<FileID, File> m_files;

        std::unordered_map < size_t, std::shared_ptr < FileImporter >> m_extImporterMap;
    };

}