//By Monica Moniot
#ifndef MAM_ALLOC__INCLUDE
#define MAM_ALLOC__INCLUDE
#ifdef __cplusplus
extern "C" {
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////
// MAM_ALLOC_DISABLE_STDLIB
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//

#ifndef MAM_ALLOC_DISABLE_STDLIB
#include <stdlib.h>
#define MAM_ALLOC_MALLOC malloc
#define MAM_ALLOC_REALLOC realloc
#define MAM_ALLOC_MEMCPY memcpy
#endif

#ifndef MAM_ALLOC_ASSERT
#ifdef MAM_ALLOC_DEBUG
#include <assert.h>
#define MAM_ALLOC_ASSERT(is, msg) assert((is) && msg);
#else
#define MAM_ALLOC_ASSERT(is, msg) 0
#endif
#endif

#ifndef MAM_ALLOC_SIZE_T
#define MAM_ALLOC_SIZE_T int
#endif
#ifndef MAM_ALLOC_BYTE_T
#define MAM_ALLOC_BYTE_T char
#endif
typedef MAM_ALLOC_SIZE_T mam_int;
typedef MAM_ALLOC_BYTE_T mam_byte;


#ifdef MAM_ALLOC_STATIC
#undef MAM_ALLOC_STATIC
#define MAM_ALLOC__DECLR static
#else
#define MAM_ALLOC__DECLR
#endif
#define MAM_ALLOC__DECLS static inline

#ifndef MAM_ALLOC_COOKIE
#define MAM_ALLOC_COOKIE 1234567890
#endif

#define mam_ptr_add(type, ptr, n) ((type*)((mam_byte*)(ptr) + (n)))
#define mam_ptr_dist(ptr0, ptr1) ((mam_int)((mam_byte*)(ptr1) - (mam_byte*)(ptr0)))

#define mam_will_overflow(allocator, n) ((allocator->mem_size + n) > allocator->mem_capacity)


typedef union MamTape {
	mam_byte mem[1];
	struct {
		mam_int mem_size;
		mam_int mem_capacity;
		mam_int item_size;
	};
} MamTape;

typedef union MamStack {
	mam_byte mem[1];
	struct {
		mam_int mem_size;
		mam_int mem_capacity;
	};
} MamStack;

typedef union MamPool {
	mam_byte mem[1];
	struct {
		mam_int first_unused;
		mam_int mem_size;
		mam_int mem_capacity;
		mam_int item_size;
	};
} MamPool;

typedef union MamHeap {
	mam_byte mem[1];
	struct {
		mam_int mem_size;
		mam_int mem_capacity;
		mam_int head_block;
		mam_int end_block;
	};
} MamHeap;


#ifdef MAM_ALLOC_DEBUG
#define mam__overflow_msg "mam_alloc: buffer overflow detected"
#define mam__underflow_msg "mam_alloc: bottom of buffer was overwritten; could be a buffer underflow or an overflow of the adjacent buffer"
MAM_ALLOC__DECLS void mam_checki(void* allocator, mam_int item, mam_int item_size) {
	MAM_ALLOC_ASSERT(*((int*)((mam_byte*)allocator + item) - 1) == MAM_ALLOC_COOKIE, mam__underflow_msg);
	MAM_ALLOC_ASSERT(*(int*)((mam_byte*)allocator + item + item_size) == MAM_ALLOC_COOKIE, mam__overflow_msg);
}
MAM_ALLOC__DECLS void mam_check(void* ptr, mam_int ptr_size) {
	MAM_ALLOC_ASSERT(*((int*)ptr - 1) == MAM_ALLOC_COOKIE, mam__underflow_msg);
	MAM_ALLOC_ASSERT(*(int*)((mam_byte*)ptr + ptr_size) == MAM_ALLOC_COOKIE, mam__overflow_msg);
}
MAM_ALLOC__DECLS void mam_check_belowi(void* allocator, mam_int item) {
	MAM_ALLOC_ASSERT(*((int*)((mam_byte*)allocator + item) - 1) == MAM_ALLOC_COOKIE, mam__underflow_msg);
}
MAM_ALLOC__DECLS void mam_check_below(void* ptr) {
	MAM_ALLOC_ASSERT(*((int*)ptr - 1) == MAM_ALLOC_COOKIE, mam__underflow_msg);
}
#define mam_get_ptr(type, allocator, item) (mam_check_below(mam_ptr_add(mam_byte, allocator, item)), mam_ptr_add(type, allocator, item))

#define mam_checker_correct_size(size)   (size + 2*sizeof(int))
#define mam_checker_uncorrect_size(size) (size - 2*sizeof(int))
#define mam_checker_correct_item(item)   (item +   sizeof(int))
#define mam_checker_uncorrect_item(item) (item -   sizeof(int))

MAM_ALLOC__DECLS void mam_checker_mark(void* allocator, mam_int item, mam_int item_size) {
	//takes item uncorrected and item_size corrected
	*mam_ptr_add(int, allocator, item) = MAM_ALLOC_COOKIE;
	*(mam_ptr_add(int, allocator, item + item_size) - 1) = MAM_ALLOC_COOKIE;
}
MAM_ALLOC__DECLS void mam_checker_unmark(void* allocator, mam_int item, mam_int item_size) {
	int* bot_cookie = mam_ptr_add(int, allocator, item);
	int* top_cookie = mam_ptr_add(int, allocator, item + item_size) - 1;
	MAM_ALLOC_ASSERT(*bot_cookie == MAM_ALLOC_COOKIE, mam__underflow_msg);
	MAM_ALLOC_ASSERT(*top_cookie == MAM_ALLOC_COOKIE, mam__overflow_msg);
	*bot_cookie = 0;
	*top_cookie = 0;
}

#else
#define mam_checki(allocator, item, item_size) 0
#define mam_check(ptr, ptr_size) 0
#define mam_check_below(ptr) 0
#define mam_get_ptr(type, allocator, item) mam_ptr_add(type, allocator, item)

#define mam_checker_correct_size(size)   size
#define mam_checker_uncorrect_size(size) size
#define mam_checker_correct_item(item)   item
#define mam_checker_uncorrect_item(item) item

#define mam_checker_mark(allocator, item, item_size) 0
#define mam_checker_unmark(allocator, item, item_size) 0
#endif


#define mam__grow(allocator, n, offset) {\
	mam_int dbl_capacity = 2*allocator->mem_capacity;\
	mam_int min_needed = allocator->mem_size + n;\
	mam_int new_capacity = (dbl_capacity > min_needed) ? dbl_capacity : min_needed;\
	mam_int alloc_size = offset + new_capacity;\
	allocator = (MAM_ALLOC_REALLOC(((void*)allocator), alloc_size));\
}


