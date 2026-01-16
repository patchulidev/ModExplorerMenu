#define STB_IMAGE_IMPLEMENTATION

#include "Graphic.h"
#include "external/stb_image.h"

namespace Modex
{
	bool GraphicManager::GetD3D11Texture(const char* filename, ID3D11ShaderResourceView** out_srv, int& out_width,
		int& out_height)
	{
		const auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
        ASSERT_MSG(renderer == nullptr, "Failed to get BSGraphics Renderer!");

		const auto& renderData = renderer->GetRuntimeData();
		ASSERT_MSG(renderData.forwarder == nullptr, "Failed to get D3D11 Device from Renderer!");

        auto *device    = reinterpret_cast<ID3D11Device *>(renderData.forwarder);

		if (!device) {
			PrettyLog::Error("D3D11 Device is null, cannot create texture for image: {}", filename);
			return false;
		}

		auto* render_manager = RE::BSGraphics::Renderer::GetSingleton();
		if (!render_manager) {
			PrettyLog::Error("Cannot find RE::BSGraphics::Renderer singleton. Uh oh!");
			return false;
		}

		// Load from disk into a buffer
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);

		if (image_data == NULL) {
			PrettyLog::Error("Failed to load image: {}", filename);
			return false;
		}

		// Create texture
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.Width = image_width;
		desc.Height = image_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		ID3D11Texture2D* p_texture = nullptr;
		D3D11_SUBRESOURCE_DATA sub_resource;
		sub_resource.pSysMem = image_data;
		sub_resource.SysMemPitch = desc.Width * 4;
		sub_resource.SysMemSlicePitch = 0;

		device->CreateTexture2D(&desc, &sub_resource, &p_texture);

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof srv_desc);
		srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = desc.MipLevels;
		srv_desc.Texture2D.MostDetailedMip = 0;

		device->CreateShaderResourceView(p_texture, &srv_desc, out_srv);
		p_texture->Release();

		out_width = image_width;
		out_height = image_height;
		stbi_image_free(image_data);

		return true;
	}

	
	GraphicManager::Image GraphicManager::GetImage(std::string a_name)
	{
		auto found = image_library.find(a_name);
		if (found != image_library.end()) {
			return found->second;
		}
		return Image();
	}

	std::string GraphicManager::GetImageName(Image a_image)
	{
		for (const auto& [key, value] : image_library) {
			if (value.texture == a_image.texture) {
				return key;
			}
		}

		return "None";
	}

	std::map<std::string, GraphicManager::Image> GraphicManager::GetListOfImages()
	{
		return GraphicManager::image_library;
	}

	void GraphicManager::LoadImagesFromFilepath(const std::filesystem::path& a_path, std::map<std::string, Image>& out_struct)
	{
		ASSERT_MSG(!std::filesystem::exists(a_path), "Could not locate Image directory, expected at: {}", a_path.string());

		for (const auto& entry : std::filesystem::directory_iterator(a_path)) {
			if (entry.path().extension() != ".png") {
				continue;
			}

        	auto index = entry.path().stem().string();

			bool success = GraphicManager::GetD3D11Texture(entry.path().string().c_str(), 
				&out_struct[index].texture,
				out_struct[index].width, 
				out_struct[index].height);

			if (!success) {
				PrettyLog::Error("Failed to get D3D11 texture from image: {}", entry.path().string());
			}
		}

		PrettyLog::Debug("Loaded {} images from \"{}\"", out_struct.size(), a_path.string());
	}

	void GraphicManager::Init()
	{
		image_library["None"] = Image();
		GraphicManager::LoadImagesFromFilepath(GraphicManager::IMAGE_PATH, GraphicManager::image_library);

		if (std::filesystem::exists(GraphicManager::IMGUI_PATH)) {
			GraphicManager::LoadImagesFromFilepath(GraphicManager::IMGUI_PATH, GraphicManager::imgui_library);
		} else {
			PrettyLog::Info("ImGui Icon Library mod/directory not found. Skipping Custom ImGui Library loading.");
		}
	}
}
