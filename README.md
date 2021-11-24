##Linear Pool Allocator
The linear pool allocator is an attempt to combine the benefits of:
* a Linear Fixed Size Allocator - 
* and a Pool Allocator
###Use Case
The primary designed use case was to be used allocating tree structures, where the majority of nodes are created at initial construction.