/* memoryManager.cpp
 *
 * This file is part of the project "Memory manager", which was created in
 * context of an recuritment process in September 2018.
 *
 * @author Sebastian Müller <senpo@posteo.de>
 * @date 24-09-2018
 */

#include <iostream>
#include <cstddef>
#include <map>

// defined by synopsis

typedef void *ralloc_t;
typedef uintptr_t vaddr_t;
typedef enum {
  ALLOCATE_ANY,
  ALLOCATE_EXACT,
  ALLOCATE_ABOVE,
  ALLOCATE_BELOW
} allocation_flags;

// manager class

class memoryManagerManager {
  /* memoryManagerManager class
   *
   * Logical memory management class to manage a physically allocated memory
   * range.
   */

  public:
    memoryManagerManager(vaddr_t base, size_t length, size_t granularity) {
      d_base = base;
      d_total_length = length;
      d_granularity = granularity;
      // set whole memory region to free
      d_free_memory[base] = length;
    }

    ~memoryManagerManager() {}

    vaddr_t create_allocation(size_t length, allocation_flags flags,
        vaddr_t optional_hint) {
      // function variables
      vaddr_t new_base = -1;
      // round length to fit granularity
      length = round_length(length);

      // error handling
      if(flags != ALLOCATE_ANY && (optional_hint < d_base ||
          optional_hint >= (d_base + d_total_length))) {
        std::cerr << "WARNING: Address provided in optional_hint outside of "
            "allocated physical memory! No allocation performed." << std::endl;
        return new_base;
      }

      // interpret flags
      switch(flags) {
        case ALLOCATE_ANY:
          new_base = allocate_any(length);
          break;
        case ALLOCATE_EXACT:
          // error handling if granularity does not match
          if((optional_hint - d_base) % d_granularity != 0) {
            std::cerr << "WARNING: Address provided in optional_hint does not "
                "match granularity pattern! No allocation performed." <<
                std::endl;
          } else {
            new_base = allocate_exact(length, optional_hint);
          }
          break;
        case ALLOCATE_ABOVE:
          new_base = allocate_above(length, optional_hint);
          break;
        case ALLOCATE_BELOW:
          new_base = allocate_below(length, optional_hint);
          break;
      }
      return new_base;
    }

    void remove_allocation(vaddr_t base, size_t length) {
      // error handling
      if(base < d_base || base + length > d_base + d_total_length) {
        std::cerr << "WARNING: Attempted to free allocation outside of "
          "physical memory. No allocation removed." << std::endl;
        return;
      }
      // check base and length for granularity grid
      if(length % d_granularity != 0) {
        std::cerr << "WARNING: Length to be freed does not mach granularity "
          "grid. No allocation removed." << std::endl;
        return;
      }
      if((base - d_base) % d_granularity != 0) {
        std::cerr << "WARNING: Base address to be freed does not match "
          "granularity grid. No allocation removed." << std::endl;
        return;
      }
      // variables if freed memory can be combined with succeeding memory
      bool touch_back = false;
      size_t space_behind = 0;
      vaddr_t addr_behind;
      std::map<vaddr_t, size_t>::iterator it;

      // search for first free space which is above base
      it = d_free_memory.upper_bound(base);
      // error handling
      if(it != d_free_memory.end() && it->first < base + length) {
        // this space is not allocated
        std::cerr << "WARNING: Desired memory region to free is not entirely "
          "allocated! No freeing has been performed." << std::endl;
        return;
      }
      // check if merge with free space above is needed
      if(it != d_free_memory.end() && it->first == base + length) {
        touch_back = true;
        addr_behind = it->first;
        space_behind = it->second;
      }

      // check if allocated space is adjacent to free memory below
      if(it != d_free_memory.begin() &&
          std::prev(it)->first + std::prev(it)->second == base) {
        // examine preceeding free memory section
        it = std::prev(it);
        // expand preceeding free memory
        it->second += length;
        if(touch_back) {
          // expand both preceeding and succeeding free memory
          d_free_memory.erase(addr_behind);
          it->second += space_behind;
        }
      } else {
        if(touch_back) {
          // just expand succeeding free memory
          d_free_memory.erase(addr_behind);
          d_free_memory[base] = length + space_behind;
        } else {
          // no adjacent free memory -> create new entry
          d_free_memory[base] = length;
        }
      }
    }

