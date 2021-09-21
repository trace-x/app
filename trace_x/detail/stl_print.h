#ifndef _TRACE_X_STL_PRINT_
#define _TRACE_X_STL_PRINT_

#include <ostream>
#include <utility>
#include <iterator>
#include <set>

namespace stl_print_traits
{

struct anyx
{
    template <class T>
    anyx(const T &);
};

typedef char yes;
typedef char (&no)[2];

no operator << (const anyx &, const anyx &);

template <class T> yes detect(T const&);
no detect(no);

template <typename StreamType, typename T>
struct has_streaming_support
{
    static StreamType &stream;
    static T &x;
    static const bool value = (sizeof(detect(stream << x)) == sizeof(yes));
};

template<typename T>
struct support_ostream_operator : has_streaming_support<std::ostream, T> {};

template <bool, typename T> struct enable_if { };
template <typename T> struct enable_if<true, T> { typedef T type; };

} // namespace stl_print_traits

namespace trace_x_detail
{

inline std::ostream &operator <<(std::ostream &os, char c)
{
    return os << static_cast<int>(c);
}

inline std::ostream &operator <<(std::ostream &os, unsigned char c)
{
    return os << static_cast<unsigned int>(c);
}

template <bool, typename S, typename T> struct conditional { };
template <typename S, typename T> struct conditional<true,  S, T> { typedef S type; };
template <typename S, typename T> struct conditional<false, S, T> { typedef T type; };

// SFINAE type trait to detect whether T::const_iterator exists.

template<typename T>
struct has_const_iterator
{
private:
    typedef char                      yes;
    typedef struct { char array[2]; } no;

    template<typename C> static yes test(typename C::const_iterator*);
    template<typename C> static no  test(...);
public:
    static const bool value = sizeof(test<T>(0)) == sizeof(yes);
    typedef T type;
};

// SFINAE type trait to detect whether "T::const_iterator T::begin/end() const" exist.

template <typename T>
struct has_begin_end
{
    struct Dummy { typedef void const_iterator; };
    typedef typename conditional<has_const_iterator<T>::value, T, Dummy>::type TType;
    typedef typename TType::const_iterator iter;

    struct Fallback { iter begin() const; iter end() const; };
    struct Derived : TType, Fallback { };

    template<typename C, C> struct ChT;

    template<typename C> static char (&f(ChT<iter (Fallback::*)() const, &C::begin>*))[1];
    template<typename C> static char (&f(...))[2];
    template<typename C> static char (&g(ChT<iter (Fallback::*)() const, &C::end>*))[1];
    template<typename C> static char (&g(...))[2];

