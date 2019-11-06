#include "Button.h"

NS_CCR_BEGIN

Button* Button::create()
{
	creator::Button* widget = new (std::nothrow) creator::Button();
	if (widget && widget->init())
	{
		widget->autorelease();
		return widget;
	}
	CC_SAFE_DELETE(widget);
	return nullptr;
}

Button* Button::create(const std::string& normalImage, const std::string& selectedImage, const std::string& disabledImage, cocos2d::ui::Widget::TextureResType texType)
{
	creator::Button* btn = new (std::nothrow) creator::Button();
	if (btn && btn->init(normalImage, selectedImage, disabledImage, texType))
	{
		btn->autorelease();
		return btn;
	}

	CC_SAFE_DELETE(btn);
	return nullptr;
}

Button::Button() :
	cocos2d::ui::Button::Button()
{
	_actionDuration = 0.1f;
	_transitionType = TransitionType::NONE;
	_enableAutoGrayEffect = false;
	_originalScale = 1.0f;
}

Button::~Button()
{
}

void Button::addChild(Node* child)
{
	if (child->getName() == _nodeBgName && !_nodeBgName.empty())
	{
		auto sprite = dynamic_cast<cocos2d::ui::Scale9Sprite*>(child);
		if (sprite)
		{
			this->removeProtectedChild(_buttonNormalRenderer);
			_buttonNormalRenderer = sprite;
			this->addProtectedChild(_buttonNormalRenderer, -2, -1);

			return;
		}
	}

	cocos2d::ui::Button::addChild(child);
}

void Button::onPressStateChangedToCancel()
{
	_buttonNormalRenderer->setVisible(true);
	_buttonClickedRenderer->setVisible(false);
	_buttonDisabledRenderer->setVisible(false);
	_buttonNormalRenderer->setState(cocos2d::ui::Scale9Sprite::State::NORMAL);

	if (_pressedTextureLoaded)
	{
		if (_transitionType == TransitionType::COLOR)
		{
			this->stopAllActions();
			this->setColor(cocos2d::Color3B(_normalColor.r, _normalColor.b, _normalColor.g));
			this->setOpacity(_normalColor.a);
		}
		else if (_pressedActionEnabled)
		{
			this->stopAllActions();
			this->setScale(_originalScale, _originalScale);
		}
	}
	else if (_transitionType == TransitionType::SCALE)
	{
		this->stopAllActions();
		this->setScale(_originalScale, _originalScale);
	}
	else
	{
		_buttonNormalRenderer->stopAllActions();
		_buttonNormalRenderer->setScale(1.0);

		if (nullptr != _titleRenderer)
		{
			_titleRenderer->stopAllActions();
			_titleRenderer->setScaleX(1.0f);
			_titleRenderer->setScaleY(1.0f);
		}
	}
}

void Button::onPressStateChangedToNormal()
{
	_buttonNormalRenderer->setVisible(true);
	_buttonClickedRenderer->setVisible(false);
	_buttonDisabledRenderer->setVisible(false);
	_buttonNormalRenderer->setState(cocos2d::ui::Scale9Sprite::State::NORMAL);

	if (_pressedTextureLoaded)
	{
		if (_transitionType == TransitionType::COLOR)
		{
			this->stopAllActions();
			auto action = cocos2d::TintTo::create(_actionDuration, cocos2d::Color3B(_normalColor.r, _normalColor.b, _normalColor.g));
			auto fadeAction = cocos2d::FadeTo::create(_actionDuration, _normalColor.a);
			auto spawn = cocos2d::Spawn::create(action, fadeAction, nullptr);

			this->runAction(cocos2d::EaseSineOut::create(spawn));
		}
		else if (_pressedActionEnabled)
		{
			this->stopAllActions();
			this->runAction(cocos2d::EaseSineOut::create(cocos2d::ScaleTo::create(_actionDuration, _originalScale, _originalScale)));
		}
	}
	else if (_transitionType == TransitionType::SCALE)
	{
		this->stopAllActions();
		this->runAction(cocos2d::EaseSineOut::create(cocos2d::ScaleTo::create(_actionDuration, _originalScale, _originalScale)));
	}
	else
	{
		_buttonNormalRenderer->stopAllActions();
		_buttonNormalRenderer->setScale(1.0);

		if (nullptr != _titleRenderer)
		{
			_titleRenderer->stopAllActions();
			_titleRenderer->setScaleX(1.0f);
			_titleRenderer->setScaleY(1.0f);
		}
	}
}

