#pragma once

template<size_t slot_size, int pool_size = 1024*4> struct linear_pool_allocator {
 private:
    struct slot_node {
        char* next;
    };
    static_assert(sizeof(slot_node) <= slot_size);

    char pool[slot_size * pool_size];

    char* head{ &pool[slot_size * pool_size] };
    char** tail{ &head };

 public:
    void* allocate_linear() {
        if (*tail == &pool[0]) return nullptr;
        *tail -= slot_size;
        return reinterpret_cast<void*>(*tail);
    }

    void* allocate() {
		if (head == &pool[0]) return nullptr;
		
		if (tail == &head) {
            head -= slot_size;
            return reinterpret_cast<void*>(head); 
		} else {
			auto node = reinterpret_cast<slot_node*>(head);
            if (tail == &node->next) tail = &head;
			head = node->next;
            return reinterpret_cast<void*>(node);
        }

    [[gnu::nonnull]] void deallocate(void* blk) {
        if (*tail == reinterpret_cast<char*>(blk)) {
            *tail += slot_size;
        } else {
            auto node = reinterpret_cast<slot_node*>(blk);
            node->next = head;
            if (tail == &head) tail = &node->next;
            head = reinterpret_cast<char*>(blk);
        }
    }

    void deallocateAll() {
        head = &pool[slot_size * pool_size];
        tail = &head;
    }
};
