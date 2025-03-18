#pragma once
#include "pch.h"
#include "std_ext.hpp"

//TODO пересмотреть reinterpret_cast. Нет ли UB? 
//std::launder  std::bitcast 
//std::aligned std::assume_aligned
//TODO стоит ли вообще при (де)сериализации использовать знаковые типы? их представление в памяти не стандартизировано?

namespace sern {

    namespace impl_details {
        template <class T, class... Args>
        concept manual_read = requires(std::istream & is, Args&&... args) {
            { T::sern_read(is, std::forward<Args>(args)...) };
        };

        template <class T, class... Args>
        concept manual_write = requires(std::ostream & is, Args&&... args) {
            { std::declval<T>().sern_write(is, std::forward<Args>(args)...) };
        };

        template <class TOS, class T, class... Args>
        concept ostream_type_writeable = requires(TOS & os, const T & val, Args&&... args) {
            { os.write(val, std::forward<Args>(args)...) };
        };

        template <class TOS, class T, class Size, class... Args>
        concept ostream_write_array = requires(TOS & os, T val, Size size, Args&&... args) {
            { os.write(val, size, std::forward<Args>(args)...) };
        };


        template <typename T>
        concept tuple_like = requires (T x) {
            typename std::tuple_size<T>::type;
            typename std::tuple_element<0, T>::type;
        };

        template <class T>
        concept fixed_arithmetic = std::is_arithmetic_v<T> || std::is_enum_v<T>;

        template <typename T>
        concept fixed_array = std_ext::is_stdarray<T>::value;

        template <typename T>
        concept fixed_type = fixed_array<T> || fixed_arithmetic<T>;

        template <typename T>
        concept dynamic_array = std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<typename T::value_type, typename T::allocator_type>>;

        template<class T>
        constexpr std::size_t get_optimize_sizeof() {
            if constexpr (fixed_arithmetic<T>) {
                return sizeof(T);
            }
            else if constexpr (fixed_array<T>) {
                //Рекурсивный обход для того, чтобы убедиться, что листовой тип - арифметический тип
                return std::size(T{}) * get_optimize_sizeof<std::iter_value_t<T>>();
            }
            else return 0;
        }

        template <class T>
        concept can_optimize_read = (get_optimize_sizeof<T>() > 0);
    }


    template<class T>
    struct reader {
        template<class Size, class... Args>
        constexpr static auto read_dynamic_array(std::istream& is, Size size, Args&&... args) {
            using namespace impl_details;
            T res{};
            res.reserve(size);
            using VT = std::iter_value_t<T>;
            if constexpr (can_optimize_read<VT>)
            {
                static_assert(sizeof...(args) == 0);
                constexpr auto sz = get_optimize_sizeof<VT>();
                auto data = std::make_unique_for_overwrite<char[]>(size * sz);
                is.read(data.get(), size * sz);
                std_ext::generate_ni(std::back_inserter(res), size, [&](auto i) {return std_ext::from_bytes<VT>(&data[i * sz], sz);});
                //std::size_t pos = 0;
                //std::generate_n(std::back_inserter(res), size, [&]() {return std_ext::from_bytes<VT>(&data[(pos+=sz)-sz], sz);});
                //const auto mv = std::make_move_iterator(reinterpret_cast<VT*>(data.get()));
                //res.insert(std::end(res), mv, mv + size);
                //for (auto v : res) 
                 //   std::cout << (int)v << ' ';
            }
            else {
                std::generate_n(std::back_inserter(res), size, [&]() {return reader<VT>::read(is, std::forward<Args>(args)...);});
            }
            return res;
        }

        template<class... Args>
        constexpr static auto read_fixed_type(std::istream& is, Args&&... args) {
            using namespace impl_details;
            if constexpr (can_optimize_read<T>) {
                static_assert(sizeof...(Args) == 0);
                std::array<char, get_optimize_sizeof<T>()> out;
                is.read(std::data(out), std::size(out));
                return std_ext::from_bytes<T>(std::data(out), out.size());
                //return *reinterpret_cast<T*>(std::data(out));
            }
            else {
                static_assert(fixed_array<T>);
                T res{};
                std::generate(std::begin(res), std::end(res), [&]() {return reader<std::iter_value_t<T>>::read(is, std::forward<Args>(args)...);});
                return res;
            }
        }

