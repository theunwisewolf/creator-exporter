#pragma once

#include <cmath>
#include <functional>
#include <vector>

#include "../Macros.h"
#include "cocos2d.h"

NS_CCR_BEGIN

class ScrollView;

enum class LayoutType
{
	None,
	Horizontal,
	Vertical,
	Grid
};

enum class ResizeMode
{
	None,
	Container,
	Children
};

enum class AxisDirection
{
	Horizontal,
	Vertical
};

enum class VerticalDirection
{
	BottomToTop,
	TopToBottom
};

enum class HorizontalDirection
{
	LeftToRight,
	RightToLeft
};

struct LayoutItem
{
	void* Data;
	int Tag;
	cocos2d::Vec2 Scale;
	cocos2d::Vec2 Position;
	cocos2d::Vec2 CreatorPosition;
	cocos2d::Size ContentSize;
	cocos2d::Vec2 AnchorPoint;
};

class Layout : public cocos2d::Node
{
private:
	LayoutType m_LayoutType;
	ResizeMode m_ResizeMode;
	AxisDirection m_StartAxis;
	VerticalDirection m_VerticalDirection;
	HorizontalDirection m_HorizontalDirection;

	float m_PaddingLeft;
	float m_PaddingRight;
	float m_PaddingTop;
	float m_PaddingBottom;

	cocos2d::Size m_CellSize;

	float m_SpacingX;
	float m_SpacingY;

	bool m_AffectedByScale;
	bool m_LayoutDirty = false;

	bool initialized = false;
	cocos2d::Vec2 originalPosition;

	// For Recycling the Scrollview
	creator::ScrollView* m_ScrollView = nullptr;

	float m_ItemTolerableWidth = 0.0f;
	float m_ItemTolerableHeight = 0.0f;
	bool m_RecycleElements = false;

	int m_StartIndex; // Index of the first child being drawn in the recycling scrollview
	int m_EndIndex;	  // Index of the last child being drawn in the recycling scrollview

	std::vector<cocos2d::Node*> m_LayoutItemPrefabs;
	std::vector<LayoutItem> m_LayoutItems;
	std::function<void(cocos2d::Node*, const LayoutItem&)> m_PrefabUpdateCallBack;

private:
	void doLayout();

	float doHorizontalLayout(float baseWidth, bool rowBreak, const std::function<float(cocos2d::Node*, float, float)>& fnPositionY, bool applyChildren);
	float doHorizontalLayout(float baseWidth, bool rowBreak, const std::function<float(const LayoutItem&, float, float)>& fnPositionY, bool applyChildren);
	void doLayoutGridAxisHorizontal(cocos2d::Vec2 layoutAnchor, cocos2d::Size layoutSize);

	float doVerticalLayout(float baseHeight, bool columnBreak, const std::function<float(cocos2d::Node*, float, float)>& fnPositionX, bool applyChildren);
	float doVerticalLayout(float baseHeight, bool columnBreak, const std::function<float(const LayoutItem&, float, float)>& fnPositionX, bool applyChildren);
	void doLayoutGridAxisVertical(cocos2d::Vec2 layoutAnchor, cocos2d::Size layoutSize);

	void doGridLayout();
	void doBasicLayout();

	inline float getUsedScaleValue(float value) { return m_AffectedByScale ? std::abs(value) : 1; }
	float getHorizontalBaseWidth(const cocos2d::Vector<cocos2d::Node*>& children);
	float getVerticalBaseHeight(const cocos2d::Vector<cocos2d::Node*>& children);

public:
	static Layout* create();

	Layout();
	virtual ~Layout();

	bool init() override;
	void onEnter() override;
	void onExit() override;
	void visit(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags) override;
	virtual void update(float dt) override;
	virtual void removeAllChildren() override;

