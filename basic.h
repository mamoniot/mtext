//By Monica Moniot
#ifndef INCLUDE__BASIC_H
#define INCLUDE__BASIC_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


typedef uint8_t  byte;
typedef int32_t  relptr;
typedef uint32_t uint;
typedef int8_t   int8;
typedef uint8_t  uint8;
typedef int16_t  int16;
typedef uint16_t uint16;
typedef int32_t  int32;
typedef uint32_t uint32;
typedef int64_t  int64;
typedef uint64_t uint64;

#define KILOBYTE (((int64)1)<<10)
#define MEGABYTE (((int64)1)<<20)
#define GIGABYTE (((int64)1)<<30)
#define TERABYTE (((int64)1)<<40)

#define MACRO_CAT_(a, b) a ## b
#define MACRO_CAT(a, b) MACRO_CAT_(a, b)
#define UNIQUE_NAME(prefix) MACRO_CAT(prefix, __LINE__)

#define cast(type, value) ((type)(value))
#define min(v0, v1) (((v0) < (v1)) ? (v0) : (v1))
#define max(v0, v1) (((v0) > (v1)) ? (v0) : (v1))
#define from_cstr(str) str, strlen(str)
#define ptr_add(type, ptr, n) ((type*)((byte*)(ptr) + (n)))
#define ptr_dist(ptr0, ptr1) ((int64)((byte*)(ptr1) - (byte*)(ptr0)))
#define alloc(type, size) ((type*)malloc(sizeof(type)*(size)))
#define memzero(ptr, size) memset(ptr, 0, size)
#define for_each_lt(name, size) int32 UNIQUE_NAME(name) = (size); for(int32 name = 0; name < UNIQUE_NAME(name); name += 1)
#define for_each_in_range(name, r0, r1) int32 UNIQUE_NAME(name) = (r1); for(int32 name = (r0); name <= UNIQUE_NAME(name); name += 1)
#define for_ever(name) for(int32 name = 0;; name += 1)
#define NULLOP() 0
#define INVALID_EXECUTION() (*((byte*)(0x1337)) = 0)

//NOT C CODE
#ifdef __cplusplus
	#define swap(v0, v1) auto UNIQUE_NAME(__t) = *(v0); *(v0) = *(v1); *(v1) = UNIQUE_NAME(__t)
	#define for_each_in(name, array, size) auto UNIQUE_NAME(name) = (array) + (size); for(auto name = (array); name != UNIQUE_NAME(name); name += 1)
#endif


#define tape_destroy(tape) ((tape) ? free(__tape_base(tape)) : 0)
#define tape_push(tape_ptr, v) (__tape_may_grow((tape_ptr), 1), (*(tape_ptr))[__tape_base(*(tape_ptr))[1]++] = (v))
#define tape_size(tape) ((tape) ? __tape_base(tape)[1] : 0)
#define tape_reserve(tape_ptr, n) (__tape_may_grow((tape_ptr), (n)), __tape_base(*(tape_ptr))[1] += (n), &((*(tape_ptr))[__tape_base(*(tape_ptr))[1] - (n)]))
#define tape_get_last(tape) ((tape)[__tape_base(tape)[1] - 1])
#define tape_reset(tape) ((tape) ? (__tape_base(tape)[1] = 0) : 0)

#define __tape_base(tape) ((uint32*)(tape) - 2)
#define __tape_may_grow(tape_ptr, n) (((*tape_ptr == 0) || (__tape_base(*tape_ptr)[1] + n >= __tape_base(*tape_ptr)[0])) ? ((*(void**)tape_ptr) = __tape_grow((void*)(*tape_ptr), n, sizeof(**tape_ptr))) : 0)

