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

#include "CreatorReader.h"

#include "animation/AnimateClip.h"
#include "animation/AnimationClip.h"

#include "ParticleSystem.h"
#include "ui/Button.h"
#include "ui/Layout.h"
#include "ui/PageView.h"
#include "ui/RichtextStringVisitor.h"
#include "ui/ScrollView.h"
#include "ui/WidgetExport.h"
#include "ui/RichText.h"

#include "collider/Collider.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace cocos2d;
using namespace creator;
using namespace creator::buffers;

Reader* Reader::instance = nullptr;

USING_NS_CCR;

static void setSpriteQuad(V3F_C4B_T2F_Quad* quad, const cocos2d::Size& origSize, const int x, const int y, float x_factor, float y_factor);
static void tileSprite(cocos2d::Sprite* sprite);

namespace
{
template <typename T, typename U>
void setupAnimClipsPropValue(T fbPropList, U& proplist)
{
	if (fbPropList)
	{
		for (const auto fbProp : *fbPropList)
		{
			const auto fbFrame = fbProp->frame();
			const auto fbValue = fbProp->value();

			const auto fbCurveType = fbProp->curveType();
			std::string curveType = fbCurveType ? fbCurveType->c_str() : "";

			const auto fbCurveData = fbProp->curveData();
			std::vector<float> curveData;
			if (fbCurveData)
				for (const auto& value : *fbCurveData)
					curveData.push_back(value);

			proplist.push_back({fbFrame, fbValue, curveData, curveType});
		}
	}
}

template <typename T, typename U>
void setupAnimClipsPropString(T fbPropList, U& proplist)
{
	if (fbPropList)
	{
		for (const auto fbProp : *fbPropList)
		{
			const auto fbFrame = fbProp->frame();
			const auto fbValue = fbProp->value();

			const auto fbCurveType = fbProp->curveType();
			std::string curveType = fbCurveType ? fbCurveType->c_str() : "";

			const auto fbCurveData = fbProp->curveData();
			std::vector<float> curveData;
			if (fbCurveData)
				for (const auto& value : *fbCurveData)
					curveData.push_back(value);

			proplist.push_back({fbFrame,
				fbValue->str(),
				curveData,
				curveType});
		}
	}
}

template <typename T, typename U>
void setupAnimClipsPropVec2(T fbPropList, U& proplist)
{
	if (fbPropList)
	{
		for (const auto fbProp : *fbPropList)
		{
			const auto fbFrame = fbProp->frame();
			const auto fbValue = fbProp->value();

			const auto fbCurveType = fbProp->curveType();
			std::string curveType = fbCurveType ? fbCurveType->c_str() : "";

			const auto fbCurveData = fbProp->curveData();
			std::vector<float> curveData;
			if (fbCurveData)
				for (const auto& value : *fbCurveData)
					curveData.push_back(value);

			proplist.push_back({fbFrame,
				cocos2d::Vec2(fbValue->x(), fbValue->y()),
				curveData,
				curveType});
		}
	}
}

template <typename T, typename U>
void setupAnimClipsPropColor(T fbPropList, U& proplist)
{
	if (fbPropList)
	{
		for (const auto fbProp : *fbPropList)
		{
			const auto fbFrame = fbProp->frame();
			const auto fbValue = fbProp->value();

			const auto fbCurveType = fbProp->curveType();
			std::string curveType = fbCurveType ? fbCurveType->c_str() : "";

			const auto fbCurveData = fbProp->curveData();
			std::vector<float> curveData;
			if (fbCurveData)
				for (const auto& value : *fbCurveData)
					curveData.push_back(value);

			proplist.push_back({fbFrame,
				cocos2d::Color3B(fbValue->r(), fbValue->g(), fbValue->b()),
				curveData,
				curveType});
		}
	}
}
} // namespace

//
// Reader main class
//
Reader::Reader() :
	_version(""),
	_positionDiffDesignResolution(0, 0),
	m_SpriteRectScale(1.0f)
{
	Reader::instance = this;

	_animationManager = new AnimationManager();
	_collisionManager = new ColliderManager();
	_widgetManager = new WidgetManager();
	m_SpriteFrameCache = new SpriteFrameCache();

	_animationManager->autorelease();
	_collisionManager->autorelease();
	_widgetManager->autorelease();

	CC_SAFE_RETAIN(_animationManager);
	CC_SAFE_RETAIN(_collisionManager);
	CC_SAFE_RETAIN(_widgetManager);
}

Reader::~Reader()
{
	// Stop all animations that were run by play on load
	_animationManager->stopAnimationClipsRunByPlayOnLoad();

	CC_SAFE_RELEASE_NULL(_collisionManager);
	CC_SAFE_RELEASE_NULL(_animationManager);
	CC_SAFE_RELEASE_NULL(_widgetManager);

	delete m_SpriteFrameCache;
}

bool Reader::loadScene(const std::string& filename)
{
	// File last read, that means data will still be present
	if (_lastReadFile == filename)
	{
		return true;
	}

	_lastReadFile = filename;

	FileUtils* fileUtils = FileUtils::getInstance();

	const std::string& fullpath = fileUtils->fullPathForFilename(filename);
	if (fullpath.empty())
	{
		CCLOG("Reader: Scene file not found: %s", filename.c_str());
		return false;
	}

	_data = fileUtils->getDataFromFile(fullpath);

	const void* buffer = _data.getBytes();
	auto sceneGraph = GetNodeGraph(buffer);
	_version = sceneGraph->version()->str();

	this->setupScene();

	return true;
}

bool Reader::loadPrefab(const std::string& filename)
{
	// File last read, that means data will still be present
	if (_lastReadFile == filename)
	{
		return true;
	}

	_lastReadFile = filename;
	FileUtils* fileUtils = FileUtils::getInstance();

	const std::string& fullpath = fileUtils->fullPathForFilename(filename);
	if (fullpath.empty())
	{
		CCLOG("Reader: Prefab file not found: %s", filename.c_str());
		return false;
	}

	_data = fileUtils->getDataFromFile(fullpath);

	const void* buffer = _data.getBytes();
	auto nodeGraph = GetNodeGraph(buffer);
	_version = nodeGraph->version()->str();

	this->setupPrefab();

	return true;
}

void Reader::setupScene()
{
	const void* buffer = _data.getBytes();
	auto sceneGraph = GetNodeGraph(buffer);

	const auto& designResolution = sceneGraph->designResolution();
	const auto& fitWidth = sceneGraph->resolutionFitWidth();
	const auto& fitHeight = sceneGraph->resolutionFitHeight();

	auto director = cocos2d::Director::getInstance();
	auto glview = director->getOpenGLView();

	if (fitWidth && fitHeight)
	{
		glview->setDesignResolutionSize(designResolution->w(), designResolution->h(), ResolutionPolicy::SHOW_ALL);
	}
	else if (fitHeight)
	{
		glview->setDesignResolutionSize(designResolution->w(), designResolution->h(), ResolutionPolicy::FIXED_HEIGHT);
	}
	else if (fitWidth)
	{
		glview->setDesignResolutionSize(designResolution->w(), designResolution->h(), ResolutionPolicy::FIXED_WIDTH);
	}
	else
	{
		if (designResolution)
		{
			glview->setDesignResolutionSize(designResolution->w(), designResolution->h(), ResolutionPolicy::NO_BORDER);
		}
	}

	m_SpriteFrameCache->AddSpriteFrames();
	this->setupCollisionMatrix();

	if (designResolution)
	{
		const auto& realDesignResolution = Director::getInstance()->getOpenGLView()->getDesignResolutionSize();
		_positionDiffDesignResolution = cocos2d::Vec2((realDesignResolution.width - designResolution->w()) / 2,
			(realDesignResolution.height - designResolution->h()) / 2);
	}
}

void Reader::setupPrefab()
{
	const void* buffer = _data.getBytes();
	auto sceneGraph = GetNodeGraph(buffer);

	const auto& designResolution = sceneGraph->designResolution();

	m_SpriteFrameCache->AddSpriteFrames();

	if (designResolution)
	{
		const auto& realDesignResolution = Director::getInstance()->getOpenGLView()->getDesignResolutionSize();
		_positionDiffDesignResolution = cocos2d::Vec2((realDesignResolution.width - designResolution->w()) / 2, (realDesignResolution.height - designResolution->h()) / 2);
	}
}

void Reader::setupSpriteFrames()
{
	
}

void Reader::setupCollisionMatrix()
{
	const void* buffer = _data.getBytes();
	const auto& nodeGraph = GetNodeGraph(buffer);
	const auto& collisionMatrixBuffer = nodeGraph->collisionMatrix();

	std::vector<std::vector<bool>> collisionMatrix;
	for (const auto& matrixLineBuffer : *collisionMatrixBuffer)
	{
		std::vector<bool> line;
		for (const auto& value : *(matrixLineBuffer->value()))
			line.push_back(value);

		collisionMatrix.push_back(line);
	}

	_collisionManager->setCollistionMatrix(collisionMatrix);
}

cocos2d::Scene* Reader::getSceneGraph() const
{
	// Remove all old animations
	_animationManager->RemoveAllAnimations();
	
	const void* buffer = _data.getBytes();

	auto sceneGraph = GetNodeGraph(buffer);
	auto nodeTree = sceneGraph->root();

	_widgetManager->clearWidgets();
	cocos2d::Node* node = this->createTree(nodeTree);

	// Make scene at the center of screen
	// should not just node's position because it is a Scene, and it will cause issue that click position is not correct(it is a bug of cocos2d-x)
	// and should not change camera's position
	for (auto& child : node->getChildren())
	{
		if (dynamic_cast<Camera*>(child) == nullptr)
		{
			child->setPosition(child->getPosition() + _positionDiffDesignResolution);
		}
	}

	_animationManager->playOnLoad();

	node->addChild(_collisionManager);
	node->addChild(_animationManager);
	_collisionManager->start();

	_widgetManager->setupWidgets();
	node->addChild(_widgetManager);

	auto scene = static_cast<cocos2d::Scene*>(node);
	shiftOriginRecursively(scene);

#ifdef CREATOR_READER_DEBUG
	// Print the position of each node
	std::vector<cocos2d::Node*> stack;
	stack.push_back(scene);

	while (stack.size())
	{
		cocos2d::Node* currentNode = stack.back();

		stack.pop_back();

		// Push all it's children into the stack
		for (long index = currentNode->getChildrenCount() - 1; index >= 0; index--)
		{
			stack.push_back(currentNode->getChildren().at(index));
		}

		auto parentName = (currentNode->getParent()) ? currentNode->getParent()->getName().c_str() : "Scene";
		CCLOG("%s (parent: %s) position: %f, %f", currentNode->getName().c_str(), parentName, currentNode->getPosition().x, currentNode->getPosition().y);
	}
#endif

	return scene;
}

