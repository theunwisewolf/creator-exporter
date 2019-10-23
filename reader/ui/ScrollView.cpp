#include "ScrollView.h"

NS_CCR_BEGIN

ScrollView* ScrollView::create()
{
	creator::ScrollView* widget = new (std::nothrow) creator::ScrollView();
	if (widget && widget->init())
	{
		widget->autorelease();
		return widget;
	}
	CC_SAFE_DELETE(widget);
	return nullptr;
}

ScrollView::ScrollView() :
	cocos2d::ui::ScrollView()
{
}

ScrollView::~ScrollView()
{
}

void ScrollView::onEnter()
{
	cocos2d::ui::ScrollView::onEnter();
	this->reInit();
}

void ScrollView::reInit()
{
	m_OriginShift = _innerContainer->getContentSize() - _contentSize;
	m_LastPosition = _innerContainer->getPosition();
	m_ContainerBounds.setRect(
		m_LastPosition.x + m_OriginShift.x,
		m_LastPosition.y + m_OriginShift.y,
		m_LastPosition.x + _contentSize.width + m_OriginShift.x,
		m_LastPosition.y + _contentSize.height + m_OriginShift.y);
}

void ScrollView::setInnerContainerSize(const cocos2d::Size& size)
{
	cocos2d::ui::ScrollView::setInnerContainerSize(size);

	this->reInit();
}

void ScrollView::visit(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags)
{
	// Find the scroll direction
	if (_direction == cocos2d::ui::ScrollView::Direction::VERTICAL)
	{
		float deltaY = (_innerContainer->getPositionY() - m_LastPosition.y);
		m_LastPosition = _innerContainer->getPosition();
		m_ContainerBounds.setRect(m_LastPosition.x + m_OriginShift.x, m_LastPosition.y + m_OriginShift.y, m_LastPosition.x + _contentSize.width + m_OriginShift.x, m_LastPosition.y + _contentSize.height + m_OriginShift.y);

		if (deltaY > 0)
		{
			m_MovedUpdwards = true;
			m_MovedDownwards = false;
		}
		else if (deltaY < 0)
		{
			m_MovedUpdwards = false;
			m_MovedDownwards = true;
		}
		else
		{
			m_MovedUpdwards = false;
			m_MovedDownwards = false;
		}
	}
	else
	{
		CCLOG("Horizontal dynamic scrollview not supported");
	}

	cocos2d::ui::ScrollView::visit(renderer, parentTransform, parentFlags);
}

NS_CCR_END
