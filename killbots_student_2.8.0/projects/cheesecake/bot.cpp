#include "bot.h"
#include "time.h"

extern "C"
{
	BotInterface27 BOT_API *CreateBot27()
	{
		return new Cheesecake();
	}
}

Cheesecake::Cheesecake()
{
	m_rand(time(0)+(int)this);
}

Cheesecake::~Cheesecake()
{
}

void Cheesecake::init(const BotInitialData &initialData, BotAttributes &attrib)
{
	m_initialData = initialData;
	attrib.health=1.0;
	attrib.motor=1.0;
	attrib.weaponSpeed=1.0;
	attrib.weaponStrength=1.0;
	//dir.set(m_rand.norm()*2.0 - 1.0, m_rand.norm()*2.0 - 1.0);
	dir.set(1, 0);
	m_angle = 0;
	pickTarget();
}

void Cheesecake::update(const BotInput &input, BotOutput27 &output)
{
	output.moveDirection = moveTarget - input.position;
	if (output.moveDirection.length() < 2)
	{
		pickTarget();
	}
	//output.moveDirection.set(1, 0);
	//output.moveDirection.set(m_rand.norm()*2.0-1.0, m_rand.norm()*2.0-1.0);
	output.motor = 1.0;

	bool spotted = false;
	kf::Vector2 enemyPos(0, 0);
	for (int i = 0;i<input.scanResult.size();++i)
	{
		if (input.scanResult[i].type == VisibleThing::e_robot)
		{
			
			spotted = true;
			enemyPos = input.scanResult[i].position;
			break;
		}
	}

	if (spotted)
	{
		output.lookDirection = enemyPos - input.position;
		output.action = BotOutput::shoot;
		m_angle = atan2(output.lookDirection.y, output.lookDirection.x);
	}
	else
	{
		kf::Vector2 look(1, 0);
		look = look.rotate(m_angle);
		output.lookDirection = look;
		m_angle += m_initialData.scanFOV*2.0;
		output.action = BotOutput::scan;
	}


	//output.spriteFrame = (output.spriteFrame+1)%2;
	
	output.text.clear();
	char buf[100];
	sprintf(buf, "Hi! %d", input.health);
	output.text.push_back(TextMsg(buf, input.position - kf::Vector2(0.0f, 1.0f), 0.0f, 0.7f, 1.0f,80));

	output.lines.clear();
	output.lines.push_back(Line(input.position, moveTarget, 1.0, 1.0, 0.0));
	//output.action = BotOutput::scan;
}

void Cheesecake::pickTarget()
{
	int x;
	int y;
	do
	{
		x = m_rand() % (m_initialData.mapData.width - 2) + 1;
		y = m_rand() % (m_initialData.mapData.height - 2) + 1;
	} while (m_initialData.mapData.data[y*m_initialData.mapData.width + x].wall);

	moveTarget.set(x+0.5, y+0.5);
}


void Cheesecake::result(bool won)
{
	
}

void Cheesecake::bulletResult(bool hit)
{

}
