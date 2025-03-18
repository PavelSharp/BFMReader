#pragma once
#include "pch.h"
#include "std_ext.hpp"
#include "sern.hpp"

namespace sern2 {
    template<std_ext::number T>
    struct json_number {
        T val;
        friend auto& operator<<(auto& wr, json_number v) {
            if constexpr (std::is_same_v<T, bool>)
                return wr << (v.val ? "true" : "false");
            else//TODO перейти на std::to_chars
                return wr << std::format("{}", v.val);
        }
    };


    template<class CharT, class Traits = std::char_traits<CharT>> requires requires{std_ext::is_any_same<CharT, char, char8_t>;}
    struct basic_json_string_view : public std::basic_string_view<CharT, Traits> {
        friend auto& operator<<(auto& wr, basic_json_string_view v) {
            wr << '"';
            for (const char c : v) {
                const auto ic = static_cast<unsigned char>(c);
                if (ic <= 0x1F || c == '"' || c == '\\') {
                    wr << '\\';
                    switch (c)
                    {
                    case '"': case '\\': wr << c; break;
                    case '\b': wr << 'b'; break;
                    case '\f': wr << 'f'; break;
                    case '\n': wr << 'n'; break;
                    case '\r': wr << 'r'; break;
                    case '\t': wr << 't'; break;
                    default:
                    {
                        constexpr static auto hexdig = "0123456789ABCDEF";
                        wr << "u00" << hexdig[ic >> 4] << hexdig[ic & 0xF];
                    }
                    }
                }
                else
                    wr << c;
            }
            wr << '"';
            return wr;
        }
    };
    using json_string_view = basic_json_string_view<char>;
    using json_u8string_view = basic_json_string_view<char8_t>;

    enum class json_action { begin, end, delim };

    template<class... Args>
    struct many_out {
        std::tuple<Args...> val;
        friend constexpr auto& operator<<(auto& wr, const many_out& v) {
            return std::apply([&wr](const auto&... args) { (wr << ... << args); }, v.val), wr;
        }
        friend constexpr auto& operator<<(auto& wr, many_out&& v) {
            return std::apply([&wr](auto&&... args) { (wr << ... << std::move(args)); }, std::move(v.val)), wr;
        }
    };

    constexpr auto make_many_out(auto&&... args) { return many_out{ std::make_tuple(std::forward<decltype(args)>(args)...) }; }
    constexpr auto tie_many_out(auto&... args) { return many_out{ std::tie(args...) }; }

    struct json_formatter {
        template<json_action Action>
        constexpr void range(auto& wr, const auto&) {
            switch (Action)
            {
            case json_action::begin: wr << '[';break;
            case json_action::end: wr << ']';break;
            case json_action::delim:  wr << ", ";break;
            }
        }

        template<json_action Action, class... TF>
        constexpr void structure(auto& wr, const TF&...) {
            switch (Action)
            {
            case json_action::begin: wr << '{';break;
            case json_action::end: wr << '}';break;
            case json_action::delim:  wr << ", ";break;
            }
        }

        template<std::size_t ind>
        constexpr auto structure_field(const auto& owner){
            return make_many_out('"', field_name(owner, ind), "\": ");
        }

        template<class CharT, class Traits> requires requires {typename basic_json_string_view<CharT, Traits>::value_type;}
        constexpr auto escape_string(std::basic_string_view<CharT, Traits> val) {
            return basic_json_string_view<CharT, Traits>{val};
        }

        template<class T>
        constexpr auto number(T val) {
            return json_number<T>{ val };
        }
    };



    struct json_formatter_settings {
        std::optional<std::string_view> indent;
        bool compact;
        enum field_policy_kind { quoted, unquoted, disabled } field_policy;
        std::optional<std::size_t> fields_limit;
        std::optional<std::size_t> array_limit;
    };
    namespace details {
        constexpr auto minimal_fmt = json_formatter_settings{ .indent = {}, .compact = false };
        constexpr auto indents_fmt = json_formatter_settings{ .indent = "   ", .compact = false};
        constexpr auto indents_compact_fmt = json_formatter_settings{ .indent = indents_fmt.indent, .compact = true };