        template <class... Args>
        constexpr static T read(std::istream& is, Args&&... args) {
            using namespace impl_details;

            if constexpr (manual_read<T, Args...>) {
                return T::sern_read(is, std::forward<Args>(args)...);
            }
            else if constexpr (fixed_type<T>) {
                return read_fixed_type(is, std::forward<Args>(args)...);
            }
            else if constexpr (dynamic_array<T>) {
                return read_dynamic_array(is, std::forward<Args>(args)...);
            }
            else if constexpr (tuple_like<T>) {
                static_assert(sizeof...(Args) == 0);
                T res{};
                std::apply([&](auto&... field) {((field = reader<std::remove_reference_t<decltype(field)>>::read(is)), ...);}, res);
                return res;
            }
            else {
                static_assert(std::is_trivial_v<T> && std::is_aggregate_v<T>);
                T res{};
                //if constexpr (!std_ext::only_one_field<T>()) {
                static_assert(sizeof...(Args) == 0);
                std_ext::any_apply([&](auto&... field) {((field = reader<std::remove_reference_t<decltype(field)>>::read(is)), ...);}, res);
                //}
                /*else {
                    auto& [a] = res;
                    a=reader<std::remove_reference_t<decltype(a)>>::read(is, std::forward<Args>(args)...);
                }*/
                return res;
            }
        }
    };

    /*template <class... Ts>
    struct reader<std::variant<Ts...>> {
        template <class VarT, class... Args>
        inline constexpr static std::variant<Ts...> read(std::istream& is, VarT, Args&&... args) {
            return reader<std::get<VarT>>::read(is, std::forward<Args>(args)...);
        }
    };*/
    template <class T, class... Args>
    void rread(std::istream& is, T& field, Args&&... args) {
        field = reader<T>::read(is, std::forward<Args>(args)...);
    }
    template <class... Ts>
    void many_rread(std::istream& is, Ts&... fields) {
        ((fields = reader<Ts>::read(is)), ...);
    }


    template<class T, class TOS>
    struct writer {
        template<class... Args>
        constexpr static void write_fixed_type(TOS& os, const T& val, Args&&... args) {
            using namespace impl_details;
            if constexpr (can_optimize_read<T>) {
                static_assert(sizeof...(Args) == 0);
                os.write(reinterpret_cast<const char*>(&val), get_optimize_sizeof<T>());
            }
            else {
                static_assert(fixed_array<T>);
                using VT = std::iter_value_t<T>;
                std::for_each(std::begin(val), std::end(val), [&](const VT& v) {writer<VT, TOS>::write(os, v, std::forward<Args>(args)...);});
            }
        }

        template<class... Args>
        constexpr static void write_dynamic_array(TOS& os, const T& val, Args&&... args) {
            using namespace impl_details;
            using VT = std::iter_value_t<T>;
            if constexpr (can_optimize_read<VT>) {
                static_assert(sizeof...(Args) == 0);
                os.write(reinterpret_cast<const char*>(std::data(val)), std::size(val) * get_optimize_sizeof<VT>());
            }
            else {
                static_assert(dynamic_array<T>);
                std::for_each(std::begin(val), std::end(val), [&](const VT& v) {writer<VT, TOS>::write(os, v, std::forward<Args>(args)...);});
            }
        }

        template<class TF>
        constexpr static void process_field(TOS& os, const TF& field) {
            writer<std::decay_t<TF>, TOS>::write(os, field);
        }

        template <class... Args>
        constexpr static void write(TOS& os, const T& val, Args&&... args) {
            using namespace impl_details;
            if constexpr (manual_write<T, Args...>) {
                val.sern_write(os, std::forward<Args>(args)...);
            }
            else if constexpr (fixed_type<T>) {
                write_fixed_type(os, val, std::forward<Args>(args)...);
            }
            else if constexpr (dynamic_array<T>) {
                write_dynamic_array(os, val, std::forward<Args>(args)...);
            }
            else if constexpr (tuple_like<T>) {
                static_assert(sizeof...(Args) == 0);
                std::apply([&](auto&... field) {((process_field(os, field)), ...);}, val);
            }
            else {
                static_assert(sizeof...(Args) == 0);
                static_assert(std::is_aggregate_v<T>);
                //TODO На самом деле, может быть записано намного больше типов чем прочитано, так как reinterpret_cast<char*>(v) - корректно
                //В данном случае, отказаться от проверки static_assert(std::is_aggregate_v<T>) - это деталь std_ext::any_apply
                std_ext::any_apply([&](auto&... field) {((process_field(os, field)), ...);}, val);
            }
        }
    };

