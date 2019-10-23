#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "CreatorReader_generated.h"
#include "Macros.h"

NS_CCR_BEGIN

class ParticleSystem : public cocos2d::ParticleSystemQuad
{
	public:
	static ParticleSystem* create(const std::string& filename);

	virtual void setVisible(bool visible) override;
};

NS_CCR_END
