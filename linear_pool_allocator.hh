#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>

template<std::size_t slot_size, std::size_t slot_alignment = alignof(std::max_align_t), int pool_size = 1024> 
struct linear_pool_allocator {
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

	block_t* head{ nullptr }; 
	block_t* frontier{ &pool[pool_size] }; // This is correct! Always starts as a linear allocator and to the RIGHT of the last element
	block_t pool[pool_size];

 public:
	[[nodiscard]] void* allocate_linear() {
        if (frontier == &pool[0]) return nullptr;
        frontier -= 1;
        return static_cast<void*>(frontier);
	}

	[[nodiscard]] void* allocate() {
        if (head != nullptr) {
			auto blk = head;
			auto node = std::launder( reinterpret_cast<slot_node*>(blk->storage) );
			head = node->next;
			return static_cast<void*>(blk);
		}

        return allocate_linear();
	}

	void deallocate(void* _blk) {
		auto blk = static_cast<block_t*>(_blk);
		
		if (blk == frontier) { 
            // _blk is top of linear stack, pop to free.
			frontier += 1;
		} else { 
            // blk will be made part of the freelist
			::new (static_cast<void*>(blk->storage)) slot_node{head};
			head = blk;
		}
	}

	void deallocateAll() {
		frontier = &pool[pool_size];
		head = nullptr;
	}
	
	linear_pool_allocator() = default;
	
	linear_pool_allocator(linear_pool_allocator const&) = delete;
	linear_pool_allocator& operator=(linear_pool_allocator const&) = delete;
	
	linear_pool_allocator(linear_pool_allocator&&) = delete;
	linear_pool_allocator& operator=(linear_pool_allocator&&) = delete;
};