void Button::onPressStateChangedToPressed()
{
	_buttonNormalRenderer->setState(cocos2d::ui::Scale9Sprite::State::NORMAL);

	if (_pressedTextureLoaded)
	{
		_buttonNormalRenderer->setVisible(false);
		_buttonClickedRenderer->setVisible(true);
		_buttonDisabledRenderer->setVisible(false);

		if (_transitionType == TransitionType::COLOR)
		{
			this->stopAllActions();

			auto action = cocos2d::TintTo::create(_actionDuration, cocos2d::Color3B(_pressedColor.r, _pressedColor.b, _pressedColor.g));
			auto fadeAction = cocos2d::FadeTo::create(_actionDuration, _pressedColor.a);
			auto spwan = cocos2d::Spawn::create(action, fadeAction, nullptr);

			this->runAction(cocos2d::EaseSineOut::create(spwan));
		}
		else if (_pressedActionEnabled)
		{
			this->stopAllActions();

			cocos2d::ScaleTo* zoomChildAction = cocos2d::ScaleTo::create(_actionDuration,
				_originalScale * (1.0f + _zoomScale),
				_originalScale * (1.0f + _zoomScale));

			this->runAction(cocos2d::EaseSineOut::create(zoomChildAction));
		}
	}
	else if (_transitionType == TransitionType::SCALE)
	{
		this->stopAllActions();

		cocos2d::ScaleTo* zoomChildAction = cocos2d::ScaleTo::create(_actionDuration,
			_originalScale * (1.0f + _zoomScale),
			_originalScale * (1.0f + _zoomScale));

		this->runAction(cocos2d::EaseSineOut::create(zoomChildAction));
	}
	else
	{
		_buttonNormalRenderer->setVisible(true);
		_buttonClickedRenderer->setVisible(true);
		_buttonDisabledRenderer->setVisible(false);

		_buttonNormalRenderer->stopAllActions();
		_buttonNormalRenderer->setScale(1.0f + _zoomScale, 1.0f + _zoomScale);

		if (nullptr != _titleRenderer)
		{
			_titleRenderer->stopAllActions();
			_titleRenderer->setScaleX(1.0f + _zoomScale);
			_titleRenderer->setScaleY(1.0f + _zoomScale);
		}
	}
}

void Button::onPressStateChangedToDisabled()
{
	// if _disabledTextureLoaded is null
	if (!_disabledTextureLoaded)
	{
		if (_normalTextureLoaded)
		{
			_buttonNormalRenderer->setState(cocos2d::ui::Scale9Sprite::State::GRAY);
		}
	}
	else
	{
		_buttonNormalRenderer->setVisible(false);
		_buttonDisabledRenderer->setVisible(true);
	}

	if (_transitionType == TransitionType::COLOR)
	{
		this->stopAllActions();

		auto action = cocos2d::TintTo::create(_actionDuration, cocos2d::Color3B(_disabledColor.r, _disabledColor.b, _disabledColor.g));
		auto fadeAction = cocos2d::FadeTo::create(_actionDuration, _disabledColor.a);
		auto spwan = cocos2d::Spawn::create(action, fadeAction, nullptr);

		this->runAction(cocos2d::EaseSineOut::create(spwan));
	}

	_buttonClickedRenderer->setVisible(false);
	_buttonNormalRenderer->setScale(1.0);
	_buttonClickedRenderer->setScale(1.0);
}

void Button::setTransitionType(Button::TransitionType transitionType)
{
	_transitionType = transitionType;
	if (_transitionType == TransitionType::COLOR)
	{
		this->setCascadeColorEnabled(true);
		this->setCascadeOpacityEnabled(true);
	}
}

void Button::onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* unusedEvent)
{
	cocos2d::ui::Button::onTouchEnded(touch, unusedEvent);
}

void Button::cancelUpEvent()
{
	this->retain();
	cocos2d::ui::Button::cancelUpEvent();

	if (this->isEnabled())
	{
		onPressStateChangedToCancel();
	}
	this->release();
}

void Button::onClickListener(std::function<void(creator::Button* button)>&& listener)
{
	m_ListenerArg1 = std::move(listener);
	m_ListenerArg0 = nullptr;

	cocos2d::ui::Button::addTouchEventListener([this](cocos2d::Ref* sender, cocos2d::ui::Button::TouchEventType type) {
		if (type != cocos2d::ui::Button::TouchEventType::ENDED)
			return;

		m_ListenerArg1(this);
	});
}

void Button::onClick(std::function<void()>&& listener)
{
	m_ListenerArg0 = std::move(listener);
	m_ListenerArg1 = nullptr;

	cocos2d::ui::Button::addTouchEventListener([this](cocos2d::Ref* sender, cocos2d::ui::Button::TouchEventType type) {
		if (type != cocos2d::ui::Button::TouchEventType::ENDED)
			return;

		m_ListenerArg0();
	});
}

void Button::disableClicksOnScroll()
{
	if (m_ListenerArg0)
	{
		cocos2d::ui::Button::addTouchEventListener([this](cocos2d::Ref* sender, cocos2d::ui::Button::TouchEventType type) {
			if (type == cocos2d::ui::Widget::TouchEventType::BEGAN)
			{
				m_PositionDiff = 0;
			}
			else if (type == cocos2d::ui::Widget::TouchEventType::MOVED)
			{
				float positionDiffX = std::abs(_touchMovePosition.x - _touchBeganPosition.x);
				float positionDiffY = std::abs(_touchMovePosition.y - _touchBeganPosition.y);
				m_PositionDiff = std::max(positionDiffX, positionDiffY);
			}
			else if (type == cocos2d::ui::Widget::TouchEventType::ENDED && m_PositionDiff < 20)
			{
				m_ListenerArg0();
			}
		});
	}

	if (m_ListenerArg1)
	{
		cocos2d::ui::Button::addTouchEventListener([this](cocos2d::Ref* sender, cocos2d::ui::Button::TouchEventType type) {
			if (type == cocos2d::ui::Widget::TouchEventType::BEGAN)
			{
				m_PositionDiff = 0;
			}
			else if (type == cocos2d::ui::Widget::TouchEventType::MOVED)
			{
				float positionDiffX = std::abs(_touchMovePosition.x - _touchBeganPosition.x);
				float positionDiffY = std::abs(_touchMovePosition.y - _touchBeganPosition.y);
				m_PositionDiff = std::max(positionDiffX, positionDiffY);
			}
			else if (type == cocos2d::ui::Widget::TouchEventType::ENDED && m_PositionDiff < 20)
			{
				m_ListenerArg1(this);
			}
		});
	}
}

NS_CCR_END