#define mam_tape_get_buffer(type, tape) (mam_ptr_add(type, tape, sizeof(MamTape)), MAM_ALLOC_ASSERT(sizeof(type) == (tape)->item_size), "mam_alloc: attempt to get a tape buffer of the wrong type size")
#define mam_tape_from_buffer(buffer) mam_ptr_add(MamTape, buffer, -sizeof(MamTape))

MAM_ALLOC__DECLS MamTape* mam_tape_initn(void* alloc_ptr, mam_int alloc_size, mam_int item_size) {
	MamTape* tape = (MamTape*)alloc_ptr;
	tape->mem_size = sizeof(MamTape);
	tape->mem_capacity = alloc_size;
	tape->item_size = item_size;
	return tape;
}
#define mam_tape_init(type, ptr, size) mam_tape_initn(ptr, size, sizeof(type))
#define mam_tape_initb(type, ptr, size) mam_tape_get_buffer(type, mam_tape_init_nt(ptr, size, sizeof(type)))

#define mam_tape_new(type) mam_tape_newn(sizeof(type))
#define mam_tape_newb(type) mam_tape_get_buffer(type, mam_tape_new(type))
MAM_ALLOC__DECLS MamTape* mam_tape_newn(mam_int item_size) {
	mam_int init_capacity = 4*item_size;
	mam_int alloc_size = sizeof(MamTape) + init_capacity;
	MamTape* tape = (MamTape*)(MAM_ALLOC_MALLOC(alloc_size));
	MAM_ALLOC_ASSERT(tape != 0, "mam_alloc: failed to allocate memory for tape");
	mam_tape_init_nt(tape, alloc_size, item_size);
	return tape;
}

