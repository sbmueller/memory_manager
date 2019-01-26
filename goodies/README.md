# Memory manager

Programming task for COMPANY's interview process. September 2018.

Candidate: Sebastian Müller

Date: 24. September 2018

## Solution
### Class `memoryManagerManager`
The API is precicely defined by the synopsis. Since in
`create_memory_manager()` a handle to a Memory manager should be returned, It
is sensible to create a class to take care of all the functionality. I decided
to implement the class `memoryManagerManager`. The API calls just determine
which instance of `memoryManagerManager` to use (in case there are multiple
options) and pass the remaining arguments to the public class methods.

To track allocation and freeing actions during runtime, a data structure has to
be used to store the information about free and allocated memory. I chose to use
a `std::map<valloc_t, size_t> d_free_memory` for that. It depicts the *free*
memory regions with key start address and value length. This data structure has
several advantages:

- Access complexity of O(1)
- Search complexity of O(log n) since internally BST
- Adding/removing entries complexity O(1)
- Keys are unique, exactly like free memory region start addresses
- Order of keys always garanteed, similar to physical memory addresses which
  they represent
- Search and comparison between keys is easy and efficient with methods
  `find()`, `upper_bound()` and `lower_bound()`

I chose to track the free memory instead of occupied memory because search for
free memory is easier that way. I assume over the runtime, more logical
allocations than deallocations will be performed, leading me to prioritize
efficiency for allocation (= search for free memory) actions.

### Interesting Methods

#### Constructor
The constructor of `memoryManagerManager` just stores the three properties
`base`, `length` and `granularity` in private class members. `d_free_memory` is
initialized to single entry `<base, length>` (there is one free region which
takes up the whole physical space).

#### Destructor
Since no additional allocations are performed, the deconstructor is left empty.

#### `create_allocation()`
This is the class method which gets invoked by API function `allocate_range()`.
First, the `length` parameter is rounded up to the granularity set in the class
member. This ensures to have allocation lengths and start/end addresses that
always fit the granularity grid.

Next, some basic error handling is performed, checking if `optional_hint` lies
outside the physical memory.

Finally, the allocation type is checked and private methods are called for each
of the four allocation types.

#### `remove_allocation()`
This method gets invoked by the API method `free_range()`.  In comparison to
allocation, this method is a bit more complex because of the underlying
data structure of the logical memory management.

First, the next free memory greater than the base address to be freed is
searched using `upper_bound()`. If that free memory is part of the space that is
desired to be freed, a warning is thrown since it was not allocated beforehand.

If the start address of the free memory is adjacent to the space to be freed,
some variables are set to be able to join free memory regions later. Next, the
free memory region below the allocation to be freed is investigated. Now, four
cases can happen:

1. Both memories, below and above, are adjacent and need to be joined
2. Only memory below is adjacent
3. Only memory above is adjacent
4. None of the above

Depending on what is true, free memory entries will be created that equal the
representation with the lowest number of "free memory" entries in the map.

#### `allocate_any()`
If `allocate_range()` was called with `ALLOCATE_ANY` flag, this method gets
invoked. It iterates over the free memory map and tries to find a free memory
section which is long enough to hold the desired allocation. If one is found,
it's base address is returned and the free memory map is updated.

#### `allocate_exact()`
If `allocate_range()` was called with `ALLOCATE_EXACT` flag, this method gets
invoked. Initially, the first free memory region which is above `optional_hint`
is found with `upper_bound()`. Then it is checked wether there is any free
memory below that. If this is the case, the next free memory below is searched,
which must be less equal `optional_hint`. Finally, it is checked wether
`optional_hint` and the desired `length` can be allocated in that free space. If
yes, `optional_hint` is returned and the free memory map is updated.

#### `allocate_above()`
This method covers the flag `ALLOCATE_ABOVE`. A reverse iterator is used to move
backwards through the map, beginning at the very end, down to the last free
memory which could hold the desired allocation if placed at the very end of the
free memory region. If a free memory region is found which is capable
of holding the desired allocation, it is placed at the very end of it.

#### `allocate_below()`
This method covers the flag `ALLOCATE_BELOW`. An iterator is used to move
forward through te map up to the last free memory which could hold the whole
allocation below `optional_hint`. If a matching free memory region is found, the
allocation is placed at it's very beginning.
