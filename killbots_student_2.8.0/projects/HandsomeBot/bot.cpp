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
	//Basic Attributes
	m_initialData = initialData;
	attrib.health=11.0;
	attrib.motor=0.75;
	attrib.weaponSpeed=0.0;
	attrib.weaponStrength = 2.5;

	//General
	firstFrame = true;
	seenShots.clear();	//Not currently in use anyway.

	//Behaviour
	botState = BotState::e_lost;
	enemySeen = false;
	framesHunting = 0;
	framesLost = 0;

	//Scanning Data
	scanDir.set(1, 0);
	scanAngle = (m_initialData.scanFOV*2.0f) * 4.5f;
	scanDirMod = 1;
	lostAngleMod = 0;
	lookMod = 1;
	
	//Firing
	shot = false;
	leadState = AimState::e_noTarget;
	attemptedLead = false;	
	shotsFired = false;
	aimAngleMod = (m_initialData.scanFOV*2.0f)/13.0f;
	aimDirMod = 1;

	//Set Up move Targets
	moveTargets.push_back(new kf::Vector2(1, m_initialData.mapData.height - 3));
	moveTargets.push_back(new kf::Vector2(2, 7));
	moveTargets.push_back(new kf::Vector2(2, m_initialData.mapData.height - 3));
	moveTargets.push_back(new kf::Vector2(15, m_initialData.mapData.height - 3));
	curTarget = -1;
	
	//Movement/Pathfinding
	//Pathfinding Move Costs (though Diag isn't used anymore.
	adjGVal = 10;
	diagGVal = 14; 
	moveTimer = 0.0f;

	setupNodes();
}

void HandsomeBot::update(const BotInput &input, BotOutput27 &output)
{
	enemySeen = false;
	moveTimer += 0.1f;	//Known time between updates.

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
				shotsFired = 0;
				enemySeen = true;
				lastPos = curPos;
				curPos = input.scanResult[i].position;
				
				/*
				if (leadState == AimState::e_needLead)
				{
					nextPos = curPos + (curPos - lastPos);
					kf::Vector2 aimVec = nextPos - input.position;
					float hitTime = aimVec.length() / input.bulletSpeed;
					nextPos += (nextPos - curPos) * hitTime;

					leadState = AimState::e_haveLead;
				}
				*/
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
		//scanDir = scanDir.rotate(scanAngle);
		scanDir = output.moveDirection * lookMod;
		output.lookDirection = scanDir;
		output.action = BotOutput::scan;
		framesLost++;
		lookMod *= -1;

		/*
		if (framesLost > 3) 
		{
			scanDir = scanDir.rotate(m_initialData.scanFOV*2.0f);
			framesLost = 0;
		}
		*/
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
		if (shotsFired < 10) 
		{
			kf::Vector2 aimDir;
			aimDir = curPos - input.position;
			aimDir = aimDir.rotate(aimAngleMod * (shotsFired % 5) * aimDirMod);
			output.lookDirection = aimDir;
			aimDirMod *= -1;
			shotsFired++;

			output.action = BotOutput::shoot;
		}
		else 
		{
			shotsFired = 0;
			botState = BotState::e_lost;
		}
		/*
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
		*/
		break;

	default:
		break;
	}

	//Determine Movement Direction
	kf::Vector2 offsetVec;
	offsetVec.set(0.5, 0.5);
	
	output.moveDirection = (targetPtr->location - input.position) + offsetVec;

	//Determine if we've reached our current node (or we're stuck).
	if (output.moveDirection.length() < 1 || moveTimer >= 1.0f)
	{
		if (targetPtr->parentID > -1 && moveTimer < 1.0f)
		{
			prevTargetPtr = targetPtr;
			targetPtr = nodes[targetPtr->parentID];
			output.moveDirection = (targetPtr->location - input.position) + offsetVec;
			//output.moveDirection -= input.velocity;

			kf::Vector2 normalDir = output.moveDirection;
			normalDir.normalise();
			kf::Vector2 normalVel = input.velocity;
			normalVel.normalise();
			float angleBetween = acos(normalDir.dot(normalVel)) * 180.0f/3.141592f;

			if (angleBetween > 15.0f)
			{
				output.moveDirection -= input.velocity * 0.5f;
			}
		}
		else 
		{
			pickTarget(input.position);
		}

		output.motor = 1.0f;
		moveTimer = 0;
	}
	else 
	{
		output.motor = 1.0f;
	}
	
	//If hit, freak out.
	/*
	if(input.health < lastHealth)
	{
		pickTarget(input.position);
	}
	*/

	//output.spriteFrame = (output.spriteFrame+1)%2;

	//Debug Stuff
	output.text.clear();
	//char buf[100];
	//sprintf(buf, "%d", scanAngle);
	//output.text.push_back(TextMsg(buf, input.position - kf::Vector2(0.0f, 1.0f), 0.0f, 0.7f, 1.0f,80));
	std::string testOutput = "HP: " + std::to_string(input.health + 1600);
	//output.text.push_back(TextMsg(testOutput, input.position - kf::Vector2(0.0f, 1.0f), 0.0f, 0.7f, 1.0f, 80));

	output.lines.clear();
	output.lines.push_back(Line(input.position, targetPtr->location + offsetVec, 1.0, 1.0, 0.0));
	//output.lines.push_back(Line(input.position, lastPos, 1.0, 0.0, 0.0));
	//output.lines.push_back(Line(input.position, curPos, 0.0, 1.0, 0.0));
	//output.lines.push_back(Line(input.position, nextPos, 0.0, 0.0, 1.0));

	kf::Vector2 pos1;
	kf::Vector2 pos2;
	pos2.set(1, 1);
	pos1.set(2, 2);

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
	curTarget++;
	curTarget = curTarget % 4;

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

	getAStarPath(*moveTargets[curTarget], inputPos);

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
	int xPos = startPos.x;
	int yPos = startPos.y;
	startPos.set(xPos, yPos);//floorf(startPos.x), roundf(startPos.y));
	/*
	if (m_initialData.mapData.data[locationToID(startPos)].wall)
	{

	}
	*/
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
		}

		if (nodes[checkNode]->edgesMask & aStarNode::NodeEdges::e_bottom)
		{
			targetLocation.set(nodeX, nodeY + 1);
			targetID = locationToID(targetLocation);
			setNodeValues(nodes[targetID], nodes[checkNode], targetNode);
		}

		if (nodes[checkNode]->edgesMask & aStarNode::NodeEdges::e_left)
		{
			targetLocation.set(nodeX - 1, nodeY);
			targetID = locationToID(targetLocation);
			setNodeValues(nodes[targetID], nodes[checkNode], targetNode);
		}

		if (nodes[checkNode]->edgesMask & aStarNode::NodeEdges::e_right)
		{
			targetLocation.set(nodeX + 1, nodeY);
			targetID = locationToID(targetLocation);
			setNodeValues(nodes[targetID], nodes[checkNode], targetNode);
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
	prevTargetPtr = nodes[checkNode];
}

//This function should definitely be in the actual class. Silly...
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
