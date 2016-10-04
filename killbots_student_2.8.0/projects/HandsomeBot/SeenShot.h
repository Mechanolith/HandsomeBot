#pragma once
#include "bot_interface.h"
#include "kf/kf_random.h"
class SeenShot
{
public:
	SeenShot( VisibleThing _thing);
	~SeenShot();

	VisibleThing thing;
	std::vector<kf::Vector2> positions;
};

