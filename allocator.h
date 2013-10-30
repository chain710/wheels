#ifndef _WHEELS_ALLOCATOR_H_
#define _WHEELS_ALLOCATOR_H_

#include <cstddef>
#include <multi_queue.h>
#include <cstdint>

namespace wheels
{
    class fixed_size_allocator_t
    {
    private:
        enum qid_t
        {
            q_free = 0,
            q_used = 1,
        };
        
        struct block_t
        {
            uint32_t flag_;
            void* data_;
        };

        enum bflag_t
        {
            block_none = 0,
            block_used = 0x00000001,
        };

        typedef multi_queue_t<block_t> block_mqueue_t;

        int bsize_;
        int capacity_;
        block_mqueue_t* bqueue_;
        void* data_;

        // deny copy-cons
        fixed_size_allocator_t(const fixed_size_allocator_t& c);
        inline size_t address_to_idx(void* addr)
        {
            return ((char*)addr - (char*)data_) / bsize_;
        }
    public:
        fixed_size_allocator_t();
        ~fixed_size_allocator_t();
        
        // alloc one block, return the address
        void* alloc();
        // dealloc one block, return < 0 if error, coredump if fatal
        int free(void* p);

        int initialize(size_t bsize, size_t capacity);
        size_t get_bsize() const { return bsize_; }
        size_t get_capacity() const { return capacity_; }
        size_t get_used_num() const { return bqueue_->get_num(q_used); }
        size_t get_free_num() const { return capacity_ - get_used_num(); }
    };

    class allocator_t
    {
    public:
        struct allocator_info_t
        {
            size_t capacity_;
            size_t bsize_;
        };

        allocator_t();
        ~allocator_t();

        // alloc one block, return the address
        void* alloc(size_t sz);
        // dealloc one block, return < 0 if error, coredump if fatal
        int free(void* p);
        // realloc, return NULL if oom or error(you can still use p), non-NULL if succ, coredump if fatal
        void* realloc(size_t sz, void* p);

        int initialize(allocator_info_t* allocators);
    private:
        struct block_head_t
        {
            int guard_;
            int asize_; // allocator's bsize
            char data_[0];
        };

        // deny copy-cons
        allocator_t(const allocator_t& c);
        fixed_size_allocator_t* get_allocator(size_t bsize);
        fixed_size_allocator_t* find_best_allocator(size_t bsize);
        // return min allocator which >= bsize, -1 if bsize is larger than all allocators
        int get_ceiling_allocator(size_t bsize) const;

        inline bool less_bsize(int idx, size_t bsize) const
        {
            return (idx < 0)? true: (allocators_[idx].get_bsize() < bsize);
        }

        inline block_head_t* dataptr2bhead(void* p)
        {
            block_head_t* bh = (block_head_t*)((char*)p - offsetof(block_head_t, data_));
            return (bh && MAGIC_GUARD == bh->guard_)? bh: NULL;
        }

        fixed_size_allocator_t* allocators_;
        size_t anum_;
        const static int MAGIC_GUARD;
    };
}

#endif
