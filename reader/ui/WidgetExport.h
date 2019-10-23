/**
 * Widget handler for widget components exported from Creator
 *
 * The code was originally taken from DreLaptop's Pull request on the
 * original creator_luacpp_support repo, but his code only partially supported widget's
 * and only supported alignment in 9 directions + no support for stretching was there.
 *
 * This is my improved version which completely supports the widget component!
 *
 * Ref:
 * https://github.com/cocos-creator/engine/blob/master/cocos2d/core/base-ui/CCWidgetManager.js
 *
 * @author AmN
 */

#pragma once

#include "../CreatorReader_generated.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "../Macros.h"
#include "../helpers/Helper.h"

NS_CCR_BEGIN

#define CREATOR_ALIGN_TOP		 (1 << 0)
#define CREATOR_ALIGN_MIDDLE	 (1 << 1)
#define CREATOR_ALIGN_BOTTOM	 (1 << 2)
#define CREATOR_ALIGN_LEFT		 (1 << 3)
#define CREATOR_ALIGN_CENTER	 (1 << 4)
#define CREATOR_ALIGN_RIGHT		 (1 << 5)
#define CREATOR_ALIGN_HORIZONTAL (CREATOR_ALIGN_LEFT | CREATOR_ALIGN_CENTER | CREATOR_ALIGN_RIGHT)
#define CREATOR_ALIGN_VERTICAL	 (CREATOR_ALIGN_TOP | CREATOR_ALIGN_MIDDLE | CREATOR_ALIGN_BOTTOM)

class WidgetAdapter : public cocos2d::Ref
{
	private:
	friend class WidgetManager;

	// Widget properties
	int alignFlags;
	bool isAbsoluteLeft;
	bool isAbsoluteRight;
	bool isAbsoluteTop;
	bool isAbsoluteBottom;
	bool isAbsoluteHorizontalCenter;
	bool isAbsoluteVerticalCenter;

	float top;
	float bottom;
	float left;
	float right;
	float horizontalCenter;
	float verticalCenter;

	// Should the widget be aligned only once?
	bool isAlignOnce;

	// Widget target; Parent of Node
	cocos2d::Node* target;

	// The node that contains the widget component
	cocos2d::Node* node;

	public:
	static WidgetAdapter* create();
	WidgetAdapter();
	virtual ~WidgetAdapter();

	bool init();
	void setIsAlignOnce(bool isAlignOnce);
	inline void setNode(cocos2d::Node* node) { this->node = node; }
	inline void setAlignFlags(int alignFlags) { this->alignFlags = alignFlags; }
	inline void setIsAbsoluteLeft(bool value) { this->isAbsoluteLeft = value; }
	inline void setIsAbsoluteRight(bool value) { this->isAbsoluteRight = value; }
	inline void setIsAbsoluteTop(bool value) { this->isAbsoluteTop = value; }
	inline void setIsAbsoluteBottom(bool value) { this->isAbsoluteBottom = value; }
	inline void setIsAbsoluteHorizontalCenter(bool value) { this->isAbsoluteHorizontalCenter = value; }
	inline void setIsAbsoluteVerticalCenter(bool value) { this->isAbsoluteVerticalCenter = value; }

	inline void setTop(float value) { this->top = value; };
	inline void setBottom(float value) { this->bottom = value; };
	inline void setLeft(float value) { this->left = value; };
	inline void setRight(float value) { this->right = value; };
	inline void setHorizontalCenter(float value) { this->horizontalCenter = value; };
	inline void setVerticalCenter(float value) { this->verticalCenter = value; };

	// TODO: support the align target of a widget component, default target is parent Node
	void setLayoutTarget(cocos2d::Node* layoutTarget);
	void align();

	private:
	void setupLayout();

	cocos2d::Size getReadOnlyNodeSize(cocos2d::Node* target);

	inline bool isStretchedAcrossWidth() { return (this->alignFlags & CREATOR_ALIGN_LEFT) && (this->alignFlags & CREATOR_ALIGN_RIGHT); }
	inline bool isStretchedAcrossHeight() { return (this->alignFlags & CREATOR_ALIGN_TOP) && (this->alignFlags & CREATOR_ALIGN_BOTTOM); }
	inline bool isAlignHorizontalCenter() { return (this->alignFlags & CREATOR_ALIGN_CENTER); }
	inline bool isAlignVerticalCenter() { return (this->alignFlags & CREATOR_ALIGN_MIDDLE); }
	inline bool isAlignLeft() { return (this->alignFlags & CREATOR_ALIGN_LEFT); }
	inline bool isAlignBottom() { return (this->alignFlags & CREATOR_ALIGN_BOTTOM); }
};

class WidgetManager : public cocos2d::Node
{
	public:
	WidgetManager();
	virtual ~WidgetManager();

	virtual void update(float dt) override;

	// Manual alignment
	void forceDoAlign();
	void addWidget(WidgetAdapter* widget);
	void alignNewWidgets();
	void clearWidgets();

	private:
	friend class Reader;

	void setupWidgets();
	void doAlign();

	private:
	bool forceAlignDirty;
	cocos2d::Vector<WidgetAdapter*> widgets;
	cocos2d::Vector<WidgetAdapter*> newWidgets; // Set of widgets that have not yet been setup and aligned yet; All widgets must be aligned at least once
};
NS_CCR_END
