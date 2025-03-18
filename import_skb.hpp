#pragma once
#include "pch.h"
#include "sern.hpp"

namespace br2proj::skb {
    struct skb_header {
        int32 unkown1;//TODO it's verion]
        int32 bones_count;
    };

    struct skb_bone {
        fstring<24> name;//TODO. кости 88 89 90 [RAYNE_DRESS.SKB] содержали доп данные после \0. Может быть, это предыдущие имена
        int32 unkown1;
        int32 parent_bone_index;
        int32 sibling_bone_index;
        array<float32, 9> unkown2;//looks like are matrix
    };

    struct skb_file {
        skb_header header;
        vector<skb_bone> bones;

        static skb_file sern_read(std::istream& is) {
            skb_file res{};
            sern::rread(is, res.header);
            sern::rread(is, res.bones, res.header.bones_count);
            return res;
        }
    };
}