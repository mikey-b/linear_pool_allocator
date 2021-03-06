#pragma once
#include <type_traits>
#include <cstddef>

template<std::size_t slot_size, std::size_t slot_alignment = alignof(std::max_align_t), int pool_size = 1024*64> struct linear_pool_allocator {
 private:
	using block_t = typename std::aligned_storage<slot_size, slot_alignment>::type;

	struct slot_node {
		slot_node* next;
	};
	static_assert(sizeof(slot_node) <= sizeof(block_t));

	block_t* head{ &pool[pool_size] }; // This is correct! Always starts as a linear allocator and to the RIGHT of the last element
	block_t** tail{ &head };
	block_t pool[pool_size];

 public:
	template<bool isInitialConstruction = false>
	[[gnu::malloc]] void* allocate_linear() {
		if constexpr (isInitialConstruction) {
			// We can remove the indirection if we can guarentee that there has never
			// been a dealloation yet. This is now identical to a bump allocator.
			// assert(*tail == head);
			if (head == &pool[0]) return nullptr;
			head -= 1;
			return reinterpret_cast<void*>(head);
		} else {
			if (*tail == &pool[0]) return nullptr;
			*tail -= 1;
			return reinterpret_cast<void*>(*tail);
		}
	}

	[[gnu::malloc]] void* allocate() {
		if (head == &pool[0]) return nullptr;

		if (tail == &head) {
			head -= 1;
			return reinterpret_cast<void*>(head); 
		} else {
			auto node = reinterpret_cast<slot_node*>(head);
			if (reinterpret_cast<slot_node**>(tail) == &node->next) {
				tail = &head;
			}
			head = reinterpret_cast<block_t*>(node->next);
			return reinterpret_cast<void*>(node);                
		}
	}

	[[gnu::nonnull]] void deallocate(void* blk) {
		if (*tail == reinterpret_cast<block_t*>(blk)) {
			*tail += 1;
		} else {
			auto node = reinterpret_cast<slot_node*>(blk);
			node->next = reinterpret_cast<slot_node*>(head);
			if (tail == &head) tail = reinterpret_cast<block_t**>(&node->next);
			head = reinterpret_cast<block_t*>(blk);
		}
	}

	void deallocateAll() {
		head = &pool[pool_size]; // This is correct! Always starts as a linear allocator and to the RIGHT of the last element
		tail = &head;
	}
};