    template <class T, std::size_t N, class TOS>
    struct writer<T[N], TOS> {
        template <class... Args>
        constexpr static void write(TOS& os, T(&val)[N], Args&&... args) {
            using namespace impl_details;
            if  constexpr (can_optimize_read<T>) {
                static_assert(sizeof...(Args) == 0);
                os.write(reinterpret_cast<const char*>(&val), N * get_optimize_sizeof<T>());
            }
            else {
                std::for_each(std::begin(val), std::end(val), [&](const T& v) {writer<T, TOS>::write(os, v, std::forward<Args>(args)...);});
            }
        }
    };

    template <class TOS, class... Ts>
    struct writer<std::variant<Ts...>, TOS> {
        template <class... Args>
        constexpr static void write(TOS& os, const std::variant<Ts...>& val, Args&&... args) {
            std::visit(
                [&](auto& inner) {writer<std::decay_t<decltype(inner)>, TOS>::write(os, inner, std::forward<Args>(args)...);},
                val);
        }
    };

    template<class T, class TOS, class ...Args>
    constexpr void write(TOS& os, const T& val, Args&& ...args) {
        writer<T, TOS>::write(os, val, std::forward<Args>(args)...);
    }

    template <class TOS, class... Ts>
    constexpr void many_write(TOS& os, const Ts&... vals) {
        ((writer<Ts, TOS>::write(os, vals)), ...);
    }



    //Типы, которые при дессиарилизации должны иметь какое-то ожидаемое(expected) значение
    //TODO подумать над std::expected
    template<std::size_t N>
    struct efstring_constuctor {
        constexpr static auto length = N - 1;//-1 - null term
        //char data[length]{};
        std::array<char, length> data;
        consteval efstring_constuctor(const char(&str)[N]) {
            std::copy_n(std::begin(str), length, std::begin(data));
        }
    };

    //Literal for string without null term
    template<efstring_constuctor S>
    consteval auto operator""_N() {
        return S.data;
    }
    //TODO перейти на consteval
    template<efstring_constuctor Expected>
    struct efstring {
        using TVal = decltype(Expected.data);
        TVal val;
        consteval auto size() { return decltype(Expected)::length; }
        constexpr static TVal expected = Expected.data;
        constexpr operator TVal& () { return val; }
        constexpr operator const TVal& () const { return val; }
        constexpr bool check() const { return std::equal(std::begin(val), std::end(val), std::begin(expected)); }

        inline static efstring sern_read(std::istream& is) {
            efstring res{ sern::reader<TVal>::read(is) };
            if (!res.check()) throw "unkown ident";
            return res;
        }
        //todo sern_write
    };



    template <typename TNum, TNum Expected>
    struct enumb {
        TNum val;
        constexpr static auto expected = Expected;

        constexpr bool check() const { return val == expected; }
        operator TNum& () { return val; }
        operator const TNum& () const { return val; }

        inline static enumb sern_read(std::istream& is) {
            enumb res{ sern::reader<TNum>::read(is) };
            if (!res.check()) throw "unkown val";
            return res;
        }
    };

    template <typename TNum, TNum Min, TNum Max>
    struct ebnumb {
        TNum val;
        constexpr static auto expected_min = Min;
        constexpr static auto expected_max = Max;

        constexpr bool check() const { return (expected_min <= val) && (val <= expected_max); }
        operator TNum& () { return val; }
        operator const TNum& () const { return val; }

        inline static ebnumb sern_read(std::istream& is) {
            ebnumb res{ sern::reader<TNum>::read(is) };
            if (!res.check()) throw "unkown val";
            return res;
        }
    };

    template <std::size_t Size, class TByte = std::byte, class TArray = std::array<TByte, Size>>
    struct ealign {
        TArray val;
        template<std::integral T>
        constexpr static T get_align_size(T pos) {
            static_assert(std::is_unsigned_v<decltype(Size)>);
            //return (pos + Size - 1) / Size * Size - pos;
            return Size - 1 - ((pos - 1) % Size);
            //TODO[10.02.2025] Shouldn't it be a+Size-1-((pos-1) % Size)
        }

