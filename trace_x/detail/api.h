#ifndef TRACE_X_API_H
#define TRACE_X_API_H

#if (defined WIN32 || defined _WIN32 || defined WINCE)
#ifdef TRACE_STATICLIB
# define TRACE_X_API
#else
# ifdef TRACE_API_EXPORT
#  define TRACE_X_API __declspec(dllexport)
# else
#  define TRACE_X_API __declspec(dllimport)
# endif
#endif //ifdef TRACE_STATICLIB
#else
#define TRACE_X_API
#endif //#if (defined WIN32 || defined _WIN32 || defined WINCE)

#endif //#ifndef TRACE_X_API_H
