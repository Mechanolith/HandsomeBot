#pragma once
#include "bot_interface.h"
#include "kf/kf_random.h"

class aStarNode
{
public:
	aStarNode(kf::Vector2 loc);
	~aStarNode();

	enum NodeState
	{
		e_none,
		e_open,
		e_closed
	};

	int parentID;
	int hVal;
	int gVal;
	int fVal;
	kf::Vector2 location;
	NodeState state;

};

