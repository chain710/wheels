#ifndef _MULTI_QUEUE_
#define _MULTI_QUEUE_

#include <cstddef>

namespace wheels
{
    // forward declaration
    template<class T>
    class multi_queue_t;

    // queue iterator
    template<class T>
    class _mqueue_const_iterator_t
    {
    public:
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef multi_queue_t<T>* queue_pointer;
        typedef const multi_queue_t<T>* const_queue_pointer;

        _mqueue_const_iterator_t(int cidx, const_queue_pointer c):
            current_(cidx), container_(c)
        {
        }

        _mqueue_const_iterator_t(const _mqueue_const_iterator_t& c):
            current_(c.current_), container_(c.container_)
        {
        }

        _mqueue_const_iterator_t& operator++()
        {
            current_ = container_->get_next_id(current_);
            return *this;
        }

        _mqueue_const_iterator_t operator++(int)
        {
            _mqueue_const_iterator_t tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const _mqueue_const_iterator_t& right) const
        {
            return container_ == right.container_
                && current_ == right.current_;
        }

        bool operator!=(const _mqueue_const_iterator_t& right) const
        {
            return container_ != right.container_
                || current_ != right.current_;
        }

        const_reference operator*() const
        {
            return *container_->get(current_);
        }

        const_pointer operator->() const
        {
            return container_->get(current_);
        }

    protected:
        int current_;
        const_queue_pointer container_;
    };

    template<class T>
    class _mqueue_iterator_t: public _mqueue_const_iterator_t<T>
    {
    public:
        typedef _mqueue_const_iterator_t<T> base_t;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef multi_queue_t<T>* queue_pointer;
        typedef const multi_queue_t<T>* const_queue_pointer;

        _mqueue_iterator_t(int cidx, const_queue_pointer c):
            base_t(cidx, c)
        {
        }

        _mqueue_iterator_t(const _mqueue_iterator_t& c):
            base_t(c.current_, c.container_)
        {
        }

        reference operator*()
        {
            return *const_cast<queue_pointer>(base_t::container_)->get(base_t::current_);
        }

        pointer operator->()
        {
            return const_cast<queue_pointer>(base_t::container_)->get(base_t::current_);
        }
    };

    // multi queue
    template<typename T>
    class multi_queue_t
    {
    public:
        struct qnode_t
        {
            int qid_;   // queue id
            int prev_;
            int next_;
        };

        struct queue_t
        {
            int head_;
            int tail_;
            int num_;
        };

        typedef _mqueue_iterator_t<T> iterator_t;
        typedef _mqueue_const_iterator_t<T> const_iterator_t;

        // reserve queue(0) for internal use. actual queue num=max_list+1
        multi_queue_t(int node_capacity, int queue_capacity)
        {
            nodes_ = new qnode_t[node_capacity];
            data_ = new T[node_capacity];
            queues_ = new queue_t[queue_capacity+1];
            node_capacity_ = node_capacity;
            queue_capacity_ = queue_capacity;
            init_queue();
        }

        virtual ~multi_queue_t()
        {
            if (queues_)
            {
                delete []queues_;
                queues_ = NULL;
            }

            if (nodes_)
            {
                delete []nodes_;
                nodes_ = NULL;
            }

            if (data_)
            {
                delete []data_;
                data_ = NULL;
            }

            node_capacity_ = 0;
            queue_capacity_ = 0;
        }

        inline const_iterator_t cbegin(int qid) const
        {
            if (!is_valid_queue(qid))
            {
                return const_iterator_t(-1, NULL);
            }

            return const_iterator_t(queues_[qid].head_, this);
        }

        inline const_iterator_t cend(int qid) const
        {
            if (!is_valid_queue(qid))
            {
                return const_iterator_t(-1, NULL);
            }

            return const_iterator_t(-1, this);
        }

        inline iterator_t begin(int qid)
        {
            if (!is_valid_queue(qid))
            {
                return iterator_t(-1, NULL);
            }

            return iterator_t(queues_[qid].head_, this);
        }

