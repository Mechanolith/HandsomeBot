#ifndef BOT_H
#define BOT_H
#include "bot_interface.h"
#include "kf/kf_random.h"
#include "SeenShot.h"

#ifdef BOT_EXPORTS
#define BOT_API __declspec(dllexport)
#else
#define BOT_API __declspec(dllimport)
#endif

class HandsomeBot:public BotInterface27
{
public:
	HandsomeBot();
	virtual ~HandsomeBot();
	virtual void init(const BotInitialData &initialData, BotAttributes &attrib);
	virtual void update(const BotInput &input, BotOutput27 &output);
	virtual void result(bool won);
	virtual void bulletResult(bool hit);
	float floatMod(float input, float modNum);
	void pickTarget();

	enum BotState
	{
		e_lost,
		e_hunting,
		e_attacking
	};

	enum AimState 
	{
		e_noTarget,
		e_needLead,
		e_haveLead
	};

	kf::Xor128 m_rand;
	BotInitialData m_initialData;
	kf::Vector2 dir;
	kf::Vector2 lastPos;
	kf::Vector2 curPos;
	kf::Vector2 nextPos;
	bool firstFrame;
	BotState botState;
	kf::Vector2 scanDir;
	float scanAngle;
	std::vector<SeenShot> seenShots;
	bool enemySeen;
	float scanDirMod;
	float framesHunting;
	float framesLost;
	float lostAngleMod;
	bool shot;
	kf::Vector2 moveTarget;
	AimState leadState;
	int lastHealth;
};

#endif