        constexpr static auto sern_read(std::istream& is) {
            ealign res{};
            //TODO избавиться от такого каста
            is.read(reinterpret_cast<char*>(std::data(res.val)), get_align_size(static_cast<std::streamoff>(is.tellg())));
            return res;
        }
        //TODO
        /*constexpr void sern_write(auto& os) const {
            os.write(reinterpret_cast<const char*>(std::data(val)), get_align_size(static_cast<std::streamoff>(os.tellp())));
        }*/
    };
}













//1 2 3 4 5 6 7 8
//4 3 2 1 8 7 6 5
//TODO
/*
Типы могут быть:
1)С мануальной записью
2)За запись которых полностью отвечает TOS(в основном, это арифметические или enums типы, так как их представления в файле зависет формата
3)Контейнеры - такие типы, для предстовления которых достаточно записать(прочитать) все их элементы(поля)
3.1)Массивы(все хранимые элементы одного типа)
3.1.1)Количество элементов известно на этапе компиляции
3.1.2)Количество элементов не известно на этапе компиляции
3.2)Кортежы
3.3)Структуры. Конструктор - не более одного пользв paramless конструктора, успешность записи(чтения) структуры зависит от std_ext::any_apply
*/
namespace sern2 {

    namespace impl_details_wr {

        template <class TOS, class T, class... Args>
        concept manual_write = requires(TOS & is, Args&&... args) {
            { std::declval<T>().sern_write(is, std::forward<Args>(args)...) };
        };

        template <class TOS, class T, class... Args>
        concept ostream_type_writeable = requires(TOS & os, T && val, Args&&... args) {
            { os.write_type(std::forward<T>(val), std::forward<Args>(args)...) };
        };

        template <class TOS, class T, class... Args>
        concept ostream_range_writeable = requires(TOS & os, T && val, Args&&... args) {
            { os.write_range(std::forward<T>(val), std::forward<Args>(args)...) };
        };

        template <class TOS, class T, class... Args>
        concept ostream_struct_writeable = requires(TOS & os, T && val, Args&&... args) {
            { os.write_struct(std::forward<T>(val), std::forward<Args>(args)...) };
        };

        template <class TOS, class T, class... TF>
        concept ostream_fields_writeable = requires(TOS & os, T && val, TF&&... fields) {
            { os.write_fields(std::forward<T>(val), std::forward<TF>(fields)...) };
        };

        template<class T>
        struct writer_impl;
    }

    template<class T>
    struct user_writer : impl_details_wr::writer_impl<T> {};

    template<class T>
    struct writer : user_writer<std::remove_cvref_t<T>> {};

    //Данный тип предназначен для возможности применения стандартного(т.е. библиотечного) 
    //поведения сериализации. Может быть использован в пользовательских специализациях user_writer 
    template<class T>
    struct sern_writer : impl_details_wr::writer_impl<std::remove_cvref_t<T>> {};

    //TODO Хм, будут ли частичные специализации метода write иметь ожидаемое поведение?
    //TODO при входе в метод write или в writer_impl::write проверить состояние потока(м.б. новым методом, напр, is_ok) 
    //если поток в плохом состоянии то выйти. А также проверять состояние в write_range.
    //Проблема: при записи контейнеров метод is_ok будет вызываться несколько раз,
    //Один раз для самого типа, а также для каждого элемента
    //TODO При отсутсвие метода write_type в пользовательском потоке воспользоваться оператором << ?
    template<class T, class TOS, class ...Args>
    constexpr void write(TOS& os, T&& val, Args&& ...args) {
        //std_ext::tp<std::remove_cvref_t<T>>();
        writer<T>::write(os, std::forward<T>(val), std::forward<Args>(args)...);
    }

    template <class TOS, class... Ts>
    constexpr void many_write(TOS& os, Ts&&... vals) {
        ((write(os, std::forward<Ts>(vals))), ...);
    }

    template<class TOS, std::ranges::input_range T, class... Args>
    constexpr void write_range(TOS& os, T&& val, Args&& ...args) {
        std::ranges::for_each(std::forward<decltype(val)>(val), [&](auto&& el) {write(os, std::forward<decltype(el)>(el), std::forward<Args>(args)...);});
    }

    template <class TOS, class TOwner, class... Ts>
    constexpr void write_fields(TOS& os, TOwner&& owner, Ts&&... fields) {
        if constexpr (impl_details_wr::ostream_fields_writeable<TOS, decltype(owner)>)
            os.write_fields(std::forward<TOwner>(owner), std::forward<decltype(fields)>(fields)...);
        else
            many_write(os, std::forward<Ts>(fields)...);
    }

