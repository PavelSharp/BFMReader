#include "pch.h"
#include "import_bfm.hpp"
#include "bfm_tmp.hpp"
#include "utils.hpp"
#include "sern.hpp"
#include "import_pod3.hpp"
#include "import_smb.hpp"
#include "import_tex.hpp"
#include "BFMReader.h"
#include "conv_tex.hpp"
#include "sern_json.hpp"

using namespace br2proj;
using namespace br2proj::bfm;

vector<aiNode*> obones;
aiNode* build_hierarchy(const tmp_model& model) {

    obones.reserve(model.skeleton.size());

    for (int i = 0; i < model.skeleton.size(); ++i) {
        const tmp_bone& ibone = model.skeleton[i];
        aiNode* obone = obones.emplace_back(new aiNode());
        obone->mName = ibone.name;
        if (ibone.parent_ind != -1) {
            obone->mParent = obones[ibone.parent_ind];// TODO необходима ли эта инфомрация?
            obones[ibone.parent_ind]->addChildren(1, new aiNode*[] { obone });
        } else if (i != 0) throw "odd case";

        obone->mTransformation = ibone.transform;
    }
    return obones[0];//TODO изолированные кости (если такое существует)
}

/*vector<aiNode*> obones2;
aiNode* build_hierarchy2(const exp_model& model) {

    obones2.reserve(model.skeleton.size());

    for (int i = 0; i < model.skeleton.size(); ++i) {
        const exp_bone& ibone = model.skeleton[i];
        aiNode* obone = obones2.emplace_back(new aiNode());
        obone->mName = "!"+ibone.name;
        if (ibone.parent_ind != -1) {
            obone->mParent = obones2[ibone.parent_ind];// TODO необходима ли эта инфомрация?
            obones2[ibone.parent_ind]->addChildren(1, new aiNode * [] { obone });
        }
        else if (i != 0) throw "odd case";

        obone->mTransformation = ibone.transform;
    }
    return obones2[0];//TODO изолированные кости (если такое существует)
}*/


