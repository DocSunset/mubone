#pragma once
#include <cstddef>
#include <tuple>

namespace mubone
{

template<class... Ts> class List {};
template<class... Ts> List(const Ts&...) -> List<Ts...>;
template<class H, class... Ts> List(const H&, const Ts&...) -> List<H, Ts...>;
template<class H, class... Ts> List(const H&, const List<Ts...>&) -> List<H, Ts...>;

template<class Head, class... Tail>
class List<Head, Tail...>
{
public:
    using head_type = Head;
    using tail_type = List<Tail...>;
    static constexpr std::size_t length = tail_type::length + 1;
    constexpr operator bool() {return true;}

    head_type head;
    tail_type tail;

    constexpr List() : head{}, tail{} {}
    constexpr List(const head_type& h, const Tail&... t) : head{h}, tail{t...} {}
    constexpr List(const head_type& h, const tail_type& t) : head{h}, tail{t} {}
};

template<> class List<>
{
public:
    static constexpr std::size_t length = 0;
    constexpr operator bool() {return false;}
};

// basic access
template<class T, class... Ts> constexpr T& head(List<T, Ts...>& l) {return l.head;}
template<class T, class... Ts> constexpr List<Ts...>& tail(List<T, Ts...>& l) {return l.tail;}
template<class T, class... Ts> constexpr const T& head(const List<T, Ts...>& l) {return l.head;}
template<class T, class... Ts> constexpr const List<Ts...>& tail(const List<T, Ts...>& l) {return l.tail;}

// list terminator checks
using Nil = List<>;

template<class list>
struct is_nil
{
    static constexpr bool value = std::is_same_v<list, Nil>;
};
template<class list>
inline constexpr bool is_nil_v = is_nil<list>::value;

template<class... Lists>
struct any_nil
{
    static constexpr bool value = (... && is_nil_v<Lists>);
};
template<class... Lists>
inline constexpr bool any_nil_v = any_nil<Lists...>::value;

// element access
template<class sought, class... Ts>
constexpr sought& get(List<Ts...>& l)
{
    if constexpr (is_nil_v<List<Ts...>>) throw "key not found";
    else if constexpr (std::is_same_v<sought, typename List<Ts...>::head_type>) return l.head;
    else return get<sought>(l.tail);
}

template<class sought, class... Ts>
constexpr const sought& get(const List<Ts...>& l)
{
    if constexpr (is_nil_v<List<Ts...>>) throw "key not found";
    else if constexpr (std::is_same_v<sought, typename List<Ts...>::head_type>) return l.head; 
    else return get<sought>(l.tail);
}

// useful functions
template<class func, class... Ts>
constexpr void for_each(List<Ts...>& l, func f)
{
    if constexpr (is_nil_v<List<Ts...>>) return;
    else
    {
        f(l.head);
        for_each(l.tail, f);
    }
}

template<class func, class... Ts>
constexpr auto map(const List<Ts...>& l, func f)
{
    if constexpr (is_nil_v<List<Ts...>>) return Nil{};
    else return List{f(l.head), map(l.tail, f)};
}

template<class... Lists>
constexpr auto zip(Lists&... lists)
{
    if constexpr (any_nil_v<Lists...>) return Nil{};
    else return List{std::make_tuple(head(lists) ...), zip(tail(lists) ...)};
}

} // namespace mubone
