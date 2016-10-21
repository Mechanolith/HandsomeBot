#include "aStarNode.h"

aStarNode::aStarNode(kf::Vector2 loc)
{
	parentID = -1;
	hVal = 0;
	gVal = 0;
	fVal = 0;
	location = loc;
	state = NodeState::e_none;
}


aStarNode::~aStarNode()
{
}