aiScene* build_scene(const tmp_model& model) {
    using namespace br2proj::utils;

    aiNode* skel_root = build_hierarchy(model);
    //aiNode* mesh_root = build_hierarchy2(model);
    aiMesh** omeshes = std_ext::transform_newi(model.meshes, [&](const tmp_mesh& imesh, const int mesh_i) {
        aiMesh* omesh = new aiMesh();
        omesh->mName = imesh.name;
        omesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
        omesh->mNumVertices = imesh.points.size();

        std::unordered_map<int, vector<aiVertexWeight>> inflbns;

        for (int i = 0;i < imesh.points.size(); ++i) {
            for (const auto& js : imesh.points[i].joints) {
                if (js.bone_ind == -1) break;
                inflbns[js.bone_ind].push_back(aiVertexWeight(i, js.wieght));
            }
        }

        aiBone** bones = std_ext::transform_new(inflbns, [&](const auto& p) {
            auto bn = new aiBone();

            bn->mWeights = std_ext::copy_new(p.second);
            bn->mNumWeights = p.second.size();
            // bn->
             /*vector <aiVertexWeight> q;
             for (int i = 0;i < p.second.size();++i) {
                 for (int j = 0;j < vert_max_bones;++j) {
                     if (imesh.points[j].joints[j].bone_ind == -1) break;
                     q.push_back(aiVertexWeight(i, imesh.points[i].joints[j].wieght));
                 }
             }*/

            matrix4x4 m{ model.skeleton[p.first].global_transform };
            m.Inverse();
            bn->mOffsetMatrix = m;
            bn->mName = aiconv(model.skeleton[p.first].name);
            return bn;
            });

        omesh->mBones = bones;
        omesh->mNumBones = inflbns.size();


        auto pts = std_ext::split_many(imesh.points, [&](const tmp_point& pt) { return std::make_tuple(
            aiconv(pt.pos), aiconv(pt.normal_vec), aiuvconv(pt.uv)
        );});

        omesh->mVertices = std::get<0>(pts);
        omesh->mNormals = std::get<1>(pts);
        omesh->mTextureCoords[0] = std::get<2>(pts);
        omesh->mMaterialIndex = imesh.material_ind;



        aiFace* ofaces = std_ext::transform_new(imesh.triangles, [&](const triangle& iface) {return aiconv(iface);});
        omesh->mFaces = ofaces;
        omesh->mNumFaces = imesh.triangles.size();

        /*const auto q = obones2[imesh.bone_ind];
        q->mMeshes = std_ext::copy_new2(q->mMeshes, q->mNumMeshes, q->mNumMeshes + 1);
        q->mMeshes[q->mNumMeshes] = mesh_i;
        q->mNumMeshes++;*/
        
     
        return omesh;
        });

    aiMaterial** omaterials = std_ext::transform_newi(model.materials, [&](const tmp_material& imat, int i) {
        aiMaterial* material = new aiMaterial();
        aiString diff = aiconv(imat.diffuse_name);
        aiString bump = aiconv(imat.bumpmap_name);
        aiString gloss = aiconv(imat.glossmap_name);
        aiString name = aiconv("mat" + std::to_string(i));
        //TODO check if string empty thene don't write
        material->AddProperty(& diff, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
        material->AddProperty(&bump, AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0));
        material->AddProperty(&gloss, AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0));
        material->AddProperty(&name, AI_MATKEY_NAME);//TODO обязательно ли это?

        return material;
        });

    const std::size_t meshes_count = model.meshes.size();

    aiNode* root = new aiNode();
    root->mName = "Scene";

    aiNode* mesh_root = new aiNode();
    mesh_root->mNumMeshes = meshes_count;
    mesh_root->mMeshes = std_ext::numerate_new<unsigned int>(meshes_count);
    mesh_root->mName = "Mesh";

    root->addChildren(2, new aiNode * [] {skel_root, mesh_root});
    mesh_root->mParent = root;
    skel_root->mParent = root;

    aiScene* scene = new aiScene();
    scene->mMeshes = omeshes;
    scene->mNumMeshes = meshes_count;
    scene->mMaterials = omaterials;
    scene->mNumMaterials = model.materials.size();
    scene->mRootNode = root;
    scene->mMetaData = new aiMetadata();

    return scene;
}

template<class DirIt, class Func>
void visit_pods(DirIt first, DirIt last, std::string_view ext, Func f) {
    using namespace pod3;
    for (;first!=last; first++)
    {
        const fs::directory_entry& entry = *first;
        if (!entry.is_regular_file() || !std_ext::ends_with_ic(entry.path().generic_string(), ".pod")) continue;
        std::ifstream is(entry.path(), std::ios_base::binary);
        std::cout << entry.path() << '\n';
        if (entry.path() != R"(G:\Games\_INPUT\BR2\W32ART.pod)") continue;
        pod3_file pod = sern::reader::read<pod3_file>(is);
        for (size_t i = 0;i < pod.paths.size();++i) {
            if (pod.paths[i] != R"(ART\~XERX_BUMPMAP.TEX)"&&pod.paths[i] != R"(ART\~XERX_GLOSSMAP.TEX)") continue;//std::cout << "asd";
            if (std_ext::ends_with_ic(pod.paths[i], ext)) {
                std::cout << pod.paths[i] << '\n';
                is.seekg(pod.entries[i].depth);
                //bytestream bs = pod.extract_file(is, i);
                f(is);
            }
        }
      //  return;
    }
}

struct q {
    int a;
};

template<>
struct sern2::user_writer<std::variant<int, double>> {
    template <class TOS, class T, class... Args>
    constexpr static void write(TOS& os, T&& val, Args&&... args) {
        std::cout << "USER varint";
    }
};

template<class TOS>
class endian_ostream: public TOS {
public:
    std::endian endian = std::endian::native;

    template<class... Args>
    endian_ostream(Args&&... args):TOS(std::forward<Args>(args)...) {
    }

