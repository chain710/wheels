#ifndef _WHEELS_CUSTOM_NEW_H_
#define _WHEELS_CUSTOM_NEW_H_

#include <new>

template<class T>
void* operator new(std::size_t sz, T& m)
{
    return m.alloc(sz);
}

template<class T>
void* operator new[](std::size_t sz, T& m)
{
    return m.alloc(sz);
}

template<class T>
void operator delete(void* p, T& m)
{
    m.free(p);
}

template<class T>
void operator delete[](void* p, T& m)
{
    m.free(p);
}

#endif
