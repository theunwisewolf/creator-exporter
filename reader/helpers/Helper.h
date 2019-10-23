#pragma once

#include "cocos2d.h"

// This functions shift the origin from the bottom left to the center of the screen
static void shiftOrigin(cocos2d::Node* node)
{
	const cocos2d::Node* parent = node->getParent();

	// only adjust position if there is a parent, and the parent is no the root scene
	if (parent && dynamic_cast<const cocos2d::Scene*>(parent) == nullptr)
	{
		const auto p_ap = parent->getAnchorPoint();
		const auto p_cs = parent->getContentSize();

		const auto offset = cocos2d::Vec2(p_ap.x * p_cs.width, p_ap.y * p_cs.height);
		const auto new_pos = node->getCreatorPosition() + offset;
		//const auto new_pos = (node->getHasWidget() ? : node->getPosition() : node->getCreatorPosition()) + offset;

		node->setPosition(new_pos);
	}
}

static inline void shiftOriginRecursively(cocos2d::Node* root)
{
	shiftOrigin(root);

	auto children = root->getChildren();
	for (auto child : children)
	{
		shiftOriginRecursively(child);
	}
}