   /*template<class... Ts>
    constexpr void write_type(std::variant<Ts...>) {
        std::cout << "varint";
    }*/

    constexpr void write_type(int* v) {
        std::cout << "POINTER";
    }

    //TODO эта перезрузка должна быть доступна если T - арифметический тип или фиксированный массив
    template<std::size_t Extent,  class... Args> //class T, 
    constexpr void write_type(std::span<int, Extent> val, Args&&... args) {
        std::cout << "span";
    }
    constexpr void write_type(std::floating_point auto val) {
        std::cout << "a";
    }
    constexpr void write_type(std::integral auto val) {
        std::cout << "b";
    }

    /*template<std::integral Size, class... Args>
void write(const char* data, Size size, Args&&... args) {
    static_assert(sizeof...(args) == 0);
    std::cout << "QQQ!!!arr";
    TOS::write(data, static_cast<std::streamsize>(size));
}

template<class T, std::integral Size, class... Args>
void write(const T* val, Size size, Args&&... args) {
    //static_assert(sizeof...(args) == 0);
    std::cout << "!!!arr";
    sern2::write_range(*this, std::span{ val, size }, std::forward<Args>(args)...);
}*/
    /*template<class T, class... Args>
    void write(T val, Args&&... args) {
        static_assert(sizeof...(args) == 0);
        if constexpr (std::integral<T>) {
            std::cout << "i";
        }
        else if constexpr (std::floating_point<T>) {
            std::cout << "f";
        }
        else if constexpr (std::is_enum_v<T>) {
            std::cout << "e";
        }
        else {
            static_assert(false);
        }
    }
    template<class T, class... Args>
    void write(T val, Args&&... args) requires std::floating_point<T> || std::integral<T> || std::is_enum_v<T> {
        static_assert(sizeof...(args) == 0);
        if constexpr (std::floating_point<T>) {

        }
        else if constexpr (std::integral<T>) {

        }
        else if constexpr (std::is_enum_v<T>) {

        }
        else {
            static_assert(false);
        }
        write_impl(val);
    }*/




};
//todo le_ostream, br_ostream
using eofstream = endian_ostream<std::ofstream>;

void qwe(eofstream& s, const std::vector<int>& v) {
    sern2::write(s, v);
    //std::cout << sern2::is_dynamic_array<const std::vector<int>&>::value;
}

struct wr1 {
    constexpr static void sern_write(auto& os) {

    }
};

struct wr2 {
    int q;
    wr2(int a):q(a){}
    constexpr static void sern_write(auto& os) {

    }
};

struct agg {
//private:
 //   int fld;
public:
    int a;
    double b;
    float c;
    agg(int aa, double bb, float cc) {

    }
    //std::vector<int> v;
};

struct agg2 {
    char c[20]{'!','Q'};
    //std::vector<int> v;
};


std::string s = "asd";

template<class T> 
int proc(T&& field) {
    field = 123;
    //std::cout << std_ext::type_name<decltype(field)> << '\n';
    return 1;
}


/*struct person {
    std::string first_name = "Ivan";
    std::string last_name = "Ivanov";

    struct {
        std::string city_address = "Moskovskoe sh., 101, kv.101";
        std::string city = "Leningrad";
        int zip = 101101;
        struct
        {
            std::string nes1 = "nes1";
            double nes2 = 3.14;
            std::array< std::array<int, 3>, 3> nes3{ std::array<int,3>{1,2},std::array<int,3>{3,4,5} };
            std::vector<std::pair<int, const char*>> nes4{ std::make_pair(1,"a"), std::make_pair(2,"b") };
            constexpr auto sern_fields() const { return std::array{ "nes1","nes2", "nes3", "nes3", "nes4"}; }
        } nested;
        constexpr auto sern_fields() const { return std::array{ "city_address","city", "zip", "nested"}; }
    } address;
    std::vector<std::string> phones{"812 123-1234", "916 123-4567"};

    constexpr auto sern_fields() const {return std::array{"first_name","last_name", "address", "phones"}; }

   // int sfld=0;
    //template<class TOS>
    //constexpr void sern_write(TOS& os) const {
    //    sern2::write_struct(os, *this, first_name, last_name);
        //sern2::many_write(os, first_name, last_name);
    //}
};*/


