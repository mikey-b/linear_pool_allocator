#pragma once
#include <type_traits>
#include <cstddef>

template<std::size_t slot_size, std::size_t slot_alignment = alignof(std::max_align_t), int pool_size = 1024*64> struct linear_pool_allocator {
 private:
    using bucket_t = typename std::aligned_storage<slot_size, slot_alignment>::type;

    struct slot_node {
        slot_node* next;
    };
    static_assert(sizeof(slot_node) <= sizeof(bucket_t));

    bucket_t* head{ &pool[pool_size] }; // This is correct! Always starts as a linear allocator and to the RIGHT of the last element
    bucket_t** tail{ &head };
    bucket_t pool[pool_size];

 public:
    [[gnu::malloc]] void* allocate_linear() {
        if (*tail == &pool[0]) return nullptr;
        *tail -= 1;
        return reinterpret_cast<void*>(*tail);
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
			head = reinterpret_cast<bucket_t*>(node->next);
			return reinterpret_cast<void*>(node);                
        }
    }

    [[gnu::nonnull]] void deallocate(void* blk) {
        if (*tail == reinterpret_cast<bucket_t*>(blk)) {
            *tail += 1;
        } else {
            auto node = reinterpret_cast<slot_node*>(blk);
            node->next = reinterpret_cast<slot_node*>(head);
            if (tail == &head) tail = reinterpret_cast<bucket_t**>(&node->next);
            head = reinterpret_cast<bucket_t*>(blk);
        }
    }

    void deallocateAll() {
        head = &pool[pool_size]; // This is correct! Always starts as a linear allocator and to the RIGHT of the last element
        tail = &head;
    }
};