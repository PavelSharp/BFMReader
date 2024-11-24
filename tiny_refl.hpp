#pragma once
#include "pch.h"
#include "std_ext.hpp"

namespace std_ext {
    template <class T>
    struct type_name_t {
        consteval static std::string_view do_type_name() {
            const auto get_wrapped_name = []<class TT>() -> std::string_view { return std::source_location::current().function_name(); };
            const auto char_name = get_wrapped_name.template operator() < char > ();
            const auto void_name = get_wrapped_name.template operator() < void > ();
            const auto wrapped_name = get_wrapped_name.template operator() < T > ();
            const auto depth = std::ranges::mismatch(char_name, void_name).in1 - char_name.begin();
            return wrapped_name.substr(depth, wrapped_name.size() + 4 - char_name.size());
        }
        constexpr static auto value = do_type_name();
    };


    template <class T>
    constexpr std::string_view type_name_v = type_name_t<T>::value;

    template <class... Ts>
    constexpr void tp()
    {
        std::cout << "\n================\n";
        //(std::cout << ... << type_name<Ts>);
        (((std::cout << type_name_v<Ts>) << '\n'), ...);
        std::cout << "================\n";
    }


    template<class T, std::size_t max_fields>
    struct fields_count_t {
        using NR = std::remove_cvref_t<T>;
        constexpr static std::size_t ncount = std::numeric_limits<std::size_t>::max();
        //TODO Неправильно отработает struct sq { int fl1; int fl2;private:int fl3; };
        //Приватных полей не должно быть, решение - is_aggregate_v но тогда другие типы,
        //которые ранее были допустимы перестанут подходить. Подумать над std::standart_layout
        template<std::size_t Fields>
        consteval static auto internal_count() {
            if constexpr (Fields > 0)
                return can_construct<NR, Fields> ? Fields : internal_count<Fields - 1>();
            else
                return can_construct<NR, 0> ? 0 : ncount;
        }

        consteval static auto do_count() {
            if constexpr (tuple_like<NR>)
                return std::tuple_size_v<NR>;
            else if constexpr (can_construct<NR, max_fields + 1>)
                return ncount;
            else
                return internal_count<max_fields>();

        }

        consteval static auto do_assert_count() {
            constexpr auto res = do_count();
            static_assert(res != ncount, "Failed to count the number of fields");
            return res;
        }
    };

    template<class T, std::size_t max_fields = 32>
    constexpr auto fields_count = fields_count_t<T, max_fields>::do_assert_count();

    template<class T>
    struct any_size : std::integral_constant<std::size_t, fields_count<T>> { };

    template<class T>
    constexpr std::size_t any_size_v = any_size<T>::value;

    template <class Func, class T >
    constexpr decltype(auto) simple_apply(Func&& func, T&& obj)
    {
#include "macro_utils.hpp"
#define IDENT(x) ,x
#define FWD(x) ,forward_like<decltype(obj)>(x)
#define DO_FIELDS(...) \
            {auto& [REMOVE_FIRST_COMMA(FOREACH(IDENT,__VA_ARGS__))] = obj;\
            return std::invoke(std::forward<Func>(func), REMOVE_FIRST_COMMA(FOREACH(FWD,__VA_ARGS__)));}

        constexpr auto count = fields_count<T, 16>;
        if constexpr (count == 16)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16)
        else if constexpr (count == 15)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15)
        else if constexpr (count == 14)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14)
        else if constexpr (count == 13)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13)
        else if constexpr (count == 12)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12)
        else if constexpr (count == 11)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11)
        else if constexpr (count == 10)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10)
        else if constexpr (count == 9)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8, m9)
        else if constexpr (count == 8)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7, m8)
        else if constexpr (count == 7)
            DO_FIELDS(m1, m2, m3, m4, m5, m6, m7)
        else if constexpr (count == 6)
            DO_FIELDS(m1, m2, m3, m4, m5, m6)
        else if constexpr (count == 5)
            DO_FIELDS(m1, m2, m3, m4, m5)
        else if constexpr (count == 4)
            DO_FIELDS(m1, m2, m3, m4)
        else if constexpr (count == 3)
            DO_FIELDS(m1, m2, m3)
        else if constexpr (count == 2)
            DO_FIELDS(m1, m2)
        else if constexpr (count == 1)
            DO_FIELDS(m1)
        else
            return std::invoke(std::forward<Func>(func));

