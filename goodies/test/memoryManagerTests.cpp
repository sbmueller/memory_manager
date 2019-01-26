#include <gtest/gtest.h>
// bad practice, but unavoidable since only one solution file is allowed
#include "../../memoryManager.cpp"

typedef std::map<vaddr_t, size_t> memmap;

ralloc_t ra;
vaddr_t ra_base;
memoryManagerManager* ra_mgr;
const size_t alloc_length = 1312;
const size_t alloc_gran = 8;
const vaddr_t error_flag = static_cast<vaddr_t>(-1);

class memoryManagerTests: public ::testing::Test {
  protected:
    void SetUp() {
      ra_base = reinterpret_cast<vaddr_t>(malloc(alloc_length));
      ra = create_memory_manager(ra_base, alloc_length, alloc_gran);
      // convert to manager pointer
      ra_mgr = static_cast<memoryManagerManager*>(ra);
    }

    void TearDown() {
      destroy_memory_manager(ra);
    }
};

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

size_t remaining_space(memmap map) {
  size_t total_free_space = 0;
  std::map<vaddr_t, size_t>::iterator it;
  for(it = map.begin(); it != map.end(); it++) {
    total_free_space += it->second;
  }
  return total_free_space;
}

TEST_F(memoryManagerTests, readInitProperties) {
  memmap free_memory = ra_mgr->get_free_memory();
  // check if base key is in map
  EXPECT_NE(free_memory.end(), free_memory.find(ra_base));
  // check correct value for key
  EXPECT_EQ(alloc_length, free_memory[ra_base]);
}

TEST_F(memoryManagerTests, allocateZeroLength) {
  allocate_range(ra, 0, ALLOCATE_ANY, 1234);
  memmap free_memory = ra_mgr->get_free_memory();
  // nothing should have happened
  // check if base key is in map
  EXPECT_NE(free_memory.end(), free_memory.find(ra_base));
  // check correct value for key
  EXPECT_EQ(alloc_length, free_memory.at(ra_base));
}

TEST_F(memoryManagerTests, allocateAny) {
  // should allocate 48
  allocate_range(ra, 43, ALLOCATE_ANY, 1234);
  // should allocate 8
  allocate_range(ra, 3, ALLOCATE_ANY, 1234);
  // should allocate 104
  allocate_range(ra, 100, ALLOCATE_ANY, 1234);
  memmap free_memory = ra_mgr->get_free_memory();
  size_t new_len = free_memory.begin()->second;
  // check correct value for key
  EXPECT_EQ(new_len, alloc_length - 160);
}

TEST_F(memoryManagerTests, allocateExact) {
  vaddr_t alloc = allocate_range(ra, 7, ALLOCATE_EXACT, ra_base + 48);
  memmap free_memory = ra_mgr->get_free_memory();
  size_t total_free_space = remaining_space(free_memory);
  // check resizing
  EXPECT_EQ(free_memory.begin()->first + free_memory.begin()->second,
      ra_base + 48);
  EXPECT_EQ(std::prev(free_memory.end())->first, ra_base + 48 + 8);
  // check remaining space
  EXPECT_EQ(total_free_space, alloc_length - 8);
  // exact allocation address
  EXPECT_EQ(alloc, ra_base + 48);
}

TEST_F(memoryManagerTests, allocateAbove) {
  // should allocate 104
  vaddr_t alloc = allocate_range(ra, 100, ALLOCATE_ABOVE, ra_base + 1000);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_GT(alloc, ra_base + 1000);
  size_t total_free_space = remaining_space(free_memory);
  EXPECT_EQ(total_free_space, alloc_length - 104);
}

TEST_F(memoryManagerTests, allocateBelow) {
  // should allocate 104
  vaddr_t alloc = allocate_range(ra, 100, ALLOCATE_BELOW, ra_base + 1000);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_LT(alloc, ra_base + 1000);
  size_t total_free_space = remaining_space(free_memory);
  EXPECT_EQ(total_free_space, alloc_length - 104);
}

TEST_F(memoryManagerTests, removeAllocation) {
  allocate_range(ra, 7, ALLOCATE_EXACT, ra_base + 48);
  free_range(ra, ra_base + 48, 8);
  memmap free_memory = ra_mgr->get_free_memory();
  size_t total_free_space = remaining_space(free_memory);
  EXPECT_EQ(total_free_space, alloc_length);
  EXPECT_EQ(free_memory.size(), 1);
}

TEST_F(memoryManagerTests, wrongExactAddress1) {
  vaddr_t alloc = allocate_range(ra, 7, ALLOCATE_EXACT, ra_base + 50);
  EXPECT_EQ(alloc, error_flag);
}

TEST_F(memoryManagerTests, wrongExactAddress2) {
  allocate_range(ra, alloc_length / 2, ALLOCATE_EXACT, ra_base);
  allocate_range(ra, 8, ALLOCATE_EXACT, ra_base + alloc_length / 4);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(1, free_memory.size());
  EXPECT_EQ(ra_base + alloc_length / 2, free_memory.begin()->first);
  EXPECT_EQ(alloc_length / 2, free_memory.begin()->second);
}

