#include "bot.h"
#include "time.h"
#include "stdafx.h"
#include <string>

extern "C"
{
	BotInterface27 BOT_API *CreateBot27()
	{
		return new HandsomeBot();
	}
}

HandsomeBot::HandsomeBot()
{
	m_rand(time(0)+(int)this);
}

HandsomeBot::~HandsomeBot()
{
}

void HandsomeBot::init(const BotInitialData &initialData, BotAttributes &attrib)
{
	m_initialData = initialData;
	attrib.health=1.0;
	attrib.motor=1.0;
	attrib.weaponSpeed=1.0;
	attrib.weaponStrength = 1.5;
	scanDir.set(1, 0);
	botState = BotState::e_lost;
	firstFrame = true;
	scanAngle = (m_initialData.scanFOV*2.0f) * 4.5f;//((360.0f/(m_initialData.scanFOV*2.0f))/3) * (m_initialData.scanFOV*2.0f);
	seenShots.clear();
	enemySeen = false;
	scanDirMod = 1;
	framesHunting = 0;
	framesLost = 0;
	lostAngleMod = 0;
	shot = false;
	leadState = AimState::e_noTarget;
	attemptedLead = false;

	adjGVal = 10;
	diagGVal = 14;

	/*
	kf::Vector2 initVec;
	initVec.set(-1. - 1);
	invalidVecPtr = new kf::Vector2(initVec);

	movePoints.resize(m_initialData.mapData.width * m_initialData.mapData.height, invalidVecPtr);
	*/
	setupNodes();
}