MAM_ALLOC__DECLS mam_int mam_tape_reservei(MamTape* tape, mam_int item_n) {
	mam_int size = item_n*tape->item_size;
	if(mam_will_overflow(tape, size)) {
		mam__grow(tape, size, sizeof(MamTape));
		MAM_ALLOC_ASSERT(tape != 0, "mam_alloc: failed to reallocate memory for tape")
	}
	mam_int item = tape->mem_size;
	tape->mem_size += size;
	return item;
}
MAM_ALLOC__DECLS void* mam_tape_reserven(MamTape* tape, mam_int item_n) {
	return mam_ptr_add(void, tape, mam_tape_reservei(tape, item_n));
}
#define mam_tape_reserve(type, tape, item_n) ((type*)mam_tape_reserven(tape, item_n))
MAM_ALLOC__DECLS mam_int mam_tape_reserveib(void* buffer, mam_int item_n) {
	MamTape* tape = mam_tape_from_buffer(buffer);
	return (mam_tape_reservei(tape, item_n) - sizeof(MamTape))/tape->item_size;
}
#define mam_tape_reserveb(buffer, item_n) (&buffer[mam_tape_reserveib(buffer, item_n)])

MAM_ALLOC__DECLS void mam_tape_append(MamTape* tape, void* item) {
	mam_int size = tape->item_size;
	if(mam_will_overflow(tape, size)) {
		mam__grow(tape, size, sizeof(MamTape));
		MAM_ALLOC_ASSERT(tape != 0, "mam_alloc: failed to reallocate memory for tape")
	}
	void* ret = ((void*)tape->mem[tape->mem_size]);
	tape->mem_size += size;
	MAM_ALLOC_MEMCPY(ret, item, size);//TODO: remove
}
#define mam_tape_appendb(buffer, item) mam_tape_append(mam_tape_from_buffer(buffer), item)

MAM_ALLOC__DECLS mam_int mam_tape_size(MamTape* tape) {
	return (tape->mem_size - sizeof(MamTape))/tape->item_size;
}
#define mam_tape_sizeb(buffer) (mam_tape_from_buffer(buffer)->mem_size - sizeof(MamTape))/sizeof(*buffer)


MAM_ALLOC__DECLS MamStack* mam_stack_init(void* alloc_ptr, mam_int alloc_size) {
	MamStack* stack = (MamStack*)alloc_ptr;
	stack->mem_size = sizeof(MamStack) + sizeof(mam_int);
	stack->mem_capacity = alloc_size;
	//place 0 on the stack so invalid pops can be detected
	*(mam_ptr_add(mam_int, stack, stack->mem_size) - 1) = 0;
	return stack;
}
MAM_ALLOC__DECLS MamStack* mam_stack_new(void) {
	mam_int alloc_size = sizeof(MamStack) + 16;
	MamStack* stack = (MamStack*)(MAM_ALLOC_MALLOC(alloc_size));
	MAM_ALLOC_ASSERT(stack != 0, "mam_alloc: failed to allocate memory for stack");
	mam_stack_init(stack, alloc_size);
	return stack;
}

