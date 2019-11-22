#include "Layout.h"

#include "ScrollView.h"

NS_CCR_BEGIN

Layout* Layout::create()
{
	Layout* layout = new (std::nothrow) Layout();
	if (layout && layout->init())
	{
		layout->autorelease();
	}
	else
	{
		CC_SAFE_DELETE(layout);
	}

	return layout;
}

Layout::Layout() :
	m_LayoutType(LayoutType::None),
	m_ResizeMode(ResizeMode::None),
	m_StartAxis(AxisDirection::Horizontal),
	m_PaddingLeft(0),
	m_PaddingRight(0),
	m_PaddingTop(0),
	m_PaddingBottom(0),
	m_SpacingX(0),
	m_SpacingY(0),
	m_VerticalDirection(VerticalDirection::TopToBottom),
	m_HorizontalDirection(HorizontalDirection::LeftToRight),
	m_AffectedByScale(false),
	m_CellSize(cocos2d::Size(40, 40)),
	m_ScrollView(nullptr)
{
}

Layout::~Layout()
{
}

bool Layout::init()
{
	if (!Node::init())
	{
		return false;
	}

	//this->scheduleUpdate();

	return true;
}

void Layout::update(float dt)
{
	this->updateLayout();
}

void Layout::onEnter()
{
	Node::onEnter();

	this->scheduleUpdate();

	m_LayoutDirty = true;
}

void Layout::onExit()
{
	Node::onExit();

	this->unscheduleUpdate();
}

void Layout::addChildNoDirty(cocos2d::Node* node)
{
	Node::addChild(node);
}

void Layout::addChildNoDirty(cocos2d::Node* node, int zOrder)
{
	Node::addChild(node, zOrder);
}

void Layout::addChild(cocos2d::Node* node)
{
	Node::addChild(node);

#ifdef CC_PLATFORM_PC
	// AABB is an internal node for drawing the bounding box around a node
	if (node->getName() == "##AABB##")
	{
		return;
	}
#endif

	m_LayoutDirty = true;

	{
		if (m_LayoutDirty && this->getChildrenCount() > 0)
		{
			this->doLayout();
			this->adjustPosition();
			m_LayoutDirty = false;
		}
	}
}

void Layout::addChild(cocos2d::Node* node, int zOrder)
{
	Node::addChild(node, zOrder);

#ifdef CC_PLATFORM_PC
	// AABB is an internal node for drawing the bounding box around a node
	if (node->getName() == "##AABB##")
	{
		return;
	}
#endif

	m_LayoutDirty = true;

	{
		if (m_LayoutDirty && this->getChildrenCount() > 0)
		{
			this->doLayout();
			this->adjustPosition();
			m_LayoutDirty = false;
		}
	}
}

void Layout::removeChild(cocos2d::Node* node, bool cleanup)
{
	const auto& name = node->getName();
	Node::removeChild(node, cleanup);

#ifdef CC_PLATFORM_PC
	// AABB is an internal node for drawing the bounding box around a node
	if (name == "##AABB##")
	{
		return;
	}
#endif

	m_LayoutDirty = true;

	{
		if (m_LayoutDirty && this->getChildrenCount() > 0)
		{
			this->doLayout();
			this->adjustPosition();
			m_LayoutDirty = false;
		}
	}
}

void Layout::setContentSize(const cocos2d::Size& size)
{
	Node::setContentSize(size);

	m_LayoutDirty = true;
}

void Layout::setPosition(const cocos2d::Vec2& position)
{
	Node::setPosition(position);

	if (!this->initialized)
	{
		this->initialized = true;
		this->originalPosition = position;
	}
}

