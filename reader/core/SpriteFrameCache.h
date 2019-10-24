#pragma once

#include <string>
#include <unordered_map>

#include "cocos2d.h"

#include "../CreatorReader_generated.h"
#include "../Macros.h"

NS_CCR_BEGIN

class Reader;

class SpriteFrameCache
{
	friend class Reader;

private:
	// A list of spriteframes used
	// Spriteframes that have multiple sizes present for different resolutions
	std::unordered_map<std::string, cocos2d::SpriteFrame*> m_SplitSpriteFrames;
	// Spriteframes which are having only one size
	std::unordered_map<std::string, cocos2d::SpriteFrame*> m_NoSplitSpriteFrames;
	// Spriteframes which are part of a texture atlas
	std::unordered_map<std::string, cocos2d::SpriteFrame*> m_AtlasSpriteFrames;

	static SpriteFrameCache* instance;

public:
	inline static SpriteFrameCache* i() { return SpriteFrameCache::instance; }
	SpriteFrameCache();
	~SpriteFrameCache();

	void AddSpriteFrames();

    inline std::unordered_map<std::string, cocos2d::SpriteFrame*> GetSplitSpriteFrames() const { return m_SplitSpriteFrames; }
	inline std::unordered_map<std::string, cocos2d::SpriteFrame*> GetNoSplitSpriteFrames() const { return m_NoSplitSpriteFrames; }
	inline std::unordered_map<std::string, cocos2d::SpriteFrame*> GetAtlasSpriteFrames() const { return m_AtlasSpriteFrames; }

	inline bool IsSplit(cocos2d::SpriteFrame* sf)
    {
        auto it = std::find_if(std::begin(m_SplitSpriteFrames), std::end(m_SplitSpriteFrames), [sf](const auto& pair) {
			return pair.second == sf;
		});

		return it != std::end(m_SplitSpriteFrames);
    }

	inline bool IsSplit(const std::string& name) { return m_SplitSpriteFrames.count(name) > 0; }

	inline bool IsNoSplit(cocos2d::SpriteFrame* sf)
    {
        auto it = std::find_if(std::begin(m_NoSplitSpriteFrames), std::end(m_NoSplitSpriteFrames), [sf](const auto& pair) {
			return pair.second == sf;
		});

		return it != std::end(m_NoSplitSpriteFrames);
    }

	inline bool IsNoSplit(const std::string& name) { return m_NoSplitSpriteFrames.count(name) > 0; }

	inline bool IsPartOfAtlas(cocos2d::SpriteFrame* sf)
    {
        auto it = std::find_if(std::begin(m_AtlasSpriteFrames), std::end(m_AtlasSpriteFrames), [sf](const auto& pair) {
			return pair.second == sf;
		});

		return it != std::end(m_AtlasSpriteFrames);
    }

	inline bool IsPartOfAtlas(const std::string& name) { return m_AtlasSpriteFrames.count(name) > 0; }
	
	void AddToNoSplit(const std::string& name, cocos2d::SpriteFrame* sf);
	void AddToSplit(const std::string& name, cocos2d::SpriteFrame* sf);
	void AddToAtlas(const std::string& name, cocos2d::SpriteFrame* sf);
};

NS_CCR_END
