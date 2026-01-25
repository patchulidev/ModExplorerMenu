#pragma once

#include <nlohmann/json.hpp>

namespace nlohmann {
    template <>
    struct adl_serializer<ImVec4> {
        static void to_json(json& j, const ImVec4& vec) {
            j = json::array({ vec.x, vec.y, vec.z, vec.w });
        }

        static void from_json(const json& j, ImVec4& vec) {
            if (j.is_array() && j.size() == 4) {
                vec.x = j[0].get<float>();
                vec. y = j[1].get<float>();
                vec.z = j[2].get<float>();
                vec.w = j[3].get<float>();
            } else {
                vec = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
    };

    template<>
    struct adl_serializer<ImVec2> {
        static void to_json(json& j, const ImVec2& vec) {
            j = json::array({ vec.x, vec.y });
        }

        static void from_json(const json& j, ImVec2& vec) {
            if (j.is_array() && j.size() == 2) {
                vec.x = j[0].get<float>();
                vec.y = j[1].get<float>();
            } else {
                vec = ImVec2(0.0f, 0.0f);
            }
        }
    };
}