void Layout::doLayout()
{
	if (m_LayoutType == LayoutType::Horizontal)
	{
		float newWidth = this->getHorizontalBaseWidth(this->getChildren());
		auto fnPositionY = [](cocos2d::Node* child, float, float) -> float {
			// We have to make sure that we subtract the offset that was added by "shiftOrigin" so as to get the original position from Creator
			//			auto parent = child->getParent();
			//
			//			const auto p_ap = parent->getAnchorPoint();
			//			const auto p_cs = parent->getContentSize();
			//
			//			const auto offset = cocos2d::Vec2(p_ap.x * p_cs.width, p_ap.y * p_cs.height);

			return child->getCreatorPosition().y; // - offset.y;
		};

		this->doHorizontalLayout(newWidth, false, fnPositionY, true);
		Node::setContentSize(cocos2d::Size(newWidth, this->getContentSize().height));
	}
	else if (m_LayoutType == LayoutType::Vertical)
	{
		float newHeight = this->getVerticalBaseHeight(this->getChildren());

		auto fnPositionX = [](cocos2d::Node* child, float, float) -> float {
			return child->getCreatorPosition().x;
		};

		this->doVerticalLayout(newHeight, false, fnPositionX, true);
		Node::setContentSize(cocos2d::Size(this->getContentSize().width, newHeight));
	}
	else if (m_LayoutType == LayoutType::None)
	{
		//		if (this.resizeMode === ResizeMode.CONTAINER)
		//		{
		//			this._doLayoutBasic();
		//		}
	}
	else if (m_LayoutType == LayoutType::Grid)
	{
		this->doGridLayout();
	}
}

void Layout::doBasicLayout()
{
	// TODO
	//	auto children = this->getChildren();
	//	cocos2d::Rect allChildrenBoundingBox;
	//
	//	for (auto& child: children)
	//	{
	//		if (child->isVisible())
	//		{
	//			allChildrenBoundingBox.unionWithRect(child->getBoundingBox());
	//		}
	//	}
	//
	//	float leftBottomInParentSpace = this->getParent()->convertToNodeSpaceAR(cocos2d::Vec2(allChildrenBoundingBox., allChildrenBoundingBox.y));
	//	leftBottomInParentSpace = cocos2d::Vec2(leftBottomInParentSpace.x - m_PaddingLeft, leftBottomInParentSpace.y - m_PaddingBottom);
	//
	//	float rightTopInParentSpace = this->getParent()->convertToNodeSpaceAR(cc.v2(allChildrenBoundingBox.x + allChildrenBoundingBox.width,
	//																			allChildrenBoundingBox.y + allChildrenBoundingBox.height));
	//	rightTopInParentSpace = cocos2d::Vec2(rightTopInParentSpace.x + m_PaddingRight, rightTopInParentSpace.y + m_PaddingTop);
	//
	//	var newSize = cocos2d::Size(((rightTopInParentSpace.x - leftBottomInParentSpace.x).toFixed(2)),
	//						  parseFloat((rightTopInParentSpace.y - leftBottomInParentSpace.y).toFixed(2)));
	//
	//	var layoutPosition = this.node.getPosition();
	//	var newAnchorX = (layoutPosition.x - leftBottomInParentSpace.x) / newSize.width;
	//	var newAnchorY = (layoutPosition.y - leftBottomInParentSpace.y) / newSize.height;
	//	var newAnchor = cc.v2(parseFloat(newAnchorX.toFixed(2)), parseFloat(newAnchorY.toFixed(2)));
	//
	//	this.node.setAnchorPoint(newAnchor);
	//	this.node.setContentSize(newSize);
}

float Layout::getHorizontalBaseWidth(const cocos2d::Vector<cocos2d::Node*>& children)
{
	float newWidth = 0;
	float activeChildCount = 0;

	if (m_ResizeMode == ResizeMode::Container)
	{
		for (auto& child : children)
		{
#ifdef CC_PLATFORM_PC
			// AABB is an internal node for drawing the bounding box around a node
			if (child->getName() == "##AABB##")
			{
				continue;
			}
#endif

			if (child->isVisible())
			{
				activeChildCount++;
				newWidth += child->getContentSize().width * this->getUsedScaleValue(child->getScaleX());
			}
		}

		newWidth += (activeChildCount - 1) * m_SpacingX + m_PaddingLeft + m_PaddingRight;
	}
	else
	{
		newWidth = this->getContentSize().width;
	}

	return newWidth;
}

