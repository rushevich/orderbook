#pragma once
#include <atomic>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace rushevich {

// A fixed-size, lock-free single-producer single-consumer queue implemented as a ring buffer
// Inspired by rigtorp’s SPSC queue implementation (but still written by me)

template <std::destructible T, typename Allocator = std::allocator<T>> class SPSCQueue {

public:
    // We employ the technique of having one dummy element that distinguishes the
    explicit SPSCQueue(size_t capacity) : _capacity { capacity } {
        assert(_capacity > 0 && "SPSC QUEUE capacity must be > 0");
        _capacity++;
        if (_capacity > std::numeric_limits<size_t>::max() - 2 * PADDING) {
            _capacity = std::numeric_limits<size_t>::max() - 2 * PADDING;
        }
#ifdef __cpp_lib_allocate_at_least
        const auto [memory, allocatedCapacity]
            = ATraits::allocate_at_least(_allocator, _capacity + 2 * PADDING);
        _capacity = allocatedCapacity - 2 * PADDING;
        _buf = memory;

#else
        _buf = ATraits::allocate(_allocator, _capacity + 2 * PADDING);
#endif
    }

    [[nodiscard]] __attribute__((always_inline)) size_t advance(size_t index) const noexcept {
        return (++index == _capacity) ? 0 : index;
    }

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    // The functions that write to the queue only gets called from the producer thread.
    // It follows that the producer thread is the only one that is mutating the _writeIdx
    // So we can always load the _writeIdx using relaxed memory ordering, as these operations
    // are implicitly sequenced before one-another by design
    // This is of course provided that the roles of producer and consumer stay fixed

    // emplace blocks when the queue is full
    template <typename... Args>
    void emplace(Args&&... args) noexcept(std::is_nothrow_move_constructible_v<T>
                                          && std::is_nothrow_constructible_v<T, Args&&...>) {
        const auto writeIdx = _writeIdx.load(relaxed);
        const auto nextWriteIdx = advance(writeIdx);
        // Spin / block until the reader pops one off
        while (_readIdxCache == nextWriteIdx) {
            _readIdxCache = _readIdx.load(acquire);
        }
        // Once this loop exits, we know the _readIdx has progressed one forth, and we now have
        // space to write another object
        ATraits::construct(_allocator, _buf + writeIdx + PADDING, std::forward<Args>(args)...);

        // Use memory order release to ensure a synchronizes-with relationship with the reader
        // thread that acquires all changes to the _writeIdx
        _writeIdx.store(nextWriteIdx, release);
    }

    [[nodiscard]] T* front() noexcept {
        const auto readIdx = _readIdx.load(relaxed);
        if (readIdx == _writeIdxCache) {
            _writeIdxCache = _writeIdx.load(acquire);
            if (readIdx == _writeIdxCache) {
                return nullptr;
            }
        }

        return &_buf[readIdx + PADDING];
    }

    // Calling pop() with an invalid (nullptr) result from front() prior results in undefined
    // behavior... don’t do it
    // This behavior is a-la std::vector’s pop_back() invoking UB when the container is empty
    void pop() noexcept {
        const auto readIdx = _readIdx.load(relaxed);
        // If readIdx == writeIdx , the queue is currently empty, so this spins until some object
        // gets placed on the queue
        ATraits::destroy(_allocator, _buf + readIdx + PADDING);
        const auto nextReadIdx = advance(readIdx);
        _readIdx.store(nextReadIdx, release);
    }

    // This function implies absolutely no inter-thread synchronization and only hinges on
    // destruction occurring on one thread
    void unsynchronized_pop() noexcept {
        const auto readIdx = _readIdx.load(relaxed);
        ATraits::destroy(_allocator, _buf + readIdx + PADDING);
        const auto nextReadIdx = advance(readIdx);
        _readIdx.store(nextReadIdx, relaxed);
    }

    ~SPSCQueue() noexcept {
        while (front() != nullptr) {
            unsynchronized_pop();
        }
        ATraits::deallocate(_allocator, _buf, _capacity + 2 * PADDING);
    }

private:
    using ATraits = std::allocator_traits<Allocator>;
    static constexpr size_t cacheLineSize = std::hardware_destructive_interference_size;
    static constexpr size_t PADDING = (cacheLineSize - 1) / sizeof(T) + 1;
    T* _buf { nullptr };
    [[no_unique_address]] Allocator _allocator;
    size_t _capacity {};

    alignas(cacheLineSize) std::atomic<size_t> _readIdx = 0UZ;
    alignas(cacheLineSize) std::atomic<size_t> _writeIdx = 0UZ;
    alignas(cacheLineSize) size_t _readIdxCache { 0UZ };
    alignas(cacheLineSize) size_t _writeIdxCache { 0UZ };

    // for some sizeof(T) = sz, the minimum number of elements for the buffer to contain such that
    // it occupies a cache line solely is:
    // cache_line_size / sz -> this has to be rounded up in the case of integer division causing
    // truncation, so we do: (cache_line_size - 1) / sz + 1
    static constexpr auto relaxed = std::memory_order_relaxed;
    static constexpr auto acquire = std::memory_order_acquire;
    static constexpr auto release = std::memory_order_release;
};
} // namespace rushevich