        template<const std::string_view& indent>
        struct depth_t {
            std::size_t offset;

            friend constexpr auto& operator<<(auto& wr, depth_t v) {
                for (auto i : std::views::iota(std::size_t{ 0 }, v.offset))
                    wr << indent;
                return wr;
            }
            constexpr auto operator ++() noexcept {
                return ++offset, * this;
            }
            constexpr auto operator --() noexcept {
                return --offset, * this;
            }
        };
    }

    template<const json_formatter_settings& Setting>
    class json_formatter_basic2 {



        template<json_action Action>
        constexpr void range(auto& wr, const auto&) {


                    /*    case json_action::begin: wr << '[';break;
                        case json_action::end: wr << ']';break;
                        case json_action::delim:  wr << ", ";break;

                            using val_t = std::iter_value_t<decltype(rng)>;
                            if constexpr (Action == json_action::begin) return make_many_out("[\n", ++depth);
                            else if constexpr (Action == json_action::end) return make_many_out('\n', --depth, ']');
                            else if constexpr (Action == json_action::delim) {
                                if constexpr (compact && (std::is_arithmetic_v<val_t> || std::is_enum_v<val_t>))
                                    return make_many_out(", ");
                                else
                                    return make_many_out(",\n", depth);
                            }*/

        }

        template<json_action Action, class... TF>
        constexpr void structure(auto& wr, const TF&...) {
           // if constexpr (Action == json_action::begin) return make_many_out("{\n", ++depth);
            //else if constexpr (Action == json_action::end) return make_many_out('\n', --depth, '}');
            //else if constexpr (Action == json_action::delim) return make_many_out(",\n", depth);
            /*switch (Action)
            {
            case json_action::begin: wr << '{';break;
            case json_action::end: wr << '}';break;
            case json_action::delim:  wr << ", ";break;
            }*/
        }

        template<class CharT, class Traits> requires requires {typename basic_json_string_view<CharT, Traits>::value_type;}
        constexpr auto escape_string(std::basic_string_view<CharT, Traits> val) {
            return basic_json_string_view<CharT, Traits>{val};
        }

        template<class T>
        constexpr auto number(T val) {
            return json_number<T>{ val };
        }
    };

    template<bool compact>
    struct json_indent_formatter_basic :public json_formatter {
        struct depth_t {
            constexpr static std::string_view model = "   ";
            std::size_t offset;

            friend constexpr auto& operator<<(auto& wr, depth_t v) {
                for (auto i : std::views::iota(std::size_t{ 0 }, v.offset))
                    wr << model;
                return wr;
            }
            constexpr auto operator ++() noexcept {
                return ++offset, * this;
            }
            constexpr auto operator --() noexcept {
                return --offset, * this;
            }
        };
        depth_t depth;

        /*template<json_action Action>
        constexpr auto range(const auto& rng) {
            using val_t = std::iter_value_t<decltype(rng)>;
            if constexpr (Action == json_action::begin) return make_many_out("[\n", ++depth);
            else if constexpr (Action == json_action::end) return make_many_out('\n', --depth, ']');
            else if constexpr (Action == json_action::delim) {
                if constexpr (compact && (std::is_arithmetic_v<val_t> || std::is_enum_v<val_t>))
                    return make_many_out(", ");
                else
                    return make_many_out(",\n", depth);
            }

            switch (Action)
            {
            case json_action::begin: wr << "[\n" << ++depth; break;
            case json_action::end: wr << '\n' << --depth << ']';break;
            case json_action::delim: {
                if ((std::is_arithmetic_v<val_t> || std::is_enum_v<val_t>) && compact)
                    wr << ", ";
                else
                    wr << ",\n" << depth;
                break;
            }
            }

        }*/