float Layout::getVerticalBaseHeight(const cocos2d::Vector<cocos2d::Node*>& children)
{
	float newHeight = 0;
	float activeChildCount = 0;

	if (m_ResizeMode == ResizeMode::Container)
	{
		for (auto& child : children)
		{
#ifdef CC_PLATFORM_PC
			// AABB is an internal node for drawing the bounding box around a node
			if (child->getName() == "##AABB##")
			{
				continue;
			}
#endif

			if (child->isVisible())
			{
				activeChildCount++;
				newHeight += child->getContentSize().height * this->getUsedScaleValue(child->getScaleY());
			}
		}

		newHeight += (activeChildCount - 1) * m_SpacingY + m_PaddingBottom + m_PaddingTop;
	}
	else
	{
		newHeight = this->getContentSize().height;
	}

	return newHeight;
}

float Layout::doHorizontalLayout(float baseWidth, bool rowBreak, const std::function<float(cocos2d::Node*, float, float)>& fnPositionY, bool applyChildren)
{
	cocos2d::Vec2 layoutAnchor = this->getAnchorPoint();
	auto children = this->getChildren();

	int sign = 1;
	float paddingX = m_PaddingLeft;
	float leftBoundaryOfLayout = -layoutAnchor.x * baseWidth;

	if (m_HorizontalDirection == HorizontalDirection::RightToLeft)
	{
		sign = -1;
		leftBoundaryOfLayout = (1 - layoutAnchor.x) * baseWidth;
		paddingX = m_PaddingRight;
	}

	float nextX = leftBoundaryOfLayout + sign * paddingX - sign * m_SpacingX;
	float rowMaxHeight = 0;
	float tempMaxHeight = 0;
	float secondMaxHeight = 0;

	float row = 0;
	float containerResizeBoundary = 0;

	float maxHeightChildAnchorY = 0;

	float activeChildCount = 0;
	for (size_t i = 0; i < children.size(); ++i)
	{
		auto child = children.at(i);

		if (child->isVisible())
		{
			activeChildCount++;
		}
	}

	float newChildWidth = m_CellSize.width;
	if (m_LayoutType != LayoutType::Grid && m_ResizeMode == ResizeMode::Children)
	{
		newChildWidth = (baseWidth - (m_PaddingLeft + m_PaddingRight) - (activeChildCount - 1) * m_SpacingX) / activeChildCount;
	}

	for (auto& child : children)
	{
#ifdef CC_PLATFORM_PC
		// AABB is an internal node for drawing the bounding box around a node
		if (child->getName() == "##AABB##")
		{
			continue;
		}
#endif
		if (!child->isVisible())
		{
			continue;
		}

		float childScaleX = this->getUsedScaleValue(child->getScaleX());
		float childScaleY = this->getUsedScaleValue(child->getScaleY());

		// For resizing children
		if (m_ResizeMode == ResizeMode::Children)
		{
			float height = child->getContentSize().height;
			float width = newChildWidth / childScaleX;

			if (m_LayoutType == LayoutType::Grid)
			{
				height = m_CellSize.height / childScaleY;
			}

			child->setContentSize(cocos2d::Size(width, height));
		}

		float anchorX = child->getAnchorPoint().x;
		float childBoundingBoxWidth = child->getContentSize().width * childScaleX;
		float childBoundingBoxHeight = child->getContentSize().height * childScaleY;

		if (secondMaxHeight > tempMaxHeight)
		{
			tempMaxHeight = secondMaxHeight;
		}

		if (childBoundingBoxHeight >= tempMaxHeight)
		{
			secondMaxHeight = tempMaxHeight;
			tempMaxHeight = childBoundingBoxHeight;
			maxHeightChildAnchorY = child->getAnchorPoint().y;
		}

		if (m_HorizontalDirection == HorizontalDirection::RightToLeft)
		{
			anchorX = 1 - child->getAnchorPoint().x;
		}

		nextX = nextX + sign * anchorX * childBoundingBoxWidth + sign * m_SpacingX;
		float rightBoundaryOfChild = sign * (1 - anchorX) * childBoundingBoxWidth;

		if (rowBreak)
		{
			float rowBreakBoundary = nextX + rightBoundaryOfChild + sign * (sign > 0 ? m_PaddingRight : m_PaddingLeft);
			float leftToRightRowBreak = m_HorizontalDirection == HorizontalDirection::LeftToRight && rowBreakBoundary > (1 - layoutAnchor.x) * baseWidth;
			float rightToLeftRowBreak = m_HorizontalDirection == HorizontalDirection::RightToLeft && rowBreakBoundary < -layoutAnchor.x * baseWidth;

			if (leftToRightRowBreak || rightToLeftRowBreak)
			{
				if (childBoundingBoxHeight >= tempMaxHeight)
				{
					if (secondMaxHeight == 0)
					{
						secondMaxHeight = tempMaxHeight;
					}

					rowMaxHeight += secondMaxHeight;
					secondMaxHeight = tempMaxHeight;
				}
				else
				{
					rowMaxHeight += tempMaxHeight;
					secondMaxHeight = childBoundingBoxHeight;
					tempMaxHeight = 0;
				}

				nextX = leftBoundaryOfLayout + sign * (paddingX + anchorX * childBoundingBoxWidth);
				row++;
			}
		}

		float finalPositionY = fnPositionY(child, rowMaxHeight, row);
		if (baseWidth >= (childBoundingBoxWidth + m_PaddingLeft + m_PaddingRight))
		{
			if (applyChildren)
			{
				child->setPosition(cocos2d::Vec2(nextX, finalPositionY));
				//CCLOG("%s position: %f, %f", child->getName().c_str(), nextX, finalPositionY);
			}
		}

		float signX = 1;
		float tempFinalPositionY;
		float topMarign = (tempMaxHeight == 0) ? childBoundingBoxHeight : tempMaxHeight;

		if (m_VerticalDirection == VerticalDirection::TopToBottom)
		{
			containerResizeBoundary = containerResizeBoundary || this->getContentSize().height;
			signX = -1;
			tempFinalPositionY = finalPositionY + signX * (topMarign * maxHeightChildAnchorY + m_PaddingBottom);

			if (tempFinalPositionY < containerResizeBoundary)
			{
				containerResizeBoundary = tempFinalPositionY;
			}
		}
		else
		{
			containerResizeBoundary = containerResizeBoundary || -this->getContentSize().height;
			tempFinalPositionY = finalPositionY + signX * (topMarign * maxHeightChildAnchorY + m_PaddingTop);

			if (tempFinalPositionY > containerResizeBoundary)
			{
				containerResizeBoundary = tempFinalPositionY;
			}
		}

		nextX += rightBoundaryOfChild;
	}

	return containerResizeBoundary;
}

