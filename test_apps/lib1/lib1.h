#ifndef _LIB1_H_

#if (defined WIN32 || defined _WIN32 || defined WINCE)
#ifdef TRACE_STATICLIB
# define ___LIB_API_
#else
# ifdef LIB_API_EXPORT
#  define ___LIB_API_ __declspec(dllexport)
# else
#  define ___LIB_API_ __declspec(dllimport)
# endif
#endif //ifdef TRACE_STATICLIB
#else
#define ___LIB_API_
#endif //#if (defined WIN32 || defined _WIN32 || defined WINCE)

namespace libone
{

___LIB_API_ void foo();

}

namespace libzero
{
___LIB_API_ void foo();
}

#endif