struct extra {
    std::string nes1 = "nes1";
    double nes2 = 3.14159265358;
    std::array< std::array<int, 3>, 3> nes3{ std::array<int,3>{1,2},std::array<int,3>{3,4,5} };
    std::vector<std::pair<int, const char*>> nes4{ std::make_pair(1,"a"), std::make_pair(2,"b") };
    constexpr auto sern_fields() const { return std::array{ "nes1","nes2", "nes3", "nes3", "nes4" }; }

};

struct address {

    std::string city_address = "asdf sh., 101, kv.101";
    std::string city = "qwer";
    int zip = 101101;
    enum class abc_enum { it1 = -5, qqwe = -3, pav = 10, first = it1, last = pav } enumval=abc_enum::qqwe;
    extra nested;
    constexpr auto sern_fields() const { return std::array{ "city_address","city", "zip", "nested" }; }

};

struct person {
    std::string first_name = "Ivan";
    std::string last_name = "Ivanov";
    address address;
    std::vector<std::string> phones{ "812 123-1234", "916 123-4567" };
    std::unordered_map<int, std::vector<int>> map{ {0, std::vector{314,1,2,2}}, { 1,std::vector{0,1,2,2} } };
    array<double, 3> arr{3.4,3.6, 1.2};
    constexpr auto sern_fields() const { return std::array{ "first_name","last_name", "address", "phones" }; }

    // int sfld=0;
     //template<class TOS>
     //constexpr void sern_write(TOS& os) const {
     //    sern2::write_struct(os, *this, first_name, last_name);
         //sern2::many_write(os, first_name, last_name);
     //}
};



struct wrap {

    /*template<class T>
    decltype(auto) operator <<(T&& val) {
        using NR = std::remove_reference_t<T>;
        if constexpr (std_ext::is_any_same_v<NR, std::byte, unsigned char, signed char>) {
            return std::cout << static_cast<int>(val);
        }
        else
            return std::cout << std::forward<T>(val);
    }*/
    //+=string_view
    //+=char

    decltype(auto) operator <<(std::string_view v) {
        return std::cout << v;
    }

    decltype(auto) operator <<(char v) {
        return std::cout << v;
    }

};

struct tree {
    int val;
    tree* left;
    tree* right;
};


struct ci_char_traits : public std::char_traits<char>
{
    static char to_upper(char ch)
    {
        return std::toupper(static_cast<unsigned char>(ch));
    }

    static bool eq(char c1, char c2)
    {
        return to_upper(c1) == to_upper(c2);
    }

    static bool lt(char c1, char c2)
    {
        return to_upper(c1) < to_upper(c2);
    }

    static int compare(const char* s1, const char* s2, std::size_t n)
    {
        while (n-- != 0)
        {
            if (to_upper(*s1) < to_upper(*s2))
                return -1;
            if (to_upper(*s1) > to_upper(*s2))
                return 1;
            ++s1;
            ++s2;
        }
        return 0;
    }

    static const char* find(const char* s, std::size_t n, char a)
    {
        const auto ua{ to_upper(a) };
        while (n-- != 0)
        {
            if (to_upper(*s) == ua)
                return s;
            s++;
        }
        return nullptr;
    }
};