void* __tape_grow(void* tape, uint32 inc, uint32 item_size);
#ifdef BASIC_IMPLEMENTATION
	void* __tape_grow(void* tape, uint32 inc, uint32 item_size) {
	uint32* ptr;
	uint32 new_capacity;
	if(tape) {
		uint32* tape_base = __tape_base(tape);
		uint32 dbl_cur = 2*tape_base[0];
		uint32 min_needed = tape_base[1] + inc;
		new_capacity = dbl_cur > min_needed ? dbl_cur : min_needed;
		ptr = (uint32*)realloc(tape_base, item_size*new_capacity + 2*sizeof(uint32));
	} else {
		new_capacity = inc;
		ptr = (uint32*)malloc(item_size*new_capacity + 2*sizeof(uint32));
		ptr[1] = 0;
	}
	if(ptr) {
		ptr[0] = new_capacity;
		return (void*)(ptr + 2);
	} else {
		return (void*)(2*sizeof(uint32)); // try to force a NULL pointer exception later
	}
}
#endif


#define DEBUG_COOKIE 1234567890
static inline void check(void* base, relptr item, uint32 size) {
	assert(*((int32*)((byte*)base + item) - 1) == DEBUG_COOKIE);//The bottom of the buffer was overwritten
	assert(*(int32*)((byte*)base + item + size) == DEBUG_COOKIE);//The top of the buffer was overwritten
}
#define check_at(item, size) {\
	assert((*((int32*)item - 1) == DEBUG_COOKIE) && "Bottom of buffer was overwritten");\
	assert((*(int32*)((byte*)item + sizeof(*item)*(size)) == DEBUG_COOKIE) && "Top of buffer was overwritten");\
}
static inline void check_below(void* item) {
	assert(*((int32*)item - 1) == DEBUG_COOKIE);//The bottom of the buffer was overwritten
}

#define ptr_get(type, base, i) (check_below(ptr_add(byte, base, i)), ptr_add(type, base, i))

typedef struct {
	uint32 size;
	uint32 capacity;
} Stack;
void stack_init(Stack* stack, uint32 size);

relptr stack_push_rel(Stack* stack, uint32 size);
#define stack_push(type, stack, size) ptr_add(type, stack, stack_push_rel(stack, sizeof(type)*(size)))
void stack_extend(Stack* stack, uint32 size);

void stack_pop(Stack* stack);
void stack_get_last(Stack* stack);

typedef struct {
	relptr first_unused;
	uint32 size;
	uint32 capacity;
	uint32 page_size;
} Pool;
void pool_init(Pool* pool, uint32 page_size, uint32 size);

relptr pool_reserve_rel(Pool* pool);
#define pool_reserve(type, pool) ptr_add(type, pool, pool_reserve_rel(pool))

void pool_free_rel(Pool* pool, relptr i);
void pool_free(Pool* pool, void* ptr);


typedef struct {
	uint32 size;
	uint32 capacity;
	relptr head_block;
	relptr end_block;
} Slab;
void slab_init(Slab* slab, uint32 capacity);

relptr slab_alloc_rel(Slab* slab, uint32 size);
#define slab_alloc(type, slab, size) ptr_add(type, slab, slab_alloc_rel(slab, sizeof(type)*(size)))

void slab_free_rel(Slab* slab, relptr item);
static inline void slab_free(Slab* slab, void* ptr) {
	slab_free_rel(slab, ptr_dist(slab, ptr));
}
relptr slab_realloc_rel(Slab* slab, relptr item, uint32 size);
#define slab_realloc(type, slab, ptr, size) ptr_add(type, slab, slab_realloc_rel(slab, ptr, sizeof(type)*size))


