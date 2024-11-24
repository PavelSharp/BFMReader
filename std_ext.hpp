#pragma once
#include "pch.h"


//todo. zip функция - для движения одновремменно по нескольким итератором внутри range-based for
namespace std_ext {

    constexpr bool ascii_isalpha(char c) {
        return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
    }
    constexpr bool ascii_isdigit(char c) {
        return c >= '0' && c <= '9';
    }

    template<class OutputIt, class Size, class Generator>
    constexpr OutputIt generate_ni(OutputIt first, Size count, Generator g)
    {
        Size i{ 0 };
        return std::generate_n(first, count, [&]() {return g(i++);});
        //for (Size i = 0; i < count; ++i, ++first)
         //   *first = g(i);

        //return first;
    }

    template<class InputIt, class UnaryOp>
    auto transform_newi(InputIt first, InputIt last, UnaryOp unary_op)
    {
        auto end = new decltype(unary_op(*first,0))[std::distance(first, last)];
        const auto ptr = end;
        for (std::size_t i = 0; first != last;++i)
            *end++ = unary_op(*first++, i);
        return ptr;
    }
    template<class Container, class UnaryOp>
    auto transform_newi(const Container& cont, UnaryOp unary_op)
    {
        return  transform_newi(cont.begin(), cont.end(), unary_op);
    }

    template<class InputIt, class UnaryOp>
    auto transform_new(InputIt first, InputIt last, UnaryOp unary_op)
    {
        auto end = new decltype(unary_op(*first))[std::distance(first, last)];
        const auto ptr = end;
        std::transform(first, last, ptr, unary_op);
        return ptr;
    }
    template<class Container, class UnaryOp>
    auto transform_new(const Container& cont, UnaryOp unary_op)
    {
        return  transform_new(cont.begin(), cont.end(), unary_op);
    }

    template<typename T, typename Size>
    T* copy_new2(const T* ptr, Size sz, Size sz2) {
        T* out = new T[sz2];
        std::copy(ptr, ptr + sz, out);
        return out;
    }

    template<typename T, typename Size>
    T* copy_new(const T* ptr, Size sz) {
        T* out = new T[sz];
        std::copy(ptr, ptr + sz, out);
        return out;
    }

    template<typename Cont>
    auto copy_new(const Cont& c) {
        return copy_new(c.data(), c.size());
    }
    template <typename T>
    T* numerate_new(const T count) {
        T* res = new T[count];
        for (T i = 0;i < count;++i) {
            res[i] = i;
        }
        return res;
    }

    template <typename T, typename TSize>
    T* concat_new(T* ptr1, TSize count1, T* ptr2, TSize count2) {
        T* res = new T[count1+count2];
        std::copy(ptr1, ptr1 + count1, res);
        std::copy(ptr2, ptr2 + count2, res+count1);//todo +1?
        return res;
    }

    bool ends_with_ic(std::string_view main, std::string_view s) {
        if (main.length() < s.length())
            return false;

        auto pair = std::mismatch(main.end() - s.length(), main.end(), s.begin(), s.end(), [](unsigned char l, unsigned char r) {return std::toupper(l) == std::toupper(r);});
        return pair.first == main.end();
    }

    template <typename... Args>
    struct add_tuple_pointer {
    };
    template <typename... Args>
    struct add_tuple_pointer<std::tuple<Args...>> {
        using type = std::tuple<std::add_pointer_t<Args>...>;
    };

    template<class Container, class UnaryOp>
    auto split_many(const Container& cont, UnaryOp op)
    {
        using tup = decltype(op(cont.front()));
        typename add_tuple_pointer<tup>::type res{};

        std::apply([&](auto&&... args) {((args = new std::remove_reference_t<decltype(*args)>[cont.size()]), ...);}, res);
        auto begin_ptr = res;

        const auto writer = [&res]<std::size_t... Is>(const tup & data, std::index_sequence<Is...>) {
            ((*std::get<Is>(res)++ = std::get<Is>(data)), ...);
        };
        for (auto& elem : cont)
        {
            writer(op(elem), std::make_index_sequence<std::tuple_size_v<tup>>{});
        }

        return begin_ptr;
    }

