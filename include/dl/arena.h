#pragma once
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace dl {

// Arena allocator for type T, with pointer stability.
// Objects are allocated in fixed-size blocks to avoid pointer invalidation.
template<typename T, size_t BlockSize = 1024> class Arena
{
public:
	Arena()  = default;
	~Arena() = default;

	// Non-copyable, movable.
	Arena(const Arena&)                = delete;
	Arena& operator=(const Arena&)     = delete;
	Arena(Arena&&) noexcept            = default;
	Arena& operator=(Arena&&) noexcept = default;

	// Construct an object in-place in the arena, return its pointer.
	// The pointer will never be invalidated until clear() or destruction.
	template<typename... Args> T* emplace(Args&&... args)
	{
		// Allocate a new block if needed.
		if (blocks_.empty() || block_pos_ == BlockSize) {
			blocks_.emplace_back(std::make_unique<Block>());
			block_pos_ = 0;
		}
		// Placement-new the object in the current block.
		T* ptr = reinterpret_cast<T*>(&blocks_.back()->data[block_pos_]);
		new (ptr) T(std::forward<Args>(args)...);
		++block_pos_;
		++size_;
		return ptr;
	}

	// Destroy all objects and release all memory.
	void clear()
	{
		for (auto& blk : blocks_) {
			// Only the last block may be partially filled.
			size_t n = (blk.get() == blocks_.back().get()) ? block_pos_ : BlockSize;
			for (size_t i = 0; i < n; ++i) {
				T* ptr = reinterpret_cast<T*>(&blk->data[i]);
				ptr->~T();
			}
		}
		blocks_.clear();
		block_pos_ = 0;
		size_      = 0;
	}

	size_t size() const { return size_; }
	bool   empty() const { return size_ == 0; }

private:
	// A block of uninitialized storage for BlockSize objects of type T.
	struct Block
	{
		// Use std::aligned_storage to ensure proper alignment.
		typename std::aligned_storage<sizeof(T), alignof(T)>::type data[BlockSize];
	};
	// All allocated blocks.
	std::vector<std::unique_ptr<Block>> blocks_;
	// Current position in the last block.
	size_t block_pos_ = 0;
	// Total number of objects allocated.
	size_t size_ = 0;
};

}   // namespace dl