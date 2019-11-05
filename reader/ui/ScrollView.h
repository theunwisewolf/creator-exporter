#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "../CreatorReader_generated.h"
#include "../Macros.h"

NS_CCR_BEGIN

class ScrollView : public cocos2d::ui::ScrollView
{
private:
	cocos2d::Vec2 m_OriginShift;
	cocos2d::Vec2 m_LastPosition;
	cocos2d::Rect m_ContainerBounds;

	bool m_MovedUpdwards;
	bool m_MovedDownwards;

public:
	/**
     * Create an empty ScrollView.
     * @return A ScrollView instance.
     */
	static ScrollView* create();

	ScrollView();
	~ScrollView();

	void reInit();
	void setInnerContainerSize(const cocos2d::Size& size);
	inline cocos2d::Rect getContainerBounds() const { return m_ContainerBounds; }
	inline bool movedUpwards() const { return m_MovedUpdwards; }
	inline bool movedDownwards() const { return m_MovedDownwards; }

	void onEnter() override;
	void visit(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags) override;
};

NS_CCR_END
