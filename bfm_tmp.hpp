#pragma once
#include "pch.h"
#include "import_bfm.hpp"
#include "utils.hpp"

//Ќазначение этих структур - временное представление, удобное дл¤ экспорта в большинство форматов
namespace br2proj::bfm {
    struct tmp_ppoint {
        vector3 pos;
        int bone_ind = -1;
    };

    struct tmp_joint {
        int bone_ind=-1;
        float wieght;
    };

    struct tmp_point {
        vector3 pos;
        vector3 normal_vec;
        vector2 uv;
        vector3 binorm_vec;
        vector3 tang_vec;
        array<tmp_joint, VertMaxBones> joints;
    };

    struct tmp_mesh {
        string name;
        int material_ind;
        int bone_ind;

        vector<tmp_point> points;
        vector<triangle> triangles;
       // std::unordered_map<int,vector<int>> infl_bones;

        vector<tmp_ppoint> raw_points;

    };

    struct tmp_material {
        string diffuse_name;
        string bumpmap_name;
        string glossmap_name;
    };

    struct tmp_bone {
        string name;
        int parent_ind;
        matrix4x4 transform;
        matrix4x4 global_transform;
    };

    struct tmp_model {
        vector<tmp_material> materials;
        vector<tmp_mesh> meshes;
        vector<tmp_bone> skeleton;
    };

    using namespace br2proj::utils;
    using namespace br2proj::skb;

    vector<tmp_bone> build_skeleton(const bfm_model& imp) {
        vector<tmp_bone> res;
        res.reserve(imp.skb.bones.size());
        for (int i = 0;i < imp.skb.bones.size();++i) {

            const skb_bone& ibone = imp.skb.bones[i];
            if (ibone.parent_bone_index >= i) throw "Bones not sorted";//TODO check for all models

            tmp_bone obone;
            obone.name = conv(ibone.name);
            obone.parent_ind = ibone.parent_bone_index;

            obone.transform = matrix4x4{};
            const vector3 ipos = conv(imp.bfm.bones.depth[i]);
            aiMatrix4Translation(&obone.transform, &ipos);

            obone.global_transform = (obone.parent_ind != -1) ? res[obone.parent_ind].global_transform * obone.transform : matrix4x4{};

            res.push_back(obone);
        }
        return res;
    }

    vector<tmp_material> build_materials(const bfm_file& imp) {
        vector<tmp_material> res;
        res.reserve(imp.materials.size());
        std::transform(imp.materials.cbegin(), imp.materials.cend(), std::back_inserter(res), [](const  bfm_material& imat) {
            tmp_material omat;
            omat.diffuse_name = conv(imat.diffuse_name);
            omat.bumpmap_name = conv(imat.bumpmap_name);
            omat.glossmap_name = conv(imat.glossmap_name);
            return omat;
        });
        return res;
    }

    tmp_model prepare_to_export(const bfm_model& imp) {
        tmp_model res{};
        res.skeleton = build_skeleton(imp);
        res.materials = build_materials(imp.bfm);

        for (int i = 0;i < imp.bfm.meshes.size(); ++i) {
            const bfm_mesh& imesh = imp.bfm.meshes[i];
            const bfm_mesh_desc& idesc = imp.bfm.mesh_desc[i];
            const bfm_part& ipart = imp.bfm.parts[idesc.part_index];

            tmp_mesh omesh;
            omesh.name = conv(ipart.name);
            omesh.material_ind = idesc.material_index;
            omesh.triangles.assign(imesh.triangles.begin(), imesh.triangles.end());
            omesh.bone_ind = ipart.bone_index;

            //auto d = res.skeleton[ipart.bone_index].transform;
           // d.Inverse();

            for (int j = 0;j < imesh.points.size();++j) {
                const bfm_point& ipt = imesh.points[j];
                tmp_point opt{};
                opt.normal_vec = conv(ipt.normal_vec);
                opt.uv = conv(ipt.uv);
                opt.binorm_vec = conv(ipt.binorm_vec);
                opt.tang_vec = conv(ipt.tang_vec);
           
                for (int n = 0; n < ipt.sub_count;++n) {
                    const matrix4x4 final_trans = res.skeleton[ipt.bone_index[n]].global_transform * ipt.weights[n];
                    vector3 vertex = conv(ipt.subpoints[n]);
                    opt.pos += final_trans * vertex;
                    opt.joints[n] = tmp_joint{ipt.bone_index[n], ipt.weights[n] };
                    //omesh.infl_bones[ipt.bone_index[n]].push_back(j);
                    omesh.raw_points.emplace_back(res.skeleton[ipt.bone_index[n]].global_transform * vertex, ipt.bone_index[n]);
                }

                if (ipt.sub_count < VertMaxBones)
                    opt.joints[ipt.sub_count].bone_ind = -1;//TODO проверить, что если ipt.bone_index[n]=-1 то все следующие точки также отсутствуют

                //opt.pos*=d;

                omesh.points.push_back(opt);
            }
            res.meshes.push_back(omesh);
        }
        return res;
    }
  
}