void Layout::doLayoutGridAxisHorizontal(cocos2d::Vec2 layoutAnchor, cocos2d::Size layoutSize)
{
	float baseWidth = layoutSize.width;

	float sign = 1;
	float bottomBoundaryOfLayout = -layoutAnchor.y * layoutSize.height;
	float paddingY = m_PaddingBottom;

	if (m_VerticalDirection == VerticalDirection::TopToBottom)
	{
		sign = -1;
		bottomBoundaryOfLayout = (1 - layoutAnchor.y) * layoutSize.height;
		paddingY = m_PaddingTop;
	}

	auto fnPositionY = [&](cocos2d::Node* child, float topOffset, float row) {
		return bottomBoundaryOfLayout + sign * (topOffset + child->getAnchorPoint().y * child->getContentSize().height * this->getUsedScaleValue(child->getScaleY()) + paddingY + row * this->m_SpacingY);
	};

	float newHeight = 0;
	if (m_ResizeMode == ResizeMode::Container)
	{
		// Calculate the new height of container, it won't change the position of it's children
		float boundary = this->doHorizontalLayout(baseWidth, true, fnPositionY, false);
		newHeight = bottomBoundaryOfLayout - boundary;

		if (newHeight < 0)
		{
			newHeight *= -1;
		}

		bottomBoundaryOfLayout = -layoutAnchor.y * newHeight;

		if (m_VerticalDirection == VerticalDirection::TopToBottom)
		{
			sign = -1;
			bottomBoundaryOfLayout = (1 - layoutAnchor.y) * newHeight;
		}
	}

	this->doHorizontalLayout(baseWidth, true, fnPositionY, true);

	if (m_ResizeMode == ResizeMode::Container)
	{
		Node::setContentSize(cocos2d::Size(baseWidth, newHeight));
	}
}

