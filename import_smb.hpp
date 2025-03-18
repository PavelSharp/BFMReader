#pragma once
#include "pch.h"
#include "sern.hpp"

//WW_TESLA_GRATE.SMB <-- Toolset Crash
namespace br2proj::smb {
	using point_2f = vector2t<float32>;
	using point_3f = vector3t<float32>;
	using triangle = trianglet<uint16>;
	using bound_box = bound_boxt<point_3f>;

	struct smb_header {
		int32 ver_id;//6E(windows), 0A(PS2 or Br2 Demo). 04(Br1)
		int32 meshes;
		int32 collisions_count;
		int32 impact_points;

		int32 materials_count;
		int32 unkown2;
		int32 unkown3;
	};

	struct smb_material {
		int32 alpha_byte;
		byte_array<12> unk1;
		fstring<72> diffuse_name;
		fstring<72> bumpmap_name;
		fstring<72> glossmap_name;
		byte_array<128> unk4;
	};

	struct smb_mesh_info {
		fstring<32> name;
		int16 material_index;
		byte_array<32> unkown1;
		int32 length;
		int32 unkown2;
		int32 vertices;
		int32 triangles;
		int32 unkown3;
	};

	struct smb_point {
		point_3f vertex;
		point_3f normal;
		point_2f uv;
		array<float32, 12> unkown;
	};

	struct smb_mesh {
		vector<smb_point> points;
		vector<triangle> triangles;

		static smb_mesh sern_read(std::istream& is, const smb_mesh_info& info) {
			smb_mesh res{};
			sern::rread(is, res.points, info.vertices);
			sern::rread(is, res.triangles, info.triangles);
			return res;
		}
	};

	struct smb_collission {
		fstring<36> name;
		int32 points_count;
		int32 triangles_count;
		vector<point_3f> points;
		vector<triangle> triangles;
		byte_array<36> unkown1;//TODO размер Вероятно, triangles_count[20.11.2024]
		static smb_collission sern_read(std::istream& is) {
			smb_collission res;
			sern::many_rread(is, res.name, res.points_count, res.triangles_count);
			sern::rread(is, res.points, res.points_count);
			sern::rread(is, res.triangles, res.triangles_count);
			std::cout << is.tellg() << '\n';
			sern::rread(is, res.unkown1);
			return res;
		}
	};


	struct smb_file {
		smb_header header;
		vector<smb_material> materials;
		vector<smb_collission> collisions;

		bound_box unkown1;
		vector<smb_mesh_info> mesh_info;
		vector<smb_mesh> meshes;

		static smb_file sern_read(std::istream& is){
			smb_file res{};
			sern::rread(is,res.header);
			//std::cout << is.tellg() << '\n';
			sern::rread(is, res.materials, res.header.materials_count);

			std::cout << is.tellg() << '\n';

			sern::rread(is, res.collisions, res.header.collisions_count);
			sern::rread(is, res.unkown1);
			//sern::rread(is, res.unkown1);
			//sern::rread(is, res.unkown2);
			//748. 1mat=344 2 mat = 704. 3 mat = 1064
			std::cout << is.tellg() << '\n';
			//impact info
			
			sern::rread(is, res.mesh_info, res.header.meshes);
			
			is.seekg((is.tellg() + static_cast<std::streampos>(15)) & ~15);
			
			auto it = res.mesh_info.begin();
			res.meshes.reserve(res.header.materials_count);
			std::generate_n(std::back_inserter(res.meshes), res.header.meshes, [&]() {return sern::reader<smb_mesh>::read(is, *it++); });

			
			//3 mat - MANS_TOMB_DEBRIS04.SMB
			return res;
		}
	};
}