MAM_ALLOC__DECLS mam_int mam_stack_pushi(MamStack* stack, mam_int size) {
	size = mam_checker_correct_size(size);
	mam_int alloc_size = size + sizeof(mam_int);

	if(mam_will_overflow(stack, alloc_size)) {
		mam__grow(stack, alloc_size, sizeof(MamStack));
		MAM_ALLOC_ASSERT(stack != 0, "mam_alloc: failed to reallocate memory for stack")
	}
	mam_int item = stack->mem_size;
	stack->mem_size += size;
	//add previous size for pop
	*mam_ptr_add(mam_int, stack, stack->mem_size) = item;
	stack->mem_size += sizeof(mam_int);

	mam_checker_mark(stack, item, size);
	return mam_checker_correct_item(item);
}
MAM_ALLOC__DECLS void mam_stack_extend(MamStack* stack, mam_int size) {
	mam_int pre_stack_size = *(mam_ptr_add(mam_int, stack, stack->mem_size) - 1);

	if(mam_will_overflow(stack, size)) {
		mam__grow(stack, size, sizeof(MamStack));
		MAM_ALLOC_ASSERT(stack != 0, "mam_alloc: failed to reallocate memory for stack")
	}
	stack->mem_size += size;
	mam_checker_mark(stack, pre_stack_size, stack->mem_size - pre_stack_size - sizeof(mam_int));
	//copy over previous size for pop
	*(mam_ptr_add(mam_int, stack, stack->mem_size) - 1) = pre_stack_size;
}
MAM_ALLOC__DECLS void mam_stack_pop(MamStack* stack) {
	mam_int pre_stack_size = *(mam_ptr_add(mam_int, stack, stack->mem_size) - 1);

	MAM_ALLOC_ASSERT(pre_stack_size > 0, "mam_alloc: attempt to pop stack when stack was empty");
	mam_checker_unmark(stack, pre_stack_size, stack->mem_size - pre_stack_size - sizeof(mam_int));

	stack->mem_size = pre_stack_size;
}

MAM_ALLOC__DECLS mam_int mam_stack_get_lasti(MamStack* stack) {
	mam_int pre_stack_size = *(mam_ptr_add(mam_int, stack, stack->mem_size) - 1);
	return mam_checker_correct_item(pre_stack_size);
}
MAM_ALLOC__DECLS void* mam_stack_get_lastn(MamStack* stack) {
	return mam_ptr_add(void*, stack, mam_stack_get_lasti(stack));
}
#define mam_stack_get_last(type, stack) ((type*)mam_stack_get_lastn(stack))

MAM_ALLOC__DECLS mam_int mam_stack_get_prei(MamStack* stack, mam_int item) {
	item = mam_checker_uncorrect_item(item);
	mam_int pre_stack_size = *(mam_ptr_add(mam_int, stack, item) - 1);
	return mam_checker_correct_item(pre_stack_size);
}
MAM_ALLOC__DECLS void* mam_stack_get_pren(MamStack* stack, void* ptr) {
	return mam_ptr_add(void*, stack, mam_stack_get_prei(stack, mam_ptr_dist(stack, ptr)));
}
#define mam_stack_get_pre(type, stack, ptr) ((type*)mam_stack_get_pren(stack, ptr))



MAM_ALLOC__DECLS MamPool* mam_pool_initn(void* alloc_ptr, mam_int alloc_size, mam_int item_size) {
	item_size = mam_checker_correct_size(item_size);
	MamPool* pool = (MamPool*)alloc_ptr;
	pool->first_unused = 0;
	pool->mem_size = sizeof(MamPool);
	pool->mem_capacity = alloc_size;
	pool->item_size = (item_size > sizeof(mam_int)) ? item_size : sizeof(mam_int);
	return pool;
}
#define mam_pool_init(type, alloc_ptr, alloc_size) mam_pool_initn(alloc_ptr, alloc_size, sizeof(type))

MAM_ALLOC__DECLS MamPool* mam_pool_newn(mam_int item_size) {
	mam_int alloc_size = sizeof(MamPool) + 4*item_size;
	MamPool* pool = (MamPool*)(MAM_ALLOC_MALLOC(alloc_size));
	MAM_ALLOC_ASSERT(pool != 0, "mam_alloc: failed to allocate memory for pool");
	mam_pool_initn(pool, alloc_size, item_size);
	return pool;
}
#define mam_pool_new(type) mam_pool_newn(sizeof(type))

MAM_ALLOC__DECLS mam_int mam_pool_alloci(MamPool* pool) {
	mam_int item = pool->first_unused;
	if(item) {
		pool->first_unused = *mam_ptr_add(mam_int, pool, item);
	} else {
		mam_int size = pool->item_size;
		if(mam_will_overflow(pool, size)) {
			mam__grow(pool, size, sizeof(MamPool));
			MAM_ALLOC_ASSERT(pool != 0, "mam_alloc: failed to reallocate memory for pool")
		}
		item = pool->mem_size;
		pool->mem_size += size;
	}
	mam_checker_mark(pool, item, pool->item_size);
	return mam_checker_correct_item(item);
}
MAM_ALLOC__DECLS void* mam_pool_allocn(MamPool* pool) {
	return mam_ptr_add(void, pool, mam_pool_alloci(pool));
}
#define mam_pool_alloc(type, pool) ((type*)mam_pool_allocn(pool))

