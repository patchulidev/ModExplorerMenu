#pragma once
#include <PCH.h>

// Source: ersh1
// https://github.com/ersh1/OpenAnimationReplacer/blob/main/src/UI/UIWelcomeBanner.h

namespace Modex
{
    class UIBanner
    {
    public:
    	static inline UIBanner* GetSingleton()
		{
			static UIBanner singleton;
			return std::addressof(singleton);
		}

        inline static constexpr float DISPLAY_TIME = 8.0f;

        void Display();
        bool ShouldDisplay() const;
        void Draw();
    
    private:
        float   lingerTime = 0.0f;
        bool    firstFrame = false;
    };
}