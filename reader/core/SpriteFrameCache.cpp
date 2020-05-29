#include "SpriteFrameCache.h"

#include "../CreatorReader.h"

NS_CCR_BEGIN

SpriteFrameCache* SpriteFrameCache::instance = nullptr;

SpriteFrameCache::SpriteFrameCache()
{
	SpriteFrameCache::instance = this;
}

SpriteFrameCache::~SpriteFrameCache()
{
	SpriteFrameCache::instance = nullptr;
}

void SpriteFrameCache::AddSpriteFrames(const void* buffer)
{
	buffer = buffer ? buffer : Reader::i()->_data.getBytes();
	const auto& sceneGraph = buffers::GetNodeGraph(buffer);
	const auto& spriteFrames = sceneGraph->spriteFrames();
	auto frameCache = cocos2d::SpriteFrameCache::getInstance();
    float scale = Reader::i()->m_SpriteRectScale;

	if (spriteFrames)
	{
		for (const auto& spriteFrame : *spriteFrames)
		{
			// Assumption: The atlas has already been loaded into the spriteframe cache
			if (spriteFrame->atlas())
			{
				cocos2d::SpriteFrame* sf = frameCache->getSpriteFrameByName(spriteFrame->name()->str());
				if (sf)
				{
					const auto& centerRect = spriteFrame->centerRect();
					sf->setCenterRectInPixels(cocos2d::Rect(centerRect->x() * scale, centerRect->y() * scale, centerRect->w() * scale, centerRect->h() * scale));
					m_AtlasSpriteFrames.emplace(spriteFrame->name()->str(), sf);
				}
				else
				{
					CCLOG("Failed to find spriteframe %s in any atlas. Did you forget to load the atlas that contains this spriteframe?", spriteFrame->name()->c_str());
				}
			}
			else
			{
				std::string name = spriteFrame->name()->str();
				cocos2d::SpriteFrame* sf = frameCache->getSpriteFrameByName(name);

				// Spriteframe already loaded
				if (sf)
					continue;

				std::string filepath = spriteFrame->texturePath()->str();
				const auto& rect = spriteFrame->rect();
				const auto& rotated = spriteFrame->rotated();
				const auto& offset = spriteFrame->offset();
				const auto& originalSize = spriteFrame->originalSize();

				// Find the actual file name
				std::string filename = filepath;
				if (filename.find("creator/resources/sprites/") != std::string::npos)
				{
					filename = filename.replace(filename.begin(), filename.begin() + std::string("creator/resources/sprites/").length(), "");
				}

				// If the file is inside the "split_qualities" folder, the file path will be prefixed with m_SpriteBasePath
				std::string search = "split_qualities/";
				size_t position = filename.find(search);
				if (position != std::string::npos)
				{
					// size_t start_pos = name.find("split_qualities/");
					// name.replace(start_pos, name.length(), "");

					// Erase this as we do not need it anymore
					filename.erase(position, search.length());
					filepath = std::string("sprites/").append(Reader::i()->m_SpriteBasePath).append("/").append(filename);

					std::string dirname = filepath.substr(0, filepath.find_last_of("/\\")).append("/");
					for (const auto& path: Reader::i()->m_PathReplacements)
					{
						if (dirname.rfind(path.first, 0) == 0)
						{
							filepath.replace(0, path.first.length(), path.second);
						}
					}

					sf = cocos2d::SpriteFrame::create(filepath,
						cocos2d::Rect(rect->x() * scale, rect->y() * scale, rect->w() * scale, rect->h() * scale),
						rotated,
						cocos2d::Vec2(offset->x(), offset->y()),
						cocos2d::Size(originalSize->w() * scale, originalSize->h() * scale));

					if (sf)
					{
						const auto& centerRect = spriteFrame->centerRect();
						sf->setCenterRectInPixels(cocos2d::Rect(centerRect->x() * scale, centerRect->y() * scale, centerRect->w() * scale, centerRect->h() * scale));
						m_SplitSpriteFrames.emplace(name, sf);
					}
				}
				// No split needed
				else
				{
					// Erase no_split (if present)
					search = "no_split/";
					position = filename.find(search);

					if (position != std::string::npos)
					{
						// size_t start_pos = name.find("no_split/");
						// name.replace(start_pos, name.length(), "");
						filename.erase(position, search.length());
					}

					filepath = std::string("sprites/").append(filename);

					std::string dirname = filepath.substr(0, filepath.find_last_of("/\\")).append("/");
					auto it = Reader::i()->m_PathReplacements.find(dirname);
					if (it != std::end(Reader::i()->m_PathReplacements))
					{
						filepath.replace(0, dirname.length(), it->second);
					}

					sf = cocos2d::SpriteFrame::create(filepath,
						cocos2d::Rect(rect->x(), rect->y(), rect->w(), rect->h()),
						rotated,
						cocos2d::Vec2(offset->x(), offset->y()),
						cocos2d::Size(originalSize->w(), originalSize->h()));

					if (sf)
					{
						const auto& centerRect = spriteFrame->centerRect();
						sf->setCenterRectInPixels(cocos2d::Rect(centerRect->x(), centerRect->y(), centerRect->w(), centerRect->h()));
						m_NoSplitSpriteFrames.emplace(name, sf);
					}
				}

				if (sf)
				{
					frameCache->addSpriteFrame(sf, name);
				}
			}
		}
	}
}

void SpriteFrameCache::AddToNoSplit(const std::string& name, cocos2d::SpriteFrame* sf)
{
	assert(cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(name) == nullptr && "[SpriteFrameCache.AddToNoSplit]: Spriteframe already added");
	
	m_NoSplitSpriteFrames.emplace(name, sf);
	cocos2d::SpriteFrameCache::getInstance()->addSpriteFrame(sf, name);
}

void SpriteFrameCache::AddToSplit(const std::string& name, cocos2d::SpriteFrame* sf)
{
	assert(cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(name) == nullptr && "[SpriteFrameCache.AddToSplit]: Spriteframe already added");
	
	 m_SplitSpriteFrames.emplace(name, sf);
	cocos2d::SpriteFrameCache::getInstance()->addSpriteFrame(sf, name);
}

void SpriteFrameCache::AddToAtlas(const std::string& name, cocos2d::SpriteFrame* sf)
{
	assert(cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(name) == nullptr && "[SpriteFrameCache.AddToAtlas]: Spriteframe already added");
	
	m_AtlasSpriteFrames.emplace(name, sf);
	cocos2d::SpriteFrameCache::getInstance()->addSpriteFrame(sf, name);
}

NS_CCR_END