    // memory map getter
    std::map<vaddr_t, size_t> get_free_memory() {
      return memoryManagerManager::d_free_memory;
    }

  private:
    vaddr_t d_base;
    size_t d_total_length;
    size_t d_granularity;
    // map to track free memory
    std::map<vaddr_t, size_t> d_free_memory;

    size_t round_length(size_t length) {
      // round length up to next granularity
      return ((length + d_granularity - 1) / d_granularity) * d_granularity;
    }

    vaddr_t round_base(vaddr_t base) {
      return ((base - d_base) / d_granularity) * d_granularity + d_base;
    }

    vaddr_t allocate_any(size_t length) {
      vaddr_t new_base = -1;
      std::map<vaddr_t, size_t>::iterator it;

      // search for first free memory which can hold length
      for(it = d_free_memory.begin(); it != d_free_memory.end(); it++) {
        if(it->second >= length) {
          new_base = it->first;
          // remove free memory to be allocated
          d_free_memory.erase(it);
          // insert remaining free memory
          if(it->second - length > 0) {
            d_free_memory[it->first + length] = it->second - length;
          }
          break;
        }
      }
      return new_base;
    }

    vaddr_t allocate_exact(size_t length, vaddr_t optional_hint) {
      vaddr_t new_base = -1;
      std::map<vaddr_t, size_t>::iterator it;
      size_t space_behind;
      size_t space_in_front;

      // search for first free space which is above optional_hint
      it = d_free_memory.upper_bound(optional_hint);
      // check if there exists free memory below it
      if(it != d_free_memory.begin()) {
        // go to previous free space
        it = std::prev(it);
        // check if new allocation fits in current free memory chunk
        if(it->first + it->second >= optional_hint + length) {
          new_base = optional_hint;
          space_in_front = optional_hint - it->first;
          space_behind = it->second - length - space_in_front;
          // insert new free space behind new allocation
          if(space_behind > 0) {
            d_free_memory[optional_hint + length] = space_behind;
          }
          if(space_in_front > 0) {
            // shrink free space in front of new allocation
            it->second = space_in_front;
          } else {
            // remove free memory entry
            d_free_memory.erase(it);
          }
        }
      }
      return new_base;
    }

    vaddr_t allocate_above(size_t length, vaddr_t optional_hint) {
      vaddr_t new_base = -1;
      // iterate backwards, because chances are higher at the end
      std::map<vaddr_t, size_t>::reverse_iterator it;
      for(it = d_free_memory.rbegin();
          it->first + it->second - length > optional_hint; it++) {
        // since length is round to granularity, new_base will always
        // yield an address matching to granularity.
        if(it->second >= length) {
          new_base = it->first + it->second - length;
          if(it->second - length > 0) {
            // shrink free memory
            it->second -= length;
          } else {
            // delete free memory
            // https://stackoverflow.com/questions/1830158/how-to-call-erase-
            // with-a-reverse-iterator
            d_free_memory.erase(--(it.base()));
          }
          break;
        }
      }
      return new_base;
    }

    vaddr_t allocate_below(size_t length, vaddr_t optional_hint) {
      vaddr_t new_base = -1;
      // iterate forward beginning at first free space
      std::map<vaddr_t, size_t>::iterator it;
      for(it = d_free_memory.begin(); it->first + length < optional_hint;
          it++) {
        if(it->second >= length) {
          new_base = it->first;
          if(it->second - length > 0) {
            // if space if not taken up entirely, create new free space
            d_free_memory[it->first + length] = it->second - length;
          }
          // delete free space
          d_free_memory.erase(it);
          break;
        }
      }
      return new_base;
    }
};

// API

ralloc_t create_memory_manager(vaddr_t base, size_t length,
    size_t granularity) {
  ralloc_t new_manager = new memoryManagerManager(base, length, granularity);
  return new_manager;
}

void destroy_memory_manager(ralloc_t ralloc) {
  delete static_cast<memoryManagerManager*>(ralloc);
}

vaddr_t allocate_range(ralloc_t ralloc, size_t length, allocation_flags flags,
    vaddr_t optional_hint) {
  return static_cast<memoryManagerManager*>(ralloc)->
    create_allocation(length, flags, optional_hint);
}

void free_range(ralloc_t ralloc, vaddr_t base, size_t length) {
  static_cast<memoryManagerManager*>(ralloc)->
    remove_allocation(base, length);
}