void HandsomeBot::update(const BotInput &input, BotOutput27 &output)
{
	enemySeen = false;

	if (firstFrame) 
	{
		pickTarget(input.position);
		lastHealth = input.health;
		firstFrame = false;
	}

	//Check what you've seen. Respond accordingly.
		for (int i = 0; i < input.scanResult.size(); i++)
		{
			//Saw enemy bot. Stop searching, prepare for attack.
			if (input.scanResult[i].type == VisibleThing::e_robot)
			{
				botState = BotState::e_attacking;
				lastPos = curPos;
				curPos = input.scanResult[i].position;
				enemySeen = true;

				if (leadState == AimState::e_needLead)
				{
					nextPos = curPos + (curPos - lastPos);
					kf::Vector2 aimVec = nextPos - input.position;
					float hitTime = aimVec.length() / input.bulletSpeed;
					nextPos += (nextPos - curPos) * hitTime;

					leadState = AimState::e_haveLead;
				}
				break;
			}
			//Saw enemy bullet, store it so we can track it.
			else if (input.scanResult[i].type == VisibleThing::e_bullet)
			{
				bool seen = false;

				for (int j = 0; j < seenShots.size(); j++)
				{
					if (seenShots[j].thing.name == input.scanResult[i].name)
					{
						seen = true;
						seenShots[j].positions.push_back(input.scanResult[i].position);
					}
				}

				if (!seen)
				{
					seenShots.push_back(SeenShot::SeenShot(input.scanResult[i]));
				}
				break;
			}
		}

	//Adjust behaviour based on current knowledge of enemy location.
	switch (botState)
	{
	//If we've got no idea where the opponent is, start looking around everywhere.
	case BotState::e_lost:
		//Determine Scan direction
		scanDir = scanDir.rotate(scanAngle);
		output.lookDirection = scanDir;
		output.action = BotOutput::scan;
		framesLost++;

		if (framesLost > 3) 
		{
			scanDir = scanDir.rotate(m_initialData.scanFOV*2.0f);
			framesLost = 0;
		}	
		break;

	case BotState::e_hunting:
		scanDir = scanDir.rotate(m_initialData.scanFOV *2.0f * scanDirMod * framesHunting);
		output.lookDirection = scanDir;
		output.action = BotOutput::scan;
		scanDirMod *= -1;
		framesHunting++;

		if (framesHunting > 40) 
		{
			framesLost = 0;
			botState = BotState::e_lost;
		}
		break;

	case BotState::e_attacking:
		if (leadState == AimState::e_noTarget) 
		{
			output.action = BotOutput::scan;
			leadState = AimState::e_needLead;

			if (attemptedLead) 
			{
				framesHunting = 1;
				botState = BotState::e_hunting;
				attemptedLead = false;
			}
			else 
			{
				attemptedLead = true;
			}
		}

		if (!shot && leadState == AimState::e_haveLead) 
		{
			output.lookDirection = nextPos - input.position;
			output.action = BotOutput::shoot;
			shot = true;
		}
		else if (shot)
		{
			output.action = BotOutput::scan;
			leadState = AimState::e_needLead;
			shot = false;
			framesHunting = 1;
			botState = BotState::e_hunting;
		}
		break;

	default:
		break;
	}

	//Determine Movement Direction
	bool endOfPath = false;
	kf::Vector2 offsetVec;
	offsetVec.set(0.5, 0.5);
	//if (movePoints.size() > 0) 
	//{
		//if (movePoints[curPoint] != invalidVecPtr)
		//{
			//output.moveDirection = *movePoints[curPoint] - input.position;
			output.moveDirection = (targetPtr->location - input.position) + offsetVec;
		//}
		//else 
		//{
//			endOfPath = true;
		//}
	//}

	if (output.moveDirection.length() < 2 || endOfPath)
	{
		if (targetPtr->parentID > -1)
		{
			targetPtr = nodes[targetPtr->parentID];
		}
		else 
		{
			pickTarget(input.position);
		}
	}
	
	//If hit, freak out.
	if(input.health < lastHealth)
	{
		//pickTarget(input.position);
	}

	output.motor = 1.0;

	//output.spriteFrame = (output.spriteFrame+1)%2;

	//Debug Stuff
	output.text.clear();
	//char buf[100];
	//sprintf(buf, "%d", scanAngle);
	//output.text.push_back(TextMsg(buf, input.position - kf::Vector2(0.0f, 1.0f), 0.0f, 0.7f, 1.0f,80));
	std::string testOutput = "HP: " + std::to_string(input.health + 1600);
	output.text.push_back(TextMsg(testOutput, input.position - kf::Vector2(0.0f, 1.0f), 0.0f, 0.7f, 1.0f, 80));

	output.lines.clear();
	output.lines.push_back(Line(input.position, targetPtr->location, 1.0, 1.0, 0.0));
	output.lines.push_back(Line(input.position, lastPos, 1.0, 0.0, 0.0));
	output.lines.push_back(Line(input.position, curPos, 0.0, 1.0, 0.0));
	output.lines.push_back(Line(input.position, nextPos, 0.0, 0.0, 1.0));

	kf::Vector2 pos1;
	kf::Vector2 pos2;
	pos2.set(1, 1);
	pos1.set(2, 2);

	/*
	for (int k = 0; k < movePoints.size(); k++) 
	{
		if (k > 0 && movePoints[k] != invalidVecPtr) 
		{
			output.lines.push_back(Line(*movePoints[k - 1], *movePoints[k], 1.0, 0.0, 1.0));
		}
		else 
		{
			output.lines.push_back(Line(input.position, *movePoints[k], 1.0, 0.0, 1.0));
		}
	}
	*/

	aStarNode* drawNode = targetPtr;

	do 
	{
		if (drawNode->parentID > -1) 
		{
			output.lines.push_back(Line(drawNode->location + offsetVec, nodes[drawNode->parentID]->location + offsetVec, 1.0, 0.0, 1.0));
			drawNode = nodes[drawNode->parentID];
		}
	} while (drawNode->parentID > -1);

	lastHealth = input.health;
}

void HandsomeBot::result(bool won)
{

}

void HandsomeBot::bulletResult(bool hit)
{

}

float HandsomeBot::floatMod(float input, float modNum)
{
	while (input > modNum)
	{
		input -= modNum;
	}

	return input;
}

void HandsomeBot::pickTarget(kf::Vector2 inputPos)
{
	//Free MovePoint Vector Memory
	/*
	for(int i = 0; i < movePoints.size(); i++)
	{
		if (movePoints[i] != invalidVecPtr) {
			delete movePoints[i];
			movePoints[i] = invalidVecPtr;
		}
	}
	*/

	//movePoints.clear();
	curPoint = 1;
	float limiter = 7;

	int x;
	int y;
	do
	{
		x = m_rand() % (m_initialData.mapData.width - 3) + 2;
		y = m_rand() % (m_initialData.mapData.height - 3) + 2;
	} while (m_initialData.mapData.data[y*m_initialData.mapData.width + x].wall);

	kf::Vector2 targetPos;
	targetPos.set(x, y);

	getAStarPath(targetPos, inputPos);

	//moveTarget.set(x + 0.5, y + 0.5);
	
	//Random Movement On A Line
	/*
	for (int i = 0; i < 10; i++) 
	{
		kf::Vector2 moveVec = moveTarget - inputPos;
		kf::Vector2 nextPoint;
		float multiplier = (i + 1.0f) / 10.0f;
		float randDif = (rand() % 3 - 1) * (moveVec.length()/limiter);
		nextPoint.set((moveVec.x + randDif) * multiplier, (moveVec.y + randDif) * multiplier);
		nextPoint += inputPos;
		movePoints.push_back(nextPoint);
	}
	*/
}

