#pragma once

#include <iostream>
#include <array>
#include <vector>
#include <fstream>
#include <type_traits>
#include <algorithm>
#include <concepts>
#include <filesystem>
#include <cstddef>
#include <string>
#include <string_view>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>
#include <variant>
#include <memory>
#include <unordered_map>
#include <bit>
#include <ranges>
#include <source_location>
#include <any>
#include <sstream>
#include <typeinfo>
#include <optional>
#include <format>


#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/vector3.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/vector3.inl>
#include <assimp/quaternion.h>

#include "std_ext.hpp"
#include "tiny_refl.hpp"
#include "base_types.hpp"
//TODO перепроверить заголовки
//TODO перепроверить заголовки assimp

namespace fs = std::filesystem;
namespace ranges = std::ranges;