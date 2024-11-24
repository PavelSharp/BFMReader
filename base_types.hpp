#pragma once
#include "pch.h"
#include "sern.hpp"

namespace br2proj {
    using matrix4x4 = aiMatrix4x4;
    using vector3 = aiVector3D;
    using vector2 = aiVector2D;
    using quaternion = aiQuaternion;
    using string = std::string;

    using size_t = std::size_t;

    template<typename T, std::size_t Size>
    using array = std::array<T, Size>;
    template<typename T>
    using vector = std::vector<T>;
    template<std::size_t Size>
    using byte_array = std::array<std::uint8_t, Size>;//TODO перейти на std::byte или unsigned char

	template<std::floating_point T>
	struct vector2t { T x, y; };

	template<std::floating_point T>
	struct vector3t { T x, y, z; };

	template<std::integral T>
	struct trianglet { T a, b, c; };

	template<typename T>
	struct bound_boxt { T start, end; };

    //Фиксированные и доступные для сериализации типы
    template<std::size_t Size>
    using fstring = array<char, Size>;
	using byte = std::byte;
	using uint16 = std::uint16_t;
    using int16 = std::int16_t;
    using int32 = std::int32_t;
    using uint32 = std::uint32_t;
    using int64 = std::int64_t;
    using float32 = float;
    static_assert(std::numeric_limits<float32>::is_iec559);

	template <int16 V>
	using eint16 = sern::enumb<decltype(V), V>;
	template <int32 V>
	using eint32 = sern::enumb<decltype(V), V>;
	template <uint16 V>
	using euint16 = sern::enumb<decltype(V), V>;
	template <uint32 V>
	using euint32 = sern::enumb<decltype(V), V>;

	template <int16 Min, int16 Max>
	using ebint16 = sern::ebnumb<int16, Min, Max>;
	template <int32 Min, int32 Max>
	using ebint32 = sern::ebnumb<int32, Min, Max>;
	template <uint16 Min, uint16 Max>
	using ebuint16 = sern::ebnumb<uint16, Min, Max>;
	template <uint32 Min, uint32 Max>
	using ebuint32 = sern::ebnumb<uint32, Min, Max>;

	template<class TEnum>//std::underlying_type_t<TEnum> First = , std::underlying_type_t<TEnum> Last =
	using eb_enum = sern::ebnumb<TEnum, TEnum::first, TEnum::last>;

	template<size_t Size>
	using ealign = sern::ealign<Size, byte, array<byte, Size>>;
}