void HandsomeBot::setupNodes()
{
	int mapWidth = m_initialData.mapData.width;

	for (int i = 0; i < (mapWidth * m_initialData.mapData.height); i++)
	{
		kf::Vector2 nodePos;
		nodePos.set((i % mapWidth), ((i - (i % mapWidth)) / mapWidth));
		nodes.push_back(new aStarNode(nodePos, mapWidth, m_initialData.mapData.height));
	}
}

aStarNode* HandsomeBot::getNode(kf::Vector2 location)
{
	return nodes[location.x + (location.y * m_initialData.mapData.height)];
}

int HandsomeBot::locationToID(kf::Vector2 location)
{
	int output = 0;

	output = location.x + (location.y * m_initialData.mapData.height);

	return output;
}

kf::Vector2 HandsomeBot::IDToLocation(int ID)
{
	kf::Vector2 output;

	int mapWidth = m_initialData.mapData.width;
	output.set((ID % mapWidth), ((ID - (ID % mapWidth)) / mapWidth));

	return output;
}

void HandsomeBot::getAStarPath(kf::Vector2 startPos, kf::Vector2 targetPos)
{
	nextToPathTarget = false;
	startPos.set(roundf(startPos.x), roundf(startPos.y));
	targetPos.set(roundf(targetPos.x), roundf(targetPos.y));
	aStarNode* startNode = getNode(startPos);
	aStarNode* targetNode = getNode(targetPos);
	int checkNode = locationToID(startNode->location);

	//Set H values relative to target, Reset general values.
	for (int i = 0; i < nodes.size(); i++)
	{
		nodes[i]->hVal = abs(nodes[i]->location.x - targetPos.x) + abs(nodes[i]->location.y - targetPos.y);
		nodes[i]->state = aStarNode::NodeState::e_none;
		nodes[i]->gVal = 0;
		nodes[i]->parentID = -1;
	}

	do {
		nodes[checkNode]->state = aStarNode::NodeState::e_closed;

		int targetID = -1;
		int nodeX = nodes[checkNode]->location.x;
		int nodeY = nodes[checkNode]->location.y;
		kf::Vector2 targetLocation;

		//If the top exists.
		if (nodes[checkNode]->edgesMask & aStarNode::NodeEdges::e_top)
		{
			targetLocation.set(nodeX, nodeY-1);
			targetID = locationToID(targetLocation);
			setNodeValues(nodes[targetID], nodes[checkNode], targetNode);
			//horizNodeSweep(nodes[checkNode], (nodes[checkNode]->location.y - 1), targetNode);
		}

		if (nodes[checkNode]->edgesMask & aStarNode::NodeEdges::e_bottom)
		{
			targetLocation.set(nodeX, nodeY + 1);
			targetID = locationToID(targetLocation);
			setNodeValues(nodes[targetID], nodes[checkNode], targetNode);
			//horizNodeSweep(nodes[checkNode], (nodes[checkNode]->location.y + 1), targetNode);
		}

		if (nodes[checkNode]->edgesMask & aStarNode::NodeEdges::e_left)
		{
			targetLocation.set(nodeX - 1, nodeY);
			targetID = locationToID(targetLocation);
			setNodeValues(nodes[targetID], nodes[checkNode], targetNode);
			//vertNodeSweep(nodes[checkNode], (nodes[checkNode]->location.x - 1), targetNode);
		}

		if (nodes[checkNode]->edgesMask & aStarNode::NodeEdges::e_right)
		{
			targetLocation.set(nodeX + 1, nodeY);
			targetID = locationToID(targetLocation);
			setNodeValues(nodes[targetID], nodes[checkNode], targetNode);
			//vertNodeSweep(nodes[checkNode], (nodes[checkNode]->location.x + 1), targetNode);
		}

		if (!nextToPathTarget)
		{
			checkNode = findNextNode();
		}
		else
		{
			targetNode->parentID = checkNode;
		}
	} while (!nextToPathTarget);

	//CheckNode is now for tracing the path back.
	checkNode = locationToID(targetNode->location);
	targetPtr = nodes[checkNode];
	
	//bool noMoreData = false;

	//Do
	/*
	for(int k = 0; k < movePoints.size(); k++)
	{
		if(!noMoreData)
		{
			kf::Vector2* vecPtr = new kf::Vector2(nodes[checkNode]->location);
			movePoints[k] = vecPtr;
			checkNode = nodes[checkNode]->parentID;
		}

		if (checkNode == locationToID(startNode->location))
		{
			noMoreData = true;
		}
	} 
	*/
	//while (checkNode != locationToID(startNode->location));
}