int main()
{
    using namespace std_ext;
    //tp<any_element<0, std::tuple<int, int&, int>>::type>();
   // tp<std::tuple_element_t<0, std::tuple<int, int&, int>>>();
    //std::cout << std_ext::any_size_v<std::tuple<int,int,int>>;
    //auto tyy = std::make_tuple(1, 2, 3);
    //std_ext::tp<decltype(std_ext::any_get<0>(ee))>();
    //tp<decltype(std_ext::any_get<int>(ee))>();

    // auto myArray = std::make_from_tuple<std::array<std::string, 2>>(t);

     //std::cout << members.size();
     //constexpr auto fields_count = std_ext::fields_count<sq>;

     //std::cout << std_ext::fields_count<tex_indexed8_alpha>;//br2proj::tex::tex_indexed8_alpha


     //std::cout << std::boolalpha << std_ext::can_construct<sq, 3>;
    
    auto is = utils::in_bstream(R"(G:\Games\BloodRayne 2\tests\_07\2.~XERX.TEX)");
    auto tex = tex::tex_file::sern_read(is);

    /*
        auto is = utils::in_bstream(R"(G:\Games\BloodRayne 2\tests\_07\BFM\RAYNE.BFM)");
    auto tex = br2proj::bfm::bfm_file::sern_read(is);
    */
    std::string aa;
    aa += 'a';
    aa += "qwe";
    //<wrap>
    sern2::json_ostream js2{};
    sern2::write(js2, std::basic_string<char, ci_char_traits>{"qwe"});
    sern2::write(js2, std::u8string{ u8"abc" });

    sern2::write(js2, tree{ 1, new tree{2, new tree{4,nullptr,nullptr}, nullptr}, new tree{3, nullptr, nullptr} });
    sern2::write(js2, person{});

    js2.limit = 17;
    sern2::write(js2, "asd\u001dasd");
    std::cout << js2.writer.data;
    //std::copy(js2.writer.data.begin(), js2.writer.data.end(), std::ostreambuf_iterator<char>(std::cout));
    return 0;
    sern2::write(js2, tex);//person{}
    
    //std::vector{ std::vector{1,2,3},std::vector{1,2,3},std::vector{1,2,3} }
    
    sern2::json_ostream js{};//<std::ostringstream>
    //using aq=
    //WWW<decltype(&pers.sfld)>();

    /*using namespace br2proj::tex;
    auto is=utils::in_bstream(R"(G:\Games\BloodRayne 2\tests\_07\2.~XERX.TEX)");
    auto tex = tex_file::sern_read(is);
    sern2::write(js, tex);*/


    sern2::write(js, std::make_tuple(1, std::array<person, 1>{}, "2", std::array<std::array<int, 3>, 3>{ std::array<int, 3>{} }, std::unordered_map<std::string, int>{ {"peter", 8888}, { "anna", 7777 } }, std::vector{ 1,2,3 }, std::make_tuple(std::make_tuple(1, 2, 3))));
    //std::cout << sern2::impl_details_wr::ostream_writeable<decltype(js), std::span<const char, 15>>;
   // js.write_type(std::span<const char, 15>{"asd"});


    //sern2::write(js, 3);
    //sern2::write<int&&>(js, 3);
    //sern2::write(js, "asd");
    //sern2::write(js, std::string_view{ "asd" });
    //sern2::write(js, std::string{ "asd" });

    //auto q = std::vector{ std::vector{ "asd","q","we"},std::vector{"asd2","q2","we2"},std::vector{"asd3","q3","we3"} };
    //sern2::write(js, std::unordered_map<std::string, int>{ {"peter", 8888}, { "anna", 7777 } });
    //sern2::write(js, std::vector{ std::vector{ "asd","q","we"},std::vector{"asd2","q2","we2"},std::vector{"asd3","q3","we3"}});

    //sern2::write(js, true);
    //std::copy(js2.writer.data.begin(), js2.writer.data.end(), std::ostreambuf_iterator<char>(std::cout));
    std::cout << js.writer.data;
    return 0;



    // double d = 3.14;
    // os.write(reinterpret_cast<char*>(&d), sizeof d); // binary output
    // os<< 123 << "abc" << '\n';
    // os.close();
    agg ag{ 0,0,0 };// << field
    const auto& ag2 = ag;
    int v = 0;
    auto ar2 = std::array{ 1,2,3 };

    auto tup1 = std::make_tuple(1, 2);
    const auto& tup2 = tup1;
    std::cout << std_ext::tuple_like<decltype(tup2)>;
    //std_ext::tp<decltype(tup2)>();

    //std::cout << std_ext::is_brace_constructible_v2<std::array<int,3>><<"LE";
    v = std_ext::any_apply2([&](auto&&... args)->decltype(auto) { return ((proc(std::forward<decltype(args)>(args))) + ...); }, ag);
    std::cout << ag.c;


    //return 0;
    //TODO работает ли запись многомерных массивов?
    auto sis = utils::out_bstream<eofstream>(R"(G:\Games\BloodRayne 2\tests\_07\BFM\~~2.bin)");
    sern2::testsr::test01();
    //!sern2::write(sis, "hello c string");
    //!sern2::many_write(sis, 1, "2", '3', 4.0);
    //!int car[3] = { 1,2,3 };
    //!sern2::write(sis, car);
    //!int* darr = new int[3] {1, 2, 3};
    //!sern2::write(sis, darr, 3);

    sern2::write(sis, std::map<int, std::map<std::string, std::vector<int>>>{ {1, std::map<std::string, std::vector<int>>{{ "a", std::vector<int>{1, 2, 3} }}}, { 2,std::map<std::string, std::vector<int>>{{ "a",std::vector<int>{1,2,3} } } }});
    sern2::write(sis, new int{ 1 });


    sern2::write(sis, std::variant<int, double>{1.0});
    sern2::write(sis, std::string{ "asd" } + std::string{ "qqq" });
    sern2::write(sis, s);



    auto vec = std::vector{ 'a','b','c' };

    qwe(sis, std::vector{ 1,2,3 });
    sern2::write(sis, std::vector{ 'a','b','c' });
    sern2::write(sis, std::vector{ 1,2,3 });
    sern2::write(sis, wr1{});
    sern2::write(sis, wr2{ 0 });

    const char d[20] = { 'a','b' };
    std::cout << "!!";



   sern2::write(sis, agg2{});

    //auto w=sern::wwriter{ sis, std::endian::native};

    sern2::write(sis, d);



    int dxd[2][3] = { {1,2,3}, {4,5,6} };
    sern2::write(sis, dxd);
    sern2::write(sis, std::array{ std::vector{ 'a','b','c' },std::vector{ 'a','b','c' },std::vector{ 'a','b','c' } });


    enum MyEnum { VALUE1, VALUE2 };
    sern2::write<const MyEnum&>(sis, VALUE1);
    sern2::write(sis, 1.0);

    //w.many_write(1,2,3,4,6,7);
    //G:\Games\BloodRayne 2\tests\_06_BFM_5_YEARS\rayne_dress.tex
    //G:\Games\BloodRayne 2\tests\_07\2.~XERX.TEX
    using namespace br2proj::tex;
    /*
    auto is=utils::in_bstream(R"(G:\Games\BloodRayne 2\tests\_07\2.~XERX.TEX)");
    auto tex = tex_file::sern_read(is);
    conv_tex{}.to_tiff(tex, R"(G:\Games\BloodRayne 2\tests\_07\~~.TIF)");

    auto os = utils::out_bstream(R"(G:\Games\BloodRayne 2\tests\_06_BFM_5_YEARS\~~.bin)");
    sern::write(os, tex);*/

    /*auto is = utils::in_bstream(R"(G:\Games\BloodRayne 2\tests\_07\BFM\RAYNE.BFM)");
    auto tex = bfm_file::sern_read(is);

    auto os = utils::out_bstream(R"(G:\Games\BloodRayne 2\tests\_07\BFM\~~.bin)");
    sern::wwriter{ os }.write(tex);*/




    //conv_tex{}.to_tiff(is, R"(G:\Games\BloodRayne 2\tests\_07\~~.TIF)");
    


    /*TIFFSetField(image, TIFFTAG_IMAGEWIDTH, 1);
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, 1);
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 32);
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    TIFFSetField(image, TIFFTAG_STRIPBYTECOUNTS, 3);//?
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 3);//?
    TIFFSetField(image, TIFFTAG_STRIPBYTECOUNTS, 3);//?
    TIFFSetField(image, TIFFTAG_XRESOLUTION, 3);//?
    TIFFSetField(image, TIFFTAG_YRESOLUTION, 3);//?
    TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, 3);//?*/


  




    /*Assimp::Importer imp;
    auto scene = imp.ReadFile("G:\\Games\\BloodRayne 2\\tests\\_06_BFM_5_YEARS\\out\\human.dae", 0);
    Assimp::Exporter exporter;
    exporter.Export(scene, "assxml", R"(G:\Games\BloodRayne 2\tests\_06_BFM_5_YEARS\out\!BRQ2.dae)");
    return 0;*/
    //auto path = fs::path(R"(G:\Games\BloodRayne 2\tests\_04_ReaderBFM\All\input\ST_LUNGS.BFM)");
    //auto path = fs::path(R"(G:\Games\BloodRayne 2\tests\_04_ReaderBFM\All\input\RAYNE_DRESS.BFM)");

    //G:\Games\BloodRayne 2\tests\_03_ReaderSMB\~RAYNE.SMB
    //G:\Games\BloodRayne 2\tests\_01_ReaderPOD3\W32MODEL\MODELS\VEHICLE_CAMARO.SMB

    /*using namespace br2proj::smb;//VEHICLE_CAMARO.SMB
    std::ifstream sif(R"(G:\Games\BloodRayne 2\tests\_07\0_0\~RAYNE_DRESS.SMB)", std::ios_base::binary);
    smb_file smb = sern::reader<smb_file>::read(sif);
    auto q = smb;

    auto pit = fs::recursive_directory_iterator(R"(G:\Games\_INPUT\BR2)");
    std::ifstream bif(R"(G:\Games\_INPUT\BR2\W32MODEL.pod)", std::ios_base::binary);
    
   // visit_pods(pit, fs::recursive_directory_iterator{}, ".tex", [](std::istream& is) {sern::reader::read<tex::tex_file>(is);});
    
    
    
    auto path = fs::path(R"(G:\Games\BloodRayne 2\tests\_06_BFM_5_YEARS\RAYNE.BFM)");
    auto imodel = bfm_model::import(path);
    auto emodel = prepare_to_export(imodel);
    aiScene*  scene=build_scene(emodel);
    Assimp::Exporter exporter;
    if (exporter.Export(scene, "collada", R"(G:\Games\BloodRayne 2\tests\_06_BFM_5_YEARS\out\RAYNE_DRESS.dae)") != AI_SUCCESS, aiProcess_PopulateArmatureData)
        std::cout << exporter.GetErrorString();*/
    
    //assxml collada
    //    for (int i = 0;i < exporter.GetExportFormatCount();++i) {
    //        std::cout << exporter.GetExportFormatDescription(i)->fileExtension<<'\n';}

    /*Assimp::Importer imp;
    auto scene = imp.ReadFile("G:\\Games\\BloodRayne 2\\tests\\_06_BFM_5_YEARS\\ex.dae", 0);
    //aiProcess_CalcTangentSpace |aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
    
    Assimp::Exporter exporter;
    exporter.Export(scene, "assxml", R"(G:\Games\BloodRayne 2\tests\_06_BFM_5_YEARS\out\!BRQ.dae)");*/

    return 0;
}
//auto path = fs::path(R"(G:\Games\BloodRayne 2\tests\_06_BFM_5_YEARS\RAYNE.BFM)");
// GOOD FOR TEST auto path = fs::path(R"(G:\Games\BloodRayne 2\tests\_04_ReaderBFM\All\input\ADZII_CAGE.BFM)"); 
// GOOD FOR TEST auto path = fs::path(R"(G:\Games\BloodRayne 2\tests\_04_ReaderBFM\All\input\ST_LUNGS.BFM)");
//auto path = fs::path(R"(G:\Games\BloodRayne 2\tests\_04_ReaderBFM\All\input\UA_WEST_STRAND.BFM)");
//BONES ROOT ODD  auto path = fs::path(R"(G:\Games\BloodRayne 2\tests\_04_ReaderBFM\All\input\HEART.BFM)");

