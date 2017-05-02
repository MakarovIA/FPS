// By Polyakov Pavel

#include "PracticeProject.h"
#include "BotBlackBoardData.h"

UBotBlackBoardData::UBotBlackBoardData()
    : HealthThreshold(30.f), InfluenceCooling(.95f), OverwatchRange(1000.f), BotLastHealth(-1.f){}

ABot* UBotBlackBoardData::getTarget() 
{
	return VisibleEnemies.Num() == 0 ? nullptr : VisibleEnemies[0];
}