    //Данный тип предназначен для пользовательских специализацией функционала, 
    //по получению имени поля или преобразования значния перечесления в строку.
    template<class T>
    struct sern_name {
        constexpr auto operator()(std::size_t i) requires (!std::is_enum_v<T>) {
            return std_ext::field_names<T>[i];
        }
        constexpr auto operator()(T val) requires (std::is_enum_v<T>) {
            return std_ext::enum_name(val);
        }
    };

//    template <class T>
//    inline constexpr bool custom_sern_field = requires(std::size_t i) {{ std::declval<T>().sern_field(i) };};

    template<class T>
    constexpr auto field_name(const T& val, std::size_t i) {
        constexpr bool custom_sern_field = requires(std::size_t i) { { std::declval<T>().sern_field(i) }; };
        if constexpr (custom_sern_field)
            return val.sern_field(i);
        else 
            return sern_name<T>{}(i);
    }
    template<class T>
    constexpr auto enum_name(T val) requires (std::is_enum_v<T>) {
        return sern_name<T>{}(val);
    }


    /*template <class NR>
    struct writeable_range : std::bool_constant<std::ranges::input_range<NR>> {
        template<class TOS, class T, class... Args>
        constexpr static void write(TOS& os, T&& val, Args&& ...args) { write_range(os, std::forward<T>(val), std::forward<Args>(args)...); }
    };*/
//    template <class NR>
//    constexpr bool is_writible_range_v = writeable_range<NR>::value;


    template <std::ranges::input_range NR>
    struct writeable_range {
        template<class TOS, class T, class... Args>
        constexpr static void write(TOS& os, T&& val, Args&& ...args) { write_range(os, std::forward<T>(val), std::forward<Args>(args)...); }
    };

    namespace impl_details_wr {
        namespace utils {
            template<class TOS, class T, class... Args>
            constexpr static void write_range(TOS& os, T&& val, Args&&... args) {
                if constexpr (ostream_range_writeable<TOS, decltype(val), Args...>)
                    os.write_range(std::forward<T>(val), std::forward<Args>(args)...);
                else
                    writeable_range<std::remove_cvref_t<T>>::write(os, std::forward<T>(val), std::forward<Args>(args)...);
            }
            template<class TOS, class T, class... Args>
            constexpr static void write_struct(TOS& os, T&& val, Args&&... args) {
                if constexpr (ostream_struct_writeable<TOS, decltype(val), Args...>) {
                    os.write_struct(std::forward<T>(val), std::forward<Args>(args)...);
                }
                else {
                    static_assert(sizeof...(Args) == 0);
                    std_ext::any_apply2([&](auto&&... field) {write_fields(os, val, std::forward<decltype(field)>(field)...);}, val);
                }
            }
        }



        template<class NR>
        struct writer_impl {
        private:
            //Прим. указатель вида [const] char* обычно рассматривается как строка, но такой перегрузки здесь нет
            //Так как невозможно гарантировать что это null-terminated
            template<class TOS, class T, std::integral Size, class... Args>
            constexpr static void write_pointer(TOS& os, T&& val, Size size, Args&&... args) {
                sern2::write(os, std::span{ val, static_cast<std::size_t>(size) }, std::forward<Args>(args)...);
            }
            template<class TOS, class T, class... Args>
            constexpr static void write_pointer(TOS& os, T&& val, Args&&... args) {
                //TODO os.is_not_null
                if (val!=nullptr)
                    sern2::write(os, *val, std::forward<Args>(args)...);
            }
        public:
            template <class TOS, class T, class... Args>
            constexpr static void write(TOS& os, T&& val, Args&&... args) {
                if constexpr (manual_write<TOS, decltype(val), Args...>) {
                    val.sern_write(os, std::forward<Args>(args)...);
                }
                else if constexpr (ostream_type_writeable<TOS, decltype(val), Args...> || std::is_arithmetic_v<NR>) {
                    os.write_type(std::forward<T>(val), std::forward<Args>(args)...);
                }
                else if constexpr (std::is_enum_v<NR>) {
                    os.write_type(static_cast<std::underlying_type_t<NR>>(val), std::forward<Args>(args)...);
                }
                else if constexpr (std::ranges::input_range<NR>) {
                    utils::write_range(os, std::forward<T>(val), std::forward<Args>(args)...);
                }
                else if constexpr (std::is_pointer_v<NR>) {
                    write_pointer(os, std::forward<T>(val), std::forward<Args>(args)...);
                }
                else {
                    utils::write_struct(os, std::forward<T>(val), std::forward<Args>(args)...);
                }
            }
        };


