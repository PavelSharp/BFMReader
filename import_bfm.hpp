#pragma once
#include "pch.h"
#include "sern.hpp"
#include "import_skb.hpp"

//todo ps2 and any platforms
namespace br2proj::bfm {
    using point_2f = vector2t<float32>;
    using point_3f = vector3t<float32>;
    using triangle = trianglet<uint16>;
    using bound_box = bound_boxt<point_3f>;
    
    //the maximum number of bones that can be connected to a single vertex
    constexpr int VertMaxBones = 4;

    //Ќазначение этих структур - максимально точно передать структуру файла bfm
    struct bfm_header {
        int32 unkown1;//All is 6[TODO it's verion]
        int32 unkown2;//All is 1
        int32 meshes;
        int32 bones;
        int32 materials;
        int32 attached;
        int32 unkown3;//All is 3
        int32 unkown4;//All is 0
        fstring<80> skeleton;
    };


    struct bfm_part {
        fstring<30> name;//D
        int32 bone_index;//[NEW 24.11.2024] индекс кости относительно которой задан bound_box
        bound_box box;//Ќужно уточнени¤, возможно это не bound box, совпадало частично. 
    };

    struct bfm_attached_part {
        fstring<24> name;
        int32 bone_index;
        bound_box unkown1;
        bound_box unkown2;
    };

    struct bfm_material {
        byte_array<16> unkown1;
        fstring<72> diffuse_name;//D
        fstring<72> bumpmap_name;//D
        fstring<72> glossmap_name;//D
        byte_array<128> unkown2;
    };

    struct bfm_bones {
        vector<point_3f> depth;
        vector<bound_box> unkown;
        vector<int32> bone_type;
        vector<int32> child_ind;

        static bfm_bones sern_read(std::istream& is, const bfm_header& head) {
            bfm_bones res;
            sern::rread(is, res.depth, head.bones);
            sern::rread(is, res.unkown, head.bones);
            sern::rread(is, res.bone_type, head.bones);
            sern::rread(is, res.child_ind, head.bones);
            return res;
        }
    };

    struct bfm_mesh_desc {
        int32 unkown1;// All is 3
        int32 material_index;//D
        int32 unkown2;
        int16 part_index; //[09.03.2025] очевидно, что это принадлежит unkown2_data
        vector<int16> unkown2_data;//TODO провести тесты дл¤ всех моделей bfm. ¬озможно, количесвто этих значений сильно ограничено
        int32 unkown3;
        vector<int16> unkown3_data;//TODO провести тесты дл¤ всех моделей bfm. ¬озможно, количесвто этих значений сильно ограничено
        int32 unkown4;
        int32 bytes_count;//size of the vertex and triangle data for mesh
        int32 unkown5;
        int32 points_count;//D
        int32 triangles_count;//D
        int32 unkown6;//MAYBE bones_count
        static bfm_mesh_desc sern_read(std::istream& is) {
            bfm_mesh_desc res{};
            sern::many_rread(is, res.unkown1, res.material_index, res.unkown2, res.part_index);
            if (res.material_index == -842150451) {
                res.material_index = 0;//TODO need more tests. ¬ таком случае, количесвто meshes должно быть равно 1 'Materail index is default(-842150451) but meshes count not 1'
            }
            sern::rread(is, res.unkown2_data, res.unkown2 - 1);
            sern::rread(is, res.unkown3);
            sern::rread(is, res.unkown3_data, res.unkown3);
            sern::many_rread(is, res.unkown4, res.bytes_count, res.unkown5, res.points_count, res.triangles_count, res.unkown6);
            return res;
        }
    };

    //“акое предстовление, когда вместо одной вершины с несколькими прив¤занными кост¤ми и с соотвеющеми весами,
    //используетс¤ несколько субточек, при этом на одну субточку приходитьс¤ один вес и одна кость
    //примен¤етс¤ в md5mesh
    struct bfm_point {
        ebint32<1, VertMaxBones> sub_count;//D
        array<point_3f, VertMaxBones>  subpoints;//D
        array<float32, VertMaxBones> weights;//D
        point_3f normal_vec;//D
        array<int32, VertMaxBones> bone_index;
        point_2f uv;//D
        point_3f binorm_vec;//D
        point_3f tang_vec;//D
    };

    struct bfm_mesh {
        vector<bfm_point> points;//D
        vector<triangle> triangles;//D
        static bfm_mesh sern_read(std::istream& is, const bfm_mesh_desc& mesh_desc) {
            bfm_mesh res;
            sern::rread(is, res.points, mesh_desc.points_count);
            sern::rread(is, res.triangles, mesh_desc.triangles_count);
            return res;
        }
    };

    struct bfm_file {
        bfm_header header;
        vector<bfm_part> parts;
        vector<bfm_attached_part> att_parts;
        vector<bfm_material> materials;
        bfm_bones bones;
        int32 mesh_desc_count;
        vector<bfm_mesh_desc> mesh_desc;
        ealign<16> align;
        vector<bfm_mesh> meshes;
        //TODO остаютс¤ недочитанные данные. Ќапр., Rayne.BFM

        static bfm_file sern_read(std::istream& is) {
            bfm_file res;

            sern::rread(is, res.header);
            sern::rread(is, res.parts, res.header.meshes);
            sern::rread(is, res.att_parts, res.header.attached);
            sern::rread(is, res.materials, res.header.materials);
            sern::rread(is, res.bones, res.header);
            sern::rread(is, res.mesh_desc_count);
            sern::rread(is, res.mesh_desc, res.mesh_desc_count);
            
            sern::rread(is, res.align);
                       
            auto it = res.mesh_desc.begin();
            res.meshes.reserve(res.header.meshes);
            std::generate_n(std::back_inserter(res.meshes), res.header.meshes, [&]() {return sern::reader<bfm_mesh>::read(is, *it++); });

            return res;
        }
    };

    using namespace  br2proj::skb;
    struct bfm_model {
        bfm_file bfm;
        skb_file skb;
        static bfm_model import(const fs::path& path) {
            bfm_model res{};
            std::ifstream bif(path, std::ios_base::binary);
            sern::rread(bif, res.bfm);

            auto skb_name = std::string(res.bfm.header.skeleton.data());
            skb_name.back() = 'b';

            std::ifstream sif(path.parent_path() / skb_name, std::ios_base::binary);

            sern::rread(sif, res.skb);

            if (res.skb.header.bones_count != res.bfm.header.bones) throw "bones count error";//todo asserts

            return res;
        }
    };
}