        inline iterator_t end(int qid)
        {
            if (!is_valid_queue(qid))
            {
                return iterator_t(-1, NULL);
            }

            return iterator_t(-1, this);
        }

        // swap two elements in the same queue
        int swap(int left, int right)
        {
            if (!is_valid_index(left) || !is_valid_index(right)
                || nodes_[left].qid_ != nodes_[right].qid_)
            {
                return -1;
            }

            if (right == left) return 0;
            queue_t& queue = queues_[nodes_[left].qid_];

            int lp = nodes_[left].prev_;
            int ln = nodes_[left].next_;
            int rp = nodes_[right].prev_;
            int rn = nodes_[right].next_;
            if (is_valid_index(lp)) nodes_[lp].next_ = right;
            if (is_valid_index(ln)) nodes_[ln].prev_ = right;
            if (is_valid_index(rp)) nodes_[rp].next_ = left;
            if (is_valid_index(rn)) nodes_[rn].prev_ = left;

            nodes_[left].prev_ = (left == rp)? right: rp;
            nodes_[left].next_ = (left == rn)? right: rn;
            nodes_[right].prev_ = (right == lp)? left: lp;
            nodes_[right].next_ = (right == ln)? left: ln;
            if (left == queue.head_)
            {
                queue.head_ = right;
            }
            else if (right == queue.head_)
            {
                queue.head_ = left;
            }

            if (left == queue.tail_)
            {
                queue.tail_ = right;
            }
            else if (right == queue.tail_)
            {
                queue.tail_ = left;
            }

            return 0;
        }

        // move node from queue[qid].head to queue[0].tail
        int remove(int qid)
        {
            return move(qid, 0);
        }

        // move node from queue[0].head to queue[qid].tail
        int append(int qid)
        {
            return move(0, qid);
        }

        // move node from queue[src].head to queue[dst].tail
        int move(int src, int dst)
        {
            if (!is_valid_queue(src) || !is_valid_queue(dst))
            {
                return -1;
            }

            queue_t& src_queue = queues_[src];
            queue_t& dst_queue = queues_[dst];
            if (!is_valid_index(src_queue.head_) || dst_queue.tail_ >= node_capacity_)
            {
                return -1;
            }

            // remove it from src.head
            int midx = src_queue.head_;
            qnode_t& mnode = nodes_[midx];

            if (mnode.next_ < 0)
            {
                // empty
                init_queue(src_queue);
            }
            else
            {
                nodes_[mnode.next_].prev_ = -1;
                src_queue.head_ = mnode.next_;
                --src_queue.num_;
            }

            // append it to dst.tail
            if (dst_queue.tail_ < 0)
            {
                // empty, dst.head_ must < 0
                dst_queue.head_ = midx;
            }
            else
            {
                nodes_[dst_queue.tail_].next_ = midx;
            }

            mnode.prev_ = dst_queue.tail_;
            mnode.next_ = -1;
            mnode.qid_ = dst;
            dst_queue.tail_ = midx;
            ++dst_queue.num_;
            return 0;
        }

        // move src after dst, src & dst MUST in the same queue
        int move_after(int src, int dst)
        {
            if (!is_valid_index(src) || !is_valid_index(dst)
                || nodes_[src].qid_ != nodes_[dst].qid_)
            {
                return -1;
            }

            queue_t& flag = queues_[nodes_[src].qid_];
            if (src == dst) return 0;
            int sp = nodes_[src].prev_;
            int sn = nodes_[src].next_;

            if (sp == dst) return 0;

            if (is_valid_index(sp)) nodes_[sp].next_ = sn;
            if (is_valid_index(sn)) nodes_[sn].prev_ = sp;

            nodes_[src].prev_ = dst;
            nodes_[src].next_ = nodes_[dst].next_;
            nodes_[dst].next_ = src;

            if (src == flag.head_)
            {
                flag.head_ = sn;
            }

            if (src == flag.tail_)
            {
                flag.tail_ = sp;
            }
            else if (dst == flag.tail_)
            {
                flag.tail_ = src;
            }

            return 0;
        }