        template <class... Ts>
        struct writer_impl<std::variant<Ts...>> {
            template <class TOS, class T, class... Args>
            constexpr static void write(TOS& os, T&& val, Args&&... args) {
                if constexpr (ostream_type_writeable<TOS, decltype(val), Args...>)
                    os.write_type(std::forward<T>(val), std::forward<Args>(args)...);
                else 
                    std::visit([&](auto&& inner) { sern2::write(os, std::forward<decltype(inner)>(inner), std::forward<Args>(args)...); }, std::forward<T>(val));
            }
        };
        //Не удалять это специализацию, так как c-массивы разворачиваются в указатели.
        // Если пользовательский поток имеет метод для записи указателей то c-массивы будут туда отправлены
        template <class NR, std::size_t N>
        struct writer_impl<NR[N]> {
            template <class TOS, class T, class... Args>
            constexpr static void write(TOS& os, T&& val, Args&&... args) {
                if constexpr (ostream_type_writeable<TOS, decltype(val), Args...>)
                    os.write_type(std::forward<decltype(val)>(val), std::forward<Args>(args)...);
                else
                    utils::write_range(os, std::forward<decltype(val)>(val), std::forward<Args>(args)...);
            }
        };
    }
}

#ifndef NDEBUG
#include <deque>
namespace sern2::testsr {

    struct nop_stream {
        constexpr void write_type(std::floating_point auto val) {}
        constexpr void write_type(std::integral auto val) {}
        //todo сделать оператор <<(или выбрать другое написание)
    };

    void test01() {
        nop_stream ss{};
        {
            std::vector vec{ std::deque{std::list{1,2,9,8},std::list{3,4},std::list{5,6}},  std::deque{std::list{7},std::list{8,9},std::list{10}} };
            //sern2::many_write(ss, vec, std::as_const(vec), std::move(vec));
            //ss << sern2::binary << vec << std::as_const(vec) << std::move(vec);
            //ss << sern2::binary << args(new int[] { 1, 2, 3 }, 0);
            //(ss << sern2::binary)(1)(2)(3);
            write(ss, vec);
            write(ss, std::as_const(vec));
            write(ss, std::move(vec));
        }
        {
            float car[3] = { 1,2,3 };
            write(ss, car);
            write(ss, std::as_const(car));
            //write(ss, std::move(car));
        }
        {
            const char car[20] = { 'a','b' };
            write(ss, car);
            write(ss, std::as_const(car));
            //write(ss, std::move(car));
        }

        write(ss, "hello c string");
        many_write(ss, 1, '2abc', '3', 4.0, std::string{"hello cpp string"});

        

        std::byte* darr1 = new std::byte[3]{ std::byte{1}, std::byte{2}, std::byte{3} };
        sern2::write(ss, darr1, 3);
        delete[] darr1;


    }
}
#endif

/*template<class T>
struct args_t {
    T func;
};

//TODO stackoverflow lamda capturing and perfect forwarding
template<class T, class ...Args>
auto args(T&& val, Args&& ...args) {
    return args_t{ [&] <class TOS>(TOS & os) { sern2::write(os, std::forward<T>(val), std::forward<Args>(args)...); } };
}

struct binary_t {};
constexpr binary_t binary{};
template<class TOS>
struct write_op {
    TOS& os;
    template <class T>
    constexpr write_op& operator <<(T&& obj) {
        sern2::write(os, std::forward<T>(obj));
        return *this;
    }

    template <class T>
    constexpr write_op& operator <<  (args_t<T>&& obj) {
        obj.func(os);
        return *this;
    }
};



template <class TOS>
constexpr auto operator <<(TOS& os, binary_t) noexcept {
    return write_op{os};
}*/

