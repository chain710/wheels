#ifndef _SCOPED_GUARD_H_
#define _SCOPED_GUARD_H_

#include <cstddef>            // for std::ptrdiff_t
#include <assert.h>           // for assert

namespace wheels
{
    // code from google-url

    template <typename T, typename DELETER>
    class scoped_guard_t {
    private:

        T* ptr_;
        DELETER deleter_;
        scoped_guard_t(scoped_guard_t const &);
        scoped_guard_t & operator=(scoped_guard_t const &);

    public:

        typedef T element_type;
        typedef scoped_guard_t<T, DELETER> self_type;

        explicit scoped_guard_t(T* p = 0): ptr_(p) {}

        ~scoped_guard_t() {
            // about type_must_be_complete, see https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Checked_delete
            typedef char type_must_be_complete[sizeof(T)? 1: -1];
            deleter_(ptr_);
        }

        void reset(T* p = 0) {
            typedef char type_must_be_complete[sizeof(T)? 1: -1];

            if (ptr_ != p) {
                deleter_(ptr_);
                ptr_ = p;
            }
        }

        T& operator*() const {
            assert(ptr_ != 0);
            return *ptr_;
        }

        T& operator[](std::ptrdiff_t i) const {
            assert(ptr_ != 0);
            assert(i >= 0);
            return ptr_[i];
        }

        T* operator->() const  {
            assert(ptr_ != 0);
            return ptr_;
        }

        bool operator==(T* p) const {
            return ptr_ == p;
        }

        bool operator!=(T* p) const {
            return ptr_ != p;
        }

        T* get() const  {
            return ptr_;
        }

        void swap(scoped_guard_t & b) {
            T* tmp = b.ptr_;
            b.ptr_ = ptr_;
            ptr_ = tmp;
        }

        T* release() {
            T* tmp = ptr_;
            ptr_ = 0;
            return tmp;
        }

        DELETER& get_deleter() {
            return deleter_;
        }
    private:

        // no reason to use these: each scoped_ptr should have its own object
        bool operator==(self_type const& p) const;
        bool operator!=(self_type const& p) const;
    };

    template<typename T>
    struct _default_deleter_t
    {
        inline void operator()(T* p)
        {
            delete p;
        }
    };

    template<typename T>
    struct _default_array_t
    {
        inline void operator()(T* p)
        {
            delete []p;
        }
    };

    // in cxx11 we could replace following macro with using
#define scoped_ptr(T) scoped_guard_t< T, _default_deleter_t >
#define scoped_array(T) scoped_guard_t< T, _default_array_t >
}

#endif
