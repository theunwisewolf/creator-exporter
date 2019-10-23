#include "ParticleSystem.h"

NS_CCR_BEGIN

ParticleSystem* ParticleSystem::create(const std::string& filename)
{
	auto* ret = new (std::nothrow) ParticleSystem();
	if (ret && ret->initWithFile(filename))
	{
		ret->autorelease();
		return ret;
	}

	CC_SAFE_DELETE(ret);
	return ret;
}

void ParticleSystem::setVisible(bool visible)
{
	bool resetSystem = (visible && _visible != visible);
	cocos2d::ParticleSystemQuad::setVisible(visible);

	// ParticleSystem is already visible
	if (resetSystem)
	{
		this->resetSystem();
	}
}

NS_CCR_END
