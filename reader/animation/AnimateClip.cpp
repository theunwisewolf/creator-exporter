/****************************************************************************
 Copyright (c) 2017 Chukong Technologies Inc.

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "AnimateClip.h"
#include "AnimationClip.h"
#include "AnimationClipProperties.h"
#include "Bezier.h"
#include "Easing.h"

#include "../CreatorReader.h"
#include "../core/SpriteFrameCache.h"

namespace
{

creator::AnimationClip* g_clip = nullptr;

// -1: invalid index
// -2: haven't reached first frame, so it should be the same as first frame
template <typename P>
int getValidIndex(const P& properties, float elapsed)
{
	if (properties.empty())
		return -1;

	if (properties.front().frame > elapsed)
		return -2;

	if (properties.back().frame <= elapsed)
		return properties.size() - 1;

	for (int i = 0, len = properties.size(); i < len; ++i)
	{
		const auto& prop = properties[i];
		if (prop.frame > elapsed)
			return i - 1;
	}

	return -1;
}

template <typename P>
float getPercent(const P& p1, const P& p2, float elapsed)
{
	const auto& curveType = p1.curveType;
	const auto& curveData = p1.curveData;
	auto ratio = (elapsed - p1.frame) / (p2.frame - p1.frame);

	if (!curveType.empty())
	{
		const auto& easingFunc = creator::Easing::getFunction(curveType);
		ratio = easingFunc(ratio);
	}
	if (!curveData.empty())
	{
		ratio = creator::Bazier::computeBezier(curveData, ratio);
	}

	return ratio;
}

void assignValue(float src, float& dst)
{
	dst = src;
}

void assignValue(std::string src, std::string& dst)
{
	dst = std::move(src);
}

void assignValue(bool src, bool& dst)
{
	dst = src;
}

void assignValue(const cocos2d::Color3B& src, cocos2d::Color3B& dst)
{
	dst.r = src.r;
	dst.g = src.g;
	dst.b = src.b;
}

void assignValue(const cocos2d::Vec2& src, cocos2d::Vec2& dst)
{
	dst.x = src.x;
	dst.y = src.y;
}

template <typename T>
void computeNextValue(T start, T end, float percent, T& out)
{
	out = start + percent * (end - start);
}

void computeNextValue(const cocos2d::Color3B& start, const cocos2d::Color3B& end, float percent, cocos2d::Color3B& out)
{
	computeNextValue(start.r, end.r, percent, out.r);
	computeNextValue(start.g, end.g, percent, out.g);
	computeNextValue(start.b, end.b, percent, out.b);
}

void computeNextValue(const cocos2d::Vec2& start, const cocos2d::Vec2& end, float percent, cocos2d::Vec2& out)
{
	computeNextValue(start.x, end.x, percent, out.x);
	computeNextValue(start.y, end.y, percent, out.y);
}

void computeNextValue(std::string start, const std::string& end, float percent, std::string& out)
{
	out = std::move(start);
}

void computeNextValue(bool start, bool end, float percent, bool& out)
{
	out = start;
}

template <typename P, typename T>
bool getNextValue(const P& properties, float elapsed, T& out)
{
	int index = getValidIndex(properties, elapsed);
	if (index == -1)
		return false;

	if (index == -2)
	{
		assignValue(properties.front().value, out);
		return true;
	}

	if (index == properties.size() - 1)
	{
		assignValue(properties.back().value, out);
		return true;
	}

	const auto& prop = properties[index];
	const auto& nextProp = properties[index + 1];
	float percent = getPercent(prop, nextProp, elapsed);
	computeNextValue(prop.value, nextProp.value, percent, out);

	return true;
}
} // namespace

USING_NS_CCR;

AnimateClip* AnimateClip::createWithAnimationClip(cocos2d::Node* rootTarget, AnimationClip* clip)
{
	AnimateClip* animate = new (std::nothrow) AnimateClip;
	if (animate && animate->initWithAnimationClip(rootTarget, clip))
		animate->autorelease();
	else
	{
		delete animate;
		animate = nullptr;
	}

	return animate;
}

AnimateClip::AnimateClip() :
	_clip(nullptr), _elapsed(0), _rootTarget(nullptr), _needStop(true), _durationToStop(0.f), _currentFramePlayed(false)
{
}

AnimateClip::~AnimateClip()
{
	// A loop animate might keep running until destruction, memory will leak if not stop it
	if (_running)
	{
		stopAnimate();
	}

	CC_SAFE_RELEASE(_clip);
	CC_SAFE_RELEASE(_rootTarget);
}

void AnimateClip::startAnimate()
{
	_running = true;
	scheduleUpdate();
}

void AnimateClip::stopAnimate()
{
	// release self
	_running = false;

	this->unscheduleUpdate();

	if (_endCallback)
		_endCallback();
}

void AnimateClip::pauseAnimate()
{
	this->unscheduleUpdate();
}

void AnimateClip::resumeAnimate()
{
	this->scheduleUpdate();
}

void AnimateClip::setCallbackForEndevent(const AnimateEndCallback& callback)
{
	_endCallback = callback;
}

bool AnimateClip::initWithAnimationClip(cocos2d::Node* rootTarget, AnimationClip* clip)
{
	_clip = clip;
	_rootTarget = rootTarget;
	CC_SAFE_RETAIN(_rootTarget);

	if (_clip)
	{
		_clip->retain();
		_durationToStop = _clip->getDuration();

		auto wrapMode = clip->getWrapMode();
		if (wrapMode == AnimationClip::WrapMode::Loop || wrapMode == AnimationClip::WrapMode::LoopReverse || wrapMode == AnimationClip::WrapMode::PingPong // PingPong and PingPongReverse are loop animations
			|| wrapMode == AnimationClip::WrapMode::PingPongReverse)
			_needStop = false;

		// assign it to be used in anonymous namespace
		g_clip = _clip;
	}

	return clip != nullptr;
}

void AnimateClip::update(float dt)
{
	// This ensures that the clip starts at 0
	if (_currentFramePlayed)
	{
		_elapsed += (dt * _clip->getSpeed());
	}
	else
	{
		_currentFramePlayed = true;
	}

	if (_needStop && _elapsed >= _durationToStop)
	{
		_elapsed = _durationToStop;

		// For making sure that the last frame is played properly
		const auto& allAnimProperties = _clip->getAnimProperties();
		for (const auto& animProperties : allAnimProperties)
		{
			this->doUpdate(animProperties, true);
		}

		this->stopAnimate();
		return;
	}

	const auto& allAnimProperties = _clip->getAnimProperties();
	for (const auto& animProperties : allAnimProperties)
	{
		this->doUpdate(animProperties);
	}
}

void AnimateClip::doUpdate(const AnimProperties& animProperties, bool lastFrame) const
{
	auto target = getTarget(animProperties.path);

	if (target)
	{
		auto elapsed = computeElapse();

		// update position
		cocos2d::Vec2 nextPos;
		if (getNextValue(animProperties.animPosition, elapsed, nextPos))
		{
			target->setPosition(nextPos);
			target->alignCenter();
		}

		// update color
		cocos2d::Color3B nextColor;
		if (getNextValue(animProperties.animColor, elapsed, nextColor))
			target->setColor(nextColor);

		// update scaleX
		float nextValue;
		if (getNextValue(animProperties.animScaleX, elapsed, nextValue))
		{
			target->setScaleX(nextValue);
		}

		// update scaleY
		if (getNextValue(animProperties.animScaleY, elapsed, nextValue))
		{
			target->setScaleY(nextValue);
		}

		// rotation
		if (getNextValue(animProperties.animRotation, elapsed, nextValue))
			target->setRotation(-nextValue);

		// SkewX
		if (getNextValue(animProperties.animSkewX, elapsed, nextValue))
			target->setSkewX(nextValue);

		// SkewY
		if (getNextValue(animProperties.animSkewY, elapsed, nextValue))
			target->setSkewY(nextValue);

		// Opacity
		if (getNextValue(animProperties.animOpacity, elapsed, nextValue))
		{
			auto label = dynamic_cast<cocos2d::Label*>(target);
			if (label && (label->isShadowEnabled() || label->getLabelEffectType() != cocos2d::LabelEffect::NORMAL))
			{
				cocos2d::Color4B color = label->getTextColor();
				label->setTextColor({color.r, color.g, color.b, static_cast<GLubyte>(nextValue)});
			}
			
			target->setOpacity(nextValue);
		}

		// anchor x
		if (getNextValue(animProperties.animAnchorX, elapsed, nextValue))
			target->setAnchorPoint(cocos2d::Vec2(nextValue, target->getAnchorPoint().y));

		// anchor y
		if (getNextValue(animProperties.animAnchorY, elapsed, nextValue))
			target->setAnchorPoint(cocos2d::Vec2(target->getAnchorPoint().x, nextValue));

		// position x
		if (getNextValue(animProperties.animPositionX, elapsed, nextValue))
		{
			target->setPositionX(nextValue);
			target->alignCenter();
		}

		// position y
		if (getNextValue(animProperties.animPositionY, elapsed, nextValue))
		{
			target->setPositionY(nextValue);
			target->alignCenter();
		}

		// Active
		bool nextBool;
		if (getNextValue(animProperties.animActive, elapsed, nextBool))
			target->setVisible(nextBool);

		// Width
		if (getNextValue(animProperties.animWidth, elapsed, nextValue))
		{
			auto size = target->getContentSize();
			size.width = nextValue;
			target->setContentSize(size);
		}

		// Height
		if (getNextValue(animProperties.animHeight, elapsed, nextValue))
		{
			auto size = target->getContentSize();
			size.height = nextValue;
			target->setContentSize(size);
		}

		// SpriteFrame
		std::string nextPath;
		if (getNextValue(animProperties.animSpriteFrame, elapsed, nextPath))
		{
			cocos2d::ui::Button* pButton = dynamic_cast<cocos2d::ui::Button*>(target);

			if (pButton)
			{
				auto frameCache = cocos2d::SpriteFrameCache::getInstance();
				auto pSpriteFrame = frameCache->getSpriteFrameByName(nextPath);
				if (pSpriteFrame)
				{
					pButton->getRendererNormal()->setSpriteFrame(pSpriteFrame);
				}
				else
				{
					pButton->getRendererNormal()->setTexture(nextPath);
				}
			}
			else
			{
				cocos2d::Sprite* pSprite = dynamic_cast<cocos2d::Sprite*>(target);
				if (pSprite)
				{
					auto frameCache = cocos2d::SpriteFrameCache::getInstance();
					auto pSpriteFrame = frameCache->getSpriteFrameByName(nextPath);
					if (pSpriteFrame)
					{
						pSprite->setSpriteFrame(pSpriteFrame);
					}
					else
					{
						pSprite->setTexture(nextPath);
					}

					if (creator::SpriteFrameCache::i()->IsNoSplit(nextPath))
					{
						pSprite->setContentSize(pSprite->getContentSize() * creator::Reader::i()->GetSpriteRectScale());
					}
					else
					{
						pSprite->setContentSize(pSprite->getContentSize());
					}
				}
			}
		}
	}
}

cocos2d::Node* AnimateClip::getTarget(const std::string& path) const
{
	if (path.empty())
		return _rootTarget;

	cocos2d::Node* ret = nullptr;
	_rootTarget->enumerateChildren(path, [&ret](cocos2d::Node* result) -> bool {
		ret = result;
		return true;
	});
	return ret;
}

float AnimateClip::computeElapse() const
{
	auto elapsed = _elapsed;
	auto duration = _clip->getDuration();

	// as the time goes, _elapsed will be bigger than duration when _needStop = false
	if (elapsed != duration) // Allows to run the last frame perfectly
	{
		elapsed = fmodf(elapsed, duration);
	}

	const auto wrapMode = _clip->getWrapMode();
	bool oddRound = (static_cast<int>(_elapsed / duration) % 2) == 0;
	if (wrapMode == AnimationClip::WrapMode::Reverse						  // reverse mode
		|| (wrapMode == AnimationClip::WrapMode::PingPong && !oddRound)		  // pingpong mode and it is the second round
		|| (wrapMode == AnimationClip::WrapMode::PingPongReverse && oddRound) // pingpongreverse mode and it is the first round
		|| (wrapMode == AnimationClip::WrapMode::LoopReverse)				  // loop reverse mode, reverse again and again
	)
		elapsed = duration - elapsed;

	return elapsed;
}
