#include "RichText.h"
#include "RichtextStringVisitor.h"

NS_CCR_BEGIN

RichText* RichText::create()
{
	RichText* widget = new (std::nothrow) RichText();
	if (widget && widget->init())
	{
		widget->autorelease();
		return widget;
	}

	CC_SAFE_DELETE(widget);
	return nullptr;
}

void RichText::setString(const std::string& str)
{
	_richElements.clear();
    std::string text = std::string("<font>").append(str).append("</font>");

	RichtextStringVisitor visitor;
	cocos2d::SAXParser parser;
	parser.setDelegator(&visitor);
	parser.parseIntrusive(const_cast<char*>(text.c_str()), text.length());

	this->initWithXML(visitor.getOutput());

	const auto& rawString = visitor.getRawString();
    if (m_MaxWidth > 0)
	{
		const auto& contentSize = this->getContentSize();
		this->setContentSize(cocos2d::Size(m_MaxWidth, contentSize.height));
	}
    else
    {
        auto maxFontSize = visitor.getMaxFontSize();
        int finalFontSize = std::max(static_cast<float>(maxFontSize), _defaults[KEY_FONT_SIZE].asFloat());
        auto label = cocos2d::Label::createWithSystemFont(rawString, _defaults[KEY_FONT_FACE].asString(), finalFontSize);

        auto realContentSize = label->getContentSize();
        auto finalWidth = std::max(realContentSize.width, _contentSize.width);
        this->setContentSize(cocos2d::Size(finalWidth, _contentSize.height));
    }

    this->ignoreContentAdaptWithSize(false);
}

NS_CCR_END
