/**
 * Widget handler for widget components exported from Creator
 * 
 * The code was originally taken from DreLaptop's Pull request on the 
 * original creator_luacpp_support repo, but his code only partially supported widget's 
 * and only supported alignment in 9 directions + no support for stretching was there. 
 * 
 * This is my improved version which completely supports the widget component!
 * 
 * @author AmN
 */

#include "WidgetExport.h"

NS_CCR_BEGIN

WidgetAdapter* WidgetAdapter::create()
{
	auto widget = new (std::nothrow) WidgetAdapter;

	if (widget && widget->init())
	{
		widget->autorelease();
		return widget;
	}

	return nullptr;
}

bool WidgetAdapter::init()
{
	return true;
}

WidgetAdapter::WidgetAdapter() :
	isAlignOnce(true),
	target(nullptr),
	node(nullptr)
{
}

WidgetAdapter::~WidgetAdapter()
{
}

void WidgetAdapter::setIsAlignOnce(bool isAlignOnce)
{
	this->isAlignOnce = isAlignOnce;
}

void WidgetAdapter::setLayoutTarget(cocos2d::Node* layoutTarget)
{
	this->target = layoutTarget;
}

cocos2d::Size WidgetAdapter::getReadOnlyNodeSize(cocos2d::Node* parent)
{
	auto typeCheck = dynamic_cast<cocos2d::Scene*>(parent);
	if (typeCheck != nullptr)
	{
		return cocos2d::Director::getInstance()->getVisibleSize();
	}
	else
	{
		return parent->getContentSize();
	}
}

void WidgetAdapter::align()
{
	this->target = this->node->getParent();

	cocos2d::Size targetSize = this->getReadOnlyNodeSize(this->target);
	cocos2d::Vec2 targetAnchor = this->target->getAnchorPoint();

	bool isRoot = (dynamic_cast<cocos2d::Scene*>(this->target) != nullptr);
	float x = this->node->getCreatorPosition().x, y = this->node->getCreatorPosition().y;
	cocos2d::Vec2 anchor = this->node->getAnchorPoint();

	if (this->alignFlags & CREATOR_ALIGN_HORIZONTAL)
	{
		float localLeft, localRight, targetWidth = targetSize.width;

		if (isRoot)
		{
			localLeft = cocos2d::Director::getInstance()->getVisibleOrigin().x;
			localRight = localLeft + cocos2d::Director::getInstance()->getVisibleSize().width;
		}
		else
		{
			localLeft = -targetAnchor.x * targetWidth;
			localRight = localLeft + targetWidth;
		}

		// adjust borders according to offsets
		localLeft += this->isAbsoluteLeft ? this->left : this->left * targetWidth;
		localRight -= this->isAbsoluteRight ? this->right : this->right * targetWidth;

		float width, anchorX = anchor.x, scaleX = this->node->getScaleX();
		if (scaleX < 0)
		{
			anchorX = 1.0 - anchorX;
			scaleX = -scaleX;
		}

		if (this->isStretchedAcrossWidth())
		{
			width = localRight - localLeft;
			if (scaleX != 0)
			{
				this->node->setContentSize(cocos2d::Size(width / scaleX, this->node->getContentSize().height));
			}

			//			if (isRoot)
			//			{
			//				x = localLeft + anchorX * width;
			//			}
			//			else
			{
				// Cocos2d-x has 0,0 at the bottom left, so we gotta fix that
				x = localLeft + (anchorX)*width;
			}
		}
		else
		{
			// As per cocos2d-x coordinate system
			//			localLeft = 0;
			//			localRight = targetWidth;
			//
			//			localLeft += this->isAbsoluteLeft ? this->left : this->left * targetWidth;
			//			localRight -= this->isAbsoluteRight ? this->right : this->right * targetWidth;

			width = this->node->getContentSize().width * scaleX;
			if (this->isAlignHorizontalCenter())
			{
				float localHorizontalCenter = this->isAbsoluteHorizontalCenter ? this->horizontalCenter : this->horizontalCenter * targetWidth;
				float targetCenter = (0.5 - targetAnchor.x) * targetSize.width;

				x = targetCenter + (anchorX - 0.5) * width + localHorizontalCenter;
			}
			else if (this->isAlignLeft())
			{
				x = localLeft + anchorX * width;
			}
			else
			{
				x = localRight + (anchorX - 1) * width;
			}
		}
	}

	if (this->alignFlags & CREATOR_ALIGN_VERTICAL)
	{
		float localTop, localBottom, targetHeight = targetSize.height;

		if (isRoot)
		{
			localBottom = cocos2d::Director::getInstance()->getVisibleOrigin().y;
			localTop = localBottom + cocos2d::Director::getInstance()->getVisibleSize().height;
		}
		else
		{
			localBottom = -targetAnchor.y * targetHeight;
			localTop = localBottom + targetHeight;
		}

		// adjust borders according to offsets
		localBottom += this->isAbsoluteTop ? this->bottom : this->bottom * targetHeight;
		localTop -= this->isAbsoluteBottom ? this->top : this->top * targetHeight;

		float height, anchorY = anchor.y, scaleY = this->node->getScaleY();
		if (scaleY < 0)
		{
			anchorY = 1.0 - anchorY;
			scaleY = -scaleY;
		}

		if (this->isStretchedAcrossHeight())
		{
			height = localTop - localBottom;
			if (scaleY != 0)
			{
				this->node->setContentSize(cocos2d::Size(this->node->getContentSize().width, height / scaleY));
			}

			//			if (isRoot)
			//			{
			//				y = localBottom + anchorY * height;
			//			}
			//			else
			{
				// Cocos2d-x has 0,0 at the bottom left, so we gotta fix that
				y = localBottom + (anchorY)*height;
			}
		}
		else
		{
			// As per cocos2d-x coordinate system
			//			localTop = targetHeight;
			//			localBottom = 0;
			//
			//			localBottom += this->isAbsoluteBottom ? this->bottom : this->bottom * targetHeight;
			//			localTop -= this->isAbsoluteTop ? this->top : this->top * targetHeight;

			height = this->node->getContentSize().height * scaleY;
			if (this->isAlignVerticalCenter())
			{
				float localVerticalCenter = this->isAbsoluteVerticalCenter ? this->verticalCenter : this->verticalCenter * targetHeight;
				float targetMiddle = (0.5 - targetAnchor.y) * targetSize.height;

				y = targetMiddle + (anchorY - 0.5) * height + localVerticalCenter;
			}
			else if (this->isAlignBottom())
			{
				y = localBottom + anchorY * height;
			}
			else
			{
				y = localTop + (anchorY - 1) * height;
			}
		}
	}

	this->node->setPosition(x, y);

	// Update the creator's position, as this is the new position of this node
	this->node->setCreatorPosition(cocos2d::Vec2(x, y));
}