void HandsomeBot::horizNodeSweep(aStarNode * currentNode, int yVal, aStarNode * targetNode)
{
	int startVal = 0;
	int endVal = 0;

	if (currentNode->edgesMask & aStarNode::NodeEdges::e_left)
	{
		startVal = locationToID(currentNode->location) - 1;
	}

	if (currentNode->edgesMask & aStarNode::NodeEdges::e_right)
	{
		endVal = locationToID(currentNode->location) + 1;
	}

	for (int i = startVal; i < endVal; i++) 
	{
		setNodeValues(nodes[i], currentNode, targetNode);
	}
}

void HandsomeBot::vertNodeSweep(aStarNode * currentNode, int xVal, aStarNode * targetNode)
{
	kf::Vector2 targetLocation;

	if (currentNode->edgesMask & aStarNode::NodeEdges::e_top)
	{
		targetLocation.set(xVal, (currentNode->location.y - 1));
		int targetID = locationToID(targetLocation);
		setNodeValues(nodes[targetID], currentNode, targetNode);
	}

	if (currentNode->edgesMask & aStarNode::NodeEdges::e_bottom)
	{
		targetLocation.set(xVal, (currentNode->location.y + 1));
		int targetID = locationToID(targetLocation);
		setNodeValues(nodes[targetID], currentNode, targetNode);
	}

	targetLocation.set(xVal, (currentNode->location.y));
	int targetID = locationToID(targetLocation);
	setNodeValues(nodes[targetID], currentNode, targetNode);
}

void HandsomeBot::setNodeValues(aStarNode * nodeToSet, aStarNode * currentNode, aStarNode * targetNode)
{
	if (!nextToPathTarget) {
		//Had to use location to ID because apparently kf::Vector2 doesn't support '=='.
		if (locationToID(targetNode->location) == locationToID(nodeToSet->location))
		{
			nextToPathTarget = true;
		}
		else if(!m_initialData.mapData.data[locationToID(nodeToSet->location)].wall)
		{
			int curGVal = 0;

			if (nodeToSet->location.x != currentNode->location.x && nodeToSet->location.y != currentNode->location.y)
			{
				curGVal = diagGVal;
			}
			else
			{
				curGVal = adjGVal;
			}

			switch (nodeToSet->state) 
			{
			case aStarNode::NodeState::e_none:
				nodeToSet->parentID = locationToID(currentNode->location);
				nodeToSet->state = aStarNode::NodeState::e_open;

				nodeToSet->gVal = currentNode->gVal + curGVal;

				nodeToSet->fVal = nodeToSet->hVal + nodeToSet->gVal;
				break;

			case aStarNode::NodeState::e_open:
				if (currentNode->gVal + curGVal < nodeToSet->gVal)
				{
					nodeToSet->parentID = locationToID(currentNode->location);
					nodeToSet->gVal = currentNode->gVal + curGVal;
					nodeToSet->fVal = nodeToSet->hVal + nodeToSet->gVal;
				}
				break;

			case aStarNode::NodeState::e_closed:
				break;
			}
		}
		else 
		{
			nodeToSet->state = aStarNode::NodeState::e_closed;
		}
	}
}

int HandsomeBot::findNextNode()
{
	int nextNode = 0;
	int currentF = INT_MAX;

	for (int i = 0; i < nodes.size(); i++) 
	{
		if (nodes[i]->state == aStarNode::NodeState::e_open) 
		{
			if (nodes[i]->fVal < currentF) 
			{
				nextNode = i;
				currentF = nodes[i]->fVal;
			}
		}
	}

	return nextNode;
}