        /* template<json_action Action, class... TF>
        constexpr auto structure(const TF&...) {//todo req
            if constexpr (Action == json_action::begin) return make_many_out("{\n", ++depth);
            else if constexpr (Action == json_action::end) return make_many_out('\n', --depth, '}');
            else if constexpr (Action == json_action::delim) return make_many_out(",\n", depth);
            
            switch (Action)
            {
                case json_action::begin: return make_many_out("{\n", ++depth);
                case json_action::end: return make_many_out('\n', --depth, '}');
                case json_action::delim: return make_many_out(",\n", depth);
            }*/
/*            switch (Action)
            {
            case json_action::begin: wr << "{\n" << ++depth;break;
            case json_action::end: wr << '\n' << --depth << '}';break;
            case json_action::delim: wr << ",\n" << depth;break;
            }*/
      //  }

        constexpr auto range_begin(const auto& rng) {
            return make_many_out("[\n", ++depth);
        }
        constexpr auto range_end(const auto& rng) {
            return make_many_out('\n', --depth, ']');
        }
        constexpr auto range_delim(const auto& val) {
            using val_t = std::remove_cvref_t<decltype(val)>;
            if constexpr (compact && (std::is_arithmetic_v<val_t> || std::is_enum_v<val_t>))
                return make_many_out(", ");
            else
                return make_many_out(",\n", depth);
        }
        template<std::size_t fields>
        constexpr auto structure_begin() {
            return make_many_out("{\n", ++depth);
        }
        template<std::size_t fields>
        constexpr auto structure_end() {
            return make_many_out('\n', --depth, '}');
        }
        constexpr auto structure_delim(std::string_view name, const auto& val) {
            return make_many_out(",\n", depth);
        }
    };

    using json_indent_formatter = json_indent_formatter_basic<false>;
    using json_compact_formatter = json_indent_formatter_basic<true>;

    template<class T>
    struct json_writer {};

    template<class CharT, class Traits, class Allocator> requires (std_ext::is_any_same<CharT, char, char8_t>)
    struct json_writer<std::basic_string<CharT, Traits, Allocator>> {
        std::basic_string<CharT, Traits, Allocator> data;
        constexpr auto& operator <<(std::string_view v) {
            //В отличе от оператора +=, использование append осуществляет неявное преобразование между char <--> char8_t
            return data.append(std::begin(v), std::end(v));
        }
        template<class CharT>
        constexpr auto& operator <<(CharT v) requires(std::is_same_v<CharT, char>) {
            return data += v, *this;
        }
    };

    template<std::size_t N>
    struct json_writer<char[N]> {
        char data[N] = { '\0' };
        char* ptr = &data[0];
        constexpr void check_border(std::size_t sz) {
            const auto total = ptr - data;
            if (N - total < sz) {
                throw std::out_of_range(std::format("json_writer<char[{}]> - Attempt to access a non-existent index. Total: {}, Requested size: {}", N, total, sz));
            }
        }
        constexpr auto& operator <<(std::string_view v) {
#ifndef NDEBUG
            check_border(v.size());
#endif // NDEBUG
            std::ranges::copy(v, ptr);
            return ptr += v.size(), *this;
        }
        template<class CharT>
        constexpr auto& operator <<(CharT v) requires(std::is_same_v<CharT, char>) {
#ifndef NDEBUG
            check_border(1);
#endif // NDEBUG
            return *ptr++ = v, *this;
        }
    };

    template<>
    struct json_writer<std::size_t> {
        std::size_t data;
        constexpr auto& operator <<(std::string_view v) {
            return data += v.size(), *this;
        }
        template<class CharT>
        constexpr auto& operator <<(CharT v) requires(std::is_same_v<CharT, char>) {
            return ++data, * this;
        }
    };


