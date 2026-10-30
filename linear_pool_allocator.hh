#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>

template<std::size_t slot_size, std::size_t slot_alignment = alignof(std::max_align_t), int pool_size = 1024> struct linear_pool_allocator {
 private:

    struct alignas(slot_alignment) block_t {
        unsigned char storage[slot_size];
    };

	struct slot_node {
		block_t* next;
	};

	static_assert(slot_size >= sizeof(slot_node), "Slot size is too small to hold a free-list node pointer.");
	static_assert(alignof(block_t) >= alignof(slot_node), "Slot alignment is insufficient for a free-list node.");
    static_assert(pool_size > 0, "Pool size must be greater than zero.");

	block_t* head{ &pool[pool_size] }; // This is correct! Always starts as a linear allocator and to the RIGHT of the last element
	block_t** tail{ &head };
	block_t pool[pool_size];

 public:
	template<bool isInitialConstruction = false>
	[[nodiscard]] void* allocate_linear() {
		if constexpr (isInitialConstruction) {
			// We can remove the indirection if we can guarentee that the freelist
			// is empty. This is now identical to a bump allocator.
			assert(tail == &head);
			if (head == &pool[0]) return nullptr;
			head -= 1;
			return static_cast<void*>(head);
		} else {
			if (*tail == &pool[0]) return nullptr;
			*tail -= 1;
			return static_cast<void*>(*tail);
		}
	}

	[[nodiscard]] void* allocate() {
		if (head == &pool[0]) return nullptr;

		if (tail == &head) {
			head -= 1;
			return static_cast<void*>(head); 
		} else {
			auto node = std::launder( reinterpret_cast<slot_node*>(head->storage) );
			// This is the last node on the freelist
			// point tail to head to signal freelist is empty
			tail = (tail == &node->next) ? &head : tail;
			head = node->next;
			return static_cast<void*>(node);
		}
	}

	void deallocate([[gnu::nonnull]] void* _blk) {
		auto blk = static_cast<block_t*>(_blk);
		
		if (*tail == blk) {
			// _blk is top of linear stack, pop to free.
			*tail += 1;
		} else { 
			// blk will be made part of the freelist
			auto node = ::new (static_cast<void*>(blk->storage)) slot_node{head};
			// Is this the first node on the freelist?
			// If yes, set tail to the address pointing to frontier
			// and signal freelist exists
			tail = (tail == &head) ? &node->next : tail;
			head = blk;
		}
	}

	void deallocateAll() {
		head = &pool[pool_size]; // This is correct! Always starts as a linear allocator and to the RIGHT of the last element
		tail = &head;
	}
	
	linear_pool_allocator() = default;
	
	linear_pool_allocator(linear_pool_allocator const&) = delete;
	linear_pool_allocator& operator=(linear_pool_allocator const&) = delete;
	
	linear_pool_allocator(linear_pool_allocator&&) = delete;
	linear_pool_allocator& operator=(linear_pool_allocator&&) = delete;
};
