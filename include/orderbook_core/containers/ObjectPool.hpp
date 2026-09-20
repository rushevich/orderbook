#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>
namespace rushevich::containers {

// The object pool allocated a bunch of memory at its creation and then dispenses indices to free
// blocks. Also, since we are allocating just once, it does not really matter which allocator we
// use. For simplicity’s sake, we will use std::allocator<T>, but also permit the user to pass in
// their own allocator
template <typename T, size_t Capacity, typename Allocator = std::allocator<T>>
    requires std::is_trivially_destructible_v<T>
class ObjectPool {
public:
    ObjectPool(Allocator allocator = Allocator {})
        : _remaining { Capacity },
          _T_Allocator { std::move(allocator) } {
        static_assert(Capacity > 0);
        _pool = Node_ATraits::allocate(_Node_Allocator, Capacity);
        // The free-list starts empty, this saves us having to go through and construct + chain 6M
        // objects on construction
    }

    // Returns a pointer to the object created using the supplied ’Args&&... args’, or nullptr if
    // the pool is out of slots
    template <typename... Args> [[nodiscard]] T* allocate(Args&&... args)

    {
        T* allocation { nullptr };
        if (_freeHead != nullptr) {
            allocation = reinterpret_cast<T*>(_freeHead);
            _freeHead = _freeHead->nextFree;
            T_ATraits::construct(_T_Allocator, allocation, std::forward<Args>(args)...);
        } else if (_bumpIndex < Capacity) {
            allocation = reinterpret_cast<T*>(_pool + _bumpIndex++);
            T_ATraits::construct(_T_Allocator, allocation, std::forward<Args>(args)...);
        } else {
            return nullptr;
        }
        _remaining--;
        return allocation;
    }

    // ’ptr’ is invalidated (set to nullptr)
    void free(T*& ptr) {
        assert(ptr != nullptr);
        T_ATraits::destroy(_T_Allocator, ptr);
        Node* node = reinterpret_cast<Node*>(ptr);
        node->nextFree = _freeHead;
        _freeHead = node;
        ptr = nullptr;
        _remaining++;
    }

    [[nodiscard]] size_t remaining() const { return _remaining; }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    ~ObjectPool() noexcept {
        assert(_pool != nullptr);
        Node_ATraits::deallocate(_Node_Allocator, _pool, Capacity);
    }

private:
    union Node {
        alignas(T) std::byte storage[sizeof(T)];
        Node* nextFree;
    };
    Node* _pool { nullptr };
    Node* _freeHead { nullptr };
    size_t _bumpIndex { 0 };
    size_t _remaining { 0 };
    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using Node_ATraits = std::allocator_traits<NodeAllocator>;
    using T_ATraits = std::allocator_traits<Allocator>;
    [[no_unique_address]] Allocator _T_Allocator;
    [[no_unique_address]] NodeAllocator _Node_Allocator;
};
} // namespace rushevich::containers