float Layout::doVerticalLayout(float baseHeight, bool columnBreak, const std::function<float(cocos2d::Node*, float, float)>& fnPositionX, bool applyChildren)
{
	cocos2d::Vec2 layoutAnchor = this->getAnchorPoint();
	auto children = this->getChildren();

	int sign = 1;
	float paddingY = m_PaddingBottom;
	float bottomBoundaryOfLayout = -layoutAnchor.y * baseHeight;

	if (m_VerticalDirection == VerticalDirection::TopToBottom)
	{
		sign = -1;
		bottomBoundaryOfLayout = (1 - layoutAnchor.y) * baseHeight;
		paddingY = m_PaddingTop;
	}

	float nextY = bottomBoundaryOfLayout + sign * paddingY - sign * m_SpacingY;
	float columnMaxWidth = 0;
	float tempMaxWidth = 0;
	float secondMaxWidth = 0;

	float column = 0;
	float containerResizeBoundary = 0;

	float maxWidthChildAnchorX = 0;
	int activeChildCount = 0;
	for (auto& child : children)
	{
#ifdef CC_PLATFORM_PC
		// AABB is an internal node for drawing the bounding box around a node
		if (child->getName() == "##AABB##")
		{
			continue;
		}
#endif

		if (child->isVisible())
		{
			activeChildCount++;
		}
	}

	float newChildHeight = m_CellSize.height;
	if (m_LayoutType != LayoutType::Grid && m_ResizeMode == ResizeMode::Children)
	{
		newChildHeight = (baseHeight - (m_PaddingTop + m_PaddingBottom) - (activeChildCount - 1) * m_SpacingY) / activeChildCount;
	}

	for (auto& child : children)
	{
#ifdef CC_PLATFORM_PC
		// AABB is an internal node for drawing the bounding box around a node
		if (child->getName() == "##AABB##")
		{
			continue;
		}
#endif

		if (!child->isVisible())
		{
			continue;
		}

		float childScaleX = this->getUsedScaleValue(child->getScaleX());
		float childScaleY = this->getUsedScaleValue(child->getScaleY());

		// For resizing children
		if (m_ResizeMode == ResizeMode::Children)
		{
			float width = child->getContentSize().width;
			float height = newChildHeight / childScaleY;

			if (m_LayoutType == LayoutType::Grid)
			{
				width = m_CellSize.width / childScaleX;
			}

			child->setContentSize(cocos2d::Size(width, height));
		}

		float anchorY = child->getAnchorPoint().y;
		float childBoundingBoxWidth = child->getContentSize().width * childScaleX;
		float childBoundingBoxHeight = child->getContentSize().height * childScaleY;

		if (secondMaxWidth > tempMaxWidth)
		{
			tempMaxWidth = secondMaxWidth;
		}

		if (childBoundingBoxWidth >= tempMaxWidth)
		{
			secondMaxWidth = tempMaxWidth;
			tempMaxWidth = childBoundingBoxWidth;
			maxWidthChildAnchorX = child->getAnchorPoint().x;
		}

		if (m_VerticalDirection == VerticalDirection::TopToBottom)
		{
			anchorY = 1 - child->getAnchorPoint().y;
		}

		nextY = nextY + sign * anchorY * childBoundingBoxHeight + sign * m_SpacingY;
		float topBoundaryOfChild = sign * (1 - anchorY) * childBoundingBoxHeight;

		if (columnBreak)
		{
			float columnBreakBoundary = nextY + topBoundaryOfChild + sign * (sign > 0 ? m_PaddingTop : m_PaddingBottom);
			float bottomToTopColumnBreak = m_VerticalDirection == VerticalDirection::BottomToTop && columnBreakBoundary > (1 - layoutAnchor.y) * baseHeight;
			float topToBottomColumnBreak = m_VerticalDirection == VerticalDirection::TopToBottom && columnBreakBoundary < -layoutAnchor.y * baseHeight;

			if (bottomToTopColumnBreak || topToBottomColumnBreak)
			{
				if (childBoundingBoxWidth >= tempMaxWidth)
				{
					if (secondMaxWidth == 0)
					{
						secondMaxWidth = tempMaxWidth;
					}

					columnMaxWidth += secondMaxWidth;
					secondMaxWidth = tempMaxWidth;
				}
				else
				{
					columnMaxWidth += tempMaxWidth;
					secondMaxWidth = childBoundingBoxWidth;
					tempMaxWidth = 0;
				}

				nextY = bottomBoundaryOfLayout + sign * (paddingY + anchorY * childBoundingBoxHeight);
				column++;
			}
		}

		float finalPositionX = fnPositionX(child, columnMaxWidth, column);
		if (baseHeight >= (childBoundingBoxHeight + (m_PaddingTop + m_PaddingBottom)))
		{
			if (applyChildren)
			{
				child->setPosition(cocos2d::Vec2(finalPositionX, nextY));
			}
		}

		int signX = 1;
		float tempFinalPositionX;
		//when the item is the last column break item, the tempMaxWidth will be 0.
		float rightMarign = (tempMaxWidth == 0) ? childBoundingBoxWidth : tempMaxWidth;

		if (m_HorizontalDirection == HorizontalDirection::RightToLeft)
		{
			signX = -1;
			containerResizeBoundary = containerResizeBoundary || this->getContentSize().width;
			tempFinalPositionX = finalPositionX + signX * (rightMarign * maxWidthChildAnchorX + m_PaddingLeft);
			if (tempFinalPositionX < containerResizeBoundary)
			{
				containerResizeBoundary = tempFinalPositionX;
			}
		}
		else
		{
			containerResizeBoundary = containerResizeBoundary || -this->getContentSize().width;
			tempFinalPositionX = finalPositionX + signX * (rightMarign * maxWidthChildAnchorX + m_PaddingRight);
			if (tempFinalPositionX > containerResizeBoundary)
			{
				containerResizeBoundary = tempFinalPositionX;
			}
		}

		nextY += topBoundaryOfChild;
	}

	return containerResizeBoundary;
}

