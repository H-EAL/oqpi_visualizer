#pragma once

template<typename _BufferImpl>
class buffer_interface
    : public _BufferImpl
{
public:
    // Construct with a buffer and a custom deleter
    template< typename Deleter >
    buffer_interface(uint8_t* pBufferInit, int32_t bufferSize, Deleter deleter)
        : _BufferImpl(pBufferInit, bufferSize, deleter)
    {}

    // Construct with buffer size
    buffer_interface(int32_t bufferSize)
        : _BufferImpl(bufferSize)
    {}

    // Single value
    template<typename T>
    bool read(T &toRead)
    {
        const int32_t sizeInBytes = sizeof(T);
        return _BufferImpl::read((uint8_t*)&toRead, sizeInBytes);
    }

    template<typename T>
    buffer_interface<_BufferImpl>& operator >>(T &toRead)
    {
        read(toRead);
        return (*this);
    }

    template<typename T>
    bool write(const T &toWrite)
    {
        const int32_t sizeInBytes = sizeof(T);
        return _BufferImpl::write((const uint8_t*)&toWrite, sizeInBytes);
    }

    template<typename T>
    buffer_interface<_BufferImpl>& operator <<(const T &toWrite)
    {
        write(toWrite);
        return (*this);
    }

    // Raw Byte array
    bool write(const void *pToWrite, int32_t sizeInBytes)
    {
        return _BufferImpl::write((const uint8_t*)pToWrite, sizeInBytes);
    }

    bool read(void *pToRead, int32_t sizeInBytes)
    {
        return _BufferImpl::read((uint8_t*)pToRead, sizeInBytes);
    }

    // Dynamic array
    template<typename T>
    bool read(T *pToRead, int32_t count)
    {
        const int32_t sizeInBytes = count * sizeof(T);
        return _BufferImpl::read((uint8_t*)pToRead, sizeInBytes);
    }

    template<typename T>
    bool write(const T *pToWrite, int32_t count)
    {
        const int32_t sizeInBytes = count * sizeof(T);
        return _BufferImpl::write((const uint8_t*)pToWrite, sizeInBytes);
    }

    // Static array
    template<typename T, int N>
    bool read(T(&toRead)[N])
    {
        const int32_t sizeInBytes = N * sizeof(T);
        return _BufferImpl::read((uint8_t*)toRead, sizeInBytes);
    }

    template<typename T, int N>
    buffer_interface<_BufferImpl>& operator >>(T(&toRead)[N])
    {
        read(toRead);
        return (*this);
    }

    template<typename T, int N>
    bool write(const T(&toWrite)[N])
    {
        const int32_t sizeInBytes = N * sizeof(T);
        return _BufferImpl::write((const uint8_t*)toWrite, sizeInBytes);
    }

    template<typename T, int N>
    buffer_interface<_BufferImpl>& operator <<(const T(&toWrite)[N])
    {
        write(toWrite);
        return (*this);
    }

    int32_t emptySpace() const
    {
        return _BufferImpl::writableSize();
    }

    int32_t usedSpace() const
    {
        return _BufferImpl::readableSize();
    }
};