cocos2d::Node* Reader::getNodeGraph() const
{
	const void* buffer = _data.getBytes();

	auto nodeGraph = GetNodeGraph(buffer);
	auto nodeTree = nodeGraph->root();

	cocos2d::Node* node = this->createTree(nodeTree);

	// Make Node at the center of screen
	// should not just node's position because it is a Scene, and it will cause issue that click position is not correct(it is a bug of cocos2d-x)
	// and should not change camera's position
	for (auto& child : node->getChildren())
	{
		if (dynamic_cast<Camera*>(child) == nullptr)
		{
			child->setPosition(child->getPosition() + _positionDiffDesignResolution);
		}
	}

	// Won't play animations for prefabs automatically
	//_animationManager->playOnLoad(node);

	//node->addChild(_collisionManager);
	//    node->addChild(_animationManager);
	//    //_collisionManager->start();
	//
	//    _widgetManager->setupWidgets();
	//    node->addChild(_widgetManager);
	//
	auto prefab = static_cast<cocos2d::Node*>(node);
	shiftOriginRecursively(prefab);

	auto actualPrefab = prefab->getChildren().at(0);
	actualPrefab->removeFromParent();

	return actualPrefab;
}

WidgetManager* Reader::getWidgetManager() const
{
	return _widgetManager;
}

AnimationManager* Reader::getAnimationManager() const
{
	return _animationManager;
}

ColliderManager* Reader::getColliderManager() const
{
	return _collisionManager;
}

std::string Reader::getVersion() const
{
	return _version;
}