MAM_ALLOC__DECLS void mam_pool_freei(MamPool* pool, mam_int item) {
	item = mam_checker_uncorrect_item(item);
	mam_checker_unmark(pool, item, pool->item_size);

	*mam_ptr_add(mam_int, pool, item) = pool->first_unused;
	pool->first_unused = item;
}
MAM_ALLOC__DECLS void mam_pool_free(MamPool* pool, void* ptr) {
	mam_pool_freei(pool, ptr_dist(pool, ptr));
}



MAM_ALLOC__DECLS MamHeap* mam_heap_init(void* alloc_ptr, mam_int alloc_size) {
	MamHeap* heap = (MamHeap*)alloc_ptr;
	heap->mem_size = sizeof(MamHeap);
	heap->mem_capacity = alloc_size;
	heap->head_block = 0;
	heap->end_block = 0;
	return heap;
}

MAM_ALLOC__DECLS MamHeap* mam_heap_new(void) {
	mam_int alloc_size = sizeof(MamHeap) + 16;
	MamHeap* heap = (MamHeap*)(MAM_ALLOC_MALLOC(alloc_size));
	MAM_ALLOC_ASSERT(heap != 0, "mam_alloc: failed to allocate memory for heap");
	mam_heap_initn(heap, alloc_size);
	return heap;
}

typedef struct Mam__Block {
	mam_int pre;//points to the previous block in memory, is 0 for the first block
	mam_int size;//the current size of the memory here
	mam_int free_pre;//When this equals 0, the block is in use
	mam_int free_next;//points to next item in free list
} Mam__Block;
MAM_ALLOC__DECLS void mam__block_set_next(Mam__Block* block, mam_int cur_i, mam_int next_i) {
	block->size = next_i - cur_i;
}
MAM_ALLOC__DECLS mam_int mam__block_get_next(Mam__Block* block, mam_int cur_i) {
	return block->size + cur_i;
}
MAM_ALLOC__DECLS mam_int mam__heap_correct_size(mam_int size) {
	return sizeof(Mam__Block)*((size + sizeof(Mam__Block) - 1)/sizeof(Mam__Block) + 1);
}

MAM_ALLOC__DECLR mam_int mam_heap_alloci(MamHeap* heap, mam_int size);
MAM_ALLOC__DECLS void* mam_heap_allocn(MamHeap* heap, mam_int size) {
	return mam_ptr_add(void, heap, mam_heap_alloci(heap, size));
}
#define mam_heap_alloc(type, heap, size) ((type*)mam_heap_allocn(heap, size))

MAM_ALLOC__DECLR void mam_heap_freei(MamHeap* heap, mam_int item);
MAM_ALLOC__DECLS void mam_heap_free(MamHeap* heap, void* ptr) {
	mam_heap_freei(heap, mam_ptr_dist(heap, ptr));
}

MAM_ALLOC__DECLR mam_int mam_heap_realloci(MamHeap* heap, mam_int item, mam_int size);
MAM_ALLOC__DECLS void* mam_heap_reallocn(MamHeap* heap, void* ptr, mam_int size) {
	return mam_ptr_add(void, heap, mam_heap_realloci(heap, mam_ptr_dist(heap, ptr), size));
}
#define mam_heap_realloc(type, heap, ptr, size) ((type*)mam_heap_reallocn(heap, ptr, size))

