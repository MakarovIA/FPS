// By Polyakov Pavel

#pragma once

#include "VoronoiNavData.h"
#include "AIPlayerState.h"
#include "Bot.h"

#include "AIController.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"

#include "BotBlackBoardData.generated.h"

class UBTService_CollectData;

/** Tactics related online properties that Voronoi Faces have */
struct PRACTICEPROJECT_API FVoronoiOnlinePropeties final
{
    /** Value from [-1; 1] -> [AllyControl; EnemyControl] */
    float Influence;

    /** Indicates kill vs death rate in this area */
    float Frag;

    /** Number of enemies watching this area. -1 if information is not avaliable */
    int32 EnemyOverwatch;

    FVoronoiOnlinePropeties()
        : Influence(0)
        , Frag(0)
        , EnemyOverwatch(-1) {}
};

/** Wrapper around data AI uses */
UCLASS(NotBlueprintable)
class PRACTICEPROJECT_API UBotBlackBoardData : public UObject
{
	GENERATED_BODY()
	
    TMap<const FVoronoiFace*, FVoronoiOnlinePropeties> MapData;
    TArray<ABot*> VisibleEnemies, VisibleAllies;

    friend UBTService_CollectData;

public:
    UBotBlackBoardData();

    /** Parameter used to determine whether Hide branch should be activated in behavior tree */
    float HealthThreshold;

    /** Parameter used to determine how fast influence return to neutral */
    float InfluenceCooling;

    /** Parameter used to determine range for overwatch calculation */
    float OverwatchRange;

    /** Current target to move to */ 
	FVector Target;

	/** Current target to move to (follow) */
	ABot* FollowTarget;
	TArray<ABot*> CurrentFollowTarget;
	bool StartFollowing;

	/** Current target to move to (hide) */
	FVector HideTarget;
	bool StartHiding;

	/*for reacting to taking damage from behind*/
	float BotLastHealth;
	FVector BotLastPosition;

	UFUNCTION(BlueprintCallable, Category = "Target")
	ABot* getTarget();
	
    /** Retrieve tactical information */
    FORCEINLINE const TMap<const FVoronoiFace*, FVoronoiOnlinePropeties>& GetMapData() const { return MapData; }

    /** Get array of visible enemies */
    FORCEINLINE const TArray<ABot*>& GetVisibleEnemies() const { return VisibleEnemies; }

    /** Get array of visible allies */
    FORCEINLINE const TArray<ABot*>& GetVisibleAllies() const { return VisibleAllies; }
};
