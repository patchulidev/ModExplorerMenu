#pragma once

namespace RE
{
	class TESBoundObject;
}

namespace REX::W32
{
	struct ID3D11Texture2D;
	struct ID3D11ShaderResourceView;
}

namespace Modex
{
	class Item3DPreview
	{
	public:
		static constexpr unsigned int kTexSize = 2048;

		// Margin applied to prevent clipping capture bounds
		static constexpr float kSafetyMargin = 1.5f;

		static Item3DPreview* GetSingleton();

		void Begin();
		void End();
		void Render();

		void Request(RE::TESBoundObject* a_item, ImVec2 a_screenPos, ImVec2 a_screenSize,
		             float a_modelScale = -1.0f, float a_offsetX = 0.0f, float a_offsetY = 0.0f);
		void* GetSRV() const;

		ImVec2 GetCapturedSize() const { return m_lastCapturedSize; }
		void GetDisplayUV(ImVec2& a_uv0, ImVec2& a_uv1) const;
		static bool HasValidModel(RE::TESBoundObject* a_item);

	private:
		Item3DPreview() = default;

		bool Initialize();
		void Shutdown();

		bool                m_initialized = false;
		bool                m_running     = false;
		bool                m_requested   = false;
		RE::TESBoundObject* m_current     = nullptr;

		ImVec2 m_capturePos        = ImVec2(0.0f, 0.0f);
		ImVec2 m_captureSize       = ImVec2(0.0f, 0.0f);   // full rect including safety margin
		ImVec2 m_innerSize         = ImVec2(0.0f, 0.0f);   // displayed portion (no margin) — drives UV crop
		ImVec2 m_lastCapturedSize  = ImVec2(0.0f, 0.0f);   // actual captured pixels after backbuffer clamp
		ImVec2 m_modelInTexture    = ImVec2(0.0f, 0.0f);   // model's centre position in captured-texture pixels

		// Per-request offset overrides applied at Render; falls back to theme when false.
		float  m_overrideOffsetX   = 0.0f;
		float  m_overrideOffsetY   = 0.0f;
		bool   m_hasOverrideOffset = false;

		REX::W32::ID3D11Texture2D*          m_dstTex     = nullptr;
		REX::W32::ID3D11ShaderResourceView* m_dstSRV     = nullptr;
		REX::W32::ID3D11Texture2D*          m_scratchTex = nullptr;
	};
}