#ifdef MAM_ALLOC_IMPLEMENTATION
#undef MAM_ALLOC_IMPLEMENTATION
MAM_ALLOC__DECLR mam_int mam_heap_alloci(MamHeap* heap, mam_int size) {
	size = mam_checker_correct_size(size);
	size = mam__heap_correct_size(size);
	mam_int original_i = heap->head_block;
	if(original_i) {
		mam_int cur_i = original_i;
		do {
			Mam__Block* cur_block = mam_ptr_add(Mam__Block, heap, cur_i);
			mam_int next_i = cur_block->free_next;
			heap->head_block = next_i;
			if(cur_block->size == size) {//remove cur_block from free list
				mam_int pre_i = cur_block->free_pre;
				if(next_i == cur_i) {//all items would be removed from free list
					heap->head_block = 0;
				} else {
					heap->head_block = next_i;
					mam_ptr_add(Mam__Block, heap, pre_i)->free_next = next_i;
					mam_ptr_add(Mam__Block, heap, next_i)->free_pre = pre_i;
				}
				cur_block->free_pre = 0;

				mam_checker_mark(heap, cur_i + sizeof(Mam__Block), size - sizeof(Mam__Block));
				return mam_checker_correct_item(cur_i + sizeof(Mam__Block));
			} else if(cur_block->size > size) {
				mam_int pre_i = cur_block->free_pre;

				mam_int fragment_i = cur_i + size;
				Mam__Block* fragment_block = mam_ptr_add(Mam__Block, heap, fragment_i);
				fragment_block->pre = cur_i;
				fragment_block->size = cur_block->size - size;
				cur_block->size = size;
				if(heap->end_block == cur_i) {
					heap->end_block = fragment_i;
				}

				if(next_i == cur_i) {//only one item in free list
					heap->head_block = fragment_i;
					fragment_block->free_pre = fragment_i;
					fragment_block->free_next = fragment_i;
				} else {
					fragment_block->free_pre = pre_i;
					fragment_block->free_next = next_i;
					mam_ptr_add(Mam__Block, heap, pre_i)->free_next = fragment_i;
					mam_ptr_add(Mam__Block, heap, next_i)->free_pre = fragment_i;
				}
				cur_block->free_pre = 0;

				mam_checker_mark(heap, cur_i + sizeof(Mam__Block), size - sizeof(Mam__Block));
				return mam_checker_correct_item(cur_i + sizeof(Mam__Block));
			}
			cur_i = next_i;
		} while(original_i != cur_i);
	}
	//no free space, push the size
	if(mam_will_overflow(heap, size)) {
		mam__grow(heap, size, sizeof(MamHeap));
		MAM_ALLOC_ASSERT(heap != 0, "mam_alloc: failed to reallocate memory for heap")
	}
	mam_int item = heap->mem_size;
	Mam__Block* block = mam_ptr_add(Mam__Block, heap, item);
	if(heap->end_block) {
		mam__block_set_next(mam_ptr_add(Mam__Block, heap, heap->end_block), heap->end_block, item);
		block->pre = heap->end_block;
	}
	block->size = size;
	block->free_pre = 0;
	heap->mem_size += size;
	heap->end_block = item;

	mam_checker_mark(heap, item + sizeof(Mam__Block), size - sizeof(Mam__Block));
	return mam_checker_correct_item(item + sizeof(Mam__Block));
}