    template<class InpIt, class OutIt, class Pred>
    constexpr void copy_while(InpIt first, InpIt last, OutIt dest, Pred pred) {
        while ((first != last) && pred(*first)) {
            *dest++ = *first++;
        }
    }

    namespace fs = std::filesystem;
    template<class Func>
    void iterate_rec_files(const fs::path& path, Func f) {
        for (const fs::directory_entry& dir : fs::recursive_directory_iterator(path))
        {
            if (dir.is_regular_file())
                f(dir);
        }
    }
    namespace fs = std::filesystem;
    template<class Func>
    void iterate_files(const fs::path& path, Func f) {
        for (const fs::directory_entry& dir : fs::directory_iterator(path))
        {
            if (dir.is_regular_file())
                f(dir);
        }
    }

    //TODO if c++23 using built-in
    template<class T, class U>
    [[nodiscard]] constexpr auto&& forward_like(U&& x) noexcept
    {
        constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
        if constexpr (std::is_lvalue_reference_v<T&&>)
        {
            if constexpr (is_adding_const)
                return std::as_const(x);
            else
                return static_cast<U&>(x);
        }
        else
        {
            if constexpr (is_adding_const)
                return std::move(std::as_const(x));
            else
                return std::move(x);
        }
    }

    /*//TODO if c++23 using built-in
    template<typename E>
    struct is_scoped_enum : std::bool_constant < requires
    {
        requires std::is_enum_v<E>;
        requires !std::is_convertible_v<E, std::underlying_type_t<E>>;
    } > {};
    template< class T >
    inline constexpr bool is_scoped_enum_v = is_scoped_enum<T>::value;

    template<class T>
    inline constexpr bool is_unscoped_enum_v = std::is_enum_v<T> && !is_scoped_enum_v<T>;*/


    template<class T, T Min, T Max>
    using make_integer_range =
        decltype([] <T... Is> (std::integer_sequence<T, Is...>) {
        return std::integer_sequence<T, Is + Min...>{};
    }(std::make_integer_sequence<T, Max - Min + 1>{}));


    template <class T, class... Ts>
    concept is_any_same = std::disjunction<std::is_same<T, Ts>...>::value;

    template <class T, class... Ts>
    concept is_all_same = std::conjunction<std::is_same<T, Ts>...>::value;

    template<class T>
    concept is_char_type = std_ext::is_any_same<T, char, char8_t, char16_t, char32_t, wchar_t>;

    template<class T>
    concept ptrsz_range = std::ranges::contiguous_range<T> && std::ranges::sized_range<T>;

    template<class T>
    concept number = std::integral<T> || std::floating_point<T>;

    /*//TODO вызов std::make_integer_sequence делается по скобкам {} но здесь по скобкам ()
    template<typename T, T Min, T Max>
    constexpr auto make_integer_range()
    {
        static_assert (Max >= Min);
        return[] <T... Is> (std::integer_sequence<T, Is...>)
        {
            return std::integer_sequence<T, Is + Min...>{};
        }(std::make_integer_sequence<T, Max - Min + 1>{});
    }*/

    template <class T> struct is_stdarray : std::false_type {};
    template <class T, std::size_t N> struct is_stdarray<std::array<T, N>> : std::true_type { using array_t = std::array<T, N>;};


    struct any_type {
        template <class T>
        constexpr operator T& ();

         template <class T>
         constexpr operator T&& ();
    };


    //TODO is_brace_constructible_v<std::array<int, 2>, std_ext::any_type, std_ext::any_type>
    //TODO struct base_st {int a; int b;}; struct st : base_st {int c;}; is_brace_constructible_v<st, any_type, any_type, any_type>
    //TODO struct sq2 { std::unique_ptr<int> fl1, fl2;};  is_brace_constructible_v<sq2, any_type, any_type>
    /*TODO  is_brace_constructible_v<sq3, any_type>
    struct sq3 {
    int fl1;
    sq3(int a) {};
    sq3(sq3&&) {};

    sq3(sq3&) = delete;
    sq3& operator=(const sq3&) = delete;

    };

    struct sq2 {
       sq3 fl1, fl2;
    };
    */
    template< class T, class ...Args >
    inline constexpr bool is_brace_constructible = requires {T{ std::declval<Args>()...};};