/*float ax = -1000, ay = -1000, az = -1000;
float ix = 1000, iy = 1000, iz = 1000;
for (auto& pt : imesh.points) {
    if (pt.pos.x > ax) ax = pt.pos.x;
    if (pt.pos.y > ay) ay = pt.pos.y;
    if (pt.pos.z > az) az = pt.pos.z;

    if (pt.pos.x < ix) ix = pt.pos.x;
    if (pt.pos.y < iy) iy = pt.pos.y;
    if (pt.pos.z < iz) iz = pt.pos.z;
}

std::cout << "inp\n" << imesh.box.start.x << ' ' << imesh.box.start.y << ' ' << imesh.box.start.z << "\n";
std::cout << imesh.box.end.x << ' ' << imesh.box.end.y << ' ' << imesh.box.end.z << "\n\n";

std::cout << "calc\n" << ix << ' ' << iy << ' ' << iz << '\n';
std::cout << ax << ' ' << ay << ' ' << az << "\n\n";*/

/*Assimp::Importer imp;

auto a = imp.ReadFile("G:\\Games\\BloodRayne 2\\tests\\_06_BFM_5_YEARS\\ex.dae", aiProcess_CalcTangentSpace |
    aiProcess_Triangulate |
    aiProcess_JoinIdenticalVertices |
    aiProcess_SortByPType);
int b = 0;*/
// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"
/*for (int i = 0;i < model.skeleton.size();++i) {
    std::cout << model.skeleton[i].name << "[";
    for (int j = 0;j < model.skeleton.size();++j) {
        if (model.skeleton[j].parent_ind == i) {
            std::cout << model.skeleton[j].name << "\n";
            //  std::cout<<
        }
    }
    std::cout << "]\n";
}*/
/*

        aiBone** bones = std_ext::transform_new(imesh.infl_bones, [&](const auto& p) {
            auto bn = new aiBone();
            bn->mWeights = std_ext::transform_new(p, [&](const int vert) {
                return aiVertexWeight(vert, js.wieght);
                aiVertexWeight* w = new aiVertexWeight[imesh.points[vert].joints.size()];
                std::transform()
                std_ext::transform_new(imesh.points[vert].joints, [&](const int vert) {});

                });
            bn->mNumWeights = p.second.size();
            matrix4x4 m{ model.cskel[p.first] };
            m.Inverse();
            bn->mOffsetMatrix = m;
            bn->mName = model.skeleton[p.first].name;
            return bn;
        });
*/