void WidgetAdapter::setupLayout()
{
	auto parent = this->node->getParent();
	CCASSERT(parent != nullptr, "WidgetAdapter: Node's parent can't be null");

	if (this->target == nullptr)
	{
		this->target = parent;
	}
}

//
// WidgetManager
//

WidgetManager::WidgetManager() :
	forceAlignDirty(false)
{
	this->setName("WidgetManager");
}

WidgetManager::~WidgetManager()
{
}

void WidgetManager::update(float dt)
{
	this->doAlign();
}

void WidgetManager::forceDoAlign()
{
	this->forceAlignDirty = true;
	this->doAlign();
}

void WidgetManager::doAlign()
{
	for (auto& widget : this->widgets)
	{
		if (this->forceAlignDirty || !(widget->isAlignOnce))
		{
			//widget->align();

			// For the widget, the origin will be Center of the screen, so we need to adjust the positions accordingly
			//shiftOrigin(m_CurrentScene);
		}
	}

	this->forceAlignDirty = false;
}

void WidgetManager::setupWidgets()
{
	for (auto& widget : this->widgets)
	{
		widget->setupLayout();
		widget->align();
	}

	this->newWidgets.clear();

	scheduleUpdate();
}

void WidgetManager::addWidget(WidgetAdapter* widget)
{
	this->widgets.pushBack(widget);
	this->newWidgets.pushBack(widget);
}

void WidgetManager::alignNewWidgets()
{
	for (auto& widget : this->newWidgets)
	{
		widget->setupLayout();
		widget->align();
	}

	this->newWidgets.clear();
}

void WidgetManager::clearWidgets()
{
	this->widgets.clear();
	this->newWidgets.clear();
}

NS_CCR_END
