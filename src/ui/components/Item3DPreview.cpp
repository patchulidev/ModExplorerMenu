#include "Item3DPreview.h"

#include "RE/B/BSShaderRenderTargets.h"
#include "RE/I/Inventory3DManager.h"
#include "RE/N/NiAVObject.h"
#include "RE/R/Renderer.h"
#include "RE/T/TESBoundObject.h"
#include "RE/T/TESModel.h"
#include "RE/U/UI3DSceneManager.h"
#include "config/ThemeConfig.h"
#include "ui/core/UIManager.h"

#include <d3d11_1.h>

namespace Modex
{
	Item3DPreview* Item3DPreview::GetSingleton()
	{
		static Item3DPreview singleton;
		return std::addressof(singleton);
	}

	// TODO: Sometime in the future need to validate this.
	bool Item3DPreview::HasValidModel(RE::TESBoundObject* a_item)
	{
		return a_item;
	}

	void Item3DPreview::GetDisplayUV(ImVec2& a_uv0, ImVec2& a_uv1) const
	{
		const float kTex = static_cast<float>(kTexSize);

		const float effectiveWidth = (m_innerSize.x > 0.0f) ? (std::min)(m_innerSize.x, m_lastCapturedSize.x) : m_lastCapturedSize.x;
		const float effectiveHeight = (m_innerSize.y > 0.0f) ? (std::min)(m_innerSize.y, m_lastCapturedSize.y) : m_lastCapturedSize.y;
		const float startX = m_modelInTexture.x - effectiveWidth * 0.5f;
		const float startY = m_modelInTexture.y - effectiveHeight * 0.5f;

		a_uv0 = ImVec2(startX / kTex,             startY / kTex);
		a_uv1 = ImVec2((startX + effectiveWidth) / kTex,   (startY + effectiveHeight) / kTex);
	}

	void* Item3DPreview::GetSRV() const
	{
		return reinterpret_cast<void*>(m_dstSRV);
	}