TEST_F(memoryManagerTests, wrongExactAddress3) {
  allocate_range(ra, alloc_length / 2, ALLOCATE_EXACT, ra_base +
      alloc_length / 2);
  // wrong API call
  allocate_range(ra, alloc_length / 2, ALLOCATE_EXACT, ra_base +
      alloc_length / 4);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(1, free_memory.size());
  EXPECT_EQ(ra_base, free_memory.begin()->first);
  EXPECT_EQ(alloc_length / 2, free_memory.begin()->second);
}

TEST_F(memoryManagerTests, addressOutsideOfPhy) {
  vaddr_t alloc_exact = allocate_range(ra, 100, ALLOCATE_EXACT, ra_base - 1);
  vaddr_t alloc_above = allocate_range(ra, 100, ALLOCATE_ABOVE, ra_base +
      alloc_length);
  vaddr_t alloc_below = allocate_range(ra, 100, ALLOCATE_BELOW, ra_base);
  EXPECT_EQ(alloc_exact, error_flag);
  EXPECT_EQ(alloc_above, error_flag);
  EXPECT_EQ(alloc_below, error_flag);
}

TEST_F(memoryManagerTests, randomTest1) {
  allocate_range(ra, 56, ALLOCATE_EXACT, ra_base + 48);
  vaddr_t alloc = allocate_range(ra, 100, ALLOCATE_ANY, 0);
  // should be behind exact allocation
  EXPECT_EQ(alloc, ra_base + 48 + 56);
}

TEST_F(memoryManagerTests, allocateAllExact) {
  allocate_range(ra, alloc_length, ALLOCATE_EXACT, ra_base);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(0, free_memory.size());
}

TEST_F(memoryManagerTests, allocateAllExact2) {
  allocate_range(ra, alloc_length / 2, ALLOCATE_EXACT, ra_base);
  allocate_range(ra, alloc_length / 2, ALLOCATE_EXACT, ra_base +
      alloc_length / 2);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(0, free_memory.size());
}

TEST_F(memoryManagerTests, allocateAllAny) {
  allocate_range(ra, alloc_length / 2, ALLOCATE_ANY, ra_base);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(ra_base + alloc_length / 2, free_memory.begin()->first);
  EXPECT_EQ(alloc_length / 2, free_memory.begin()->second);
  allocate_range(ra, (alloc_length - 1) / 2, ALLOCATE_ANY, ra_base);
  free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(0, free_memory.size());
}

TEST_F(memoryManagerTests, removeFromFull1) {
  allocate_range(ra, alloc_length, ALLOCATE_ANY, ra_base);
  // should free 1312 at ra_base
  free_range(ra, ra_base, alloc_length);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(1, free_memory.size());
  EXPECT_EQ(ra_base, free_memory.begin()->first);
  EXPECT_EQ(alloc_length, free_memory.begin()->second);
}

TEST_F(memoryManagerTests, removeFromFull2) {
  allocate_range(ra, alloc_length, ALLOCATE_ANY, ra_base);
  // should free 656 at ra_base + 656
  free_range(ra, ra_base + alloc_length / 2, alloc_length / 2);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(1, free_memory.size());
  EXPECT_EQ(ra_base + alloc_length / 2, free_memory.begin()->first);
  EXPECT_EQ(alloc_length / 2, free_memory.begin()->second);
}

TEST_F(memoryManagerTests, removeFromFull3) {
  allocate_range(ra, alloc_length, ALLOCATE_ANY, ra_base);
  // should free 656 at ra_base
  free_range(ra, ra_base, alloc_length / 2);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(1, free_memory.size());
  EXPECT_EQ(ra_base, free_memory.begin()->first);
  EXPECT_EQ(alloc_length / 2, free_memory.begin()->second);
}

TEST_F(memoryManagerTests, removeAbove) {
  allocate_range(ra, alloc_length, ALLOCATE_ANY, ra_base);
  // should free 80 at location 48
  free_range(ra, ra_base + 48, 32);
  free_range(ra, ra_base + 80, 48);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(1, free_memory.size());
  EXPECT_EQ(ra_base + 48, free_memory.begin()->first);
  EXPECT_EQ(80, free_memory.begin()->second);
}

TEST_F(memoryManagerTests, removeBelow) {
  allocate_range(ra, alloc_length, ALLOCATE_ANY, ra_base);
  // should free 80 at location 48
  free_range(ra, ra_base + 80, 48);
  free_range(ra, ra_base + 48, 32);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(1, free_memory.size());
  EXPECT_EQ(ra_base + 48, free_memory.begin()->first);
  EXPECT_EQ(80, free_memory.begin()->second);
}

TEST_F(memoryManagerTests, removeAboveAndBelow) {
  allocate_range(ra, alloc_length, ALLOCATE_ANY, ra_base);
  // remaining allocation between 48 and 80
  free_range(ra, ra_base + 80, 48);
  free_range(ra, ra_base + 32, 16);
  // free remaining
  free_range(ra, ra_base + 48, 32);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(1, free_memory.size());
  EXPECT_EQ(ra_base + 32, free_memory.begin()->first);
  EXPECT_EQ(96, free_memory.begin()->second);
}

TEST_F(memoryManagerTests, removeOutsideOfPhy) {
  allocate_range(ra, alloc_length, ALLOCATE_ANY, ra_base);
  // should free nothing
  free_range(ra, ra_base + 48, 2624);
  memmap free_memory = ra_mgr->get_free_memory();
  EXPECT_EQ(0, free_memory.size());
}