/*template<class T>
constexpr void write_struct(T&& val) {

    *this << '{';
    inc_depth();
    std_ext::any_apply2([&](auto&&... field) {
        int i = 0;
        ((
        *this<<(i == 0 ? "" : ",")<<'\n', *this<<offset, write_string(get_name(val, i++)) << ':', sern2::write(*this, std::forward<decltype(field)>(field))
        ), ...); }, std::forward<T>(val));

    *this <<"\n"<< dec_depth() << "}";
 }*/

 //if constexpr (std_ext::any_apply2([&](auto&&... field) { return ostream_struct_writeable<TOS, decltype(val), std::forward<decltype(field)>(field)...>; }, val)) {
 ////  os.write_struct(val, std::forward<Args>(args)...);
 //if constexpr (ostream_struct_writeable<TOS, decltype(val)>) {
 //    static_assert(sizeof...(Args) == 0);
     //todo std::forward<T>(val) vs val vs move
 //    std_ext::any_apply2([&](auto&&... field) {os.write_struct(std::move(val), std::forward<decltype(field)>(field)...); }, val);
 //}
 //else
 //{
    // static_assert(sizeof...(Args) == 0);
    // std_ext::any_apply2([&](auto&&... field) {((
    //     sern2::write(os, std::forward<decltype(field)>(field))
    //     ), ...); }, std::forward<T>(val));
 //}


/*#include <iostream>
#include <charconv>

template<class TConv, std::size_t Size>
struct to_chars_size_result {
    constexpr static auto size = Size;
    TConv conv;
    constexpr auto operator()(char* first, char* last) {
        conv(first, last);//std::invoke
    }
};

template<std::size_t Size, class TConv>
consteval auto make_chars_size_result(TConv conv) {
    return to_chars_size_result<TConv, Size>{conv};
}*/

/*template<std::size_t Size>
struct qq{
    constexpr static auto size = Size;
    consteval auto operator()() const{
        return Size;
    }
};*/

/*constexpr auto empty_base = 0;
template<int base = 10, std::integral T>
consteval auto to_chars_size(T val) {

    if constexpr (base != empty_base)
        return make_chars_size_result<5>([val](char* first, char* last) {return std::to_chars(first, last, val, base);});
    else
        return make_chars_size_result<8>([val](char* first, char* last, int b = 10) {return std::to_chars(first, last, val, b);});
}

template<int base = 10, std::integral T, class F>
consteval auto to_chars_size2(T val, F f) {

    if constexpr (base != empty_base)
        return make_chars_size_result<5>([val](char* first, char* last) {return std::to_chars(first, last, val, base);});
    else
        return make_chars_size_result<8>([val](char* first, char* last, int b = 10) {return std::to_chars(first, last, val, b);});
}

int main() {
    auto a = to_chars_size(8);
    std::array<int, a.size> k;

    std::cout << k.size();
}*/


/*
#include <iostream>
#include <charconv>

template<bool IsCompiled, class T>
struct pp_base {
    using value_type = T;
    constexpr static bool is_compiled = IsCompiled;
};

template<auto V>
struct compiletime_constant:public pp_base<true, decltype(V)> {
    constexpr static auto value = V;
    constexpr operator auto() const noexcept { return value; }
    constexpr auto operator()() const noexcept { return value; }
};

template<class T>
struct runtime_constant:public pp_base<false, T> {
    T value;
    constexpr runtime_constant(T val):value{val}{};
    constexpr operator auto() const noexcept { return value; }
    constexpr auto operator()() const noexcept { return value; }
};

template<auto V>
consteval auto make_constant(){
    return compiletime_constant<V>{};
}
template<class T>
constexpr auto make_constant(T v){
    return runtime_constant<T>{v};
}

template<class T>
concept constant = requires {
    {T::is_compiled} -> std::convertible_to<bool>;
    {std::declval<T>().value} -> std::convertible_to<typename T::value_type>;
};

template<class T>
concept integral_constant = constant<T> && std::integral<typename T::value_type>;

template<class T>
concept int_constant = constant<T> && std::convertible_to<typename T::value_type, int>;

template<class... Args>
constexpr unsigned int build_switch_table(Args... bools){
    return (... | bools);
}

void to_chars2(integral_constant auto value, int_constant auto base) {
    if constexpr(value.is_compiled && base.is_compiled){
        std::cout<<"1";
    } else
    if constexpr(value.is_compiled){
        std::cout<<"2";
    } else{
        std::cout<<"3";
    }
}

int main(){
   build_switch_table(true, true, false);

    to_chars2(make_constant<5>(), make_constant(10));
  // std::array<int, v()> arr;
}*/