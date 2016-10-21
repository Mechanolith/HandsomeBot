#ifndef BOT_H
#define BOT_H
#include "bot_interface.h"
#include "kf/kf_random.h"
#include "SeenShot.h"
#include "aStarNode.h"

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
	void pickTarget(kf::Vector2 inputPos);
	void getAStarPath(kf::Vector2 startPos, kf::Vector2 targetPos);
	void setupNodes();
	aStarNode* getNode(kf::Vector2 location);

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

	//General
	kf::Xor128 m_rand;
	BotInitialData m_initialData;
	BotState botState;

	//Scanning/Searching
	float scanAngle;	
	float scanDirMod;
	kf::Vector2 scanDir;
	
	//Enemy Shot Predicting
	std::vector<SeenShot> seenShots;

	//Enemy Tracking/Hunting
	bool enemySeen;
	float framesHunting;
	float framesLost;
	float lostAngleMod;

	//Movement
	int curPoint;
	kf::Vector2 moveTarget;
	std::vector<kf::Vector2> movePoints;
	std::vector<aStarNode*> nodes;
	
	//Aiming
	bool shot;
	bool attemptedLead;
	AimState leadState;
	kf::Vector2 lastPos;
	kf::Vector2 curPos;
	kf::Vector2 nextPos;
	
	//Misc
	bool firstFrame;
	int lastHealth;
};

#endif