#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

namespace gfs
{
    class BinaryStreamRead;
    class BinaryStreamWrite;

    struct BinaryStreamable
    {
        virtual ~BinaryStreamable() = default;
        virtual void Read(BinaryStreamRead& stream) = 0;
        virtual void Write(BinaryStreamWrite& stream) const = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // Write
    //////////////////////////////////////////////////////////////////////////

    class BinaryStreamWrite
    {
    public:
        BinaryStreamWrite() = default;
        BinaryStreamWrite(const std::filesystem::path& filename);
        ~BinaryStreamWrite();

        void Close();

        auto TellP() -> uint64_t;
        void SeekP(uint64_t pos, bool fromEnd);

        void Write(const BinaryStreamable& value);
        void Write(uint64_t size, const void* data);

        template<typename T>
        void WriteVector(const std::vector<T>& vector);

        template<typename T, std::enable_if_t<!std::is_base_of_v<BinaryStreamable, T>, bool> = true>
        void Write(const T& value);

        operator bool() const;

    private:
        std::ofstream m_stream;
    };

    template<>
    void BinaryStreamWrite::Write<std::string>(const std::string& value);

    template<typename T, std::enable_if_t<!std::is_base_of_v<BinaryStreamable, T>, bool>>
    void BinaryStreamWrite::Write(const T& value)
    {
        m_stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template<typename T>
    void BinaryStreamWrite::WriteVector(const std::vector<T>& vector)
    {
        uint64_t size = vector.size();
        m_stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
        for (auto it = vector.begin(); it != vector.end(); ++it)
            Write(*it);
    }

    //////////////////////////////////////////////////////////////////////////
    // Read
    //////////////////////////////////////////////////////////////////////////

    class MemBuf : public std::basic_streambuf<char>
    {
    public:
        MemBuf() = default;
        MemBuf(const uint8_t* p, size_t l)
        {
            setg((char*)p, (char*)p, (char*)p + l);
        }
    };

    class MemStream : public std::istream
    {
    public:
        MemStream() = default;
        MemStream(const uint8_t* p, size_t l) : std::istream(&m_buffer), m_buffer(p, l)
        {
            rdbuf(&m_buffer);
        }

    private:
        MemBuf m_buffer;
    };

    class BinaryStreamRead
    {
    public:
        BinaryStreamRead() = default;
        BinaryStreamRead(const uint8_t* p, size_t l);
        ~BinaryStreamRead();

        void Close();

        auto TellG() -> uint64_t;
        void SeekG(uint64_t pos, bool fromEnd);
        void Ignore(uint64_t size);

        void Read(BinaryStreamable& value);
        void Read(uint64_t size, void* data);
        auto Read(uint64_t size) -> std::vector<uint8_t>;

        template<typename T>
        void ReadVector(std::vector<T>& vector);

        template<typename T, std::enable_if_t<!std::is_base_of_v<BinaryStreamable, T>, bool> = true>
        void Read(T& value);

        operator bool() const;

    private:
        MemStream m_stream;
    };

    template<>
    void BinaryStreamRead::Read<std::string>(std::string& value);

    template<typename T, std::enable_if_t<!std::is_base_of_v<BinaryStreamable, T>, bool>>
    void BinaryStreamRead::Read(T& value)
    {
        m_stream.read(reinterpret_cast<char*>(&value), sizeof(T));
    }

    template<typename T>
    void BinaryStreamRead::ReadVector(std::vector<T>& vector)
    {
        uint64_t size = 0;
        m_stream.read(reinterpret_cast<char*>(&size), sizeof(size));
        vector.resize(size);
        for (auto it = vector.begin(); it != vector.end(); ++it)
            Read(*it);
    }

}