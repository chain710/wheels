#include <allocator.h>
#include <cstdlib>
#include <algorithm>

using namespace wheels;

const int allocator_t::MAGIC_GUARD = 0x343;

fixed_size_allocator_t::fixed_size_allocator_t():
    bsize_(0), capacity_(0), bqueue_(NULL), data_(NULL)
{

}

fixed_size_allocator_t::~fixed_size_allocator_t()
{
    if (bqueue_)
    {
        delete bqueue_;
        bqueue_ = NULL;
    }

    if (data_)
    {
        delete [](char*)data_;
        data_ = NULL;
    }
    
    bsize_ = 0;
    capacity_ = 0;
}

int fixed_size_allocator_t::initialize( size_t bsize, size_t capacity )
{
    if (bqueue_ || bsize <= 0 || capacity <= 0)
    {
        return -1;
    }

    bqueue_ = new block_mqueue_t(capacity, 2);
    data_ = new char[bsize*capacity];
    if (NULL == bqueue_ || NULL == data_)
    {
        return -1;
    }

    char* p = (char*)data_;
    for (block_mqueue_t::iterator_t it = bqueue_->begin(q_free); 
        it != bqueue_->end(q_free); ++it)
    {
        it->flag_ = block_none;
        it->data_ = p;
        p += bsize;
    }

    return 0;
}

void* fixed_size_allocator_t::alloc()
{
    int ret = bqueue_->append(q_used);
    if (ret < 0)
    {
        // out of memory
        return NULL;
    }

    int id = bqueue_->get_tail(q_used);
    block_t* b = bqueue_->get(id);
    b->flag_ |= block_used;
    return b->data_;
}

int fixed_size_allocator_t::free( void* p )
{
    if (NULL == p)
    {
        return 0;
    }

    size_t id = address_to_idx(p);
    block_t* b = bqueue_->get(id);
    if (NULL == b || b->data_ != p || block_used != (b->flag_ & block_used))
    {
        return -1;
    }

    int ret = bqueue_->swap(id, bqueue_->get_head(q_used));
    if (ret < 0)
    {
        // FATAL: should never happen!
        abort();
    }

    b->flag_ &= (uint32_t)(~block_used);
    if (bqueue_->remove(q_used) < 0)
    {
        // FATAL: should never happen!
        abort();
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
bool _cmp_allocinfo(const allocator_t::allocator_info_t& l, const allocator_t::allocator_info_t& r)
{
    return l.bsize_ < r.bsize_;
}

allocator_t::allocator_t():
    allocators_(NULL), anum_(0)
{
}

allocator_t::~allocator_t()
{
    if (allocators_)
    {
        delete []allocators_;
        allocators_ = NULL;
    }
    
    anum_ = 0;
}

int allocator_t::initialize( allocator_info_t* allocators )
{
    if (NULL == allocators)
    {
        return -1;
    }

    anum_ = 0;
    for (int i = 0; allocators[i].bsize_ != 0; ++i)
    {
        ++anum_;
    }

    std::sort(&allocators[0], &allocators[anum_ -1], _cmp_allocinfo);
    allocators_ = new fixed_size_allocator_t[anum_];
    if (NULL == allocators_)
    {
        return -1;
    }

    int ret;
    for (size_t i = 0; i < anum_; ++i)
    {
        ret = allocators_[i].initialize(allocators[i].bsize_+sizeof(block_head_t), allocators[i].capacity_);
        if (ret < 0)
        {
            return ret;
        }
    }

    return 0;
}

fixed_size_allocator_t* allocator_t::get_allocator( size_t bsize )
{
    int start = 0, end = anum_ -1;
    int mid = (start + end) / 2;
    while (end >= start)
    {
        fixed_size_allocator_t& fa = allocators_[mid];

        if (fa.get_bsize() == bsize)
        {
            return &fa;
        }

        if (bsize < fa.get_bsize())
        {
            end = mid - 1;
        }
        else
        {
            start = mid + 1;
        }
    }

    return NULL;
}

fixed_size_allocator_t* allocator_t::find_best_allocator( size_t bsize )
{
    int id = get_ceiling_allocator(bsize);
    if (id < 0)
    {
        return NULL;
    }

    for (size_t i = id; i < anum_; ++i)
    {
        if (allocators_[i].get_free_num() > 0)
        {
            return &allocators_[i];
        }
    }

    return NULL;
}

int allocator_t::get_ceiling_allocator( size_t bsize ) const
{
    // binary search
    int start = 0, end = anum_ -1;
    int mid = (start + end) / 2;
    while (end >= start)
    {
        fixed_size_allocator_t& fa = allocators_[mid];

        if (fa.get_bsize() == bsize || (fa.get_bsize() > bsize && less_bsize(mid-1, bsize)))
        {
            return mid;
        }

        if (bsize < fa.get_bsize())
        {
            end = mid - 1;
        }
        else
        {
            start = mid + 1;
        }
    }

    return -1;
}

void* allocator_t::alloc( size_t sz )
{
    fixed_size_allocator_t* fa = find_best_allocator(sz+sizeof(block_head_t));
    if (NULL == fa)
    {
        return NULL;
    }

    block_head_t* bh = (block_head_t*)fa->alloc();
    if (bh)
    {
        bh->asize_ = fa->get_bsize();
        bh->guard_ = MAGIC_GUARD;
        return bh->data_;
    }

    return NULL;
}

int allocator_t::free( void* p )
{
    if (NULL == p)
    {
        return 0;
    }

    block_head_t* bh = dataptr2bhead(p);
    if (NULL == bh)
    {
        return -1;
    }

    fixed_size_allocator_t* fa = get_allocator(bh->asize_);
    if (NULL == fa)
    {
        return -1;
    }

    return fa->free(p);
}

void* allocator_t::realloc( size_t sz, void* p )
{
    block_head_t* bh = dataptr2bhead(p);
    if (NULL == bh)
    {
        return NULL;
    }

    if (bh->asize_ >= (int)(sz + sizeof(block_head_t)))
    {
        return p;
    }

    void* n = alloc(sz);
    if (NULL == n)
    {
        return NULL;
    }

    memcpy(n, bh->data_, bh->asize_ - sizeof(block_head_t));
    int ret = free(p);
    if (ret < 0)
    {
        // this should never happen
        abort();
    }

    return n;
}