cocos2d::Node* Reader::createTree(const buffers::NodeTree* tree) const
{
	cocos2d::Node* node = nullptr;

	const void* buffer = tree->object();
	buffers::AnyNode bufferType = tree->object_type();
	bool parsing_button = false;
	bool parsing_layout = false;

	switch (static_cast<int>(bufferType))
	{
	case buffers::AnyNode_NONE:
		break;
	case buffers::AnyNode_Node:
		node = createNode(static_cast<const buffers::Node*>(buffer));
		break;
	case buffers::AnyNode_Label:
		node = createLabel(static_cast<const buffers::Label*>(buffer));
		break;
	case buffers::AnyNode_Layout:
		parsing_layout = true;
		node = createLayout(static_cast<const buffers::Layout*>(buffer));
		break;
	case buffers::AnyNode_RichText:
		node = createRichText(static_cast<const buffers::RichText*>(buffer));
		break;
	case buffers::AnyNode_Sprite: {
		auto spriteBuffer = static_cast<const buffers::Sprite*>(buffer);

		if (spriteBuffer->spriteType() == buffers::SpriteType::SpriteType_Sliced)
		{
			node = this->createScale9Sprite(spriteBuffer);
		}
		else if (spriteBuffer->spriteType() == buffers::SpriteType::SpriteType_Filled)
		{
			node = this->createFilledSprite(spriteBuffer);
		}
		else
		{
			node = this->createSprite(spriteBuffer);
		}
	}

	break;
	case buffers::AnyNode_TileMap:
		node = createTileMap(static_cast<const buffers::TileMap*>(buffer));
		break;
	case buffers::AnyNode_Particle:
		node = createParticle(static_cast<const buffers::Particle*>(buffer));
		break;
	case buffers::AnyNode_Scene:
		node = createScene(static_cast<const buffers::Scene*>(buffer));
		break;
	case buffers::AnyNode_Prefab:
		node = createPrefabRoot(static_cast<const buffers::Prefab*>(buffer));
		break;
	case buffers::AnyNode_ScrollView:
		node = createScrollView(static_cast<const buffers::ScrollView*>(buffer));
		break;
	case buffers::AnyNode_ProgressBar:
		node = createProgressBar(static_cast<const buffers::ProgressBar*>(buffer));
		break;
	case buffers::AnyNode_Button:
		node = createButton(static_cast<const buffers::Button*>(buffer));
		parsing_button = true;
		break;
	case buffers::AnyNode_EditBox:
		node = createEditBox(static_cast<const buffers::EditBox*>(buffer));
		break;
	case buffers::AnyNode_CreatorScene:
		break;
#if CREATOR_ENABLE_SPINE
	case buffers::AnyNode_SpineSkeleton:
		node = createSpineSkeleton(static_cast<const buffers::SpineSkeleton*>(buffer));
		break;
#endif
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	case buffers::AnyNode_VideoPlayer:
		node = createVideoPlayer(static_cast<const buffers::VideoPlayer*>(buffer));
		break;
	case buffers::AnyNode_WebView:
		node = createWebView(static_cast<const buffers::WebView*>(buffer));
		break;
#endif
	case buffers::AnyNode_Slider:
		node = createSlider(static_cast<const buffers::Slider*>(buffer));
		break;
	case buffers::AnyNode_Toggle:
		node = createToggle(static_cast<const buffers::Toggle*>(buffer));
		break;
	case buffers::AnyNode_ToggleGroup:
		node = createToggleGroup(static_cast<const buffers::ToggleGroup*>(buffer));
		break;
	case buffers::AnyNode_PageView:
		node = createPageView(static_cast<const buffers::PageView*>(buffer));
		break;
	case buffers::AnyNode_Mask:
		node = createMask(static_cast<const buffers::Mask*>(buffer));
		break;
		//        case buffers::AnyNode_DragonBones:
		//            node = createArmatureDisplay(static_cast<const buffers::DragonBones*>(buffer));
		//            break;
	case buffers::AnyNode_MotionStreak:
		node = createMotionStreak(static_cast<const buffers::MotionStreak*>(buffer));
		break;
	}

	// Set the position from creator;
	// This position is used later to shift origin of the node from the center to the bottom-left
	node->setCreatorPosition(node->getPosition());

	// recursively add its children
	const auto& children = tree->children();
	int childrenCount = 0;
	for (const auto& childBuffer : *children)
	{
		cocos2d::Node* child = createTree(childBuffer);
		if (child && node)
		{
			// should adjust child's position except Button's label
			if (parsing_button && childrenCount == 1 && dynamic_cast<cocos2d::Label*>(child) != nullptr)
			{
				auto button = static_cast<cocos2d::ui::Button*>(node);
				auto label = static_cast<cocos2d::Label*>(child);
				button->setTitleLabel(label);
			}
			else
			{
				if (parsing_layout)
				{
					static_cast<creator::Layout*>(node)->addChildNoDirty(child);
				}
				else
				{
					if (dynamic_cast<creator::ScrollView*>(node) && dynamic_cast<creator::Layout*>(child))
					{
						static_cast<creator::Layout*>(child)->setScrollView(static_cast<creator::ScrollView*>(node));
					}

					node->addChild(child);
				}
			}
		}
	}

	if (parsing_layout)
	{
		static_cast<creator::Layout*>(node)->markLayoutDirty();
		parsing_layout = false;
	}

	return node;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 * Render Nodes
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
cocos2d::Scene* Reader::createScene(const buffers::Scene* sceneBuffer) const
{
	cocos2d::Scene* scene = cocos2d::Scene::create();
	return scene;
}

void Reader::parseScene(cocos2d::Scene* scene, const buffers::Scene* sceneBuffer) const
{
}

cocos2d::Node* Reader::createPrefabRoot(const buffers::Prefab* prefabBuffer) const
{
	cocos2d::Node* node = cocos2d::Node::create();
	if (node)
	{
		this->parsePrefabRoot(node, prefabBuffer);
	}

	return node;
}

void Reader::parsePrefabRoot(cocos2d::Node* node, const buffers::Prefab* prefabBuffer) const
{
	auto nodeBuffer = prefabBuffer->node();
	const auto& name = nodeBuffer->name();
	if (name)
		node->setName(name->str());
	const auto& anchorPoint = nodeBuffer->anchorPoint();
	if (anchorPoint)
		node->setAnchorPoint(cocos2d::Vec2(anchorPoint->x(), anchorPoint->y()));
	const auto& color = nodeBuffer->color();
	if (color)
		node->setColor(cocos2d::Color3B(color->r(), color->g(), color->b()));
	const auto& opacity = nodeBuffer->opacity();
	node->setOpacity(opacity);
	const auto& cascadeOpacityEnabled = nodeBuffer->cascadeOpacityEnabled();
	node->setCascadeOpacityEnabled(cascadeOpacityEnabled);
	const auto& opacityModifyRGB = nodeBuffer->opacityModifyRGB();
	node->setOpacityModifyRGB(opacityModifyRGB);
	const auto position = nodeBuffer->position();
	if (position)
		node->setPosition(cocos2d::Vec2(position->x(), position->y()));
	node->setRotationSkewX(nodeBuffer->rotationSkewX());
	node->setRotationSkewY(nodeBuffer->rotationSkewY());
	node->setScaleX(nodeBuffer->scaleX());
	node->setScaleY(nodeBuffer->scaleY());
	node->setSkewX(nodeBuffer->skewX());
	node->setSkewY(nodeBuffer->skewY());
	const auto& tag = nodeBuffer->tag();
	node->setTag(tag);
	const auto contentSize = nodeBuffer->contentSize();
	if (contentSize)
		node->setContentSize(cocos2d::Size(contentSize->w(), contentSize->h()));
	const auto enabled = nodeBuffer->enabled();
	node->setVisible(enabled);

	// Parse animation component
	parseNodeAnimation(node, nodeBuffer);

	// Parse widget component
	parseWidget(node, nodeBuffer);
}

cocos2d::Node* Reader::createNode(const buffers::Node* nodeBuffer) const
{
	cocos2d::Node* node = cocos2d::Node::create();
	if (node)
		parseNode(node, nodeBuffer);
	return node;
}

void Reader::parseNode(cocos2d::Node* node, const buffers::Node* nodeBuffer) const
{
	const auto& globalZOrder = nodeBuffer->globalZOrder();
	node->setGlobalZOrder(globalZOrder);
	const auto& localZOrder = nodeBuffer->localZOrder();
	node->setLocalZOrder(localZOrder);
	const auto& name = nodeBuffer->name();
	if (name)
		node->setName(name->str());
	const auto& anchorPoint = nodeBuffer->anchorPoint();
	if (anchorPoint)
		node->setAnchorPoint(cocos2d::Vec2(anchorPoint->x(), anchorPoint->y()));
	const auto& color = nodeBuffer->color();
	if (color)
		node->setColor(cocos2d::Color3B(color->r(), color->g(), color->b()));
	const auto& opacity = nodeBuffer->opacity();
	node->setOpacity(opacity);
	const auto& cascadeOpacityEnabled = nodeBuffer->cascadeOpacityEnabled();
	node->setCascadeOpacityEnabled(cascadeOpacityEnabled);
	const auto& opacityModifyRGB = nodeBuffer->opacityModifyRGB();
	node->setOpacityModifyRGB(opacityModifyRGB);
	const auto position = nodeBuffer->position();
	if (position)
		node->setPosition(cocos2d::Vec2(position->x(), position->y()));
	node->setRotationSkewX(nodeBuffer->rotationSkewX());
	node->setRotationSkewY(nodeBuffer->rotationSkewY());
	node->setScaleX(nodeBuffer->scaleX());
	node->setScaleY(nodeBuffer->scaleY());
	node->setSkewX(nodeBuffer->skewX());
	node->setSkewY(nodeBuffer->skewY());
	const auto& tag = nodeBuffer->tag();
	node->setTag(tag);
	const auto contentSize = nodeBuffer->contentSize();
	if (contentSize)
		node->setContentSize(cocos2d::Size(contentSize->w(), contentSize->h()));
	const auto enabled = nodeBuffer->enabled();
	node->setVisible(enabled);

	// animation?
	parseNodeAnimation(node, nodeBuffer);

	parseColliders(node, nodeBuffer);

	parseWidget(node, nodeBuffer);
}

creator::Layout* Reader::createLayout(const buffers::Layout* nodeBuffer) const
{
	creator::Layout* layout = creator::Layout::create();

	if (layout)
	{
		this->parseLayout(layout, nodeBuffer);
	}

	return layout;
}

void Reader::parseLayout(creator::Layout* layout, const buffers::Layout* layoutBuffer) const
{
	const auto& nodeBuffer = layoutBuffer->node();
	parseNode(layout, nodeBuffer);

	creator::LayoutType layoutType = static_cast<creator::LayoutType>(layoutBuffer->layoutType());
	layout->setLayoutType(layoutType);

	creator::ResizeMode resizeMode = static_cast<creator::ResizeMode>(layoutBuffer->resizeMode());
	layout->setResizeMode(resizeMode);

	creator::AxisDirection startAxis = static_cast<creator::AxisDirection>(layoutBuffer->axisDirection());
	layout->setStartAxis(startAxis);

	creator::HorizontalDirection hDirection = static_cast<creator::HorizontalDirection>(layoutBuffer->horizontalDirection());
	layout->setHorizontalDirection(hDirection);

	creator::VerticalDirection vDirection = static_cast<creator::VerticalDirection>(layoutBuffer->verticalDirection());
	layout->setVerticalDirection(vDirection);

	const auto& sizeBuffer = layoutBuffer->cellSize();
	layout->setCellSize(cocos2d::Size(sizeBuffer->w(), sizeBuffer->h()));

	layout->setPaddingTop(layoutBuffer->paddingTop());
	layout->setPaddingBottom(layoutBuffer->paddingBottom());
	layout->setPaddingLeft(layoutBuffer->paddingLeft());
	layout->setPaddingRight(layoutBuffer->paddingRight());

	layout->setSpacingX(layoutBuffer->spacingX());
	layout->setSpacingY(layoutBuffer->spacingY());

	layout->setAffectedByScale(layoutBuffer->affectedByScale());
}

void Reader::parseNodeAnimation(cocos2d::Node* node, const buffers::Node* nodeBuffer) const
{
	const AnimationRef* animRef = nodeBuffer->anim();

	if (animRef)
	{
		AnimationInfo animationInfo;
		animationInfo.playOnLoad = animRef->playOnLoad();
		animationInfo.target = node;

		bool hasDefaultAnimclip = animRef->defaultClip() != nullptr;

		const auto& animationClips = animRef->clips();

		for (const auto& fbAnimationClip : *animationClips)
		{
			auto animClip = AnimationClip::create();

			const auto& duration = fbAnimationClip->duration();
			animClip->setDuration(duration);

			const auto& speed = fbAnimationClip->speed();
			animClip->setSpeed(speed);

			const auto& sample = fbAnimationClip->sample();
			animClip->setSample(sample);

			const auto& name = fbAnimationClip->name();
			animClip->setName(name->str());

			const auto& wrapMode = fbAnimationClip->wrapMode();
			animClip->setWrapMode(static_cast<AnimationClip::WrapMode>(wrapMode));

			// is it defalut animation clip?
			if (hasDefaultAnimclip && name->str() == animRef->defaultClip()->str())
				animationInfo.defaultClip = animClip;

			const auto& curveDatas = fbAnimationClip->curveData();
			for (const auto& fbCurveData : *curveDatas)
			{

				if (fbCurveData)
				{
					const AnimProps* fbAnimProps = fbCurveData->props();
					AnimProperties properties;

					// position
					setupAnimClipsPropVec2(fbAnimProps->position(), properties.animPosition);

					// position X
					setupAnimClipsPropValue(fbAnimProps->positionX(), properties.animPositionX);

					// position Y
					setupAnimClipsPropValue(fbAnimProps->positionY(), properties.animPositionY);

					// rotation
					setupAnimClipsPropValue(fbAnimProps->rotation(), properties.animRotation);

					// skew X
					setupAnimClipsPropValue(fbAnimProps->skewX(), properties.animSkewX);

					// skew Y
					setupAnimClipsPropValue(fbAnimProps->skewY(), properties.animSkewY);

					// scaleX
					setupAnimClipsPropValue(fbAnimProps->scaleX(), properties.animScaleX);

					// scaleY
					setupAnimClipsPropValue(fbAnimProps->scaleY(), properties.animScaleY);

					// Color
					setupAnimClipsPropColor(fbAnimProps->color(), properties.animColor);

					// opacity
					setupAnimClipsPropValue(fbAnimProps->opacity(), properties.animOpacity);

					// anchor x
					setupAnimClipsPropValue(fbAnimProps->anchorX(), properties.animAnchorX);

					// anchor y
					setupAnimClipsPropValue(fbAnimProps->anchorY(), properties.animAnchorY);

					// Active state
					setupAnimClipsPropValue(fbAnimProps->active(), properties.animActive);

					// Width
					setupAnimClipsPropValue(fbAnimProps->width(), properties.animWidth);

					// Height
					setupAnimClipsPropValue(fbAnimProps->height(), properties.animHeight);

					// SpriteFrame
					setupAnimClipsPropString(fbAnimProps->spriteFrame(), properties.animSpriteFrame);

					// path: self's animation doesn't have path
					// path is used for sub node
					if (fbCurveData->path())
					{
						properties.path = fbCurveData->path()->str();
					}

					animClip->addAnimProperties(properties);
				}
			}

			animationInfo.clips.pushBack(animClip);
		}

		// record animation information -> {node: AnimationInfo}
		_animationManager->addAnimation(animationInfo);
	}
}

void Reader::parseColliders(cocos2d::Node* node, const buffers::Node* nodeBuffer) const
{
	const auto& collidersBuffer = nodeBuffer->colliders();
	const auto& groupIndex = nodeBuffer->groupIndex();

	Collider* collider = nullptr;
	for (const auto& colliderBuffer : *collidersBuffer)
	{
		const auto& type = colliderBuffer->type();
		const auto& offsetBuffer = colliderBuffer->offset();
		cocos2d::Vec2 offset(offsetBuffer->x(), offsetBuffer->y());

		if (type == buffers::ColliderType::ColliderType_CircleCollider)
			collider = new CircleCollider(node, groupIndex, offset, colliderBuffer->radius());
		else if (type == buffers::ColliderType::ColliderType_BoxCollider)
		{
			const auto& sizeBuffer = colliderBuffer->size();
			cocos2d::Size size(sizeBuffer->w(), sizeBuffer->h());
			collider = new BoxCollider(node, groupIndex, offset, size);
		}
		else if (type == buffers::ColliderType::ColliderType_PolygonCollider)
		{
			const auto& pointsBuffer = colliderBuffer->points();
			std::vector<cocos2d::Vec2> points;
			for (const auto& pointBuffer : *pointsBuffer)
				points.emplace_back(pointBuffer->x(), pointBuffer->y());

			collider = new PolygonCollider(node, groupIndex, offset, points);
		}
		else
			assert(false);
	}

	if (collider)
	{
		_collisionManager->addCollider(collider);
	}
}

void Reader::parseWidget(cocos2d::Node* node, const buffers::Node* nodeBuffer) const
{
	WidgetAdapter* widget = nullptr;
	const auto& properties = nodeBuffer->widget();
	auto pNode = dynamic_cast<cocos2d::Node*>(node);
	if ((properties != nullptr) && (pNode != nullptr))
	{
		widget = WidgetAdapter::create();
		widget->setNode(pNode);

		// Align flags
		widget->setAlignFlags(properties->alignFlags());

		// Top, Bottom, Left and Right
		widget->setTop(properties->top());
		widget->setBottom(properties->bottom());
		widget->setLeft(properties->left());
		widget->setRight(properties->right());

		// Vertical and Horizontal Center,
		widget->setVerticalCenter(properties->verticalCenter());
		widget->setHorizontalCenter(properties->horizontalCenter());

		// Absolute or Pixelated positioning
		widget->setIsAlignOnce(properties->isAlignOnce());
		widget->setIsAbsoluteTop(properties->isAbsTop());
		widget->setIsAbsoluteBottom(properties->isAbsBottom());
		widget->setIsAbsoluteLeft(properties->isAbsLeft());
		widget->setIsAbsoluteRight(properties->isAbsRight());
		widget->setIsAbsoluteVerticalCenter(properties->isAbsHorizontalCenter());
		widget->setIsAbsoluteVerticalCenter(properties->isAbsVerticalCenter());

		_widgetManager->addWidget(widget);
		node->setHasWidget(true);
	}
}

cocos2d::ui::Scale9Sprite* Reader::createScale9Sprite(const buffers::Sprite* spriteBuffer) const
{
	cocos2d::ui::Scale9Sprite* sprite = cocos2d::ui::Scale9Sprite::create();
	sprite->setRenderingType(cocos2d::ui::Scale9Sprite::RenderingType::SLICE);

	if (sprite)
		parseScale9Sprite(sprite, spriteBuffer);

	return sprite;
}

void Reader::parseScale9Sprite(cocos2d::ui::Scale9Sprite* sprite, const buffers::Sprite* spriteBuffer) const
{
	// order is important:
	// 1st: set sprite frame
	const auto& frameName = spriteBuffer->spriteFrameName();
	if (frameName)
		sprite->setSpriteFrame(frameName->str());

	// 2nd: node properties
	const auto& nodeBuffer = spriteBuffer->node();
	parseNode(sprite, nodeBuffer);

	// Snippet from:
	// https://github.com/cocos2d/creator_to_cocos2dx/issues/174
	// Support for animations that change sprite opacity
	//	const auto& srcBlend = spriteBuffer->srcBlend();
	//	const auto& dstBlend = spriteBuffer->dstBlend();
	//
	//	if (srcBlend == GL_SRC_ALPHA && dstBlend == GL_ONE)
	//	{
	//		cocos2d::BlendFunc blendFunc;
	//		blendFunc.src = srcBlend;
	//		blendFunc.dst = dstBlend;
	//		sprite->setBlendFunc(blendFunc);
	//	}
	//
	//	// For Animations that plays around with opacity
	//	const AnimationRef *animRef = nodeBuffer->anim();
	//	if (animRef)
	//	{
	//		const auto& animationClips = animRef->clips();
	//		for (const auto& fbAnimationClip : *animationClips)
	//		{
	//			const auto& curveDatas = fbAnimationClip->curveData();
	//			for (const auto& fbCurveData : *curveDatas)
	//			{
	//				if (fbCurveData)
	//				{
	//					// Retrieve opacity curveData
	//					const AnimProps* fbAnimProps = fbCurveData->props();
	//					const auto& opacityValue = fbAnimProps->opacity();
	//
	//					// If there is an opacity data, set the blend mode as per creator;
	//					// As without setting blend from creator,
	//					// the Blend func read would be src = GL_ONE; dst = GL_ONE_MINUS_ALPHA
	//					// It will give off an additive effect instead of normal opacity animation;
	//					if (opacityValue->size() != 0)
	//					{
	//						cocos2d::BlendFunc blendFunc;
	//						blendFunc.src = srcBlend;
	//						blendFunc.dst = dstBlend;
	//						sprite->setBlendFunc(blendFunc);
	//					}
	//				}
	//			}
	//		}
	//	}
}

cocos2d::ProgressTimer* Reader::createFilledSprite(const buffers::Sprite* spriteBuffer) const
{
	cocos2d::Sprite* innerSprite = this->createSprite(spriteBuffer);
	cocos2d::ProgressTimer* sprite = cocos2d::ProgressTimer::create(innerSprite);

	if (sprite)
	{
		parseFilledSprite(sprite, spriteBuffer);
	}

	return sprite;
}

void Reader::parseFilledSprite(cocos2d::ProgressTimer* sprite, const buffers::Sprite* spriteBuffer) const
{
	const auto& nodeBuffer = spriteBuffer->node();
	parseNode(sprite, nodeBuffer);

	switch (spriteBuffer->fillType())
	{
	case buffers::SpriteFillType_Horizontal:
		sprite->setMidpoint(cocos2d::Vec2(0.0f, 0.0f));
		sprite->setType(cocos2d::ProgressTimer::Type::BAR);
		sprite->setBarChangeRate(cocos2d::Vec2(1.0f, 0.0f));
		break;
	case buffers::SpriteFillType_Vertical:
		sprite->setMidpoint(cocos2d::Vec2(0.0f, 0.0f));
		sprite->setType(cocos2d::ProgressTimer::Type::BAR);
		sprite->setBarChangeRate(cocos2d::Vec2(0.0f, 1.0f));
		sprite->setReverseDirection(true);
		break;

	case buffers::SpriteFillType_Radial:
		sprite->setMidpoint(cocos2d::Vec2(spriteBuffer->fillCenter()->x(), spriteBuffer->fillCenter()->y()));
		sprite->setType(cocos2d::ProgressTimer::Type::RADIAL);
		break;
	}

	sprite->setPercentage(spriteBuffer->fillRange() * 100.0f);
	sprite->getSprite()->setContentSize(sprite->getContentSize());
	// Doesn't support fillStart
}

cocos2d::Sprite* Reader::createSprite(const buffers::Sprite* spriteBuffer) const
{
	cocos2d::Sprite* sprite = cocos2d::Sprite::create();
	if (sprite)
	{
		parseSprite(sprite, spriteBuffer);
	}

	return sprite;
}

void Reader::parseTrimmedSprite(cocos2d::Sprite* sprite) const
{
	if (sprite)
	{
		auto pSpriteFrame = sprite->getSpriteFrame();
		pSpriteFrame->setOffset(cocos2d::Vec2(0, 0));
		sprite->setSpriteFrame(pSpriteFrame);
		sprite->setTextureRect(pSpriteFrame->getRect());
	}
}

void Reader::parseSprite(cocos2d::Sprite* sprite, const buffers::Sprite* spriteBuffer) const
{
	// order is important:
	// 1st: set sprite frame
	const auto& frameName = spriteBuffer->spriteFrameName();
	if (frameName)
		sprite->setSpriteFrame(frameName->str());

	// 2nd: node properties
	const auto& nodeBuffer = spriteBuffer->node();
	parseNode(sprite, nodeBuffer);

	// 3rd: sprite type
	const auto& spriteType = spriteBuffer->spriteType();
	switch (spriteType)
	{
	case buffers::SpriteType_Simple:
		sprite->setCenterRectNormalized(cocos2d::Rect(0, 0, 1, 1));
		break;
	case buffers::SpriteType_Tiled:
		tileSprite(sprite);
		break;
	case buffers::SpriteType_Filled:
	case buffers::SpriteType_Sliced:
		break;
	}

	// Creator doesn't premultiply alpha, so its blend function can not work in cocos2d-x.
	// const auto& srcBlend = spriteBuffer->srcBlend();
	// const auto& dstBlend = spriteBuffer->dstBlend();
	// cocos2d::BlendFunc blendFunc;
	// blendFunc.src = srcBlend;
	// blendFunc.dst = dstBlend;
	// sprite->setBlendFunc(blendFunc);

	// Snippet from:
	// https://github.com/cocos2d/creator_to_cocos2dx/issues/174
	// Support for animations that change sprite opacity
	//	const auto& srcBlend = spriteBuffer->srcBlend();
	//	const auto& dstBlend = spriteBuffer->dstBlend();
	//
	//	if (srcBlend == GL_SRC_ALPHA && dstBlend == GL_ONE)
	//	{
	//		cocos2d::BlendFunc blendFunc;
	//		blendFunc.src = srcBlend;
	//		blendFunc.dst = dstBlend;
	//		sprite->setBlendFunc(blendFunc);
	//	}
	//
	//	// For Animations that plays around with opacity
	//	const AnimationRef *animRef = nodeBuffer->anim();
	//	if (animRef)
	//	{
	//		const auto& animationClips = animRef->clips();
	//		for (const auto& fbAnimationClip : *animationClips)
	//		{
	//			const auto& curveDatas = fbAnimationClip->curveData();
	//			for (const auto& fbCurveData : *curveDatas)
	//			{
	//				if (fbCurveData)
	//				{
	//					// Retrieve opacity curveData
	//					const AnimProps* fbAnimProps = fbCurveData->props();
	//					const auto& opacityValue = fbAnimProps->opacity();
	//
	//					// If there is an opacity data, set the blend mode as per creator;
	//					// As without setting blend from creator,
	//					// the Blend func read would be src = GL_ONE; dst = GL_ONE_MINUS_ALPHA
	//					// It will give off an additive effect instead of normal opacity animation;
	//					if (opacityValue->size() != 0)
	//					{
	//						cocos2d::BlendFunc blendFunc;
	//						blendFunc.src = srcBlend;
	//						blendFunc.dst = dstBlend;
	//						sprite->setBlendFunc(blendFunc);
	//					}
	//				}
	//			}
	//		}
	//	}

#if 0
	// FIXME: do something with these values
	const auto& isTrimmed = spriteBuffer->trimEnabled();
	const auto& sizeMode = spriteBuffer->sizeMode();
#endif
}

cocos2d::TMXTiledMap* Reader::createTileMap(const buffers::TileMap* tilemapBuffer) const
{
	const auto& tmxfilename = tilemapBuffer->tmxFilename();
	cocos2d::TMXTiledMap* tilemap = TMXTiledMap::create(tmxfilename->str());
	if (tilemap)
		parseTilemap(tilemap, tilemapBuffer);
	return tilemap;
}

void Reader::parseTilemap(cocos2d::TMXTiledMap* tilemap, const buffers::TileMap* tilemapBuffer) const
{
	const auto& nodeBuffer = tilemapBuffer->node();
	parseNode(tilemap, nodeBuffer);

	// calculate scale. changing the contentSize in TMX doesn't affect its visual size
	// so we have to re-scale the map
	const auto& desiredSize = tilemapBuffer->desiredContentSize();
	const auto& currentSize = tilemap->getContentSize();

	float wr = desiredSize->w() / currentSize.width;
	float hr = desiredSize->h() / currentSize.height;

	float sx = tilemap->getScaleX();
	float sy = tilemap->getScaleY();

	tilemap->setScaleX(wr * sx);
	tilemap->setScaleY(hr * sy);
}

cocos2d::Label* Reader::createLabel(const buffers::Label* labelBuffer) const
{
	cocos2d::Label* label = nullptr;
	auto text = labelBuffer->labelText();
	auto fontSize = labelBuffer->fontSize();
	auto fontName = labelBuffer->fontName();

	auto fontType = labelBuffer->fontType();
	switch (fontType)
	{
	case buffers::FontType_TTF:
		label = cocos2d::Label::createWithTTF(text->str(), fontName->str(), fontSize);
		break;
	case buffers::FontType_BMFont:
		label = cocos2d::Label::createWithBMFont(fontName->str(), text->str());
		if (label)
			label->setBMFontSize(fontSize);
		break;
	case buffers::FontType_System:
		label = cocos2d::Label::createWithSystemFont(text->str(), fontName->str(), fontSize);
		break;
	}

	if (label)
		parseLabel(label, labelBuffer);
	return label;
}

void Reader::parseLabel(cocos2d::Label* label, const buffers::Label* labelBuffer) const
{
	const auto& nodeBuffer = labelBuffer->node();
	parseNode(label, nodeBuffer);

	const auto& lineHeight = labelBuffer->lineHeight();
	const auto& verticalA = labelBuffer->verticalAlignment();
	const auto& horizontalA = labelBuffer->horizontalAlignment();
	const auto& overflowType = labelBuffer->overflowType();
	const auto& enableWrap = labelBuffer->enableWrap();
	const auto contentSize = nodeBuffer->contentSize();
	const auto& fontSize = labelBuffer->fontSize();

	auto dh = lineHeight - fontSize;
	if (dh < 0)
		dh = 0;

	if ((cocos2d::Label::Overflow)overflowType == cocos2d::Label::Overflow::NONE)
	{
		const auto& sLabelSize = label->getContentSize();
		const auto& anchorPoint = label->getAnchorPoint();

		auto dy = lineHeight - sLabelSize.height;
		if (dy > 0)
		{
			auto textBottom = anchorPoint.y * sLabelSize.height;
			auto oldBottom = anchorPoint.y * contentSize->h();

			float adjusty = textBottom + dy - oldBottom;

			switch ((TextVAlignment)verticalA)
			{
			case TextVAlignment::CENTER:
				adjusty = textBottom + dy * 0.5 - oldBottom;
				break;
			case TextVAlignment::BOTTOM:
				adjusty = -(oldBottom - textBottom);
				break;
			default:
				break;
			}

			float y = label->getPositionY();
			label->setPositionY(y + adjusty);
		}

		label->enableWrap(false);
	}
	else
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
		//		label->setLineHeight(lineHeight);
		dh = (lineHeight - label->getRenderingFontSize());
		label->setLineSpacing(dh);
#else
		if (label->getLineHeight() != lineHeight && label->getLineHeight() > label->getRenderingFontSize())
		{
			dh = -(label->getLineHeight() - label->getRenderingFontSize());
			label->setLineSpacing(dh);
		}
#endif
	}

	if ((cocos2d::Label::Overflow)overflowType == cocos2d::Label::Overflow::CLAMP)
	{
		//		label->setLineSpacing(dh);
		label->setDimensions(contentSize->w(), contentSize->h());
		label->enableWrap(enableWrap);
	}

	if ((cocos2d::Label::Overflow)overflowType == cocos2d::Label::Overflow::SHRINK)
	{
		//		label->setLineSpacing(dh);
		label->setDimensions(contentSize->w(), contentSize->h());
		label->enableWrap(enableWrap);
	}

	if ((cocos2d::Label::Overflow)overflowType == cocos2d::Label::Overflow::RESIZE_HEIGHT)
	{
		//		label->setLineSpacing(dh);
		label->setDimensions(contentSize->w(), contentSize->h());
		//label->enableWrap(enableWrap);
	}

	//	label->setLineHeight(lineHeight);

	label->setVerticalAlignment(static_cast<cocos2d::TextVAlignment>(verticalA));
	label->setHorizontalAlignment(static_cast<cocos2d::TextHAlignment>(horizontalA));
	label->setOverflow(static_cast<cocos2d::Label::Overflow>(overflowType));

	// setContentSize has no effect on the label, we need to use setWidth and setHeight
	if (static_cast<cocos2d::Label::Overflow>(overflowType) != cocos2d::Label::Overflow::NONE)
	{
		label->setWidth(nodeBuffer->contentSize()->w());
		label->setHeight(nodeBuffer->contentSize()->h());
	}

	const auto& outline = labelBuffer->outline();
	if (outline)
	{
		const auto& color = outline->color();
		label->enableOutline(cocos2d::Color4B(color->r(), color->g(), color->b(), color->a()),
			outline->width());
	}

	const auto& shadow = labelBuffer->shadow();
	if (shadow)
	{
		const auto& color = shadow->color();
		const auto& offset = shadow->offset();
		label->enableShadow(cocos2d::Color4B(color->r(), color->g(), color->b(), color->a()),
			cocos2d::Size(offset->x(), offset->y()),
			shadow->blurRadius());
	}

	if (outline || shadow)
	{
		// The color here has to be reset because setColor sets the color of the entire fragment in the pixel shader
		// while setTextColor ensures that only the text part (excluding the outlines, shadows and glow) has the color
		label->setTextColor(cocos2d::Color4B(label->getColor().r, label->getColor().g, label->getColor().b, label->getOpacity()));
		label->setColor(cocos2d::Color3B::WHITE);
		label->setOpacity(255);
	}
}

