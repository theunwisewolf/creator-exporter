#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "../CreatorReader_generated.h"
#include "../Macros.h"

NS_CCR_BEGIN

class Button : public cocos2d::ui::Button
{
private:
	std::function<void()> m_ListenerArg0 = nullptr;
	std::function<void(creator::Button*)> m_ListenerArg1 = nullptr;

	int m_PositionDiff;

public:
	enum TransitionType
	{
		NONE,
		COLOR,
		SPRITE,
		SCALE
	};

	Button();
	~Button();

	/**
	 * Create and return a empty Button instance pointer.
	 */
	static Button* create();

	/**
	 * Create a button with custom textures.
	 * 
	 * @param normalImage 			Normal state texture name.
	 * @param selectedImage  		Selected state texture name.
	 * @param disableImage 			Disabled state texture name.
	 * @param texType    			@see `TextureResType`
	 * 
	 * @return a Button instance.
	 */
	static Button* create(const std::string& normalImage,
		const std::string& selectedImage = "",
		const std::string& disableImage = "",
		cocos2d::ui::Widget::TextureResType texType = cocos2d::ui::Widget::TextureResType::LOCAL);

	virtual void addChild(cocos2d::Node* child) override;

	virtual void onPressStateChangedToCancel();
	virtual void onPressStateChangedToNormal() override;
	virtual void onPressStateChangedToPressed() override;
	virtual void onPressStateChangedToDisabled() override;

	inline void setActionDuration(float duration) { _actionDuration = duration; }
	inline float getActionDuration() const { return _actionDuration; }

	void setTransitionType(TransitionType transitionType);
	inline TransitionType getTransitionType() const { return _transitionType; }

	inline void setNormalColor(cocos2d::Color4B& color) { _normalColor = color; }
	inline cocos2d::Color4B getNormalColor() const { return _normalColor; }

	inline void setPressedColor(cocos2d::Color4B& color) { _pressedColor = color; }
	inline cocos2d::Color4B getPressedColor() const { return _pressedColor; }

	inline void setDisabledColor(cocos2d::Color4B& color) { _disabledColor = color; }
	inline cocos2d::Color4B getDisabledColor() const { return _disabledColor; }

	inline void enableAutoGrayEffect(bool enableAutoGrayEffect) { _enableAutoGrayEffect = enableAutoGrayEffect; }
	inline int isAutoGrayEffect() const { return _enableAutoGrayEffect; }

	virtual void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* unusedEvent) override;
	virtual void cancelUpEvent() override;

	inline void setNodeBgName(const std::string& name) { _nodeBgName = name; }
	inline std::string getNodeBgName() const { return _nodeBgName; }

	inline float getOriginalScale() const { return _originalScale; }
	inline void setOriginalScale(float scale) { _originalScale = scale; }

	void onClickListener(std::function<void(creator::Button* button)>&& listener);
	void onClick(std::function<void()>&& listener);

	// This makes touches on the button only fire click events if the touch area is not moved (or moved below a minimum threshold)
	void disableClicksOnScroll();

public:
	float _actionDuration;
	TransitionType _transitionType;

	cocos2d::Color4B _normalColor;
	cocos2d::Color4B _pressedColor;
	cocos2d::Color4B _disabledColor;

	std::string _nodeBgName;
	bool _enableAutoGrayEffect;
	float _originalScale;
};

NS_CCR_END
