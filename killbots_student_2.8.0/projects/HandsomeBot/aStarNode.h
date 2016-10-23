#pragma once
#include "bot_interface.h"
#include "kf/kf_random.h"

class aStarNode
{
public:
	aStarNode(kf::Vector2 loc, int mapWidth, int mapHeight);
	~aStarNode();

	enum NodeState
	{
		e_none,
		e_open,
		e_closed
	};

	enum NodeEdges 
	{
		e_top		= 1 << 0,
		e_bottom	= 1 << 1,
		e_left		= 1 << 2,
		e_right		= 1 << 3
	};

	int parentID;
	int hVal;
	int gVal;
	int fVal;
	int edgesMask;
	kf::Vector2 location;
	NodeState state;
};

