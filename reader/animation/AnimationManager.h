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
#pragma once

#include <tuple>
#include <utility>
#include <vector>

#include "../Macros.h"
#include "AnimationClip.h"

NS_CCR_BEGIN

class AnimateClip;

struct AnimationInfo
{
	AnimationClip* defaultClip;
	cocos2d::Vector<AnimationClip*> clips;
	bool playOnLoad;
	bool attachedToScene; // True if the animation is attached to a scene; False if it is attached to a prefab
};

class AnimationManager : public cocos2d::Node
{
public:
	void playAnimationClip(cocos2d::Node* target, const std::string& animationClipName, const std::function<void()>& onEnd = nullptr);
	void playAnimationClip(cocos2d::Node* target, AnimationClip* clip, const std::function<void()>& onEnd = nullptr);
	AnimationClip* getAnimationClip(const std::string& animationClipName);

	// Stopped animations cannot be run again
	void stopAnimationClip(cocos2d::Node* target, AnimationClip* clip, bool callClipEndCallback = true);
	void stopAnimationClip(cocos2d::Node* target, const std::string& animationClipName, bool callClipEndCallback = true);
	void pauseAnimationClip(cocos2d::Node* target, const std::string& animationClipName);
	void resumeAnimationClip(cocos2d::Node* target, const std::string& animationClipName);
	AnimateClip* getAnimateClip(cocos2d::Node* target, const std::string& animationClipName);

	// if a "Play On Load" animation is a loop animation, please stop it manually.
	void stopAnimationClipsRunByPlayOnLoad();
	void RemoveAllAnimations();
	void RemoveSceneAnimations();
	void RemovePrefabAnimations();

private:
	friend class Reader;

	AnimationManager();
	~AnimationManager();

	// functions invoked by Reader only
	void addAnimation(const AnimationInfo& animationInfo);
	void playOnLoad();
	void playOnLoad(cocos2d::Node* target);

	void runAnimationClip(cocos2d::Node* target, AnimationClip* animationClip, const std::function<void()>& onEnd = nullptr);
	void removeAnimateClip(cocos2d::Node* target, const std::string& animationClipName);

	std::vector<AnimationInfo> m_Animations;
	std::vector<std::tuple<cocos2d::Node*, std::string, AnimateClip*>> m_CachedAnimates;

	CREATOR_DISALLOW_COPY_ASSIGN_AND_MOVE(AnimationManager);
};

NS_CC_END
