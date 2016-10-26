#include "nodeLink.h"

nodeLink::nodeLink(aStarNode* _partnerNode, int _moveCost)
{
	partnerNode = _partnerNode;
	moveCost = _moveCost;
}


nodeLink::~nodeLink()
{
}
