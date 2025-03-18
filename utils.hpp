#pragma once
#include "pch.h"
#include "import_bfm.hpp"

namespace br2proj::utils {
    using namespace br2proj::bfm;

    auto conv(point_3f pt) {
        return vector3{ static_cast<decltype(vector3::x)>(pt.x), static_cast<decltype(vector3::y)>(pt.y),static_cast<decltype(vector3::z)>(pt.z) };
    }
    auto conv(point_2f pt) {
        return vector2{ static_cast<decltype(vector2::x)>(pt.x), static_cast<decltype(vector2::y)>(pt.y) };
    }

    template <std::size_t TSize>
    auto conv(const fstring<TSize>& s) {
        return string(s.data());
    }

    std::ifstream in_bstream(const fs::path& path) {
        std::ifstream is(path, std::ios::binary);
        is.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        return is;
    }
    template <class T=std::ofstream>
    T out_bstream(const fs::path& path) {
        T os(path, std::ios::binary);
        os.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        return os;
    }

    /*std::ostream& operator<<(std::ostream& os, const vector3& v) {
        os << "[" << v.x << " " << v.y <<" " << v.z<< "]";
        return os;
    }
    std::ostream& operator<<(std::ostream& os, const point_3f& v) {
        os << "[" << v.x << " " << v.y << " " << v.z<< "]";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const vector2& v) {
        os << "[" << v.x << " " << v.y << "]";
        return os;
    }
    std::ostream& operator<<(std::ostream& os, const point_2f& v) {
        os << "[" << v.x << " " << v.y << "]";
        return os;
    }
    std::ostream& operator<<(std::ostream& os, const matrix4x4& v) {
        os << "[" << v.a1 << " " << v.a2 << " " << v.a3<<" "<<v.a4 << "\n";
        os << v.b1 << " " << v.b2 << " " << v.b3 << " " << v.b4 << "\n";
        os << v.c1 << " " << v.c2 << " " << v.c3 << " " << v.c4 << "\n";
        os << v.d1 << " " << v.d2 << " " << v.d3 << " " << v.d4 << "]";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const std::array<float, 9> arr) {
        for (int i = 0;i < arr.size();++i) {
            os << arr[i]<<' ';
            if ((i+1) % 3 == 0)
                os << '\n';
        }
        return os;
    }*/


    //utils for assimp
    template <typename T = ai_real>
    constexpr aiVector2t<T> aiconv(vector2 v) {
        return aiVector2t<T>(static_cast<T>(v.x), static_cast<T>(v.y));
    }
    template <typename T = ai_real>
    constexpr aiVector3t<T> aiuvconv(vector2 uv, T c = 0) {
        return aiVector3t<T>(static_cast<T>(uv.x), static_cast<T>(uv.y), c);
    }

    template <typename T = ai_real>
    constexpr aiVector3t<T> aiconv(vector3 v) {
        return aiVector3t<T>(static_cast<T>(v.x), static_cast<T>(v.y), static_cast<T>(v.z));
    }

    aiFace aiconv(triangle v) {
        aiFace r;
        using T = std::remove_pointer_t<decltype(r.mIndices)>;
        r.mNumIndices = 3;
        r.mIndices = new T[]{ static_cast<T>(v.a), static_cast<T>(v.b), static_cast<T>(v.c) };
        return r;
    }

    aiString aiconv(string str) {
        //TODO вход¤щ¤¤ строка может содержать символы после \0, возможно стоит искуственно ограничить конвертацию
        return aiString(str);
    }
    //end of utils for assimp
}