void Layout::doLayoutGridAxisVertical(cocos2d::Vec2 layoutAnchor, cocos2d::Size layoutSize)
{
	float baseHeight = layoutSize.height;

	int sign = 1;
	float leftBoundaryOfLayout = -layoutAnchor.x * layoutSize.width;
	float paddingX = m_PaddingLeft;

	if (m_HorizontalDirection == HorizontalDirection::RightToLeft)
	{
		sign = -1;
		leftBoundaryOfLayout = (1 - layoutAnchor.x) * layoutSize.width;
		paddingX = m_PaddingRight;
	}

	auto fnPositionX = [&](cocos2d::Node* child, float leftOffset, float column) {
		return leftBoundaryOfLayout + sign * (leftOffset + child->getAnchorPoint().x * child->getContentSize().width * this->getUsedScaleValue(child->getScaleX()) + paddingX + column * this->m_SpacingX);
	};

	float newWidth = 0;
	if (m_ResizeMode == ResizeMode::Container)
	{
		float boundary = this->doVerticalLayout(baseHeight, true, fnPositionX, false);

		newWidth = leftBoundaryOfLayout - boundary;
		if (newWidth < 0)
		{
			newWidth *= -1;
		}

		leftBoundaryOfLayout = -layoutAnchor.x * newWidth;

		if (m_HorizontalDirection == HorizontalDirection::RightToLeft)
		{
			sign = -1;
			leftBoundaryOfLayout = (1 - layoutAnchor.x) * newWidth;
		}
	}

	this->doVerticalLayout(baseHeight, true, fnPositionX, true);

	if (m_ResizeMode == ResizeMode::Container)
	{
		Node::setContentSize(cocos2d::Size(newWidth, baseHeight));
	}
}

void Layout::doGridLayout()
{
	if (m_StartAxis == AxisDirection::Horizontal)
	{
		this->doLayoutGridAxisHorizontal(this->getAnchorPoint(), this->getContentSize());
	}
	else if (m_StartAxis == AxisDirection::Vertical)
	{
		this->doLayoutGridAxisVertical(this->getAnchorPoint(), this->getContentSize());
	}
}

void Layout::updateLayout()
{
	if (m_LayoutDirty && this->getChildrenCount() > 0)
	{
		this->doLayout();
		this->adjustPosition();
		m_LayoutDirty = false;
	}
}

