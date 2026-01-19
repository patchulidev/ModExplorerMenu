#pragma once

namespace Modex
{
	// D3D11 Implementation Sourced from dTry's Wheeler and Llama's Tiny Hud. Adapted to NG.
	// This plugin would absolutely not be possible without dMenu, OAR, Tiny HUD, and so many
	// more public repositories

	class GraphicManager
	{
	private:
		const static inline std::filesystem::path IMAGE_PATH = std::filesystem::path("data") / "interface" / "modex" / "images";
		const static inline std::filesystem::path IMGUI_PATH = std::filesystem::path("data") / "interface" / "imguiicons" / "icons";

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
