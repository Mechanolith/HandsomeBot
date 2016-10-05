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
	//dir.set(m_rand.norm()*2.0 - 1.0, m_rand.norm()*2.0 - 1.0);
	dir.set(1, 0);
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
	output.moveDirection = movePoints[curPoint] - input.position;
	if (output.moveDirection.length() < 2)
	{
		if(curPoint < movePoints.size() - 1)
		{
			curPoint++;
		}
		else 
		{
			pickTarget(input.position);
		}
	}
	
	//If hit, freak out.
	if(input.health < lastHealth)
	{
		pickTarget(input.position);
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
	output.lines.push_back(Line(input.position, moveTarget, 1.0, 1.0, 0.0));
	output.lines.push_back(Line(input.position, lastPos, 1.0, 0.0, 0.0));
	output.lines.push_back(Line(input.position, curPos, 0.0, 1.0, 0.0));
	output.lines.push_back(Line(input.position, nextPos, 0.0, 0.0, 1.0));

	for (int k = 0; k < movePoints.size(); k++) 
	{
		if (k > 0) 
		{
			output.lines.push_back(Line(movePoints[k - 1], movePoints[k], 1.0, 0.0, 1.0));
		}
		else 
		{
			//output.lines.push_back(Line(input.position, movePoints[k], 1.0, 0.0, 1.0));
		}
	}

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
	movePoints.clear();
	curPoint = 1;
	float limiter = 7;

	int x;
	int y;
	do
	{
		x = m_rand() % (m_initialData.mapData.width - 3) + 2;
		y = m_rand() % (m_initialData.mapData.height - 3) + 2;
	} while (m_initialData.mapData.data[y*m_initialData.mapData.width + x].wall);

	moveTarget.set(x + 0.5, y + 0.5);
	
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
}