creator::RichText* Reader::createRichText(const buffers::RichText* richTextBuffer) const
{
	creator::RichText* richText = creator::RichText::create();
	parseRichText(richText, richTextBuffer);
	return richText;
}

void Reader::parseRichText(creator::RichText* richText, const buffers::RichText* richTextBuffer) const
{
	const auto& nodeBuffer = richTextBuffer->node();
	parseNode(richText, nodeBuffer);

	const auto& fontSize = richTextBuffer->fontSize();
	richText->setFontSize(fontSize);
	const auto& fontFilename = richTextBuffer->fontFilename();
	richText->setFontFace(fontFilename->str());
	richText->setMaxWidth(richTextBuffer->maxWidth());

	const auto& text = richTextBuffer->text();
	if (text)
	{
		RichtextStringVisitor visitor;
		SAXParser parser;
		parser.setDelegator(&visitor);
		parser.parseIntrusive(const_cast<char*>(text->c_str()), text->Length());

		richText->initWithXML(visitor.getOutput());

		// FIXME: content width from Creator is not correct
		// so should recompute it here

		const auto& rawString = visitor.getRawString();
		auto maxFontSize = visitor.getMaxFontSize();
		int finalFontSize = std::max(static_cast<float>(maxFontSize), fontSize);
		auto label = cocos2d::Label::createWithSystemFont(rawString, fontFilename->str(), finalFontSize);

		auto realContentSize = label->getContentSize();
		auto finalWidth = std::max(realContentSize.width, richText->getContentSize().width);
		richText->setContentSize(cocos2d::Size(finalWidth, richText->getContentSize().height));
	}

	// should do it after richText->initWithXML
	richText->ignoreContentAdaptWithSize(false);

	const auto& maxWidth = richTextBuffer->maxWidth();
	if (maxWidth > 0)
	{
		const auto& contentSize = richText->getContentSize();
		richText->setContentSize(cocos2d::Size(maxWidth, contentSize.height));
	}
}

