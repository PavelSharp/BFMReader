#pragma once
#include "pch.h"
#include "sern.hpp"
#include "pod3_crc.hpp"

namespace br2proj::pod3
{
	//«аголовок занимает 288 байт, доказано, так как первый файл всегда имеет смещение 288 
	struct pod3_header {
		//TODO перепроверить efstring, ибо предупреждение от vs
		//efstring<"POD3"> ident;
		//https://stackoverflow.com/questions/1826464/c-style-strings-as-template-arguments
		fstring<4> ident;
		uint32 checksum;//CRC-32/MPEG-2 дл¤ 280 байт этого заголовка
		fstring<80> comment;
		int32 entries_count;
		int32 journal_count;
		int32 revision;//todo eint32<1000>
		int32 priority;//todo eint32<1000>. todo перепроверить 1000 от файлов других платформ (ps2, xbox 360)
		fstring<80> auther;
		fstring<80> copyright;
		uint32 entries_offset;

		int32 unkown1;//maybe entries_crc
		int32 paths_bytes;
		int32 unkown2;//0 
		int32 unkown3;//Br2=-1. Br1 PCENGVOX.POD = 0
		uint32 unkown4;//Br2=-1 except for LANGUANGE.pod 418b3b99 and Br1. maybe audit_crc
	};

	struct pod3_entry {
		uint32 path_offset;
		int32 size;
		uint32 depth;
		uint32 timestamp;
		uint32 checksum;//CRC-32/MPEG-2 https://crccalc.com/
	};

	struct pod3_journal_entry {
		enum class log_action:int32 {add = 0, remove, change, size};

		fstring<32> user_name;
		uint32 timestamp;
		log_action action;
		fstring<256> path;
		uint32 old_timestamp;
		uint32 old_size;
		uint32 new_timestamp;
		uint32 new_size;
	};

	struct pod3_file {
		pod3_header header;
		//Usually binary data is located here
		//In addition, the decompiled br2 code reads entries, paths, journal continuously
		vector<pod3_entry> entries;
		vector<string> paths;
		vector<pod3_journal_entry> journal;

		/*bytestream extract_file(std::istream& is, std::size_t ind) {
			is.seekg(static_cast<std::streampos>(entries[ind].offset));
			bytestream bs(std::istreambuf_iterator(is), entries[ind].size);
			if (bs.size() != entries[ind].size) throw "read error";
			return bs;
		}*/
		//pod3_entry хранит смещение в виде 32 битного числа, а значит, максимальный размер pod3-архива 2GB или 4GB(учточнить) 

		static pod3_file sern_read(std::istream& is) {
			pod3_file res{};

			sern::rread(is, res.header);
			
			is.seekg(static_cast<std::streampos>(res.header.entries_offset));
			
			sern::rread(is, res.entries, res.header.entries_count);
			
			//todo сохранить в поле paths
			auto paths_data= sern::reader<vector<char>>::read(is, res.header.paths_bytes);
			sern::rread(is, res.journal, res.header.journal_count);
			//todo. отдельный метод, который переупор¤дочивал бы строки, что бы entry[i]=path[i]
			res.paths.resize(res.header.entries_count);
			for (std::size_t i = 0; i < res.entries.size(); i++) {

				std_ext::copy_while(paths_data.begin() + res.entries[i].path_offset,
					       paths_data.end(), std::back_inserter(res.paths[i]),
					[](char c) {return c != '\0'; });
			}

			// 		
			//q[0] = 'q';
			//std::cout << std::is_trivial_v<efstring<"678">>;
			//std::cout << sizeof(q);
			//std::cout << res.header.journal_count << '\n';
			//std::cout << res.header.unkown1 << '\n' << res.header.unkown4 << '\n';
			//std::cout << is.tellg() << '\n';
			// 
			//std::cout << is.tellg() << '\n';
			//auto p = is.tellg();
			//is.seekg(static_cast<std::streampos>(p));
			//std::cout << '\n' << pod_entry_crc(std::istreambuf_iterator<char> {is}, res.header.journal_count *32)<<'\n';

			//std::cout << res.header.journal_count << '\n';
			//std::cout << res.header.unkown1 << '\n' << res.header.unkown2 << res.header.unkown3 << "\n!" << res.header.unkown4 << '\n';

			//sern::rread(is, res.paths, res.header.entries_count, [](char c) {return c != '\0';});
			//std::cout <<std::hex<< res.header.checksum << '\n';
		

			//std::cout << res.entries[0].offset << ' ' << res.entries[0].size << '\n' <<std::hex<< res.entries[0].checksum;
			//std::istreambuf_iterator<char> i1{ is };
			//is.seekg(static_cast<std::streampos>(res.entries[0].offset));
			//std::cout << '\n' << pod_entry_crc_n(std::istreambuf_iterator<char> {is}, res.entries[0].size);
			//std::cout << res.header.flag0<<' '<<res.header.flag1 << '!';
			//std::cout << std::hex<<res.header.checksum<< ' '<<res.header.unkown1<<'!';
			return res;
		}
	};
}