    static bool const beg_value = sizeof(f<Derived>(0)) == 2;
    static bool const end_value = sizeof(g<Derived>(0)) == 2;
};

// Basic is_container template; specialize to have value "true" for all desired container types

template<typename T> struct is_container { static const bool value = has_const_iterator<T>::value && has_begin_end<T>::beg_value && has_begin_end<T>::end_value; };

template<typename T, std::size_t N> struct is_container<T[N]> { static const bool value = true; };

template<std::size_t N> struct is_container<char[N]> { static const bool value = false; };

// Holds the delimiter values for a specific character type

template<typename TChar>
struct delimiters_values
{
    typedef TChar char_type;
    const TChar * prefix;
    const TChar * delimiter;
    const TChar * postfix;
};

// Defines the delimiter values for a specific container and character type

template<typename T, typename TChar>
struct delimiters
{
    typedef delimiters_values<TChar> type;
    static const type values;
};

// Default delimiters

template<typename T> struct delimiters<T, char> { static const delimiters_values<char> values; };
template<typename T> const delimiters_values<char> delimiters<T, char>::values = { "[", ", ", "]" };
template<typename T> struct delimiters<T, wchar_t> { static const delimiters_values<wchar_t> values; };
template<typename T> const delimiters_values<wchar_t> delimiters<T, wchar_t>::values = { L"[", L", ", L"]" };

// Delimiters for (multi)set and unordered_(multi)set

template<typename T, typename TComp, typename TAllocator>
struct delimiters< ::std::set<T, TComp, TAllocator>, char> { static const delimiters_values<char> values; };

template<typename T, typename TComp, typename TAllocator>
const delimiters_values<char> delimiters< ::std::set<T, TComp, TAllocator>, char>::values = { "{", ", ", "}" };

template<typename T, typename TComp, typename TAllocator>
struct delimiters< ::std::set<T, TComp, TAllocator>, wchar_t> { static const delimiters_values<wchar_t> values; };

template<typename T, typename TComp, typename TAllocator>
const delimiters_values<wchar_t> delimiters< ::std::set<T, TComp, TAllocator>, wchar_t>::values = { L"{", L", ", L"}" };

template<typename T, typename TComp, typename TAllocator>
struct delimiters< ::std::multiset<T, TComp, TAllocator>, char> { static const delimiters_values<char> values; };

template<typename T, typename TComp, typename TAllocator>
const delimiters_values<char> delimiters< ::std::multiset<T, TComp, TAllocator>, char>::values = { "{", ", ", "}" };

template<typename T, typename TComp, typename TAllocator>
struct delimiters< ::std::multiset<T, TComp, TAllocator>, wchar_t> { static const delimiters_values<wchar_t> values; };

template<typename T, typename TComp, typename TAllocator>
const delimiters_values<wchar_t> delimiters< ::std::multiset<T, TComp, TAllocator>, wchar_t>::values = { L"{", L", ", L"}" };

// Delimiters for pair (reused for tuple, see below)

template<typename T1, typename T2> struct delimiters< ::std::pair<T1, T2>, char> { static const delimiters_values<char> values; };
template<typename T1, typename T2> const delimiters_values<char> delimiters< ::std::pair<T1, T2>, char>::values = { "(", ", ", ")" };
template<typename T1, typename T2> struct delimiters< ::std::pair<T1, T2>, wchar_t> { static const delimiters_values<wchar_t> values; };
template<typename T1, typename T2> const delimiters_values<wchar_t> delimiters< ::std::pair<T1, T2>, wchar_t>::values = { L"(", L", ", L")" };

// Iterator microtrait class to handle C arrays uniformly

template <typename T> struct get_iterator { typedef typename T::const_iterator iter; };
template <typename T, std::size_t N> struct get_iterator<T[N]> { typedef const T * iter; };

template <typename T> typename stl_print_traits::enable_if<has_const_iterator<T>::value, typename T::const_iterator>::type begin(const T & c) { return c.begin(); }
template <typename T> typename stl_print_traits::enable_if<has_const_iterator<T>::value, typename T::const_iterator>::type end(const T & c) { return c.end(); }
template <typename T, size_t N> const T * begin(const T(&x)[N]) { return &x[0];     }
template <typename T, size_t N> const T * end  (const T(&x)[N]) { return &x[0] + N; }

// Prints a pair to the stream using delimiters from delimiters<std::pair<T1, T2>>.
template<typename T1, typename T2, typename TChar, typename TCharTraits>
inline std::basic_ostream<TChar, TCharTraits> & operator<<(std::basic_ostream<TChar, TCharTraits> & stream, const std::pair<T1, T2> & value)
{
    if (delimiters<std::pair<T1, T2>, TChar>::values.prefix != NULL)
        stream << delimiters<std::pair<T1, T2>, TChar>::values.prefix;

    stream << value.first;

    if (delimiters<std::pair<T1, T2>, TChar>::values.delimiter != NULL)
        stream << delimiters<std::pair<T1, T2>, TChar>::values.delimiter;

    stream << value.second;

    if (delimiters<std::pair<T1, T2>, TChar>::values.postfix != NULL)
        stream << delimiters<std::pair<T1, T2>, TChar>::values.postfix;

    return stream;
}

// Functor to print containers. You can use this directly if you want to specificy a non-default delimiters type.

template<typename T, typename TChar = char, typename TCharTraits = ::std::char_traits<TChar>, typename TDelimiters = delimiters<T, TChar> >
struct print_container_helper
{
    typedef TChar char_type;
    typedef TDelimiters delimiters_type;
    typedef std::basic_ostream<TChar, TCharTraits> ostream_type;
    typedef typename get_iterator<T>::iter TIter;

    print_container_helper(const T & container)
        : _container(container)
    {
    }