    template<class Writer = json_writer<char[2048]>, class Formatter = json_compact_formatter>
    struct json_ostream {
        Writer writer;
        Formatter formatter;
        enum field_policy_kind { quoted, unquoted, disabled } field_policy = unquoted;
        std::optional<std::size_t> limit = {};

        constexpr void write_as_sequnce(const auto& val) {
            writer << formatter.range_begin(val);
            auto lim = limit;
            for (auto beg = std::ranges::begin(val); beg != std::ranges::end(val);++beg) {
                if (beg != std::ranges::begin(val)) writer << formatter.range_delim(*beg);
                if (lim && (lim.value()-- == 0)) {
                    writer << "...";
                    if constexpr (std::ranges::sized_range<std::decay_t<decltype(val)>>) {
                        //os <<'('<< std::ranges::distance(beg, std::ranges::end(val)) << ')';
                        writer << '(' << formatter.number(std::ranges::size(val)) << ')';
                    }
                    break;
                }
                write(*this, *beg);
            }
            writer << formatter.range_end(val);
        }

        template<class CharT, class Traits>
        constexpr void write_as_string(std::basic_string_view<CharT, Traits>  val) {
            constexpr bool can_escape = requires{ {formatter.escape_string(val)}; };
            if constexpr (can_escape)
                writer << formatter.escape_string(val);
            else
                write_as_sequnce(val);
        }

        template<class T>
        constexpr void write_type(T val) requires (std::is_arithmetic_v<T>) {
            writer << formatter.number(val);
        }

        template<class T>
        constexpr void write_type(T val) requires (std::is_enum_v<T>) {
            writer << enum_name(val);
        }

        template<class CharT, class Traits>
        constexpr void write_type(std::basic_string_view<CharT, Traits>  val) {
            write_as_string(val);
        }

        /*template<class T>
        constexpr void write_field_name(const T& val, std::size_t ind) {
            //os << std_ext::type_name_v<T><<"::";
            if (field_policy == quoted)
                writer << '"' << field_name(val, ind) << "\":";
            else if (field_policy == unquoted)
                writer << field_name(val, ind) << ": ";
        }*/

        template<std::size_t ind, std::size_t count>
        void proc_field(const auto& owner) {

        }

        template<std::size_t ind, std::size_t count>
        void proc_field(const auto& owner, const auto& field, const auto&... fields) {
            if constexpr (ind != 0) {
                writer << formatter.structure_delim(field_name(owner, ind), field);

                //writer << formatter.structure_field<count,ind>(owner, field, field_writer);
            }
            writer << formatter.structure_field<ind>(owner);
            //write_field_name(owner, ind);
            //writer <<'('<<json_number{ind} << ')';
            sern2::write(*this, field);
            proc_field<ind + 1, count>(owner, fields...);
        }


        template<class T, class... TF>
        constexpr void write_fields(const T& val, const TF&... fields) {
            writer << formatter.structure_begin<sizeof...(fields)>();

            proc_field<0, sizeof...(fields)>(val, fields...);

            /*auto format = [&, i = 0](auto& field) mutable {
                if (i != 0) 
                    writer << formatter.structure_delim(field_name(val, i), field);
                write_field_name(val, i++);
                sern2::write(*this, field);
            };

            (..., format(fields));*/

            writer << formatter.structure_end<sizeof...(fields)>();
        }

        template<class T>
        constexpr void write_range(const T& val) {
            using val_t = std::iter_value_t<T>;
            constexpr bool is_string = std_ext::is_char_type<val_t> && std_ext::ptrsz_range<T>;
            if constexpr (!is_string)
                write_as_sequnce(val);
            else
            {
                constexpr bool has_traits = requires {typename T::traits_type;};
                if constexpr (has_traits)
                    write_as_string(std::basic_string_view<val_t, typename T::traits_type>(std::ranges::begin(val), std::ranges::end(val)));
                else
                    write_as_string(std::basic_string_view<val_t>(std::ranges::begin(val), std::ranges::end(val)));
            }
        }
    };
}