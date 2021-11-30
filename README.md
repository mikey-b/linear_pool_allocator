# Linear Pool Allocator
The Linear Pool Allocator is an attempt to combine the benefits of:
* a Linear Fixed Size Allocator - A contiguous sequence of fixed sized blocks, with a monotonically incrementing index representing the current available blocks.
* and a Pool Allocator - A contiguous sequence of fixed sized blocks with a linked list embedded into the free blocks, and a head pointer that holds the currently available blocks.

The shortcomings of a Linear Allocator is the lack of ability to deallocate and reuse blocks.
The shortcomings of a Pool Allocator is the initial initialisation of the linked list of free blocks, and the allocation cost is higher when compared to a Linear Allocator.

The Linear Pool Allocator works by treating all first allocations of a block as a Linear allocation. When a block is deallocated, that block is then added to the head of the pool allocator side of the allocator. This means there is no upfront construction of the pool allocators linked list, and first time allocations are always done via the faster linear allocation.

## Algorithm
struct slot_node - A structure of a free slot for the pool allocator

block_t pool[pool_size] - An array of blocks, uninitialised
block_t* head{ &pool[pool_size] } - Head is a pointer to the last element of the pool (This is a decrementing linear allocator).
block_t** tail{ &head } - This is a pointer to either the head variable, or a ->next variable within a free slot node.

void* allocate() / void dellocate(void*)
* **Linear Mode**
If tail is equal to the address of head - We know that the pool allocator has no free slots in the linked list, and we use the linear allocation method. In this mode, the value of head is the linear allocation monotonic index.

* **Pool Mode**
When a block is deallocated, We perform the normal pool allocation steps. The monotonic index value is set into the first free slot's ->next value. The value of tail is then set to the address of ->next. Further deallocations does not move this value as the linked list is First In, Last Out.

When in Pool Mode, an allocation will default to the Pool allocation strategy. The first free slot is selected, head is set to the value of ->next, and that slot is returned. If tail was pointing to that slots ->next variable, tail is set to head - returning to Linear Mode.

void* allocate_linear()
* **Forced Linear Mode**
The monotonic index value is always available via the tail pointer. This means you can override the use of the pool allocation strategy and always perform a linear allocation. This can potentially increase fragmentation, but provide guarentees on the performance and guarntee linear memory is returned.
 
## Use Case
The primary designed use case is to be used allocating tree structures, where the majority of nodes are created at initial construction and transformations are performed on that structure. E.g. Reading in a file and constructing a tree from that data. This allocator leverages the speed of a linear allocator when the bulk of the nodes are created, and still allows the possibility of deallocations when manipulating the tree.

This allocator is designed to be composed with other allocator compontents (Segreator, Bucketizer etc) that manage multiple allocators to enable expanding, condensing, and policies of allocations.

## Benchmark
The benchmark is X speed up compared against my standard Pool Allocator. This is by no means a thorough benchmark - My Pool Allocator that I compare against might not be the fastest. 

The Y axis is the number of blocks the allocator manages - All blocks of 8 bytes in size. 

This benchmark shows the allocation and deallocation of all nodes in sequence. An additional "wipe" deallocation method is also supplied to deallocate all nodes at once.

![Benchmark](https://github.com/mikey-b/lib/blob/main/benchmark-27.11.21.png?raw=true "Benchmark")

### Results

The default strategy of Linear Pool Allocator is slower than the default Pool Allocator (red vs blue bars), until pool size is > 32k. While I would like to get this smaller, I am not surprised by this. I also suspect the cache is helping the standard pool allocator here - the small node sizes and allocation style of the benchmark is most likely showing the worst case difference.

The benefit of this allocator is to be able to utilise the linear allocator behaviour when a tree is initially being constructed - when most allocations occur.

Using the linear allocation strategy enables a 1.5x - 2x speed up over a pool allocator. This is most likely slower than a standard linear allocator (not measured here) - But provides the possibility to deallocate and reuse.

## Next
I have no interest in writing or maintaining a library. By all means, if you see value in this, you are welcome to it. 