    template< class T >
    inline constexpr bool is_sum_sizeof_v = std::is_fundamental_v<T> || std::is_enum_v<T>;
  
    template< class T>
    using is_sum_sizeof = std::is_fundamental<T>;

    template <typename T>
    constexpr bool only_one_field() {
        using an = any_type;
        //TODO static assert for trivial and aggregate
        if constexpr (!is_brace_constructible<T, an, an> && is_brace_constructible<T, an>) {
            return true;
        }
        else return false;

    }

    template <typename TFunc, typename T>
    constexpr void any_apply(TFunc call, T& obj)
    {
        using type = std::decay_t<T>;
        using an = any_type;
        //TODO Таким образм мы получим все поля ТОЛЬКО для агрегатных типов
       static_assert(!is_brace_constructible<type, an, an, an, an, an, an, an, an, an, an, an, an, an, an, an, an>);
        if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an, an, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = obj;
               call(a, b, c, d, e, f, g, h, i, j, k, l, m, n ,o);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g, h, i, j, k, l, m, n] = obj;
               call(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g, h, i, j, k, l, m] = obj;
               call(a, b, c, d, e, f, g, h, i, j, k, l, m);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g, h, i, j, k, l] = obj;
               call(a, b, c, d, e, f, g, h, i, j, k, l);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g, h, i, j, k] = obj;
               call(a, b, c, d, e, f, g, h, i, j, k);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g, h, i, j] = obj;
               call(a, b, c, d, e, f, g, h, i, j);
        } else if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g, h, i] = obj;
               call(a, b, c, d, e, f, g, h, i);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g, h] = obj;
               call(a, b, c, d, e, f, g, h);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f, g] = obj;
               call(a, b, c, d, e, f, g);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an, an>) {
            auto&& [a, b, c, d, e, f] = obj;
               call(a, b, c, d, e, f);
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an, an>) {
            auto&& [a, b, c, d, e] = obj;
               call(a, b, c, d, e);//todo std::forward
        }
        else if constexpr (is_brace_constructible<type, an, an, an, an>) {
            auto&& [a, b, c, d] = obj;
               call(a, b, c, d);
        }
        else if constexpr (is_brace_constructible<type, an, an, an>) {
            auto&& [a, b, c] = obj;
               call(a, b, c);
        }
        else if constexpr (is_brace_constructible<type, an, an>) {
            auto&& [a, b] = obj;
               call(a, b);
        }
        else if constexpr (is_brace_constructible<type, an>) {
            auto&& [a] = obj;
               call(a);
        } //else if constexpr (is_brace_constructible_v<type>) {
           // call();
       // }
    }
   
    //TODO Напр, структура struct {int a; double b;} отвечает требованиям тривиальности но в случае обратного порядка байтов будет 
    //прочитана неправильно. Следовательно, внести более строгие ограничения на тип To - арифметический или enum 
    template<class To, std::endian InpEnd = std::endian::native, class TByte>
        requires ((std::is_same_v<TByte, char> || std::is_same_v<TByte, unsigned char> || std::is_same_v<TByte, std::byte>)
    && std::is_trivially_copyable_v<To>)
        To from_bytes(const TByte* inp, std::size_t size=sizeof(To)) {
        To res;
        if constexpr (InpEnd == std::endian::native)
            std::memcpy(&res, inp, size);
        else
            std::reverse_copy(inp, inp + size, reinterpret_cast<TByte*>(&res));
        return res;
    }










    template<class... Types>
    struct type_sequence {
        consteval size_t size() noexcept { return sizeof...(Types); }
    };
    template<class T, size_t N, class... Types>
    struct make_type_sequence_impl : make_type_sequence_impl<T, N - 1, T, Types...> {};
    template<class T, class... Types>
    struct make_type_sequence_impl<T, 0, Types...> {
        using type = type_sequence<Types...>;
    };
    template<class T, size_t N>
    using make_type_sequence = make_type_sequence_impl<T, N>::type;

    template<class T, std::size_t count, class AnyType=any_type>
    constexpr bool can_construct = 
        []<class... Types>(type_sequence<Types...>) { return is_brace_constructible<T, Types...>; }
        (make_type_sequence <AnyType, count > {});


        template <typename T, std::size_t N>
        concept is_tuple_element = requires (T t) {
            typename std::tuple_element_t<N, std::remove_const_t<T>>;
            { get<N>(t) } -> std::convertible_to<std::tuple_element_t<N, T>&>;
        };
        template <typename T>
        concept is_tuple_like_impl = requires {
            typename std::tuple_size<T>::type;
            std::same_as<decltype(std::tuple_size_v<T>), std::size_t>;
        }&& []<std::size_t... I>(std::index_sequence<I...>)
        {
            return (is_tuple_element<T, I> && ...);
        }(std::make_index_sequence<std::tuple_size_v<T>>{});

        template<class T>
        concept tuple_like = is_tuple_like_impl<std::remove_cvref_t<T>>;

        template <typename T>
        concept pair_like = tuple_like<T> && std::tuple_size_v<std::remove_cvref_t<T>> == 2;

