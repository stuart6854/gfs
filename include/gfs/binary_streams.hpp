#pragma once

#if 0
	#include <filesystem>
	#include <fstream>
	#include <string>
	#include <type_traits>
	#include <vector>

namespace gfs
{
    class BinaryStreamRead;
    class BinaryStreamWrite;

    class ReadOnlyByteBuffer
    {
    public:
        ReadOnlyByteBuffer(uint64_t size);
        ~ReadOnlyByteBuffer();

        void Read(uint64_t size, uint8_t* data);

        template<typename T>
        void Read(T& value);

        auto GetSize() const -> auto { return m_size; }
        auto GetData() const -> void* { return m_buffer; }

    private:
        uint8_t* m_buffer;
        uint64_t m_size;
        uint64_t m_position;
    };

    template<typename T>
    void ReadOnlyByteBuffer::Read(T& value)
    {
        Read(sizeof(T), reinterpret_cast<uint8_t*>(&value));
    }

    template<>
    void ReadOnlyByteBuffer::Read(std::string& value);

    class WriteOnlyByteBuffer
    {
    public:
        WriteOnlyByteBuffer(uint64_t initialCapacity = 1024 * 1024 * 10);
        ~WriteOnlyByteBuffer();

        void SetCapacity(uint64_t newCapacity);
        void SetSize(uint64_t newSize);
        void SetPosition(uint64_t newPosition);

        void Write(uint64_t size, const uint8_t* data);

        template<typename T>
        void Write(const T& value);

        auto GetPosition() const -> auto { return m_position; }
        auto GetCapacity() const -> auto { return m_capacity; }
        auto GetSize() const -> auto { return m_size; }
        auto GetData() const -> uint8_t* { return m_buffer; }

    private:
        uint8_t* m_buffer;
        uint64_t m_capacity;
        uint64_t m_size;

        uint64_t m_position;
    };

    template<typename T>
    void WriteOnlyByteBuffer::Write(const T& value)
    {
        Write(sizeof(T), reinterpret_cast<const uint8_t*>(&value));
    }

    template<>
    void WriteOnlyByteBuffer::Write(const std::string& value);

    struct BinaryStreamable
    {
        virtual ~BinaryStreamable() = default;
        virtual void Read(ReadOnlyByteBuffer& buffer) = 0;
        virtual void Write(WriteOnlyByteBuffer& buffer) const = 0;
    };

}

#endif