//return []<class WW, class TQ, TQ... ints>(const WW& mems, std::integer_sequence<TQ, ints...>) { return std::array{ get_name<&std::get<ints>(mems)>()... }; }  (members, std::make_integer_sequence<int, std::tuple_size_v<decltype(members)> >{});

//std::apply([](const auto&... flds) {return  }, members);
//return std::array{a1,a2};
//return std::apply([](auto&&... flds) {return std::array{ get_name<&flds>()... }; }, std::move(members));

// 

 //return { extract(get_name<&std::get<0>(members)>()), extract(get_name<&std::get<1>(members)>()) };
 //return get_name<&std::get<0>(members)>();


        /*const auto la = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return toy<res[Is]...>{}.data;
        };

        return la(std::make_index_sequence<3>{});*/
        //const auto la = []<char... Is>(std::integer_sequence<char, Is...>) { return toy<Is...>{}.data; }

      /*  if constexpr (std::is_signed_v<decltype(num)> && num < 0) {
            return make_toy<'-'>(int_to_str2i<-num>());
        }
        else
            return make_toy(int_to_str2i<num>());*/

/*template<std::integral auto num>
struct possi_formatter {
    constexpr static auto base = 10;
    static_assert(std::is_unsigned_v<decltype(num)> || (std::is_signed_v<decltype(num)> && num >= 0));
    template <char... Is>
    constexpr static auto format() {
        return get_str<'t', 'u', 'p', Is...>::data;
    }
};*/