#ifdef BASIC_IMPLEMENTATION
#undef BASIC_IMPLEMENTATION
	static inline uint32 checker_correct_size(uint32 size) {
		return size + 2*sizeof(int32);
	}
	static inline relptr checker_correct_ptr(relptr i) {
		return i + sizeof(int32);
	}
	static inline relptr checker_uncorrect_ptr(relptr i) {
		return i - sizeof(int32);
	}
	static inline void checker_mark(void* base, relptr item, uint32 size) {
		*ptr_add(int32, base, item) = DEBUG_COOKIE;
		*(ptr_add(int32, base, item + size) - 1) = DEBUG_COOKIE;
	}
	static inline void checker_unmark(void* base, relptr item, uint32 size) {
		int32* bot_cookie = ptr_add(int32, base, item);
		int32* top_cookie = ptr_add(int32, base, item + size) - 1;
		assert(*bot_cookie == DEBUG_COOKIE);
		assert(*top_cookie == DEBUG_COOKIE);
		*bot_cookie = 0;
		*top_cookie = 0;
	}


	void stack_init(Stack* stack, uint32 size) {
		stack->size = sizeof(Stack);
		stack->capacity = size - stack->size;
		*ptr_add(uint32, stack, stack->size) = 0;
		stack->size += sizeof(uint32);
	}
	relptr stack_push_rel(Stack* stack, uint32 size) {
		size = checker_correct_size(size);
		relptr ret = stack->size;
		stack->size += size;

		assert(stack->size + sizeof(uint32) < stack->capacity);
		//add previous size for pop
		*ptr_add(uint32, stack, stack->size) = ret;
		stack->size += sizeof(uint32);
		checker_mark(stack, ret, size);
		return checker_correct_ptr(ret);
	}
	void stack_extend(Stack* stack, uint32 size) {
		relptr pre_stack_size = *(ptr_add(uint32, stack, stack->size) - 1);
		stack->size += size;

		assert(stack->size < stack->capacity);
		checker_mark(stack, pre_stack_size, stack->size - pre_stack_size - sizeof(uint32));
		//copy over previous size for pop
		*(ptr_add(uint32, stack, stack->size) - 1) = pre_stack_size;
	}
	void stack_pop(Stack* stack) {
		relptr pre_stack_size = *(ptr_add(uint32, stack, stack->size) - 1);

		assert(pre_stack_size > 0);
		checker_unmark(stack, pre_stack_size, stack->size - pre_stack_size - sizeof(uint32));

		stack->size = pre_stack_size;
	}
	void stack_get_last(Stack* stack) {
		relptr pre_stack_size = *(ptr_add(uint32, stack, stack->size) - 1);
		return checker_correct_ptr(pre_stack_size);
	}
	void stack_get_pre(Stack* stack, relptr item) {
		relptr pre_stack_size = *(ptr_add(uint32, stack, stack->size) - 1);
		return checker_correct_ptr(pre_stack_size);
	}

	void pool_init(Pool* pool, uint32 page_size, uint32 size) {
		page_size = checker_correct_size(page_size);
		pool->first_unused = 0;
		pool->size = sizeof(Pool);
		pool->capacity = size - pool->size;
		pool->page_size = max(page_size, sizeof(relptr));
	}
	relptr pool_reserve_rel(Pool* pool) {
		relptr ret = pool->first_unused;
		if(ret) {
			pool->first_unused = *ptr_add(relptr, pool, ret);
		} else {
			ret = pool->size;
			pool->size += pool->page_size;
			assert(pool->size < pool->capacity);
		}
		checker_mark(pool, ret, pool->page_size);
		return checker_correct_ptr(ret);
	}
	void pool_free_rel(Pool* pool, relptr i) {
		i = checker_uncorrect_ptr(i);
		checker_unmark(pool, i, pool->page_size);

		*ptr_add(relptr, pool, i) = pool->first_unused;
		pool->first_unused = i;
	}
	void pool_free(Pool* pool, void* ptr) {
		pool_free_rel(pool, ptr_dist(pool, ptr));
	}

	typedef struct {
		relptr pre;//points to the previous block in memory, is 0 for the first block
		uint32 size;//the current size of the memory here
		relptr free_pre;//When this equals 0, the block is in use
		relptr free_next;//points to next item in free list
	} __Block;
	static inline void __block_set_next(__Block* block, relptr cur_i, relptr next_i) {
		block->size = next_i - cur_i;
	}
	static inline relptr __block_get_next(__Block* block, relptr cur_i) {
		return block->size + cur_i;
	}
	static inline uint32 __slab_correct_size(uint32 size) {
		return sizeof(__Block)*((size + sizeof(__Block) - 1)/sizeof(__Block) + 1);
	}

	void slab_init(Slab* slab, uint32 capacity) {
		slab->size = sizeof(Slab);
		slab->capacity = capacity;
		slab->head_block = 0;
		slab->end_block = 0;
	}

	relptr slab_alloc_rel(Slab* slab, uint32 size) {
		size = checker_correct_size(size);
		size = __slab_correct_size(size);
		relptr original_i = slab->head_block;
		if(original_i) {
			relptr cur_i = original_i;
			do {
				__Block* cur_block = ptr_add(__Block, slab, cur_i);
				relptr next_i = cur_block->free_next;
				slab->head_block = next_i;
				if(cur_block->size == size) {//remove cur_block from free list
					relptr pre_i = cur_block->free_pre;
					if(next_i == cur_i) {//all items would be removed from free list
						slab->head_block = 0;
					} else {
						slab->head_block = next_i;
						ptr_add(__Block, slab, pre_i)->free_next = next_i;
						ptr_add(__Block, slab, next_i)->free_pre = pre_i;
					}
					cur_block->free_pre = 0;

					checker_mark(slab, cur_i + sizeof(__Block), size - sizeof(__Block));
					return checker_correct_ptr(cur_i + sizeof(__Block));
				} else if(cur_block->size > size) {
					relptr pre_i = cur_block->free_pre;

					relptr fragment_i = cur_i + size;
					__Block* fragment_block = ptr_add(__Block, slab, fragment_i);
					fragment_block->pre = cur_i;
					fragment_block->size = cur_block->size - size;
					cur_block->size = size;
					if(slab->end_block == cur_i) {
						slab->end_block = fragment_i;
					}

					if(next_i == cur_i) {//only one item in free list
						slab->head_block = fragment_i;
						fragment_block->free_pre = fragment_i;
						fragment_block->free_next = fragment_i;
					} else {
						fragment_block->free_pre = pre_i;
						fragment_block->free_next = next_i;
						ptr_add(__Block, slab, pre_i)->free_next = fragment_i;
						ptr_add(__Block, slab, next_i)->free_pre = fragment_i;
					}
					cur_block->free_pre = 0;

					checker_mark(slab, cur_i + sizeof(__Block), size - sizeof(__Block));
					return checker_correct_ptr(cur_i + sizeof(__Block));
				}
				cur_i = next_i;
			} while(original_i != cur_i);
		}
		//no free space, push the size
		relptr i = slab->size;
		__Block* block = ptr_add(__Block, slab, i);
		if(slab->end_block) {
			__block_set_next(ptr_add(__Block, slab, slab->end_block), slab->end_block, i);
			block->pre = slab->end_block;
		}
		block->size = size;
		block->free_pre = 0;
		slab->size += size;
		slab->end_block = i;

		assert(slab->size <= slab->capacity);
		checker_mark(slab, i + sizeof(__Block), size - sizeof(__Block));
		return checker_correct_ptr(i + sizeof(__Block));
	}

	void slab_free_rel(Slab* slab, relptr item) {
		item = checker_uncorrect_ptr(item);
		relptr cur_i = item - sizeof(__Block);
		__Block* cur_block = ptr_add(__Block, slab, cur_i);
		relptr pre_i = cur_block->pre;
		__Block* pre_block = ptr_add(__Block, slab, pre_i);
		relptr next_i = __block_get_next(cur_block, cur_i);
		__Block* next_block = ptr_add(__Block, slab, next_i);

		assert(!cur_block->free_pre);
		checker_unmark(slab, item, cur_block->size - sizeof(__Block));

		relptr head_i = slab->head_block;
		if(pre_i && pre_block->free_pre) {//is free, combine
			if(slab->end_block == cur_i) {//is last block in the whole slab, remove from slab
				slab->end_block = pre_block->pre;
				slab->size -= cur_block->size + pre_block->size;
				//remove pre_block
				relptr free_pre_i = pre_block->free_pre;
				relptr free_next_i = pre_block->free_next;
				if(head_i == pre_i) {
					if(pre_i == free_next_i) {//empty free list
						slab->head_block = 0;
					} else {
						slab->head_block = free_next_i;
						ptr_add(__Block, slab, free_pre_i)->free_next = free_next_i;
						ptr_add(__Block, slab, free_next_i)->free_pre = free_pre_i;
					}
				} else {
					ptr_add(__Block, slab, free_pre_i)->free_next = free_next_i;
					ptr_add(__Block, slab, free_next_i)->free_pre = free_pre_i;
				}
			} else if(next_block->free_pre) {//remove next_block and cur_block
				relptr free_pre_i = next_block->free_pre;
				relptr free_next_i = next_block->free_next;
				if(head_i == next_i) {
					slab->head_block = pre_i;
				}
				ptr_add(__Block, slab, free_next_i)->free_pre = free_pre_i;
				ptr_add(__Block, slab, free_pre_i)->free_next = free_next_i;
				__block_set_next(pre_block, pre_i, __block_get_next(next_block, next_i));
				ptr_add(__Block, slab, __block_get_next(next_block, next_i))->pre = pre_i;
			} else {//remove cur_block
				__block_set_next(pre_block, pre_i, next_i);
				next_block->pre = pre_i;
			}
		} else if(slab->end_block == cur_i) {//cur_block is last block in slab, remove it
			slab->end_block = pre_i;
			slab->size -= cur_block->size;
		} else if(next_block->free_pre) {//inherit next_block's free list info
			relptr free_pre_i = next_block->free_pre;
			relptr free_next_i = next_block->free_next;
			if(head_i == next_i) {
				slab->head_block = cur_i;
				if(free_pre_i == next_i) {//next_block is only member of free list
					cur_block->free_pre = cur_i;
					cur_block->free_next = cur_i;
				} else {
					cur_block->free_pre = free_pre_i;
					cur_block->free_next = free_next_i;
					ptr_add(__Block, slab, free_next_i)->free_pre = cur_i;
					ptr_add(__Block, slab, free_pre_i)->free_next = cur_i;
				}
			} else {
				cur_block->free_pre = free_pre_i;
				cur_block->free_next = free_next_i;
				ptr_add(__Block, slab, free_next_i)->free_pre = cur_i;
				ptr_add(__Block, slab, free_pre_i)->free_next = cur_i;
			}
			//remove next_block
			__block_set_next(cur_block, cur_i, __block_get_next(next_block, next_i));
			ptr_add(__Block, slab, __block_get_next(next_block, next_i))->pre = cur_i;
		} else if(head_i) {//add cur_block to the free list
			__Block* head_block = ptr_add(__Block, slab, head_i);
			relptr last_i = head_block->free_pre;
			ptr_add(__Block, slab, last_i)->free_next = cur_i;
			head_block->free_pre = cur_i;
			cur_block->free_next = head_i;
			cur_block->free_pre = last_i;
		} else {//list is empty
			slab->head_block = cur_i;
			cur_block->free_pre = cur_i;
			cur_block->free_next = cur_i;
		}
	}

	relptr slab_realloc_rel(Slab* slab, relptr item, uint32 size) {
		relptr cur_i = item - sizeof(__Block);
		__Block* cur_block = ptr_add(__Block, slab, cur_i);
		if(cur_block->size >= size) {
			return item;
		} else {
			relptr new_item = slab_alloc_rel(slab, size);
			memcpy(ptr_add(byte*, slab, new_item), ptr_add(byte*, slab, item), cur_block->size);
			slab_free_rel(slab, item);
			return new_item;
		}
	}
#endif


#ifdef __cplusplus
}
#endif
#endif