cocos2d::ParticleSystemQuad* Reader::createParticle(const buffers::Particle* particleBuffer) const
{
	const auto& particleFilename = particleBuffer->particleFilename();
	creator::ParticleSystem* particle = creator::ParticleSystem::create(particleFilename->str());

	if (particle)
	{
		parseParticle(particle, particleBuffer);
	}

	return particle;
}

void Reader::parseParticle(creator::ParticleSystem* particle, const buffers::Particle* particleBuffer) const
{
	const auto& nodeBuffer = particleBuffer->node();
	parseNode(particle, nodeBuffer);

	const auto& texturePath = particleBuffer->texturePath();
	if (texturePath)
	{
		auto sf = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(texturePath->c_str());
		particle->setTexture(sf->getTexture());
	}
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 * UI Nodes
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
cocos2d::ui::ScrollView* Reader::createScrollView(const buffers::ScrollView* scrollViewBuffer) const
{
	cocos2d::ui::ScrollView* scrollView = scrollViewBuffer->recycleElements() ? creator::ScrollView::create() : cocos2d::ui::ScrollView::create();
	parseScrollView(scrollView, scrollViewBuffer);
	return scrollView;
}

void Reader::parseScrollView(cocos2d::ui::ScrollView* scrollView, const buffers::ScrollView* scrollViewBuffer) const
{
	const auto& nodeBuffer = scrollViewBuffer->node();
	parseNode(scrollView, nodeBuffer);

	// backgroundImage:string;
	// backgroundImageScale9Enabled:bool;
	// backgroundImageColor:ColorRGB;
	// direction:ScrollViewDirection;
	// bounceEnabled:bool;
	// innerContainerSize:Size;

	const auto& backgroundImage = scrollViewBuffer->backgroundImage();
	const auto& backgroundImageScale9Enabled = scrollViewBuffer->backgroundImageScale9Enabled();
	const auto& backgroundImageColor = scrollViewBuffer->backgroundImageColor();
	const auto& direction = scrollViewBuffer->direction();
	const auto& bounceEnabled = scrollViewBuffer->bounceEnabled();
	const auto& innerContainerSize = scrollViewBuffer->innerContainerSize();

	if (backgroundImage)
	{
		scrollView->setBackGroundImage(backgroundImage->str(), cocos2d::ui::Widget::TextureResType::PLIST);
		scrollView->setBackGroundImageScale9Enabled(backgroundImageScale9Enabled);
		scrollView->setBackGroundImageColor(cocos2d::Color3B(backgroundImageColor->r(), backgroundImageColor->g(), backgroundImageColor->b()));
	}

	scrollView->setScrollBarEnabled(false);
	scrollView->setDirection(static_cast<cocos2d::ui::ScrollView::Direction>(direction));
	scrollView->setBounceEnabled(bounceEnabled);
	scrollView->setInnerContainerSize(cocos2d::Size(innerContainerSize->w(), innerContainerSize->h()));

	// FIXME: Call setJumpToPercent at the end, because it depens on having the contentSize correct
	// FIXME: uses the anchorPoint for the percent in the bar, but this migh break if it changes the position of the bar content node
	const auto& anchorPoint = scrollViewBuffer->node()->anchorPoint();
	scrollView->jumpToPercentHorizontal(anchorPoint->x() * 100.0f);
	scrollView->jumpToPercentVertical((1 - anchorPoint->y() * 100.0f));
}

cocos2d::ui::LoadingBar* Reader::createProgressBar(const buffers::ProgressBar* progressBarBuffer) const
{
	ui::LoadingBar* progressBar = ui::LoadingBar::create();

	parseProgressBar(progressBar, progressBarBuffer);
	return progressBar;
}

void Reader::parseProgressBar(cocos2d::ui::LoadingBar* progressBar, const buffers::ProgressBar* progressBarBuffer) const
{
	const auto& nodeBuffer = progressBarBuffer->node();
	parseNode(progressBar, nodeBuffer);

	progressBar->ignoreContentAdaptWithSize(false);

	if (progressBarBuffer->barSpriteFrameName())
	{
		auto sf = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(progressBarBuffer->barSpriteFrameName()->c_str());
		//progressBar->loadTexture(SpriteFrame *spriteframe);
		progressBar->loadTexture(sf);
		//progressBar->loadTexture(progressBarBuffer->barSpriteFrameName()->c_str());
	}

	progressBar->setContentSize(cocos2d::Size(progressBarBuffer->barContentSize()->w(), progressBarBuffer->barContentSize()->h()));
	progressBar->setPercent(progressBarBuffer->percent());

	// TODO: other types support?
	if (progressBarBuffer->barSpriteType() == buffers::SpriteType_Sliced)
		progressBar->setScale9Enabled(true);

	//	// Add the background of this progress bar as a child
	//	if (progressBarBuffer->backgroundSpriteFrameName())
	//	{
	//		auto sprite = cocos2d::Sprite::createWithSpriteFrameName(progressBarBuffer->backgroundSpriteFrameName()->c_str());
	//		//sprite->setPosition(cocos2d::Vec2(-progressBarBuffer->barPosition()->x(), -progressBarBuffer->barPosition()->y()));
	//		sprite->setStretchEnabled(true);
	//		sprite->setContentSize(cocos2d::Size(nodeBuffer->contentSize()->w(), nodeBuffer->contentSize()->h()));
	//		sprite->setAnchorPoint(cocos2d::Vec2(progressBarBuffer->node()->anchorPoint()->x(), progressBarBuffer->node()->anchorPoint()->y()));
	//		// background sprite should show first
	//		progressBar->addChild(sprite, -1);
	//		sprite->setPosition(cocos2d::Vec2(-progressBarBuffer->barPosition()->x(), 100+ -progressBarBuffer->barPosition()->y()));
	//		CCLOG("Bar Size: %f, %f", nodeBuffer->contentSize()->w(), nodeBuffer->contentSize()->h());
	//	}

	if (progressBarBuffer->reverse())
	{
		progressBar->setDirection(cocos2d::ui::LoadingBar::Direction::RIGHT);
	}
}

cocos2d::ui::EditBox* Reader::createEditBox(const buffers::EditBox* editBoxBuffer) const
{
	const auto& contentSize = editBoxBuffer->node()->contentSize();
	const auto& spriteFrameName = editBoxBuffer->backgroundImage();

	auto editBox = ui::EditBox::create(cocos2d::Size(contentSize->w(), contentSize->h()),
		spriteFrameName->str(),
		cocos2d::ui::Widget::TextureResType::PLIST);
	parseEditBox(editBox, editBoxBuffer);
	return editBox;
}

void Reader::parseEditBox(cocos2d::ui::EditBox* editBox, const buffers::EditBox* editBoxBuffer) const
{
	const auto& nodeBuffer = editBoxBuffer->node();
	parseNode(editBox, nodeBuffer);

	// backgroundImage:string;
	// returnType:EditBoxReturnType;
	// inputFlag:EditBoxInputFlag;
	// inputMode:EditBoxInputMode;
	// fontSize:int;
	// fontColor:ColorRGB;
	// placeholder:string;
	// placeholderFontSize:int;
	// placeholderFontColor:ColorRGB;
	// maxLength:int;
	// text:string;
	const auto& returnType = editBoxBuffer->returnType();
	const auto& inputFlag = editBoxBuffer->inputFlag();
	const auto& inputMode = editBoxBuffer->inputMode();
	const auto& fontSize = editBoxBuffer->fontSize();
	const auto& fontColor = editBoxBuffer->fontColor();
	const auto& placerholder = editBoxBuffer->placeholder();
	const auto& placerholderFontSize = editBoxBuffer->placeholderFontSize();
	const auto& placerholderFontColor = editBoxBuffer->placeholderFontColor();
	const auto& maxLen = editBoxBuffer->maxLength();
	const auto& text = editBoxBuffer->text();
	const auto& horizontalAlignment = editBoxBuffer->horizontalAlignment();
	const auto& verticalAlignment = editBoxBuffer->verticalAlignment();

	//editBox->setReturnType(static_cast<cocos2d::ui::EditBox::KeyboardReturnType>(returnType));
	editBox->setInputFlag(static_cast<cocos2d::ui::EditBox::InputFlag>(inputFlag));
	editBox->setInputMode(static_cast<cocos2d::ui::EditBox::InputMode>(inputMode));
	editBox->setFontSize(fontSize);
	editBox->setFontColor(cocos2d::Color3B(fontColor->r(), fontColor->g(), fontColor->b()));
	editBox->setPlaceHolder(placerholder->c_str());
	editBox->setPlaceholderFontSize(placerholderFontSize);
	editBox->setPlaceholderFontColor(cocos2d::Color3B(placerholderFontColor->r(), placerholderFontColor->g(), placerholderFontColor->b()));
	editBox->setMaxLength(maxLen);
	editBox->setText(text->c_str());
	//editBox->setTextVerticalAlignment(static_cast<cocos2d::TextVAlignment>(verticalAlignment));
	editBox->setTextHorizontalAlignment(static_cast<cocos2d::TextHAlignment>(horizontalAlignment));
}

cocos2d::ui::Button* Reader::createButton(const buffers::Button* buttonBuffer) const
{
	creator::Button* button = nullptr;

	const auto& spriteFrameName = buttonBuffer->spriteFrameName();
	const auto& pressedSpriteFrameName = buttonBuffer->pressedSpriteFrameName();
	const auto& disabledSpriteFrameName = buttonBuffer->disabledSpriteFrameName();
	if (spriteFrameName)
	{
		button = creator::Button::create(spriteFrameName->str(),
			pressedSpriteFrameName ? pressedSpriteFrameName->str() : spriteFrameName->str(),
			disabledSpriteFrameName ? disabledSpriteFrameName->str() : spriteFrameName->str(),
			cocos2d::ui::Widget::TextureResType::PLIST);
	}
	else
	{
		button = creator::Button::create();
	}

	this->parseButton(button, buttonBuffer);
	return button;
}

void Reader::parseButton(creator::Button* button, const buffers::Button* buttonBuffer) const
{
	const auto& nodeBuffer = buttonBuffer->node();
	this->parseNode(button, nodeBuffer);

	const auto& ignoreContentAdaptWithSize = buttonBuffer->ignoreContentAdaptWithSize();
	button->ignoreContentAdaptWithSize(ignoreContentAdaptWithSize);

	button->setActionDuration(buttonBuffer->duration());
	const auto transition = buttonBuffer->transition();
	button->setTransitionType(static_cast<creator::Button::TransitionType>(transition));

	if (buttonBuffer->backgroundNodeName())
	{
		button->setNodeBgName(buttonBuffer->backgroundNodeName()->str());
	}

	if (transition == creator::Button::TransitionType::COLOR)
	{
		const auto normalColor = buttonBuffer->normalColor();
		const auto pressedColor = buttonBuffer->pressedColor();
		const auto disabledColor = buttonBuffer->disabledColor();

		Color4B normal = Color4B(normalColor->r(), normalColor->g(), normalColor->b(), normalColor->a());
		Color4B pressed = Color4B(pressedColor->r(), pressedColor->g(), pressedColor->b(), pressedColor->a());
		Color4B disabled = Color4B(disabledColor->r(), disabledColor->g(), disabledColor->b(), disabledColor->a());

		button->setNormalColor(normal);
		button->setPressedColor(pressed);
		button->setDisabledColor(disabled);
	}
	else if (transition == creator::Button::TransitionType::SCALE)
	{
		button->setZoomScale(buttonBuffer->zoomScale() - 1);
		button->setPressedActionEnabled(true);
		button->setOriginalScale(button->getScale());
	}
	else
	{
		button->setPressedActionEnabled(false);
	}

	auto spriteType = buttonBuffer->spriteType();
	if (buttonBuffer->trimEnabled() || SpriteType::SpriteType_Sliced == spriteType)
	{
		auto size = button->getContentSize();

		auto pNormal = button->getRendererNormal();
		auto pClick = button->getRendererClicked();
		auto pDisabled = button->getRendererDisabled();

		parseTrimmedSprite(pNormal);
		parseTrimmedSprite(pClick);
		parseTrimmedSprite(pDisabled);

		button->setContentSize(size);
	}
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
cocos2d::experimental::ui::VideoPlayer* Reader::createVideoPlayer(const buffers::VideoPlayer* videoPlayerBuffer) const
{
	auto videoPlayer = cocos2d::experimental::ui::VideoPlayer::create();
	parseVideoPlayer(videoPlayer, videoPlayerBuffer);
	return videoPlayer;
}

void Reader::parseVideoPlayer(cocos2d::experimental::ui::VideoPlayer* videoPlayer, const buffers::VideoPlayer* videoPlayerBuffer) const
{
	const auto& nodeBuffer = videoPlayerBuffer->node();
	parseNode(videoPlayer, nodeBuffer);

	const auto& fullScreen = videoPlayerBuffer->fullScreen();
	videoPlayer->setFullScreenEnabled(fullScreen);

	const auto& keepAspect = videoPlayerBuffer->keepAspect();
	videoPlayer->setKeepAspectRatioEnabled(keepAspect);

	const auto& isLocal = videoPlayerBuffer->isLocal();
	const auto& url = videoPlayerBuffer->url();
	if (isLocal)
		videoPlayer->setFileName(url->str());
	else
		videoPlayer->setURL(url->str());
}

cocos2d::experimental::ui::WebView* Reader::createWebView(const buffers::WebView* webViewBuffer) const
{
	auto webView = cocos2d::experimental::ui::WebView::create();
	parseWebView(webView, webViewBuffer);
	return webView;
}

void Reader::parseWebView(cocos2d::experimental::ui::WebView* webView, const buffers::WebView* webViewBuffer) const
{
	const auto& nodeBuffer = webViewBuffer->node();
	parseNode(webView, nodeBuffer);

	const auto& url = webViewBuffer->url();
	if (url)
		webView->loadURL(url->str());
}
#endif

cocos2d::ui::Slider* Reader::createSlider(const buffers::Slider* sliderBuffer) const
{
	auto slider = cocos2d::ui::Slider::create();
	parseSlider(slider, sliderBuffer);
	return slider;
}

void Reader::parseSlider(cocos2d::ui::Slider* slider, const buffers::Slider* sliderBuffer) const
{
	const auto& nodeBuffer = sliderBuffer->node();
	parseNode(slider, nodeBuffer);

	const auto& percent = sliderBuffer->percent();
	slider->setPercent(percent);
	slider->setMaxPercent(100);
	slider->setScale9Enabled(true);

	const auto& barSpritePath = sliderBuffer->barTexturePath();
	if (barSpritePath)
	{
		slider->loadBarTexture(barSpritePath->str());
		const auto& barSize = sliderBuffer->barSize();
		slider->setCapInsetsBarRenderer(cocos2d::Rect(0, 0, barSize->w(), barSize->h()));
	}

	cocos2d::Sprite* render = nullptr;

	const auto& normalSpritePath = sliderBuffer->normalTexturePath();
	if (normalSpritePath)
	{
		slider->loadSlidBallTextureNormal(normalSpritePath->str());
		render = slider->getSlidBallNormalRenderer();
	}

	const auto& pressedSpritePath = sliderBuffer->pressedTexturePath();
	if (pressedSpritePath)
		slider->loadSlidBallTexturePressed(pressedSpritePath->str());

	const auto& disabledSpritePath = sliderBuffer->disabledTexturePath();
	if (disabledSpritePath)
		slider->loadSlidBallTextureDisabled(disabledSpritePath->str());

	slider->setUnifySizeEnabled(true);

	if (render)
	{
		const auto&& ballSize = sliderBuffer->ballSize();
		const auto&& ball = slider->getSlidBallRenderer();
		const auto contentSize = render->getContentSize();
		ball->setScale(ballSize->w() / contentSize.width,
			ballSize->h() / contentSize.height);
	}
}

cocos2d::ui::CheckBox* Reader::createToggle(const buffers::Toggle* toggleBuffer) const
{
	const auto& backgroundSpritePath = toggleBuffer->backgroundSpritePath();
	const auto& checkMarkSpritePath = toggleBuffer->checkMarkSpritePath();
	const std::string strBackgroundSpritePath = backgroundSpritePath ? backgroundSpritePath->str() : "";
	const std::string crossSpritePath = checkMarkSpritePath ? checkMarkSpritePath->str() : "";

	auto checkBox = cocos2d::ui::CheckBox::create(strBackgroundSpritePath, crossSpritePath);
	parseToggle(checkBox, toggleBuffer);
	return checkBox;
}

void Reader::parseToggle(cocos2d::ui::CheckBox* checkBox, const buffers::Toggle* toggleBuffer) const
{
	const auto& nodeBuffer = toggleBuffer->node();
	parseNode(checkBox, nodeBuffer);

	const auto& isChecked = toggleBuffer->isChecked();
	checkBox->setSelected(isChecked);

	const auto& zoomScale = toggleBuffer->zoomScale();
	checkBox->setZoomScale(zoomScale);
	checkBox->ignoreContentAdaptWithSize(false);

	const auto& interactable = toggleBuffer->interactable();
	if (!interactable)
	{
		checkBox->setTouchEnabled(false);

		const auto& enableAutoGrayEffect = toggleBuffer->enableAutoGrayEffect();
		if (enableAutoGrayEffect)
			checkBox->setSelected(false);
	}
}

cocos2d::ui::RadioButtonGroup* Reader::createToggleGroup(const buffers::ToggleGroup* toggleGroupBuffer) const
{
	auto radioGroup = cocos2d::ui::RadioButtonGroup::create();
	parseToggleGroup(radioGroup, toggleGroupBuffer);
	return radioGroup;
}

void Reader::parseToggleGroup(cocos2d::ui::RadioButtonGroup* radioGroup, const buffers::ToggleGroup* toggleGroupBuffer) const
{
	const auto& nodeBuffer = toggleGroupBuffer->node();
	parseNode(radioGroup, nodeBuffer);

	const auto& allowSwitchOff = toggleGroupBuffer->allowSwitchOff();
	if (allowSwitchOff)
		radioGroup->setAllowedNoSelection(true);

	const auto& toggles = toggleGroupBuffer->toggles();
	for (const auto& toggleBuffer : *toggles)
	{
		const auto& backgroundSpritePath = toggleBuffer->backgroundSpritePath();
		const auto& checkMarkSpritePath = toggleBuffer->checkMarkSpritePath();
		const std::string strBackgroundSpritePath = backgroundSpritePath ? backgroundSpritePath->str() : "";
		const std::string crossSpritePath = checkMarkSpritePath ? checkMarkSpritePath->str() : "";
		auto radioButton = cocos2d::ui::RadioButton::create(strBackgroundSpritePath, crossSpritePath);

		const auto& radioButtonNodeBuffer = toggleBuffer->node();
		parseNode(radioButton, radioButtonNodeBuffer);

		const auto& isChecked = toggleBuffer->isChecked();
		radioButton->setSelected(isChecked);

		const auto& interactable = toggleBuffer->interactable();
		if (!interactable)
		{
			radioButton->setTouchEnabled(false);

			const auto& enableAutoGrayEffect = toggleBuffer->enableAutoGrayEffect();
			if (enableAutoGrayEffect)
				radioButton->setSelected(false);
		}

		const auto& zoomScale = toggleBuffer->zoomScale();
		radioButton->setZoomScale(zoomScale);
		radioButton->ignoreContentAdaptWithSize(false);

		radioGroup->addRadioButton(radioButton);
		radioGroup->addChild(radioButton);
	}
}

cocos2d::ui::PageView* Reader::createPageView(const buffers::PageView* pageViewBuffer) const
{
	auto pageview = CreatorPageView::create();
	parsePageView(pageview, pageViewBuffer);
	return pageview;
}

void Reader::parsePageView(cocos2d::ui::PageView* pageview, const buffers::PageView* pageViewBuffer) const
{
	const auto& nodeBuffer = pageViewBuffer->node();
	parseNode(pageview, nodeBuffer);

	const auto& direction = pageViewBuffer->direction();
	pageview->setDirection(static_cast<cocos2d::ui::ScrollView::Direction>(direction));

	const auto& inertia = pageViewBuffer->inertia();
	pageview->setInertiaScrollEnabled(inertia);

	const auto& bounceEnabled = pageViewBuffer->bounceEnabled();
	pageview->setBounceEnabled(bounceEnabled);

	// indicator
	const auto& indicator = pageViewBuffer->indicator();
	const auto ICSpriteFrame = indicator->spriteFrame();
	if (ICSpriteFrame)
	{
		// should enable before loading texture
		pageview->setIndicatorEnabled(true);

		const auto spriteFrameFromTP = indicator->spriteFrameFromTP();
		const auto textureType = spriteFrameFromTP ? cocos2d::ui::Widget::TextureResType::PLIST : cocos2d::ui::Widget::TextureResType::LOCAL;
		pageview->setIndicatorIndexNodesTexture(ICSpriteFrame->str(), textureType);

		const auto& space = indicator->space();
		pageview->setIndicatorSpaceBetweenIndexNodes(space);

		const auto& positionAnchor = indicator->positionAnchor();
		pageview->setIndicatorPositionAsAnchorPoint(cocos2d::Vec2(positionAnchor->x(), positionAnchor->y()));
	}

	// pages
	const auto& pages = pageViewBuffer->pages();
	for (const auto& page : *pages)
	{
		auto imageView = cocos2d::ui::ImageView::create();

		const auto& spriteFrame = page->spriteFrame();
		const auto& spriteFrameFromTP = page->spriteFrameFromTP();
		const auto textureType = spriteFrameFromTP ? cocos2d::ui::Widget::TextureResType::PLIST : cocos2d::ui::Widget::TextureResType::LOCAL;
		imageView->loadTexture(spriteFrame->str(), textureType);

		// should prase node after loading texture
		const auto& imageViewNodeBuffer = page->node();
		parseNode(imageView, imageViewNodeBuffer);

		const auto scale9Enabled = page->scale9Enabled();
		imageView->setScale9Enabled(scale9Enabled);

		pageview->addPage(imageView);
	}

	// background
	const auto& background = pageViewBuffer->background();

	if (background)
	{
		const auto& backgroundSpriteFrame = background->spriteFrame();
		if (backgroundSpriteFrame)
		{
			const auto& spriteFrameFromTP = background->spriteFrameFromTP();
			const auto textureType = spriteFrameFromTP ? cocos2d::ui::Widget::TextureResType::PLIST : cocos2d::ui::Widget::TextureResType::LOCAL;
			pageview->setBackGroundImage(backgroundSpriteFrame->str(), textureType);
			pageview->setBackGroundImageScale9Enabled(true);
		}
	}
}

cocos2d::ClippingNode* Reader::createMask(const buffers::Mask* maskBuffer) const
{
	auto mask = cocos2d::ClippingNode::create();
	parseMask(mask, maskBuffer);
	return mask;
}

void Reader::parseMask(cocos2d::ClippingNode* mask, const buffers::Mask* maskBuffer) const
{
	const auto& nodeBuffer = maskBuffer->node();
	parseNode(mask, nodeBuffer);

	const auto& inverted = maskBuffer->inverted();
	mask->setInverted(inverted);

	const auto& type = maskBuffer->type();
	if (MaskType::MaskType_Rect == type)
	{
		const auto size = mask->getContentSize();
		cocos2d::Vec2 rectangle[4] = {
			{0, 0},
			{size.width, 0},
			{size.width, size.height},
			{0, size.height}};
		auto stencil = cocos2d::DrawNode::create();
		stencil->drawPolygon(rectangle, 4, cocos2d::Color4F::WHITE, 1, cocos2d::Color4F::WHITE);

		mask->setStencil(stencil);
	}
	else if (MaskType::MaskType_Ellipse == type)
	{
		const auto size = mask->getContentSize();
		const auto r = MIN(size.width, size.height) / 2;
		const auto& segments = maskBuffer->segments();
		auto stencil = cocos2d::DrawNode::create();
		stencil->drawSolidCircle(cocos2d::Vec2(r, r), r, M_PI * 2, segments, cocos2d::Color4F::WHITE);

		mask->setStencil(stencil);
	}
	else
	{
		// image stencil type
		const auto& alphaThreshold = maskBuffer->alphaThreshold();
		const auto& spriteFrame = maskBuffer->spriteFrame();
		auto stencil = cocos2d::Sprite::createWithSpriteFrameName(spriteFrame->c_str());
		stencil->setContentSize(mask->getContentSize());

		mask->setStencil(stencil);
		mask->setAlphaThreshold(alphaThreshold);
	}
}

cocos2d::MotionStreak* Reader::createMotionStreak(const buffers::MotionStreak* motionStreakBuffer) const
{
	const auto& timeToFade = motionStreakBuffer->timeToFade();
	const auto& minSeg = motionStreakBuffer->minSeg();
	const auto& strokeWidth = motionStreakBuffer->strokeWidth();

	const auto& color = motionStreakBuffer->strokeColor();
	const cocos2d::Color3B strokeColor(color->r(), color->g(), color->b());

	const auto& imagePath = motionStreakBuffer->texturePath();

	auto motionStreak = cocos2d::MotionStreak::create(timeToFade, minSeg, strokeWidth, strokeColor, imagePath->c_str());
	parseMotionStreak(motionStreak, motionStreakBuffer);

	return motionStreak;
}

void Reader::parseMotionStreak(cocos2d::MotionStreak* motionStreak, const buffers::MotionStreak* motionStreakBuffer) const
{
	const auto& nodeBuffer = motionStreakBuffer->node();

	// can not reuse parseNode because MotionStreak::setOpacity will cause assert error
	// parseNode(motionStreak, nodeBuffer);
	{
		auto node = motionStreak;
		const auto& globalZOrder = nodeBuffer->globalZOrder();
		node->setGlobalZOrder(globalZOrder);
		const auto& localZOrder = nodeBuffer->localZOrder();
		node->setLocalZOrder(localZOrder);
		const auto& name = nodeBuffer->name();
		if (name)
			node->setName(name->str());
		const auto& anchorPoint = nodeBuffer->anchorPoint();
		if (anchorPoint)
			node->setAnchorPoint(cocos2d::Vec2(anchorPoint->x(), anchorPoint->y()));
		const auto& color = nodeBuffer->color();
		if (color)
			node->setColor(cocos2d::Color3B(color->r(), color->g(), color->b()));
		const auto& cascadeOpacityEnabled = nodeBuffer->cascadeOpacityEnabled();
		node->setCascadeOpacityEnabled(cascadeOpacityEnabled);
		const auto& opacityModifyRGB = nodeBuffer->opacityModifyRGB();
		node->setOpacityModifyRGB(opacityModifyRGB);
		const auto position = nodeBuffer->position();
		if (position)
			node->setPosition(cocos2d::Vec2(position->x(), position->y()));
		node->setRotationSkewX(nodeBuffer->rotationSkewX());
		node->setRotationSkewY(nodeBuffer->rotationSkewY());
		node->setScaleX(nodeBuffer->scaleX());
		node->setScaleY(nodeBuffer->scaleY());
		node->setSkewX(nodeBuffer->skewX());
		node->setSkewY(nodeBuffer->skewY());
		const auto& tag = nodeBuffer->tag();
		node->setTag(tag);
		const auto contentSize = nodeBuffer->contentSize();
		if (contentSize)
			node->setContentSize(cocos2d::Size(contentSize->w(), contentSize->h()));
		const auto enabled = nodeBuffer->enabled();
		node->setVisible(enabled);

		// animation?
		parseNodeAnimation(node, nodeBuffer);

		parseColliders(node, nodeBuffer);
	}

	const auto& fastMode = motionStreakBuffer->fastMode();
	motionStreak->setFastMode(fastMode);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 * Misc Nodes
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if CREATOR_ENABLE_SPINE
spine::SkeletonAnimation* Reader::createSpineSkeleton(const buffers::SpineSkeleton* spineBuffer) const
{
	const auto& jsonFile = spineBuffer->jsonFile()->str();
	const auto& atlasFile = spineBuffer->atlasFile()->str();
	const auto& timeScale = spineBuffer->timeScale();

	auto spine = spine::SkeletonAnimation::createWithJsonFile(jsonFile, atlasFile, timeScale);

	if (spine)
		parseSpineSkeleton(spine, spineBuffer);
	return spine;
}

void Reader::parseSpineSkeleton(spine::SkeletonAnimation* spine, const buffers::SpineSkeleton* spineBuffer) const
{
	const auto& nodeBuffer = spineBuffer->node();
	parseNode(spine, nodeBuffer);

	// defaultSkin:string;
	// defaultAnimation:string;
	// loop:bool;
	// premultipliedAlpha:bool;
	// timeScale:float = 1;
	// debugSlots:bool;
	// debugBones:bool;

	const auto& defaultSkin = spineBuffer->defaultSkin()->str();
	const auto& defaultAnimation = spineBuffer->defaultAnimation()->str();
	const auto& loop = spineBuffer->loop();
	//    const auto& premultipledAlpha = spineBuffer->premultipliedAlpha();
	const auto& debugSlots = spineBuffer->debugSlots();
	const auto& debugBones = spineBuffer->debugBones();

	spine->setSkin(defaultSkin);
	spine->setAnimation(0, defaultAnimation, loop);
	spine->setDebugSlotsEnabled(debugSlots);
	spine->setDebugBonesEnabled(debugBones);
}
#endif

//dragonBones::CCArmatureDisplay* Reader::createArmatureDisplay(const buffers::DragonBones* dragonBonesBuffer) const
//{
//    const auto& boneDataPath = dragonBonesBuffer->boneDataPath();
//    const auto& atlasDataPath = dragonBonesBuffer->textureDataPath();
//
//    if (boneDataPath && atlasDataPath)
//    {
//        auto factory = dragonBones::CCFactory::getInstance();
//        const auto& boneDataName = dragonBonesBuffer->boneDataName();
//
//        // DragonBones can not reload Bone data in debug mode, may cause asset crash.
//        if (factory->getDragonBonesData(boneDataName->str()) == nullptr)
//        {
//            factory->loadDragonBonesData(boneDataPath->str());
//            factory->loadTextureAtlasData(atlasDataPath->str());
//        }
//
//        const auto& armatureName = dragonBonesBuffer->armature();
//        auto display = factory->buildArmatureDisplay(armatureName->str());
//        parseArmatureDisplay(display, dragonBonesBuffer);
//
//        return display;
//    }
//    else
//        return nullptr;
//
//}

//void Reader::parseArmatureDisplay(dragonBones::CCArmatureDisplay* armatureDisplay, const buffers::DragonBones* dragonBonesBuffer) const
//{
//    const auto& nodeBuffer = dragonBonesBuffer->node();
//    parseNode(armatureDisplay, nodeBuffer);
//
//    const auto& animationName = dragonBonesBuffer->animation();
//    if (animationName)
//    {
//        armatureDisplay->getAnimation().play(animationName->str());
//    }
//}

//
// Helper methods
//

void Reader::Reset()
{
	// Stop all animations that were run by play on load
	_animationManager->stopAnimationClipsRunByPlayOnLoad();

	CC_SAFE_RELEASE_NULL(_collisionManager);
	CC_SAFE_RELEASE_NULL(_animationManager);
	CC_SAFE_RELEASE_NULL(_widgetManager);

	// Create new instances
	_animationManager = new AnimationManager();
	_collisionManager = new ColliderManager();
	_widgetManager = new WidgetManager();

	_animationManager->autorelease();
	_collisionManager->autorelease();
	_widgetManager->autorelease();

	CC_SAFE_RETAIN(_animationManager);
	CC_SAFE_RETAIN(_collisionManager);
	CC_SAFE_RETAIN(_widgetManager);
}	

//
// Helper free functions
//
static void setSpriteQuad(cocos2d::V3F_C4B_T2F_Quad* quad, const cocos2d::Size& origSize, const int x, const int y, float x_factor, float y_factor)
{
	float offset_x = origSize.width * x;
	float offset_y = origSize.height * y;

	quad->bl.vertices.set(cocos2d::Vec3(offset_x, offset_y, 0));
	quad->br.vertices.set(cocos2d::Vec3(offset_x + (origSize.width * x_factor), offset_y, 0));
	quad->tl.vertices.set(cocos2d::Vec3(offset_x, offset_y + (origSize.height * y_factor), 0));
	quad->tr.vertices.set(cocos2d::Vec3(offset_x + (origSize.width * x_factor), offset_y + (origSize.height * y_factor), 0));

	if (x_factor != 1.0f || y_factor != 1.0f)
	{
		float x_size = (quad->br.texCoords.u - quad->bl.texCoords.u) * x_factor;
		float y_size = (quad->tl.texCoords.v - quad->bl.texCoords.v) * y_factor;

		quad->br.texCoords = Tex2F(quad->bl.texCoords.u + x_size, quad->bl.texCoords.v);
		quad->tl.texCoords = Tex2F(quad->tl.texCoords.u, quad->bl.texCoords.v + y_size);
		quad->tr.texCoords = Tex2F(quad->bl.texCoords.u + x_size, quad->bl.texCoords.v + y_size);
	}
}

static void tileSprite(cocos2d::Sprite* sprite)
{
	const auto new_s = sprite->getContentSize();
	const auto frame = sprite->getSpriteFrame();
	const auto orig_s_pixel = frame->getOriginalSizeInPixels();
	const auto orig_rect = frame->getRectInPixels();

	// cheat: let the sprite calculate the original Quad for us.
	sprite->setContentSize(orig_s_pixel);
	V3F_C4B_T2F_Quad origQuad = sprite->getQuad();

	// restore the size
	sprite->setContentSize(new_s);

	const float f_x = new_s.width / orig_rect.size.width;
	const float f_y = new_s.height / orig_rect.size.height;
	const int n_x = std::ceil(f_x);
	const int n_y = std::ceil(f_y);

	const int totalQuads = n_x * n_y;

	// use new instead of malloc, since Polygon info will release them using delete
	V3F_C4B_T2F_Quad* quads = new (std::nothrow) V3F_C4B_T2F_Quad[totalQuads];
	unsigned short* indices = new (std::nothrow) unsigned short[totalQuads * 6];

	// populate the vertices
	for (int y = 0; y < n_y; ++y)
	{
		for (int x = 0; x < n_x; ++x)
		{
			quads[y * n_x + x] = origQuad;
			float x_factor = (orig_rect.size.width * (x + 1) <= new_s.width) ? 1 : f_x - (long)f_x;
			float y_factor = (orig_rect.size.height * (y + 1) <= new_s.height) ? 1 : f_y - (long)f_y;
			CCLOG("x=%g, y=%g", x_factor, y_factor);
			setSpriteQuad(&quads[y * n_x + x], orig_rect.size, x, y, x_factor, y_factor);
		}
	}

	// populate the indices
	for (int i = 0; i < totalQuads; i++)
	{
		indices[i * 6 + 0] = (GLushort)(i * 4 + 0);
		indices[i * 6 + 1] = (GLushort)(i * 4 + 1);
		indices[i * 6 + 2] = (GLushort)(i * 4 + 2);
		indices[i * 6 + 3] = (GLushort)(i * 4 + 3);
		indices[i * 6 + 4] = (GLushort)(i * 4 + 2);
		indices[i * 6 + 5] = (GLushort)(i * 4 + 1);
	}

	TrianglesCommand::Triangles triangles;
	triangles.vertCount = 4 * totalQuads;
	triangles.indexCount = 6 * totalQuads;
	triangles.verts = (V3F_C4B_T2F*)quads;
	triangles.indices = indices;

	PolygonInfo poly;
	poly.setTriangles(triangles);

	// FIXME: setPolygonInfo will create new arrays and copy the recently alloced one
	// there should be a way to pass ownership so that it is not needed to copy them
	sprite->setPolygonInfo(poly);
}
