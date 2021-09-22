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

#include <iostream>

namespace libzero
{

class ___LIB_API_ Singletone
{
public:
    Singletone();

    void close()
    {
        std::cerr << "close()" << std::endl;
    }

    static Singletone * instance()
    {
        static Singletone sone;

        return &sone;
    }

private:
    int t;
    char buf[100];
};

___LIB_API_ void foo();

}

#endif
