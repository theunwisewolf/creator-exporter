
#include "AnimationManager.h"

#include "AnimateClip.h"

NS_CCR_BEGIN

AnimationManager::AnimationManager()
{
	this->setName("AnimationManager");
}

AnimationManager::~AnimationManager()
{
	for (auto&& animationInfo : _animations)
	{
		animationInfo.target->release();
	}
}

void AnimationManager::addAnimation(const AnimationInfo& animationInfo)
{
	_animations.push_back(animationInfo);
}

void AnimationManager::playOnLoad()
{
	for (auto& animationInfo : _animations)
	{
		if (animationInfo.playOnLoad && animationInfo.defaultClip)
			runAnimationClip(animationInfo.target, animationInfo.defaultClip);
	}
}

void AnimationManager::playOnLoad(cocos2d::Node* target)
{
	for (auto& animationInfo : _animations)
	{
		if (animationInfo.target == target && animationInfo.playOnLoad && animationInfo.defaultClip)
			runAnimationClip(animationInfo.target, animationInfo.defaultClip);
	}
}

void AnimationManager::stopAnimationClipsRunByPlayOnLoad()
{
	for (auto& animationInfo : _animations)
	{
		if (animationInfo.playOnLoad && animationInfo.defaultClip)
			stopAnimationClip(animationInfo.target, animationInfo.defaultClip->getName());
	}
}

AnimationClip* AnimationManager::getAnimationClip(cocos2d::Node* target, const std::string& animationClipName)
{
	bool foundTarget = false;
	bool foundAnimationClip = false;

	for (auto& animationInfo : _animations)
	{
		if (animationInfo.target == target)
		{
			for (auto& animClip : animationInfo.clips)
			{
				if (animClip->getName() == animationClipName)
				{
					return animClip;
				}
			}

			foundTarget = true;
			break;
		}
	}

	if (!foundTarget)
	{
		CCLOG("Can not find target: %p", target);
	}
	else if (!foundAnimationClip)
	{
		CCLOG("Can not find animation clip name \"%s\" for target %p", animationClipName.c_str(), target);
	}

	return nullptr;
}

void AnimationManager::playAnimationClip(cocos2d::Node* target, const std::string& animationClipName, const std::function<void()>& onEnd)
{
	auto clip = this->getAnimationClip(target, animationClipName);
	if (clip)
	{
		this->runAnimationClip(target, clip, onEnd);
	}
}

void AnimationManager::playAnimationClip(cocos2d::Node* target, AnimationClip* clip, const std::function<void()>& onEnd)
{
	this->runAnimationClip(target, clip, onEnd);
}

void AnimationManager::stopAnimationClip(cocos2d::Node* target, const std::string& animationClipName)
{
	auto animateClip = getAnimateClip(target, animationClipName);
	if (animateClip)
	{
		animateClip->stopAnimate();
		removeAnimateClip(target, animationClipName);
	}
}

void AnimationManager::pauseAnimationClip(cocos2d::Node* target, const std::string& animationClipName)
{
	auto animateClip = getAnimateClip(target, animationClipName);
	if (animateClip)
		animateClip->pauseAnimate();
}

void AnimationManager::resumeAnimationClip(cocos2d::Node* target, const std::string& animationClipName)
{
	auto animateClip = getAnimateClip(target, animationClipName);
	if (animateClip)
		animateClip->resumeAnimate();
}

void AnimationManager::runAnimationClip(cocos2d::Node* target, AnimationClip* animationClip, const std::function<void()>& onEnd)
{
	auto animate = AnimateClip::createWithAnimationClip(target, animationClip);
	animate->retain();
	this->retain();
	animate->setCallbackForEndevent([=]() {
		removeAnimateClip(target, animationClip->getName());
		this->release();

		// If there is an on end function supplied, run it
		if (onEnd)
		{
			onEnd();
		}
	});

	animate->startAnimate();
	_cachedAnimates.emplace_back(target, animationClip->getName(), animate);
}

void AnimationManager::removeAnimateClip(cocos2d::Node* target, const std::string& animationClipName)
{
	for (auto iter = _cachedAnimates.begin(), end = _cachedAnimates.end(); iter != end; ++iter)
	{
		auto&& e = *iter;
		if (std::get<0>(e) == target && std::get<1>(e) == animationClipName)
		{
			// release AnimateClip
			std::get<2>(e)->autorelease();

			_cachedAnimates.erase(iter);
			break;
		}
	}
}

AnimateClip* AnimationManager::getAnimateClip(cocos2d::Node* target, const std::string& animationClipName)
{
	for (auto&& e : _cachedAnimates)
	{
		if (std::get<0>(e) == target && std::get<1>(e) == animationClipName)
			return std::get<2>(e);
	}

	return nullptr;
}

NS_CCR_END
