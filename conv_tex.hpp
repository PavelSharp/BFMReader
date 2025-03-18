#pragma once
#include "pch.h"
#include "import_tex.hpp"
#include "utils.hpp"
#include "sern.hpp"
#include <tiffio.h>
//#include "tiffio.hxx"

//TIFFWriteEncodedStrip()
//https://stackoverflow.com/questions/4624144/c-libtiff-read-and-save-file-from-and-to-memory
//при просмотре, некоторые изображения лучше преобразовать в 1600x1200; 1280x1024
//https://stackoverflow.com/questions/3451806/getting-error-when-trying-to-apply-extrasamples-tag-to-a-tiff-file-to-be-writt
namespace br2proj::tex {
	class conv_tex {
    public:
        bool enable_compression = true;
        bool save_mipmaps = true;
    private:
        void tiff_init_image(TIFF* tif, size_t w, size_t h, int samples_per_pixel, bool is_palette) {
            TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
            TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
            TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);

            TIFFSetField(tif, TIFFTAG_COMPRESSION, enable_compression ? COMPRESSION_LZW : COMPRESSION_NONE);
            TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel);
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, is_palette?PHOTOMETRIC_PALETTE:PHOTOMETRIC_RGB);
            TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            ////TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
        }

        void to_tiff_indexed8(const tex_file& tex, TIFF* tif) {
            const auto& data = std::get<tex_indexed8>(tex.data);

            auto tables = std_ext::split_many(data.palette, [](pixel_rgb px) {return std::make_tuple(
                (short)(px.r), (short)(px.g), (short)(px.b));
                });//todo delete pointers

            size_t w = tex.header.width, h = tex.header.height;
            for (auto mip_it = data.mipmaps.begin(); mip_it != data.mipmaps.end(); mip_it++, w >>= 1, h >>= 1) {

                if (w != tex.header.width)
                    TIFFWriteDirectory(tif);

                tiff_init_image(tif, w, h, 1, true);

                TIFFSetField(tif, TIFFTAG_COLORMAP, std::get<0>(tables), std::get<1>(tables), std::get<2>(tables));
                TIFFWriteEncodedStrip(tif, 0, (void*)mip_it->data(), w * h);
            }
        }


        void to_tiff_indexed_alpha(const tex_file& tex, TIFF* tif) {
            const auto& data = std::get<tex_indexed8_alpha>(tex.data);

            auto tables = std_ext::split_many(data.palette, [](pixel_rgb px) {return std::make_tuple(
                (short)(px.r), (short)(px.g), (short)(px.b));
                });//todo delete pointers
            //todo _TIFFmalloc(), _TIFFrealloc(), и _TIFFfree() 
            size_t w = tex.header.width, h = tex.header.height;
            auto scan = std::make_unique_for_overwrite<byte[]>(2 * w * h);
            auto mip_it = data.mipmaps.begin();
            auto alp_it = data.alphas.begin();
            for (; mip_it != data.mipmaps.end(); mip_it++, alp_it++, w >>= 1, h >>= 1) {

                if (w != tex.header.width)
                    TIFFWriteDirectory(tif);

                tiff_init_image(tif, w, h, 2, true);

                TIFFSetField(tif, TIFFTAG_COLORMAP, std::get<0>(tables), std::get<1>(tables), std::get<2>(tables));

                const uint16 extras[] = { EXTRASAMPLE_UNASSALPHA };
                TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, extras);

                auto index_it = mip_it->begin();
                auto av_it = alp_it->begin();

                size_t q = 0;
                for (size_t i = 0;i < h; i++) {
                    for (size_t j = 0;j < w;j++) {
                        scan[q++] = *index_it++;
                        scan[q++] = *av_it++;
                    }
                }

                TIFFWriteEncodedStrip(tif, 0, (void*)scan.get(), w * h * 2);
            }
        }


        void to_tiff_argb(const tex_file& tex, TIFF* tif) {
            const auto& data = std::get<tex_argb>(tex.data);
            size_t w = tex.header.width, h = tex.header.height;
            auto scan = std::make_unique_for_overwrite<uint32[]>(w * h);

            for (auto mip_it = data.mipmaps.begin(); mip_it != data.mipmaps.end(); mip_it++, w >>= 1, h >>= 1) {

                if (w != tex.header.width)
                    TIFFWriteDirectory(tif);

                tiff_init_image(tif, w, h, 4, false);
                auto index_it = mip_it->begin();
                for (size_t i = 0;i < h;i++) {
                    for (size_t j = 0;j < w;j++) {
                        scan[j] = guarantee_rgba(*index_it++);
                    }
                    TIFFWriteScanline(tif, scan.get(), i, 0);
                }
            }
        }

        void tiff_do_conv(const tex_file& tex, TIFF* tif) {
            std::cout << (int)tex.header.format.val << 'F';
            switch (tex.header.format)
            {
            case formats::Indexed8: to_tiff_indexed8(tex, tif);break;
            case formats::Indexed8Alpha: to_tiff_indexed_alpha(tex, tif);break;
            case formats::ARGB: to_tiff_argb(tex, tif);break;
            default: throw;
            }
        }
	public:
        //todo accept time stamp from pod
        //todo save tif in memory
		/*void to_tiff(const tex_file& tex, std::ostream& os) {
            TIFF* mtif = TIFFStreamOpen("MemTIFF", &os);
            tiff_do_conv(tex, tif);
            TIFFClose(tif);
		}*/
        void to_tiff(const tex_file& tex, const fs::path& out) {
            TIFF* tif = TIFFOpenW(out.generic_wstring().data(), "w");
            tiff_do_conv(tex, tif);
            TIFFClose(tif);
        }
        void to_tiff(std::istream& in, const fs::path& out) {
            to_tiff(sern::reader<tex_file>::read(in), out);
        }
        void to_tiff(const fs::path& in, const fs::path& out) {
            std::ifstream is=utils::in_bstream(in);
            to_tiff(is, out);
        }

	};
}