#include "AnimationManager.h"

#include "AnimateClip.h"

NS_CCR_BEGIN

AnimationManager::AnimationManager()
{
	this->setName("AnimationManager");
}

AnimationManager::~AnimationManager()
{
// 	for (auto&& animationInfo : _animations)
// 	{
// //		animationInfo.target->release();
// 	}
}

void AnimationManager::addAnimation(const AnimationInfo& animationInfo)
{
//	animationInfo.target->retain();
	m_Animations.push_back(animationInfo);
}

void AnimationManager::playOnLoad()
{
	assert(0 && "PlayOnLoad animations are not supported. Please play manually!");
	// for (auto& animationInfo : _animations)
	// {
	// 	if (animationInfo.playOnLoad && animationInfo.defaultClip)
	// 		runAnimationClip(animationInfo.target, animationInfo.defaultClip);
	// }
}

void AnimationManager::playOnLoad(cocos2d::Node* target)
{
	assert(0 && "PlayOnLoad animations are not supported. Please play manually!");
	// for (auto& animationInfo : _animations)
	// {
	// 	if (animationInfo.target == target && animationInfo.playOnLoad && animationInfo.defaultClip)
	// 		runAnimationClip(animationInfo.target, animationInfo.defaultClip);
	// }
}

void AnimationManager::stopAnimationClipsRunByPlayOnLoad()
{
	// for (auto& animationInfo : _animations)
	// {
	// 	if (animationInfo.playOnLoad && animationInfo.defaultClip)
	// 		stopAnimationClip(animationInfo.target, animationInfo.defaultClip->getName());
	// }
}

AnimationClip* AnimationManager::getAnimationClip(const std::string& animationClipName)
{
	bool foundAnimationClip = false;

	for (auto& animationInfo : m_Animations)
	{
		for (auto& animClip : animationInfo.clips)
		{
			if (animClip->getName() == animationClipName)
			{
				return animClip;
			}
		}
	}

	return nullptr;
}

void AnimationManager::playAnimationClip(cocos2d::Node* target, const std::string& animationClipName, const std::function<void()>& onEnd)
{
	auto clip = this->getAnimationClip(animationClipName);
	if (clip)
	{
		this->runAnimationClip(target, clip, onEnd);
	}
	else
	{
		CCLOG("Animation clip not found %s", animationClipName.c_str());
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
//		removeAnimateClip(target, animationClipName);
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
	auto animateClip = AnimateClip::createWithAnimationClip(target, animationClip);
	animateClip->retain();
	this->retain();

	animateClip->setCallbackForEndevent([=]() {
		// If there is an on end function supplied, run it
		if (onEnd)
		{
			onEnd();
		}

		this->removeAnimateClip(target, animationClip->getName());
		
		animateClip->release();
		this->release();
	});

	animateClip->startAnimate();
	m_CachedAnimates.emplace_back(target, animationClip->getName(), animateClip);
}

void AnimationManager::removeAnimateClip(cocos2d::Node* target, const std::string& animationClipName)
{
	for (auto iter = m_CachedAnimates.begin(), end = m_CachedAnimates.end(); iter != end; ++iter)
	{
		auto&& e = *iter;
		if (std::get<0>(e) == target && std::get<1>(e) == animationClipName)
		{
			m_CachedAnimates.erase(iter);
			break;
		}
	}
}

AnimateClip* AnimationManager::getAnimateClip(cocos2d::Node* target, const std::string& animationClipName)
{
	for (auto&& e : m_CachedAnimates)
	{
		if (std::get<0>(e) == target && std::get<1>(e) == animationClipName)
			return std::get<2>(e);
	}

	return nullptr;
}

void AnimationManager::RemoveAllAnimations()
{
	m_Animations.clear();
}

void AnimationManager::RemoveSceneAnimations()
{
	decltype(m_Animations)::iterator it;
	for (it = std::begin(m_Animations); it != std::end(m_Animations); ) 
	{
		if (it->attachedToScene)
		{
			it = m_Animations.erase(it);    
		}
		else 
		{
			++it;
		}
	}
}

void AnimationManager::RemovePrefabAnimations()
{
	decltype(m_Animations)::iterator it;
	for (it = std::begin(m_Animations) ; it != std::end(m_Animations); ) 
	{
		if (!it->attachedToScene)
		{
			it = m_Animations.erase(it);    
		}
		else 
		{
			++it;
		}
	}
}

NS_CCR_END
