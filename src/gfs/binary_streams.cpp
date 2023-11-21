#include "gfs/binary_streams.hpp"

namespace gfs
{
    BinaryStreamRead::BinaryStreamRead(const std::filesystem::path& filename)
    {
        m_stream = std::ifstream(filename, std::ios::binary);
        m_stream.unsetf(std::ios_base::skipws);
    }

    BinaryStreamRead::~BinaryStreamRead()
    {
        Close();
    }

    void BinaryStreamRead::Close()
    {
        m_stream.clear();
    }

    auto BinaryStreamRead::TellG() -> uint64_t
    {
        return uint64_t(m_stream.tellg());
    }

    void BinaryStreamRead::SeekG(uint64_t pos, bool fromEnd)
    {
        m_stream.seekg(pos, fromEnd ? std::ios::end : std::ios::beg);
    }

    void BinaryStreamRead::Read(BinaryStreamable& value)
    {
        value.Read(*this);
    }

    auto BinaryStreamRead::Read(uint64_t size) -> std::vector<uint8_t>
    {
        std::vector<uint8_t> buffer(size);
        m_stream.read(reinterpret_cast<char*>(buffer.data()), size);
        return buffer;
    }

    void BinaryStreamRead::Read(uint64_t size, void* data)
    {
        m_stream.read(reinterpret_cast<char*>(data), size);
    }

    template<>
    void BinaryStreamRead::Read<std::string>(std::string& value)
    {
        uint32_t strSize = 0;
        m_stream.read(reinterpret_cast<char*>(&strSize), sizeof(strSize));
        value.resize(strSize, char(0));
        m_stream.read(value.data(), sizeof(char) * strSize);
    }

    BinaryStreamRead::operator bool() const
    {
        return bool(m_stream);
    }

    BinaryStreamWrite::BinaryStreamWrite(const std::filesystem::path& filename)
    {
        m_stream = std::ofstream(filename, std::ios::binary);
        m_stream.unsetf(std::ios_base::skipws);
    }

    BinaryStreamWrite::~BinaryStreamWrite()
    {
        Close();
    }

    void BinaryStreamWrite::Close()
    {
        m_stream.close();
    }

    auto BinaryStreamWrite::TellP() -> uint64_t
    {
        return uint64_t(m_stream.tellp());
    }

    void BinaryStreamWrite::SeekP(uint64_t pos, bool fromEnd)
    {
        m_stream.seekp(pos, fromEnd ? std::ios::end : std::ios::beg);
    }

    void BinaryStreamWrite::Write(const BinaryStreamable& value)
    {
        value.Write(*this);
    }

    void BinaryStreamWrite::Write(uint64_t size, const void* data)
    {
        m_stream.write(reinterpret_cast<const char*>(data), size);
    }

    template<>
    void BinaryStreamWrite::Write<std::string>(const std::string& value)
    {
        auto strSize = uint32_t(value.size());
        m_stream.write(reinterpret_cast<const char*>(&strSize), sizeof(strSize));
        m_stream.write(value.data(), sizeof(char) * strSize);
    }

    BinaryStreamWrite::operator bool() const
    {
        return bool(m_stream);
    }

}