void Layout::adjustPosition()
{
	const cocos2d::Node* parent = this->getParent();
	if (parent)
	{
		const auto p_ap = parent->getAnchorPoint();
		const auto p_cs = parent->getContentSize();

		const auto offset = cocos2d::Vec2(p_ap.x * p_cs.width, p_ap.y * p_cs.height);
		const auto new_pos = this->getCreatorPosition() + offset;

		//		Node::setPosition(new_pos);

		for (auto& child : this->getChildren())
		{
#ifdef CC_PLATFORM_PC
			// AABB is an internal node for drawing the bounding box around a node
			if (child->getName() == "##AABB##")
			{
				continue;
			}
#endif
			const auto p_ap = this->getAnchorPoint();
			const auto p_cs = this->getContentSize();

			const auto offset = cocos2d::Vec2(p_ap.x * p_cs.width, p_ap.y * p_cs.height);
			const auto new_pos = child->getPosition() + offset;

			child->setPosition(new_pos);
		}
	}

	//	if (m_ScrollView)
	//	{
	//		m_ScrollView->setInnerContainerSize(cocos2d::Size(m_ScrollView->getInnerContainerSize().width, _contentSize.height));
	//		m_ScrollView->scrollToTop(0.0f, false);
	//	}
}

void Layout::showChild(cocos2d::Node* child)
{
	if (child)
	{
		child->setVisible(true);
		this->markLayoutDirty();
		this->updateLayout();
	}
}

void Layout::hideChild(cocos2d::Node* child)
{
	if (child)
	{
		child->setVisible(false);
		this->markLayoutDirty();
		this->updateLayout();
	}
}

void Layout::visit(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags)
{
	// quick return if not visible. children won't be drawn.
	if (!_visible)
	{
		return;
	}

	if (m_ScrollView)
	{
		uint32_t flags = processParentFlags(parentTransform, parentFlags);

		// IMPORTANT:
		// To ease the migration to v3.0, we still support the Mat4 stack,
		// but it is deprecated and your code should not rely on it
		_director->pushMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
		_director->loadMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

		bool visibleByCamera = isVisitableByVisitingCamera();
		int i = 0;

		int skippedChildren = 0;
		int drawnChildren = 0;

		m_StartIndex = -1;
		m_EndIndex = -1;

		if (!_children.empty())
		{
			this->sortAllChildren();
			cocos2d::Rect bounds = m_ScrollView->getContainerBounds();

			// draw children zOrder < 0
			for (auto size = _children.size(); i < size; ++i)
			{
				// TODO: Draw only visible nodes
				auto node = _children.at(i);

				// float childScaleX = this->getUsedScaleValue(node->getScaleX());
				float childScaleY = this->getUsedScaleValue(node->getScaleY());
				float childHeight = node->getContentSize().height * childScaleY;

				//				if (m_ScrollView->movedUpwards())
				//				{
				// Get the top edge of this child
				float topEdge = (1.0f - node->getAnchorPoint().y) * childHeight + node->getPosition().y;
				topEdge = (_anchorPoint.y * _contentSize.height) - topEdge;

				float bottomEdge = node->getPosition().y - (node->getAnchorPoint().y) * childHeight;
				bottomEdge = (_anchorPoint.y * _contentSize.height) - bottomEdge;

				// Check if y is in bounds
				if ((topEdge >= bounds.origin.y && topEdge <= bounds.size.height) || (bottomEdge >= bounds.origin.y && bottomEdge <= bounds.size.height))
				//				if (1)
				{
					m_StartIndex = m_StartIndex == -1 ? i : m_StartIndex;

					if (node /* && node->getLocalZOrder() < 0*/)
						node->visit(renderer, _modelViewTransform, flags);
					else
						break;

					drawnChildren++;
				}
				else
				{
					m_EndIndex = m_EndIndex == -1 ? i - 1 : m_EndIndex;
					skippedChildren++;
				}
				//				}
			}

			// self draw
			if (visibleByCamera)
				this->draw(renderer, _modelViewTransform, flags);

			for (auto it = _children.cbegin() + i, itCend = _children.cend(); it != itCend; ++it)
				(*it)->visit(renderer, _modelViewTransform, flags);
		}
		else if (visibleByCamera)
		{
			this->draw(renderer, _modelViewTransform, flags);
		}

		_director->popMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

		// FIX ME: Why need to set _orderOfArrival to 0??
		// Please refer to https://github.com/cocos2d/cocos2d-x/pull/6920
		// reset for next frame
		// _orderOfArrival = 0;

		return;
	}

	cocos2d::Node::visit(renderer, parentTransform, parentFlags);
}

NS_CCR_END
