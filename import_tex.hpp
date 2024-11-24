#pragma once
#include "pch.h"
//~XERX.tex
//PARK_BRANCHES_01_A.TEX
//PARK_BRANCH_SILHOUETTE.TEX

namespace br2proj::tex {
	constexpr size_t TexPaletteSize = 256;

	using pixel_rgba = uint32;
	
	constexpr pixel_rgba guarantee_rgba(pixel_rgba pix) noexcept{
		byte* p = (byte*)&pix;
		//TODO если на архитектуре иной порядок байт чем на этой, то нужно ли делать flip?
		return
			(static_cast<uint32>(p[2])) +
			(static_cast<uint32>(p[1]) << 8) +
			(static_cast<uint32>(p[0]) << 16) +
			(static_cast<uint32>(p[3]) << 24);
	}
	//using pixel_rgb = std::array<byte, 3>;
	//TODO!!! Гарантируется, ли здесь отсутствие нежелательного выравнивания?(sizeof == 3)
 	struct pixel_rgb {byte r, g, b;};

	enum class formats:uint32 { Indexed8 = 1, Indexed8Alpha, ARGB,		first=Indexed8, last=ARGB };
	//bytes per pexel
	/*constexpr int get_bpp(formats frm) {
		switch (frm)
		{
			case formats::Indexed8:return 1;
			case formats::Indexed8Alpha:return 1;
			case formats::ARGB:return 4;
			default: throw;
		}
	}*/
	struct tex_header {
		
		int32 ver_id;
		eb_enum<formats> format;
		int32 width;
		int32 height;
		
		int32 unkown1;
		int32 mipmaps_exp2;
		int32 unkown2;
		int32 unkown3;

		constexpr auto mipmaps_count() const noexcept {
			return mipmaps_exp2 + 1;
		}

		constexpr auto mipmap_pixels(size_t level) const noexcept {
			return (static_cast<size_t>(width) * height) >> (2 * level);
		}
	};

	using tex_indexed24_mip = vector<byte>;
	using tex_indexed24_alpha_mip = vector<byte>;
	using tex_rgba_mip = vector<pixel_rgba>;

	struct tex_indexed8 {
		array<pixel_rgb, TexPaletteSize> palette;
		vector<tex_indexed24_mip> mipmaps;
		static auto sern_read(std::istream& is, const tex_header& header) {
			tex_indexed8 res{};
			sern::rread(is, res.palette);

			res.mipmaps.reserve(header.mipmaps_count());
			std_ext::generate_ni(std::back_inserter(res.mipmaps), header.mipmaps_count(), [&](size_t i)
				{return sern::reader<tex_indexed24_mip>::read(is, header.mipmap_pixels(i));});

			return res;
		}
	};
	struct tex_indexed8_alpha:tex_indexed8 {
		vector<tex_indexed24_alpha_mip> alphas;
		static auto sern_read(std::istream& is, const tex_header& header) {
			tex_indexed8_alpha res{tex_indexed8::sern_read(is, header)};

			res.alphas.reserve(header.mipmaps_count());
			std_ext::generate_ni(std::back_inserter(res.alphas), header.mipmaps_count(), [&](size_t i)
				{return sern::reader<tex_indexed24_alpha_mip>::read(is, header.mipmap_pixels(i));});
			
			return res;
		}

		constexpr std::string_view sern_field(std::size_t ind) const {
			return std::array{ "palette", "mipmaps", "alphas" }[ind];
		}

		template<class TOS>
		constexpr void sern_write(TOS& os) const {
			sern2::write_fields(os, *this, palette, mipmaps, alphas);
		}

	};
	struct tex_argb {
		vector<tex_rgba_mip> mipmaps;
		static auto sern_read(std::istream& is, const tex_header& header) {
			tex_argb res{};

			res.mipmaps.reserve(header.mipmaps_count());
			std_ext::generate_ni(std::back_inserter(res.mipmaps), header.mipmaps_count(), [&](size_t i)
				{return sern::reader<tex_rgba_mip>::read(is, header.mipmap_pixels(i));});

			return res;
		}

		//constexpr static auto sern_write_size_policy = sern::size_write_policy::Skip;
		//inline void sern_write(std::ostream& os) const {
		//	sern::write(os, mipmaps);
		//}
	};

	struct tex_file {
		tex_header header;
		std::variant<tex_indexed8, tex_indexed8_alpha, tex_argb> data;

		static tex_file sern_read(std::istream& is) {
			tex_file res{};
			sern::rread(is, res.header);
			//TODO Чтение после конца потока должно давать нули. Ещё раз перепроверить, что такие изображения существуют и если да, то написать обертку для istream
			switch (res.header.format)
			{
				case formats::Indexed8: res.data = sern::reader<tex_indexed8>::read(is, res.header);break;
				case formats::Indexed8Alpha: res.data = sern::reader<tex_indexed8_alpha>::read(is, res.header);break;
				case formats::ARGB: res.data = sern::reader<tex_argb>::read(is, res.header);break;
				default:throw;
			}
			return res;
		}

		//inline void sern_write(std::ostream& os) const {
		//	sern::many_write(os, header, data);
		//}
	};
}


/*if (header.format != tex_header::RGBA) {
	
	std::cout << "state" << is.good()<<' ';
	sern::rread(is, res.table, 256);
	sern::rread(is, res.indices, sz);
	if (header.type == tex_header::format::Indexed24alpha) {
		sern::rread(is, res.alpha, sz);
	}
}
else {
	sern::rread(is, res.raw, sz);
}*/