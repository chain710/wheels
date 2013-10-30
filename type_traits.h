#ifndef _WHEEL_TYPE_TRAITS_H_
#define _WHEEL_TYPE_TRAITS_H_

namespace wheels
{
    // from stl
    template<bool,
    class _Ty1,
    class _Ty2>
    struct _If
    {	// type is _Ty2 for assumed false
        typedef _Ty2 type;
    };

    template<class _Ty1,
    class _Ty2>
    struct _If<true, _Ty1, _Ty2>
    {	// type is _Ty1 for assumed true
        typedef _Ty1 type;
    };
}

#endif