	bool Item3DPreview::Initialize()
	{
		if (m_initialized) return true;

		auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
		if (!renderer) return false;
		const auto& data = renderer->GetRuntimeData();
		auto* device = reinterpret_cast<ID3D11Device*>(data.forwarder);
		if (!device) return false;

		// Source is the on-screen swap chain. Get the underlying texture.

		auto* rtv = reinterpret_cast<ID3D11RenderTargetView*>(
			data.renderWindows[0].renderView);
		if (!rtv) return false;

		ID3D11Resource* srcRes = nullptr;
		rtv->GetResource(&srcRes);
		if (!srcRes) return false;

		ID3D11Texture2D* srcTex = nullptr;
		srcRes->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&srcTex));
		srcRes->Release();
		if (!srcTex) return false;

		D3D11_TEXTURE2D_DESC srcDesc = {};
		srcTex->GetDesc(&srcDesc);
		srcTex->Release();

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width            = kTexSize;
		desc.Height           = kTexSize;
		desc.MipLevels        = 1;
		desc.ArraySize        = 1;
		desc.Format           = srcDesc.Format;
		desc.SampleDesc.Count = 1;
		desc.Usage            = D3D11_USAGE_DEFAULT;
		desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

		ID3D11Texture2D* tex = nullptr;
		if (FAILED(device->CreateTexture2D(&desc, nullptr, &tex))) return false;
		m_dstTex = reinterpret_cast<REX::W32::ID3D11Texture2D*>(tex);

		ID3D11ShaderResourceView* srv = nullptr;
		if (FAILED(device->CreateShaderResourceView(tex, nullptr, &srv))) { Shutdown(); return false; }
		m_dstSRV = reinterpret_cast<REX::W32::ID3D11ShaderResourceView*>(srv);

		// Scratch holds the original backbuffer pixels we overwrite each
		// frame. Same format/size as the destination — only difference is
		// it never needs to be sampled, so no SRV.

		ID3D11Texture2D* scratch = nullptr;
		if (FAILED(device->CreateTexture2D(&desc, nullptr, &scratch))) { Shutdown(); return false; }
		m_scratchTex = reinterpret_cast<REX::W32::ID3D11Texture2D*>(scratch);

		m_initialized = true;
		return true;
	}

	void Item3DPreview::Shutdown()
	{
		auto release = [](auto*& p) {
			if (p) {
				reinterpret_cast<IUnknown*>(p)->Release();
				p = nullptr;
			}
		};

		release(m_dstSRV);
		release(m_dstTex);
		release(m_scratchTex);
		m_initialized = false;
	}

	void Item3DPreview::Begin()
	{
		if (m_running) return;

		if (auto* mgr = RE::Inventory3DManager::GetSingleton()) {
			mgr->Begin3D(RE::INTERFACE_LIGHT_SCHEME::kInventory);
			m_running = true;
		}
	}

	void Item3DPreview::End()
	{
		if (auto* mgr = RE::Inventory3DManager::GetSingleton(); mgr && m_running) {
			mgr->UnloadInventoryItem();
			mgr->End3D();
		}

		m_running   = false;
		m_requested = false;
		m_current   = nullptr;
		Shutdown();
	}

	void Item3DPreview::Request(RE::TESBoundObject* a_item, ImVec2 a_screenPos, ImVec2 a_screenSize)
	{
		if (!m_running || a_item == nullptr) return;

		if (a_item != m_current) {
			if (auto* mgr = RE::Inventory3DManager::GetSingleton()) {
				mgr->UnloadInventoryItem();
				mgr->LoadInventoryItem(a_item, nullptr);
			}
			m_current = a_item;
		}

		const float modelScale = ThemeConfig::GetWidgetStyle().itemPreview.previewModelScale;
		const float expand     = (modelScale > 0.0f) ? (1.0f / modelScale) : 1.0f;

		m_capturePos  = a_screenPos;
		m_innerSize   = ImVec2(a_screenSize.x * expand,                 a_screenSize.y * expand);
		m_captureSize = ImVec2(m_innerSize.x * kSafetyMargin, m_innerSize.y * kSafetyMargin);
		m_requested   = true;
	}

	void Item3DPreview::Render()
	{
		const bool req = m_requested;
		m_requested = false;
		if (!m_running || !req) return;

		// Don't paint backbuffer while fading out menu;
		if (!UIManager::GetSingleton()->IsMenuFullyOpen()) return;
		if (!m_initialized && !Initialize()) return;

		auto* inv = RE::Inventory3DManager::GetSingleton();
		if (!inv) return;
		
		// See: https://github.com/patchulidev/ModExplorerMenu/issues/48
		// Capture backbuffer in place without translation to avoid issues;

		{
			auto* scn0 = RE::UI3DSceneManager::GetSingleton();
			auto& runtime0 = inv->GetRuntimeData();
			if (scn0 && !runtime0.loadedModels.empty()) {
				auto* spModel0 = runtime0.loadedModels.back().spModel.get();
				if (spModel0 && spModel0->worldBound.radius > 0.0f) {
					const auto& vf = scn0->viewFrustum;
					const auto& t  = spModel0->local.translate;
					const float world_minx  = -vf.fLeft   * t.y;
					const float world_minz  = -vf.fBottom * t.y;
					const float world_width  = -vf.fRight * t.y - world_minx;
					const float world_height = -vf.fTop   * t.y - world_minz;

					const auto sz = RE::BSGraphics::Renderer::GetScreenSize();
					if (sz.width > 0 && sz.height > 0) {
						const float ratio_x = world_width  / static_cast<float>(sz.width);
						const float ratio_y = world_height / static_cast<float>(sz.height);
						if (ratio_x != 0.0f && ratio_y != 0.0f) {
							const auto& c = spModel0->worldBound.center;
							const float model_sx = -(c.x + world_minx) / ratio_x;
							const float model_sy = -(c.z + world_minz) / ratio_y;
							m_capturePos = ImVec2(
								model_sx - m_captureSize.x * 0.5f,
								model_sy - m_captureSize.y * 0.5f);
						}
					}
				}
			}
		}

		// Resolve all the D3D11 resources we need for the pipeline below
		// (save → clear → render → capture → restore) is the magic sauce.
		auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
		if (!renderer) return;
		const auto& data = renderer->GetRuntimeData();
		auto* context    = reinterpret_cast<ID3D11DeviceContext*>(data.context);
		auto* dstTex     = reinterpret_cast<ID3D11Texture2D*>(m_dstTex);
		auto* scratchTex = reinterpret_cast<ID3D11Texture2D*>(m_scratchTex);
		auto* rtv        = reinterpret_cast<ID3D11RenderTargetView*>(
			data.renderWindows[0].renderView);
		if (!context || !dstTex || !scratchTex || !rtv) return;

		ID3D11Resource* srcRes = nullptr;
		rtv->GetResource(&srcRes);
		if (!srcRes) return;
		ID3D11Texture2D* srcTex = nullptr;
		srcRes->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&srcTex));
		srcRes->Release();
		if (!srcTex) return;

		// Compute the clamped backbuffer rect once. Save/clear/capture/restore
		// all operate on this single box.
		const auto screenSize = RE::BSGraphics::Renderer::GetScreenSize();
		int left   = static_cast<int>(m_capturePos.x);
		int top    = static_cast<int>(m_capturePos.y);
		int width  = static_cast<int>(m_captureSize.x);
		int height = static_cast<int>(m_captureSize.y);
		if (left < 0) { width += left;  left = 0; }
		if (top  < 0) { height += top;  top  = 0; }
		if (width  <= 0 || height <= 0) { srcTex->Release(); return; }

		const int maxW = static_cast<int>(screenSize.width)  - left;
		const int maxH = static_cast<int>(screenSize.height) - top;
		if (width  > maxW) width  = maxW;
		if (height > maxH) height = maxH;
		if (width  > static_cast<int>(kTexSize)) width  = static_cast<int>(kTexSize);
		if (height > static_cast<int>(kTexSize)) height = static_cast<int>(kTexSize);
		if (width  <= 0 || height <= 0) { srcTex->Release(); return; }

		D3D11_BOX box = {};
		box.left   = static_cast<UINT>(left);
		box.top    = static_cast<UINT>(top);
		box.front  = 0;
		box.right  = static_cast<UINT>(left + width);
		box.bottom = static_cast<UINT>(top + height);
		box.back   = 1;

		// Step 1 (save): copy the current backbuffer rect into our scratch texture
		// texture so we can restore it after the capture. Without this,
		// the FRAME-coloured rect we paint next remains on the backbuffer
		// and is visible through Modex's semi-transparent UI.
		context->CopySubresourceRegion(scratchTex, 0, 0, 0, 0, srcTex, 0, &box);

		// Step 2 (clear): paint a solid background colour into that rect.
		// The engine paints the model on top, so our capture catches
		// model + background. ClearView is a D3D11.1 feature; the cast is
		// the standard way of reaching it from the base context.
		{
			ID3D11DeviceContext1* ctx1 = nullptr;
			if (SUCCEEDED(context->QueryInterface(__uuidof(ID3D11DeviceContext1),
					reinterpret_cast<void**>(&ctx1))) && ctx1) {
				D3D11_RECT rect = { left, top, left + width, top + height };
				const ImVec4 fc = ThemeConfig::GetColor("FRAME");
				const FLOAT  bg[4] = { fc.x, fc.y, fc.z, 1.0f };
				ctx1->ClearView(rtv, bg, &rect, 1);
				ctx1->Release();
			}
		}

		// Step 3 (render): engine paints the model to the backbuffer.
		inv->Render();

		// Step 4 (capture): copy backbuffer rect → top-left of our
		// destination texture. ImGui::Image crops the rest with UVs.
		context->CopySubresourceRegion(dstTex, 0, 0, 0, 0, srcTex, 0, &box);

		// Step 5 (restore): write the saved world pixels back onto the
		// backbuffer. After this, the rect we just operated on contains
		// the original world content again — no visible square behind
		// Modex's semi-transparent UI.
		D3D11_BOX scratchBox = {};
		scratchBox.left   = 0;
		scratchBox.top    = 0;
		scratchBox.front  = 0;
		scratchBox.right  = static_cast<UINT>(width);
		scratchBox.bottom = static_cast<UINT>(height);
		scratchBox.back   = 1;
		context->CopySubresourceRegion(srcTex, 0,
			static_cast<UINT>(left), static_cast<UINT>(top), 0,
			scratchTex, 0, &scratchBox);

		srcTex->Release();

		m_lastCapturedSize = ImVec2(static_cast<float>(width), static_cast<float>(height));

		// Where the model's projected centre lands inside the texture
		const float model_cx = m_capturePos.x + m_captureSize.x * 0.5f;
		const float model_cy = m_capturePos.y + m_captureSize.y * 0.5f;
		m_modelInTexture = ImVec2(model_cx - static_cast<float>(left),
		                          model_cy - static_cast<float>(top));

		const auto& offsets = ThemeConfig::GetWidgetStyle().itemPreview;
		m_modelInTexture.x -= offsets.previewOffsetX;
		m_modelInTexture.y -= offsets.previewOffsetY;
	}
}
