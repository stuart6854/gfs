#include "gfs/binary_streams.hpp"

#include <cstring>

namespace gfs
{
    static auto NextPowerOf2(uint64_t value) -> uint64_t
    {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value |= value >> 32;
        value++;
        return value;
    }

    ReadOnlyByteBuffer::ReadOnlyByteBuffer(uint64_t size)
        : m_buffer(new uint8_t[size]),
        m_size(size),
        m_position(0)
    {
    }

    ReadOnlyByteBuffer::~ReadOnlyByteBuffer()
    {
        delete[] m_buffer;
    }

    void ReadOnlyByteBuffer::Read(uint64_t size, uint8_t* data)
    {
        std::memcpy(data, m_buffer + m_position, size);
        m_position += size;
    }

    template<>
    void ReadOnlyByteBuffer::Read(std::string& value)
    {
        uint64_t strLen = 0;
        Read(strLen);
        value.resize(strLen, char(0));
        Read(sizeof(std::string::value_type) * strLen, reinterpret_cast<uint8_t*>(value.data()));
    }

    WriteOnlyByteBuffer::WriteOnlyByteBuffer(uint64_t initialCapacity)
        : m_buffer(new uint8_t[initialCapacity]),
        m_capacity(initialCapacity),
        m_size(0),
        m_position(0)
    {
    }

    WriteOnlyByteBuffer::~WriteOnlyByteBuffer()
    {
        delete[] m_buffer;
    }

    void WriteOnlyByteBuffer::SetCapacity(uint64_t newCapacity)
    {
        if (newCapacity <= m_capacity)
            return;

        uint8_t* newBuffer = new uint8_t[newCapacity];
        std::memcpy(newBuffer, m_buffer, m_size);
        delete[] m_buffer;

        m_buffer = newBuffer;
        m_capacity = newCapacity;
    }

    void WriteOnlyByteBuffer::SetSize(uint64_t newSize)
    {
        if (newSize > m_capacity)
            SetCapacity(newSize);

        m_size = newSize;
        if (m_position > m_size)
            m_position = m_size;
    }

    void WriteOnlyByteBuffer::SetPosition(uint64_t pos)
    {
        m_position = pos;
    }

    void WriteOnlyByteBuffer::Write(uint64_t size, const uint8_t* data)
    {
        if (m_position + size > m_capacity)
            SetCapacity(NextPowerOf2(m_capacity));

        std::memcpy(&m_buffer[m_position], data, size);

        m_position += size;
        if (m_position > m_size)
            m_size = m_position;
    }

    template<>
    void WriteOnlyByteBuffer::Write(const std::string& value)
    {
        uint64_t strLen = value.size();
        Write(strLen);
        Write(sizeof(std::string::value_type) * strLen, reinterpret_cast<const uint8_t*>(value.data()));
    }

}