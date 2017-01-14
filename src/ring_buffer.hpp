#pragma once

#include <atomic>
#include <memory>
#include <cassert>
#include "buffer_interface.hpp"


// We use buffer_interface to provide some helpful generic accessors
using ring_buffer = buffer_interface<class ring_buffer_impl>;

// Thread safe single consumer, single producer, non overwriting ring buffer
class ring_buffer_impl
{
    static void default_array_delete(uint8_t* ptr)
    {
        std::default_delete<uint8_t[]> defaultDeleter;
        defaultDeleter(ptr);
    }

    // Small structure to keep track of the current index at which we can operate (read or write)
    // and the number of time we looped over the buffer.
    struct cursor_t
    {
        cursor_t(int32_t i = 0, int32_t lf = 0) : index(i), loopFlag(lf) {}
        int32_t index;
        int32_t loopFlag;
    };

    // Magic number to indicate that the data doesn't need to be sliced
    enum { DO_NOT_SLICE = -1 };

protected:
    ring_buffer_impl(int32_t bufferSize)
        : buffer_(new uint8_t[bufferSize], &ring_buffer_impl::default_array_delete)
        , bufferSize_(bufferSize)
        , readCursor_(cursor_t(0, 0))
        , writeCursor_(cursor_t(0, 0))
    {}

    template<typename _Deleter>
    ring_buffer_impl(uint8_t* pBufferInit, int32_t bufferSize, _Deleter deleter)
        : buffer_(pBufferInit, deleter)
        , bufferSize_(bufferSize)
        , readCursor_(cursor_t(0, 0))
        , writeCursor_(cursor_t(0, 0))
    {}

protected:
    bool read(uint8_t *dst, int32_t size)
    {
        assert(dst != nullptr);
        assert(size > 0);

        // Check if we have enough space to read the requested size
        if (size <= readableSize())
        {
            // Does the available data loop back to the beginning of the buffer
            const auto sliceAt = sliceRead(size);

            // Determine whether we have to do 2 reads or not
            const auto firstSliceSize = (sliceAt == DO_NOT_SLICE) ? size : sliceAt;
            // In any case the first part always starts at readIndex()
            std::memcpy(dst, buffer_.get() + readIndex(), firstSliceSize);

            if (sliceAt != DO_NOT_SLICE)
            {
                // Read the last part, it's always located at the head of the buffer
                const auto secondSliceSize = size - sliceAt;
                std::memcpy(dst + sliceAt, buffer_.get(), secondSliceSize);
            }

            advanceReadIndex(size);
            return true;
        }

        return false;
    }

    bool write(const uint8_t *src, int32_t size)
    {
        assert(src != nullptr);
        assert(size > 0);

        if (size <= writableSize())
        {
            const auto sliceAt = sliceWrite(size);

            const auto firstSliceSize = (sliceAt == DO_NOT_SLICE) ? size : sliceAt;
            std::memcpy(buffer_.get() + writeIndex(), src, firstSliceSize);

            if (sliceAt != DO_NOT_SLICE)
            {
                const auto secondSliceSize = size - sliceAt;
                std::memcpy(buffer_.get(), src + sliceAt, secondSliceSize);
            }

            advanceWriteIndex(size);
            return true;
        }

        return false;
    }

protected:
    int32_t readIndex() const
    {
        return readCursor_.load(std::memory_order_relaxed).index;
    }

    int32_t writeIndex() const
    {
        return writeCursor_.load(std::memory_order_relaxed).index;
    }

    int32_t readableSize() const
    {
        const auto r = readCursor_.load(std::memory_order_relaxed);
        const auto w = writeCursor_.load(std::memory_order_relaxed);

        const auto isFull = (r.index == w.index) && (r.loopFlag != w.loopFlag);
        const auto normalizedWriteIndex = ((w.index < r.index) || isFull) ? (bufferSize_ + w.index) : w.index;

        return normalizedWriteIndex - r.index;
    }

    int32_t writableSize() const
    {
        return bufferSize_ - readableSize();
    }

    int32_t sliceRead(int32_t size) const
    {
        return ((readIndex() + size) > bufferSize_) ? bufferSize_ - readIndex() : DO_NOT_SLICE;
    }

    int32_t sliceWrite(int32_t size) const
    {
        return ((writeIndex() + size) > bufferSize_) ? bufferSize_ - writeIndex() : DO_NOT_SLICE;
    }

    void advanceReadIndex(int32_t size)
    {
        const auto r = readCursor_.load(std::memory_order_relaxed);
        const auto newReadIndex = (r.index + size) % bufferSize_;
        const auto newReadLoopFlag = (r.index + size) >= bufferSize_ ? (r.loopFlag ^ 1) : r.loopFlag;

        readCursor_.store(cursor_t(newReadIndex, newReadLoopFlag), std::memory_order_relaxed);
    }

    void advanceWriteIndex(int32_t size)
    {
        const auto w = writeCursor_.load(std::memory_order_relaxed);
        const auto newWriteIndex = (w.index + size) % bufferSize_;
        const auto newWriteLoopFlag = (w.index + size) >= bufferSize_ ? (w.loopFlag ^ 1) : w.loopFlag;

        writeCursor_.store(cursor_t(newWriteIndex, newWriteLoopFlag), std::memory_order_relaxed);
    }

private:
    std::atomic<cursor_t>                           readCursor_;
    std::atomic<cursor_t>                           writeCursor_;
    std::unique_ptr<uint8_t[], void(*)(uint8_t*)>   buffer_;
    const int32_t                                   bufferSize_;
};