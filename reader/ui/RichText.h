#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "../Macros.h"

NS_CCR_BEGIN

class RichText : public cocos2d::ui::RichText
{
private:
    float m_MaxWidth;

public:
	static RichText* create();

    inline void setMaxWidth(float maxWidth) { m_MaxWidth = maxWidth; }
    void setString(const std::string& text);
};

NS_CCR_END
