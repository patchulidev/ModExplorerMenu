#pragma once

#include "pch.h"
#include "ui/core/UIManager.h"
#include "ui/components/UIWindow.h"

// Source: ersh1
// https://github.com/ersh1/OpenAnimationReplacer/blob/main/src/UI/UIWelcomeBanner.h

namespace Modex
{
    class UIBanner : public UIWindow
    {
    private:
        float   lingerTime = 8.0f;
        bool    firstFrame = false;
    public:
        inline static constexpr float DISPLAY_TIME = 8.0f;

        void Display();
        bool ShouldDisplay() const;
        void Draw();
    
    };
}
