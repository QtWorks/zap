//
// Created by Darren Otgaar on 2016/05/01.
//

#ifndef ZAP_POD_HPP
#define ZAP_POD_HPP

namespace zap { namespace core {

#include <cctype>

/*
 * Table Builder
 *
 * Allows executing arbitrary Meta functions at compile time to populate const static tables.  This allows the various
 * pod types and bitfields to be checked at compile-time rather than run-time.
 *
 */

template <size_t... Parms> struct data_table {
    static constexpr size_t data[sizeof...(Parms)] = { Parms... };
};

template <size_t... Parms> constexpr size_t data_table<Parms...>::data[sizeof...(Parms)];

template <size_t k, typename TEMPL, template <size_t, typename> class FNC, size_t... Parms>
struct table_gen_i {
    typedef typename table_gen_i<k-1, TEMPL, FNC, FNC<k, TEMPL>::value, Parms...>::result result;
};

template <typename TEMPL, template <size_t, typename> class FNC, size_t... Parms>
struct table_gen_i<0, TEMPL, FNC, Parms...> {
    typedef data_table<FNC<0, TEMPL>::value, Parms...> result;
};

template <size_t k, typename POD_T, template <size_t, typename> class FNC>
struct generate_table {
    typedef typename table_gen_i<k-1, POD_T, FNC>::result result;
};

/* Remove This Stuff */
enum class vertex_attribute : size_t {
    VA_POSITION,
            VA_NORMAL,
            VA_TEXCOORD,
            VA_TANGENT
};

bool operator==(vertex_attribute v, size_t s) { return size_t(v) == s; }
bool operator==(size_t s, vertex_attribute v) { return s == size_t(v); }

/* Up Above */

template <typename... Args>
struct pod {
};

/*
 * podfield represents an enumerated field.
 * T    - The stored type
 * ID_T - The enumeration type
 * ID   - The enumerator value for the instance
 *
 * Use MAKE_PODFIELD() to generate member names
 */

template <typename T, typename ID_T, ID_T ID>
struct podfield {
    using type = T;
    using id_t = ID_T;
    constexpr static id_t field_id = ID;
    podfield() = default;
    podfield(const T& v) : value(v) { }
    podfield(const podfield& r) : value(r.value) { }
    podfield& operator=(const podfield& r) { if(this != &r) { value = r.value; } return *this; }
    T value;
};

#define MAKE_PODFIELD(MemberName, ID_T, ID) \
template <typename T> struct podfield<T, ID_T, ID> { \
    static_assert(std::is_trivially_copyable<T>::value, "podfield<> only accepts trivially copyable types"); \
    using type = T; \
    using id_t = ID_T; \
    constexpr static id_t field_id = ID; \
    podfield() = default; \
    podfield(const T& v) : value(v) { } \
    union { \
        T MemberName; \
        T value; \
    }; \
}; \
template <typename T> \
using MemberName = podfield<T, ID_T, ID>;

/*
 * Generate a table of fields connected to a particular enumerator
 */

MAKE_PODFIELD(position, vertex_attribute, vertex_attribute::VA_POSITION)
MAKE_PODFIELD(normal,   vertex_attribute, vertex_attribute::VA_NORMAL)
MAKE_PODFIELD(tangent,  vertex_attribute, vertex_attribute::VA_TANGENT)
MAKE_PODFIELD(texcoord, vertex_attribute, vertex_attribute::VA_TEXCOORD)

template <typename Parm> struct is_dynafield : std::false_type { };
template <typename T, typename ID_T, ID_T ID> struct is_dynafield<podfield<T, ID_T, ID>> : std::true_type { };

template <typename Arg, typename... Args>
struct pod<Arg, Args...> : Arg, pod<Args...> {
    static_assert(is_dynafield<Arg>::value, "pod<> only accepts podfield<> parameters");
    constexpr pod() = default;
    pod(const Arg& arg, Args... args) : Arg(arg), pod<Args...>(args...) { }
};

// Now, how to query the values

template <size_t k, typename... Args> struct pod_query;

template <typename Arg, typename... Args>
struct pod_query<0, pod<Arg, Args...>> {
    using type = Arg;
    constexpr static size_t bytesize = sizeof(Arg);
};

template <size_t k, typename Arg, typename... Args>
struct pod_query<k, pod<Arg, Args...>> {
    using type = typename pod_query<k-1, pod<Args...>>::type;
    constexpr static size_t bytesize = pod_query<k-1, pod<Args...>>::bytesize;
};

template <size_t k, typename... Args>
constexpr typename std::enable_if<k==0, typename pod_query<0, pod<Args...>>::type>::type& get(pod<Args...>& D) {
return D;
};

template <size_t k, typename Arg, typename... Args>
constexpr typename std::enable_if<k!=0, typename pod_query<k, pod<Arg, Args...>>::type>::type& get(pod<Arg, Args...>& D) {
pod<Args...>& base = D;
return get<k-1>(base);
};

template <size_t k, typename POD_T> struct pod_offset;
template <typename POD_T> struct pod_offset<0,POD_T> {
    constexpr static size_t value = 0;
};

template <size_t k, typename POD_T> struct pod_offset {
    constexpr static size_t value = pod_query<k-1,POD_T>::bytesize + pod_offset<k-1,POD_T>::value;
};

template <size_t k, typename POD_T> struct pod_id;
template <typename POD_T> struct pod_id<0,POD_T> {
    enum {
        value = size_t(pod_query<0,POD_T>::type::field_id)
    };
};
template <size_t k, typename POD_T> struct pod_id {
    enum {
        value = size_t(pod_query<k,POD_T>::type::field_id)
    };
};

template <size_t k, typename POD_T>
struct pod_size {
    enum {
        value = pod_query<k,POD_T>::bytesize
    };
};

template <typename... Args>
struct mapping : pod<Args...> {
    static_assert(std::is_trivially_copyable<pod<Args...>>::value, "mapping<> must be trivially copyable");

    using pod_t = pod<Args...>;
    constexpr static size_t size = sizeof...(Args);
    constexpr static size_t bytesize = sizeof(pod_t);

    mapping() = default;
    mapping(Args... args) : pod<Args...>(args...) { }
};

}}

#endif //ZAP_POD_HPP
