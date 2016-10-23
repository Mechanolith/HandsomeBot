#include "aStarNode.h"

aStarNode::aStarNode(kf::Vector2 loc, int mapWidth, int mapHeight)
{
	parentID = -1;
	hVal = 0;
	gVal = 0;
	fVal = 0;
	location = loc;
	state = NodeState::e_none;

	edgesMask = 15; //0000 1111

	//Find where there is no adjacent Tiles.
	if (location.x == 0)
	{
		edgesMask &= ~aStarNode::NodeEdges::e_left;
	}
	else if (location.x == (mapWidth - 1))
	{
		edgesMask &= ~aStarNode::NodeEdges::e_right;
	}

	if (location.y == 0)
	{
		edgesMask &= ~aStarNode::NodeEdges::e_top;
	}
	else if (location.y == (mapHeight - 1))
	{
		edgesMask &= ~aStarNode::NodeEdges::e_bottom;
	}
}


aStarNode::~aStarNode()
{
}