#undef IDENT
#undef FWD
#undef DO_FIELDS
#include "macro_utils_undef.hpp"
/*        using NR = std::remove_reference_t<T>;
        
#include "macro_utils.hpp"
#define ANYTYPE(x) ,any_type
#define IDENT(x) ,x
#define FWD(x) ,forward_like<decltype(obj)>(x)
#define DO_FIELDS(...) \
            if constexpr(is_brace_constructible_v<NR, REMOVE_FIRST_COMMA(FOREACH(ANYTYPE,__VA_ARGS__))>) \
            {auto& [REMOVE_FIRST_COMMA(FOREACH(IDENT,__VA_ARGS__))] = obj;\
            return std::invoke(std::forward<Func>(func), REMOVE_FIRST_COMMA(FOREACH(FWD,__VA_ARGS__)));} else

            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8, f9)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7, f8)
            DO_FIELDS(f1, f2, f3, f4, f5, f6, f7)
            DO_FIELDS(f1, f2, f3, f4, f5, f6)
            DO_FIELDS(f1, f2, f3, f4, f5)
            DO_FIELDS(f1, f2, f3, f4)
            DO_FIELDS(f1, f2, f3)
            DO_FIELDS(f1, f2)
            DO_FIELDS(f1)
            return std::invoke(std::forward<Func>(func));


#undef FWD
#undef IDENT
#undef ANYTYPE
#include "macro_utils_undef.hpp"*/
    }

    template <class Func, class T>
    constexpr decltype(auto) any_apply2(Func&& func, T&& obj) {
        if constexpr (tuple_like<T>) {
            return std::apply(std::forward<Func>(func), std::forward<T>(obj));
        }
        else {
            return simple_apply(std::forward<Func>(func), std::forward<T>(obj));
        }
    }



    namespace field_names_impl {
        template <typename T>
        struct wrapper
        {
            const T value;
            static const wrapper<T> fake;
        };

    //    template <class T>
      //  consteval const T& get_fake() { return wrapper<T>::fake.value; }
        template<class T>
        constexpr const T& to_fake = wrapper<T>::fake.value;

        template<class T>
        constexpr auto to_tuple = std_ext::any_apply2([](const auto&... flds) {return std::tie(flds...); }, to_fake<T>);

        //template<class T>
        //constexpr auto to_tuple2 = std_ext::any_apply2([](auto&&... flds) {return std::make_tuple(flds...); }, to_fake<T>);

        struct two_fld { int fld;int dlf; };

        template <auto P, class Tp, std::size_t ind>
        consteval std::string_view get_name() { return std::source_location::current().function_name(); }

        template<class InputIt1, class InputIt2>
        constexpr auto chmismatch(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, char ch)
        {
            std::size_t res = 0;
            while (first1 != last1 && first2 != last2) {
                if (*first1 == ch)
                    ++first1;
                else if (*first2 == ch)
                    ++first2;
                else
                    if (*first1 == *first2) {
                        ++first1, ++first2;
                        ++res;
                    }
                    else break;
              
            }
            return res;
        }

        consteval std::string_view extract(std::string_view str) {
            constexpr auto members = to_tuple<two_fld>;

            constexpr std::string_view fl1 = get_name<&std::get<0>(members), two_fld, 0>();
            constexpr std::string_view fl2 = get_name<&std::get<1>(members), two_fld, 1>();
            constexpr char space = ' ';
            auto depth = chmismatch(fl1.cbegin(), fl1.cend(), fl2.cbegin(), fl2.cend(), space);
            auto it = std::find_if_not(str.begin(), str.end(), [depth](char c) mutable -> bool { return c == space ? true : depth--; });
            //TODO индентификатор поля может ещё включать "Unicode character with the Unicode property XID_Continue"
            auto end_it = std::find_if_not(it, str.end(), [](char c) {return std_ext::ascii_isalpha(c) || std_ext::ascii_isdigit(c) || c=='_';});
            return std::string_view{ it, end_it };
        }

        template <std::integral auto Num>
        struct tuple_field_name {
            constexpr static std::string_view tup = "tup";
            constexpr static auto str_num = std_ext::compile_time::int2str_v<Num>;
            constexpr static auto value = std_ext::compile_time::strcat_v<tup, str_num>;
        };

        //Длина и тип массива указаны явно, теперь специализации для пустых типов работают естественно 
        template <class T, std::size_t... Ints>
        constexpr auto field_names_impl = std::array<std::string_view, sizeof...(Ints)>{ extract(get_name<&std::get<Ints>(to_tuple<T>), T, Ints>())... };

        template <class... Ts, std::size_t... Ints>
        constexpr auto field_names_impl<std::tuple<Ts...>, Ints...> = std::array<std::string_view, sizeof...(Ints)>{ tuple_field_name<Ints>::value... };

        template <class T, std::size_t... Ints>
        consteval auto get_field_names(std::index_sequence<Ints...>){return field_names_impl<T, Ints...>;}

        template <class T>
        constexpr auto field_names = get_field_names<T>(std::make_index_sequence<std_ext::fields_count<T>> {});
    }
    template <class T>
    constexpr auto field_names = field_names_impl::field_names<std::remove_cvref_t<T>>;

    namespace enum_names_impl {
        template <auto P>
        consteval std::string_view get_name() { return std::source_location::current().function_name(); }

        enum class scop_enum { val };

        consteval std::string_view enum_extract(std::string_view str) {
                
                constexpr std::string_view fl1 = get_name<scop_enum::val>();
                constexpr std::string_view fl2 = get_name<0>();
                constexpr char space = ' ';
                auto depth = field_names_impl::chmismatch(fl1.cbegin(), fl1.cend(), fl2.cbegin(), fl2.cend(), space);
                auto it = std::find_if_not(str.begin(), str.end(), [depth](char c) mutable -> bool { return c == space ? true : depth--; });
                auto end_it = std::find_if_not(it, str.end(), [](char c) {return std_ext::ascii_isalpha(c) || std_ext::ascii_isdigit(c) || c == '_' || c=='(' || c==')' || c==' ' || c==':';});
                return std::string_view{ it, end_it};
        }

        template <class T, T... Vals>
        constexpr auto enum_names_vals = std::array<std::string_view, sizeof...(Vals)>{ enum_extract(get_name<Vals>())... };

        template <class T, std::underlying_type_t<T>... Vals>
        consteval auto get_enum_names(std::integer_sequence<std::underlying_type_t<T>, Vals...>) { return enum_names_vals<T, static_cast<T>(Vals)...>; }
    }


    template <class T, auto first = static_cast<std::underlying_type_t<T>>(T::first), auto last = static_cast<std::underlying_type_t<T>>(T::last)>
    constexpr auto enum_values = enum_names_impl::get_enum_names<T>(make_integer_range<std::underlying_type_t<T>, first, last>{});

    template<class T>
    constexpr auto enum_name(T enm) {
        using UT = std::underlying_type_t<T>;
        constexpr bool can_convert = requires {{enum_values<T>};};
        if constexpr (can_convert)
            return enum_values<T>[static_cast<UT>(enm) - static_cast<UT>(T::first)];
        else
            return std::to_string(static_cast<UT>(enm));
    }

    template <class T, std::size_t ind>
    concept get_available = requires(T&& val) {{ get<ind>(std::forward<T>(val)) };};

    template <class T, class TF>
    concept get_types_available = requires(T && val) { { get<TF>(std::forward<T>(val)) }; };

    template <std::size_t I, get_available<I> T>
    constexpr decltype(auto) any_get(T&& val) {
        return get<I>(std::forward<T>(val));
    }
    template <class TF, get_types_available<TF> T>
    constexpr decltype(auto) any_get(T&& val) {
        return get<TF>(std::forward<T>(val));
    }
    template <std::size_t I, class T>
    constexpr decltype(auto) any_get(T& val) requires (!get_available<T, I>) {
        return any_apply2([](auto&... fs) -> decltype(auto) {return get<I>(std::tie(fs...));}, val);
    }
    template <class TF, class T>
    constexpr decltype(auto) any_get(T& val) requires (!get_types_available<T, TF>) {
        return any_apply2([](auto&... fs) -> decltype(auto) {return get<TF>(std::tie(fs...));}, val);
    }

   /* template<std::size_t I, class T>
    struct any_element {
        inline static auto f = [](auto&&... fs) -> decltype(auto) {return get<I>(std::tie(fs...));};
          using type = decltype(any_apply2<decltype(f), const T&>(f, std::declval<T>()));
    };*/



    namespace tests {
        using namespace std::literals;

        namespace apply {
            enum categrary {val, cval, lval, rval, clval, crval};
            template<class T>
            constexpr categrary get_category() {
                constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;

                if (std::is_lvalue_reference_v<T>)
                    return is_const ? clval : lval;
                else if (std::is_rvalue_reference_v<T>)
                    return is_const ? crval : rval;
                else
                    return is_const ? cval : val;

            }

            void test() {
                struct obj {
                    int data;
                    constexpr obj(int d):data(d){}
                    //constexpr obj(obj&) = delete;
                    //constexpr obj(obj&&) = delete;
                };
                struct data {
                    obj obj;
                };

                data d1{ obj{0} };
                const data d2{ obj{1} };

                //constexpr auto vv = any_apply2([]<class T>(auto q) {return get_category<decltype(q)>();  }, d1);
                constexpr auto lv = any_apply2([](auto&& q) {return get_category<decltype(q)>();  }, d1);
                constexpr auto clv = any_apply2([](auto&& q) {return get_category<decltype(q)>();  }, d2);
                constexpr auto rv = any_apply2([](auto&& q) {return get_category<decltype(q)>();  }, std::move(d1));
                constexpr auto crv = any_apply2([](auto&& q) {return get_category<decltype(q)>();  }, std::move(d2));
                
                static_assert(lv == lval && clv == clval && rv == rval && crv==crval, "apply failed");

            }

        }

        namespace filed_names {
            using tuple1 = std::tuple<char, float, std::vector<std::string>, char*>;
            constexpr static std::array tuple1_names{ "tup0"sv, "tup1"sv, "tup2"sv, "tup3"sv };

            using emp_tuple = std::tuple<>;
            constexpr static std::array<std::string_view, 0> emp_tuple_names{  };

            using iipair = std::pair<int, int>;
            constexpr static std::array iipair_names{ "first"sv, "second"sv };

            struct struct1 {
                int hi;
                double hello;
                std::vector<int> nice_to_meet_u;
                void random_method() {}
                //TODO DO NOT COMPILE. WHY? void virtual virt_method() {}
                constexpr static std::array names{ "hi"sv, "hello"sv, "nice_to_meet_u"sv };
            };

            struct struct2 {
                std::string field1;
                std::variant<int, double> field2;

                struct2(struct2&&) = delete;
                struct2(const struct2&) = delete;
                struct2& operator=(const struct2&) = delete;
                struct2& operator=(struct2&&) = delete;
                constexpr static std::array names{ "field1"sv, "field2"sv };
            };

            struct emp_struct {
                constexpr static std::array<std::string_view, 0> names{ };
            };

            struct struct3 {
                int ver_id, width, height, unkown1, mipmaps_exp2;
                char name[24];
                int parent_index;
                std::array<float, 9> arr;

                constexpr static std::array names{
                    "ver_id"sv, "width"sv, "height"sv, "unkown1"sv, "mipmaps_exp2"sv, "name"sv, "parent_index"sv, "arr"sv
                };
            };

            class struct4 {
            public:
                char* name;
                struct {
                    void* c;
                } nes43ted;
                int nm___4314___q;
                constexpr static std::array names{
                    "name"sv, "nes43ted"sv, "nm___4314___q"sv
                };
            };

            template<class T, const std::ranges::input_range auto& rng>
            consteval void tester() { static_assert(compile_time::is_equal_ranges_v<field_names<T>, rng>); }

            template<class T>
            consteval void struct_tester() { tester<T, T::names>(); }


            consteval void test_01() {
                struct_tester<struct1>();
                //TODO not working fields_tester<struct2>();
                struct_tester<emp_struct>();
                struct_tester<struct3>();
                struct_tester<struct4>();
                tester<tuple1, tuple1_names>();
                tester<emp_tuple, emp_tuple_names>();
                tester<iipair, iipair_names>();
            }
        }

        namespace enum_names {
            enum class scop_enum1 { item = -5, some_name, one_MORE_name, first = item, last = one_MORE_name};
            constexpr static std::array scop_enum1_names{ "std_ext::tests::enum_names::scop_enum1::item"sv, "std_ext::tests::enum_names::scop_enum1::some_name"sv, "std_ext::tests::enum_names::scop_enum1::one_MORE_name"sv};
            enum enum2 { item, some_name, one_MORE_name, first = item, last = one_MORE_name };
            constexpr static std::array enum2_names{ "std_ext::tests::enum_names::item"sv, "std_ext::tests::enum_names::some_name"sv, "std_ext::tests::enum_names::one_MORE_name"sv };

            template<class T, const std::ranges::input_range auto& rng>
            consteval void tester() { 
                static_assert(std_ext::compile_time::is_equal_ranges_v<enum_values<T>, rng>);
            }

            consteval void test() {
                tester<scop_enum1, scop_enum1_names>();
                tester<enum2, enum2_names>();
                
               // constexpr auto a = enum_name(scop_enum1::one_MORE_name);
               // static_assert(std_ext::compile_time::is_equal_ranges_v<a, scop_enum1_names[2]>);
              //  std::cout << ::std_ext::enum_name(scop_enum1::one_MORE_name);
             //   tester<enum_name() >();
                //tester<
            }

        }

    }
}