    inline void operator()(ostream_type & stream) const
    {
        if (delimiters_type::values.prefix != NULL)
            stream << delimiters_type::values.prefix;

        if (_container.begin() != _container.end())
            for (TIter it = _container.begin(), it_end = _container.end(); ; )
            {
                stream << *it;

                if (++it == it_end) break;

                if (delimiters_type::values.delimiter != NULL)
                    stream << delimiters_type::values.delimiter;
            }

        if (delimiters_type::values.postfix != NULL)
            stream << delimiters_type::values.postfix;
    }

private:
    const T & _container;
};

// Type-erasing helper class for easy use of custom delimiters.
// Requires TCharTraits = std::char_traits<TChar> and TChar = char or wchar_t, and MyDelims needs to be defined for TChar.
// Usage: "cout << custom_delims<MyDelims>(x)".

struct custom_delims_base
{
    virtual ~custom_delims_base() { }
    virtual std::ostream & stream(std::ostream &) = 0;
    virtual std::wostream & stream(std::wostream &) = 0;
};

template<typename T, typename Delims>
struct custom_delims_wrapper : public custom_delims_base
{
    custom_delims_wrapper(const T & t_) : t(t_) { }

    std::ostream & stream(std::ostream & s)
    {
        return s << print_container_helper<T, char, std::char_traits<char>, Delims>(t);
    }
    std::wostream & stream(std::wostream & s)
    {
        return s << print_container_helper<T, wchar_t, std::char_traits<wchar_t>, Delims>(t);
    }

private:
    const T & t;
};

template<typename Delims>
struct custom_delims
{
    template<typename Container> custom_delims(const Container & c) : base(new custom_delims_wrapper<Container, Delims>(c)) { }
    ~custom_delims() { delete base; }
    custom_delims_base * base;
};

template<typename TChar, typename TCharTraits, typename Delims>
inline std::basic_ostream<TChar, TCharTraits> & operator<<(std::basic_ostream<TChar, TCharTraits> & s, const custom_delims<Delims> & p)
{
    return p.base->stream(s);
}

// Template aliases for char and wchar_t delimiters
// Enable these if you have compiler support
//
// Implement as "template<T, C, A> const sdelims::type sdelims<std::set<T,C,A>>::values = { ... }."

//template<typename T> using pp_sdelims = delimiters<T, char>;
//template<typename T> using pp_wsdelims = delimiters<T, wchar_t>;

// Prints a print_container_helper to the specified stream.

template<typename T, typename TChar, typename TCharTraits, typename TDelimiters>
inline std::basic_ostream<TChar, TCharTraits> & operator<<(std::basic_ostream<TChar, TCharTraits> &stream,
                                                      const print_container_helper<T, TChar, TCharTraits, TDelimiters> &helper)
{
    helper(stream);
    return stream;
}

// Prints a container to the stream using default delimiters

template<typename T, typename TChar, typename TCharTraits>
inline typename stl_print_traits::enable_if< is_container<T>::value && stl_print_traits::support_ostream_operator<typename T::value_type>::value, std::basic_ostream<TChar, TCharTraits>&>::type
operator<<(std::basic_ostream<TChar, TCharTraits> &stream, const T &container)
{
    return stream << print_container_helper<T, TChar, TCharTraits>(container);
}

// A wrapper for raw C-style arrays. Usage: int arr[] = { 1, 2, 4, 8, 16 };  std::cout << wrap_array(arr) << ...

template<typename T>
struct array_wrapper_n
{
    typedef const T * const_iterator;
    typedef T value_type;

    array_wrapper_n(const T * const a, size_t n) : _array(a), _n(n) { }
    inline const_iterator begin() const { return _array; }
    inline const_iterator end() const { return _array + _n; }

private:
    const T * const _array;
    size_t _n;
};

template<typename T>
inline array_wrapper_n<T> print_array(const T * const a, size_t n)
{
    return array_wrapper_n<T>(a, n);
}

} // namespace trace_x_detail


namespace stl_print_traits
{

//Here we dublicates defined in trace_x_detail namespace shift operators for has_streaming_support works

template<typename T1, typename T2, typename TChar, typename TCharTraits>
inline std::basic_ostream<TChar, TCharTraits> & operator<<(std::basic_ostream<TChar, TCharTraits> &stream, const std::pair<T1, T2> &value)
{
    return trace_x_detail::operator <<(stream, value);
}

template<typename T, typename TChar, typename TCharTraits>
inline typename enable_if< trace_x_detail::is_container<T>::value && stl_print_traits::support_ostream_operator<typename T::value_type>::value, std::basic_ostream<TChar, TCharTraits>&>::type
operator<<(std::basic_ostream<TChar, TCharTraits> &stream, const T &container)
{
    return trace_x_detail::operator <<(stream, container);
}

} // namespace stl_print_traits

#endif // _TRACE_X_STL_PRINT_