;




    
    namespace compile_time {

        template<const std::string_view&... vals>
        struct strcat_t {
            consteval static auto do_cat() {
                constexpr std::size_t len = (... + vals.size());
                std::array<char, len + 1> arr;
                auto it = arr.begin();
                const auto con = [&it](std::string_view s) {
                    it = std::copy(s.begin(), s.end(), it);
                    };
                (..., con(vals));
                arr[len] = '\0';
                return arr;
            }

            static constexpr auto arr = do_cat();
            static constexpr std::string_view value{ arr.data(), arr.size() - 1 };
        };

        template <const std::string_view&... vals>
        static constexpr auto strcat_v = strcat_t<vals...>::value;

        template<const std::ranges::input_range auto& lhs, const std::ranges::input_range auto& rhs>
        struct is_equal_ranges_t {
            consteval static bool do_equal() {
                return std::ranges::equal(lhs, rhs);
            }
            static constexpr auto value = do_equal();
        };

        template <const std::ranges::input_range auto& lhs, const std::ranges::input_range auto& rhs>
        static constexpr auto is_equal_ranges_v = is_equal_ranges_t<lhs, rhs>::value;


        template <char... Is>
        struct crstr_t {
            constexpr static char chars[] = { Is..., '\0' };
            constexpr static std::string_view value{ chars, sizeof...(Is) };
        };

        template <char... Is>
        static constexpr auto crstr_v = crstr_t<Is...>::value;

        template<std::integral auto num, unsigned int Base = 10, bool must_sign = false>
        struct formatter {
            constexpr static auto base = Base;

            template <char... Is>
            consteval static auto add_sign() {
                if constexpr (std::is_signed_v<decltype(num)> && num < 0)
                    return crstr_v<'-', Is...>;
                else
                    if constexpr (must_sign)
                        return crstr_v<'+', Is...>;
                    else
                        return crstr_v<Is...>;
            }

            template <char... Is>
            consteval static auto format() {
                static_assert(base == 2 || base == 8 || base == 10 || base == 16);
                switch (base)
                {
                case 2: return add_sign<'0', 'b', Is...>();
                case 8: return add_sign<'0', Is...>();
                case 16: return add_sign<'0', 'x', Is...>();
                default: return add_sign<Is...>();
                }
            }

            consteval static auto format_bool() {
                return num ? crstr_v<'t', 'r', 'u', 'e'> : crstr_v<'f', 'a', 'l', 's', 'e'>;
            }
        };


        template <class Formatter>
        struct num2str_t {
            template <std::integral TNum, TNum Loc, char... Is>
            consteval static auto do_int2str() requires !std::is_same_v<TNum, bool> {
                constexpr auto next = Loc / static_cast<TNum>(Formatter::base);
                constexpr auto pg = static_cast<int>(Loc % static_cast<TNum>(Formatter::base));
                constexpr auto dig = '0' + (pg < 0 ? (-pg) : pg);
                if constexpr (next == 0)
                    return   Formatter::template format<dig, Is...>();
                else
                    return do_int2str<TNum, next, dig, Is...>();
            }

            template <std::integral TNum, TNum Loc>
            consteval static auto do_int2str() requires std::is_same_v<TNum, bool> {
                return Formatter::format_bool();
            }
        };

        template <std::integral auto Num, class Formatter = formatter<Num>>
        constexpr auto int2str_v = num2str_t<Formatter>::template do_int2str<decltype(Num), Num>();
    }
}