	virtual void addChildNoDirty(cocos2d::Node* node); // Adds a child without marking the layout as dirty; Good when adding multiple elements, so you can mark layout dirty at a later stage and prevent recalculations
	virtual void addChildNoDirty(cocos2d::Node* node, int zOrder);
	virtual void addChild(cocos2d::Node* node) override;
	virtual void addChild(cocos2d::Node* node, int zOrder) override;
	virtual void removeChild(cocos2d::Node* node, bool cleanup = true) override;
	virtual void setContentSize(const cocos2d::Size& size) override;
	virtual void setPosition(const cocos2d::Vec2& position) override;
	virtual void showChild(cocos2d::Node* child);
	virtual void hideChild(cocos2d::Node* child);

	inline void setLayoutType(const LayoutType& type) { m_LayoutType = type; }
	inline void setResizeMode(const ResizeMode& mode) { m_ResizeMode = mode; }
	inline void setStartAxis(const AxisDirection& startAxis) { m_StartAxis = startAxis; }
	inline void setVerticalDirection(const VerticalDirection& direction) { m_VerticalDirection = direction; }
	inline void setHorizontalDirection(const HorizontalDirection& direction) { m_HorizontalDirection = direction; }
	inline void setPaddingLeft(const float& value) { m_PaddingLeft = value; }
	inline void setPaddingRight(const float& value) { m_PaddingRight = value; }
	inline void setPaddingTop(const float& value) { m_PaddingTop = value; }
	inline void setPaddingBottom(const float& value) { m_PaddingBottom = value; }
	inline void setCellSize(const cocos2d::Size& size) { m_CellSize = size; }
	inline void setSpacingX(const float& value) { m_SpacingX = value; }
	inline void setSpacingY(const float& value) { m_SpacingY = value; }
	inline void setAffectedByScale(bool value) { m_AffectedByScale = value; }
	inline void setScrollView(creator::ScrollView* scrollView) { m_ScrollView = scrollView; }

	inline VerticalDirection getVerticalDirection() const { return m_VerticalDirection; }
	inline HorizontalDirection getHorizontalDirection() const { return m_HorizontalDirection; }
	inline LayoutType getLayoutType() const { return m_LayoutType; }
	inline ResizeMode getResizeMode() const { return m_ResizeMode; }
	inline AxisDirection getStartAxis() const { return m_StartAxis; }
	inline float getPaddingLeft() const { return m_PaddingLeft; }
	inline float getPaddingRight() const { return m_PaddingRight; }
	inline float getPaddingTop() const { return m_PaddingTop; }
	inline float getPaddingBottom() const { return m_PaddingBottom; }
	inline cocos2d::Size getCellSize() const { return m_CellSize; }
	inline float getSpacingX() const { return m_SpacingX; }
	inline float getSpacingY() const { return m_SpacingY; }
	inline bool getAffectedByScale() const { return m_AffectedByScale; }

	inline void markLayoutDirty() { m_LayoutDirty = true; }
	inline bool isDirty() const { return m_LayoutDirty; }

	void updateLayout();
	void adjustPosition();

	// For recycling
	inline void setDoRecycle(bool value) noexcept { m_RecycleElements = value; }
	inline bool getDoRecycle() const noexcept { return m_RecycleElements; }

	// Extra width and height that will be added when checking whether an item is in layout's view
	inline void setLayoutItemTolerance(const cocos2d::Size& tolerance) noexcept
	{
		m_ItemTolerableWidth = tolerance.width;
		m_ItemTolerableHeight = tolerance.height;
	}

	void initItems();
	bool isItemInView(const LayoutItem& item);
	LayoutItem& createLayoutItem();																												  // Creates a new LayoutItem with the provided "Data"
	void setCreatePrefabCallback(std::function<cocos2d::Node*()>&& callback, std::size_t maxPrefabs);											  // Called to create a prefab
	inline void setUpdatePrefabCallback(std::function<void(cocos2d::Node*, const LayoutItem&)>&& callback) { m_PrefabUpdateCallBack = callback; } // Called when updating a prefab
};

NS_CCR_END
