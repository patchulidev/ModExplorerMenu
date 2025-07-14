#pragma once

#include "d3d11.h"
#include "extern/IconsLucide.h"
#include "include/L/Language.h"

// clang-format off

namespace Modex
{
	// D3D11 Implementation Sourced from dTry's Wheeler and Llama's Tiny Hud. Adapted to NG.

	class GraphicManager
	{
	private:
		const static inline std::filesystem::path image_path = "Data/Interface/Modex/images";
		const static inline std::filesystem::path imgui_path = "Data/Interface/ImGuiIcons/Icons";

	public:
		using D3D11View = ID3D11ShaderResourceView*;

		struct Image
		{
			D3D11View 					texture = nullptr;
			int32_t 					width = 0;
			int32_t 					height = 0;
		};

		static inline std::map<std::string, GraphicManager::Image> image_library;
		static inline std::map<std::string, GraphicManager::Image> imgui_library;

		static void 		Init();
		static void 		LoadImagesFromFilepath(const std::filesystem::path& a_path, std::map<std::string, Image>& out_struct);
		static bool 		GetD3D11Texture(const char* filename, ID3D11ShaderResourceView** out_srv, int& out_width, int& out_height);

		[[nodiscard]] static Image GetImage(std::string a_name);
		[[nodiscard]] static std::string GetImageName(Image a_image);
		[[nodiscard]] static std::map<std::string, Image> GetListOfImages();
	};
}