MAM_ALLOC__DECLR void mam_heap_freei(MamHeap* heap, mam_int item)  {
	item = mam_checker_uncorrect_item(item);
	mam_int cur_i = item - sizeof(Mam__Block);
	Mam__Block* cur_block = mam_ptr_add(Mam__Block, heap, cur_i);
	mam_int pre_i = cur_block->pre;
	Mam__Block* pre_block = mam_ptr_add(Mam__Block, heap, pre_i);
	mam_int next_i = mam__block_get_next(cur_block, cur_i);
	Mam__Block* next_block = mam_ptr_add(Mam__Block, heap, next_i);

	MAM_ALLOC_ASSERT(!cur_block->free_pre, "mam_alloc: attempted to free freed memory");
	mam_checker_unmark(heap, item, cur_block->size - sizeof(Mam__Block));

	mam_int head_i = heap->head_block;
	if(pre_i && pre_block->free_pre) {//is free, combine
		if(heap->end_block == cur_i) {//is last block in the whole heap, remove from heap
			heap->end_block = pre_block->pre;
			heap->mem_size -= cur_block->size + pre_block->size;
			//remove pre_block
			mam_int free_pre_i = pre_block->free_pre;
			mam_int free_next_i = pre_block->free_next;
			if(head_i == pre_i) {
				if(pre_i == free_next_i) {//empty free list
					heap->head_block = 0;
				} else {
					heap->head_block = free_next_i;
					mam_ptr_add(Mam__Block, heap, free_pre_i)->free_next = free_next_i;
					mam_ptr_add(Mam__Block, heap, free_next_i)->free_pre = free_pre_i;
				}
			} else {
				mam_ptr_add(Mam__Block, heap, free_pre_i)->free_next = free_next_i;
				mam_ptr_add(Mam__Block, heap, free_next_i)->free_pre = free_pre_i;
			}
		} else if(next_block->free_pre) {//remove next_block and cur_block
			mam_int free_pre_i = next_block->free_pre;
			mam_int free_next_i = next_block->free_next;
			if(head_i == next_i) {
				heap->head_block = pre_i;
			}
			mam_ptr_add(Mam__Block, heap, free_next_i)->free_pre = free_pre_i;
			mam_ptr_add(Mam__Block, heap, free_pre_i)->free_next = free_next_i;
			mam__block_set_next(pre_block, pre_i, mam__block_get_next(next_block, next_i));
			mam_ptr_add(Mam__Block, heap, mam__block_get_next(next_block, next_i))->pre = pre_i;
		} else {//remove cur_block
			mam__block_set_next(pre_block, pre_i, next_i);
			next_block->pre = pre_i;
		}
	} else if(heap->end_block == cur_i) {//cur_block is last block in heap, remove it
		heap->end_block = pre_i;
		heap->mem_size -= cur_block->size;
	} else if(next_block->free_pre) {//inherit next_block's free list info
		mam_int free_pre_i = next_block->free_pre;
		mam_int free_next_i = next_block->free_next;
		if(head_i == next_i) {
			heap->head_block = cur_i;
			if(free_pre_i == next_i) {//next_block is only member of free list
				cur_block->free_pre = cur_i;
				cur_block->free_next = cur_i;
			} else {
				cur_block->free_pre = free_pre_i;
				cur_block->free_next = free_next_i;
				mam_ptr_add(Mam__Block, heap, free_next_i)->free_pre = cur_i;
				mam_ptr_add(Mam__Block, heap, free_pre_i)->free_next = cur_i;
			}
		} else {
			cur_block->free_pre = free_pre_i;
			cur_block->free_next = free_next_i;
			mam_ptr_add(Mam__Block, heap, free_next_i)->free_pre = cur_i;
			mam_ptr_add(Mam__Block, heap, free_pre_i)->free_next = cur_i;
		}
		//remove next_block
		mam__block_set_next(cur_block, cur_i, mam__block_get_next(next_block, next_i));
		mam_ptr_add(Mam__Block, heap, mam__block_get_next(next_block, next_i))->pre = cur_i;
	} else if(head_i) {//add cur_block to the free list
		Mam__Block* head_block = mam_ptr_add(Mam__Block, heap, head_i);
		mam_int last_i = head_block->free_pre;
		mam_ptr_add(Mam__Block, heap, last_i)->free_next = cur_i;
		head_block->free_pre = cur_i;
		cur_block->free_next = head_i;
		cur_block->free_pre = last_i;
	} else {//list is empty
		heap->head_block = cur_i;
		cur_block->free_pre = cur_i;
		cur_block->free_next = cur_i;
	}
}

MAM_ALLOC__DECLR mam_int mam_heap_realloci(MamHeap* heap, mam_int item, mam_int size) {
	mam_int cur_i = item - sizeof(Mam__Block);
	Mam__Block* cur_block = mam_ptr_add(Mam__Block, heap, cur_i);
	if(cur_block->size >= size) {
		return item;
	} else {
		mam_int new_item = mam_heap_alloci(heap, size);
		MAM_ALLOC_MEMCPY(mam_ptr_add(mam_byte*, heap, new_item), mam_ptr_add(mam_byte*, heap, item), cur_block->size);
		mam_heap_freei(heap, item);
		return new_item;
	}
}
#endif

#ifdef __cplusplus
}
#endif
#endif