        // move src before dst, src & dst MUST in the same queue
        int move_before( int src, int dst )
        {
            if (!is_valid_index(src) || !is_valid_index(dst)
                || nodes_[src].qid_ != nodes_[dst].qid_)
            {
                return -1;
            }

            if (src == dst) return 0;
            queue_t& flag = queues_[nodes_[src].qid_];
            int sp = nodes_[src].prev_;
            int sn = nodes_[src].next_;

            if (sn == dst) return 0;

            if (is_valid_index(sp)) nodes_[sp].next_ = sn;
            if (is_valid_index(sn)) nodes_[sn].prev_ = sp;

            nodes_[src].prev_ = nodes_[dst].prev_;
            nodes_[src].next_ = dst;
            nodes_[dst].prev_ = src;

            if (src == flag.head_)
            {
                flag.head_ = sn;
            }
            else if (dst == flag.head_)
            {
                flag.head_ = src;
            }

            if (src == flag.tail_)
            {
                flag.tail_ = sp;
            }

            return 0;
        }

        T* get(int idx)
        {
            return is_valid_index(idx)? &data_[idx]: NULL;
        }

        const T* get(int idx) const
        {
            return is_valid_index(idx)? &data_[idx]: NULL;
        }

        int get_idx(const T* data) const
        {
            int idx = ((size_t)data - (size_t)data_) / sizeof(T);
            if (is_valid_index(idx) && &data_[idx] == data)
            {
                return idx;
            }

            return -1;
        }

        const T* get_next(const T* data) const
        {
            return get(get_next_id(get_idx(data)));
        }

        const T* get_prev(const T* data) const
        {
            return get(get_prev_id(get_idx(data)));
        }

        T* get_next(const T* data)
        {
            return get(get_next_id(get_idx(data)));
        }

        T* get_prev(const T* data)
        {
            return get(get_prev_id(get_idx(data)));
        }

        int get_next_id(int idx) const
        {
            return is_valid_index(idx)? nodes_[idx].next_: -1;
        }

        int get_prev_id(int idx) const
        {
            return is_valid_index(idx)? nodes_[idx].prev_: -1;
        }

        void reset()
        {
            init_queue();
        }

        int get_capacity() const
        {
            return node_capacity_;
        }

        inline int get_num(int qid) const
        {
            if (!is_valid_queue(qid)) return -1;
            return queues_[qid].num_;
        }

        inline int get_head(int qid) const
        {
            if (!is_valid_queue(qid)) return -1;
            return queues_[qid].head_;
        }

        inline int get_tail(int qid) const
        {
            if (!is_valid_queue(qid)) return -1;
            return queues_[qid].tail_;
        }

        inline int get_queue_id(int idx) const
        {
            if (!is_valid_index(idx)) return -1;
            return nodes_[idx].qid_;
        }

        static void init_queue(queue_t& q)
        {
            memset(&q, 0, sizeof(q));
            q.head_ = -1;
            q.tail_ = -1;
        }

    private:
        // deny copy-cons
        multi_queue_t(const multi_queue_t& l)
        {
        }

        void init_queue()
        {
            for (int i = 0; i <= queue_capacity_; ++i)
            {
                init_queue(queues_[i]);
            }

            // all belong to queue[0]
            queues_[0].head_ = 0;
            queues_[0].tail_ = node_capacity_ - 1;
            queues_[0].num_ = node_capacity_;
            for (int i = 0; i < node_capacity_; ++i)
            {
                nodes_[i].qid_ = 0;
                nodes_[i].prev_ = i - 1;
                nodes_[i].next_ = i + 1;
            }

            nodes_[node_capacity_ - 1].next_ = -1;
        }

        inline bool is_valid_index(int i) const
        {
            return i >= 0 && i < node_capacity_;
        }
        
        inline bool is_valid_queue(int i) const
        {
            return i >= 0 && i <= queue_capacity_;
        }

        queue_t* queues_;
        qnode_t* nodes_;
        T* data_;
        int node_capacity_;
        int queue_capacity_;
    };
}


#endif
